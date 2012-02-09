#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include "stream.h"
#include <assert.h>
#include <algorithm>
extern "C" {
#include "yarn.h"
}

/*
 * Blocks are read/written in blockSize units.
 * usedBlocks == 0              -> stream empty
 * usedBlocks == numberOfBlocks -> stream full
 */

#ifdef LOCK_FREE
typedef volatile int CAS_int;
#endif

struct Stream
{
    char** blocks;
    unsigned int blockSize;
    unsigned int numberOfBlocks;
    
    unsigned int currentReadIndex;
    unsigned int currentWriteIndex;
    
    bool readBlockIsActive;
    bool writeBlockIsActive;
    
#ifndef LOCK_FREE
    lock* activeBlocks;
#else
    CAS_int activeBlocks;
#endif
    
    Stream(unsigned int size, unsigned int numBlocks)
    {
	blocks = new char*[numBlocks];
	for (unsigned int i = 0; i < numBlocks; i++)
	{
	    blocks[i] = new char[size];
	    memset(blocks[i], 0, size);
	}
	numberOfBlocks = numBlocks;
	blockSize = size;
#ifndef LOCK_FREE
	activeBlocks = new_lock(0);
#else
	activeBlocks = 0;
#endif
	currentWriteIndex = 0;
	currentReadIndex = 0;
	readBlockIsActive = false;
	writeBlockIsActive = false;
    }
    
    ~Stream()
    {
#ifndef LOCK_FREE
	free_lock(activeBlocks);
#endif
	for (unsigned int i = 0; i < numberOfBlocks; i++)
	{
	    delete [] blocks[i];
	}
	delete [] blocks;
    }
};

void protected_twist(lock *bolt, enum twist_op op, long val)
{
    possess(bolt);
    twist(bolt, op, val);
}

void protected_wait_for(lock *bolt, enum wait_op op, long val)
{
    possess(bolt);
    wait_for(bolt, op, val);
    release(bolt);
}

#ifdef LOCK_FREE
void spin_until_less(CAS_int* val, int target)
{
    while (*val >= target)
    {
	usleep(10);
	//printf("spin < %d %d\n", *val, target);
    }
}

void spin_until_greater(CAS_int* val, int target)
{
    while (*val <= target)
    {
	usleep(10);
	//printf("spin > %d %d\n", *val, target);
    }
}

void spin_until_eq(CAS_int* val, int target)
{
    while (*val != target)
    {
	usleep(10);
	//printf("spin = %d %d\n", *val, target);
    }
}

void CAS_twist(CAS_int* val, int delta)
{
    while (1)
    {
	int v = *val;
	if (__sync_bool_compare_and_swap(val, v, v + delta))
	{
	    return;
	}
    }
}
#endif

Stream* CreateStream(unsigned int blockSize, unsigned int numberOfBlocks)
{
    return new Stream(blockSize, numberOfBlocks);
}
void DestroyStream(Stream* stream)
{
    delete stream;
}

int gPuts = 0;
int gReads = 0;

void* GetWriteBlock(Stream* stream, unsigned int* blockSize)
{
#ifndef LOCK_FREE
    protected_wait_for(stream->activeBlocks, TO_BE_LESS_THAN, stream->numberOfBlocks);
#else
    spin_until_less(&stream->activeBlocks, stream->numberOfBlocks);
#endif
    assert(stream->writeBlockIsActive == false);
    stream->writeBlockIsActive = true;
    if (blockSize)
    {
	*blockSize = stream->blockSize;
    }
    return stream->blocks[stream->currentWriteIndex];
}

void PutWriteBlock(Stream* stream)
{
    assert(stream->writeBlockIsActive == true);
    stream->currentWriteIndex = (stream->currentWriteIndex + 1) % stream->numberOfBlocks;
    stream->writeBlockIsActive = false;
#ifndef LOCK_FREE
    protected_twist(stream->activeBlocks, BY, 1);
#else
    CAS_twist(&stream->activeBlocks, 1);
#endif
}

void* GetReadBlock(Stream* stream, unsigned int* blockSize)
{
#ifndef LOCK_FREE
    protected_wait_for(stream->activeBlocks, TO_BE_MORE_THAN, 0);
#else
    spin_until_greater(&stream->activeBlocks, 0);
#endif
    assert(stream->readBlockIsActive == false);
    stream->readBlockIsActive = true;
    if (blockSize)
    {
	*blockSize = stream->blockSize;
    }
    return stream->blocks[stream->currentReadIndex];
}

void ReleaseReadBlock(Stream* stream)
{
    assert(stream->readBlockIsActive == true);
    
    stream->currentReadIndex = (stream->currentReadIndex + 1) % stream->numberOfBlocks;
    stream->readBlockIsActive = false;
#ifndef LOCK_FREE
    protected_twist(stream->activeBlocks, BY, -1);
#else
    CAS_twist(&stream->activeBlocks, -1);
#endif
}

void BlockUntilStreamIsEmpty(Stream* stream)
{
#ifndef LOCK_FREE
    protected_wait_for(stream->activeBlocks, TO_BE, 0);
#else
    spin_until_eq(&stream->activeBlocks, 0);
#endif
}

long gMaxBlocksOut = -1;

void rndsleep()
{
    //usleep(random() % 100);
}

// 1Gb in 16k blocks
//#define LOOP_COUNT ((1024 * 1024 * 1024) / (16 * 1024))
#define LOOP_COUNT ((1024 * 1024))
#define BLOCK_COUNT 10000

void producer(void* context)
{
    Stream* s = (Stream*)context;
    
    printf("Producing\n");
    unsigned int blockSize;
    for (int i = 0; i < LOOP_COUNT; i++)
    {
	//printf("%d ", i);
#ifndef LOCK_FREE
	long blockCount = peek_lock(s->activeBlocks);
#else
	long blockCount = s->activeBlocks;
#endif
	rndsleep();
	int* block = (int*)GetWriteBlock(s, &blockSize);
	gMaxBlocksOut = std::max(gMaxBlocksOut, blockCount);
	*block = i;
	rndsleep();
	PutWriteBlock(s);
	gPuts++;
    }
    printf("Done producing\n");
}

void consumer(void* context)
{
    Stream* s = (Stream*)context;
    unsigned int blockSize;
    int nextVal = 0;
    printf("Consuming\n");
    while (nextVal < LOOP_COUNT)
    {
	rndsleep();
	int* block = (int*)GetReadBlock(s, &blockSize);
	if (nextVal != *block)
	{
	    printf("%d %d\n", nextVal, *block);
	    printf("Ri %d Wi %d\n", s->currentReadIndex, s->currentWriteIndex);
	    printf("Ra %d Wa %d\n", s->readBlockIsActive, s->writeBlockIsActive);
	    assert(0);
	}
	nextVal++;
	rndsleep();
	ReleaseReadBlock(s);
	gReads++;
    }
    printf("Done Consuming\n");
}

void stream_test()
{
    printf("Beginning stream test\n");
    Stream* s = CreateStream(1024, BLOCK_COUNT);
    thread* l = launch(consumer, s);
    thread* p = launch(producer, s);
    join(p); 
    join(l);
    printf("Max blocks is %d of %d\n", (int)gMaxBlocksOut, BLOCK_COUNT);
}

#ifdef STREAM_TEST_MAIN
int main(int argc, char** argv)
{
    stream_test();
}
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
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

struct Stream
{
    char** blocks;
    unsigned int blockSize;
    unsigned int numberOfBlocks;
    
    unsigned int currentReadIndex;
    unsigned int currentWriteIndex;
    
    bool readBlockIsActive;
    bool writeBlockIsActive;
    
    lock* activeBlocks;
    
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
	activeBlocks = new_lock(0);
	currentWriteIndex = 0;
	currentReadIndex = 0;
	readBlockIsActive = false;
	writeBlockIsActive = false;
    }
    
    ~Stream()
    {
	free_lock(activeBlocks);
	for (unsigned int i = 0; i < numberOfBlocks; i++)
	{
	    delete [] blocks[i];
	}
	delete [] blocks;
    }
};

void locked_twist(lock *bolt, enum twist_op op, long val)
{
    possess(bolt);
    twist(bolt, op, val);
}

void locked_wait_for(lock *bolt, enum wait_op op, long val)
{
    possess(bolt);
    wait_for(bolt, op, val);
    release(bolt);
}

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
    locked_wait_for(stream->activeBlocks, TO_BE_LESS_THAN, stream->numberOfBlocks);
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
    locked_twist(stream->activeBlocks, BY, 1);
}

void* GetReadBlock(Stream* stream, unsigned int* blockSize)
{
    locked_wait_for(stream->activeBlocks, TO_BE_MORE_THAN, 0);
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
    locked_twist(stream->activeBlocks, BY, -1);
}

void BlockUntilStreamIsEmpty(Stream* stream)
{
    locked_wait_for(stream->activeBlocks, TO_BE, 0);
}

long gMaxBlocksOut = -1;

void rndsleep()
{
    //usleep(random() % 100);
}

// 1Gb in 16k blocks
#define LOOP_COUNT ((1024 * 1024 * 1024) / (16 * 1024))
#define BLOCK_COUNT 10000

void producer(void* context)
{
    Stream* s = (Stream*)context;
    
    printf("Producing\n");
    unsigned int blockSize;
    for (int i = 0; i < LOOP_COUNT; i++)
    {
	//printf("%d ", i);
	long blockCount = peek_lock(s->activeBlocks);
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

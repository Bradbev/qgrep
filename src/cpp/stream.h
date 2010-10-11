#ifndef __STREAM_H
#define __STREAM_H

/*
 * Zerocopy, threadsafe, block based stream.
 * The stream is threadsafe for the scenario of a single producers & a
 * single consumers on different threads.  It does not support
 * multiple threads consuming or producing.
 *
 * This stream is designed to connect a producer and a consumer in a
 * one way fashion.
 * The data source obtains a data block with GetWriteBlock, which will
 * return a pointer to memory that can be written to.  GetWriteBlock
 * will also fill out the size of the returned block in the blockSize
 * argument.  If the stream is full then GetWriteBlock will block the
 * thread until there is room to obtain a block.
 * The data source must signal that it is finished with the active
 * write block by calling PutWriteBlock.  These functions must be
 * called as pairs or the code will assert.
 * 
 * The data sink reads data from the stream by using GetReadBlock,
 * which functions in a similar manner to GetWriteBlock.
 */

struct Stream;

Stream* CreateStream(unsigned int blockSize, unsigned int numberOfBlocks);
void DestroyStream(Stream* stream);

void* GetWriteBlock(Stream* stream, unsigned int* blockSize = 0);
void PutWriteBlock(Stream* stream);

void* GetReadBlock(Stream* stream, unsigned int* blockSize = 0);
void ReleaseReadBlock(Stream* stream);

void BlockUntilStreamIsEmpty(Stream* stream);

#endif

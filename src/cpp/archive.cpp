#include <assert.h>
#include "archive.h"
#include "stream.h"
#include <stdlib.h>
#include <stdio.h>
#include <archive.h>
#include <archive_entry.h>
#include <re2/re2.h>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <set>
#include <string>

struct ltstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) < 0;
  }
};
typedef std::set<const char*, ltstr> StringSet;

StringSet gStaleFiles;
StringSet gDeletedFiles;

using namespace re2;

extern "C" {
#include "yarn.h"
}

// Block handling
struct NamedDataBlock;

NamedDataBlock* CreateNamedDataBlock(const char* name, void* block, unsigned int blockSize);

const char* GetNameFromBlock(NamedDataBlock* block);
void* GetDataFromBlock(NamedDataBlock* block, unsigned int* usableSize);
const char* GetNameFromBlock(NamedDataBlock* block);
void SetUsedDataSize(NamedDataBlock* block, unsigned int usedSize);

int LineCount(const char* input, int len);

struct NamedDataBlock
{
    unsigned int dataStart;
    unsigned int dataSize;
    char data[0];
};

struct RegexMatchState
{
    // Init settings
    matchHitCallback callbackFunction;
    void* context;
    RE2* pattern;
    // Internal use
    const char* filename;
    unsigned int currentLineCount;
};

char* CopyString(const char* s)
{
    int len = strlen(s);
    char* ret = (char*)malloc(len+1);
    memcpy(ret, s, len);
    ret[len] = 0;
    return ret;
}

void InitMatchState(RegexMatchState* state, RE2* pattern, matchHitCallback callbackFunction, void* callbackContext)
{
    state->callbackFunction = callbackFunction;
    state->context = callbackContext;
    state->pattern = pattern;
    state->filename = NULL;
    state->currentLineCount = 0;
}

struct ConsumerThreadContext
{
    Stream* dataStream;
    matchHitCallback callbackFunction;
    RE2* pattern;
};

void MatchInBlock(RegexMatchState* state, NamedDataBlock* block)
{
    unsigned int dataSize;
    const char* data = (const char*)GetDataFromBlock(block, &dataSize);
    int lineCount = 0;
    // Test the whole block for a match, fast rejection
    if (RE2::PartialMatch(StringPiece(data, dataSize), *(state->pattern)))
    {
	const char* lineStart = data;
	// test each line for a match
	while (1)
	{
	    lineCount++;
	    const char* lineEnd = (const char*)memchr(lineStart, '\n', dataSize - (lineStart - data));
	    if (lineEnd == NULL)
	    {
		state->currentLineCount += lineCount;
		return;
	    }
	    if (RE2::PartialMatch(StringPiece(lineStart, lineEnd - lineStart), *(state->pattern)))
	    {
		state->callbackFunction(state->context,
					GetNameFromBlock(block),
					lineCount,
					lineStart,
					lineEnd);
	    }
	    lineStart = lineEnd + 1;
	};
    }
    else
    {
	state->currentLineCount += LineCount(data, dataSize);
    }
}

    
NamedDataBlock* CreateNamedDataBlock(const char* name, void* block, unsigned int blockSize)
{
    name = name ? name : "";
    unsigned int namelen = strlen(name) + 1;
    assert (namelen < blockSize);
    NamedDataBlock* ret = (NamedDataBlock*)block;
    
    strcpy(ret->data, name);
    ret->dataStart = namelen;
    ret->dataSize = blockSize - sizeof(NamedDataBlock) - namelen;
    
    return ret;
}

void* GetDataFromBlock(NamedDataBlock* block, unsigned int* usableSize)
{
    if (usableSize) *usableSize = block->dataSize;
    return &block->data[block->dataStart];
}

const char* GetNameFromBlock(NamedDataBlock* block)
{
    return (const char*)block->data;
}

void SetUsedDataSize(NamedDataBlock* block, unsigned int usedSize)
{
    assert(usedSize <= block->dataSize);
    block->dataSize = usedSize;
}

void* _GetEndOfBlock(NamedDataBlock* block)
{
    return &block->data[block->dataStart + block->dataSize];
}

NamedDataBlock* CopyNamedDataBlock(NamedDataBlock* source)
{
    char* start = (char*)source;
    char* end = (char*)_GetEndOfBlock(source);
    int size = end - start;
    NamedDataBlock* ret = (NamedDataBlock*)malloc(size);
    memcpy(ret, source, size);
    return ret;
}

NamedDataBlock* AppendDataToBlock(NamedDataBlock* block, void* data, unsigned int dataSize)
{   char* start = (char*)block;
    char* end = (char*)_GetEndOfBlock(block);
    int size = end - start;
    int newSize = size + dataSize;
    block = (NamedDataBlock*)realloc(block, newSize);
    memcpy(_GetEndOfBlock(block), data, dataSize);
    block->dataSize += dataSize;
    return block;
};

void namedDataBlockTest()
{
    char buf[101] = {1};
    NamedDataBlock* orig = CreateNamedDataBlock("foo", buf, 100);
    unsigned int i;
    unsigned int size;
    char* d = (char*)GetDataFromBlock(orig, &size);
    for (i = 0; i < size; i++)
    {
	d[i] = i % 255;
    }
    NamedDataBlock* copy = CopyNamedDataBlock(orig);
    char* d2 = (char*)GetDataFromBlock(copy, &size);
    printf("Testing CopyNamedDataBlock\n");
    for (i = 0; i < size; i++)
    {
	if (d2[i] != (char)(i % 255))
	{
	    printf("Fail %d %d %d\n", i, d2[i], i % 255);
	}
    }
    
    char append[50];
    for (int j = 0; j < 50; j++, i++)
    {
	append[j] = i % 255;
    }
    
    copy = AppendDataToBlock(copy, append, 50);
    
    char* d3 = (char*)GetDataFromBlock(copy, &size);
    for (i = 0; i < size; i++)
    {
	if (d3[i] != (char)(i % 255))
	{
	    printf("Fail part 2 %d %d %d\n", i, d3[i] % 255, (char)(i % 255));
	}
    }
}

static void grepFormatHit(void* context, const char* filename, unsigned int lineNumber, const char* lineStart, const char* lineEnd)
{
    printf("%s:%d:", filename, lineNumber);
    while (lineStart < lineEnd)
    {
	putchar(*lineStart);
	lineStart++;
    }
    putchar('\n');
}

void grepThreadFn(void* rawContext)
{
    ConsumerThreadContext* context = (ConsumerThreadContext*)rawContext;
    bool running = true;
    int count = 0;
    
    Stream* s = context->dataStream;
    matchHitCallback callback = context->callbackFunction;
    RE2* pattern = context->pattern;
    
    RegexMatchState state;
    while (running)
    {
	InitMatchState(&state, pattern, callback, NULL);
	count++;
	unsigned int blockSize;
	NamedDataBlock* block = (NamedDataBlock*)GetReadBlock(s, &blockSize);
	if (GetNameFromBlock(block)[0] == 0)
	{
	    ReleaseReadBlock(s);
	    break;
	}
	MatchInBlock(&state, block);
	ReleaseReadBlock(s);
    }
}

#define BLOCK_SIZE (1 * 1024 * 1024)
//#define BLOCK_SIZE (1 * 1024)
#define BLOCK_COUNT 10

int LineCount(const char* input, int len)
{
    const char* i = input;
    int count = 0;
    while (1)
    {
	i = (const char*)memchr(i, '\n', len - (i - input));
	if (i == NULL)
	    return count;
	count++;
	i++;
    };
}

int CountMatchesInBlock(const char* block, int blockSize, RE2& pattern)
{
    int count = 0;
    const char* lineStart = block;
    if (RE2::PartialMatch(StringPiece(block, blockSize), pattern))
    {
	while (1)
	{
	    const char* lineEnd = (const char*)memchr(lineStart, '\n', blockSize - (lineStart - block));
	    if (lineEnd == NULL)
		return count;
	    if (RE2::PartialMatch(StringPiece(lineStart, lineEnd - lineStart), pattern))
	    {
		count++;
	    }
	    lineStart = lineEnd + 1;
	};
    }
    return 0;
}

bool MatchPatternInLine(const StringPiece& line, RE2& pattern)
{
    return RE2::PartialMatch(line, pattern);
}

int ReadDataFromFileOrArchive(struct archive* a, FILE* f, void* dest, int bytesToRead)
{
    if (f)
	return fread(dest, 1, bytesToRead, f);
    if (a)
	return archive_read_data(a, dest, bytesToRead);
    assert(0);
    return 0;
}
/*
Correctness is king:
 - but pathologically large lines or files are unreasonable, ie forget
 about memory constraints in this case
 - Set reasonable sizes (1Mb files)
 - Fallback to a slower single threaded path
  - Copy the write block out
  - Work on the file until in memory
  - wait for stream to clear
 */
int gFallback = 0;
int gFallbackExpands = 0;
void LargeFileFallback(struct archive* a, FILE* f, NamedDataBlock* alreadyReadData, ConsumerThreadContext* context)
{
    //printf("Fallingback!\n");
    gFallback++;
    const static int expandSize = 1 * 1024 * 1024;
    char* readBlock = (char*)malloc(expandSize);
    NamedDataBlock* block = CopyNamedDataBlock(alreadyReadData);
    Stream* s = context->dataStream;
    
    int readSize;
    do
    {
	gFallbackExpands++;
	readSize = ReadDataFromFileOrArchive(a, f, readBlock, expandSize);
	block = AppendDataToBlock(block, readBlock, readSize);
    } while (readSize == expandSize);
    
    free(readBlock);
    
    // Ensure the consumer isn't doing anything
    BlockUntilStreamIsEmpty(s);
    // do match
    RegexMatchState state;
    InitMatchState(&state, context->pattern, context->callbackFunction, NULL);
    MatchInBlock(&state, block);
    free(block);
};

void LoadStaleSets(const char* filename)
{
    char staleName[1024];
    char line[1024];
    sprintf(staleName, "%s.stalefiles", filename);
    FILE* f = fopen(staleName, "r");
    if (f)
    {
	while (fgets(line, 1024, f))
	{
	    // remove trailing \n
	    line[strlen(line)-1] = 0;
	    char* str = CopyString(line);
	    if (str[0] == '-')
		gDeletedFiles.insert(&str[1]);
	    else
		gStaleFiles.insert(str);
	}
	fclose(f);
    }
    printf("Stale files are \n");
    for (StringSet::iterator i = gStaleFiles.begin();
	 i != gStaleFiles.end();
	 ++i)
    {
	printf("%s\n", *i);
    }
    printf("Deleted files are \n");
    for (StringSet::iterator i = gDeletedFiles.begin();
	 i != gDeletedFiles.end();
	 ++i)
    {
	printf("%s\n", *i);
    }
}

void ExecuteSearch(GrepParams* param)
{
  struct archive *a;
  FILE* f = NULL;
  struct archive_entry *entry;
  int r;
  
  LoadStaleSets(param->sourceArchiveName);
  
  ConsumerThreadContext* context = new ConsumerThreadContext();
  Stream* s = CreateStream(param->streamBlockSize, param->streamBlockCount);
  context->dataStream = s;
  context->callbackFunction = param->callbackFunction;
  context->pattern = new RE2(param->searchPattern);
  
  thread* consumer = launch(grepThreadFn, context);
  
  a = archive_read_new();
  archive_read_support_compression_all(a);
  archive_read_support_format_all(a);
  r = archive_read_open_filename(a, param->sourceArchiveName, 10240); // Note 1
  if (r != ARCHIVE_OK)
    exit(1);
  
  unsigned int blockSize;
  unsigned int readSize;
  unsigned int readlen;
  while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
      const char* entryName = archive_entry_pathname(entry);
      // Don't return results from deleted files
      if (gDeletedFiles.count(entryName) > 0)
      {
	  archive_read_data_skip(a); 
	  continue;
      }
      
      void* rawBlock = GetWriteBlock(s, &blockSize);
      NamedDataBlock* block = CreateNamedDataBlock(entryName, rawBlock, blockSize);
      //////
      if (gStaleFiles.count(entryName) > 0)
      {
//	  printf("Searching in %s\n", entryName);
	  f = fopen(entryName, "rb");
	  archive_read_data_skip(a); 
      }
      
      void* data = GetDataFromBlock(block, &readSize);
      readlen = ReadDataFromFileOrArchive(a, f, data, readSize);
      SetUsedDataSize(block, readlen);
      if (readlen == readSize)
      {
	  LargeFileFallback(a, f, block, context);
	  SetUsedDataSize(block, 0);
      }
      
      if (f)
      {
	  fclose(f);
	  f = NULL;
      }
      ///////
      PutWriteBlock(s);
  }
  r = archive_read_finish(a);  // Note 3
  if (r != ARCHIVE_OK)
    exit(1);
  
  void* rawBlock = GetWriteBlock(s, &blockSize);
  NamedDataBlock* endBlock = CreateNamedDataBlock(NULL, rawBlock, blockSize);
  SetUsedDataSize(endBlock, 0);
  PutWriteBlock(s);
  //printf("Waiting on join\n");
  join(consumer);
  
  delete context->pattern;
  delete context;
  DestroyStream(s);
  //printf("gFallback %d gFallbackExpands %d\n", gFallback, gFallbackExpands);
}

struct archive* CreateArchive(const char* archiveName)
{
    struct archive* a = archive_write_new();
    assert(a);
    archive_write_set_compression_gzip(a);
    archive_write_set_format_pax_restricted(a); 
    archive_write_open_file(a, archiveName);
    return a;
}

void AddFileToArchive(struct archive* a, const char* filename)
{
    struct archive_entry *entry;
    struct stat st;
    char buff[8192];
    int len;
    int fd;
    
//    printf("%s\n", filename);
    stat(filename, &st);
    entry = archive_entry_new();
    archive_entry_set_pathname(entry, filename);
    archive_entry_set_size(entry, st.st_size);
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    archive_write_header(a, entry);
    fd = open(filename, O_RDONLY);
    len = read(fd, buff, sizeof(buff));
    while ( len > 0 ) {
	archive_write_data(a, buff, len);
	len = read(fd, buff, sizeof(buff));
    }
    close(fd);
    archive_entry_free(entry);
}

void CloseArchive(struct archive* a)
{
    archive_write_close(a); 
    archive_write_finish(a);
}

void test_ReadFileNames(const char* archiveName)
{
    GrepParams params;
    params.streamBlockCount = BLOCK_COUNT;
    params.streamBlockSize = BLOCK_SIZE;
    params.callbackFunction = grepFormatHit;
    params.sourceArchiveName = archiveName;
    params.searchPattern = "foo";
    ExecuteSearch(&params);
}

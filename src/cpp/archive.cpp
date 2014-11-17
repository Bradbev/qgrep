#include <assert.h>
#include "archive.h"
#include "stream.h"
#include <stdlib.h>
#include <stdio.h>
#include <archive.h>
#include <archive_entry.h>
#include "re2/re2.h"
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <set>
#include <string>
#include "trigram.h"
#include <unistd.h>

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
    
    struct QArchive
    {
        struct archive* a;
        TrigramSplitter* ts;
        const char* archiveFileName;
    };
    
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
    RE2* secondPhasePattern;
    // Internal use
    const char* filename;
    unsigned int currentLineCount;
    bool skipBinary;
};

void InitMatchState(RegexMatchState* state, RE2* pattern, RE2* secondPhasePattern, matchHitCallback callbackFunction, void* callbackContext)
{
    state->callbackFunction = callbackFunction;
    state->context = callbackContext;
    state->pattern = pattern;
    state->secondPhasePattern = secondPhasePattern;
    state->filename = NULL;
    state->currentLineCount = 0;
    state->skipBinary = false;
}

struct ConsumerThreadContext
{
    Stream* dataStream;
    matchHitCallback callbackFunction;
    void* callbackContext;
    RE2* pattern;
    RE2* secondPhasePattern;
};

template <class T, class K>
bool SetContains(T& set, K& key)
{
    return set.count(key) > 0;
}

std::string GetBaseFromFilename(const char* filename)
{
    std::string s(filename);
    int lastDelimiter = s.find_last_of("/\\") + 1;
    return s.substr(0,lastDelimiter);
};

void MatchInBlock(RegexMatchState* state, NamedDataBlock* block)
{
    unsigned int dataSize;
    const char* data = (const char*)GetDataFromBlock(block, &dataSize);
    int lineCount = 0;
    bool scanBlock = true;
    if (state->skipBinary)
    {
        // look for a NULL char, if we find it, assume this is binary & skip
		// This appears to fail when it shouldn't :(
        void* n = memchr(data, 0, dataSize);
        if (n) scanBlock = false;
    }
    // Test the whole block for a match, fast rejection
    if (scanBlock && RE2::PartialMatch(StringPiece(data, dataSize), *(state->pattern)))
    {
        const char* lineStart = data;
        const char* dataEnd = &data[dataSize];
        // test each line for a match
        while (1)
        {
            lineCount++;
            if (lineStart >= dataEnd)
            {
                state->currentLineCount += lineCount;
                return;
            }
            const char* lineEnd = (const char*)memchr(lineStart, '\n', dataSize - (lineStart - data));
            if (lineEnd == NULL)
            {
                lineEnd = dataEnd;
            }
            if (RE2::PartialMatch(StringPiece(lineStart, lineEnd - lineStart), *(state->pattern)))
            {
                bool doCallback = true;
                if (state->secondPhasePattern)
                {
                    char output_line[4096] = {0};
                    int num_chars = snprintf(output_line, 4096, "%s:", GetNameFromBlock(block));
                    int line_len = lineEnd - lineStart;
                    int left = 4096 - num_chars;
                    int to_copy = left < line_len ? left : line_len;
                    memcpy(&output_line[num_chars], lineStart, to_copy);
                    doCallback = RE2::PartialMatch(StringPiece(output_line, num_chars + to_copy), *(state->secondPhasePattern));
                }
        
                if (doCallback)
                {
                    state->callbackFunction(state->context,
                                            GetNameFromBlock(block),
                                            lineCount,
                                            lineStart,
                                            lineEnd);
                }
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

/*
 * Consumes blocks of data and greps them
 */
void grepThreadFn(void* rawContext)
{
    ConsumerThreadContext* context = (ConsumerThreadContext*)rawContext;
    bool running = true;
    int count = 0;
    
    Stream* s = context->dataStream;
    matchHitCallback callback = context->callbackFunction;
    RE2* pattern = context->pattern;
    RE2* secondPhasePattern = context->secondPhasePattern;
    
    RegexMatchState state;
    while (running)
    {
        InitMatchState(&state, pattern, secondPhasePattern, callback, context->callbackContext);
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

int ReadDataFromFileOrArchive(struct QArchive* qa, FILE* f, void* dest, int bytesToRead)
{
    struct archive* a = qa ? qa->a : NULL;
    if (f)
        return fread(dest, 1, bytesToRead, f);
    if (a)
        return archive_read_data(a, dest, bytesToRead);
    assert(0);
    return 0;
}
/*
  #implementationdetail
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
void LargeFileFallback(struct QArchive* qa, FILE* f, NamedDataBlock* alreadyReadData, ConsumerThreadContext* context)
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
        readSize = ReadDataFromFileOrArchive(qa, f, readBlock, expandSize);
        if (readSize < 0) break;
        block = AppendDataToBlock(block, readBlock, readSize);
    } while (readSize == expandSize);
    
    free(readBlock);
    
    // Ensure the consumer isn't doing anything
    BlockUntilStreamIsEmpty(s);
    // do match
    RegexMatchState state;
    InitMatchState(&state, context->pattern, context->secondPhasePattern, context->callbackFunction, context->callbackContext);
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
            char* str = strdup(line);
            if (str[0] == '-')
                gDeletedFiles.insert(&str[1]);
            else
                gStaleFiles.insert(str);
        }
        fclose(f);
    }
    /*
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
    */
}

void ExecuteContentSearch(Stream* dataStream, struct QArchive* cacheQArchive, FILE* file, struct archive_entry* entry, const char* entryName, ConsumerThreadContext* context)
{
    /* Main search kernel */
    unsigned int blockSize;
    unsigned int readSize;
    unsigned int readlen;
    void* rawBlock = GetWriteBlock(dataStream, &blockSize);
    NamedDataBlock* block = CreateNamedDataBlock(entryName, rawBlock, blockSize);
    void* data = GetDataFromBlock(block, &readSize);
    readlen = ReadDataFromFileOrArchive(cacheQArchive, file, data, readSize);
    SetUsedDataSize(block, readlen);
    if (readlen == readSize)
    {
        LargeFileFallback(cacheQArchive, file, block, context);
        SetUsedDataSize(block, 0);
    }
    PutWriteBlock(dataStream);
}

FILE* OpenFile(std::string& baseDirectory, const char* filename)
{
    std::string trueName = baseDirectory;
    trueName.append(filename);
    //printf("Searching in %s\n", trueName.c_str());
    FILE* file = NULL;
    file = fopen(trueName.c_str(), "rb");
    if (!file)
        file = fopen(filename, "rb");
    return file;
}

struct TrigramContext
{
    Stream* dataStream;
    ConsumerThreadContext* context;
    struct QArchive* qa;
    StringSet* handledFileSet;
};

void trigram_callback(void* vcontext, const char* filename)
{
    TrigramContext* context = (TrigramContext*)vcontext;
    
    if (!SetContains(gDeletedFiles, filename))
    {
        FILE *f = fopen(filename, "r");
        if (f)
        {
            context->handledFileSet->insert(strdup(filename));
            ExecuteContentSearch(context->dataStream, context->qa, f, NULL, filename, context->context);
            fclose(f);
        }
    }
}

struct LooseFileSearchContext
{
    ConsumerThreadContext* threadContext;
    thread* consumer;
};

LooseFileSearchContext* CreateSearchContext(GrepParams* param)
{
    LooseFileSearchContext* lfsc = new LooseFileSearchContext();
    ConsumerThreadContext* context = new ConsumerThreadContext();
    context->dataStream = CreateStream(param->streamBlockSize, param->streamBlockCount);
    context->callbackFunction = param->callbackFunction;
    context->callbackContext = param->callbackContext;
    RE2::Options options;
    options.set_case_sensitive(param->caseSensitive);
    options.set_literal(param->regexIsLiteral);
    context->pattern = new RE2(param->searchPattern, options);
    
    lfsc->consumer = launch(grepThreadFn, context);
    lfsc->threadContext = context;
    return lfsc;
}

void SearchInLooseFile(LooseFileSearchContext* lfsc, const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (file)
    {
        Stream* dataStream = lfsc->threadContext->dataStream;
        ExecuteContentSearch(dataStream, NULL, file, NULL, filename, lfsc->threadContext);
        fclose(file);
        file = NULL;
    }
}

void DestroySearchContext(LooseFileSearchContext* lfsc) 
{
    ConsumerThreadContext* context = lfsc->threadContext;
    
    unsigned int blockSize;
    void* rawBlock = GetWriteBlock(context->dataStream, &blockSize);
    NamedDataBlock* endBlock = CreateNamedDataBlock(NULL, rawBlock, blockSize);
    SetUsedDataSize(endBlock, 0);
    PutWriteBlock(context->dataStream);
    //printf("Waiting on join\n");
    join(lfsc->consumer);
  
    DestroyStream(context->dataStream);
    delete context->pattern;
    delete context;
}


/*
 * @bbeveridge
 * ExecuteSearch has gotten very hairy, and should be #refactored.
 * The intended logic goes as such
 * - If it is valid to search using trigram indexing, then:
 *   1) Search using trigrams.  To avoid rehitting stalefiles, add
 *      every searched file to the staleFilesThatHaveBeenSearched set
 *   2) Skip reading the archive entirely
 *   3) Do the same handling for stalefiles & wait for regex streams
 *
 * - Search the archive by:
 *   1) iterating all files in the archive, if a file is deleted, skip
 *      it.  If a file is 'stale' then skip it in the archive & search
 *      the actual file on disk
 *
 * - Directly search any files that have been added to disk & are
 *   therefore not indexed.
 * - Wait for the RE2 threads to complete
 */
void ExecuteSearch(GrepParams* param)
{
    struct archive_entry *entry;
    int r;
    StringSet staleFilesThatHaveBeenSearched;
  
    LoadStaleSets(param->sourceArchiveName);
  
    LooseFileSearchContext* lfsc = CreateSearchContext(param);
    ConsumerThreadContext* context = lfsc->threadContext;
    Stream* dataStream = context->dataStream;

    if (context->pattern->ok() == false)
    {
        printf("FATAL: Primary regex expression has an error : %s\n", context->pattern->error().c_str());
        exit(1);
    }
    if (param->secondPhasePattern)
    {
        RE2::Options nocase;
        nocase.set_case_sensitive(false);
        context->secondPhasePattern = new RE2(param->secondPhasePattern, nocase);
      
        if (context->secondPhasePattern->ok() == false)
        {
            printf("FATAL: Secondary regex expression has an error : %s\n", context->secondPhasePattern->error().c_str());
            exit(1);
        }
    }
  
    struct QArchive cacheQArchive;
    cacheQArchive.a = NULL;
    std::string baseDirectory = GetBaseFromFilename(param->sourceArchiveName).c_str();
    if (!param->searchFilenames && !param->ignoreTrigrams)
    {
        // trigram search 
        TrigramContext tri_context;
        tri_context.dataStream = dataStream;
        tri_context.context = context;
        tri_context.qa = &cacheQArchive;
        tri_context.handledFileSet = &staleFilesThatHaveBeenSearched;
        char trifile[1024];
        sprintf(trifile, "%s.tris", param->sourceArchiveName);
  
        TrigramSplitter* ts = trigram_load_from_file(trifile);
        if (ts && trigram_string_is_searchable(param->searchPattern))
        {
            if (trigram_iterate_matching_files(ts, param->searchPattern, &tri_context, trigram_callback, 0))
            {
                goto skip_archive;
            }
            else
            {
                //printf("exiting trigram search early!\n");
            }
        }
    }
  
    {
        struct archive* cacheArchive = archive_read_new();
        cacheQArchive.a = cacheArchive;
  
        // archive handling
#if ARCHIVE_VERSION_NUMBER < 3000000
        archive_read_support_compression_all(cacheArchive);
#else
        archive_read_support_filter_all(cacheArchive);
#endif
        archive_read_support_format_all(cacheArchive);
        r = archive_read_open_filename(cacheArchive, param->sourceArchiveName, 10240); 
        if (r != ARCHIVE_OK)
        {
            printf("FATAL: %s", archive_error_string(cacheArchive));
            exit(1);
        }
  
        while (archive_read_next_header(cacheArchive, &entry) == ARCHIVE_OK) {
            const char* entryName = archive_entry_pathname(entry);
      
            // Don't return results from deleted files
            if (SetContains(gDeletedFiles, entryName))
            {
                archive_read_data_skip(cacheArchive); 
                continue;
            }
      
            // Handle file name search
            if (param->searchFilenames)
            {
                if (RE2::PartialMatch(entryName, *(context->pattern)))
                {
                    param->callbackFunction(param->callbackContext, entryName, 1, 0, 0);
                    staleFilesThatHaveBeenSearched.insert(strdup(entryName));
                }
                archive_read_data_skip(cacheArchive); 
                continue;
            }
      
            FILE* file = NULL;
            /* Handle files that are stale in the cache */
            if (SetContains(gStaleFiles, entryName))
            {
                staleFilesThatHaveBeenSearched.insert(strdup(entryName));
                file = OpenFile(baseDirectory, entryName);
                if (!file)
                {
                    printf("[WARN] Unable to open %s, falling back to cache\n", entryName);
                }
                else
                {
                    archive_read_data_skip(cacheArchive); 
                }
            }
      
            ExecuteContentSearch(dataStream, &cacheQArchive, file, entry, entryName, context);
      
            if (file)
            {
                fclose(file);
                file = NULL;
            }
        }
#if ARCHIVE_VERSION_NUMBER < 3000000
        r = archive_read_finish(cacheArchive);  
#else
        r = archive_read_free(cacheArchive);
#endif
        if (r != ARCHIVE_OK)
        {
            printf("archive_read_finish didn't finish properly\n");
            exit(1);
        }
    }
  
 skip_archive: 
    // Handle added files
    StringSet::iterator end = gStaleFiles.end();
    for (StringSet::iterator i = gStaleFiles.begin(); i != end; ++i)
    {
        if (SetContains(staleFilesThatHaveBeenSearched, *i))
            continue;
      
        if (param->searchFilenames)
        {
            if (RE2::PartialMatch(*i, *(context->pattern)))
            {
                param->callbackFunction(param->callbackContext, *i, 1, 0, 0);
            }
            continue;
        }

        FILE* file = OpenFile(baseDirectory, *i);
        if (file)
        {
            ExecuteContentSearch(dataStream, &cacheQArchive, file, NULL, *i, context);
            fclose(file);
            file = NULL;
        }
    }

    DestroySearchContext(lfsc);
}

struct QArchive* CreateArchive(const char* archiveName, ArchiveCreateParams* param)
{
    struct QArchive* ret = new QArchive;
    if (param->useTrigrams)
    {
        ret->ts = trigram_new();
    }
    else
    {
        ret->ts = NULL;
    }
    ret->archiveFileName = strdup(archiveName);
    struct archive* a = archive_write_new();
    assert(a);
#if ARCHIVE_VERSION_NUMBER < 3000000
    archive_write_set_compression_gzip(a);
    archive_write_set_format_pax_restricted(a); 
    archive_write_open_file(a, archiveName);
#else
    archive_write_add_filter_gzip(a);
    archive_write_set_format_pax_restricted(a); 
    archive_write_open_filename(a, archiveName);
#endif
    ret->a = a;
    return ret;
}

void AddFileToArchive(struct QArchive* qa, const char* filename)
{
    struct archive* a = qa->a;
    struct archive_entry *entry;
    struct stat st;
    char buff[100 * 1024];
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
    
    if (qa->ts) trigram_start_file(qa->ts, filename);
    fd = open(filename, O_RDONLY);
    len = read(fd, buff, sizeof(buff));
    while ( len > 0 ) {
        if (qa->ts) trigram_add_data(qa->ts, buff, len);
        archive_write_data(a, buff, len);
        len = read(fd, buff, sizeof(buff));
    }
    close(fd);
    if (qa->ts) trigram_stop_file(qa->ts);
    archive_entry_free(entry);
}

void CloseArchive(struct QArchive* qa)
{
    struct archive* a = qa->a;
    if (qa->ts)
    {
        char fname[1024];
        sprintf(fname, "%s.tris", qa->archiveFileName);
        trigram_save_to_file(qa->ts, fname);
    }
    archive_write_close(a); 
#if ARCHIVE_VERSION_NUMBER < 3000000
    archive_write_finish(a);
#else
    archive_write_free(a);
#endif
    delete qa;
}



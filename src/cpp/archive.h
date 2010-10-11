#ifndef __ARCHIVE_H
#define __ARCHIVE_H

extern "C" 
{

typedef void (*matchHitCallback)(void* context, const char* filename, unsigned int lineNumber, const char* lineStart, const char* lineEnd);
struct GrepParams
{
    unsigned int streamBlockSize;
    unsigned int streamBlockCount;
    const char* sourceArchiveName;
    matchHitCallback callbackFunction;
    const char* searchPattern;
    unsigned int caseInsensitive;
};
void ExecuteSearch(GrepParams* param);
    
// Creation functions    
struct archive;
struct archive* CreateArchive(const char* archiveName);
void AddFileToArchive(struct archive* a, const char* filename);
void CloseArchive(struct archive* a);

void dllTest(const char* foo);
    
}


#endif // __ARCHIVE_H

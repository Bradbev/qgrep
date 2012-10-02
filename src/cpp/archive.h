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
    void* callbackContext;
    const char* searchPattern;
    unsigned int caseSensitive;
    unsigned int searchFilenames;
    unsigned int regexIsLiteral;
    unsigned int ignoreTrigrams;
    const char* secondPhasePattern;
};
void ExecuteSearch(GrepParams* param);
    
//void ExecuteSimpleSearch(const char* archiveName, const char* options, const char* regex, const char* secondPhaseRegex);
//void ExecuteSimpleColouredSearch(const char* archiveName, const char* options, const char* regex, const char* secondPhaseRegex, const char* colour);
    
// Creation functions    
struct ArchiveCreateParams
{
    bool useTrigrams;
};
    
struct QArchive;
struct QArchive* CreateArchive(const char* archiveName, ArchiveCreateParams* param);
void AddFileToArchive(struct QArchive* qa, const char* filename);
void CloseArchive(struct QArchive* qa);

}


#endif // __ARCHIVE_H

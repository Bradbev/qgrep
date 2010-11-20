#ifndef __FILEWATCHER_H
#define __FILEWATCHER_H

extern "C"
{

typedef unsigned long FileWatch_Id;
typedef void (*FileWatch_Callback)(FileWatch_Id id, const char* directory, const char* filename, int action);
FileWatch_Id FileWatch_AddWatch(const char* directory, FileWatch_Callback callback);
void FileWatch_RemoveWatch(FileWatch_Id id);
void FileWatch_Update();
    
} // extern "C"

#endif __FILEWATCHER_H

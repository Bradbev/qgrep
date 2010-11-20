#include "filewatcher.h"
#include "FileWatcher/FileWatcher.h"
#include <stdio.h>
#include <map>

class CallbackDelegate : public FW::FileWatchListener
{
public:
    CallbackDelegate(FileWatch_Callback callback) : mCallback(callback) {};
    void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action)
    {
	mCallback(watchid, dir.c_str(), filename.c_str(), action);
    }
    
    FileWatch_Callback mCallback;
};

typedef std::map<FileWatch_Id, CallbackDelegate*> WatchToCallback;
FW::FileWatcher gFileWatcher;
WatchToCallback gWatchToCallbackMap;
FileWatch_Id FileWatch_AddWatch(const char* directory, FileWatch_Callback callback)
{
    CallbackDelegate* d = new CallbackDelegate(callback);
    FileWatch_Id id = gFileWatcher.addWatch(directory, d);
    gWatchToCallbackMap[id] = d;
    return id;
}

void FileWatch_RemoveWatch(FileWatch_Id id)
{
    gFileWatcher.removeWatch(id);
    delete gWatchToCallbackMap[id];
    gWatchToCallbackMap.erase(id);
}

void FileWatch_Update()
{
    gFileWatcher.update();
}

#include "filewatcher.h"
#include "FileWatcher/FileWatcher.h"
#include <stdio.h>
#include <iostream>
#include <map>

class CallbackDelegate : public FW::FileWatchListener
{
public:
    CallbackDelegate(FileWatch_Callback callback) : mCallback(callback) {};
    void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action)
    {
	printf("C callback %s\n", filename.c_str());
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
    FileWatch_Id id;
    printf("Adding %s\n", directory);
    try {
	id = gFileWatcher.addWatch(directory, d);
	gWatchToCallbackMap[id] = d;
    } catch (FW::Exception e)
    {
	std::cout << "AddExcept" << e.what() << "\n";
	delete d;
	id = -1;
    }
    return id;
}

void FileWatch_RemoveWatch(FileWatch_Id id)
{
    if (id != (unsigned int)-1)
    {
	try {
	    gFileWatcher.removeWatch(id);
	} catch (FW::Exception e)
	{
	    std::cout << "Remove" << e.what() << "\n";
	}
	delete gWatchToCallbackMap[id];
	gWatchToCallbackMap.erase(id);
    }
}

void FileWatch_Update()
{
    try {
	gFileWatcher.update();
    } catch (FW::Exception e)
    {
	std::cout << "Update" << e.what() << "\n";
    }
}

import threading, os

gHandles = {}
gHandleID = 0;

def slow_GetFileset(root):
    fileset = {}
    for root, subFolders, files in os.walk(root):
        for file in files:
            try:
                fullname = os.path.abspath(os.path.join(root,file))
                fileset[fullname] = os.stat(fullname).st_mtime
            except:
                pass
    return fileset

def slow_TimerPoll(handle):
    monitorInfo = gHandles[handle]
    callback = monitorInfo["callback"]
    oldfileset = monitorInfo["fileset"]
    context = monitorInfo["context"]
    fileset = slow_GetFileset(monitorInfo["dir"])
            
    for k in oldfileset.keys():
        if k in fileset:
            if oldfileset[k] != fileset[k]:
                callback(context, k, "modified")
        else:
            callback(context, k, "deleted")
            
    for k in fileset.keys():
        if not k in oldfileset:
            callback(context, k, "new")
            
    monitorInfo["fileset"] = fileset
            
    t = threading.Timer(monitorInfo["timeout"], slow_TimerPoll, [handle])
    monitorInfo["timer"] = t
    t.start()

def slow_MonitorDirectory(directory, callback, context):
    global gHandleID
    global gHandles
    handle = gHandleID
    gHandleID = gHandleID + 1
    
    monitorInfo = {}
    monitorInfo["callback"] = callback
    monitorInfo["context"] = context
    monitorInfo["fileset"] = slow_GetFileset(directory)
    monitorInfo["timeout"] = 60
    t = threading.Timer(monitorInfo["timeout"], slow_TimerPoll, [handle])
    monitorInfo["timer"] = t
    monitorInfo["dir"] = directory
    gHandles[handle] = monitorInfo
    
    t.start()
    return handle

def slow_ForgetMonitor(handle):
    if handle in gHandles:
        try:
            gHandles[handle]["timer"].cancel()
        except:
            pass
        del gHandles[handle]
    

MonitorDirectory = slow_MonitorDirectory
ForgetMonitor = slow_ForgetMonitor
    
    

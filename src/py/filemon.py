import threading, os, platform
from ctypes import *

#######################################
libigrep = None
if platform.system() == 'Windows':
    libigrep = CDLL("libigrep.dll")
else:    
    libigrep = CDLL("libigrep.dylib");
    
WatchCallbackType = CFUNCTYPE(None, c_int, c_char_p, c_char_p, c_int)
libigrep.FileWatch_AddWatch.restypes = c_int
libigrep.FileWatch_AddWatch.argtypes = [c_char_p, WatchCallbackType]
libigrep.FileWatch_RemoveWatch.argtypes = [c_int]
libigrep.FileWatch_Update.argtypes = []

gCallbacksForPython = {}
gActionNames = { 1 : "new", 2 : "deleted", 4 : "modified" }
gNativeTimer = None

class NativeCallback:
    def __init__(self, f, c):
        self.function = f
        self.context = c
        
def native_update():
    libigrep.FileWatch_Update()
    gNativeTimer = threading.Timer(1, native_update)
    gNativeTimer.start()

def native_callback(id, directory, filename, action):
    callback = gCallbacksForPython.get(id, None)
    actionName = gActionNames.get(action, None);
    if callback and actionName:
        callback.function(callback.context, filename, actionName)
    
gCallbackForC = WatchCallbackType(native_callback)

def native_MonitorDirectory(directory, callback, context):
    global gNativeTimer
    id = libigrep.FileWatch_AddWatch(directory, gCallbackForC)
    gCallbacksForPython[id] = NativeCallback(callback, context);
    if not gNativeTimer:
        gNativeTimer = threading.Timer(1, native_update)
        gNativeTimer.start()
        
    return id
    
def native_ForgetMonitor(id):
    libigrep.FileWatch_RemoveWatch(id);
    del gCallbacksForPython[id]
    if len(gCallbacksForPython) == 0:
        gNativeTimer.cancel();

#######################################

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
    

MonitorDirectory = native_MonitorDirectory
ForgetMonitor = native_ForgetMonitor

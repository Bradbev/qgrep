import threading, os, platform, time
from ctypes import *

libigrep = None
if platform.system() == 'Windows':
    libigrep = CDLL("libigrep.dll")
else:
    libigrep = CDLL("libigrep.dylib")
    
WatchCallbackType = CFUNCTYPE(None, c_int, c_char_p, c_char_p, c_int)
libigrep.FileWatch_AddWatch.restypes = c_int
libigrep.FileWatch_AddWatch.argtypes = [c_char_p, WatchCallbackType]
libigrep.FileWatch_RemoveWatch.argtypes = [c_int]
libigrep.FileWatch_Update.argtypes = []

gCallbacksForPython = {}
gActionNames = { 1 : "new", 2 : "deleted", 4 : "modified" }
gNativeTimer = None

gNativeHandles = {}
gNativeHandleID = 0;

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
    # Start single update timer, if not already running
    global gNativeTimer, gNativeHandleID, gNativeHandles
    if not gNativeTimer:
        gNativeTimer = threading.Timer(1, native_update)
        gNativeTimer.start()
    
    nativeIds = []
    # add toplevel directory watch
    id = libigrep.FileWatch_AddWatch(directory, gCallbackForC)
    if id != -1:
        gCallbacksForPython[id] = NativeCallback(callback, context);
        nativeIds.append(id)
    
    # add subdirectory watches
    for root, subFolders, files in os.walk(directory):
        for s in subFolders:
            id = libigrep.FileWatch_AddWatch(root + s, gCallbackForC)
            if id != -1:
                gCallbacksForPython[id] = NativeCallback(callback, context)
                nativeIds.append(id)
    
    handle = gNativeHandleID
    gNativeHandleID = gNativeHandleID + 1
    gNativeHandles[handle] = nativeIds
    return handle
    
def native_ForgetMonitor(handle):
    global gNativeTimer, gNativeHandleID, gNativeHandles
    idsToRemove = gNativeHandles.get(handle, None)
    if idsToRemove:
        del gNativeHandles[handle]
        for id in idsToRemove:
            libigrep.FileWatch_RemoveWatch(id);
            del gCallbacksForPython[id]
    
    # delete the timer if there are no more callbacks
    if len(gCallbacksForPython) == 0:
        gNativeTimer.cancel()
        gNativeTimer = None
        

#######################################

import sys, os, re, warnings
from ctypes import *
import filemon

libigrep = CDLL("libigrep.dylib");
libigrep.CreateArchive.restype = c_void_p
libigrep.AddFileToArchive.argtypes = [c_void_p, c_char_p]
libigrep.CloseArchive.argtypes = [c_void_p]

def tmpnam():
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        return os.tmpnam()

def searchMultiplePatterns(string, patterns):
    for p in patterns:
        if p.search(string):
            return True;
    return False;

def walkDirectoryForFiles(rootdir, regexs):
    patterns = [ re.compile(r) for r in regexs];
    for root, subFolders, files in os.walk(rootdir):
        for file in files:
            fullname = os.path.join(root,file)
            if searchMultiplePatterns(fullname, patterns):
                yield fullname

def pairToDescription(p):
    return { "bases" : p[0], "exprs" : p[1] };

def listToPairs(l): 
    return zip(l[::2], l[1::2])

gProjects = {};
gBasePath = os.path.expanduser("~/");

############### Project creation
def MakeArchiveName(name):
    return os.path.normpath(os.path.join(gBasePath,".igrep", name + ".tgz"))

class Project:
    def __init__(self, name, *args):
        global gProjects
        gProjects[name] = self
        self.name = name
        self.archivefile = MakeArchiveName(name)
        self.watchers = []
        self.staleSet = set()
        self.files = map(pairToDescription, listToPairs(args))
        
def ProjectNames():
    for p in gProjects:
        yield p
        
def ProjectExists(name):
    return name in gProjects

def GetProjectByName(name):
    return gProjects.get(name, None)

############### Listing files    
def SetExtend(s, l):
    for e in l:
        s.add(e);
    
def GetProjectFiles(name):
    project = gProjects.get(name, None)
    if not project:
        return
    files = set();
    for f in project.files:
        exprs = f["exprs"]
        for b in f["bases"]:
            SetExtend(files, walkDirectoryForFiles(b, exprs))
    return sorted(files)

################ Generating the archive
def TryAtomicRename(old, new):
    #print (old + " -> " + new)
    try:
        os.rename(old, new)
    except OSError:
        try:
            os.remove(new)
        except OSError:
            pass
        else:
            os.rename(old, new)

def GenerateProjectArchive(name):
    project = gProjects.get(name, None)
    if not project:
        return
    tmpname = tmpnam()
    archive = libigrep.CreateArchive(tmpname);
    for f in GetProjectFiles(name):
        print(f)
        libigrep.AddFileToArchive(archive, f)
    libigrep.CloseArchive(archive)
    TryAtomicRename(tmpname, project.archivefile)
    
################## Project watching 
def OutputStaleSet(project):    
    tmpname = tmpnam()
    with open(tmpname, "wb") as f:
        f.writelines("\n".join(sorted(project.staleSet)))
        f.write("\n")
    TryAtomicRename(tmpname, project.archivefile + ".stalefiles")

def FileChangedCallback(project, file, changeType):
    staleSet = project.staleSet
    delname = "-" + file;
    if changeType == "deleted":
        if file in staleSet:
            staleSet.remove(file)
        staleSet.add(delname)
    else:
        if delname in staleSet:
            staleSet.remove(delname)
        staleSet.add(file)
    OutputStaleSet(project)
        
def StartWatchingProject(name):
    project = gProjects.get(name, None)
    if not project:
        return
    for f in project.files:
        exprs = f["exprs"]
        for b in f["bases"]:
            project.watchers.append(filemon.MonitorDirectory(b, FileChangedCallback, project))
                                       
def StopWatchingProject(name):
    project = gProjects.get(name, None)
    if not project:
        return
    for w in project.watchers:
        filemon.ForgetMonitor(w)
    project.watchers = []
    
################ Commandline processing

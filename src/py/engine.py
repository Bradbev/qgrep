import sys, os, re, warnings, platform
from ctypes import *
import filemon

libigrep = None
if platform.system() == 'Windows':
    libigrep = CDLL("libigrep.dll")
else:    
    libigrep = CDLL("libigrep.dylib");
    
libigrep.CreateArchive.restype = c_void_p
libigrep.AddFileToArchive.argtypes = [c_void_p, c_char_p]
libigrep.CloseArchive.argtypes = [c_void_p]
libigrep.ExecuteSimpleSearch.argtypes = [c_char_p, c_char_p, c_char_p]

def tmpnam():
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        return os.tmpnam()

def searchMultiplePatterns(string, patterns):
    for p in patterns:
        if p.search(string):
            return True;
    return False;

def walkDirectoryForFiles(rootdir, regexs, ignoredRegexs):
    patterns = [ re.compile(r) for r in regexs]
    ignored =  [ re.compile(r) for r in ignoredRegexs]
    for root, subFolders, files in os.walk(rootdir):
        for file in files:
            fullname = os.path.join(root,file)
            if searchMultiplePatterns(fullname, patterns) and not searchMultiplePatterns(fullname, ignored):
                yield fullname

gProjects = {};
gBasePath = os.path.expanduser("~/");

############### Project creation
def MakeArchiveName(name):
    return os.path.normpath(os.path.join(gBasePath,".igrep", name + ".tgz"))

class TrackOrIgnore:
    def __init__(self, track, path, regexs):
        self.track = track
        self.path = path
        self.regexs = regexs
        
    def __str__(self):
        return str([self.track, self.path, self.regexs])
        
def track(path, *regexs):
    return TrackOrIgnore(True, path, regexs)

def ignore(*regexs):
    return TrackOrIgnore(False, "", regexs)

class Project:
    def __init__(self, name, *args):
        global gProjects
        gProjects[name] = self
        self.name = name
        self.archivefile = MakeArchiveName(name)
        self.watchers = []
        self.staleSet = set()
        self.trackedFiles = filter(lambda x: x.track is True, args)
        s = set()
        for i in filter(lambda x: x.track is False, args):
            for r in i.regexs:
                s.add(r)
        self.ignoredRegexs = s
        
def ProjectNames():
    for p in gProjects:
        yield p
        
def ProjectExists(name):
    return name in gProjects

def GetProjectByName(name):
    return gProjects.get(name, None)

def FileIsInProject(project, filename):
    ignoredPatterns = [re.compile(r) for r in project.ignoredRegexs]
    if searchMultiplePatterns(filename, ignoredPatterns):
        return False
    for f in project.trackedFiles:
        if filename.startswith(f.path):
            patterns = [re.compile(r) for r in f.regexs]
            if searchMultiplePatterns(filename, patterns):
                    return True
    return False

############### Listing files    
def SetExtend(s, l):
    for e in l:
        s.add(e);
    
def GetProjectFiles(name):
    project = gProjects.get(name, None)
    if not project:
        return
    files = set();
    for f in project.trackedFiles:
        SetExtend(files, walkDirectoryForFiles(f.path, f.regexs, project.ignoredRegexs))
    return sorted(files)

################ Generating the archive
def TryDelete(name):
    #print ("Deleting " + name)
    try:
        os.remove(name)
    except OSError:
        pass

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
            
def GetStalefilename(project):
    return project.archivefile + ".stalefiles"

def GenerateProjectArchive(name):
    project = gProjects.get(name, None)
    if not project:
        return
    tmpname = tmpnam()
    archive = libigrep.CreateArchive(tmpname);
    fileCount = 0
    for f in GetProjectFiles(name):
        fileCount = fileCount + 1
        libigrep.AddFileToArchive(archive, f)
    libigrep.CloseArchive(archive)
    print("Added %d files to archive" % fileCount)
    TryAtomicRename(tmpname, project.archivefile)
    TryDelete(GetStalefilename(project))
    
################## Project watching 
def OutputStaleSet(project):    
    tmpname = tmpnam()
    with open(tmpname, "wb") as f:
        f.writelines("\n".join(sorted(project.staleSet)))
        f.write("\n")
    TryAtomicRename(tmpname, GetStalefilename(project))
    
def FileChangedCallback(project, file, changeType):
    print(file, changeType, FileIsInProject(project, file))
    staleSet = project.staleSet
    delname = "-" + file;
    if changeType == "deleted":
        if file in staleSet:
            staleSet.remove(file)
        staleSet.add(delname)
    else:
        if delname in staleSet:
            staleSet.remove(delname)
        if FileIsInProject(project, file):
            staleSet.add(file)
    OutputStaleSet(project)
        
def StartWatchingProject(project):
    for files in project.trackedFiles:
        project.watchers.append(filemon.MonitorDirectory(files.path, FileChangedCallback, project))
                                       
def StopWatchingProject(project):
    for w in project.watchers:
        filemon.ForgetMonitor(w)
    project.watchers = []
    
################ Commandline processing

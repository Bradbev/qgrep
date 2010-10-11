import sys, os
sys.path.append("../src/py")

import engine   

gTestPass = 0
gTestFail = 0

def test(name, expr):
    global gTestFail
    global gTestPass
    if expr:
        gTestPass = gTestPass + 1
        sys.stdout.write(".")
    else:
        gTestFail = gTestFail + 1
        print ("\n" + name + " Failed!")
        
def testResults():
    if gTestFail == 0:
        print("\nAll tests {0} passed".format(gTestPass));
    else:
        print("\n{0} tests failed {1} passed".format(gTestFail, gTestPass));

def dirtest(f):
    return os.path.abspath("../data/dirtest/" + f)

def lcompare(a, b):
    for x,y in map(None, a,b):
        if x != y:
            return False
    return True

def test_walkDirectoryForFiles():
    base = "../data/dirtest";
    test("matchall",
         lcompare([ dirtest("dirA/aa.c"), dirtest("dirA/aa.cpp"),
                    dirtest("dirA/sound_core.c"), dirtest("dirb/bb.c"),
                    dirtest("dirb/bb.h"), dirtest("dirb/sound_firmware.c")],
                  engine.walkDirectoryForFiles("../data/dirtest", ["."])))
    test("match .c",
         lcompare([ dirtest("dirA/aa.c"), dirtest("dirA/sound_core.c"), 
                    dirtest("dirb/bb.c"), dirtest("dirb/sound_firmware.c")],
                  engine.walkDirectoryForFiles("../data/dirtest", ["\\.c$"])))
    
#test_walkDirectoryForFiles()
#testResults();


#project = { name : "test",
            #files : [ { bases : [ "base1", "base2" ], exprs : [ ".*\.cpp"] }
                      #]
            #}

engine.Project("test",
             [dirtest("")], [".*\.c"],
             [dirtest("dirb")], [".*"]
             )


#engine.GenerateProjectArchive("test")
engine.StartWatchingProject("test")


#import time
#def w():
    #for root, subFolders, files in os.walk("/Users/bradbeveridge/development"):
        #for file in files:
            #yield file;
            #
#t = time.clock()
#l = list(w());
#print (len(l), (time.clock() - t))
        

#def cb(f, reason):
    #print "Callback " + f + " " + reason

#import filemon
#filemon.MonitorDirectory("/Users/bradbeveridge/development/c/igrep/src", cb)

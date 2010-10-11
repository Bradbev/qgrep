#!/usr/bin/python

### Command line handling for instagrep
import sys
import engine

############################################
import os
def dirtest(f):
    return os.path.abspath("../data/dirtest/" + f)

engine.Project("test",
             [dirtest("")], [".*\.c"],
             [dirtest("dirb")], [".*"]
             )
engine.Project("test2",
             [dirtest("")], [".*\.c"],
             [dirtest("dirb")], [".*"]
             )
############################################


gCommands = {};

def AddCommand(commandFunc, name, helpstring, longhelpstring):
    gCommands[name] = {"func" : commandFunc, "shorthelp" : helpstring, "longhelp" : longhelpstring}
    
def ProjectExistsOrExit(name):
    if not engine.ProjectExists(name):
        print(name + " is not a valid project")
        listprojects()
        exit(0)

def usage():
    print "Usage"
    l = 0;
    for k in gCommands.keys():
        l = max(len(k) + 5, l)
    formatstr = "{0:>" + str(l) + "} - "
    for k in sorted(gCommands.keys()):
        print (formatstr.format(k) + gCommands[k]["shorthelp"])
        
def help(arg = "", *ignored):
    if arg == "" or arg == "help":
        usage()
        return
    command = gCommands.get(arg, "")
    if command:
        print command["longhelp"]
    else:
        print "Unknown command " + arg
        
def listprojects(*ignored):
    print("Known projects")
    for n in engine.ProjectNames():
        print("\t" + n)
    
def regen(name=None, *ignored):
    ProjectExistsOrExit(name)
    print ("Regenerating " + name + ", may take some time...")
    engine.GenerateProjectArchive(name)
    print ("Done regenerating");
        
def files(name=None, filter=None, *ignored):
    ProjectExistsOrExit(name)
    print "files"
        
def search(name=None, *optionsAndRegex):
    print "search"
        
def startservice(*ignored):
    print "startservice"
        
def stopservice(*ignored):
    print "stopservice"
        
AddCommand(help, "help", "Provides further help for commands", "")
AddCommand(listprojects, "projects", "Lists all known projects", "longhelp")
AddCommand(regen, "regen", "<project> regenerates the database for <project>", "longhelp")
AddCommand(files, "files", "<project> <regex> filters the filenames in <project> through <regex>", "longhelp")
AddCommand(search, "search", "<project> <regex> searches for <regex> in the given project", "longhelp")
AddCommand(startservice, "start-service", "Launches ", "longhelp")
AddCommand(stopservice, "stops-ervice", "<project> <regex> searches for <regex> in the given project", "longhelp")

def main(argv):                         
    if argv:
        commandSpec = gCommands.get(argv[0], None)
        if commandSpec:
            return commandSpec["func"](*argv[1:]);
    print "Falling through to usage"
    usage();

main(sys.argv[1:])

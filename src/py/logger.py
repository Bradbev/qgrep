gTTY = False
gFile = None

def Init():
    pass

def Deinit():
    if gFile:
        gFile.close()

def EnableFile(filename):
    global gFile
    if gFile:
        gFile.close()
        gFile = None
    if filename:
        gFile = open(filename, "a+")

def EnableTTY(enable):
    global gTTY
    gTTY = enable

def Log(logString):
    if gTTY:
        print(logString)
    
    if gFile:
        gFile.write(logString)
        gFile.write("\n")
        
def Print(logString):
    global gTTY
    print(logString)
    # Don't log twice to TTY
    tty = gTTY
    gTTY = False
    Log(logString)
    gTTY = tty
    
    
# EnableFile("./test")        
# EnableTTY(True)

# Log("foo", "bar")
# Log("foo2", "bar2")

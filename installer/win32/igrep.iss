; -- igrep.iss --

[Setup]
AppName=Instant Grep
AppVersion=0.5
DefaultDirName={pf}\igrep
DefaultGroupName=igrep
; UninstallDisplayIcon={app}\MyProg.exe
Compression=lzma2
SolidCompression=yes
OutputDir=c:\dev\igrep\installer\win32
SourceDir=c:\dev\igrep\dist\win32

[Files]
Source: "*"; DestDir: "{app}"
;Source: "bzip2.dll"; DestDir: "{app}"
;Source: "libarchive2.dll"; DestDir: "{app}"
;Source: "libgcc_s_dw2-1.dll"; DestDir: "{app}"
;Source: "libre2.so.0"; DestDir: "{app}"
;Source: "libstdc++-6.dll"; DestDir: "{app}"
;Source: "pthreadGC2.dll"; DestDir: "{app}"
;Source: "zlib1.dll"; DestDir: "{app}"
;Source: "Readme.txt"; DestDir: "{app}"; Flags: isreadme

;[Icons]
;Name: "{group}\My Program"; Filename: "{app}\MyProg.exe"

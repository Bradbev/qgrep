; -- qgrep.iss --

[Setup]
ChangesEnvironment=true
AppName=Quick Grep
AppVersion=2.0.15
DefaultDirName={pf}\qgrep
DefaultGroupName=qgrep
; UninstallDisplayIcon={app}\MyProg.exe
Compression=lzma2
SolidCompression=yes
OutputDir=d:\mydev\qgrep\installer\win32
SourceDir=d:\mydev\qgrep\dist\win32

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

[Tasks]
Name: modifypath; Description: &Add qgrep directory to your environmental path; Flags: unchecked

[Code]
const
  ModPathName = 'modifypath';
  ModPathType = 'user';
  
function ModPathDir(): TArrayOfString;
begin
  setArrayLength(Result, 1)
  Result[0] := ExpandConstant('{app}');
end;
#include "modpath.iss"

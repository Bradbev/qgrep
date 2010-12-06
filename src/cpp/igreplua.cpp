extern "C" {
#include "lua.h" 
#include "lauxlib.h"
#include "lualib.h"
}

#include <archive.h>
#include <archive_entry.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "archive.h"
#include <sys/types.h>
#include <dirent.h>
#include <set>
#include <string>
#include "re2/re2.h"

#include <fcntl.h> /* For STDIN_FILENO */
#include <sys/select.h> /* For select() */

lua_State* gLuaState = NULL;

static int traceback (lua_State *L) 
{
    if (!lua_isstring(L, 1))  /* 'message' not a string? */
	return 1;  /* keep it intact */
    lua_getfield(L, LUA_GLOBALSINDEX, "debug");
    if (!lua_istable(L, -1)) {
	lua_pop(L, 1);
	return 1;
    }
    lua_getfield(L, -1, "traceback");
    if (!lua_isfunction(L, -1)) {
	lua_pop(L, 2);
	return 1;
    }
    lua_pushvalue(L, 1);  /* pass error message */
    lua_pushinteger(L, 2);  /* skip this function and traceback */
    lua_call(L, 2, 1);  /* call debug.traceback */
    return 1;
}

int DoCall (lua_State *L, int narg, int clear) 
{
    int status;
    int base = lua_gettop(L) - narg;  /* function index */
    lua_pushcfunction(L, traceback);  /* push traceback function */
    lua_insert(L, base);  /* put it under chunk and args */
    status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
    lua_remove(L, base);  /* remove traceback function */
    /* force a complete garbage collection in case of errors */
    if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
    return status;
}

int Report (lua_State *L, int status) 
{
    if (status && !lua_isnil(L, -1)) {
	const char *msg = lua_tostring(L, -1);
	if (msg == NULL) msg = "(error object is not a string)";
	printf("%s", msg);
	printf("\n");
	lua_pop(L, 1);
    }
    return status;
}

int ReportedDoCall (lua_State *L, int narg, int clear)
{
    return Report(L, DoCall(L, narg, clear));
}

int lua_NewTable(lua_State* L)
{
    lua_newtable(L);
    return lua_gettop(L);
}

#define lua_SetTable(L, table, key, valueType, value) { \
	lua_pushstring(L, (key));			\
	lua_push##valueType(L, (value));		\
	lua_settable(L, (table)); }

void GetIgrepPath(char* buffer)
{
    sprintf(buffer, "%s/.igrep", getenv("HOME"));
}

bool FileExists(const char* path)
{
    struct stat dummy;
    return !stat(path, &dummy);
}

class DirWalker
{
public:
    DirWalker(const char* base, bool recursive = false)
    {
	mDir = NULL;
	mRecurse = recursive;
	mDirectorySet.insert(base);
	mCurrentFullPath = "";
	mCurrentShortPath = "";
    }
    
    std::string next()
    {
retryDirOpen:
	if (!mDir)
	{
	    if (mDirectorySet.empty()) 
		return std::string("");
	    StringSet::iterator head = mDirectorySet.begin();
	    std::string nextDir = *head;
	    //printf("next dir = %s\n", nextDir.c_str());
	    mDirectorySet.erase(head);
	    mCurrentShortPath = nextDir;
	    mCurrentFullPath += nextDir;
	    mCurrentFullPath += "/";
	    mDir = opendir(nextDir.c_str());
	    //printf("opendir %s %p\n", nextDir.c_str(), mDir);
	    //printf("retry1\n");
	    goto retryDirOpen;
	}
	struct dirent* entry = readdir(mDir);
	if (entry == NULL)
	{
	    //printf("readdir -> null\n");
	    closedir(mDir);
	    //printf("mCurrentShortPath %s\n", mCurrentShortPath.c_str());
	    //printf("mCurrentFullPath shrinking %s -> ", mCurrentFullPath.c_str());
	    mCurrentFullPath.resize(mCurrentFullPath.length() - mCurrentShortPath.length() - 1);
	    //printf("%s\n", mCurrentFullPath.c_str());
	    mDir = NULL;
	    //printf("retry2\n");
	    goto retryDirOpen;
	}
	if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
	{
	    //printf("retry3 %s\n", entry->d_name);
	    goto retryDirOpen;
	}
	if (entry->d_type == DT_DIR && mRecurse)
	{
	    std::string nextToInsert = mCurrentFullPath + entry->d_name;
	    //printf("mCurrentFullPath = %s\n", mCurrentFullPath.c_str());
	    //printf("inserting %s\n", nextToInsert.c_str());
	    mDirectorySet.insert(nextToInsert.c_str());
	}
	return std::string(mCurrentFullPath + entry->d_name);
    }
private:
    bool mRecurse;
    DIR* mDir;
    typedef std::set<std::string> StringSet;
    StringSet mDirectorySet;
    std::string mCurrentFullPath;
    std::string mCurrentShortPath;
};

class ArchiveWalker
{
public:
    ArchiveWalker(const char* archiveName)
    {
	mArchive = archive_read_new();
	archive_read_support_compression_all(mArchive);
	archive_read_support_format_all(mArchive);
	int r = archive_read_open_filename(mArchive, archiveName, 10240); 
	if (r != ARCHIVE_OK)
	{
	    printf("FATAL: %s", archive_error_string(mArchive));
	    exit(1);
	}
    }
    
    std::string next()
    {
	if (archive_read_next_header(mArchive, &mEntry) == ARCHIVE_OK)
	{
	    std::string entryName = archive_entry_pathname(mEntry);
	    archive_read_data_skip(mArchive); 
	    return entryName;
	}
	return std::string("");
    }
private:
    struct archive* mArchive;
    struct archive_entry* mEntry;
};

// (timeoutSeconds:int) -> boolean
int C_waitForKeypress(lua_State* L)
{
   fd_set read_fds;
   struct timeval timeout;
   
   FD_ZERO( &read_fds );
   FD_SET( STDIN_FILENO, &read_fds );
   
   timeout.tv_sec = luaL_checkinteger(L, 1);
   timeout.tv_usec = 0;
   int select_result = select( 1, &read_fds, NULL, NULL, &timeout );
   if (select_result <= 0)
   {
       lua_pushboolean(L, false);
   }
   else
   {
       lua_pushboolean(L, true);
   }
   return 1;
}

// Gets the igrep path base
// (nil) -> string
int C_igreppath(lua_State* L)
{
    char buf[1024];
    GetIgrepPath(buf);
    lua_pushstring(L, buf);
    return 1;
};

// Does a file exist?
// (filename:string) -> boolean
int C_fileexists(lua_State* L)
{
    lua_pushboolean(L, FileExists(luaL_checkstring(L, -1)));
    return 1;
}

// Returns a directory walker object
// (basepath:string, optional recurse:boolean) -> lightuserdata
int C_walkdir(lua_State* L)
{
    const char* dir = luaL_checkstring(L, 1);
    bool recurse = false;
    if (lua_gettop(L) >= 2)
    {
	recurse = lua_toboolean(L, 2);
    }
    DirWalker* dw = new DirWalker(dir, recurse);
    lua_pushlightuserdata(L, dw);
    return 1;
}

// Get the next directory entry
// (dirwalker:lightuserdata) -> string
// (dirwalker:lightuserdata) -> nil - No more entries
int C_walkdir_next(lua_State* L)
{
    DirWalker* dw = (DirWalker*)lua_topointer(L, 1);
    std::string next = dw->next();
    if (next != "")
    {
	lua_pushstring(L, next.c_str());
    }
    else
    {
	delete dw;
	lua_pushnil(L);
    }
    return 1;
}

// Get information 
// (filename:string) -> table
int C_fileinfo(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);
    struct stat info;
    if (stat(path, &info) == 0)
    {
	int table = lua_NewTable(L);
	lua_SetTable(L, table, "mtime", integer, info.st_mtimespec.tv_sec);
	lua_SetTable(L, table, "isdir", boolean, info.st_mode & S_IFDIR);
	return 1;
    }
    else
    {
	// File doesn't exist
	lua_pushnil(L);
	return 1;
    }
}

// (pattern:lightuserdata) -> nil
int C_re2_gc(lua_State* L)
{
    re2::RE2* pattern = *((re2::RE2**)luaL_checkudata(L, 1, "RE2Pattern"));
    //printf("deleting %p\n", pattern);
    delete pattern;
    return 0;
}

// Compare an input string to a pattern
// (pattern:userdata, str:string) -> boolean
int C_re2_partialMatch(lua_State* L)
{
    re2::RE2* pattern = *((re2::RE2**)luaL_checkudata(L, 1, "RE2Pattern"));
    const char* str = luaL_checkstring(L, 2);
    lua_pushboolean(L, RE2::PartialMatch(str, *pattern));
    return 1;
}

// Compare an input string to a pattern
// (str:string, pattern:lightuserdata) -> boolean
// (str:string, pattern:string) -> boolean
int C_re2_fullMatch(lua_State* L)
{
    re2::RE2* pattern = *((re2::RE2**)luaL_checkudata(L, 1, "RE2Pattern"));
    const char* str = luaL_checkstring(L, 2);
    lua_pushboolean(L, RE2::FullMatch(str, *pattern));
    return 1;
}

// Compile a string into an RE2 pattern
// (pattern:string) -> lightuserdata
int C_re2_compile(lua_State* L)
{
    re2::RE2* pattern = new re2::RE2(luaL_checkstring(L, 1));
    re2::RE2** lData = (re2::RE2**)lua_newuserdata(L, sizeof(re2::RE2*));
    *lData = pattern;
    //printf("newuserdata %p\n", pattern);
    if (luaL_newmetatable(L, "RE2Pattern"))
    {
	int metatable = lua_gettop(L);
	lua_SetTable(L, metatable, "__gc", cfunction, C_re2_gc);
	lua_SetTable(L, metatable, "partialMatch", cfunction, C_re2_partialMatch);
	lua_SetTable(L, metatable, "fullMatch", cfunction, C_re2_fullMatch);
	lua_setfield(L, -1, "__index");                        /* hook up the __index */
	luaL_newmetatable(L, "RE2Pattern");          /* Leave the MT on the stack */
    }
    lua_setmetatable(L, -2);
    return 1;
}

luaL_Reg luaFunctions[] =
{
    { "regex", C_re2_compile },
    { "igreppath", C_igreppath },
    { "fileexists", C_fileexists },
    { "walkdir", C_walkdir },
    { "walkdir_next", C_walkdir_next },
    { "fileinfo", C_fileinfo },
    { "waitForKeypress", C_waitForKeypress },
    { NULL, NULL}, 
};

// (archiveName:string) -> lightuserdata
int C_archive_CreateArchive(lua_State* L)
{
    const char* archiveName = luaL_checkstring(L,1);
    lua_pushlightuserdata(L, CreateArchive(archiveName));
    return 1;
}

// (archive:lightuserdata, filename:string) -> nil
int C_archive_AddFileToArchive(lua_State* L)
{
    struct archive* a = (struct archive*)lua_topointer(L, 1);
    const char* filename = luaL_checkstring(L, 2);
    AddFileToArchive(a, filename);
    return 0;
}

// (archive:lightuserdata) -> nil
int C_archive_CloseArchive(lua_State* L)
{
    struct archive* a = (struct archive*)lua_topointer(L, 1);
    CloseArchive(a);
    return 0;
}

// (archiveName:string> -> lightuserdata
int C_archive_OpenArchive(lua_State* L)
{
    ArchiveWalker* aw = new ArchiveWalker(luaL_checkstring(L, 1));
    lua_pushlightuserdata(L, aw);
    return 1;
}

// (archive:lightuserdata) -> string | nil
int C_archive_ArchiveNext(lua_State* L)
{
    ArchiveWalker* aw = (ArchiveWalker*)lua_topointer(L, 1);
    std::string next = aw->next();
    if (next == "")
    {
	delete aw;
	lua_pushnil(L);
	return 1;
    }
    lua_pushstring(L, next.c_str());
    return 1;
}

luaL_Reg archiveFunctions[] =
{
    { "CreateArchive", C_archive_CreateArchive },
    { "AddFileToArchive", C_archive_AddFileToArchive },
    { "CloseArchive", C_archive_CloseArchive },
    { "OpenArchive", C_archive_OpenArchive },
    { "ArchiveNext", C_archive_ArchiveNext },
    { NULL, NULL}, 
};

void InitLua()
{
    if (!gLuaState)
    {
	gLuaState = luaL_newstate();
	luaL_openlibs(gLuaState);
	luaL_register(gLuaState, "c", luaFunctions);
	luaL_register(gLuaState, "archive", archiveFunctions);
	luaL_dofile(gLuaState, "igrep.lua");
    }
}

void usage()
{
    InitLua();
    lua_getfield(gLuaState, LUA_GLOBALSINDEX, "usage");    
    ReportedDoCall(gLuaState, 0,0);
    exit(1);
}

void lua_ProjectExistsOrDie(const char* projectName)
{
    InitLua();
    lua_getfield(gLuaState, LUA_GLOBALSINDEX, "GetProjectOrDie");    
    lua_pushstring(gLuaState, projectName);
    ReportedDoCall(gLuaState, 1,0);
}

bool StrStartsWith(const char* str, const char* startsWith)
{
    if (str == NULL || *str == 0)
    {
	return false;
    }
    while (*str && *startsWith)
    {
	if (tolower(*str) != tolower(*startsWith))
	    return false;
	str++; startsWith++;
    }
    return true;
}

char* GetProjectFileName(char* buffer, const char* project)
{
    char buf[1024];
    GetIgrepPath(buf);
    sprintf(buffer, "%s/%s.tgz", buf, project);
    return buffer;
}

void FastPathSearch(int argc, const char** argv, bool searchFileNames = false)
{
    const char* project = argv[2];
    const char* options = "";
    const char* regex = "";
    char projectFile[1024];
    switch (argc)
    {
    case 5: // has options
	options = argv[3];
	regex = argv[4];
	break;
    case 4: // options omitted
	regex = argv[3];
	break;
    default:
	printf("Insufficient arguments for command '%s'\n", argv[1]);
	usage();
    }
    GetProjectFileName(projectFile, project);
    if (FileExists(projectFile))
    {
	char realOptions[10];
	sprintf(realOptions, "%s%s", options, searchFileNames ? "f" : "");
	ExecuteSimpleSearch(projectFile, realOptions, regex);
    }
    else
    {
	lua_ProjectExistsOrDie(project);
	printf("Project is registered, but archive does not exist.\n");
	printf("Run 'igrep build %s' to generate archive\n", project);
    }
}

int main(int argc, const char** argv)
{
    if (argc > 1)
    {
	bool search = StrStartsWith(argv[1], "search");
	bool files = StrStartsWith(argv[1], "files");
	if (search || files)
	{
	    FastPathSearch(argc, argv, files);
	    return 0;
	}
    }
    
    // Fall through to lua handling
    InitLua();
    lua_getfield(gLuaState, LUA_GLOBALSINDEX, "main");    
    for (int i = 0; i < argc; i++)
    {
	lua_pushstring(gLuaState, argv[i]);
    }
    ReportedDoCall(gLuaState, argc,0);
    
    return 0;
}

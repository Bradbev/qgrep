extern "C" {
#include "lua.h" 
#include "lauxlib.h"
#include "lualib.h"
}

#if EMBED_BINARY
#include "igreplua.bin.h"
#endif

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

// Forward declares
void lua_ProjectExistsOrDie(const char* projectName);

lua_State* gLuaState = NULL;

bool gVerbose = false;

int Log(const char* fmt, ...)
{
    if (gVerbose)
    {
	va_list ap;
	va_start(ap, fmt);
	int ret = vprintf(fmt, ap);
	va_end(ap);
	return ret;
    }
    return 0;
}

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

int ReportedDoFile(lua_State *L, const char* filename)
{
    luaL_loadfile(L, filename);
    return ReportedDoCall(L, 0, 0);
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

void GetQgrepPath(char* buffer)
{
    char* home = getenv("QGREP_HOME");
	if (!home) home = getenv("HOME");
#ifdef WIN32
    if (home)
    {
	sprintf(buffer, "%s/.qgrep", home);
    }
    else
    {
	sprintf(buffer, "%s%s/.qgrep", getenv("HOMEDRIVE"), getenv("HOMEPATH"));
    }
#else
    sprintf(buffer, "%s/.qgrep", home);
#endif
}

bool FileExists(const char* path)
{
    struct stat dummy;
    return !stat(path, &dummy);
}

char* GetProjectFileName(char* buffer, const char* project)
{
    char buf[1024];
    GetQgrepPath(buf);
    sprintf(buffer, "%s/%s.tgz", buf, project);
    return buffer;
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
    
    bool IsDir(struct dirent* entry)
    {
#ifdef WIN32
	const char* path = std::string(mCurrentFullPath + entry->d_name).c_str();
	struct stat info;
	if (stat(path, &info) == 0)
	{
	    return info.st_mode & S_IFDIR;
	}
	return false;
#else
	return (entry->d_type == DT_DIR);
#endif
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
	if (IsDir(entry) && mRecurse)
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

//#include <fcntl.h> /* For STDIN_FILENO */
//#include <sys/select.h> /* For select() */

// (timeoutSeconds:int) -> boolean
int C_waitForKeypress(lua_State* L)
{
#if 0
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
#else
    return 0;
#endif
}

// Gets the qgrep path base
// (nil) -> string
int C_qgreppath(lua_State* L)
{
    char buf[1024];
    GetQgrepPath(buf);
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
#if defined(WIN32) || defined(LINUX)
	lua_SetTable(L, table, "mtime", integer, info.st_mtime);
#else
	lua_SetTable(L, table, "mtime", integer, info.st_mtimespec.tv_sec);
#endif
	lua_SetTable(L, table, "isdir", boolean, info.st_mode & S_IFDIR);
	lua_SetTable(L, table, "size",  integer, info.st_size);
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
// (pattern:string) -> userdata
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

// (nil) -> bool
int C_isVerbose(lua_State* L)
{
    lua_pushboolean(L, gVerbose);
    return 1;
}

// (dirname:string) -> NIL
int C_mkdir(lua_State* L)
{
    const char* dirname = luaL_checkstring(L,1);
    int ret = mkdir(dirname, 0777);
    lua_pushinteger(L, ret);
    return 1;
}

// callback for lua initiated searches
static void luaHitCallback(void* context, const char* filename, unsigned int lineNumber, const char* lineStart, const char* lineEnd)
{
    lua_State* L = (lua_State*)context;
    int top = lua_gettop(L);
    // Top of L should have the function to call, duplicate it
    lua_pushvalue(L, top);
    lua_pushstring(L, filename);
    lua_pushinteger(L, lineNumber);
    lua_pushlstring(L, lineStart, lineEnd-lineStart);
    ReportedDoCall(L, 3, 1);
}


void test_error(int test, const char* msg)
{
    if (!test)
    {
	printf("%s\n", msg);
	printf("Exiting...\n");
	exit(0);
    }
}
// arg is a table with the following elements
// project  - string name of the project
// callback - function(filename, linenumber, linestring)
// regex    - regex to search for
// caseSensitive  - optional
// regexIsLiteral - optional
int C_executeSearch(lua_State* L)
{
    const int arg_table = 1;
    lua_getfield(L, arg_table, "project");            /* idx:2 */
    test_error(lua_isstring(L, -1), "execute_search needs a key of 'project' which must be a string");
    const char* project = luaL_checkstring(L, 2);

    char projectFile[1024];
    GetProjectFileName(projectFile, project);
    if (FileExists(projectFile))
    {
	// unpack the rest of the table
	lua_getfield(L, arg_table, "regex");          /* idx:3 */
	test_error(lua_isstring(L, -1), "execute_search needs a key of 'regex' which must be a string");
	lua_getfield(L, arg_table, "caseSensitive");  /* idx:4 */
	lua_getfield(L, arg_table, "regexIsLiteral"); /* idx:5 */
	// MUST leave the callback on the top of the stack
	lua_getfield(L, arg_table, "callback");       /* idx:6 */
	test_error(lua_isfunction(L, -1), "execute_search needs a key of 'callback' which must be a lua function");
	
	// marshal into C data
	const char* regex   = luaL_checkstring(L, 3);
	bool caseSensitive  = lua_isnil(L, 4) || lua_toboolean(L, 4);
	bool regexIsLiteral = lua_toboolean(L, 5);
	
	GrepParams params;
	memset(&params, 0, sizeof(params));
	// standard parms
	params.streamBlockSize = 1 * 1024 * 1024;
	params.streamBlockCount = 10;
	params.searchFilenames = false;
	// custom parms
	params.sourceArchiveName = projectFile;
	params.callbackFunction = luaHitCallback;
	params.caseSensitive = caseSensitive;
	params.regexIsLiteral = regexIsLiteral;
	params.searchPattern = regex;
	params.callbackContext = (void*)L;
	ExecuteSearch(&params);
    }
    else
    {
	lua_ProjectExistsOrDie(project);
	printf("Project is registered, but archive does not exist.\n");
	printf("Run 'qgrep build %s' to generate archive\n", project);
    }
    return 0;
}

luaL_Reg luaFunctions[] =
{
    { "regex", C_re2_compile },
    { "mkdir", C_mkdir },
    { "qgreppath", C_qgreppath },
    { "fileexists", C_fileexists },
    { "walkdir", C_walkdir },
    { "walkdir_next", C_walkdir_next },
    { "fileinfo", C_fileinfo },
    { "waitForKeypress", C_waitForKeypress },
    { "isVerbose", C_isVerbose },
    { "execute_search", C_executeSearch},
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

// (archiveWalker:lightuserdata) -> string | nil
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
	int loadStatus = 0;
#if EMBED_BINARY
	luaL_loadbuffer(gLuaState, lua_binary_data, sizeof(lua_binary_data), "main");
	loadStatus = ReportedDoCall(gLuaState, 0, 0);	
#else
	printf("---- DEBUG DEBUG Doing file\n");
	loadStatus = ReportedDoFile(gLuaState, "igrep.lua");
#endif
	if (loadStatus != 0)
	{
	    printf("Error occured when loading project file\n");
	    exit(1);
	}
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

void FastPathSearch(int argc, const char** argv)
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
	ExecuteSimpleSearch(projectFile, options, regex);
    }
    else
    {
	lua_ProjectExistsOrDie(project);
	printf("Project is registered, but archive does not exist.\n");
	printf("Run 'qgrep build %s' to generate archive\n", project);
    }
}

int main(int argc, const char** argv)
{
    if (argc > 1)
    {
	if (strcmp("v", argv[1]) == 0)
	{
	    gVerbose = true;
	    argc--;
	    argv = &argv[1];
	}
	Log("Verbosity test %s\n", "foo");
	bool search = StrStartsWith(argv[1], "search");
	if (search)
	{
	    FastPathSearch(argc, argv);
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

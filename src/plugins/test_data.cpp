#include "render.h"
#include "input.h"
#include "system.h"
#include <stdlib.h>
#include "luarepl/luarepl.h"
#include "luabound/luautil.h"
#include "sys/time.h"
#include <map>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

// comment block 1, line 1/2 #block1
// comment block 1, line 2/2

// ABSTRACT THESE HEADERS
#include "SDL.h"

/* comment block 2, line 1/2  #block2 */
/* comment block 2, line 2/2 */

int main(int argc, char*argv[])
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    ren_openlib(L);
    input_openlib(L);
    
/*
 * Comment block 3, line 2/4  #block3
 * Comment block 3, line 3/4  @name
 */
    
    if (argc == 2)
    {
	printf("Loading file to test %s\n", argv[1]);
	luaL_loadfile(L, argv[1]);
	if (LuaUtil::ReportedDoCall(L, 0, 0))
	{
	    printf("Failed to load lua files, not running\n");
	}
	return 0;
    }
    ren_start();
    
    LuaRepl::Init(L, LuaRepl::AUTO_SHUTDOWN_ON_EXIT, LuaRepl::SOCKET_AND_STDOUT);
    //LuaRepl::ListenForDebugger(PORTNO, 0);
    LuaRepl::AutoConnectToEmacsDebugger();
    
    bool running = true;
    luaL_loadfile(L, "main.lua");
    if (LuaUtil::ReportedDoCall(L, 0, 0))
    {
	printf("Failed to load lua files, not running\n");
	running = false;
    }
    
    while (running)
    {
	LuaRepl::Pump(L);

	running = !input_should_quit();
	
	// input_safe_poll can't throw errors
	lua_getglobal(L, "input_safe_poll");
	LuaUtil::ReportedDoCall(L, 0, 0);
	
	lua_getglobal(L, "update");
	LuaUtil::ReportedDoCall(L, 0, 0);
	
	ren_update();
    }
    
    lua_close(L);
    ren_stop();
    return 0;
}

// testing comments /*

/* a */ /* b */

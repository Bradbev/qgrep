-- TDD
-- Simple test framework.
-- Runs global functions that begin with test_ in alphabetical order
-- if a function in named test_.*_setup, then that setup function will be run
-- for every function that matches the prefix part
-- setup functions should not assert or perform checks

RUNNING_TESTS = true

local gTestsRun = 0
local gTestsFailed = 0
local gCurrentTestPassed = 0

function printf(fmt, ...)
   io.write(string.format(fmt, ...))
end

function tprint(t)
   local function tprint_impl(t, indent)
      local function do_indent(indent)
	 for i=1,indent * 3 do
	    printf(" ")
	 end
      end

      indent = indent or 0
      if type(t) == "table" then
	 for k,v in pairs(t) do
	    do_indent(indent)
	    printf("%q  :", k)
	    if type(v) == "table" then 
	       printf("--->\n")
	       tprint_impl(v, indent + 1)
	    else
	       printf("%q\n", tostring(v))
	    end
	 end
      else
	 do_indent(indent)
	 printf("%q\n", t or "NIL")
      end
   end
   tprint_impl(t)
end

function eq(v1, v2)
   local function table_eq(a,b) 
      for k,v in pairs(a) do
	 if eq(v, b[k]) == false then return false 
	 end
      end
      return true
   end
   
   if type(v1) == "table" and type(v2) == "table" then
      return table_eq(v1, v2) and table_eq(v2, v1)
   end
   if v1 == v2 then return true end
   return false
end

function checkeq(a,b)
   local ok = eq(a,b)
   if not ok then
      print("######## checkeq failed (A) ########")
      tprint(a)
      print("######## checkeq failed (B) ########")
      tprint(b)
      print("######## checkeq failed (DONE) ########")
   end
   check(ok)
end

function check(expression)
   gTestsRun = gTestsRun + 1
   if expression then 
	  io.write(".")
	  return true
   else
	  print("Check failed")
	  print(debug.traceback())
	  gTestsFailed = gTestsFailed + 1
	  gCurrentTestPassed = false
	  return false
   end
end

function checknot(expression)
   return check(not expression)
end

function checkerror(f)
   local status, ret = pcall(f)
   return checknot(status)
end

local function getSetupPrefix(s)
   return string.match(s, "(.*)_setup")
end

local function addSetupFunction(t, funcname)
   table.insert(t, getSetupPrefix(funcname))
   table.sort(t)
   return t
end

local function getSetupFunctions(setups, s)
   local r = {}
   for _, name in pairs(setups) do
      if string.match(s, name, 1, true) then
	 table.insert(r, name .. "_setup")
      end
   end
   return r
end

function run_all_tests()
   print("Running Tests")
   local funcs = {}
   local setupfuncs = {}
   for k,v in pairs(_G) do
      if type(v) == "function" and string.match(k, "^test_") then
	 if getSetupPrefix(k) then
	    setupfuncs = addSetupFunction(setupfuncs, k)
	 else
	    table.insert(funcs, k)
	 end
      end
   end
   table.sort(funcs)
   gTestsRun = 0
   for i,name in pairs(funcs) do
	  gCurrentTestPassed = true
	  gTestsRun = gTestsRun + 1
	  io.write("Testing " .. name .. " ")
	  for _, s in pairs(getSetupFunctions(setupfuncs, name)) do
	     _G[s]()
	  end
	  
	  local status, ret = xpcall(_G[name], debug.traceback)
	  if status == false or ret == false or gCurrentTestPassed == false then
		 gTestsFailed = gTestsFailed + 1
		 io.write("-> failed. ")
		 if gCurrentTestPassed == false then
			print("Internal check failed")
		 elseif status then
			io.write(string.format("Returned %s\n", tostring(ret)))
		 else
			io.write(string.format("Caught error: %s\n", tostring(ret)))
		 end
	  else
		 io.write("-> passed\n")
	  end
   end
   local passed = gTestsRun - gTestsFailed
   print(string.format("Ran %d tests, %d passed (%.0f%% ok)", gTestsRun, passed, passed/gTestsRun*100))
end


--[[
function test_meta_0() 
   check(gSetup)
end

function test_meta_setup() 
   gSetup = true
end

function test_prefix_get()
   local setup = "test_meta_setup"
   local test = "test_meta_0"
   check(getSetupPrefix(setup) == "test_meta")
   checknot(getSetupPrefix(test))
end

function test_prefix_match()
   local test = "test_meta_long_test"
   local setupfuncs = addSetupFunction({}, "test_meta_setup")
   setupfuncs = addSetupFunction(setupfuncs, "test_meta_long_setup")
   setupfuncs = addSetupFunction(setupfuncs, "test_meta_first_setup")
   check(getSetupFunctions(setupfuncs, test)[1] == "test_meta_setup")
   check(getSetupFunctions(setupfuncs, test)[2] == "test_meta_long_setup")
end

run_all_tests()
--]]

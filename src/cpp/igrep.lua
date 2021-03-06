gVersion = "2.0.16"
gVerbose = false

------------- Util
function Log(...)
   if c.isVerbose() then
      print(unpack(arg))
   end
end

function key_sorted_pairs(t)
   local sorted = {}
   for k,_ in pairs(t) do 
      table.insert(sorted, k)
   end
   table.sort(sorted)
   return coroutine.wrap(
	function()
	   for _,k in pairs(sorted) do
	      coroutine.yield(k, t[k])
	   end
	end
     )
end

-- Shift all indexes in a table down, dropping index 1
function tableshift(t, shiftAmount)
   shiftAmount = (shiftAmount or 1) + 1
   local ret = {}
   for i = shiftAmount, #t do
      table.insert(ret, t[i])
   end
   return ret
end

-- Iterator for directory walking
function walkdir(base, recurse)
   recurse = recurse or false
   local dirwalker = c.walkdir(base, recurse)
   return function()
	     return c.walkdir_next(dirwalker)
	  end
end

function iterateArchive(archiveName)
   local archiveWalker = archive.OpenArchive(archiveName)
   return function()
	     return archive.ArchiveNext(archiveWalker)
	  end
end

function iterateArchiveWithData(archiveName)
   local archiveWalker = archive.OpenArchive(archiveName)
   return function()
	     return archive.ArchiveNextWithData(archiveWalker)
	  end
end

function iterateToTable(i)
   local ret = {}
   while true do
      local val = i()
      if val then
	 table.insert(ret, val)
      else
	 return ret
      end
   end
end

function kvmap(f, t)
   local ret = {}
   for k,v in pairs(t) do
      ret[k] = f(k,v)
   end
   return ret
end

function map(f, t)
   local ret = {}
   for k,v in pairs(t) do
      ret[k] = f(v)
   end
   return ret
end

function filter(f, t)
   local ret = {}
   for k,v in ipairs(t) do
      if f(v) then
	 table.insert(ret, v)
      end
   end
   return ret
end

-- prints a table
function tprint(t)
   kvmap(print, t)
end

function setInsert(s, v)
   s[v] = true
end

function setToList(s)
   local ret = {}
   for k,v in pairs(s) do
      table.insert(ret, k)
   end
   return ret
end

function fullMatchAny(regexs, str)
   for k,v in ipairs(regexs) do
      if v:fullMatch(str) then return true end
   end
   return false
end

function partialMatchAny(regexs, str)
   for k,v in ipairs(regexs) do
      if v:partialMatch(str) then return true end
   end
   return false
end

function IsFile(path)
   local info = c.fileinfo(path)
   if info and not info.isdir then
      return true
   end
   return false
end

function filteredWalkDir(path, include, ignore)
   local ret = {}
   for f in walkdir(path, true) do
      if partialMatchAny(include, f) and not 
	 partialMatchAny(ignore, f) and
	 IsFile(f) then
	 table.insert(ret, f)
      end
   end
   return ret
end

------------- Project handling
gProjects = {}

function PathExpand(path)
   if path:sub(1,1) == "~" then
      return c.qgreppath() .. path:sub(2)
   else
      return path
   end
end

function tracked(x)
   return x.tracked
end

function track(path, ...)
   local regexs = arg
   if #regexs == 0 then
      regexs = {"."}
   end
   return { tracked = true, path = PathExpand(path), regexs = map(c.regex, regexs) }
end

function ignored(x)
   return x.ignored
end

function ignore(...)
   return { ignored = true, regexs = map(c.regex, arg) }
end

function is_property(x)
   return x.property
end

function property(props)
   return { property = true, properties = props }
end


MonitorPeriodInMinutes = 10
function GetMonitorPeriodInSeconds()
   if MonitorPeriodInMinutes and type(MonitorPeriodInMinutes) == "number" then
      return math.floor(MonitorPeriodInMinutes) * 60
   else
      return 10 * 60
   end
end

function Project(tableArg)
   if type(tableArg) ~= "table" or type(tableArg[1]) ~= "string" then
      print( type(tableArg), type(tableArg[1]))
      print("Project must be called with a string as the first table entry")
      print("Project{\"name\", track(\".\"), ignore(\".*c\")")
   end
   -- zap the first element of the table so that filters below don't pick up the name
   local name = tableArg[1]
   tableArg[1] = {}
   local proj = { name = name, properties = { useTrigrams = true } }
   proj.tracked = filter(tracked, tableArg)
   
   -- flatten the ignored expressions
   local ignoreExprs = {}
   for k,v in ipairs(filter(ignored, tableArg)) do
      for k,v in ipairs(v.regexs) do
	 setInsert(ignoreExprs, v)
      end
   end
   
   -- gather properties
   for k,v in ipairs(filter(is_property, tableArg)) do
      for k,v in pairs(v.properties) do
	 proj.properties[k] = v
      end
   end
    
   proj.ignoreExprs = setToList(ignoreExprs)
   
   proj.archiveFile = c.qgreppath() .. "/" .. name .. ".tgz"
   proj.staleFile = proj.archiveFile .. ".stalefiles"
   proj.filenamesFile = proj.archiveFile .. ".files"
   proj.filesInArchive = nil
   proj.filesInArchiveScannedAt = 0
   
   gProjects[name] = proj
end

local function norm_path(path)
   loop_count = 0
   function check_infinite_loop()
      loop_count = loop_count + 1
      if loop_count > 10000 then
         print("Loop count exceeded on ", path)
         os.exit(0)
      end
   end

   local ret = string.gsub(path, "\\", "/")
   ret = string.gsub(ret, "//", "/")
   while ret:find("/%.%./") do
	  ret = ret:gsub("/%a-/%.%./", "/")
      check_infinite_loop()
   end
   while ret:find("/%./") do
	  ret = ret:gsub("/%./", "/")
      check_infinite_loop()
   end
   while ret:find("//") do
	  ret = ret:gsub("//", "/")
      check_infinite_loop()
   end
   return ret
end

function IterateProjectFiles(project)
   local seen = {}
   return coroutine.wrap(
	function ()
 	   for k,v in pairs(project.tracked) do
 	      for k,v in pairs(filteredWalkDir(v.path, v.regexs, project.ignoreExprs)) do
			 v = norm_path(v)
			 if not seen[v] then
				seen[v] = true
				coroutine.yield(v)
			 end
	      end
	   end
	end)
end

function GetFileTime(path)
   local info = c.fileinfo(path)
   if info and not info.isdir then
      return info.mtime
   end
   return 0
end

function CreateProjectStaleFiles(project)
   -- scrape the files from the zip file
   local archiveTime = GetFileTime(project.archiveFile)
   if archiveTime > project.filesInArchiveScannedAt then
      Log("reloading archive")
      project.filesInArchive = iterateToTable(iterateArchive(project.archiveFile))
      project.filesInArchiveScannedAt = archiveTime
   end
   if project.filesInArchive == nil then return end
   
   local time = archiveTime
   
   local tmpname = os.tmpname()
   local file = io.open(tmpname, "w")
   local filenames = {}
   
   local fileHasEntries = false
   -- Catch the new & modified files
   for f in IterateProjectFiles(project) do
      filenames[f] = true
      if GetFileTime(f) > time then
	 Log(f)
	 file:write(f .. "\n")
	 fileHasEntries = true
      end
   end
   -- Catch the deleted files
   local namesToRemove = {}
   for k,v in pairs(project.filesInArchive) do
      if not filenames[v] then
	 namesToRemove[v] = true
	 file:write("-" .. v .. "\n")
	 fileHasEntries = true
      end
   end
   
   file:close()
   if fileHasEntries then
      FileRename(tmpname, project.staleFile)
   end
end
------------- Command Handling ------------
gCommands = {}
gLongHelp = {}

function defCommand(func, name, help)
   gCommands[name] = { func = func, help = help }
end

function defHelp(func, longhelpString)
   gLongHelp[func] = longhelpString
end

function ExecuteCommandLine(args)
   local command = args[1] or ""
   local entry = gCommands[command]
   if entry then
      entry.func(unpack(tableshift(args)))
   else
      print(string.format("Unknown command '%s'", command))
      usage()
   end
end
-------------------------------------------

function usage()
   print("In the following help [] denotes optional arguments, <> denotes required arguments")
   print("Verbose mode is enabled with 'qgrep v ...'")
   print("Commands for qgrep")
   for k,v in key_sorted_pairs(gCommands) do
      print(string.format("%20s  -  %s", k, v.help))
   end
end

function GetProjectOrDie(projectName)
   projectName = projectName or ""
   local project = gProjects[projectName]
   if not project then
      print("Unknown project " .. projectName)
      listprojects()
      os.exit(0)
   end
   return project
end

function help(command)
   local entry = gCommands[command]
   if entry then
      local longHelp = gLongHelp[entry.func]
      if longHelp then
	 print("---------------------------------------------------")
	 print("Extended help for " .. command .. ":")
	 print(longHelp)
	 print("---------------------------------------------------")
      else
	 print(command .. " has no extended help")
      end
   else
      if command then print(command .. " is an unknown command") end
      usage();
   end
end

function FileRename(old, new)
   if not os.rename(old, new) then
      os.remove(new)
      if not os.rename(old,new) then
	 local size = 2^13      -- good buffer size (8K)
	 local fin = io.open(old, "r")
	 local fout = io.open(new, "w+")
	 while fin and fout do
	    local block = fin:read(size)
	    if not block then 
	       fin:close()
	       fout:close()
	       break 
	    end
	    fout:write(block)
	 end	 
      end
   end
end

function scanstalefiles(projectName)
   local project = GetProjectOrDie(projectName)
   print("Scanning project for stale files, may take a little while")
   CreateProjectStaleFiles(project)
end
defHelp(scanstalefiles,
"Check modification times and only update the stalefile.  Do not rebuild the full project indexes.")

function build(projectName, options)
   local project = GetProjectOrDie(projectName)
   print("Building " .. projectName .. ", may take some time")
   if options and options:find("T") then
      project.properties.useTrigrams = false
   end
   if project.properties.useTrigrams then
      print("Including trigram index")
   end
   local tmpname = os.tmpname()
   local tmpfilenames = os.tmpname()
   local filenamesfile = io.open(tmpfilenames, "w")
   local a = archive.CreateArchive(tmpname, project.properties)
   local count = 0
   for f in IterateProjectFiles(project) do
      Log("Adding file", f)
      count = count + 1
      archive.AddFileToArchive(a, f)
      filenamesfile:write(string.format("%s\n", f))
   end
   archive.CloseArchive(a)
   filenamesfile:close()
   
   print("Added " .. count .. " files to archive");
   FileRename(tmpname, project.archiveFile)
   FileRename(tmpfilenames, project.filenamesFile)
   if project.properties.useTrigrams then
      FileRename(tmpname..".tris", project.archiveFile..".tris")
   end
   os.remove(project.staleFile)
   
   print("Done building")
end

function build_all()
   print("Buliding all projects")
   for k,v in pairs(gProjects) do
      build(k)
   end
end

function listprojects()
   print("Known projects")
   for k,v in pairs(gProjects) do
      print("\t" .. k)
   end
end

function split(s, sep)
        local sep, fields = sep or ",", {}
        local pattern = string.format("([^%s]+)", sep)
        s:gsub(pattern, function(c) fields[#fields+1] = c end)
        return fields
end

function files(projectNames, regex)
   for _, projectName in pairs(split(projectNames)) do
	  local project = GetProjectOrDie(projectName)
	  regex = regex or "."
	  local pattern = c.regex("(?i)"..regex)
      if not c.fileexists(project.filenamesFile) then
         for f in IterateProjectFiles(project) do
            if pattern:partialMatch(f) then
               print(f)
            end
         end
		 return
	  end

	  local stale = {}
	  local deleted = {}
	  if c.fileexists(project.staleFile) then
		 for sf in io.lines(project.staleFile) do
			if sf:sub(1,1) == "-" then 
			   deleted[sf:sub(2)] = true
			else
			   stale[sf] = true
			end
		 end
	  end
	  local shown = {}
	  for l in io.lines(project.filenamesFile) do
		 if not deleted[l] and pattern:partialMatch(l) then
			print(l)
			shown[l] = true
		 end
	  end
	  for k,v in pairs(stale) do
		 if not shown[k] and pattern:partialMatch(k) then
			print(k)
		 end
	  end
   end
end
defHelp(files,
		"Searches filenames in the project instead of file contents."
)

function PathIsInProject(project, path) 
   for k,v in pairs(project.tracked) do
      local p = v.path
      if path:sub(1, p:len()) == p then return true end
   end
   return false
end

function ProjectIsAuto(project)
   return (project.properties.auto == nil) or (project.properties.auto ~= false)
end

gPath = ""
function auto_search(path, options, regex, secondPhaseRegex)
   if path == "." then path = c.getcwd() end
   gPath = path
   --print(path, options, regex, secondPhaseRegex)
   for name, project in pairs(gProjects) do
      if PathIsInProject(project, path) and ProjectIsAuto(project) then
         c.fastpathsearch(name, options, regex, secondPhaseRegex)
      end
   end
end

function autofiles(path, regex)
   if path == "." then path = c.getcwd() end
   gPath = path
   for name, project in pairs(gProjects) do
      if PathIsInProject(project, path) and ProjectIsAuto(project) then
         files(name, regex)
      end
   end
end

function no_db_search(projectName, options, regex, secondPhaseRegex)
   local project = GetProjectOrDie(projectName)
   print("no_db_search", projectName, options, regex, secondPhaseRegex)
   local context = c.createSearchContext(gPath, options, regex, secondPhaseRegex)
   for f in IterateProjectFiles(project) do
      c.searchInLooseFile(context, f)
   end
   c.destroySearchContext(context)
end

function startservice()
   print("Press any key to stop project monitoring")
   while true do
      Log("Service scan")
      for k,v in pairs(gProjects) do
	 CreateProjectStaleFiles(v)
      end
      if c.waitForKeypress(GetMonitorPeriodInSeconds()) then return end
   end
end
defHelp(startservice,
"startservice activates the file watching portion of qgrep. qgrep does not        \
exit from this mode until a key is pressed. When qgrep is run with startservice   \
it will begin monitoring all projects for file changes.  qgrep currently only     \
supports active monitoring, meaning that it will periodically scan all files in   \
all projects to determine those that have changed.  This scan is relatively fast, \
but may still take several seconds for very large projects.                       \
How often the scan runs can be controlled by setting MonitorPeriodInMinutes is    \
your project.lua file, such as:                                                   \
MonitorPeriodInMinutes = 2                                                        \
The default scan rate is ten minutes.  It is possible for qgrep to return         \
inconsistent results if a file has changed and not yet been scanned."
)

function version()
   print("qgrep version " .. gVersion)
end
defHelp(version, 
"Displays the version string for qgrep")

function lua_api_help()
   c.lua_api_help()
end

function main(...)
   local newArgs = tableshift(arg)
   ExecuteCommandLine(newArgs)
end

function search_help_only() end
defHelp(search_help_only,
[[Search is the primary use of qgrep.  The search command will not 
interpret the projects.lua file if possible.  By default matches 
will be returned in a format that is the same as grep, which is
<filename>:<linenumber>:<line>
	 
Syntax is qgrep search <project> [-options] <regex> [secondPhaseRegex]
The options list must be prefixed with '-' 
Options are:
  f - search only filenames in the project 
  i - causes the search to be case insensitive
  l - treat the regex as a literal string
  s - print summary
  T - do not use the trigram algorithm when searching
  V - output matches in a format suitable for Visual Studio to jump to.
      The format is <filename> (<linenumber>):<line>
  \\\\ - replace forward slashes (/) with backslashes    
  / - replace backslashes (\\) with forward slashes

Replacing slashes may be useful for editors that expect a certain slash type.
<regex> - the regular expression to search for.  Syntax can be found at
          http://code.google.com/p/re2/wiki/Syntax

[secondPhaseRegex] - optional second filter stage.  The second stage regex
takes the approximate output from the first stage (including filenames) and
only prints if that output matches.  This phase is case insensitive.
This is approximately the same as 
  qgrep search <project> <regex> | grep -i <secondPhaseRegex>
]])
  
defHelp(build, "the T option disables trigraph indexing during the build")

defCommand(help,         "help",          "Provides further help for commands")
defCommand(build,        "build",         "<project> [T] regenerates the database for <project>")
defCommand(build_all,    "build-all",     "Builds all projects")
defCommand(listprojects, "projects",      "Lists all known projects")
defCommand(search_help_only,   "search",  "<project> [-filsTV/\\] <regex> [secondPhaseRegex] searches for <regex> in the given project.")
defCommand(scanstalefiles, "scan-stale",  "<project> scans for stale files in the project.  Does not rebuild project")
defCommand(files,        "files",         "<project> <regex> filters the filenames in <project> through <regex>")
defCommand(startservice, "start-service", "Begins monitoring all projects ")
defCommand(version,      "version",       "Prints the version")
defCommand(lua_api_help, "lua-api-help",  "Lists help for Lua API that QGrep exposes")
defCommand(auto_search,  "auto",          "<path> [-filsTV/\\] <regex> [secondPhaseRegex] like 'search', but will scan all projects that could include <path>")
defCommand(autofiles,  "autofiles",      "<path> <regex> like 'files', but will scan all projects that could include <path>")
defCommand(no_db_search, "no_db_search",  "projectName, options, regex, secondPhaseRegex")


------------- Project config file handling
configFile = c.qgreppath() .. "/projects.lua" 
if c.fileexists(configFile) then
   dofile(configFile)
else
   print("Unable to find project config file " .. configFile)
   print("Would you like a default config file to be created? [y/n]")
   local line = io.read()
   if line and line:lower() == "y" then
      c.mkdir(c.qgreppath())
      local f = io.open(configFile, "w")
      if not f then
	 print("Unable to create " .. configFile .. " sorry :(")
	 os.exit(0)
      end
      f:write(
---- RAW TEXT START
[[
Project{"exampleproject"
   ,track("basedirectory", "\\.c", "\\.h", "\\.lua")
   ,ignore(".*\\.o")
}
]]
---- RAW TEXT END
)
      print("Created!")
      f:close()
   end
   os.exit(0)
end
-------------------------------------------
-- Try to load plugins from the plugin path
local lua_files_regex = { c.regex(".*\.lua$") }

local plugins = c.getpluginpath()
if plugins then
   for i, plugin in pairs(filteredWalkDir(plugins, lua_files_regex, {})) do
	  dofile(plugin)
   end
end

-- Load plugins
for i, plugin in pairs(filteredWalkDir(c.qgreppath() .. "/plugins", lua_files_regex, {})) do
   dofile(plugin)
end


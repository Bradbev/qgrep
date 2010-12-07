------------- Util
-- Shift all indexes in a table down, dropping index 1
function tableshift(t)
   local ret = {}
   for i = 2, #t do
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

function filteredWalkDir(path, include, ignore)
   local ret = {}
   for f in walkdir(path, true) do
      if fullMatchAny(include, f) and not 
	 fullMatchAny(ignore, f) then
	 table.insert(ret, f)
      end
   end
   return ret
end

------------- Project handling
gProjects = {}

function tracked(x)
   return x.tracked
end

function track(path, ...)
   return { tracked = true, path = path, regexs = map(c.regex, arg) }
end

function ignored(x)
   return not tracked(x)
end

function ignore(...)
   return { regexs = map(c.regex, arg) }
end

gMonitorFrequency = 60
function MonitorFrequency(frequency)
   gMonitorFrequency = math.floor(frequency)
end

function Project(name, ...)
   local proj = { name = name }
   proj.tracked = filter(tracked, arg)
   
   -- flatten the ignored expressions
   local ignoreExprs = {}
   for k,v in ipairs(filter(ignored, arg)) do
      for k,v in ipairs(v.regexs) do
	 setInsert(ignoreExprs, v)
      end
   end
   proj.ignoreExprs = setToList(ignoreExprs)
   
   proj.archiveFile = c.igreppath() .. "/" .. name .. ".tgz"
   proj.staleFile = proj.archiveFile .. ".stalefiles"
   proj.filesInArchive = nil
   
   gProjects[name] = proj
end

function IterateProjectFiles(project)
   return coroutine.wrap(
	function ()
 	   for k,v in pairs(project.tracked) do
 	      for k,v in pairs(filteredWalkDir(v.path, v.regexs, project.ignoreExprs)) do
 		 coroutine.yield(v)
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
   project.filesInArchive = project.filesInArchive or iterateToTable(iterateArchive(project.archiveFile))
   local time = math.max(GetFileTime(project.archiveFile), 
			 GetFileTime(project.staleFile))
   
   local tmpname = os.tmpname()
   local file = io.open(tmpname, "w")
   local filenames = {}
   
   -- Catch the new & modified files
   for f in IterateProjectFiles(project) do
      filenames[f] = true
      if GetFileTime(f) > time then
	 file:write(f .. "\n")
      end
   end
   -- Catch the deleted files
   for k,v in pairs(project.filesInArchive) do
      if not filenames[v] then
	 file:write("-" .. v .. "\n")
      end
   end
   
   file:close()
   os.rename(tmpname, project.staleFile)
end
------------- Command Handling ------------
gCommands = {}

function defCommand(func, name, help, longhelp)
   gCommands[name] = { func = func, help = help, longhelp = longhelp }
end

function ExecuteCommandLine(args)
   local command = args[1] or ""
   local entry = gCommands[command]
   if entry then
      entry.func(unpack(tableshift(args)))
   else
      print("Unknown command " .. command)
      usage()
   end
end
-------------------------------------------

function usage()
   print("Commands for igrep")
   for k,v in pairs(gCommands) do
      print("\t" .. k .. "\t" .. v.help)
   end
end

function GetProjectOrDie(projectName)
   local project = gProjects[projectName]
   if not project then
      print("Unknown project " .. projectName)
      listprojects()
      os.exit(0)
   end
   return project
end

function help(command)
   entry = gCommands[command]
   if entry then
      print("Extended help for " .. command)
      print(entry.longhelp)
   else
      if command then print(command .. " is an unknown command") end
      usage();
   end
end

function build(projectName)
   local project = GetProjectOrDie(projectName)
   print("Building " .. projectName .. ", may take some time")
   local tmpname = os.tmpname()
   local a = archive.CreateArchive(tmpname)
   local count = 0
   for f in IterateProjectFiles(project) do
      count = count + 1
      archive.AddFileToArchive(a, f)
   end
   archive.CloseArchive(a)
   
   print("Added " .. count .. " files to archive");
   os.rename(tmpname, project.archiveFile)
   os.remove(project.staleFile)
   
   print("Done building")
end

function listprojects()
   print("Known projects")
   for k,v in pairs(gProjects) do
      print("\t" .. k)
   end
end

function startservice()
   print("Press any key to stop project monitoring")
   while true do
      for k,v in pairs(gProjects) do
	 CreateProjectStaleFiles(v)
      end
      if c.waitForKeypress(gMonitorFrequency) then return end
   end
end

function main(...)
   local newArgs = tableshift(arg)
   ExecuteCommandLine(newArgs)
end

defCommand(help, "help", "Provides further help for commands")
defCommand(build, "build", "<project> regenerates the database for <project>", "longhelp")
defCommand(listprojects, "projects", "Lists all known projects", "longhelp")
defCommand(nil, "search", "<project> [iV/\\] <regex> searches for <regex> in the given project", "longhelp")
defCommand(nil, "files", "<project> <regex> filters the filenames in <project> through <regex>", "longhelp")
defCommand(startservice, "start-service", "Begins monitoring all projects ", "longhelp")

------------- Project config file handling
configFile = c.igreppath() .. "/projects.lua" 
if c.fileexists(configFile) then
   dofile(configFile)
else
   print("Unable to find project config file " .. configFile)
end
-------------------------------------------

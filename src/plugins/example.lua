function ReportFileSize(projectName)
   local project = GetProjectOrDie(projectName)
   local files_and_sizes = {}
   print("Iterating files for " .. projectName .. ", may take some time")
   for f in IterateProjectFiles(project) do
      table.insert(files_and_sizes, { name = f, size = c.fileinfo(f).size })
   end
   table.sort(files_and_sizes, function(a,b) return a.size > b.size end)
   for _, v in pairs(files_and_sizes) do
      print(string.format("%s\t\t%s", v.size, v.name))
   end
end
defCommand(ReportFileSize,  "report-file-sizes", "Sorted list of all files in the project")

function LuaSearch(projectName, regex)
   local callback = function(filename, linenumber, line)
		       print(string.format("%s:%d:%s", filename, linenumber, line))
		    end
   local p = { 
      project = projectName, 
      regex = regex,
      callback = callback 
   }

   c.execute_search(p)
end
defCommand(LuaSearch,  "lua_search", "search using lua")

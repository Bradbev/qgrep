-- Using #hashtags and #@names inside source code 
-- How to do it:
-- qgrep for #\w+ & record every file that has a hash tag 
--   ignore pragma, if, end, elif, include, define, ifdef
-- do the same for #@\w+ for names
-- record which files have tags for the second pass
-- Gather all comment blocks from the file, search them for
-- tags & put those block ids into the map.
-- Generate HTML pages that let you browse/search 
-- Generate a trigram index for JS code that will hide comment blocks

function line_in_string(s)
   if s[#s] ~= "\n" then s = s .. "\n" end
   return s:gmatch(".-\n")
end

function extract_raw_comment_blocks_from_string(text, filename)
   local comments = {}
   local current_comment = nil
   local line_number = 1
   local function append_line(l) 
      if not current_comment then current_comment = { line = line_number } end
      table.insert(current_comment, l) 
   end
   local function end_comment_block() 
      if current_comment then
	 local comment = table.concat(current_comment) 
	 table.insert(comments, { filename = filename, line = current_comment.line, comment = comment })
	 current_comment = nil
      end
   end

   local in_comment = false
   for line in line_in_string(text) do
      local _, comment_start = line:find(".*/%*")
      local _, comment_end = line:find(".*%*/")
      local _, comment_line = line:find(".*//")
      
      if comment_line then
	 if comment_start and comment_line < comment_start then comment_start = nil end
	 if comment_end and comment_line < comment_end then comment_end = nil end
      end
      if comment_start then in_comment = true end
      if comment_end then in_comment = false end
      
      if comment_start and comment_end then
	 if comment_start > comment_end then 
	    in_comment = true
	 else
	    in_comment = false
	 end
      end
--      print("##", comment_start, comment_end, comment_line, in_comment, line)
      
      if in_comment or comment_line or comment_start or comment_end then
	 append_line(line)
      else
	 end_comment_block()
      end
      line_number = line_number + 1
   end
   return comments
end

function extract_tags_from_string(text)
   local exclude = {
      ["#define"] = true, 
      ["#defines"] = true, 
      ["#elif"] = true, 
      ["#else"] = true,
      ["#endif"] = true, 
      ["#error"] = true, 
      ["#if"] = true,
      ["#ifdef"] = true, 
      ["#ifndef"] = true, 
      ["#include"] = true,
      ["#line"] = true, 
      ["#pragma"] = true, 
      ["#undef"] = true, 
      ["#using"] = true, 
   }
   local ret = {}
   for t in text:lower():gmatch("[#]@?%a%a[%a]+") do
	  local tag = t:gsub("#@", "@")
      if not exclude[tag] then
		 ret[tag] = true
      end
   end
   return ret
end

function table_intersection(t1, t2)
   local ret = {}
   for k,v in pairs(t1) do
      if t2[k] then
	 ret[k] = true
      end
   end
   return ret
end

function extract_trigrams_from_string(s)
   local ret = {}
   for i=1,#s-2 do
      local str = s:sub(i, i+2):lower()
      if str and not str:find('\n') then
	 ret[str] = true
      end
   end
   return ret
end

function build_tag_db_from_comment_blocks(comments)
   local tags = {}
   local trigrams = {}
   local comments_with_tags = {}
   for _, comment in pairs(comments) do
      local comment_has_tags = false
      for tag, _ in pairs(extract_tags_from_string(comment.comment)) do
	 comment_has_tags = true
	 tags[tag] = (tags[tag] or 0) + 1
      end
      if comment_has_tags then
	 table.insert(comments_with_tags, comment)
      end
   end
   for i, comment in pairs(comments_with_tags) do
      for trigram, _ in pairs(extract_trigrams_from_string(comment.comment)) do
	 local g = trigrams[trigram] or {}
	 g[i] = true
	 trigrams[trigram] = g
      end
   end   
   return { comments = comments_with_tags, tags = tags, trigrams = trigrams}
end

function output_html(project_name, outfile, db)
   local f = io.open(outfile, "w")
   if not f then
	  print("Unable to open ", outfile)
	  os.exit(0)
   end
   local function emit(...)
      f:write(string.format(...))
   end
   local function escape_string(k)
      return k:gsub("['\"\\\r\n]", "\\%1")
   end
   
   for line in io.lines(c.qgreppath() .. "/plugins/hashtagfiles/tags_template.html") do
      emit("%s\n", line)
      if line:find("<body>") then
	 --emit([[<script src="jquery-1.8.3.min.js"></script>]])
	 -- emit Jquery inline
	 local jq = io.open(c.qgreppath() .. "/plugins/hashtagfiles/jquery-1.8.3.min.js")
	 emit("<script>%s</script>\n", jq:read("*a"))
	 jq:close()
	 emit("<script>\n")
	 
	 -- emit trigram datastructure
	 emit("var trigrams = {\n")
	 for k,v in pairs(db.trigrams) do
	    emit("'%s' : {", escape_string(k))
	    for comment_id in pairs(v) do
	       emit("%d : true,", comment_id)
	    end
	    emit("},\n")
	 end 
	 emit("};\n")
	 
	 emit("var comments = ['0 offset dummy comment', ")
	 for i, comments in pairs(db.comments) do
	    emit('"%s",\n', escape_string(comments.comment))
	 end
	 emit("]\n\n")
	 emit("var project_name = '%s';\n", project_name)

	 -- emit my script file
	 local js = io.open(c.qgreppath() .. "/plugins/hashtagfiles/tag_script.js")
	 emit("%s</script>\n", js:read("*a"))
	 js:close()
      end
   
      if line:find('id="searchbox"') then
	 -- populate inline comments
	 for i, comment in pairs(db.comments) do
	    local filename = comment.filename
	    emit('<div class="comment" id="%d"><pre><a class="comment_aref" href="%s">%s</a>\n%s</pre></div>', i, filename, filename, comment.comment)
	 end
      end
      
      if line:find('id="tag_column"') then
	 -- populate inline tags
	 local sorted_tags = {}
	 for tag, _ in pairs(db.tags) do
	    table.insert(sorted_tags, tag)
	 end
	 table.sort(sorted_tags)
	    
	 local have_emitted_at_header = false
	 for _, tag in pairs(sorted_tags) do
	    if have_emitted_at_header == false and tag:sub(1, 1) == "@" then
	       have_emitted_at_header = true
	       emit('<h2>Known names</h2>\n')
	    end
	    emit('<a class="tag_aref" href="%s">%s</a><br>', tag, tag)
	 end
      end
   end
   
   f:close()
end

function MakeHashTagHTML(projectName, outputfile, secondPhaseRegex)
   local project = GetProjectOrDie(projectName)
   local target_files = {}
   local target_file_count = 0
   local secondRegex = secondPhaseRegex and c.regex(secondPhaseRegex)
   local callback = function(filename, linenumber, line)
					   if target_files[filename] == nil and next(extract_tags_from_string(line)) then
						  if secondRegex == nil or secondRegex:partialMatch(filename) then
							 target_files[filename] = true
							 target_file_count = target_file_count + 1
						  end
					   end
		    end
   local p = { 
      project = projectName, 
      regex = ".*\\s[#]\\S\\S\\S+.*",
      callback = callback 
   }
   c.execute_search(p)
   
   local comment_blocks = {}
   local seen = 0
   for filename, data in iterateArchiveWithData(project.archiveFile) do
      if target_files[filename] then
	 seen = seen + 1
	 print(string.format("%d/%d : %s", seen, target_file_count, filename))
	 for _, c in pairs(extract_raw_comment_blocks_from_string(data, filename)) do
	    table.insert(comment_blocks, c)
	 end
      end
   end
   local db = build_tag_db_from_comment_blocks(comment_blocks)
   output_html(projectName, outputfile, db)
end

if not RUNNING_TESTS then
defCommand(MakeHashTagHTML,  "make-hashtags", "<project> <outputfile>")
end


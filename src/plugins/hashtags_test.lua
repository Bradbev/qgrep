require("luatest")
require("hashtags")

--[[
Define the tag DB structure
--
lower case trigram splitter
--]]

local datafile = io.open("test_data.cpp")
local data = datafile:read("*a")
datafile:close()

c = {}
function c.qgreppath() return ".." end

function test_line_in_string()
   local data = [[a
b

c]]
   local i = line_in_string(data)
   check(i() == "a\n")
   check(i() == "b\n")
   check(i() == "\n")
   check(i() == "c\n")
   check(i() == nil)
end

function test_extract_trigrams_from_string()
   local s = "abc Def #g"
   local grams = { "abc", "bc ", "c d", " de", "def", "ef ", "f #", " #g" }
   local expected_grams = {}
   for _, g in pairs(grams) do expected_grams[g] = true end
   checkeq(expected_grams, extract_trigrams_from_string(s))
end

function test_table_intersection()
   local t1 = { [1] = true, [2] = true, [3] = true }
   local t2 = { [1] = true, [3] = true }
   checkeq(table_intersection(t1,t2), t2)
end

-- This is here as reference.  I really only need the 
-- search implementation in the JS script
function search_trigrams(grams, to_find)
   to_find = to_find:lower()
   local active_grams = nil
   for k,v in pairs(extract_trigrams_from_string(to_find)) do
      local g = grams[k]
      if g == nil then return {} end
      if active_grams == nil then 
	 active_grams = g 
      else
	 active_grams = table_intersection(active_grams, g)
      end
   end
   local ret = {}
   for k,v in pairs(active_grams or {}) do
      if k:find(to_find) then
	 ret[k] = true
      end
   end
   return ret
end

function test_trigram_search()
   local s1 = "abcdefzzz bcz czz"
   local s2 = "defghi"
   local grams = {}
   function populate_gram(s)
      for trigram, _ in pairs(extract_trigrams_from_string(s)) do
	 grams[trigram] = grams[trigram] or {}
	 grams[trigram][s] = true
      end
   end
   populate_gram(s1)
   populate_gram(s2)
   checkeq(search_trigrams(grams, "abcd"), { [s1] = true })
   checkeq(search_trigrams(grams, "def"), { [s1] = true , [s2] = true })
   checkeq(search_trigrams(grams, "ghi"), { [s2] = true })
   checkeq(search_trigrams(grams, "abcdef"), { [s1] = true })
   checkeq(search_trigrams(grams, "abcdefg"), { })
   checkeq(search_trigrams(grams, "f"), { })
   checkeq(search_trigrams(grams, "abczzz"), { })
end

function test_build_tag_db_from_comment_blocks()
   local comment_blocks = extract_raw_comment_blocks_from_string(data, "filename")
   local db = build_tag_db_from_comment_blocks(comment_blocks)
   check(db)
   local comments = db.comments
   check(#comments == 3)
   local tags = db.tags
   local expected_tags = { "#block1", "#block2", "#block3", "@name" }
   local tag_form_expected = {}
   for _, v in pairs(expected_tags) do tag_form_expected[v] = 1 end
   checkeq(tags, tag_form_expected)
   local trigrams = db.trigrams
   check(trigrams["#bl"])
   checkeq(trigrams["#bl"], { [1] = true, [2] = true, [3] = true})
   checkeq(trigrams["ck1"], { [1] = true })
end

function test_extract_tags_from_string()
   local data = "#TAG1\n#tag2 #tag3 @name"
   local tags = extract_tags_from_string(data)
   local i = 0
   for _ in pairs(tags) do i = i + 1 end
   check(i == 4)
   check(tags["#tag1"])
   check(tags["#tag2"])
   check(tags["#tag3"])
   check(tags["@name"])
end

function test_extract_tags_from_string_exclude()
   local data = "#if #ifdef #endif #elif #pragma #define #line #using #undef"
   local tags = extract_tags_from_string(data)
   local i = 0
   for _ in pairs(tags) do i = i + 1 end
   check(i == 0)
end

function test_output_html()
   local comment_blocks = extract_raw_comment_blocks_from_string(data, "./test_data.cpp")
   local db = build_tag_db_from_comment_blocks(comment_blocks)
   output_html("tags", "tags.html", db)
end

function test_extract_raw_comment_blocks_from_string()
   local comment_blocks = extract_raw_comment_blocks_from_string(data, "filename")
   check(comment_blocks)
   checkeq({ filename = "filename", line = 16, comment = 
[[// comment block 1, line 1/2 #block1
// comment block 1, line 2/2
]]}, 
comment_blocks[1])
   
   checkeq({ filename = "filename", line = 19, comment = 
[[// ABSTRACT THESE HEADERS
]]}, 
comment_blocks[2])
   
   checkeq({ filename = "filename", line = 22, comment = 
[[/* comment block 2, line 1/2  #block2 */
/* comment block 2, line 2/2 */
]]}, 
comment_blocks[3])

   checkeq({ filename = "filename", line = 32, comment = 
[[/*
 * Comment block 3, line 2/4  #block3
 * Comment block 3, line 3/4  @name
 */
]]}, 
comment_blocks[4])
   
   checkeq({ filename = "filename", line = 50, comment = 
[[
    //LuaRepl::ListenForDebugger(PORTNO, 0);
]]}, 
comment_blocks[5])
   
checkeq({ filename = "filename", line = 67, comment = 
[[
	// input_safe_poll can't throw errors
]]}, 
comment_blocks[6])
   
checkeq({ filename = "filename", line = 82, comment = 
[[
// testing comments /*
]]}, 
comment_blocks[7])

checkeq({ filename = "filename", line = 84, comment = 
[[
/* a */ /* b */
]]}, 
comment_blocks[8])

end

run_all_tests()


-- reads std in and filters to std out
-- filter blocks look like 
-- <!-- begin:TAG -->
-- <!-- end:TAG -->
-- to keep tags in the filtered output put them on the command line

gTags = {}

for k,v in ipairs(arg) do
   gTags[string.lower(v)] = true
end

gPrinting = true

for line in io.lines() do
   local begin = string.match(line, "<!%-%- begin:(.*) %-%->")
   local endTag = string.match(line, "<!%-%- end")
   if begin then 
      if not gTags[string.lower(begin)] then
	 gPrinting = false
      end
   elseif endTag then 
      gPrinting = true
   elseif gPrinting then
      print(line)
   end
end

FILES = readme.text
HTML_FILES = $(patsubst %.text,html/%.html, $(FILES))
TXT_FILES = $(patsubst %.text,txt/%.txt, $(FILES))

html/%.html: %.text Makefile
	@echo Processing $<
	@cat header.html > $@
	@cat $< | lua untangle.lua html | markdown >> $@
	@cat footer.html >> $@

txt/%.txt: %.text Makefile
	@echo Text processing $<
	@cat $< | lua untangle.lua txt >> $@

all:	$(HTML_FILES) $(TXT_FILES)

dist: all
	cp txt/* $(DIST_DIR)

clean:
	rm html/*
	rm txt/*

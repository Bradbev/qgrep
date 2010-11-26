ifeq ($(shell uname -s), Darwin)
PLATFORM = osx
GREP_LIB = libigrep.dylib
GREP_EXE = igrepll
endif

ifeq ($(shell uname -s), MINGW32_NT-5.1)
PLATFORM = win32
GREP_LIB = libigrep.dll
GREP_EXE = igrepll.exe
endif

export BASE_DIR = $(PWD)
export DIST_DIR = $(PWD)/dist/$(PLATFORM)

dist:
	mkdir -p $(DIST_DIR)
	make -C src/cpp dist
	make -C src/py dist
	cp src/cpp/$(GREP_LIB) $(DIST_DIR)
	cp src/cpp/$(GREP_EXE) $(DIST_DIR)
ifeq ($(shell uname -s), MINGW32_NT-5.1)
	cp packages/libs_w32/* $(DIST_DIR)
endif

.PHONY: dist

dist_clean:
	rm -rf ./dist

.PHONY: dist_clean


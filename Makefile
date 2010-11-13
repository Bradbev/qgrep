ifeq ($(shell uname -s), Darwin)
PLATFORM = osx
GREP_LIB = libigrep.dylib
GREP_EXE = igrep
endif

ifeq ($(shell uname -s), MINGW32_NT-5.1)
PLATFORM = win32
GREP_LIB = libigrep.dll
GREP_EXE = igrep.exe
endif

DIST_DIR = ./dist/$(PLATFORM)

dist:
	mkdir -p $(DIST_DIR)
	make -C src/cpp dist
	cp src/py/*.py $(DIST_DIR)
	cp src/cpp/$(GREP_LIB) $(DIST_DIR)
	cp src/cpp/$(GREP_EXE) $(DIST_DIR)
	chmod +x $(DIST_DIR)/igrep.py

.PHONY: dist

dist_clean:
	rm -rf ./dist

.PHONY: dist_clean


ifeq ($(shell uname -s), Darwin)
PLATFORM = osx
endif

ifeq ($(shell uname -s), MINGW32_NT-5.1)
PLATFORM = win32
endif

export BASE_DIR = $(PWD)
export DIST_DIR = $(PWD)/dist/$(PLATFORM)

dist:
	rm -rf $(DIST_DIR)
	mkdir -p $(DIST_DIR)
	make -C src/cpp dist
	cp packages/libs_$(PLATFORM)/* $(DIST_DIR)

.PHONY: dist

dist_clean:
	rm -rf ./dist

.PHONY: dist_clean


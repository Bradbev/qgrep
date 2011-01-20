######################## OSX specific
ifeq ($(shell uname -s), Darwin)
PLATFORM = osx

lib_copy:
	cp packages/libs_$(PLATFORM)/* $(DIST_DIR)
	install_name_tool -change /usr/lib/libarchive.2.dylib libarchive.2.dylib $(DIST_DIR)/igrep
	install_name_tool -change obj/so/libre2.so.0 libre2.so.0 $(DIST_DIR)/igrep
	install_name_tool -change /usr/lib/libbz2.1.0.dylib libbz2.1.0.dylib $(DIST_DIR)/libarchive.2.dylib
	install_name_tool -change /usr/lib/libz.1.dylib libz.1.dylib $(DIST_DIR)/libarchive.2.dylib

#	mv $(DIST_DIR)/libre2.so.0 $(DIST_DIR)/libre2.so.1
#	mv $(DIST_DIR)/libarchive.2.dylib $(DIST_DIR)/libarchive.3.dylib
.PHONY: lib_copy
endif
####################################################

######################## Win32/MINGW specific
ifeq ($(shell uname -s), MINGW32_NT-5.1)
PLATFORM = win32

lib_copy:
	cp packages/libs_$(PLATFORM)/*.dll $(DIST_DIR)
	cp packages/libs_$(PLATFORM)/*.so.0 $(DIST_DIR)
	strip $(DIST_DIR)/*
.PHONY: lib_copy

installer: dist
	rm -f installer/win32/setup.exe
	"c:\Program Files\Inno Setup 5\ISCC.exe" installer/win32/igrep.iss
.PHONY: installer

endif
####################################################

export BASE_DIR = $(PWD)
export DIST_DIR = $(PWD)/dist/$(PLATFORM)

dist:
	rm -rf $(DIST_DIR)
	mkdir -p $(DIST_DIR)
	make -C src/cpp dist
	make -C doc dist
	make lib_copy

.PHONY: dist

dist_clean:
	rm -rf ./dist

.PHONY: dist_clean


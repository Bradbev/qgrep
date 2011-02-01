######################## OSX specific
ifeq ($(shell uname -s), Darwin)
PLATFORM = osx

lib_copy:
.PHONY: lib_copy

installer: dist
	rm -f installer/osx/*
	sh osxinst.sh
.PHONY: installer
endif

upload: installer
	ftp -u qgrepco1@qgrep.com:/www/index.html doc/html/readme.html
	ftp -u qgrepco1@qgrep.com:/www/qgrep-osx.tgz installer/osx/qgrep-osx.tgz
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


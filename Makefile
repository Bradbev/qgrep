######################## OSX specific
ifeq ($(shell uname -s), Darwin)
PLATFORM = osx
docs:
	make -C doc dist
	cp doc/txt/readme.txt ./README.markdown
.PHONY: docs

lib_copy:
.PHONY: lib_copy

installer: dist
	rm -f installer/osx/*
	sh osxinst.sh
.PHONY: installer
endif

upload: docs
	#ftp -u qgrepco1@qgrep.com:/www/index.html doc/html/readme.html
	#ftp -u qgrepco1@qgrep.com:/www/qgrep-osx.tgz installer/osx/qgrep-osx.tgz
	#ftp -u qgrepco1@qgrep.com:/www/setup.exe installer/win32/setup.exe
####################################################

######################## Win32/MINGW specific
ifeq ($(shell uname -s), MINGW32_NT-6.1)
PLATFORM = win32

docs:
	echo "Not making docs for win32"
.PHONY: docs

lib_copy:
	cp packages/libs_$(PLATFORM)/*.dll $(DIST_DIR)
	cp packages/libs_$(PLATFORM)/*.so.0 $(DIST_DIR)
	strip $(DIST_DIR)/*
.PHONY: lib_copy

installer: dist
	rm -f installer/win32/setup.exe
	"c:\Program Files (x86)\Inno Setup 5\ISCC.exe" installer/win32/igrep.iss
	cp installer/win32/setup.exe dist/win32/
.PHONY: installer

endif
####################################################

export BASE_DIR = $(PWD)
export DIST_DIR = $(PWD)/dist/$(PLATFORM)

dist: 
	rm -rf $(DIST_DIR)
	mkdir -p $(DIST_DIR)
	make -C src/cpp dist
	make lib_copy

.PHONY: dist

dist_clean:
	rm -rf ./dist

.PHONY: dist_clean


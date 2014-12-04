ifeq ($(shell uname -s), Darwin)
include Makefile.osx
else ifeq ($(shell uname -o), Msys)
include Makefile.win32
else ifeq ($(shell uname -s), Linux)
include Makefile.linux
endif


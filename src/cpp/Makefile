ifeq ($(shell uname -s), Darwin)
include Makefile.osx
endif

ifeq ($(shell uname -o), Msys)
include Makefile.win32
endif

ifeq ($(shell uname -s), Linux)
include Makefile.linux
endif


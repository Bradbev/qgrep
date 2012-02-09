ifeq ($(shell uname -s), Darwin)
include Makefile.osx
endif

ifeq ($(shell uname -s), MINGW32_NT-5.1)
include Makefile.win32
endif

ifeq ($(shell uname -s), Linux)
include Makefile.linux
endif

stream_test: stream.cpp stream.h yarn.o
	g++ -DSTREAM_TEST_MAIN -DLOCK_FREE stream.cpp yarn.o -o stream_test
	g++ -DSTREAM_TEST_MAIN stream.cpp yarn.o -o stream_test_lock

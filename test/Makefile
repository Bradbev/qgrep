BASE = ../../packages

ARCHIVE = $(BASE)/libarchive-2.8.4
INCLUDES = -I $(ARCHIVE)/libarchive/ 
LIBDIR = -L$(ARCHIVE)/.libs/
LIBS = $(LIBDIR) -larchive -lre2

SRCS = cpptest.cpp
OBJS = $(SRCS:.cpp=.o)

CFLAGS = -O0 -g -Wall
CPPFLAGS = $(CFLAGS)

cpptest: $(OBJS)
	make -C ../src/cpp libigrep.dylib
	g++ -o cpptest $(OBJS) libigrep.dylib

.PHONY: clean
clean:
	rm -f *.o cpptest
	make -C ../src/cpp clean

BASE = ../../../packages

ARCHIVE = $(BASE)/libarchive-2.8.4
INCLUDES = -I $(ARCHIVE)/libarchive/ 
LIBDIR = -L$(ARCHIVE)/.libs/
LIBS = $(LIBDIR) -larchive -lre2

IGREP_SRCS = archive.cpp stream.cpp
IGREP_OBJS = $(IGREP_SRCS:.cpp=.o) yarn.o

CFLAGS = -O3 -Wall -Werror -fPIC
CPPFLAGS = $(CFLAGS)

all: libigrep.dylib igrep

libigrep.dylib: $(IGREP_OBJS)
	@echo $(IGREP_OBJS)
	g++ -dynamiclib -o libigrep.dylib $(IGREP_OBJS) $(LIBS)

igrep: libigrep.dylib igrep.o
	g++ -o igrep igrep.o libigrep.dylib $(CFLAGS)

clean:
	rm *.o *.dylib

.PHONY: clean

dist: clean all
	cp igrep libigrep.dylib ../../dist/

.PHONY: dist

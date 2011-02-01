# igrep - Super fast full text grep for programmers

-----------------------

## Introduction 
Programmers commonly need to search code, faster searching makes for
more productivity. igrep is significantly faster than search tools
that don't build a database and is very easy to setup - simple
projects need only a couple of lines of configuration.  
igrep runs on OS X and Windows, with a Linux port coming soon.

## Installation 

### OSX 
[Download](nofile) and extract to a directory of your choice.  

### Linux 
Coming soon, please email <brad@igrep.com> to express your
interest.

### Windows 
[Download](setup) and run the Windows installer.

## Configuration and Usage 
igrep expects to find a configuration file in a specific place.  For
*nix systems the file is `~/.igrep/projects.lua`

For Windows the file is `$(HOME)/.igrep/projects.lua` if it exists, or
`$(HOMEDRIVE)$(HOMEPATH)/.igrep/projects.lua` otherwise.

If the projects.lua file cannot be found, igrep will output an error
message with the path where it is expecting to find the file. A large
part of igrep's functionality is written in Lua, and Lua is also used
for project configuration.  Every igrep command except **search**
results in the `projects.lua` file being executed.  The full Lua
language is able to be used from `projects.lua`, though it should be
kept simple.  The **search** command is intended to be as fast as
possible (the whole point of igrep!), and will run without processing
the `projects.lua` file.

### Example

    Project{"linux", 
        track("~/source/linux-2.6", "\\.c$", "\\.h$"),
        ignore("f.o", "^bar\\.c$") 
    }

**Project** is a Lua function that accepts a table as its only
  argument, the table should be a comma seperated list within {}.
  The first element in the table *must* be a string and is used to
  name the project, "linux" in our example case.  There can be any
  number of **track** or **ignore** entries after the name of the
  project.

**track(path, patterns...)** tells igrep to add *path* to the project,
  and will recursively add all subdirectories.  *patterns* is a list
  of regular expression patterns, any files under path that match any
  pattern will be added.  In the example, we wish to add any file that
  ends in '.c' or '.h'.  You can have any number of patterns after the
  path entry. Having no patterns will result in all files being
  added.  Because the project file is just Lua code, backslashes must
  be escaped.  In the above example `\\.c` becomes the regex `\.c`, so
  that `.` will be matched as the period character rather than the
  usual regex meaning of ". means match any character"

**ignore(patterns...)** tells igrep to ignore files that match any
  pattern.  There can be multiple ignore entries.  In our example, any
  filename that contains the pattern "f.o" will be ignored.  Also any
  file that is exactly named "bar.c" will be ignored.  The exactness
  is because the pattern is surrouded by `^` and `$`.  The second
  ignore pattern will never be matched because filenames are tested
  with their full path, ie all files will start with
  `~/source/linux-2.6/` 

## Benchmarks 
All benchmarks were taken on my MacBook Pro, which has a 2.26 Core Duo
CPU and a 5400rpm HDD. I am searching for a text string that is not in
the target corpus, this gives a good picture of 'worst case'
performance and how quickly the entire corpus can be searched.

The first timing is using regular find & grep to search for the text
string "NotFound_", and takes 27 seconds:

       $ time find . -iname "*.c" -or -iname "*.h" -exec grep NotFound_ {} \; 
       real 0m27.173s 
       user 0m5.991s 
       sys 0m18.970s

In comparison, searching with igrep takes 2.2 seconds - more than 10
times faster

      $ time igrep search linux NotFound_
       real 0m2.211s 
       user 0m3.035s 
       sys 0m0.142s 
       
igrep does need to pay a cost to generate its internal database, but
that cost is roughly similar to doing a single search with find/grep

     $ time igrep build linux 
     Building linux, may take some time Added
     26425 files to archive Done building
     real 0m25.307s 
     user 0m21.153s 
     sys 0m1.257s
     
## Please Donate!
If you find igrep to be useful a donation would be excellent
motivation for me to make further improvements (such as much improved
multicore support!).  
If total donations exceed $1000 Canadian dollars, I will release
igrep under a permissive licence like BSD.  

LINK TO PAYPAL AREA


## Credits 
igrep uses the following libraries, many thanks to the authors

- [RE2](http://code.google.com/p/re2/) provides the regular expression
code used for matching
- [libarchive](http://code.google.com/p/libarchive/) provides the code
used for compressing and decompressing the search material
- [PThreads-w32](http://sourceware.org/pthreads-win32/) provides
threading on Windows
- [Lua](http://www.lua.org/); much of the functionality of igrep is
programmed in Lua.


# igrep - Super fast full text grep for programmers

-----------------------

## Introduction 
Programmers commonly need to search code, faster searching makes for
more productivity. igrep is significantly faster than search tools
that don't build a database and is very easy to setup - simple
projects need only a couple of lines of configuration.  
igrep runs on OS X and Windows, with a Linux port coming soon.

## Installation 

### OSX
Currently igrep runs on OSX 10.5 or later on a 64bit Intel system.
[Download](nofile) and extract to a directory of your choice.  

### Linux 
Coming soon, please email <brad@igrep.com> to express your
interest.

### Windows 
[Download](setup) and run the Windows installer.

## Configuration and Usage 
igrep expects to find a configuration file in a specific place.  For
*nix systems the file is `~/.igrep/projects.lua`

For Windows the file is `$(HOME)/.igrep/projects.lua` if it exists, or
`$(HOMEDRIVE)$(HOMEPATH)/.igrep/projects.lua` otherwise.

If the projects.lua file cannot be found, igrep will output an error
message with the path where it is expecting to find the file. A large
part of igrep's functionality is written in Lua, and Lua is also used
for project configuration.  Every igrep command except **search**
results in the `projects.lua` file being executed.  The full Lua
language is able to be used from `projects.lua`, though it should be
kept simple.  The **search** command is intended to be as fast as
possible (the whole point of igrep!), and will run without processing
the `projects.lua` file.

### Example

    Project{"linux", 
        track("~/source/linux-2.6", "\\.c$", "\\.h$"),
        ignore("f.o", "^bar\\.c$") 
    }

**Project** is a Lua function that accepts a table as its only
  argument, the table should be a comma seperated list within {}.
  The first element in the table *must* be a string and is used to
  name the project, "linux" in our example case.  There can be any
  number of **track** or **ignore** entries after the name of the
  project.

**track(path, patterns...)** tells igrep to add *path* to the project,
  and will recursively add all subdirectories.  *patterns* is a list
  of regular expression patterns, any files under path that match any
  pattern will be added.  In the example, we wish to add any file that
  ends in '.c' or '.h'.  You can have any number of patterns after the
  path entry. Having no patterns will result in all files being
  added.  Because the project file is just Lua code, backslashes must
  be escaped.  In the above example `\\.c` becomes the regex `\.c`, so
  that `.` will be matched as the period character rather than the
  usual regex meaning of ". means match any character"

**ignore(patterns...)** tells igrep to ignore files that match any
  pattern.  There can be multiple ignore entries.  In our example, any
  filename that contains the pattern "f.o" will be ignored.  Also any
  file that is exactly named "bar.c" will be ignored.  The exactness
  is because the pattern is surrouded by `^` and `$`.  The second
  ignore pattern will never be matched because filenames are tested
  with their full path, ie all files will start with
  `~/source/linux-2.6/` 

## Benchmarks 
All benchmarks were taken on my MacBook Pro, which has a 2.26 Core Duo
CPU and a 5400rpm HDD. I am searching for a text string that is not in
the target corpus, this gives a good picture of 'worst case'
performance and how quickly the entire corpus can be searched.

The first timing is using regular find & grep to search for the text
string "NotFound_", and takes 27 seconds:

       $ time find . -iname "*.c" -or -iname "*.h" -exec grep NotFound_ {} \; 
       real 0m27.173s 
       user 0m5.991s 
       sys 0m18.970s

In comparison, searching with igrep takes 2.2 seconds - more than 10
times faster

      $ time igrep search linux NotFound_
       real 0m2.211s 
       user 0m3.035s 
       sys 0m0.142s 
       
igrep does need to pay a cost to generate its internal database, but
that cost is roughly similar to doing a single search with find/grep

     $ time igrep build linux 
     Building linux, may take some time Added
     26425 files to archive Done building
     real 0m25.307s 
     user 0m21.153s 
     sys 0m1.257s
     
## Please Donate!
If you find igrep to be useful a donation would be excellent
motivation for me to make further improvements (such as much improved
multicore support!).  
If total donations exceed $1000 Canadian dollars, I will release
igrep under a permissive licence like BSD.  

LINK TO PAYPAL AREA


## Credits 
igrep uses the following libraries, many thanks to the authors

- [RE2](http://code.google.com/p/re2/) provides the regular expression
code used for matching
- [libarchive](http://code.google.com/p/libarchive/) provides the code
used for compressing and decompressing the search material
- [PThreads-w32](http://sourceware.org/pthreads-win32/) provides
threading on Windows
- [Lua](http://www.lua.org/); much of the functionality of igrep is
programmed in Lua.


# qgrep (Quick Grep)
Super fast full text grep for programmers

-----------------------

## Introduction 
Programmers commonly need to search code, faster searching makes for
more productivity. igrep is significantly faster than search tools
that don't build a database and is very easy to setup - simple
projects need only a couple of lines of configuration.  
igrep runs on OS X and Windows, with a Linux port coming soon.

## Installation 

### OSX
Currently igrep runs on OSX 10.5 or later on a 64bit Intel system.
[Download](nofile) and extract to a directory of your choice.  

### Linux 
Coming soon, please email <brad@igrep.com> to express your
interest.

### Windows 
[Download](setup) and run the Windows installer.

## Configuration and Usage 
igrep expects to find a configuration file in a specific place.  For
*nix systems the file is `~/.igrep/projects.lua`

For Windows the file is `$(HOME)/.igrep/projects.lua` if it exists, or
`$(HOMEDRIVE)$(HOMEPATH)/.igrep/projects.lua` otherwise.

If the projects.lua file cannot be found, igrep will offer to create
the expected directory and config file. A large part of igrep's
functionality is written in Lua, and Lua is also used for project
configuration.  Every igrep command except **search** results in the
`projects.lua` file being executed.  The full Lua language is able to
be used from `projects.lua`, though it should be kept simple.  The
**search** command is intended to be as fast as possible (the whole
point of igrep!), and will run without interpreting the `projects.lua`
file. 

### Example projects.lua file

    Project{"linux", 
        track("~/source/linux-2.6", "\\.c$", "\\.h$"),
        ignore("f.o", "^bar\\.c$") 
    }

**Project** is a Lua function that accepts a table as its only
  argument, the table should be a comma seperated list within {}.
  The first element in the table *must* be a string and is used to
  name the project, "linux" in our example case.  There can be any
  number of **track** or **ignore** entries after the name of the
  project.

**track(path, patterns...)** tells igrep to add *path* to the project,
  and will recursively add all subdirectories.  *patterns* is a list
  of regular expression patterns, any files under path that match any
  pattern will be added.  In the example, we wish to add any file that
  ends in '.c' or '.h'.  You can have any number of patterns after the
  path entry. Having no patterns will result in all files being
  added.  Because the project file is just Lua code, backslashes must
  be escaped.  In the above example `\\.c` becomes the regex `\.c`, so
  that `.` will be matched as the period character rather than the
  usual regex meaning of ". means match any character"

**ignore(patterns...)** tells igrep to ignore files that match any
  pattern.  There can be multiple ignore entries.  In our example, any
  filename that contains the pattern "f.o" will be ignored.  Also any
  file that is exactly named "bar.c" will be ignored.  The exactness
  is because the pattern is surrouded by `^` and `$`.  The second
  ignore pattern will never be matched because filenames are tested
  with their full path, ie all files will start with
  `~/source/linux-2.6/` 

## Benchmarks 
All benchmarks were taken on my MacBook Pro, which has a 2.26 Core Duo
CPU and a 5400rpm HDD. I am searching for a text string that is not in
the target corpus, this gives a good picture of 'worst case'
performance and how quickly the entire corpus can be searched.

The first timing is using regular find & grep to search for the text
string "NotFound_", and takes 27 seconds:

       $ time find . -iname "*.c" -or -iname "*.h" -exec grep NotFound_ {} \; 
       real 0m27.173s 
       user 0m5.991s 
       sys 0m18.970s

In comparison, searching with igrep takes 2.2 seconds - more than 10
times faster

      $ time igrep search linux NotFound_
       real 0m2.211s 
       user 0m3.035s 
       sys 0m0.142s 
       
igrep does need to pay a cost to generate its internal database, but
that cost is roughly similar to doing a single search with find/grep

     $ time igrep build linux 
     Building linux, may take some time Added
     26425 files to archive Done building
     real 0m25.307s 
     user 0m21.153s 
     sys 0m1.257s
     
## Please Donate!
If you find igrep to be useful a donation would be excellent
motivation for me to make further improvements (such as much improved
multicore support!).  
If total donations exceed $1000 Canadian dollars, I will release
igrep under a permissive licence like BSD.  

LINK TO PAYPAL AREA


## Credits 
igrep uses the following libraries, many thanks to the authors

- [RE2](http://code.google.com/p/re2/) provides the regular expression
code used for matching
- [libarchive](http://code.google.com/p/libarchive/) provides the code
used for compressing and decompressing the search material
- [PThreads-w32](http://sourceware.org/pthreads-win32/) provides
threading on Windows
- [Lua](http://www.lua.org/); much of the functionality of igrep is
programmed in Lua.


# qgrep (Quick Grep) Super fast full text grep for programmers

-----------------------

## Introduction 
Programmers commonly need to search code, faster searching makes for
more productivity. igrep is significantly faster than search tools
that don't build a database and is very easy to setup - simple
projects need only a couple of lines of configuration.  
igrep runs on OS X and Windows, with a Linux port coming soon.

## Installation 

### OSX
Currently igrep runs on OSX 10.5 or later on a 64bit Intel system.
[Download](nofile) and extract to a directory of your choice.  

### Linux 
Coming soon, please email <brad@igrep.com> to express your
interest.

### Windows 
[Download](setup) and run the Windows installer.

## Configuration and Usage 
igrep expects to find a configuration file in a specific place.  For
*nix systems the file is `~/.igrep/projects.lua`

For Windows the file is `$(HOME)/.igrep/projects.lua` if it exists, or
`$(HOMEDRIVE)$(HOMEPATH)/.igrep/projects.lua` otherwise.

If the projects.lua file cannot be found, igrep will offer to create
the expected directory and config file. A large part of igrep's
functionality is written in Lua, and Lua is also used for project
configuration.  Every igrep command except **search** results in the
`projects.lua` file being executed.  The full Lua language is able to
be used from `projects.lua`, though it should be kept simple.  The
**search** command is intended to be as fast as possible (the whole
point of igrep!), and will run without interpreting the `projects.lua`
file. 

### Example projects.lua file

    Project{"linux", 
        track("~/source/linux-2.6", "\\.c$", "\\.h$"),
        ignore("f.o", "^bar\\.c$") 
    }

**Project** is a Lua function that accepts a table as its only
  argument, the table should be a comma seperated list within {}.
  The first element in the table *must* be a string and is used to
  name the project, "linux" in our example case.  There can be any
  number of **track** or **ignore** entries after the name of the
  project.

**track(path, patterns...)** tells igrep to add *path* to the project,
  and will recursively add all subdirectories.  *patterns* is a list
  of regular expression patterns, any files under path that match any
  pattern will be added.  In the example, we wish to add any file that
  ends in '.c' or '.h'.  You can have any number of patterns after the
  path entry. Having no patterns will result in all files being
  added.  Because the project file is just Lua code, backslashes must
  be escaped.  In the above example `\\.c` becomes the regex `\.c`, so
  that `.` will be matched as the period character rather than the
  usual regex meaning of ". means match any character"

**ignore(patterns...)** tells igrep to ignore files that match any
  pattern.  There can be multiple ignore entries.  In our example, any
  filename that contains the pattern "f.o" will be ignored.  Also any
  file that is exactly named "bar.c" will be ignored.  The exactness
  is because the pattern is surrouded by `^` and `$`.  The second
  ignore pattern will never be matched because filenames are tested
  with their full path, ie all files will start with
  `~/source/linux-2.6/` 

## Benchmarks 
All benchmarks were taken on my MacBook Pro, which has a 2.26 Core Duo
CPU and a 5400rpm HDD. I am searching for a text string that is not in
the target corpus, this gives a good picture of 'worst case'
performance and how quickly the entire corpus can be searched.

The first timing is using regular find & grep to search for the text
string "NotFound_", and takes 27 seconds:

       $ time find . -iname "*.c" -or -iname "*.h" -exec grep NotFound_ {} \; 
       real 0m27.173s 
       user 0m5.991s 
       sys 0m18.970s

In comparison, searching with igrep takes 2.2 seconds - more than 10
times faster

      $ time igrep search linux NotFound_
       real 0m2.211s 
       user 0m3.035s 
       sys 0m0.142s 
       
igrep does need to pay a cost to generate its internal database, but
that cost is roughly similar to doing a single search with find/grep

     $ time igrep build linux 
     Building linux, may take some time Added
     26425 files to archive Done building
     real 0m25.307s 
     user 0m21.153s 
     sys 0m1.257s
     
## Please Donate!
If you find igrep to be useful a donation would be excellent
motivation for me to make further improvements (such as much improved
multicore support!).  
If total donations exceed $1000 Canadian dollars, I will release
igrep under a permissive licence like BSD.  

LINK TO PAYPAL AREA


## Credits 
igrep uses the following libraries, many thanks to the authors

- [RE2](http://code.google.com/p/re2/) provides the regular expression
code used for matching
- [libarchive](http://code.google.com/p/libarchive/) provides the code
used for compressing and decompressing the search material
- [PThreads-w32](http://sourceware.org/pthreads-win32/) provides
threading on Windows
- [Lua](http://www.lua.org/); much of the functionality of igrep is
programmed in Lua.


# qgrep (Quick Grep)
Super fast full text grep for programmers

-----------------------

## Introduction 
Programmers commonly need to search code, faster searching makes for
more productivity. igrep is significantly faster than search tools
that don't build a database and is very easy to setup - simple
projects need only a couple of lines of configuration.  
igrep runs on OS X and Windows, with a Linux port coming soon.

## Installation 

### OSX
Currently igrep runs on OSX 10.5 or later on a 64bit Intel system.
[Download](nofile) and extract to a directory of your choice.  

### Linux 
Coming soon, please email <brad@igrep.com> to express your
interest.

### Windows 
[Download](setup) and run the Windows installer.

## Configuration and Usage 
igrep expects to find a configuration file in a specific place.  For
*nix systems the file is `~/.igrep/projects.lua`

For Windows the file is `$(HOME)/.igrep/projects.lua` if it exists, or
`$(HOMEDRIVE)$(HOMEPATH)/.igrep/projects.lua` otherwise.

If the projects.lua file cannot be found, igrep will offer to create
the expected directory and config file. A large part of igrep's
functionality is written in Lua, and Lua is also used for project
configuration.  Every igrep command except **search** results in the
`projects.lua` file being executed.  The full Lua language is able to
be used from `projects.lua`, though it should be kept simple.  The
**search** command is intended to be as fast as possible (the whole
point of igrep!), and will run without interpreting the `projects.lua`
file. 

### Example projects.lua file

    Project{"linux", 
        track("~/source/linux-2.6", "\\.c$", "\\.h$"),
        ignore("f.o", "^bar\\.c$") 
    }

**Project** is a Lua function that accepts a table as its only
  argument, the table should be a comma seperated list within {}.
  The first element in the table *must* be a string and is used to
  name the project, "linux" in our example case.  There can be any
  number of **track** or **ignore** entries after the name of the
  project.

**track(path, patterns...)** tells igrep to add *path* to the project,
  and will recursively add all subdirectories.  *patterns* is a list
  of regular expression patterns, any files under path that match any
  pattern will be added.  In the example, we wish to add any file that
  ends in '.c' or '.h'.  You can have any number of patterns after the
  path entry. Having no patterns will result in all files being
  added.  Because the project file is just Lua code, backslashes must
  be escaped.  In the above example `\\.c` becomes the regex `\.c`, so
  that `.` will be matched as the period character rather than the
  usual regex meaning of ". means match any character"

**ignore(patterns...)** tells igrep to ignore files that match any
  pattern.  There can be multiple ignore entries.  In our example, any
  filename that contains the pattern "f.o" will be ignored.  Also any
  file that is exactly named "bar.c" will be ignored.  The exactness
  is because the pattern is surrouded by `^` and `$`.  The second
  ignore pattern will never be matched because filenames are tested
  with their full path, ie all files will start with
  `~/source/linux-2.6/` 

## Benchmarks 
All benchmarks were taken on my MacBook Pro, which has a 2.26 Core Duo
CPU and a 5400rpm HDD. I am searching for a text string that is not in
the target corpus, this gives a good picture of 'worst case'
performance and how quickly the entire corpus can be searched.

The first timing is using regular find & grep to search for the text
string "NotFound_", and takes 27 seconds:

       $ time find . -iname "*.c" -or -iname "*.h" -exec grep NotFound_ {} \; 
       real 0m27.173s 
       user 0m5.991s 
       sys 0m18.970s

In comparison, searching with igrep takes 2.2 seconds - more than 10
times faster

      $ time igrep search linux NotFound_
       real 0m2.211s 
       user 0m3.035s 
       sys 0m0.142s 
       
igrep does need to pay a cost to generate its internal database, but
that cost is roughly similar to doing a single search with find/grep

     $ time igrep build linux 
     Building linux, may take some time Added
     26425 files to archive Done building
     real 0m25.307s 
     user 0m21.153s 
     sys 0m1.257s
     
## Please Donate!
If you find igrep to be useful a donation would be excellent
motivation for me to make further improvements (such as much improved
multicore support!).  
If total donations exceed $1000 Canadian dollars, I will release
igrep under a permissive licence like BSD.  

LINK TO PAYPAL AREA


## Credits 
igrep uses the following libraries, many thanks to the authors

- [RE2](http://code.google.com/p/re2/) provides the regular expression
code used for matching
- [libarchive](http://code.google.com/p/libarchive/) provides the code
used for compressing and decompressing the search material
- [PThreads-w32](http://sourceware.org/pthreads-win32/) provides
threading on Windows
- [Lua](http://www.lua.org/); much of the functionality of igrep is
programmed in Lua.


# qgrep (Quick Grep)
Super fast full text grep for programmers

-----------------------

## Introduction 
Programmers commonly need to search code, faster searching makes for
more productivity. qgrep is significantly faster than search tools
that don't build a database and is very easy to setup - simple
projects need only a couple of lines of configuration.  
qgrep runs on OS X and Windows, with a Linux port coming soon.

## Installation 

### OSX
Currently qgrep runs on OSX 10.5 or later on a 64bit Intel system.
[Download](nofile) and extract to a directory of your choice.  

### Linux 
Coming soon, please email <brad@qgrep.com> to express your
interest.

### Windows 
[Download](setup) and run the Windows installer.

## Configuration and Usage 
qgrep expects to find a configuration file in a specific place.  For
*nix systems the file is `~/.qgrep/projects.lua`

For Windows the file is `$(HOME)/.qgrep/projects.lua` if it exists, or
`$(HOMEDRIVE)$(HOMEPATH)/.qgrep/projects.lua` otherwise.

If the projects.lua file cannot be found, qgrep will offer to create
the expected directory and config file. A large part of qgrep's
functionality is written in Lua, and Lua is also used for project
configuration.  Every qgrep command except **search** results in the
`projects.lua` file being executed.  The full Lua language is able to
be used from `projects.lua`, though it should be kept simple.  The
**search** command is intended to be as fast as possible (the whole
point of qgrep!), and will run without interpreting the `projects.lua`
file. 

### Example projects.lua file

    Project{"linux", 
        track("~/source/linux-2.6", "\\.c$", "\\.h$"),
        ignore("f.o", "^bar\\.c$") 
    }

**Project** is a Lua function that accepts a table as its only
  argument, the table should be a comma seperated list within {}.
  The first element in the table *must* be a string and is used to
  name the project, "linux" in our example case.  There can be any
  number of **track** or **ignore** entries after the name of the
  project.

**track(path, patterns...)** tells qgrep to add *path* to the project,
  and will recursively add all subdirectories.  *patterns* is a list
  of regular expression patterns, any files under path that match any
  pattern will be added.  In the example, we wish to add any file that
  ends in '.c' or '.h'.  You can have any number of patterns after the
  path entry. Having no patterns will result in all files being
  added.  Because the project file is just Lua code, backslashes must
  be escaped.  In the above example `\\.c` becomes the regex `\.c`, so
  that `.` will be matched as the period character rather than the
  usual regex meaning of ". means match any character"

**ignore(patterns...)** tells qgrep to ignore files that match any
  pattern.  There can be multiple ignore entries.  In our example, any
  filename that contains the pattern "f.o" will be ignored.  Also any
  file that is exactly named "bar.c" will be ignored.  The exactness
  is because the pattern is surrouded by `^` and `$`.  The second
  ignore pattern will never be matched because filenames are tested
  with their full path, ie all files will start with
  `~/source/linux-2.6/` 

## Benchmarks 
All benchmarks were taken on my MacBook Pro, which has a 2.26 Core Duo
CPU and a 5400rpm HDD. I am searching for a text string that is not in
the target corpus, this gives a good picture of 'worst case'
performance and how quickly the entire corpus can be searched.

The first timing is using regular find & grep to search for the text
string "NotFound_", and takes 27 seconds:

       $ time find . -iname "*.c" -or -iname "*.h" -exec grep NotFound_ {} \; 
       real 0m27.173s 
       user 0m5.991s 
       sys 0m18.970s

In comparison, searching with qgrep takes 2.2 seconds - more than 10
times faster

      $ time qgrep search linux NotFound_
       real 0m2.211s 
       user 0m3.035s 
       sys 0m0.142s 
       
qgrep does need to pay a cost to generate its internal database, but
that cost is roughly similar to doing a single search with find/grep

     $ time qgrep build linux 
     Building linux, may take some time Added
     26425 files to archive Done building
     real 0m25.307s 
     user 0m21.153s 
     sys 0m1.257s
     
## Etc
qgrep was written by Brad Beveridge, if you find qgrep to be useful a
donation would be excellent motivation for me to make further
improvements (such as much improved multicore support!). If total
donations exceed $1000 Canadian dollars, I will release qgrep under a
permissive licence like BSD.   

This readme and a Paypal donate button can be found at www.qgrep.com


## Credits 
qgrep uses the following libraries, many thanks to their authors

- [RE2](http://code.google.com/p/re2/) provides the regular expression
code used for matching
- [libarchive](http://code.google.com/p/libarchive/) provides the code
used for compressing and decompressing the search material
- [PThreads-w32](http://sourceware.org/pthreads-win32/) provides
threading on Windows
- [Pigz](http://www.zlib.net/pigz/) provided pthread interfacing
- [Lua](http://www.lua.org/) is the programming language for much of
qgrep

# qgrep (Quick Grep)
Super fast full text grep for programmers

-----------------------

## Introduction 
Programmers commonly need to search code, faster searching makes for
increased productivity. qgrep is significantly faster than search tools
that don't build a database and is very easy to setup - simple
projects need only a couple of lines of configuration.  
qgrep runs on OS X and Windows, with a Linux port coming soon.

## Installation 

### OSX
Currently qgrep runs on OSX 10.5 or later on a 64bit Intel system.
[Download](nofile) and extract to a directory of your choice.  

### Linux 
Coming soon, please email <brad@qgrep.com> to express your
interest.

### Windows 
[Download](setup) and run the Windows installer.

## Configuration and Usage 
qgrep expects to find a configuration file in a specific place.  For
*nix systems the file is `~/.qgrep/projects.lua`

For Windows the file is `$(HOME)/.qgrep/projects.lua` if it exists, or
`$(HOMEDRIVE)$(HOMEPATH)/.qgrep/projects.lua` otherwise.

If the projects.lua file cannot be found, qgrep will offer to create
the expected directory and config file. A large part of qgrep's
functionality is written in Lua, and Lua is also used for project
configuration.  Every qgrep command except **search** results in the
`projects.lua` file being executed.  The full Lua language is able to
be used from `projects.lua`, though it should be kept simple.  The
**search** command is intended to be as fast as possible (the whole
point of qgrep!), and will run without interpreting the `projects.lua`
file. 

### Example projects.lua file

    Project{"linux", 
        track("~/source/linux-2.6", "\\.c$", "\\.h$"),
        ignore("f.o", "^bar\\.c$") 
    }

**Project** is a Lua function that accepts a table as its only
  argument, the table should be a comma seperated list within {}.
  The first element in the table *must* be a string and is used to
  name the project, "linux" in our example case.  There can be any
  number of **track** or **ignore** entries after the name of the
  project.

**track(path, patterns...)** tells qgrep to add *path* to the project,
  and will recursively add all subdirectories.  *patterns* is a list
  of regular expression patterns, any files under path that match any
  pattern will be added.  In the example, we wish to add any file that
  ends in '.c' or '.h'.  You can have any number of patterns after the
  path entry. Having no patterns will result in all files being
  added.  Because the project file is just Lua code, backslashes must
  be escaped.  In the above example `\\.c` becomes the regex `\.c`, so
  that `.` will be matched as the period character rather than the
  usual regex meaning of ". means match any character"

**ignore(patterns...)** tells qgrep to ignore files that match any
  pattern.  There can be multiple ignore entries.  In our example, any
  filename that contains the pattern "f.o" will be ignored.  Also any
  file that is exactly named "bar.c" will be ignored.  The exactness
  is because the pattern is surrouded by `^` and `$`.  The second
  ignore pattern will never be matched because filenames are tested
  with their full path, ie all files will start with
  `~/source/linux-2.6/` 

## Benchmarks 
All benchmarks were taken on my MacBook Pro, which has a 2.26 Core Duo
CPU and a 5400rpm HDD. I am searching for a text string that is not in
the target corpus, this gives a good picture of 'worst case'
performance and how quickly the entire corpus can be searched.

The first timing is using regular find & grep to search for the text
string "NotFound_", and takes 27 seconds:

       $ time find . -iname "*.c" -or -iname "*.h" -exec grep NotFound_ {} \; 
       real 0m27.173s 
       user 0m5.991s 
       sys 0m18.970s

In comparison, searching with qgrep takes 2.2 seconds - more than 10
times faster

      $ time qgrep search linux NotFound_
       real 0m2.211s 
       user 0m3.035s 
       sys 0m0.142s 
       
qgrep does need to pay a cost to generate its internal database, but
that cost is roughly similar to doing a single search with find/grep

     $ time qgrep build linux 
     Building linux, may take some time Added
     26425 files to archive Done building
     real 0m25.307s 
     user 0m21.153s 
     sys 0m1.257s
     
## Etc
qgrep was written by Brad Beveridge, if you find qgrep to be useful a
donation would be excellent motivation for me to make further
improvements (such as much improved multicore support!). If total
donations exceed $1000 Canadian dollars, I will release qgrep under a
permissive licence like BSD.   

This readme and a Paypal donate button can be found at www.qgrep.com


## Credits 
qgrep uses the following libraries, many thanks to their authors

- [RE2](http://code.google.com/p/re2/) provides the regular expression
code used for matching
- [libarchive](http://code.google.com/p/libarchive/) provides the code
used for compressing and decompressing the search material
- [PThreads-w32](http://sourceware.org/pthreads-win32/) provides
threading on Windows
- [Pigz](http://www.zlib.net/pigz/) provided pthread interfacing
- [Lua](http://www.lua.org/) is the programming language for much of
qgrep

# qgrep (Quick Grep)
Super fast full text grep for programmers

-----------------------

## Introduction 
Programmers commonly need to search code, faster searching makes for
increased productivity. qgrep is significantly faster than search tools
that don't build a database and is very easy to setup - simple
projects need only a couple of lines of configuration.  
qgrep runs on OS X and Windows, with a Linux port coming soon.

## Installation 

### OSX
Currently qgrep runs on OSX 10.5 or later on a 64bit Intel system.
[Download](qgrep-osx.tgz) and extract to a directory of your choice.  

### Linux 
Coming soon, please email <brad@qgrep.com> to express your
interest.

### Windows 
[Download](setup) and run the Windows installer.

## Configuration and Usage 
qgrep expects to find a configuration file in a specific place.  For
*nix systems the file is `~/.qgrep/projects.lua`

For Windows the file is `$(HOME)/.qgrep/projects.lua` if it exists, or
`$(HOMEDRIVE)$(HOMEPATH)/.qgrep/projects.lua` otherwise.

If the projects.lua file cannot be found, qgrep will offer to create
the expected directory and config file. A large part of qgrep's
functionality is written in Lua, and Lua is also used for project
configuration.  Every qgrep command except **search** results in the
`projects.lua` file being executed.  The full Lua language is able to
be used from `projects.lua`, though it should be kept simple.  The
**search** command is intended to be as fast as possible (the whole
point of qgrep!), and will run without interpreting the `projects.lua`
file. 

### Example projects.lua file

    Project{"linux", 
        track("~/source/linux-2.6", "\\.c$", "\\.h$"),
        ignore("f.o", "^bar\\.c$") 
    }

**Project** is a Lua function that accepts a table as its only
  argument, the table should be a comma seperated list within {}.
  The first element in the table *must* be a string and is used to
  name the project, "linux" in our example case.  There can be any
  number of **track** or **ignore** entries after the name of the
  project.

**track(path, patterns...)** tells qgrep to add *path* to the project,
  and will recursively add all subdirectories.  *patterns* is a list
  of regular expression patterns, any files under path that match any
  pattern will be added.  In the example, we wish to add any file that
  ends in '.c' or '.h'.  You can have any number of patterns after the
  path entry. Having no patterns will result in all files being
  added.  Because the project file is just Lua code, backslashes must
  be escaped.  In the above example `\\.c` becomes the regex `\.c`, so
  that `.` will be matched as the period character rather than the
  usual regex meaning of ". means match any character"

**ignore(patterns...)** tells qgrep to ignore files that match any
  pattern.  There can be multiple ignore entries.  In our example, any
  filename that contains the pattern "f.o" will be ignored.  Also any
  file that is exactly named "bar.c" will be ignored.  The exactness
  is because the pattern is surrouded by `^` and `$`.  The second
  ignore pattern will never be matched because filenames are tested
  with their full path, ie all files will start with
  `~/source/linux-2.6/` 

## Benchmarks 
All benchmarks were taken on my MacBook Pro, which has a 2.26 Core Duo
CPU and a 5400rpm HDD. I am searching for a text string that is not in
the target corpus, this gives a good picture of 'worst case'
performance and how quickly the entire corpus can be searched.

The first timing is using regular find & grep to search for the text
string "NotFound_", and takes 27 seconds:

       $ time find . -iname "*.c" -or -iname "*.h" -exec grep NotFound_ {} \; 
       real 0m27.173s 
       user 0m5.991s 
       sys 0m18.970s

In comparison, searching with qgrep takes 2.2 seconds - more than 10
times faster

      $ time qgrep search linux NotFound_
       real 0m2.211s 
       user 0m3.035s 
       sys 0m0.142s 
       
qgrep does need to pay a cost to generate its internal database, but
that cost is roughly similar to doing a single search with find/grep

     $ time qgrep build linux 
     Building linux, may take some time Added
     26425 files to archive Done building
     real 0m25.307s 
     user 0m21.153s 
     sys 0m1.257s
     
## Etc
qgrep was written by Brad Beveridge, if you find qgrep to be useful a
donation would be excellent motivation for me to make further
improvements (such as much improved multicore support!). If total
donations exceed $1000 Canadian dollars, I will release qgrep under a
permissive licence like BSD.   

This readme and a Paypal donate button can be found at www.qgrep.com


## Credits 
qgrep uses the following libraries, many thanks to their authors

- [RE2](http://code.google.com/p/re2/) provides the regular expression
code used for matching
- [libarchive](http://code.google.com/p/libarchive/) provides the code
used for compressing and decompressing the search material
- [PThreads-w32](http://sourceware.org/pthreads-win32/) provides
threading on Windows
- [Pigz](http://www.zlib.net/pigz/) provided pthread interfacing
- [Lua](http://www.lua.org/) is the programming language for much of
qgrep


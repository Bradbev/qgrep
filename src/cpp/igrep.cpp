#include <assert.h>
#include "archive.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static void grepFormatHit(void* context, const char* filename, unsigned int lineNumber, const char* lineStart, const char* lineEnd)
{
    printf("%s:%d:", filename, lineNumber);
    while (lineStart < lineEnd)
    {
	putchar(*lineStart);
	lineStart++;
    }
    putchar('\n');
}

void usage()
{
    printf("Usage is:\n"
	   "\tigrep [options] <project-name> <search-pattern>\n"
	   "\tOptions: \n"
	   "\t-i : Case insensitive \n");
    exit(1);
};

int main(int argc, char** argv)
{
    bool caseInsensitive = false;
    int c;
    
    if (argc <= 2)
	usage();
    
    opterr = 0;
     
    while ((c = getopt (argc, argv, "i")) != -1)
    {
	switch (c)
	{
	case 'i':
	    caseInsensitive = true;
	    break;
	default:
	    usage();
	}
    }
     
    const char* archiveName = argv[optind];
    const char* pattern = argv[optind+1];
    
    GrepParams params;
    params.sourceArchiveName = archiveName;
    params.callbackFunction = grepFormatHit;
    params.searchPattern = pattern;
    params.streamBlockSize = 1 * 1024 * 1024;
    params.streamBlockCount = 10;
    params.caseInsensitive = caseInsensitive;
    
    ExecuteSearch(&params);

    return 0;
}

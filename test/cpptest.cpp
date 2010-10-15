#include "../src/cpp/archive.h"
#include <stdio.h>

void test_ReadFileNames(const char* archiveName);
bool MatchPatternInLine(char* line, char* pattern);
void stream_test();
void namedDataBlockTest();

void test_CreateArchive()
{
    const char *files[] = {
	"/Users/bradbeveridge/development/c/igrep/data/dirtest/dirA/aa.c",
	"/Users/bradbeveridge/development/c/igrep/data/dirtest/dirA/aa.cpp",
	"/Users/bradbeveridge/development/c/igrep/data/dirtest/dirA/sound_core.c",
	"/Users/bradbeveridge/development/c/igrep/data/dirtest/dirb/bb.c",
	"/Users/bradbeveridge/development/c/igrep/data/dirtest/dirb/bb.h",
	"/Users/bradbeveridge/development/c/igrep/data/dirtest/dirb/sound_firmware.c",
	NULL
    };
    struct archive* a = CreateArchive("test.tgz");
    for (char **f = (char**)files; *f; f++)
    {
	AddFileToArchive(a, *f);
    }
    CloseArchive(a);
}

int main(int argc, char** argv)
{
    //stream_test();
    //namedDataBlockTest();
    //test_ReadFileNames("../data/raw-data.tar.gz");
    test_CreateArchive();
//    printf("Match? %d\n", MatchPatternInLine("hello", "ell"));
    return 0;
}

#include "trigram.h"
#include <set>
#include <map>
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include "assert.h"
#include <sys/stat.h>
#ifndef WIN32
#include <sys/mman.h>
#else
#include <windows.h>
#endif
#include <fcntl.h>

/*
 * There is some fairly odd code in here (see especially
 * trigram_save_to_file), but it is all to make trigram searching
 * fast.
 * There are two phases to using trigrams and they use different data
 * structures.
 * Building:
 * The algorithm here is to break text streams into trigrams and
 * associate each trigram with the incoming filename.  Trigrams are
 * stored in a map, providing O(logn) lookup.  Lookup speed is
 * critical because there will be n-2 lookups for a project with n
 * chars (ie, 400,000,000 for linux).  I tried a hashmap (O(1)), but
 * didn't get faster results.
 * Each trigram in the map is associated with an int vector.  This
 * vector contains the indexes of every file this trigram appears in.
 * Because the index is only going to be increasing the primary
 * optimization here is to check the last element in the vector & only
 * insert if the current file doesn't exist.
 *
 * Saving/lookup
 * Saving and lookup are intimately tied, because when doing lookup we
 * simply mmap the saved file and work with the data structures in the
 * file.  
 * The first level of lookup is finding the trigrams in an input
 * string, for 'foot' the trigrams 'foo' and 'oot' are found by using
 * a binary search algorithm.  The found structure points into a
 * vector containing indexes for which files contain that trigram.  We
 * then run a custom set union by:
 *  - finding the smallest set & iterating it
 *  - for each element, try & find that element in all sets.  Only if
 * that index is in all sets can that file be considered a match.  
 *  - as each set is searched, we shrink it by moving the lower bound
 * up 
 * 
 */

typedef unsigned int uint;

static bool validchar(char c)
{
    return '0' <= c && c <= 'z';
}

struct Trigram
{
    union {
	char gram[3];
	uint gram_uint;
    };
    Trigram()  { Reset(); }
    Trigram(const char* g)  { memcpy(gram, g, 3); };
    // Not safe, debug prints only!
    const char* AsStr()
    {
	static char ret[4];
	memcpy(ret, gram, 3);
	ret[3] = 0;
	return ret;
    }
    void Reset(){ gram_uint = 0; }
    bool PushChar(char c)
    {
	gram[0] = gram[1];
	gram[1] = gram[2];
	gram[2] = c;
	return IsValid();
    }
    bool IsValid()
    {
	return validchar(gram[0]) &&
	       validchar(gram[1]) &&
	       validchar(gram[2]);
    }
    bool operator<(const Trigram& rhs) const
    {
	return gram_uint < rhs.gram_uint;
    }
};

struct TrigramAndOffset : public Trigram
{
    uint setOffset;
    uint setSize;
};

typedef std::vector<std::string> StringVector;
typedef std::set<Trigram> TrigramSet;
typedef std::vector<uint> IndexVector;
typedef std::map<Trigram, IndexVector> TrigramMap;

struct TrigramSplitter
{
    /*
     * The build struct is used when building up the trigram data
     * structures.
     */
    struct
    {
	// members used when creating 
	StringVector filenames;
	TrigramMap trigrams;
	uint current_index;
	Trigram current_trigram;
    } build;
    
    /*
     * lookup_t is written directly to file as is when creating the data
     * set, and is mmapped when reading back.  See trigram_save_to_file
     */
    struct lookup_t
    {
	uint number_of_files;
	uint number_of_tris;
	uint file_index_offset;
	uint trigram_offset;
	uint trigram_set_offset;
	uint filename_offset;
	char data[0];
    };
    lookup_t* lookup;
};

TrigramSplitter* trigram_new()
{
    TrigramSplitter* t = new TrigramSplitter;
    t->build.current_index = 0;
    return t;
}

void trigram_delete(TrigramSplitter* t)
{
    delete t;
}

void trigram_start_file(TrigramSplitter* t, const char* filename)
{
    t->build.filenames.push_back(filename);
}

void trigram_add_data(TrigramSplitter* t, const char* data, uint data_length)
{
    Trigram tri = t->build.current_trigram;
    uint fileIndex = t->build.current_index;
    for (uint i = 0; i < data_length; i++)
    {
	if (tri.PushChar(data[i]))
	{
	    IndexVector& iv = t->build.trigrams[tri];
	    if (iv.empty() || iv[iv.size()-1] != fileIndex)
	    {
		iv.push_back(fileIndex);
	    }
	}
    }
    t->build.current_trigram = tri;
}

void trigram_stop_file(TrigramSplitter* t)
{
    t->build.current_index++;
    t->build.current_trigram.Reset();
}

uint* get_trigram_set(TrigramSplitter* t, Trigram* tri, uint* set_count_out)
{
    TrigramAndOffset* trigrams = (TrigramAndOffset*)&t->lookup->data[t->lookup->trigram_offset];
    TrigramAndOffset* lower = std::lower_bound(trigrams, &trigrams[t->lookup->number_of_tris], *((TrigramAndOffset*)tri));
    //printf("%s\n", tri->AsStr());
    //if (lower) printf("lower %s, tri %s\n", lower->AsStr(), tri->AsStr());
    if (lower && memcmp(lower->gram, tri->gram, 3) == 0)
    {
	uint* tri_set = (uint*)&t->lookup->data[lower->setOffset];
	*set_count_out = lower->setSize;
	return tri_set;
    }
    set_count_out = 0;
    return NULL;
}

const char* get_filename_from_index(TrigramSplitter* t, uint index)
{
    if (index >= t->lookup->number_of_files) 
    {
	printf("index is %d >= %d - too big\n", index, t->lookup->number_of_files);
	return NULL;
    }
    int*  file_index = (int*)&t->lookup->data[t->lookup->file_index_offset];
    const char* filenames = (const char*)&t->lookup->data[t->lookup->filename_offset];
    return filenames + file_index[index];
}

struct TrigramIntSet
{
    uint count;
    uint* current_lower_bound;
    uint* upper_bound;
};

/*
 * Binary search all sets for this index.  We will always test indexes
 * in increasing order, so any set that has a lower bound below the
 * testing index will have its lowerbound narrowed
 */
bool is_in_all_sets(std::map<Trigram, TrigramIntSet>& triToSetMap, uint test_index)
{
    std::map<Trigram, TrigramIntSet>::iterator end = triToSetMap.end();
    for (std::map<Trigram, TrigramIntSet>::iterator i = triToSetMap.begin();
	 i != end; ++i)
    {
	TrigramIntSet &set = i->second;
	uint* l = std::lower_bound(set.current_lower_bound, set.upper_bound, test_index);
	if (*l <= test_index)
	{
	    // move up this sets lower bound
	    set.current_lower_bound = l;
	}
	if (*l != test_index)
	{
	    return false;
	}
    }
    return true;
}

bool trigram_string_is_searchable(const char* string)
{
    int len = 0;
    if (!string) return false;
    while (*string)
    {
	if (!validchar(*string)) return false;
	len++;
	string++;
    }
    return len > 2;
}

bool trigram_iterate_matching_files(TrigramSplitter* t, const char* string_to_find, void* context, callback_fn callback)
{
#if 0
    TrigramAndOffset* trigrams = (TrigramAndOffset*)&t->lookup->data[t->lookup->trigram_offset];
    for (uint i = 0; i < t->lookup->number_of_tris; i++)
    {
	printf("%s\n", trigrams[i].AsStr());
    }
#endif
    
    std::map<Trigram, TrigramIntSet> triToSetMap;
    Trigram tri;
    uint min_set_size = (uint)-1;
    Trigram min_tri;
    while (*string_to_find)
    {
	if (tri.PushChar(*string_to_find))
	{
	    if (triToSetMap.count(tri) == 0)
	    {
		TrigramIntSet set;
		uint* set_data = get_trigram_set(t, &tri, &set.count);
		set.current_lower_bound = set_data;
		if (!set_data)
		{
		    return false;
		}
		else
		{
		    set.upper_bound = &set_data[set.count];
		    if (min_set_size > set.count)
		    {
			min_set_size = set.count;
			min_tri = tri;
		    }
		    triToSetMap[tri] = set;
// 		    printf("Adding trigram %s\n", tri.AsStr());
// 		    for (uint i = 0; i < set.count; i++)
// 		    {
// 			printf("\t%s\n", get_filename_from_index(t, set.current_lower_bound[i]));
// 		    }
		}
	    }
	}
	string_to_find++;
    }
//    printf("Intersecting file lists, smallest tri is %s\n", min_tri.AsStr());
    assert(min_tri.IsValid());
    // only one trigram to look for?
    TrigramIntSet smallest_set = triToSetMap[min_tri];
    if (triToSetMap.size() == 1)
    {
	uint* set = smallest_set.current_lower_bound;
	for (uint i = 0; i < smallest_set.count; i++)
	{
	    callback(context, get_filename_from_index(t, set[i]));
	}
	return true;
    }
    // erase the smallest set so we don't need to check it
    triToSetMap.erase(min_tri);
    std::vector<int> resultSet;
    resultSet.reserve(smallest_set.count);
    for (uint i = 0; i < smallest_set.count; i++)
    {
	uint test_index = smallest_set.current_lower_bound[i];
	if (is_in_all_sets(triToSetMap, test_index))
	{
	    resultSet.push_back(test_index);
	}
    }
    uint resultSize = resultSet.size();
    for (uint i = 0; i < resultSize; i++)
    {
	//printf("\t%s\n", get_filename_from_index(t, resultSet[i]));
	callback(context, get_filename_from_index(t, resultSet[i]));
    }
    return true;
}

void trigram_save_to_file(TrigramSplitter* t, const char* filename)
{
    /*
     * This is a bit complex, but basically we setup the lookup_t data
     * in memory in one big block so we can mmap a file & directly use
     * that memory.
     * The format for the data[] section in the lookup_t struct is
     * - an array of uint32s * number_of_files.  This lets us map from
     * a file index to an offset in the data block that contains the
     * filename.
     * - TrigramAndOffset array * number_of_tris.  Sorted array that
     * allows fast lookup of a trigram & trigram set offset
     * - trigram_set * number_of_tris.  Each trigram set is just a
     * sorted array of file indexes.
     * - filenames array.  All the filenames as C strings.
     */
    
    int data_size = 0;
    // uint32<number_of_files> - file index to name offset mapping
    int file_index_offset = 0;
    int file_count = t->build.filenames.size();
    data_size += file_count * sizeof(uint);
    
    // trigramAndOffset<number_of_trigrams> - trigram vector
    int trigram_offset = data_size;
    int tri_count = t->build.trigrams.size();
    data_size += tri_count * sizeof(TrigramAndOffset);
    
    // trigram set data
    int trigram_set_offset = data_size;
    TrigramMap::iterator tend = t->build.trigrams.end();
    for (TrigramMap::iterator i = t->build.trigrams.begin();
	 i != tend; ++i)
    {
	data_size += (i->second.size()) * sizeof(uint);
    }
    
    // filenames
    int filename_offset = data_size;
    StringVector::iterator s_end = t->build.filenames.end();
    for (StringVector::iterator i = t->build.filenames.begin();
	 i != s_end; ++i)
    {
	data_size += i->length() + 1;
    }
    //printf("Filesize is %d\n", data_size);
    t->lookup = (TrigramSplitter::lookup_t*)malloc(data_size+10000);
    t->lookup->number_of_files = file_count;
    t->lookup->number_of_tris = tri_count;
    t->lookup->file_index_offset = file_index_offset;
    t->lookup->trigram_offset = trigram_offset;
    t->lookup->trigram_set_offset = trigram_set_offset;
    t->lookup->filename_offset = filename_offset;
    
    int*  file_index = (int*)&t->lookup->data[file_index_offset];
    char* filenames = (char*)&t->lookup->data[filename_offset];
    int offset = 0;
    int count = 0;
    for (StringVector::iterator i = t->build.filenames.begin();
	 i != s_end; ++i, ++count)
    {
	file_index[count] = offset;
	strcpy(&filenames[offset], i->c_str());
	offset += i->length() + 1;
    }
    
    TrigramAndOffset* trigrams = (TrigramAndOffset*)&t->lookup->data[trigram_offset];
    count = 0;
    for (TrigramMap::iterator i = t->build.trigrams.begin();
	 i != tend; ++i, ++count)
    {
//	const Trigram* tri = &i->first;
//	printf("%c%c%c\n", tri->gram[0], tri->gram[1], tri->gram[2]);
	
	memcpy(&trigrams[count], &i->first, sizeof(Trigram));
	trigrams[count].setOffset = trigram_set_offset;
	int* tri_sets = (int*)&t->lookup->data[trigram_set_offset];
	trigrams[count].setSize = i->second.size();
	for (IndexVector::iterator it = i->second.begin();
	     it != i->second.end(); ++it)
	{
	    *tri_sets++ = *it;
	    trigram_set_offset += sizeof(int);
	}
    }
//     printf("-----------\n");
//     for (int tr = 0; tr < tri_count; tr++)
//     {
// 	Trigram* tri = &trigrams[tr];
// 	printf("%c%c%c\n", tri->gram[0], tri->gram[1], tri->gram[2]);
//     }
    
    /// test code
    //trigram_iterate_matching_files(t, "footbridge");
    
    ///////////////////////////
//     printf("number_of_files; %d\n", (int)t->lookup->number_of_files);
//     printf("number_of_tris; %d\n", (int)t->lookup->number_of_tris);
//     printf("file_index_offset; %d\n", (int)t->lookup->file_index_offset);
//     printf("trigram_offset; %d\n", (int)t->lookup->trigram_offset);
//     printf("trigram_set_offset; %d\n", (int)t->lookup->trigram_set_offset);
//     printf("filename_offset; %d\n", (int)t->lookup->filename_offset);
    FILE* f = fopen(filename, "wb");
    if (f)
    {
	fwrite(t->lookup, sizeof(TrigramSplitter::lookup_t) + data_size, 1, f);
    }
    fclose(f);
}

TrigramSplitter* trigram_load_from_file(const char* filename)
{
    TrigramSplitter* t = trigram_new();
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
	printf("Unable to open %s\n", filename);
	return 0;
    }
    struct stat st;
    fstat(fd, &st);
#ifdef WIN32
	// it is FASTER on win32 to NOT use mmap! (wtf)
    void* mem = malloc(st.st_size);
    read(fd, mem, st.st_size);
#else
    void* mem = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (!mem)
    {
		printf("mmap failed for %s!\n", filename);
    }
#endif
    t->lookup = (TrigramSplitter::lookup_t*)mem;
#if 0
    printf("number_of_files; %d\n", (int)t->lookup->number_of_files);
    printf("number_of_tris; %d\n", (int)t->lookup->number_of_tris);
    printf("file_index_offset; %d\n", (int)t->lookup->file_index_offset);
    printf("trigram_offset; %d\n", (int)t->lookup->trigram_offset);
    printf("trigram_set_offset; %d\n", (int)t->lookup->trigram_set_offset);
    printf("filename_offset; %d\n", (int)t->lookup->filename_offset);
#endif
    return t;
}

#include "trigram.h"
#include <set>
#include <map>
#include <vector>
#include <string>
#include <stdio.h>
#include <algorithm>
#include "assert.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

typedef unsigned int uint;

static bool validchar(char c)
{
    return '0' <= c && c <= 'z';
}

struct Trigram
{
    char gram[3];
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
    void Reset(){ gram[0] = gram[1] = gram[2] = 0; };
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
	for (int i = 0; i < 3; i++)
	{
	    if (gram[i] != rhs.gram[i]) return gram[i] < rhs.gram[i];
	}
	return false;
    }
};

struct TrigramAndOffset : public Trigram
{
    uint setOffset;
    uint setSize;
};

typedef std::vector<std::string> StringVector;
typedef std::set<uint> IndexSet;
typedef std::set<Trigram> TrigramSet;
typedef std::map<Trigram, IndexSet> TrigramMap;

struct TrigramSplitter
{
    struct
    {
	// members used when creating 
	StringVector filenames;
	TrigramMap trigrams;
	uint current_index;
	Trigram current_trigram;
    } write;
    
    /*
     * read_t is written directly to file as is when creating the data
     * set, and is mmapped when reading back.  See trigram_save_to_file
     */
    struct read_t
    {
	uint number_of_files;
	uint number_of_tris;
	uint file_index_offset;
	uint trigram_offset;
	uint trigram_set_offset;
	uint filename_offset;
	char data[0];
    };
    read_t* read;
};

TrigramSplitter* trigram_new()
{
    TrigramSplitter* t = new TrigramSplitter;
    t->write.current_index = 0;
    return t;
}

void trigram_delete(TrigramSplitter* t)
{
    delete t;
}

void trigram_start_file(TrigramSplitter* t, const char* filename)
{
    t->write.filenames.push_back(filename);
}

void trigram_add_data(TrigramSplitter* t, const char* data, uint data_length)
{
    Trigram tri = t->write.current_trigram;
    int fileIndex = t->write.current_index;
    TrigramSet seenSet;
    for (uint i = 0; i < data_length; i++)
    {
	if (tri.PushChar(data[i]))
	{
	    seenSet.insert(tri);
	}
    }
    TrigramSet::iterator end = seenSet.end();
    for (TrigramSet::iterator i = seenSet.begin();
	 i != end; ++i)
    {
	t->write.trigrams[*i].insert(fileIndex);
    }
    t->write.current_trigram = tri;
}

void trigram_stop_file(TrigramSplitter* t)
{
    t->write.current_index++;
    t->write.current_trigram.Reset();
}

uint* get_trigram_set(TrigramSplitter* t, Trigram* tri, uint* set_count_out)
{
    TrigramAndOffset* trigrams = (TrigramAndOffset*)&t->read->data[t->read->trigram_offset];
    TrigramAndOffset* lower = std::lower_bound(trigrams, &trigrams[t->read->number_of_tris], *((TrigramAndOffset*)tri));
//    printf("%c%c%c\n", tri->gram[0], tri->gram[1], tri->gram[2]);
 //   if (lower) printf("lower %c%c%c\n", lower->gram[0], lower->gram[1], lower->gram[2]);
    if (lower && memcmp(lower->gram, tri->gram, 3) == 0)
    {
	uint* tri_set = (uint*)&t->read->data[lower->setOffset];
	*set_count_out = lower->setSize;
	return tri_set;
    }
    set_count_out = 0;
    return NULL;
}

const char* get_filename_from_index(TrigramSplitter* t, uint index)
{
    if (index >= t->read->number_of_files) 
    {
	printf("index is %d >= %d - too big\n", index, t->read->number_of_files);
	return NULL;
    }
    int*  file_index = (int*)&t->read->data[t->read->file_index_offset];
    const char* filenames = (const char*)&t->read->data[t->read->filename_offset];
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
    if (!string) return false;
    while (*string)
    {
	if (!validchar(*string)) return false;
	string++;
    }
    return true;
}

bool trigram_iterate_matching_files(TrigramSplitter* t, const char* string_to_find, void* context, callback_fn callback)
{
#if 0
    TrigramAndOffset* trigrams = (TrigramAndOffset*)&t->read->data[t->read->trigram_offset];
    for (uint i = 0; i < t->read->number_of_tris; i++)
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
     * This is a bit complex, but basically we setup the read_t data
     * in memory in one big block so we can mmap a file & directly use
     * that memory.
     * The format for the data[] section in the read_t struct is
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
    int file_count = t->write.filenames.size();
    data_size += file_count * sizeof(uint);
    
    // trigramAndOffset<number_of_trigrams> - trigram vector
    int trigram_offset = data_size;
    int tri_count = t->write.trigrams.size();
    data_size += tri_count * sizeof(TrigramAndOffset);
    
    // trigram set data
    int trigram_set_offset = data_size;
    TrigramMap::iterator tend = t->write.trigrams.end();
    for (TrigramMap::iterator i = t->write.trigrams.begin();
	 i != tend; ++i)
    {
	data_size += (i->second.size()) * sizeof(uint);
    }
    
    // filenames
    int filename_offset = data_size;
    StringVector::iterator s_end = t->write.filenames.end();
    for (StringVector::iterator i = t->write.filenames.begin();
	 i != s_end; ++i)
    {
	data_size += i->length() + 1;
    }
    //printf("Filesize is %d\n", data_size);
    t->read = (TrigramSplitter::read_t*)malloc(data_size);
    t->read->number_of_files = file_count;
    t->read->number_of_tris = tri_count;
    t->read->file_index_offset = file_index_offset;
    t->read->trigram_offset = trigram_offset;
    t->read->trigram_set_offset = trigram_set_offset;
    t->read->filename_offset = filename_offset;
    
    int*  file_index = (int*)&t->read->data[file_index_offset];
    char* filenames = (char*)&t->read->data[filename_offset];
    int offset = 0;
    int count = 0;
    for (StringVector::iterator i = t->write.filenames.begin();
	 i != s_end; ++i, ++count)
    {
	file_index[count] = offset;
	strcpy(&filenames[offset], i->c_str());
	offset += i->length() + 1;
    }
    
    TrigramAndOffset* trigrams = (TrigramAndOffset*)&t->read->data[trigram_offset];
    count = 0;
    for (TrigramMap::iterator i = t->write.trigrams.begin();
	 i != tend; ++i, ++count)
    {
//	const Trigram* tri = &i->first;
//	printf("%c%c%c\n", tri->gram[0], tri->gram[1], tri->gram[2]);
	
	memcpy(&trigrams[count], &i->first, sizeof(Trigram));
	trigrams[count].setOffset = trigram_set_offset;
	trigrams[count].setSize = i->second.size();
	int* tri_sets = (int*)&t->read->data[trigram_set_offset];
	for (IndexSet::iterator it = i->second.begin();
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
//     printf("number_of_files; %d\n", (int)t->read->number_of_files);
//     printf("number_of_tris; %d\n", (int)t->read->number_of_tris);
//     printf("file_index_offset; %d\n", (int)t->read->file_index_offset);
//     printf("trigram_offset; %d\n", (int)t->read->trigram_offset);
//     printf("trigram_set_offset; %d\n", (int)t->read->trigram_set_offset);
//     printf("filename_offset; %d\n", (int)t->read->filename_offset);
    FILE* f = fopen(filename, "w");
    if (f)
    {
	fwrite(t->read, sizeof(t->read) + data_size, 1, f);
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
#if 1
    void* mem = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (!mem)
    {
	printf("mmap failed for %s!\n", filename);
    }
#else
    printf("Mallocing %d\n", (int)st.st_size);
    void* mem = malloc(st.st_size);
    read(fd, mem, st.st_size);
#endif
    t->read = (TrigramSplitter::read_t*)mem;
#if 0
    printf("number_of_files; %d\n", (int)t->read->number_of_files);
    printf("number_of_tris; %d\n", (int)t->read->number_of_tris);
    printf("file_index_offset; %d\n", (int)t->read->file_index_offset);
    printf("trigram_offset; %d\n", (int)t->read->trigram_offset);
    printf("trigram_set_offset; %d\n", (int)t->read->trigram_set_offset);
    printf("filename_offset; %d\n", (int)t->read->filename_offset);
#endif
    return t;
}

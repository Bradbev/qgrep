#ifndef TRIGRAM_H
#define TRIGRAM_H

struct TrigramSplitter;

TrigramSplitter* trigram_new();
void trigram_delete(TrigramSplitter* t);

void trigram_start_file(TrigramSplitter* t, const char* filename);
void trigram_add_data(TrigramSplitter* t, const char* data, unsigned int data_length);
void trigram_stop_file(TrigramSplitter* t);

void trigram_save_to_file(TrigramSplitter* t, const char* filename);
TrigramSplitter* trigram_load_from_file(const char* filename);

typedef void (*callback_fn)(void* context, const char* filename);
// returns false if there were no matching files
bool trigram_iterate_matching_files(TrigramSplitter* t, const char* string_to_find, void* context, callback_fn callback);

bool trigram_string_is_searchable(const char* string);

#endif // TRIGRAM_H

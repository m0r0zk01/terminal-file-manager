#pragma once

#include "columns.h"
#include "utils.h"

#include <stdbool.h>

typedef struct Files {
    File *files;
    size_t size;
    size_t capacity;
} Files;

bool FilesInit(Files *files);
bool FilesPushBack(Files *files, File file);
bool FilesDestroy(Files *files);
bool FilesClear(Files *files);

bool FetchFiles(Files *files, char *dir, size_t size);
void SortFiles(Files *files, int col, bool descending);

char *EnsureEndSlash(char *pathBegin, char *pathEnd);
char *EnsureNoEndSlash(char *pathBegin, char *pathEnd);

char *GetExtension(char *pathBegin, char *pathEnd);

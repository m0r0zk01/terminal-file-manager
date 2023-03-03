#include "files.h"
#include "columns.h"
#include "utils.h"

#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool FilesInit(Files *files) {
    ReturnIfNot(files, true);

    Files newFiles;
    newFiles.size = 0;
    newFiles.capacity = 1;
    newFiles.files=calloc(newFiles.capacity, sizeof(*newFiles.files));
    memset(newFiles.files, 0, newFiles.capacity * sizeof(*newFiles.files));
    if (newFiles.files == NULL) {
        return false;
    }
    *files = newFiles;
    return true;
}

bool FilesPushBack(Files *files, File file) {
    ReturnIfNot(files, true);

    if (files->size == files->capacity) {
        File *newFiles = realloc(files->files, (files->capacity <<= 1) * sizeof(*files->files));
        if (newFiles == NULL) {
            free(files->files);
            return false;
        }
        memset(newFiles + files->size, 0, (files->capacity - files->size) * sizeof(*files->files));
        files->files = newFiles;
    }

    files->files[files->size++] = file;
    return true;
}

bool FilesDestroy(Files *files) {
    ReturnIfNot(files, true);

    if (files->files) {
        for (size_t i = 0; i < files->size; ++i) {
            for (int j = 0; j < CNT_COLUMNS; ++j) {
                COLUMNS[j].destroy(files->files + i);
            }
        }
    }
    free(files->files);

    files->capacity = files->size = 0;
    files->files = NULL;

    return true;
}

bool FilesClear(Files *files) {
    ReturnIfNot(files, true);

    FilesDestroy(files);
    return FilesInit(files);
}

char *EnsureEndSlash(char *pathBegin, char *pathEnd) {
    if (pathBegin == pathEnd || *(pathEnd - 1) != '/') {
        *(pathEnd++) = '/';
        *pathEnd = '\0';
    }
    return pathEnd;
}

char *EnsureNoEndSlash(char *pathBegin, char *pathEnd) {
    while (pathEnd > pathBegin + 1 && *(pathEnd - 1) == '/') {
        *(--pathEnd) = '\0';
    }
    return pathEnd;
}

bool FetchFiles(Files *files, char *dir, size_t size) {
    ReturnIfNot(files, true);
    ReturnIfNot(FilesClear(files), false);

    char *end = dir + strnlen(dir, size);
    end = EnsureEndSlash(dir, end);

    DIR *dp = opendir(dir);
    DirEnt *dirent = NULL;
    while ((dirent = readdir(dp))) {
        strncpy(end, dirent->d_name, size - 1 - (end - dir));
        File newFile;
        Stat st;
        stat(dir, &st);
        for (int i = 0; i < CNT_COLUMNS; ++i) {
            COLUMNS[i].toFile(&newFile, dir, dirent, &st);
        }
        ReturnIfNot(FilesPushBack(files, newFile), false);
    }
    closedir(dp);
    *end = '\0';

    return true;
}

void SortFiles(Files *files, int col, bool descending) {
    if (!files) {
        return;
    }

    qsort(files->files, files->size, sizeof(*files->files), COLUMNS[col].compar);
    if (descending) {
        for (int i = 0; i < files->size / 2; ++i) {
            Swap(files->files[i], files->files[files->size - 1 - i]);
        }
    }
}

char *GetExtension(char *pathBegin, char *pathEnd) {
    char *ptr = pathEnd;
    while (ptr > pathBegin && *ptr != '.') {
        --ptr;
    }
    return *ptr == '.' ? ptr + 1 : pathEnd;
}

#pragma once

#include "utils.h"

#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

typedef enum Alignment {
    ALIGN_LEFT = 0,
    ALIGN_RIGHT = 1
} Alignment;

typedef struct FileMetaInfo {
    TypeOfField(Stat, st_ino) ino;
    TypeOfField(DirEnt, d_type) type; 
} FileMetaInfo;

#define FOREACH_COLUMN(ADD)                                                           \
/*          colName         type           nameString    maxWidth       colAlign    */\
        ADD(Name,           char *,        Name,       1000,          ALIGN_LEFT     )\
        ADD(Permissions,    int,           Mode,       13,            ALIGN_LEFT     )\
        ADD(Meta,           FileMetaInfo,  Meta,       0,             ALIGN_LEFT     )\
        ADD(Size,           size_t,        Size,       18,            ALIGN_RIGHT    )\
        ADD(EditDateSec,    time_t,        Edit,       19,            ALIGN_RIGHT    )\

#define ADD_TO_FILE(colName, type, ...)    \
        type colName;                      \

typedef struct File {
    FOREACH_COLUMN(ADD_TO_FILE)
} File;

typedef struct Column {
    const char *name;
    int width;
    int maxWidth;
    Alignment align;
    void(*toFile)(File *file, const char *dir, const DirEnt *dirent, const Stat *st);
    void(*destroy)(File *file);
    void(*toString)(File *file, char *buf, size_t size);
    int (*compar)(const void *f1, const void *f2);
} Column;

#define DECLARE_FUNCS(colName, ...)                                                                     \
        void colName##ToFile(File *file, const char *dir, const DirEnt *dirent, const Stat *st);        \
        void colName##Destroy(File *file);                                                              \
        void colName##ToString(File *file, char *buf, size_t size);                                     \
        int  colName##Compar(const void *f1, const void *f2);                                           \

FOREACH_COLUMN(DECLARE_FUNCS)

void MetaInfoToFile(File *file, const char *dir, const DirEnt *dirent, const Stat *st);

extern int LEFT_SIZE, RIGHT_SIZE;

void UpdateColumnWidths(int W);

#define REGISTER_COLUMN(colName, type, nameString, colMaxWidth, colAlign)  \
        {                                                                  \
            .name=#nameString,                                             \
            .width = 0,                                                    \
            .maxWidth=colMaxWidth,                                         \
            .align=colAlign,                                               \
            .toFile=colName##ToFile,                                       \
            .destroy=colName##Destroy,                                     \
            .toString=colName##ToString,                                   \
            .compar=colName##Compar,                                       \
        },                                                                 \

extern Column COLUMNS[];

extern const int CNT_COLUMNS;

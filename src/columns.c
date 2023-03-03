#include "columns.h"
#include "utils.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define st_atim st_atimespec
#define st_mtim st_mtimespec
#define st_ctim st_ctimespec
#define st_birthtime st_birthtimespec.tv_sec
#endif /* (!_POSIX_C_SOURCE || _DARWIN_C_SOURCE) */

Column COLUMNS[] = {
    FOREACH_COLUMN(REGISTER_COLUMN)
};
const int CNT_COLUMNS = sizeof(COLUMNS) / sizeof(COLUMNS[0]);

enum { SUID = 04, SGID = 02, STICKY_BIT = 01 };
enum { PERM_R = 04, PERM_W = 02, PERM_X = 01 };
enum { SPECIAL = 9, OWNER = 6, GROUP = 3, OTHER = 0 };
enum { PERMS_MAX_LEN = 10, OWNER_X_IND = 2, GROUP_X_IND = 5, OTHER_X_IND = 8 };

static int IsPermSet(int perms, int what, int who) {
    return (perms >> who) & what;
}

static const char *PermsToString(char *buf, size_t size, int perms) {
    ReturnIfNot(size, buf);

    size = Min(size, PERMS_MAX_LEN);
    for (int i = 0; i < size - 1; ++i) {
        buf[i] = (perms >> (PERMS_MAX_LEN - 2 - i)) & 1 ? "rwx"[i % 3] : '-';
    }

    if (OWNER_X_IND < size - 1 && IsPermSet(perms, SUID, SPECIAL) &&
        (IsPermSet(perms, PERM_X, GROUP) || IsPermSet(perms, PERM_X, OTHER))) {
        buf[OWNER_X_IND] = 's';
    }
    if (GROUP_X_IND < size - 1 && IsPermSet(perms, SGID, SPECIAL) &&
        IsPermSet(perms, PERM_X, OTHER)) {
        buf[GROUP_X_IND] = 's';
    }
    if (OTHER_X_IND < size - 1 && IsPermSet(perms, STICKY_BIT, SPECIAL) &&
        IsPermSet(perms, PERM_W, OTHER) && IsPermSet(perms, PERM_X, OTHER)) {
        buf[OTHER_X_IND] = 't';
    }

    buf[size - 1] = '\0';
    return buf;
}

// Name ------------------------------------------------------------------------

void NameToFile(File *file, const char *dir, const DirEnt *dirent, const Stat *st) {
    char *actualPath = NULL;
    size_t actualPathLen = 0;
    if (dirent->d_type == DT_LNK) {
        actualPath = realpath(dir, NULL);
        if (actualPath) {
            actualPathLen = strlen(actualPath);
        }
    }
    size_t nameSize = strlen(dirent->d_name) + (actualPath ? actualPathLen + 4 : 0) + 1;
    file->Name = malloc(nameSize);
    char *ptr = stpcpy(file->Name, dirent->d_name);
    if (actualPath) {
        ptr = stpcpy(ptr, " -> ");
        stpcpy(ptr, actualPath);
        free(actualPath);
    }
    file->Name[nameSize - 1] = '\0';
}

void NameDestroy(File *file) {
    free(file->Name);
}

void NameToString(File *file, char *buf, size_t size) {
    strncpy(buf, file->Name, size - 1);
    buf[size - 1] = '\0';
}

int NameCompar(const void *f1, const void *f2) {
    return strcmp(((const File *)f1)->Name, ((const File *)f2)->Name);
}

// Size ------------------------------------------------------------------------

void SizeToFile(File *file, const char *dir, const DirEnt *dirent, const Stat *st) {
    file->Size = st->st_size;
}

void SizeDestroy(File *file) {

}

void SizeToString(File *file, char *buf, size_t size) {
    snprintf(buf, size - 1, "%jd", file->Size);
    buf[size - 1] = '\0';
}

int SizeCompar(const void *f1, const void *f2) {
    return ((const File *)f1)->Size - ((const File *)f2)->Size;
}

// Permissions -----------------------------------------------------------------

void PermissionsToFile(File *file, const char *dir, const DirEnt *dirent, const Stat *st) {
    file->Permissions = st->st_mode;
}

void PermissionsDestroy(File *file) {

}

void PermissionsToString(File *file, char *buf, size_t size) {
    PermsToString(buf, size - 1, file->Permissions);
    buf[size - 1] = '\0';
}

int PermissionsCompar(const void *f1, const void *f2) {
    return ((const File *)f1)->Permissions - ((const File *)f2)->Permissions;
}

// Meta ------------------------------------------------------------------------

void MetaToFile(File *file, const char *dir, const DirEnt *dirent, const Stat *st) {
    file->Meta.ino = st->st_ino;
    file->Meta.type = dirent->d_type;
}

void MetaDestroy(File *file) {

}

void MetaToString(File *file, char *buf, size_t size) {

}

int MetaCompar(const void *f1, const void *f2) {
    return 0;
}

// Edit ------------------------------------------------------------------------

void EditDateSecToFile(File *file, const char *dir, const DirEnt *dirent, const Stat *st) {
    file->EditDateSec = st->st_mtim.tv_sec;
}

void EditDateSecDestroy(File *file) {

}

void EditDateSecToString(File *file, char *buf, size_t size) {
    struct tm  ts;
    memset(&ts, 0, sizeof(struct tm));
    localtime_r(&file->EditDateSec, &ts);
    ts.tm_sec = file->EditDateSec;
    strftime(buf, size, "%Y.%m.%d %H:%M", &ts);
}

int EditDateSecCompar(const void *f1, const void *f2) {
    return difftime(((const File *)f1)->EditDateSec, ((const File *)f2)->EditDateSec);
}

// -----------------------------------------------------------------------------

int LEFT_SIZE = 0, RIGHT_SIZE = 0;

void UpdateColumnWidths(int W) {
    int right = W / 2;
    int left = W - right;
    int cntLeft = 0, cntRight = 0;
    for (int i = 0; i < CNT_COLUMNS; ++i) {
        COLUMNS[i].width = 0;
        (*(COLUMNS[i].align == ALIGN_LEFT ? &cntLeft : &cntRight))++;
    }
    LEFT_SIZE = RIGHT_SIZE = 0;

    while ((right > 1 && cntRight > 0) || (left > 1 && cntLeft > 0)) {
        if (cntRight == 0) {
            left += right;
            right = 0;
        }
        if (cntLeft == 0) {
            right += left;
            left = 0;
        }
        int partLeft = cntLeft ? (left + cntLeft - 1) / cntLeft : 0;
        int partRight = cntRight ? (right + cntRight - 1) / cntRight : 0;
        for (int i = 0; i < CNT_COLUMNS; ++i) {
            if (COLUMNS[i].align == ALIGN_LEFT && cntLeft > 0 && COLUMNS[i].width < COLUMNS[i].maxWidth) {
                int add = Min(COLUMNS[i].maxWidth - COLUMNS[i].width, partLeft);
                COLUMNS[i].width += add;
                LEFT_SIZE += add;
                left -= add;
                if (COLUMNS[i].width >= COLUMNS[i].maxWidth) {
                    --cntLeft;
                }
            } else if (cntRight > 0 && COLUMNS[i].width < COLUMNS[i].maxWidth) {
                int add = Min(COLUMNS[i].maxWidth - COLUMNS[i].width, partRight);
                COLUMNS[i].width += add;
                RIGHT_SIZE += add;
                right -= add;
                if (COLUMNS[i].width >= COLUMNS[i].maxWidth) {
                    --cntRight;
                }
            }
        }
    }
}

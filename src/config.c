#include "config.h"
#include <dirent.h>

char *ColorOfFile(File *file) {
    switch (file->Meta.type) {
        case DT_DIR: return YEL;
        case DT_REG: return CYN;
        case DT_LNK: return RED;
        case DT_CHR: return MAG;
        default:     return WHT;
    }
}

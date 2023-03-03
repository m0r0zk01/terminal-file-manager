#pragma once

#include <sys/dir.h>
#include <sys/stat.h>

#define Max(a, b)                    \
    ({                               \
        __typeof__ (a) ___a = (a);   \
        __typeof__ (b) ___b = (b);   \
        ___a > ___b ? ___a : ___b;   \
    })                               \

#define Min(a, b)                    \
    ({                               \
        __typeof__ (a) ___a = (a);   \
        __typeof__ (b) ___b = (b);   \
        ___a < ___b ? ___a : ___b;   \
    })                               \

#define Swap(x,y) do                                                             \
    { unsigned char swap_temp[sizeof(x) == sizeof(y) ? (signed)sizeof(x) : -1];  \
      memcpy(swap_temp,&y,sizeof(x));                                            \
      memcpy(&y,&x,       sizeof(x));                                            \
      memcpy(&x,swap_temp,sizeof(x));                                            \
    } while(0)                                                                   \

typedef struct stat Stat;
typedef struct dirent DirEnt;

#define ReturnIfNot(val, ret)       \
        do {                        \
            if (!(val)) {           \
                return (ret);       \
            }                       \
        } while (0)                 \

#define SizeOfField(type, field) sizeof(((type *)0)->field)
#define TypeOfField(type, field) __typeof__(((type *)0)->field)

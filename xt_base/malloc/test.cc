
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "local_malloc.h"

bool sMemory::useLocalMalloc = true;
bool sMemory::returnClear = false;

const int onemeg = 1024*1024;

int main()
{
    int size = 52;
    int cnt = 0;
    int ct = onemeg;
    while (1) {
#ifdef __cplusplus
        char *a = new char[size];
#else
        char *a = (char*)malloc(size);
#endif
        if (!a)
            break;
//        memset(a, 0, size);
        cnt += size;
        if (cnt > ct) {
            printf("%d %lx %lx\n", cnt/onemeg, (long)a, (long)sbrk(0));
            ct += onemeg;
        }
    }
    return (0);
}


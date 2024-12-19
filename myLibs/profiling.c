
#include "inttypes.h"
#include <stdlib.h>

#define ENABLED

u64 mallocs = 0, frees = 0;

#ifdef ENABLED
#define malloc profiling_malloc
#define free profiling_free
#endif

void *profiling_malloc(u32 size) {
    mallocs++;

    #ifdef malloc
    #undef malloc
    void *result = malloc(size);
    #define malloc profiling_malloc
    #endif

    return result;
}

void profiling_free(void *mem) {
    frees++;

    #ifdef free
    #undef free
    free(mem);
    #define free profiling_free
    #endif

}
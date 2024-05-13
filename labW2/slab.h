#include <stddef.h>

void* mem_alloc(size_t);
void mem_free(void*);
void mem_show(const char*);
void* mem_realloc(void*, size_t);

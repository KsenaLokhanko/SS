#include <stddef.h>

#include "kernel.h"

#ifdef _WIN32
#include <Windows.h>

void* kernel_alloc(size_t size) {
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void kernel_free(void* ptr) {
    VirtualFree(ptr, 0, MEM_RELEASE);
}

#else

#include <sys/mman.h>

void* kernel_alloc(size_t size) {
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void kernel_free(void* ptr) {
    munmap(ptr, 0);
}

#endif

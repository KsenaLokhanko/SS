#include <stdio.h>
#include "slab.h"
#include "tester.h"
#include "config.h"

int main() {
    void* obj1 = mem_alloc(sizeof(int));
    void* obj2 = mem_alloc(sizeof(int));

    void* obj3 = mem_alloc(666);
    mem_show("Memory state after creating obj1-3:");
    getchar();

    void* obj4 = mem_alloc(9999);
    mem_show("Memory state after creating obj4:");
    getchar();

    obj2 = mem_realloc(obj2, 100);
    mem_show("Memory state after reallocation of obj2:");
    getchar();

    mem_free(obj1);
    mem_show("Memory state after free obj1:");
    getchar();

    mem_free(obj2);
    mem_show("Memory state after free obj2:");
    getchar();

    const int N = PAGE_NUM*15*4;
    void* array[N];
    for (int i = 0; i < N; i++) {
        array[i] = mem_alloc(4024);
    }
    mem_show("Full memory use:\n");
    getchar();

    void* obj5 = mem_alloc(4);
    mem_show("Full memory use and add 1 new object:\n");

//    tester(true);

    return 0;
}

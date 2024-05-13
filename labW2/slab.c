#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernel.h"
#include "config.h"

typedef struct slab_header {
    size_t size;  // Розмір об'єкта в slab
    size_t used;  // Кількість використовуваних об'єктів
    void* free;  // Вказівник на вільний блок
} slab_header_t;

typedef struct slab {
    slab_header_t header;
    char data[PAGE_NUM * PAGE_SIZE];  // Дані slab
} slab_t;

typedef struct cache {
    slab_t* slabs[CACHE_SIZE];  // Кеш з заданою розміром
    size_t count;               // Кількість slab в кеші
} cache_t;

cache_t cache;  // Глобальний кеш

slab_t* alloc_slab(size_t size) {
    slab_t* slab = kernel_alloc(sizeof(slab_t));
    if (!slab) return NULL;

    slab->header.size = size;
    slab->header.used = 0;
    slab->header.free = slab->data;

    return slab;
}

void free_slab(slab_t* slab) {
    kernel_free(slab);
}

void* mem_alloc(size_t size) {
    if (size > MAX_OBJ_SIZE) {
        fprintf(stderr, "Error: The size of the object exceeds the maximum size\n");
        return NULL;
    }

    // Перевірка кешу
    for (int i = 0; i < cache.count; i++) {
        slab_t* slab = cache.slabs[i];

        if (slab->header.size == size && slab->header.free && slab->header.used < PAGE_SIZE * PAGE_NUM / slab->header.size) {
            slab->header.used++;
            void* ptr = slab->header.free;
            slab->header.free = (char*)slab->header.free + size;
            return ptr;
        }
    }

    // Створення нового slab
    slab_t* slab = alloc_slab(size);
    if (!slab) return NULL;

    slab->header.used = 1;
    slab->header.free = slab->data + size;

    // Додати slab до кешу
    if (cache.count < CACHE_SIZE) {
        cache.slabs[cache.count++] = slab;
    } else {
        // Якщо кеш повний, потрібно видалити найменш використовуваний slab
        int victim_index = -1;
        size_t min_used = (size_t)-1;
        for (int i = 0; i < cache.count; i++) {
            slab_t* curr_slab = cache.slabs[i];
            if (curr_slab->header.used < min_used) {
                victim_index = i;
                min_used = curr_slab->header.used;
            }
        }

        if (victim_index == -1) {
            fprintf(stderr, "Error: Cache full but least used slab not found\n");
            return NULL;
        }

        free_slab(cache.slabs[victim_index]);
        cache.slabs[victim_index] = slab;
    }

    return slab->data;
}

void mem_free(void* ptr) {
    // Перебір усіх slab у кеші
    for (int i = 0; i < cache.count; i++) {
        slab_t* slab = cache.slabs[i];

        // Перевірка, чи адреса ptr належить до діапазону даного slab
        if (ptr >= (void*)slab->data && ptr < (void*)(slab->data + PAGE_NUM * PAGE_SIZE)) {
            // Звільнення об'єкта
            slab->header.used--;

            // Якщо всі об'єкти у slab вільні, потрібно звільнити slab цілком
            if (slab->header.used == 0) {
                // Пошук відповідного slab у кеші
                for (int j = i; j < cache.count - 1; j++) {
                    cache.slabs[j] = cache.slabs[j + 1];
                }
                cache.count--;

                // Звільнення пам'яті slab
                free_slab(slab);
            }
            return;
        }
    }

    // Якщо ptr не належить до жодного slab у кеші, видаємо помилку
    fprintf(stderr, "Error: Trying to delete an unknown object\n");
}

void* mem_realloc(void* ptr, size_t new_size) {
    // Перебір усіх slab у кеші
    for (int i = 0; i < cache.count; i++) {
        slab_t* slab = cache.slabs[i];

        // Перевірка, чи адреса ptr належить до діапазону даного slab
        if (ptr >= (void*)slab->data && ptr < (void*)(slab->data + PAGE_NUM * PAGE_SIZE)) {
            // Розрахунок поточного розміру об'єкта
            size_t current_size = slab->header.size;

            // Якщо новий розмір менший або рівний поточному розміру, просто повертаємо поточний показник
            if (new_size <= current_size) return ptr;

            // Перевірка, чи новий розмір вміщується в межах максимального розміру об'єкта
            if (new_size > MAX_OBJ_SIZE) {
                fprintf(stderr, "Error: new size larger than the maximum size of the object\n\n");
                return ptr;
            }

            // Виділення нового об'єкта потрібного розміру
            void* new_ptr = mem_alloc(new_size);
            if (!new_ptr) {
                fprintf(stderr, "Error: Unable to allocate memory for a new object\n\n");
                return ptr;
            }

            // Копіювання даних зі старого об'єкта в новий
            memcpy(new_ptr, ptr, current_size);

            // Звільнення пам'яті старого об'єкта
            mem_free(ptr);

            return new_ptr;
        }
    }

    // Якщо ptr не належить до жодного slab у кеші, видаємо помилку
    fprintf(stderr, "Error: attempting to resize an unknown object\n\n");
    return NULL;
}

void slab_info(slab_t* slab, int slab_num) {
    printf("\nSlab %d (%p)\n", slab_num, (void*)slab);
    printf("   Object size:\t%zu Byte\n", slab->header.size);
    printf("   Used:\t%zu/%zu\n", slab->header.used, PAGE_SIZE * PAGE_NUM / slab->header.size);
    printf("   Status:\t%s\n\n", slab->header.used == 0 ? "empty" : (slab->header.used == PAGE_SIZE * PAGE_NUM / slab->header.size ? "full" : "partial"));
}

void mem_show(const char* message) {
    printf(message);
    for (int i = 0; i < cache.count; i++) {
        slab_info(cache.slabs[i], i + 1);
    }
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"


    );
}

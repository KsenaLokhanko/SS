#include <stddef.h>
#include <stdio.h>
#include <intrin.h>

#include "allocator.h"
#include "block.h"
#include "config.h"
#include "kernel.h"

static struct block *arena = NULL;

#define ARENA_SIZE (ALLOCATOR_ARENA_PAGES * ALLOCATOR_PAGE_SIZE)
#define BLOCK_SIZE_MAX (ARENA_SIZE - BLOCK_STRUCT_SIZE)

static int
arena_alloc(void) {
    arena = kernel_alloc(ARENA_SIZE);
    if (arena != NULL) {
        arena_init(arena, ARENA_SIZE - BLOCK_STRUCT_SIZE);
        return 0;
    }
    return -1;
}

void *  // Функція, що працює з однією ареною, лінійне сканування блоків в арені, стратегія first fit
mem_alloc(size_t size) {
    struct block *block;

    if (arena == NULL && arena_alloc() < 0 || size > BLOCK_SIZE_MAX) return NULL;

    size = ROUND_BYTES(size);

    for (block = arena;; block = block_next(block)) {
        if (!block_get_flag_busy(block) && block_get_size_curr(block) >= size) {
            block_split(block, size);
            return block_to_payload(block);
        }
        if (block_get_flag_last(block)) break;
    }

    return NULL;
}

void
mem_free(void *ptr) {
    struct block *block, *block_r, *block_l;

    if (ptr == NULL) return;

    block = payload_to_block(ptr);
    block_clr_flag_busy(block);
    if (!block_get_flag_last(block)) {
        block_r = block_next(block);
        if (!block_get_flag_busy(block_r)) block_merge(block, block_r);
    }
    if (!block_get_flag_first(block)) {
        block_l = block_prev(block);
        if (!block_get_flag_busy(block_l)) block_merge(block_l, block);
    }
}

void *
mem_realloc(void *ptr, size_t size) {
    // Якщо вказаний покажчик є NULL, виділяємо новий блок пам'яті заданого розміру
    if (ptr == NULL) return mem_alloc(size);

    struct block *block = payload_to_block(ptr);

    // Перевіряємо, чи можна збільшити розмір поточного блоку
    size_t current_size = block_get_size_curr(block);
    if (size <= current_size) return ptr;
    size_t new_size = ROUND_BYTES(size);
    size_t increase = new_size - current_size;
    struct block *next_block = block_next(block);

    // Перевіряємо, чи наступний блок є вільним і чи можна розширити поточний блок
    if (!block_get_flag_last(block) && !block_get_flag_busy(next_block) && block_get_size_curr(next_block) >= increase) {
        // Розширюємо поточний блок, вільний простір відводимо на новий блок
        block_merge(block, next_block);
        block_split(block, new_size);
        return block_to_payload(block);
    } else {
        // Якщо не можна розширити поточний блок, виділяємо новий блок пам'яті потрібного розміру
        void *new_ptr = mem_alloc(size);
        if (new_ptr != NULL) {
            memcpy(new_ptr, ptr, current_size);
            mem_free(ptr);
            return new_ptr;
        }
//        return ptr;
    }
}

void    // Виводить вміст заголовків всіх блоків арени
mem_show(const char *msg) {
    const struct block *block;

    printf("%s:\n", msg);
    if (arena == NULL) {
        printf("Arena was not created\n");
        return;
    }

    printf("= Address =  Current  =  Prev  = Flags\n");
    for (block = arena;; block = block_next(block)) {
        printf(" %p | %6zu | %6zu | %s %s %s\n",
            (void *)block, block_get_size_curr(block), block_get_size_prev(block),
            block_get_flag_busy(block) ? "busy" : "free",
            block_get_flag_first(block) ? " first " : "",
            block_get_flag_last(block) ? " last" : "");

        if (block_get_flag_last(block)) break;
    }
}

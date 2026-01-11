#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<stdbool.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;

typedef i8 b8;
typedef i32 b32;

#define KiB(n) ((ui64)(n) << 10)
#define MiB(n) ((ui64)(n) << 20)
#define GiB(n) ((ui64)(n) << 30)

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define ALIGN_UP_POW2(n,p) (((ui64)(n) + ((ui64)(p) - 1)) & (~((ui64)(p) - 1)))

#define ARENA_BASE_POS (sizeof(mem_arena))
#define ARENA_ALIGN (sizeof(void*))

typedef struct {
    ui64 capacity;
    ui64 pos;
} mem_arena;

mem_arena* arena_create(ui64 capacity);
void arena_destroy(mem_arena* arena);
void* arena_push(mem_arena* arena, ui64 size, b32 non_zero);
void arena_pop(mem_arena* arena, ui64 size);
void arena_pop_to(mem_arena* arena, ui64 pos);
void arena_clear(mem_arena* arena);

#define PUSH_STRUCT(arena, T) (T*)arena_push(arena, sizeof(T), 1)
#define PUSH_ARRAY(arena, T, count) (T*)arena_push(arena, sizeof(T) * (count), 1)

#include <time.h>

typedef struct {
    ui64 allocations;
    ui64 total_bytes;
    double time_elapsed;
} perf_stats;

int main() {
    printf("======== arena allocator test ========\n\n");
    ui64 arena_size = MiB(10);
    mem_arena* arena = arena_create(arena_size);
    if (!arena) {
        printf("failed to create arena\n");
        return 1;
    }
    printf("arena created with capacity: %llu bytes (%.2f mib)\n", arena_size, arena_size / (1024.0 * 1024.0));
    clock_t start = clock();
    ui64 allocation_count = 10000;
    ui64 total_allocated = 0;
    for (ui64 i = 0; i < allocation_count; i++) {
        ui64 size = 64 + (i % 512);
        void* ptr = arena_push(arena, size, 1);
        if (!ptr) {
            printf("arena exhausted at allocation %llu\n", i);
            break;
        }
        total_allocated += size;
        if (size > 0) {
            memset(ptr, 0xAA, size);
        }
    }
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("allocations performed: %llu\n", allocation_count);
    printf("total bytes allocated: %llu (%.2f mib)\n", total_allocated, total_allocated / (1024.0 * 1024.0));
    printf("arena usage: %.2f%%\n", (total_allocated / (double)arena_size) * 100);
    printf("time elapsed: %f seconds\n", elapsed);
    printf("allocation rate: %.2f allocations/second\n", allocation_count / elapsed);
    printf("throughput: %.2f mib/second\n", (total_allocated / (1024.0 * 1024.0)) / elapsed);
    printf("\n--- testing pop operation ---\n");
    ui64 prev_pos = arena->pos;
    arena_pop(arena, 1024);
    printf("popped 1024 bytes. position before: %llu, after: %llu\n", prev_pos, arena->pos);
    printf("\n--- testing arena_clear ---\n");
    printf("position before clear: %llu\n", arena->pos);
    arena_clear(arena);
    printf("position after clear: %llu\n", arena->pos);
    arena_destroy(arena);
    printf("\n======== test completed ========\n");
    return 0;
}

mem_arena* arena_create(ui64 capacity) {
    mem_arena* arena = (mem_arena*)malloc(capacity);
    arena->capacity = capacity;
    arena->pos = ARENA_BASE_POS;
    return arena;
}

void arena_destroy(mem_arena* arena) {
    free(arena);
}

void* arena_push(mem_arena* arena, ui64 size, b32 non_zero) {
    ui64 pos_aligned = ALIGN_UP_POW2(arena->pos, ARENA_ALIGN);
    ui64 new_pos = pos_aligned + size;

    if (new_pos > arena->capacity) { return NULL; }

    arena->pos = new_pos;

    ui8* out = (ui8*)arena + pos_aligned;

    if (non_zero) {
        memset(out, 0, size);
    }

    return out;
}

void arena_pop(mem_arena* arena, ui64 size) {
    size = MIN(size, arena->pos - ARENA_BASE_POS);
    arena->pos -= size;
}

void arena_pop_to(mem_arena* arena, ui64 pos) {
    ui64 size = pos < arena->pos ? arena->pos - pos : 0;
    arena_pop(arena, size);
}

void arena_clear(mem_arena* arena) {
    arena_pop_to(arena, ARENA_BASE_POS);
}
#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <stddef.h>

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

#endif // ARENA_H

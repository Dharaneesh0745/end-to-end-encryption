#ifndef ARENA_H
#define ARENA_H

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
#include <stdexcept>

typedef uint64_t ui64;
typedef uint8_t ui8;
typedef int32_t i32;

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ALIGN_UP_POW2(n, p) (((ui64)(n) + ((ui64)(p) - 1)) & (~((ui64)(p) - 1)))
#define ARENA_BASE_POS (sizeof(MemArena))
#define ARENA_ALIGN (sizeof(void*))

// fixed-size pool allocator
class MemArena {
private:
    ui8* buffer;
    ui64 capacity;
    ui64 pos;
    std::vector<ui64> checkpoints;

public:
    // allocate the pool
    MemArena(ui64 capacity_) : capacity(capacity_), pos(ARENA_BASE_POS), checkpoints() {
        buffer = (ui8*)malloc(capacity);
        if (buffer == nullptr) {
            throw std::runtime_error("Failed to allocate arena buffer!");
        }
        memset(buffer, 0, capacity);
    }

    // cleanup
    ~MemArena() {
        if (buffer != nullptr) {
            free(buffer);
            buffer = nullptr;
        }
        checkpoints.clear();
    }

    // push--data--onto--stack
    void* push(ui64 size, i32 non_zero = 0) {
        ui64 pos_aligned = ALIGN_UP_POW2(pos, ARENA_ALIGN);
        ui64 new_pos = pos_aligned + size;

        if (new_pos > capacity) {
            throw std::overflow_error("Arena overflow!");
        }

        pos = new_pos;
        ui8* out = buffer + pos_aligned;

        if (!non_zero) {
            memset(out, 0, size);
        }

        return out;
    }

    // undo allocation
    void pop(ui64 size) {
        size = MIN(size, pos - ARENA_BASE_POS);
        pos -= size;
    }

    ui64 get_pos() const {
        return pos;
    }

    ui64 get_capacity() const {
        return capacity;
    }

    // how much we used
    ui64 get_used() const {
        return pos - ARENA_BASE_POS;
    }

    ui64 get_available() const {
        return capacity - pos;
    }

    // reset everything
    void clear() {
        pos = ARENA_BASE_POS;
        checkpoints.clear();
    }
};

#endif // ARENA_H

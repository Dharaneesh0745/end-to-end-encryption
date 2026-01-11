#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<time.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;

#define MiB(n) ((ui64)(n) << 20)

int main() {
    printf("======== normal allocator test (malloc/free) ========\n\n");
    
    // perform allocations and measure time
    clock_t start = clock();
    
    ui64 allocation_count = 10000;
    ui64 total_allocated = 0;
    
    // create an array to store pointers for deallocation
    void** ptrs = (void**)malloc(sizeof(void*) * allocation_count);
    
    if (!ptrs) {
        printf("failed to allocate pointer array\n");
        return 1;
    }
    
    printf("using malloc/free allocator\n");
    
    // allocate memory blocks
    for (ui64 i = 0; i < allocation_count; i++) {
        ui64 size = 64 + (i % 512); // allocate between 64 and 576 bytes (same as arena test)
        void* ptr = malloc(size);
        
        if (!ptr) {
            printf("malloc failed at allocation %llu\n", i);
            break;
        }
        
        ptrs[i] = ptr;
        total_allocated += size;
        
        // simulate some work with allocated memory
        if (size > 0) {
            memset(ptr, 0xAA, size);
        }
    }
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("allocations performed: %llu\n", allocation_count);
    printf("total bytes allocated: %llu (%.2f mib)\n", total_allocated, total_allocated / (1024.0 * 1024.0));
    printf("time elapsed: %f seconds\n", elapsed);
    printf("allocation rate: %.2f allocations/second\n", allocation_count / elapsed);
    printf("throughput: %.2f mib/second\n", (total_allocated / (1024.0 * 1024.0)) / elapsed);
    
    // now deallocate everything
    printf("\n--- deallocating memory ---\n");
    clock_t dealloc_start = clock();
    
    for (ui64 i = 0; i < allocation_count; i++) {
        if (ptrs[i]) {
            free(ptrs[i]);
        }
    }
    
    clock_t dealloc_end = clock();
    double dealloc_elapsed = (double)(dealloc_end - dealloc_start) / CLOCKS_PER_SEC;
    
    printf("deallocation time: %f seconds\n", dealloc_elapsed);
    printf("deallocation rate: %.2f deallocations/second\n", allocation_count / dealloc_elapsed);
    
    // cleanup
    free(ptrs);
    
    printf("\n--- summary ---\n");
    printf("total time (alloc + dealloc): %f seconds\n", elapsed + dealloc_elapsed);
    printf("average time per allocation: %f microseconds\n", (elapsed * 1000000) / allocation_count);
    printf("average time per deallocation: %f microseconds\n", (dealloc_elapsed * 1000000) / allocation_count);
    
    printf("\n======== test completed ========\n");
    
    return 0;
}

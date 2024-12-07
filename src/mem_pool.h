#ifndef MEM_POOL_H
#define MEM_POOL_H

#define _GNU_SOURCE

#include <sys/mman.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifndef ALIGNMENT_BYTES
#define ALIGNMENT_BYTES 8
#endif

typedef struct MemPool
{
    void *base;       // Base pointer to the mapped memory region
    size_t size;      // Total size of the MemPool (including header)
    size_t head;      // Current allocation position
    bool is_active;   // Flag to track if MemPool is valid/initialized
    size_t page_size; // System page size for alignment

#ifdef DEBUG
    size_t total_allocations;     // Track number of allocations made
    size_t peak_usage;            // Track maximum memory usage
    time_t creation_time;         // When the pool was created
    char *creator_file;           // Source file that created the pool
    int creator_line;             // Line number where pool was created
    size_t failed_allocations;    // Count of failed allocation attempts
    size_t total_bytes_requested; // Total bytes requested (before alignment)
#endif
} MemPool;

typedef enum
{
    POOL_COPY_OVERWRITE,
    POOL_COPY_APPEND
} PoolCopyMode;

#ifdef DEBUG
#define pool_build(size) pool_build_debug(size, __FILE__, __LINE__)
MemPool *pool_build_debug(size_t size, const char *file, int line);
#else
MemPool *pool_build(size_t size);
#endif

void *pool_fill(MemPool *pool, size_t size);

void pool_drain(MemPool *pool);

void pool_destroy(MemPool *pool);

size_t pool_measure(MemPool *pool);

MemPool *pool_resize(MemPool *pool, size_t size);

void pool_copy(MemPool *from, MemPool *to, PoolCopyMode mode);

#ifdef DEBUG
void pool_print_stats(const MemPool *pool);
double pool_get_fragmentation(const MemPool *pool);
size_t pool_get_peak_usage(const MemPool *pool);
#endif

#endif
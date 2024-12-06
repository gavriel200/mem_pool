#ifndef MEM_POOL_H
#define MEM_POOL_H

#define _GNU_SOURCE

#include <sys/mman.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>

#ifndef ALIGNMENT_BYTES
#define ALIGNMENT_BYTES 7
#endif

typedef struct MemPool
{
    void *base;       // Base pointer to the mapped memory region
    size_t size;      // Total size of the MemPool (including header)
    size_t head;      // Current allocation position
    bool is_active;   // Flag to track if MemPool is valid/initialized
    size_t page_size; // System page size for alignment
} MemPool;

MemPool *pool_build(size_t size);

void *pool_fill(MemPool *pool, size_t size);

void pool_drain(MemPool *pool);

void pool_destroy(MemPool *pool);

size_t pool_measure(MemPool *pool);

MemPool *pool_resize(MemPool *pool, size_t size);

#endif
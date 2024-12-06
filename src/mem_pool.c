#include "mem_pool.h"

MemPool *pool_build(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    size_t page_size = sysconf(_SC_PAGESIZE);

    size_t total_size = sizeof(MemPool) + size;

    size_t aligned_size = (total_size + page_size - 1) & ~(page_size - 1);

    void *memory = mmap(NULL,                        // Let system choose address
                        aligned_size,                // Size rounded to page size
                        PROT_READ | PROT_WRITE,      // Read/write permissions
                        MAP_PRIVATE | MAP_ANONYMOUS, // Private memory, not backed by file
                        -1,                          // No file descriptor
                        0);                          // No offset

    if (memory == MAP_FAILED)
    {
        return NULL;
    }

    MemPool *pool = (MemPool *)memory;
    pool->base = memory;
    pool->size = aligned_size;
    pool->head = sizeof(MemPool); // Start allocations after header
    pool->is_active = true;
    pool->page_size = page_size;

    return pool;
}

void *pool_fill(MemPool *pool, size_t size)
{
    if (!pool || !pool->is_active || size == 0)
    {
        return NULL;
    }

    size_t aligned_size = (size + ALIGNMENT_BYTES) & ~ALIGNMENT_BYTES;

    if (pool->head + aligned_size > pool->size)
    {
        return NULL;
    }

    void *ptr = (char *)pool->base + pool->head;

    pool->head += aligned_size;

    return ptr;
}

void pool_drain(MemPool *pool)
{
    if (!pool || !pool->is_active)
    {
        return;
    }

    pool->head = sizeof(MemPool);
}

void pool_destroy(MemPool *pool)
{
    if (!pool || !pool->is_active)
    {
        return;
    }

    pool->is_active = false;

    munmap(pool->base, pool->size);
}

size_t pool_measure(MemPool *pool)
{
    if (!pool || !pool->is_active)
    {
        return 0;
    }

    return pool->size - pool->head;
}

MemPool *pool_resize(MemPool *pool, size_t size)
{
    if (!pool || !pool->is_active || size == 0)
    {
        return NULL;
    }

    size_t aligned_size = (size + pool->page_size - 1) & ~(pool->page_size - 1);

    if (aligned_size == pool->size)
    {
        return pool;
    }

    if (aligned_size < pool->size)
    {
        if (pool->head > aligned_size)
        {
            return NULL;
        }
    }

    void *new_memory = mremap(pool->base, pool->size, aligned_size, MREMAP_MAYMOVE);
    if (new_memory == MAP_FAILED)
    {
        return NULL;
    }

    MemPool *new_pool = (MemPool *)new_memory;
    new_pool->base = new_memory;
    new_pool->size = aligned_size;
    return new_pool;
}

void pool_copy(MemPool *from, MemPool *to, PoolCopyMode mode)
{

    if (!from || !to || !from->is_active || !to->is_active)
    {
        return;
    }

    size_t data_size = from->head - sizeof(MemPool);

    if (mode == POOL_COPY_OVERWRITE)
    {
        to->head = sizeof(MemPool);
    }

    size_t available = pool_measure(to);
    if (data_size > available)
    {
        return;
    }

    char *src = (char *)from->base + sizeof(MemPool);
    char *dst = (char *)to->base + to->head;

    memcpy(dst, src, data_size);

    to->head += data_size;
}
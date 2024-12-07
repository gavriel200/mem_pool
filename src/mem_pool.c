#include "mem_pool.h"

size_t aligne_size(size_t size, size_t alignement)
{
    return (size + alignement - 1) & ~(alignement - 1);
}

#ifdef DEBUG
MemPool *pool_build_debug(size_t size, const char *file, int line)
#else
MemPool *pool_build(size_t size)
#endif
{
    if (size == 0)
    {
        return NULL;
    }

    size_t page_size = sysconf(_SC_PAGESIZE);

    size_t total_size = sizeof(MemPool) + size;

    size_t aligned_size = aligne_size(total_size, page_size);

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

#ifdef DEBUG
    pool->total_allocations = 0;
    pool->creation_time = time(NULL);
    pool->creator_file = strdup(file);
    pool->creator_line = line;
    pool->failed_allocations = 0;
    pool->total_bytes_requested = 0;
#endif

    return pool;
}

void *pool_fill(MemPool *pool, size_t size)
{
    if (!pool || !pool->is_active || size == 0)
    {
#ifdef DEBUG
        if (pool && pool->is_active)
        {
            pool->failed_allocations++;
        }
#endif
        return NULL;
    }

#ifdef DEBUG
    pool->total_bytes_requested += size;
#endif

    size_t aligned_size = aligne_size(size, ALIGNMENT_BYTES);

    if (pool->head + aligned_size > pool->size)
    {
#ifdef DEBUG
        pool->failed_allocations++;
#endif
        return NULL;
    }

    void *ptr = (char *)pool->base + pool->head;
    pool->head += aligned_size;

#ifdef DEBUG
    pool->total_allocations++;
    if (pool->head > pool->peak_usage)
    {
        pool->peak_usage = pool->head;
    }
#endif

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

    size_t aligned_size = aligne_size(size, pool->page_size);

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

#ifdef DEBUG
void pool_print_stats(const MemPool *pool)
{
    if (!pool || !pool->is_active)
    {
        printf("Invalid or inactive pool\n");
        return;
    }

    printf("Memory Pool Statistics:\n");
    printf("Created in: %s:%d\n", pool->creator_file, pool->creator_line);
    printf("Creation time: %s", ctime(&pool->creation_time));
    printf("Total size: %zu bytes\n", pool->size);
    printf("Currently used: %zu bytes (%.2f%%)\n",
           pool->head, (double)pool->head / pool->size * 100);
    printf("Peak usage: %zu bytes (%.2f%%)\n",
           pool->peak_usage, (double)pool->peak_usage / pool->size * 100);
    printf("Total allocations: %zu\n", pool->total_allocations);
    printf("Failed allocations: %zu\n", pool->failed_allocations);
    printf("Average allocation size: %.2f bytes\n",
           pool->total_allocations ? (double)pool->total_bytes_requested / pool->total_allocations : 0);
}

double pool_get_fragmentation(const MemPool *pool)
{
    if (!pool || !pool->is_active)
    {
        return 0.0;
    }

    size_t total_requested = pool->total_bytes_requested;
    size_t total_allocated = pool->head - sizeof(MemPool);

    return total_allocated > 0 ? (double)(total_allocated - total_requested) / total_allocated * 100 : 0;
}

size_t pool_get_peak_usage(const MemPool *pool)
{
    return pool && pool->is_active ? pool->peak_usage : 0;
}
#endif
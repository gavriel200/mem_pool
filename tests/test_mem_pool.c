#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mem_pool.h"

#define GREEN "\033[0;32m"
#define RESET "\033[0m"

#define TEST_START(name) printf("Testing %s:\n", name)
#define TEST_END(name) printf(GREEN "✓ %s passed!\n" RESET "\n", name)

void test_pool_creation()
{
    TEST_START("pool creation");

    MemPool *pool = pool_build(1024);
    assert(pool != NULL && "Pool creation failed");
    assert(pool->is_active && "Pool should be active after creation");
    assert(pool->size >= 1024 && "Pool size incorrect");
    pool_destroy(pool);

    pool = pool_build(0);
    assert(pool == NULL && "Zero size pool should return NULL");

    TEST_END("pool creation");
}

void test_basic_allocation()
{
    TEST_START("basic allocation");

    MemPool *pool = pool_build(1024);
    assert(pool != NULL && "Pool creation failed");

    int *num = pool_fill(pool, sizeof(int));
    assert(num != NULL && "Failed to allocate int");
    *num = 42;
    assert(*num == 42 && "Value corruption");

    char *str = pool_fill(pool, 16);
    assert(str != NULL && "Failed to allocate string");
    strcpy(str, "Hello, World!");
    assert(strcmp(str, "Hello, World!") == 0 && "String corruption");

    pool_destroy(pool);
    TEST_END("basic allocation");
}

void test_pool_measure()
{
    TEST_START("pool measurement");

    size_t pool_size = 1024;
    MemPool *pool = pool_build(pool_size);
    assert(pool != NULL);

    size_t initial_available = pool_measure(pool);
    assert(initial_available > 0 && "Initial measurement should be non-zero");

    void *ptr = pool_fill(pool, 256);
    assert(ptr != NULL);

    size_t after_alloc = pool_measure(pool);
    assert(after_alloc < initial_available && "Pool measure not decreasing");
    assert(after_alloc == initial_available - 256 - (256 % 8) &&
           "Incorrect remaining space calculation");

    pool_destroy(pool);
    TEST_END("pool measurement");
}

void test_pool_drain()
{
    TEST_START("pool drain");

    MemPool *pool = pool_build(1024);
    assert(pool != NULL);

    void *ptr1 = pool_fill(pool, 128);
    void *ptr2 = pool_fill(pool, 256);
    assert(ptr1 != NULL && ptr2 != NULL);

    size_t before_drain = pool_measure(pool);
    pool_drain(pool);
    size_t after_drain = pool_measure(pool);

    assert(after_drain > before_drain && "Pool drain failed to reclaim space");

    ptr1 = pool_fill(pool, 128);
    assert(ptr1 != NULL && "Failed to allocate after drain");

    pool_destroy(pool);
    TEST_END("pool drain");
}

void test_edge_cases()
{
    TEST_START("edge cases");

    void *ptr = pool_fill(NULL, 10);
    assert(ptr == NULL && "NULL pool check failed");
    pool_drain(NULL);
    pool_destroy(NULL);
    assert(pool_measure(NULL) == 0 && "NULL pool measure failed");

    MemPool *pool = pool_build(1024);
    assert(pool != NULL);

    ptr = pool_fill(pool, 0);
    assert(ptr == NULL && "Zero size allocation should return NULL");

    size_t pool_left = pool_measure(pool);
    ptr = pool_fill(pool, pool_left + 1);
    assert(ptr == NULL && "Oversized allocation should return NULL");

    pool_destroy(pool);
    TEST_END("edge cases");
}

void test_stress()
{
    TEST_START("stress conditions");

    MemPool *pool = pool_build(4096);
    assert(pool != NULL);

    void *ptrs[100];
    int successful_allocs = 0;

    for (int i = 0; i < 100; i++)
    {
        ptrs[i] = pool_fill(pool, 32);
        if (ptrs[i] == NULL)
            break;
        successful_allocs++;
        memset(ptrs[i], i, 32);
    }

    printf("  Completed %d allocations\n", successful_allocs);
    assert(successful_allocs > 0 && "Should complete at least some allocations");

    for (int i = 0; i < successful_allocs; i++)
    {
        unsigned char *mem = ptrs[i];
        for (int j = 0; j < 32; j++)
        {
            assert(mem[j] == i && "Memory corruption detected");
        }
    }

    pool_drain(pool);

    pool_destroy(pool);
    TEST_END("stress conditions");
}

void test_pool_reresize()
{
    TEST_START("pool reresize");

    size_t initial_size = 1024;
    MemPool *pool = pool_build(initial_size);
    assert(pool != NULL);

    size_t new_size = 8192;
    size_t old_size = pool->size;
    MemPool *expanded = pool_resize(pool, new_size);
    assert(expanded != NULL);
    assert(expanded->size > old_size);

    size_t smaller_size = 2048;
    old_size = expanded->size;
    MemPool *shrunk = pool_resize(expanded, smaller_size);
    assert(shrunk != NULL);
    assert(shrunk->size < old_size);

    MemPool *same = pool_resize(shrunk, shrunk->size);
    assert(same == shrunk);

    assert(pool_resize(NULL, 1024) == NULL);
    assert(pool_resize(shrunk, 0) == NULL);

    int *num = pool_fill(shrunk, sizeof(int));
    *num = 42;
    MemPool *after_data = pool_resize(shrunk, 4096);
    assert(*(int *)((char *)after_data->base + sizeof(MemPool)) == 42);

    pool_destroy(after_data);
    TEST_END("pool reresize");
}

void test_pool_copy()
{
    TEST_START("pool copy");

    MemPool *source = pool_build(1024);
    MemPool *dest = pool_build(1024);
    assert(source != NULL && dest != NULL);

    // Test overwrite mode
    int *num1 = pool_fill(source, sizeof(int));
    *num1 = 42;

    int *dest_num = pool_fill(dest, sizeof(int));
    *dest_num = 100;

    pool_copy(source, dest, POOL_COPY_OVERWRITE);
    assert(*(int *)((char *)dest->base + sizeof(MemPool)) == 42);

    // Test append mode
    MemPool *source2 = pool_build(1024);
    int *num2 = pool_fill(source2, sizeof(int));
    *num2 = 84;

    pool_copy(source2, dest, POOL_COPY_APPEND);
    int *second_num = (int *)((char *)dest->base + sizeof(MemPool) + sizeof(int) * 2);
    assert(*second_num == 84);

    pool_destroy(source);
    pool_destroy(source2);
    pool_destroy(dest);
    TEST_END("pool copy");
}

int main()
{
    printf("Running memory pool tests...\n\n");

    test_pool_creation();
    test_basic_allocation();

    test_pool_measure();
    test_pool_drain();
    test_edge_cases();
    test_stress();
    test_pool_reresize();
    test_pool_copy();

    printf(GREEN "\nAll tests passed successfully!\n" RESET);
    return 0;
}
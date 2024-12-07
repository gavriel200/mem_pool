# Memory Pool Library

A lightweight and efficient memory pool implementation in C for managing memory allocations. This library provides a simple way to handle memory allocations from pre-allocated memory blocks, reducing overhead and fragmentation compared to regular malloc/free operations.

## Features

- Fast memory allocation from pre-allocated pools
- Page-aligned memory management
- Simple and intuitive API
- Memory drain and reuse capabilities
- Efficient space measurement
- Built-in safety checks

## API Overview

```c
MemPool *pool_build(size_t size);        // Create a new memory pool
void *pool_fill(MemPool *pool, size_t size); // Allocate memory from pool
void pool_drain(MemPool *pool);          // Reset pool allocations
void pool_destroy(MemPool *pool);        // Destroy pool and free memory
size_t pool_measure(MemPool *pool);      // Get remaining pool space
```

## Usage Example

```c
// Create a new memory pool
MemPool *pool = pool_build(1024);

// Allocate memory from the pool
int *numbers = pool_fill(pool, sizeof(int) * 10);
char *string = pool_fill(pool, 100);

// Use the allocated memory
numbers[0] = 42;
strcpy(string, "Hello, World!");

// Reset all allocations
pool_drain(pool);

// Clean up
pool_destroy(pool);
```

## Building and Testing

The project includes a comprehensive test suite. To build and run the tests:

```bash
make clean && make
```

## Technical Details

- Uses POSIX mmap for memory allocation
- deafult 8-byte alignment for all allocations
- Page-size aligned pool creation
- Thread-safe for separate pools
- Zero-overhead for sequential allocations

## Requirements

- POSIX-compliant system
- C99 or later
- System with mmap support

# Memory Management System

## Overview

LKJAgent implements a sophisticated memory management system consisting of:
1. **Pool-based Allocator** - Pre-allocated memory pools for different object sizes
2. **String Management** - Dynamic string handling with multiple size tiers
3. **Object System** - Hierarchical object structures with automatic cleanup
4. **Freelist Management** - Efficient allocation/deallocation through linked lists

## Memory Pool Architecture

### Pool Design Philosophy

The memory pool system eliminates runtime malloc/free calls by pre-allocating fixed-size buffers:

```c
typedef struct {
    // String pools of different sizes
    char string16_data[POOL_STRING16_MAXCOUNT * 16];
    string_t string16[POOL_STRING16_MAXCOUNT];
    string_t* string16_freelist_data[POOL_STRING16_MAXCOUNT];
    uint64_t string16_freelist_count;
    
    // ... similar for 256B, 4KB, 64KB, 1MB pools
    
    // Object pool
    object_t object_data[POOL_OBJECT_MAXCOUNT];
    object_t* object_freelist_data[POOL_OBJECT_MAXCOUNT];
    uint64_t object_freelist_count;
} pool_t;
```

### Pool Specifications

| Pool Type | Max Count | Item Size | Total Size | Use Case |
|-----------|-----------|-----------|------------|----------|
| string16 | 1,048,576 | 16 bytes | 16 MB | Short identifiers, keys |
| string256 | 65,536 | 256 bytes | 16 MB | Medium text, values |
| string4096 | 4,096 | 4 KB | 16 MB | Large text blocks |
| string65536 | 256 | 64 KB | 16 MB | Very large content |
| string1048576 | 16 | 1 MB | 16 MB | Maximum size content |
| object | 65,536 | Variable | ~10 MB | JSON/XML structures |

**Total Pre-allocated Memory: ~90-100 MB**

## String Management

### String Structure

```c
typedef struct string_s {
    char* data;        // Pointer to character buffer
    uint64_t capacity; // Maximum size of buffer
    uint64_t size;     // Current used size
} string_t;
```

### String Allocation Strategy

1. **Size-based Selection**: Choose appropriate pool based on required capacity
2. **Automatic Promotion**: Upgrade to larger pool if content exceeds current capacity
3. **Pool Exhaustion Handling**: Graceful degradation when pools are full

### String Operations

```c
// Core string operations
result_t string_create(pool_t* pool, string_t** string);
result_t string_destroy(pool_t* pool, string_t* string);
result_t string_resize(pool_t* pool, string_t** string, uint64_t new_capacity);

// Content manipulation
result_t string_set_str(string_t* string, const char* src);
result_t string_append_str(pool_t* pool, string_t** string, const char* src);
result_t string_append_string(pool_t* pool, string_t** string, const string_t* src);
```

## Object System

### Object Structure

```c
typedef struct object_t {
    string_t* string;           // Value (for leaf nodes)
    struct object_t* child;     // First child (for containers)
    struct object_t* next;      // Next sibling
} object_t;
```

### Object Hierarchy

Objects form tree structures suitable for JSON/XML representation:

```
Object Tree Example:
root
├── "working_memory" (container)
│   ├── "todo" → "Add characters until 10"
│   └── "character_alice_about" → "female, 25 years old..."
├── "storage" (container)
│   └── "character_charlie_about" → "male, 28 years old..."
└── "state" → "thinking"
```

### Object Path Navigation

The system supports dot-notation path access:

```c
// Access nested objects using path notation
result_t object_provide_str(pool_t* pool, object_t** dst, 
                           const object_t* object, const char* path);

// Examples:
// "working_memory.todo" 
// "agent.state.base.prompt.role"
// "llm.endpoint"
```

## Memory Lifecycle

### Allocation Process

1. **Request Analysis**: Determine required object type and size
2. **Pool Selection**: Choose appropriate pool based on requirements
3. **Freelist Check**: Attempt allocation from freelist
4. **Pool Allocation**: If freelist empty, allocate from pool
5. **Initialization**: Set up object structure and relationships

### Deallocation Process

1. **Cleanup Validation**: Ensure object is valid for cleanup
2. **Child Cleanup**: Recursively clean up child objects
3. **Freelist Return**: Return object to appropriate freelist
4. **Reference Clearing**: Clear all pointers to deallocated object

### Resource Management Pattern

```c
// Typical resource management pattern
string_t* work_string = NULL;
object_t* work_object = NULL;

// Allocation
if (string_create(pool, &work_string) != RESULT_OK) {
    RETURN_ERR("Failed to create string");
}

if (object_create(pool, &work_object) != RESULT_OK) {
    if (string_destroy(pool, work_string) != RESULT_OK) {
        RETURN_ERR("Failed to cleanup string after object failure");
    }
    RETURN_ERR("Failed to create object");
}

// Use resources...

// Cleanup (in reverse order)
if (object_destroy(pool, work_object) != RESULT_OK) {
    RETURN_ERR("Failed to destroy object");
}

if (string_destroy(pool, work_string) != RESULT_OK) {
    RETURN_ERR("Failed to destroy string");
}
```

## Freelist Management

### Freelist Structure

Each pool maintains a freelist of available objects:

```c
string_t* string256_freelist_data[POOL_STRING256_MAXCOUNT];
uint64_t string256_freelist_count;
```

### Allocation Algorithm

```c
result_t pool_string256_alloc(pool_t* pool, string_t** string) {
    if (pool->string256_freelist_count > 0) {
        // Use object from freelist
        pool->string256_freelist_count--;
        *string = pool->string256_freelist_data[pool->string256_freelist_count];
        return RESULT_OK;
    }
    
    // Pool exhausted
    RETURN_ERR("String256 pool exhausted");
}
```

### Deallocation Algorithm

```c
result_t pool_string256_free(pool_t* pool, string_t* string) {
    if (pool->string256_freelist_count >= POOL_STRING256_MAXCOUNT) {
        RETURN_ERR("String256 freelist full");
    }
    
    // Return to freelist
    pool->string256_freelist_data[pool->string256_freelist_count] = string;
    pool->string256_freelist_count++;
    return RESULT_OK;
}
```

## Memory Pool Initialization

### Pool Setup Process

```c
result_t pool_init(pool_t* pool) {
    // Initialize all pools to zero
    memset(pool, 0, sizeof(pool_t));
    
    // Set up string16 pool
    for (uint64_t i = 0; i < POOL_STRING16_MAXCOUNT; i++) {
        pool->string16[i].data = &pool->string16_data[i * 16];
        pool->string16[i].capacity = 16;
        pool->string16[i].size = 0;
        pool->string16_freelist_data[i] = &pool->string16[i];
    }
    pool->string16_freelist_count = POOL_STRING16_MAXCOUNT;
    
    // ... similar setup for other pools
    
    return RESULT_OK;
}
```

## Performance Characteristics

### Time Complexity
- **Allocation**: O(1) - constant time freelist operations
- **Deallocation**: O(1) - constant time freelist return
- **Object Traversal**: O(n) - linear in object tree depth
- **Path Resolution**: O(m) - linear in path length

### Space Complexity
- **Fixed Overhead**: ~90MB pre-allocated pools
- **Dynamic Growth**: Zero - no runtime allocation
- **Memory Utilization**: Depends on object distribution across pools

### Memory Access Patterns
- **Locality**: High - objects allocated from contiguous pools
- **Fragmentation**: Zero - fixed-size allocation
- **Cache Performance**: Excellent - predictable access patterns

## Error Handling and Diagnostics

### Pool Exhaustion Detection

```c
// Example pool exhaustion check
if (pool->string4096_freelist_count == 0) {
    // Log warning or trigger paging
    printf("Warning: string4096 pool exhausted\n");
    RETURN_ERR("Pool exhausted");
}
```

### Memory Leak Detection

The system provides comprehensive memory usage statistics:

```c
printf("string16 freelist: %lu\n", pool->string16_freelist_count);
printf("string256 freelist: %lu\n", pool->string256_freelist_count);
printf("object freelist: %lu\n", pool->object_freelist_count);
```

Expected values at shutdown:
- **All freelists should equal their MAXCOUNT values**
- **Deviations indicate memory leaks**
- **Zero freelist count indicates pool exhaustion**

## Memory Safety Features

### Bounds Checking
- **Buffer Overflow Protection**: All string operations check capacity
- **Pool Boundary Validation**: Prevent access outside allocated pools
- **Null Pointer Validation**: Check all pointer arguments

### Resource Cleanup
- **Automatic Cleanup**: Failed operations clean up partial allocations
- **Exception Safety**: Cleanup paths handle all error conditions
- **Reference Tracking**: Prevent use-after-free through careful lifecycle management

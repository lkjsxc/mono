# Coding Guidelines

## Memory Safety Rules

### 1. No Dynamic Allocation
```c
// ❌ NEVER do this
char* buffer = malloc(size);

// ✅ ALWAYS do this
static char buffer[MAX_SIZE];
data_t token;
data_init(&token, buffer, sizeof(buffer));
```

### 2. Buffer Bounds Checking
```c
// ✅ Always check bounds before operations
__attribute__((warn_unused_result))
result_t safe_copy(data_t* dest, const char* src) {
    if (!dest || !src) {
        RETURN_ERR("Null parameters");
        return RESULT_ERR;
    }
    
    size_t len = strlen(src);
    if (len >= dest->capacity) {
        RETURN_ERR("Buffer overflow");
        return RESULT_ERR;
    }
    
    strcpy(dest->data, src);
    dest->size = len;
    return RESULT_OK;
}
```

## Error Handling Patterns

### 1. Function Signatures
```c
// ✅ All fallible functions must return result_t
__attribute__((warn_unused_result))
result_t function_name(params);

// ✅ Use warn_unused_result attribute
__attribute__((warn_unused_result))
result_t process_data(data_t* input, data_t* output);
```

### 2. Parameter Validation
```c
__attribute__((warn_unused_result))
result_t example_function(data_t* data, const char* input) {
    // ✅ Always validate parameters first
    if (!data) {
        RETURN_ERR("Data parameter is null");
        return RESULT_ERR;
    }
    
    if (!input) {
        RETURN_ERR("Input parameter is null");
        return RESULT_ERR;
    }
    
    if (!data->data) {
        RETURN_ERR("Data buffer is null");
        return RESULT_ERR;
    }
    
    // Function implementation...
    return RESULT_OK;
}
```

### 3. Early Return Pattern
```c
__attribute__((warn_unused_result))
result_t process_request(request_t* req) {
    if (!req) {
        RETURN_ERR("Request is null");
        return RESULT_ERR;
    }
    
    if (validate_request(req) != RESULT_OK) {
        RETURN_ERR("Request validation failed");
        return RESULT_ERR;
    }
    
    if (process_headers(req) != RESULT_OK) {
        RETURN_ERR("Header processing failed");
        return RESULT_ERR;
    }
    
    // Success path
    return RESULT_OK;
}
```

## String Handling

### 1. Use Data Tokens
```c
// ✅ Correct way to handle strings
static char buffer[512];
data_t message;

if (data_init(&message, buffer, sizeof(buffer)) != RESULT_OK) {
    RETURN_ERR("Failed to initialize data token");
    return RESULT_ERR;
}

if (data_set(&message, "Hello, World!") != RESULT_OK) {
    RETURN_ERR("Failed to set message");
    return RESULT_ERR;
}
```

### 2. String Operations
```c
// ✅ Always check return values
if (data_append(&message, " Additional text") != RESULT_OK) {
    RETURN_ERR("Failed to append text");
    return RESULT_ERR;
}

// ✅ Use safe comparison
if (data_compare(&token1, &token2) == 0) {
    // Strings are equal
}
```

## File Operations

### 1. Atomic File Writing
```c
__attribute__((warn_unused_result))
result_t save_config(const config_t* config) {
    static char temp_path[MAX_PATH_LENGTH];
    static char final_path[MAX_PATH_LENGTH];
    
    // Write to temporary file first
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", CONFIG_PATH);
    snprintf(final_path, sizeof(final_path), "%s", CONFIG_PATH);
    
    if (write_to_file(temp_path, config) != RESULT_OK) {
        RETURN_ERR("Failed to write temporary file");
        return RESULT_ERR;
    }
    
    // Atomic rename
    if (rename(temp_path, final_path) != 0) {
        RETURN_ERR("Failed to rename file");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}
```

## LLM Communication

### 1. Simple Tag Format Only
```c
// ✅ Correct tag format
const char* valid_response = 
    "<thinking>\n"
    "<analysis>Current situation analysis</analysis>\n"
    "</thinking>\n";

// ❌ NEVER use complex markup
const char* invalid_response = 
    "<thinking type=\"analysis\" xmlns=\"...\">\n"
    "  <analysis id=\"1\">...</analysis>\n"
    "</thinking>\n";
```

### 2. Tag Parsing
```c
__attribute__((warn_unused_result))
result_t extract_tag_content(const char* input, const char* tag_name, data_t* output) {
    if (!input || !tag_name || !output) {
        RETURN_ERR("Null parameters");
        return RESULT_ERR;
    }
    
    static char start_tag[64];
    static char end_tag[64];
    
    snprintf(start_tag, sizeof(start_tag), "<%s>", tag_name);
    snprintf(end_tag, sizeof(end_tag), "</%s>", tag_name);
    
    const char* start = strstr(input, start_tag);
    if (!start) {
        RETURN_ERR("Start tag not found");
        return RESULT_ERR;
    }
    
    start += strlen(start_tag);
    const char* end = strstr(start, end_tag);
    if (!end) {
        RETURN_ERR("End tag not found");
        return RESULT_ERR;
    }
    
    size_t content_len = end - start;
    if (content_len >= output->capacity) {
        RETURN_ERR("Content too long");
        return RESULT_ERR;
    }
    
    strncpy(output->data, start, content_len);
    output->data[content_len] = '\0';
    output->size = content_len;
    
    return RESULT_OK;
}
```

## Common Patterns

### 1. Function Template
```c
/**
 * @file module_name.c
 */

/**
 * @brief Brief description of what the function does
 * @param param1 Description of parameter 1
 * @param param2 Description of parameter 2
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t module_function_name(type1 param1, type2 param2) {
    // Parameter validation
    if (!param1) {
        RETURN_ERR("Parameter 1 is null");
        return RESULT_ERR;
    }
    
    if (!param2) {
        RETURN_ERR("Parameter 2 is null");
        return RESULT_ERR;
    }
    
    // Function implementation
    // ...
    
    return RESULT_OK;
}
```

### 2. Buffer Initialization
```c
// ✅ Standard pattern for buffer setup
static char buffer[MAX_BUFFER_SIZE];
data_t token;

if (data_init(&token, buffer, sizeof(buffer)) != RESULT_OK) {
    RETURN_ERR("Failed to initialize data token");
    return RESULT_ERR;
}
```

### 3. Loop with Error Checking
```c
for (size_t i = 0; i < count; i++) {
    if (process_item(items[i]) != RESULT_OK) {
        RETURN_ERR("Failed to process item");
        return RESULT_ERR;
    }
}
```

/**
 * @file tag_parser.c
 * @brief Simple tag format parser implementation for LKJAgent
 * 
 * This module provides robust parsing of simple XML-like tags from LLM
 * responses with comprehensive error handling for malformed input.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/tag_parser.h"
#include "../lkjagent.h"
#include <string.h>
#include <ctype.h>

/**
 * @defgroup Tag_Parser_Internal Internal Tag Parser Functions
 * @{
 */

/**
 * @brief Trim leading and trailing whitespace from a string
 * 
 * @param input Input string to trim
 * @param output Output buffer for trimmed string
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t trim_whitespace(const char* input, data_t* output) {
    if (!input || !output) {
        RETURN_ERR("Null pointer in trim_whitespace");
        return RESULT_ERR;
    }
    
    /* Find start of non-whitespace */
    const char* start = input;
    while (isspace(*start)) {
        start++;
    }
    
    /* Find end of non-whitespace */
    const char* end = start + strlen(start);
    while (end > start && isspace(*(end - 1))) {
        end--;
    }
    
    /* Calculate trimmed length */
    size_t trimmed_len = end - start;
    
    /* Set the trimmed content */
    if (data_clear(output) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (trimmed_len > 0) {
        /* Create temporary null-terminated string */
        char* temp = malloc(trimmed_len + 1);
        if (!temp) {
            RETURN_ERR("Memory allocation failed in trim_whitespace");
            return RESULT_ERR;
        }
        
        memcpy(temp, start, trimmed_len);
        temp[trimmed_len] = '\0';
        
        result_t result = data_set(output, temp, 0);
        free(temp);
        
        if (result != RESULT_OK) {
            return RESULT_ERR;
        }
    }
    
    return RESULT_OK;
}

/**
 * @brief Find the next occurrence of a tag in text
 * 
 * @param text Text to search
 * @param tag Tag to find (without angle brackets)
 * @param start_pos Position to start searching from
 * @param is_closing Whether to search for closing tag
 * @return Pointer to tag start, or NULL if not found
 */
static const char* find_tag(const char* text, const char* tag, size_t start_pos, bool is_closing) {
    if (!text || !tag) {
        return NULL;
    }
    
    size_t text_len = strlen(text);
    size_t tag_len = strlen(tag);
    
    if (start_pos >= text_len || tag_len == 0) {
        return NULL;
    }
    
    /* Build the tag string to search for */
    char search_tag[MAX_TAG_SIZE + 4]; /* +4 for "<", "/", ">", and null terminator */
    if (is_closing) {
        snprintf(search_tag, sizeof(search_tag), "</%s>", tag);
    } else {
        snprintf(search_tag, sizeof(search_tag), "<%s>", tag);
    }
    
    const char* found = strstr(text + start_pos, search_tag);
    return found;
}

/**
 * @brief Validate tag name format
 * 
 * @param tag_name Tag name to validate
 * @return RESULT_OK if valid, RESULT_ERR if invalid
 */
static result_t validate_tag_name(const char* tag_name) {
    if (!tag_name || tag_name[0] == '\0') {
        RETURN_ERR("Tag name is null or empty");
        return RESULT_ERR;
    }
    
    size_t len = strlen(tag_name);
    if (len >= MAX_TAG_SIZE) {
        RETURN_ERR("Tag name exceeds maximum length");
        return RESULT_ERR;
    }
    
    /* Validate tag name characters */
    for (size_t i = 0; i < len; i++) {
        char c = tag_name[i];
        if (!isalnum(c) && c != '_' && c != '-') {
            RETURN_ERR("Tag name contains invalid characters");
            return RESULT_ERR;
        }
    }
    
    /* Tag name must start with a letter */
    if (!isalpha(tag_name[0])) {
        RETURN_ERR("Tag name must start with a letter");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/** @} */

result_t tag_parse_simple(const char* input, const char* tag_name, data_t* output, bool allow_nested) {
    if (!input) {
        RETURN_ERR("Null input pointer in tag_parse_simple");
        return RESULT_ERR;
    }
    
    if (!tag_name) {
        RETURN_ERR("Null tag_name pointer in tag_parse_simple");
        return RESULT_ERR;
    }
    
    if (!output) {
        RETURN_ERR("Null output pointer in tag_parse_simple");
        return RESULT_ERR;
    }
    
    if (validate_tag_name(tag_name) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Clear output buffer */
    if (data_clear(output) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Find opening tag */
    const char* open_tag = find_tag(input, tag_name, 0, false);
    if (!open_tag) {
        RETURN_ERR("Opening tag not found");
        return RESULT_ERR;
    }
    
    /* Calculate start of content */
    size_t tag_len = strlen(tag_name);
    const char* content_start = open_tag + tag_len + 2; /* +2 for '<' and '>' */
    
    /* Find matching closing tag */
    const char* close_tag;
    if (allow_nested) {
        /* Count nested tags to find the correct closing tag */
        int nesting_level = 1;
        const char* search_pos = content_start;
        
        while (nesting_level > 0) {
            const char* next_open = find_tag(search_pos, tag_name, 0, false);
            const char* next_close = find_tag(search_pos, tag_name, 0, true);
            
            if (!next_close) {
                RETURN_ERR("Unmatched opening tag");
                return RESULT_ERR;
            }
            
            if (next_open && next_open < next_close) {
                /* Found nested opening tag */
                nesting_level++;
                search_pos = next_open + tag_len + 2;
            } else {
                /* Found closing tag */
                nesting_level--;
                if (nesting_level > 0) {
                    search_pos = next_close + tag_len + 3; /* +3 for '<', '/', '>' */
                } else {
                    close_tag = next_close;
                }
            }
        }
    } else {
        /* Simple case: find first closing tag */
        close_tag = find_tag(content_start, tag_name, 0, true);
        if (!close_tag) {
            RETURN_ERR("Matching closing tag not found");
            return RESULT_ERR;
        }
    }
    
    /* Extract content between tags */
    size_t content_len = close_tag - content_start;
    if (content_len == 0) {
        /* Empty tag content is valid */
        return RESULT_OK;
    }
    
    /* Create temporary null-terminated string for content */
    char* temp_content = malloc(content_len + 1);
    if (!temp_content) {
        RETURN_ERR("Memory allocation failed for tag content");
        return RESULT_ERR;
    }
    
    memcpy(temp_content, content_start, content_len);
    temp_content[content_len] = '\0';
    
    /* Set the extracted content */
    result_t result = data_set(output, temp_content, 0);
    free(temp_content);
    
    return result;
}

result_t tag_extract_content(const char* input, const char* opening_tag, const char* closing_tag, data_t* output, size_t start_offset) {
    if (!input) {
        RETURN_ERR("Null input pointer in tag_extract_content");
        return RESULT_ERR;
    }
    
    if (!opening_tag || !closing_tag) {
        RETURN_ERR("Null tag pointer in tag_extract_content");
        return RESULT_ERR;
    }
    
    if (!output) {
        RETURN_ERR("Null output pointer in tag_extract_content");
        return RESULT_ERR;
    }
    
    size_t input_len = strlen(input);
    if (start_offset >= input_len) {
        RETURN_ERR("Start offset exceeds input length");
        return RESULT_ERR;
    }
    
    /* Clear output buffer */
    if (data_clear(output) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Find opening tag */
    const char* open_pos = strstr(input + start_offset, opening_tag);
    if (!open_pos) {
        RETURN_ERR("Opening tag not found");
        return RESULT_ERR;
    }
    
    /* Calculate start of content */
    const char* content_start = open_pos + strlen(opening_tag);
    
    /* Find closing tag */
    const char* close_pos = strstr(content_start, closing_tag);
    if (!close_pos) {
        RETURN_ERR("Closing tag not found");
        return RESULT_ERR;
    }
    
    /* Extract content between tags */
    size_t content_len = close_pos - content_start;
    if (content_len == 0) {
        /* Empty content is valid */
        return RESULT_OK;
    }
    
    /* Create temporary null-terminated string for content */
    char* temp_content = malloc(content_len + 1);
    if (!temp_content) {
        RETURN_ERR("Memory allocation failed for tag content extraction");
        return RESULT_ERR;
    }
    
    memcpy(temp_content, content_start, content_len);
    temp_content[content_len] = '\0';
    
    /* Set the extracted content */
    result_t result = data_set(output, temp_content, 0);
    free(temp_content);
    
    return result;
}

result_t tag_parse_thinking(const char* llm_response, data_t* thinking_content) {
    if (!llm_response) {
        RETURN_ERR("Null LLM response in tag_parse_thinking");
        return RESULT_ERR;
    }
    
    if (!thinking_content) {
        RETURN_ERR("Null thinking_content pointer in tag_parse_thinking");
        return RESULT_ERR;
    }
    
    return tag_parse_simple(llm_response, "thinking", thinking_content, false);
}

result_t tag_parse_action(const char* llm_response, data_t* action_content) {
    if (!llm_response) {
        RETURN_ERR("Null LLM response in tag_parse_action");
        return RESULT_ERR;
    }
    
    if (!action_content) {
        RETURN_ERR("Null action_content pointer in tag_parse_action");
        return RESULT_ERR;
    }
    
    /* Parse the action tag and trim whitespace */
    data_t raw_action;
    if (data_init(&raw_action, 512) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    result_t parse_result = tag_parse_simple(llm_response, "action", &raw_action, false);
    if (parse_result != RESULT_OK) {
        data_destroy(&raw_action);
        return parse_result;
    }
    
    /* Trim whitespace from action content */
    result_t trim_result = trim_whitespace(raw_action.data, action_content);
    data_destroy(&raw_action);
    
    return trim_result;
}

result_t tag_parse_evaluation(const char* llm_response, data_t* evaluation_content) {
    if (!llm_response) {
        RETURN_ERR("Null LLM response in tag_parse_evaluation");
        return RESULT_ERR;
    }
    
    if (!evaluation_content) {
        RETURN_ERR("Null evaluation_content pointer in tag_parse_evaluation");
        return RESULT_ERR;
    }
    
    return tag_parse_simple(llm_response, "evaluation", evaluation_content, false);
}

result_t tag_parse_paging(const char* llm_response, data_t* paging_content) {
    if (!llm_response) {
        RETURN_ERR("Null LLM response in tag_parse_paging");
        return RESULT_ERR;
    }
    
    if (!paging_content) {
        RETURN_ERR("Null paging_content pointer in tag_parse_paging");
        return RESULT_ERR;
    }
    
    return tag_parse_simple(llm_response, "paging", paging_content, false);
}

result_t tag_parse_context_keys(const char* paging_content, context_key_t* context_keys, size_t max_keys, size_t* parsed_count) {
    if (!paging_content) {
        RETURN_ERR("Null paging_content in tag_parse_context_keys");
        return RESULT_ERR;
    }
    
    if (!context_keys) {
        RETURN_ERR("Null context_keys array in tag_parse_context_keys");
        return RESULT_ERR;
    }
    
    if (!parsed_count) {
        RETURN_ERR("Null parsed_count pointer in tag_parse_context_keys");
        return RESULT_ERR;
    }
    
    if (max_keys == 0) {
        RETURN_ERR("max_keys must be greater than zero");
        return RESULT_ERR;
    }
    
    *parsed_count = 0;
    
    /* Parse different types of context operations */
    const char* operations[] = {"load_context", "archive_context", "prioritize_context"};
    memory_layer_t layers[] = {LAYER_WORKING, LAYER_ARCHIVED, LAYER_WORKING};
    size_t importance_scores[] = {75, 25, 90};
    
    for (size_t op_idx = 0; op_idx < 3 && *parsed_count < max_keys; op_idx++) {
        const char* operation = operations[op_idx];
        size_t op_len = strlen(operation);
        
        /* Find operation in paging content */
        const char* op_pos = strstr(paging_content, operation);
        while (op_pos && *parsed_count < max_keys) {
            /* Skip to the colon after operation name */
            const char* colon_pos = strchr(op_pos + op_len, ':');
            if (!colon_pos) {
                break;
            }
            
            /* Find the end of the key list (semicolon or end of string) */
            const char* keys_start = colon_pos + 1;
            const char* keys_end = strchr(keys_start, ';');
            if (!keys_end) {
                keys_end = keys_start + strlen(keys_start);
            }
            
            /* Extract and parse individual keys */
            const char* key_start = keys_start;
            while (key_start < keys_end && *parsed_count < max_keys) {
                /* Skip whitespace */
                while (key_start < keys_end && isspace(*key_start)) {
                    key_start++;
                }
                
                if (key_start >= keys_end) {
                    break;
                }
                
                /* Find end of key (comma or end of list) */
                const char* key_end = key_start;
                while (key_end < keys_end && *key_end != ',' && !isspace(*key_end)) {
                    key_end++;
                }
                
                /* Extract key name */
                size_t key_len = key_end - key_start;
                if (key_len > 0 && key_len < sizeof(context_keys[0].key)) {
                    context_key_t* key = &context_keys[*parsed_count];
                    
                    /* Copy key name */
                    memcpy(key->key, key_start, key_len);
                    key->key[key_len] = '\0';
                    
                    /* Set key properties */
                    key->layer = layers[op_idx];
                    key->importance_score = importance_scores[op_idx];
                    key->last_accessed = time(NULL);
                    key->data_size = 0; /* Will be filled by memory system */
                    
                    (*parsed_count)++;
                }
                
                /* Move to next key */
                key_start = key_end;
                if (key_start < keys_end && *key_start == ',') {
                    key_start++;
                }
            }
            
            /* Find next occurrence of this operation */
            op_pos = strstr(op_pos + op_len, operation);
        }
    }
    
    return RESULT_OK;
}

result_t tag_validate_format(const char* input, const char* tag_name) {
    if (!input) {
        RETURN_ERR("Null input pointer in tag_validate_format");
        return RESULT_ERR;
    }
    
    if (tag_name) {
        /* Validate specific tag */
        if (validate_tag_name(tag_name) != RESULT_OK) {
            return RESULT_ERR;
        }
        
        /* Check if tag exists and is properly formed */
        const char* open_tag = find_tag(input, tag_name, 0, false);
        if (!open_tag) {
            RETURN_ERR("Specified tag not found in input");
            return RESULT_ERR;
        }
        
        const char* close_tag = find_tag(input, tag_name, 0, true);
        if (!close_tag) {
            RETURN_ERR("Closing tag not found for specified tag");
            return RESULT_ERR;
        }
        
        /* Verify order (opening tag comes before closing tag) */
        if (close_tag <= open_tag) {
            RETURN_ERR("Closing tag appears before opening tag");
            return RESULT_ERR;
        }
    } else {
        /* Validate all tags in the input */
        /* This is a simplified validation - in a full implementation,
         * this would parse all tags and verify proper nesting */
        
        const char* pos = input;
        int open_brackets = 0;
        
        while (*pos) {
            if (*pos == '<') {
                open_brackets++;
                /* Check for proper tag format */
                const char* close_bracket = strchr(pos, '>');
                if (!close_bracket) {
                    RETURN_ERR("Unclosed angle bracket found");
                    return RESULT_ERR;
                }
                pos = close_bracket;
                open_brackets--;
            }
            pos++;
        }
        
        if (open_brackets != 0) {
            RETURN_ERR("Mismatched angle brackets");
            return RESULT_ERR;
        }
    }
    
    return RESULT_OK;
}

result_t tag_parse_paging_directives(const char* paging_content, void* commands, size_t max_commands, size_t* parsed_count) {
    if (!paging_content) {
        RETURN_ERR("Null paging_content in tag_parse_paging_directives");
        return RESULT_ERR;
    }
    
    if (!commands) {
        RETURN_ERR("Null commands pointer in tag_parse_paging_directives");
        return RESULT_ERR;
    }
    
    if (!parsed_count) {
        RETURN_ERR("Null parsed_count pointer in tag_parse_paging_directives");
        return RESULT_ERR;
    }
    
    if (max_commands == 0) {
        RETURN_ERR("max_commands must be greater than zero");
        return RESULT_ERR;
    }
    
    /* This function is a placeholder for future implementation of
     * structured command parsing. The actual implementation would
     * depend on the specific command structure definition. */
    
    *parsed_count = 0;
    
    /* For now, we'll just parse the basic operations we can handle */
    /* This would be expanded based on the actual command structure needed */
    
    return RESULT_OK;
}

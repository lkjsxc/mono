/**
 * @file llm_parser.c
 * @brief LLM response parser implementation for simple tag format processing
 * 
 * This module implements comprehensive parsing capabilities for LLM responses
 * using the simple tag format. It handles extraction of thinking blocks,
 * action blocks, context keys, and paging directives with robust error handling.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/llm/llm_parser.h"
#include "../lkjagent.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/**
 * @defgroup LLM_Parser_Internal Internal LLM Parser Functions
 * @{
 */

/**
 * @brief Extract content between tags
 * 
 * @param text Text to search in
 * @param tag_name Name of tag to extract
 * @param content Buffer to store extracted content
 * @param trim_whitespace Whether to trim leading/trailing whitespace
 * @return RESULT_OK on success, RESULT_ERR if tag not found
 */
static result_t extract_tag_content(const char* text, const char* tag_name, data_t* content, bool trim_whitespace) {
    if (!text || !tag_name || !content) {
        RETURN_ERR("Invalid parameters for tag content extraction");
        return RESULT_ERR;
    }
    
    /* Build opening and closing tags */
    char opening_tag[128];
    char closing_tag[128];
    snprintf(opening_tag, sizeof(opening_tag), "<%s>", tag_name);
    snprintf(closing_tag, sizeof(closing_tag), "</%s>", tag_name);
    
    /* Clear content buffer */
    if (data_set(content, "", 0) != RESULT_OK) {
        RETURN_ERR("Failed to clear content buffer");
        return RESULT_ERR;
    }
    
    /* Find all instances of the tag */
    const char* search_pos = text;
    bool found_any = false;
    
    while (true) {
        /* Find opening tag */
        const char* tag_start = strcasestr(search_pos, opening_tag);
        if (!tag_start) {
            break;
        }
        
        /* Find content start */
        const char* content_start = tag_start + strlen(opening_tag);
        
        /* Find closing tag */
        const char* tag_end = strcasestr(content_start, closing_tag);
        if (!tag_end) {
            /* Malformed tag - continue searching */
            search_pos = tag_start + strlen(opening_tag);
            continue;
        }
        
        /* Extract content */
        size_t content_len = tag_end - content_start;
        if (content_len > 0) {
            char* temp_content = malloc(content_len + 1);
            if (!temp_content) {
                RETURN_ERR("Failed to allocate memory for tag content");
                return RESULT_ERR;
            }
            
            strncpy(temp_content, content_start, content_len);
            temp_content[content_len] = '\0';
            
            /* Trim whitespace if requested */
            char* final_content = temp_content;
            if (trim_whitespace) {
                /* Trim leading whitespace */
                while (*final_content && isspace(*final_content)) {
                    final_content++;
                }
                
                /* Trim trailing whitespace */
                char* end = final_content + strlen(final_content) - 1;
                while (end > final_content && isspace(*end)) {
                    *end = '\0';
                    end--;
                }
            }
            
            /* Append to content buffer */
            if (found_any) {
                if (data_append(content, "\n\n", 0) != RESULT_OK) {
                    free(temp_content);
                    RETURN_ERR("Failed to append content separator");
                    return RESULT_ERR;
                }
            }
            
            if (data_append(content, final_content, 0) != RESULT_OK) {
                free(temp_content);
                RETURN_ERR("Failed to append extracted content");
                return RESULT_ERR;
            }
            
            free(temp_content);
            found_any = true;
        }
        
        /* Continue searching after this tag */
        search_pos = tag_end + strlen(closing_tag);
    }
    
    return found_any ? RESULT_OK : RESULT_ERR;
}

/**
 * @brief Extract context keys using pattern matching
 * 
 * @param text Text to search in
 * @param patterns Array of patterns to match
 * @param pattern_count Number of patterns
 * @param keys Array to store found keys
 * @param max_keys Maximum number of keys
 * @param keys_found Pointer to store number of keys found
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t extract_keys_by_patterns(const char* text, const char* patterns[], size_t pattern_count,
                                        char keys[][64], size_t max_keys, size_t* keys_found) {
    if (!text || !patterns || !keys || !keys_found) {
        RETURN_ERR("Invalid parameters for pattern-based key extraction");
        return RESULT_ERR;
    }
    
    *keys_found = 0;
    
    for (size_t p = 0; p < pattern_count && *keys_found < max_keys; p++) {
        const char* pattern = patterns[p];
        const char* search_pos = text;
        
        while (*keys_found < max_keys) {
            const char* match = strstr(search_pos, pattern);
            if (!match) {
                break;
            }
            
            /* Extract key after pattern */
            const char* key_start = match + strlen(pattern);
            
            /* Skip whitespace */
            while (*key_start && isspace(*key_start)) {
                key_start++;
            }
            
            /* Find key end */
            const char* key_end = key_start;
            while (*key_end && (isalnum(*key_end) || *key_end == '_' || *key_end == '-')) {
                key_end++;
            }
            
            /* Extract key */
            size_t key_len = key_end - key_start;
            if (key_len > 0 && key_len < 63) {
                strncpy(keys[*keys_found], key_start, key_len);
                keys[*keys_found][key_len] = '\0';
                
                /* Check for duplicates */
                bool duplicate = false;
                for (size_t i = 0; i < *keys_found; i++) {
                    if (strcmp(keys[i], keys[*keys_found]) == 0) {
                        duplicate = true;
                        break;
                    }
                }
                
                if (!duplicate) {
                    (*keys_found)++;
                }
            }
            
            search_pos = key_end;
        }
    }
    
    return RESULT_OK;
}

/**
 * @brief Validate tag structure
 * 
 * @param text Text to validate
 * @param tag_name Tag to validate
 * @param errors Buffer to store error messages
 * @return RESULT_OK if valid, RESULT_ERR if errors found
 */
static result_t validate_tag_structure(const char* text, const char* tag_name, data_t* errors) {
    if (!text || !tag_name || !errors) {
        return RESULT_ERR;
    }
    
    char opening_tag[128];
    char closing_tag[128];
    snprintf(opening_tag, sizeof(opening_tag), "<%s>", tag_name);
    snprintf(closing_tag, sizeof(closing_tag), "</%s>", tag_name);
    
    const char* search_pos = text;
    size_t open_count = 0;
    size_t close_count = 0;
    bool has_errors = false;
    
    while (true) {
        const char* next_open = strcasestr(search_pos, opening_tag);
        const char* next_close = strcasestr(search_pos, closing_tag);
        
        if (!next_open && !next_close) {
            break;
        }
        
        if (next_open && (!next_close || next_open < next_close)) {
            /* Found opening tag */
            open_count++;
            search_pos = next_open + strlen(opening_tag);
        } else if (next_close) {
            /* Found closing tag */
            close_count++;
            
            if (close_count > open_count) {
                /* Unmatched closing tag */
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "Unmatched closing tag </%s> found; ", tag_name);
                data_append(errors, error_msg, 0);
                has_errors = true;
            }
            
            search_pos = next_close + strlen(closing_tag);
        }
    }
    
    if (open_count != close_count) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Mismatched %s tags: %zu open, %zu close; ", 
                tag_name, open_count, close_count);
        data_append(errors, error_msg, 0);
        has_errors = true;
    }
    
    return has_errors ? RESULT_ERR : RESULT_OK;
}

/**
 * @brief Calculate content quality metrics
 * 
 * @param content Content to analyze
 * @param metrics Pointer to store quality metrics
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t calculate_content_quality(const char* content, size_t* metrics) {
    if (!content || !metrics) {
        return RESULT_ERR;
    }
    
    size_t length = strlen(content);
    size_t word_count = 0;
    size_t sentence_count = 0;
    size_t structure_score = 0;
    
    /* Count words and sentences */
    bool in_word = false;
    for (size_t i = 0; i < length; i++) {
        char c = content[i];
        
        if (isalnum(c)) {
            if (!in_word) {
                word_count++;
                in_word = true;
            }
        } else {
            in_word = false;
            if (c == '.' || c == '!' || c == '?') {
                sentence_count++;
            }
        }
    }
    
    /* Basic quality scoring */
    if (length > 0) structure_score += 20;
    if (word_count > 5) structure_score += 20;
    if (sentence_count > 0) structure_score += 20;
    if (length > 50) structure_score += 20;
    if (word_count > 20) structure_score += 20;
    
    *metrics = MIN(structure_score, 100);
    return RESULT_OK;
}

/** @} */

result_t llm_parse_response(const char* llm_response, llm_parsed_response_t* parsed_response) {
    if (!llm_response || !parsed_response) {
        RETURN_ERR("Invalid parameters for LLM response parsing");
        return RESULT_ERR;
    }
    
    /* Clear previous parse errors */
    if (data_set(&parsed_response->parse_errors, "", 0) != RESULT_OK) {
        RETURN_ERR("Failed to clear parse errors buffer");
        return RESULT_ERR;
    }
    
    /* Reset counters */
    parsed_response->context_key_count = 0;
    parsed_response->directive_count = 0;
    parsed_response->quality_score = 0;
    
    /* Parse each block type */
    bool has_thinking = (extract_tag_content(llm_response, "thinking", &parsed_response->thinking, true) == RESULT_OK);
    bool has_action = (extract_tag_content(llm_response, "action", &parsed_response->action, true) == RESULT_OK);
    bool has_evaluation = (extract_tag_content(llm_response, "evaluation", &parsed_response->evaluation, true) == RESULT_OK);
    bool has_paging = (extract_tag_content(llm_response, "paging", &parsed_response->paging, true) == RESULT_OK);
    
    /* Validate tag structures */
    if (validate_tag_structure(llm_response, "thinking", &parsed_response->parse_errors) != RESULT_OK ||
        validate_tag_structure(llm_response, "action", &parsed_response->parse_errors) != RESULT_OK ||
        validate_tag_structure(llm_response, "evaluation", &parsed_response->parse_errors) != RESULT_OK ||
        validate_tag_structure(llm_response, "paging", &parsed_response->parse_errors) != RESULT_OK) {
        /* Continue processing despite validation errors */
    }
    
    /* Extract context keys from all sources */
    const char* key_patterns[] = {
        "key:",
        "context:",
        "reference:",
        "data:",
        "item:",
        "@",
        "#"
    };
    
    size_t keys_extracted = 0;
    if (extract_keys_by_patterns(llm_response, key_patterns, 
                                sizeof(key_patterns) / sizeof(key_patterns[0]),
                                parsed_response->context_keys, 
                                32, &keys_extracted) == RESULT_OK) {
        parsed_response->context_key_count = keys_extracted;
    }
    
    /* Parse paging directives if paging block exists */
    if (has_paging && parsed_response->paging.size > 0) {
        paging_directive_t directives[16];
        size_t directive_count = 0;
        
        if (llm_parse_directives(parsed_response->paging.data, directives, 16, &directive_count) == RESULT_OK) {
            /* Convert directives to string format for storage */
            for (size_t i = 0; i < directive_count && i < 16; i++) {
                snprintf(parsed_response->paging_directives[i], 128, "%s:%s:%s",
                        directives[i].directive_type,
                        directives[i].target_key,
                        directives[i].parameters);
            }
            parsed_response->directive_count = directive_count;
        }
    }
    
    /* Calculate overall quality score */
    size_t quality_components = 0;
    size_t total_quality = 0;
    
    if (has_thinking) {
        size_t thinking_quality = 0;
        calculate_content_quality(parsed_response->thinking.data, &thinking_quality);
        total_quality += thinking_quality;
        quality_components++;
    }
    
    if (has_action) {
        size_t action_quality = 0;
        calculate_content_quality(parsed_response->action.data, &action_quality);
        total_quality += action_quality;
        quality_components++;
    }
    
    if (has_evaluation) {
        size_t eval_quality = 0;
        calculate_content_quality(parsed_response->evaluation.data, &eval_quality);
        total_quality += eval_quality;
        quality_components++;
    }
    
    if (has_paging) {
        size_t paging_quality = 0;
        calculate_content_quality(parsed_response->paging.data, &paging_quality);
        total_quality += paging_quality;
        quality_components++;
    }
    
    /* Bonus for having multiple block types */
    if (quality_components > 1) {
        total_quality += 10 * quality_components;
    }
    
    /* Bonus for context keys */
    if (parsed_response->context_key_count > 0) {
        total_quality += MIN(parsed_response->context_key_count * 5, 20);
    }
    
    /* Calculate final score */
    if (quality_components > 0) {
        parsed_response->quality_score = MIN(total_quality / quality_components, 100);
    } else {
        parsed_response->quality_score = 0;
    }
    
    return RESULT_OK;
}

result_t llm_parse_thinking_block(const char* llm_response, data_t* thinking_content) {
    if (!llm_response || !thinking_content) {
        RETURN_ERR("Invalid parameters for thinking block parsing");
        return RESULT_ERR;
    }
    
    return extract_tag_content(llm_response, "thinking", thinking_content, true);
}

result_t llm_parse_action_block(const char* llm_response, data_t* action_content) {
    if (!llm_response || !action_content) {
        RETURN_ERR("Invalid parameters for action block parsing");
        return RESULT_ERR;
    }
    
    return extract_tag_content(llm_response, "action", action_content, true);
}

result_t llm_parse_evaluation_block(const char* llm_response, data_t* evaluation_content) {
    if (!llm_response || !evaluation_content) {
        RETURN_ERR("Invalid parameters for evaluation block parsing");
        return RESULT_ERR;
    }
    
    return extract_tag_content(llm_response, "evaluation", evaluation_content, true);
}

result_t llm_parse_paging_block(const char* llm_response, data_t* paging_content) {
    if (!llm_response || !paging_content) {
        RETURN_ERR("Invalid parameters for paging block parsing");
        return RESULT_ERR;
    }
    
    return extract_tag_content(llm_response, "paging", paging_content, true);
}

result_t llm_validate_tag_format(const char* llm_response, data_t* validation_errors) {
    if (!llm_response || !validation_errors) {
        RETURN_ERR("Invalid parameters for tag format validation");
        return RESULT_ERR;
    }
    
    /* Clear errors buffer */
    if (data_set(validation_errors, "", 0) != RESULT_OK) {
        RETURN_ERR("Failed to clear validation errors buffer");
        return RESULT_ERR;
    }
    
    /* Validate each supported tag type */
    const char* supported_tags[] = {"thinking", "action", "evaluation", "paging"};
    bool has_errors = false;
    
    for (size_t i = 0; i < sizeof(supported_tags) / sizeof(supported_tags[0]); i++) {
        if (validate_tag_structure(llm_response, supported_tags[i], validation_errors) != RESULT_OK) {
            has_errors = true;
        }
    }
    
    /* Check for unknown tags */
    const char* pos = llm_response;
    while ((pos = strchr(pos, '<')) != NULL) {
        if (pos[1] == '/' || pos[1] == '!') {
            pos++;
            continue;
        }
        
        const char* tag_end = strchr(pos, '>');
        if (!tag_end) {
            data_append(validation_errors, "Unclosed tag bracket found; ", 0);
            has_errors = true;
            pos++;
            continue;
        }
        
        /* Extract tag name */
        size_t tag_len = tag_end - pos - 1;
        if (tag_len > 0 && tag_len < 64) {
            char tag_name[65];
            strncpy(tag_name, pos + 1, tag_len);
            tag_name[tag_len] = '\0';
            
            /* Check if it's a supported tag */
            bool supported = false;
            for (size_t j = 0; j < sizeof(supported_tags) / sizeof(supported_tags[0]); j++) {
                if (strcasecmp(tag_name, supported_tags[j]) == 0) {
                    supported = true;
                    break;
                }
            }
            
            if (!supported) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "Unknown tag <%s> found; ", tag_name);
                data_append(validation_errors, error_msg, 0);
                has_errors = true;
            }
        }
        
        pos = tag_end + 1;
    }
    
    return has_errors ? RESULT_ERR : RESULT_OK;
}

result_t llm_extract_context_keys(const char* llm_response, context_key_result_t* context_keys, size_t max_keys, size_t* keys_found) {
    if (!llm_response || !context_keys || !keys_found) {
        RETURN_ERR("Invalid parameters for context key extraction");
        return RESULT_ERR;
    }
    
    *keys_found = 0;
    
    /* Define comprehensive patterns for context key detection */
    const char* patterns[] = {
        "key:",
        "context:",
        "reference:",
        "data:",
        "item:",
        "memory:",
        "stored:",
        "cached:",
        "@",
        "#",
        "recall:",
        "retrieve:",
        "lookup:"
    };
    
    char temp_keys[64][64];
    size_t temp_count = 0;
    
    if (extract_keys_by_patterns(llm_response, patterns, 
                                sizeof(patterns) / sizeof(patterns[0]),
                                temp_keys, 64, &temp_count) == RESULT_OK) {
        
        /* Convert to context_key_result_t format */
        for (size_t i = 0; i < temp_count && i < max_keys; i++) {
            strncpy(context_keys[i].key, temp_keys[i], 63);
            context_keys[i].key[63] = '\0';
            context_keys[i].importance = 50; /* Default importance */
            strcpy(context_keys[i].source_block, "general");
            context_keys[i].text_position = 0; /* Could be enhanced to track positions */
        }
        
        *keys_found = MIN(temp_count, max_keys);
    }
    
    return RESULT_OK;
}

result_t llm_parse_directives(const char* paging_content, paging_directive_t* directives, size_t max_directives, size_t* directives_found) {
    if (!paging_content || !directives || !directives_found) {
        RETURN_ERR("Invalid parameters for directive parsing");
        return RESULT_ERR;
    }
    
    *directives_found = 0;
    
    /* Parse each line for directives */
    char* content_copy = strdup(paging_content);
    if (!content_copy) {
        RETURN_ERR("Failed to allocate memory for content copy");
        return RESULT_ERR;
    }
    
    char* line = strtok(content_copy, "\n");
    size_t line_number = 0;
    
    while (line && *directives_found < max_directives) {
        line_number++;
        
        /* Skip empty lines and comments */
        while (*line && isspace(*line)) line++;
        if (*line == '\0' || *line == '#') {
            line = strtok(NULL, "\n");
            continue;
        }
        
        /* Parse directive format: COMMAND target_key [parameters] */
        char* command = strtok(line, " \t");
        if (!command) {
            line = strtok(NULL, "\n");
            continue;
        }
        
        /* Check for known directive types */
        if (strcasecmp(command, "move") == 0 ||
            strcasecmp(command, "archive") == 0 ||
            strcasecmp(command, "delete") == 0 ||
            strcasecmp(command, "importance") == 0) {
            
            char* target = strtok(NULL, " \t");
            if (target) {
                /* Store directive */
                strncpy(directives[*directives_found].directive_type, command, 31);
                directives[*directives_found].directive_type[31] = '\0';
                
                strncpy(directives[*directives_found].target_key, target, 63);
                directives[*directives_found].target_key[63] = '\0';
                
                /* Get remaining parameters */
                char* params = strtok(NULL, "");
                if (params) {
                    /* Trim leading whitespace */
                    while (*params && isspace(*params)) params++;
                    strncpy(directives[*directives_found].parameters, params, 255);
                    directives[*directives_found].parameters[255] = '\0';
                } else {
                    directives[*directives_found].parameters[0] = '\0';
                }
                
                directives[*directives_found].source_line = line_number;
                (*directives_found)++;
            }
        }
        
        line = strtok(NULL, "\n");
    }
    
    free(content_copy);
    return RESULT_OK;
}

result_t llm_response_quality_score(const char* llm_response, const llm_parsed_response_t* parsed_response, size_t* quality_score) {
    if (!llm_response || !quality_score) {
        RETURN_ERR("Invalid parameters for quality score calculation");
        return RESULT_ERR;
    }
    
    size_t score = 0;
    
    /* Basic response length check */
    size_t response_len = strlen(llm_response);
    if (response_len > 0) score += 10;
    if (response_len > 50) score += 10;
    if (response_len > 200) score += 10;
    
    /* Check for structured blocks */
    if (strstr(llm_response, "<thinking>") && strstr(llm_response, "</thinking>")) {
        score += 20;
    }
    if (strstr(llm_response, "<action>") && strstr(llm_response, "</action>")) {
        score += 20;
    }
    if (strstr(llm_response, "<evaluation>") && strstr(llm_response, "</evaluation>")) {
        score += 15;
    }
    if (strstr(llm_response, "<paging>") && strstr(llm_response, "</paging>")) {
        score += 15;
    }
    
    /* Use parsed response data if available */
    if (parsed_response) {
        /* Bonus for context keys */
        if (parsed_response->context_key_count > 0) {
            score += MIN(parsed_response->context_key_count * 2, 10);
        }
        
        /* Bonus for paging directives */
        if (parsed_response->directive_count > 0) {
            score += MIN(parsed_response->directive_count * 3, 15);
        }
        
        /* Penalty for parse errors */
        if (parsed_response->parse_errors.size > 0) {
            score = (score > 20) ? score - 20 : 0;
        }
    }
    
    *quality_score = MIN(score, 100);
    return RESULT_OK;
}

result_t llm_extract_context_keys_thinking(const char* thinking_content, char context_keys[][64], size_t max_keys, size_t* keys_found) {
    if (!thinking_content || !context_keys || !keys_found) {
        RETURN_ERR("Invalid parameters for thinking context key extraction");
        return RESULT_ERR;
    }
    
    /* Thinking-specific patterns */
    const char* patterns[] = {
        "analyzing ",
        "considering ",
        "reviewing ",
        "examining ",
        "data:",
        "information:",
        "context:",
        "based on ",
        "from "
    };
    
    return extract_keys_by_patterns(thinking_content, patterns,
                                   sizeof(patterns) / sizeof(patterns[0]),
                                   context_keys, max_keys, keys_found);
}

result_t llm_extract_context_keys_action(const char* action_content, char context_keys[][64], size_t max_keys, size_t* keys_found) {
    if (!action_content || !context_keys || !keys_found) {
        RETURN_ERR("Invalid parameters for action context key extraction");
        return RESULT_ERR;
    }
    
    /* Action-specific patterns */
    const char* patterns[] = {
        "execute ",
        "run ",
        "process ",
        "handle ",
        "using ",
        "with ",
        "target:",
        "resource:",
        "file:",
        "data:"
    };
    
    return extract_keys_by_patterns(action_content, patterns,
                                   sizeof(patterns) / sizeof(patterns[0]),
                                   context_keys, max_keys, keys_found);
}

result_t llm_extract_context_keys_evaluation(const char* evaluation_content, char context_keys[][64], size_t max_keys, size_t* keys_found) {
    if (!evaluation_content || !context_keys || !keys_found) {
        RETURN_ERR("Invalid parameters for evaluation context key extraction");
        return RESULT_ERR;
    }
    
    /* Evaluation-specific patterns */
    const char* patterns[] = {
        "assessed ",
        "evaluated ",
        "measured ",
        "performance of ",
        "quality of ",
        "results from ",
        "outcome:",
        "metric:",
        "score:",
        "rating:"
    };
    
    return extract_keys_by_patterns(evaluation_content, patterns,
                                   sizeof(patterns) / sizeof(patterns[0]),
                                   context_keys, max_keys, keys_found);
}

result_t llm_extract_context_keys_paging(const char* paging_content, char context_keys[][64], size_t max_keys, size_t* keys_found) {
    if (!paging_content || !context_keys || !keys_found) {
        RETURN_ERR("Invalid parameters for paging context key extraction");
        return RESULT_ERR;
    }
    
    /* Paging-specific patterns - extract from directive targets */
    const char* patterns[] = {
        "move ",
        "archive ",
        "delete ",
        "importance ",
        "key:",
        "target:",
        "context:",
        "memory:"
    };
    
    return extract_keys_by_patterns(paging_content, patterns,
                                   sizeof(patterns) / sizeof(patterns[0]),
                                   context_keys, max_keys, keys_found);
}

result_t llm_parsed_response_init(llm_parsed_response_t* parsed_response) {
    if (!parsed_response) {
        RETURN_ERR("Parsed response pointer is NULL");
        return RESULT_ERR;
    }
    
    /* Initialize structure */
    memset(parsed_response, 0, sizeof(llm_parsed_response_t));
    
    /* Initialize data buffers */
    if (data_init(&parsed_response->thinking, 2048) != RESULT_OK) {
        RETURN_ERR("Failed to initialize thinking buffer");
        return RESULT_ERR;
    }
    
    if (data_init(&parsed_response->action, 1024) != RESULT_OK) {
        data_clear(&parsed_response->thinking);
        RETURN_ERR("Failed to initialize action buffer");
        return RESULT_ERR;
    }
    
    if (data_init(&parsed_response->evaluation, 1024) != RESULT_OK) {
        data_clear(&parsed_response->thinking);
        data_clear(&parsed_response->action);
        RETURN_ERR("Failed to initialize evaluation buffer");
        return RESULT_ERR;
    }
    
    if (data_init(&parsed_response->paging, 512) != RESULT_OK) {
        data_clear(&parsed_response->thinking);
        data_clear(&parsed_response->action);
        data_clear(&parsed_response->evaluation);
        RETURN_ERR("Failed to initialize paging buffer");
        return RESULT_ERR;
    }
    
    if (data_init(&parsed_response->parse_errors, 1024) != RESULT_OK) {
        data_clear(&parsed_response->thinking);
        data_clear(&parsed_response->action);
        data_clear(&parsed_response->evaluation);
        data_clear(&parsed_response->paging);
        RETURN_ERR("Failed to initialize parse errors buffer");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t llm_parsed_response_cleanup(llm_parsed_response_t* parsed_response) {
    if (!parsed_response) {
        RETURN_ERR("Parsed response pointer is NULL");
        return RESULT_ERR;
    }
    
    /* Clean up data buffers */
    data_clear(&parsed_response->thinking);
    data_clear(&parsed_response->action);
    data_clear(&parsed_response->evaluation);
    data_clear(&parsed_response->paging);
    data_clear(&parsed_response->parse_errors);
    
    /* Reset structure */
    memset(parsed_response, 0, sizeof(llm_parsed_response_t));
    
    return RESULT_OK;
}

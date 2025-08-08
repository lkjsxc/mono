#include "agent/actions.h"

// Forward declarations for local helpers used before their definitions
static void destroy_string_with_warning(pool_t* pool, string_t* s, const char* context);
static void destroy_string_array_with_warning(pool_t* pool, string_t** arr, size_t count, const char* context);

result_t agent_actions_dispatch(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj) {
    object_t* type_obj = NULL;
    object_t* tags_obj = NULL;
    object_t* value_obj = NULL;

    if (agent_actions_ensure_working_memory_exists(pool, agent) != RESULT_OK) {
        printf("Warning: Failed to ensure working memory exists before dispatch logging\n");
    }

    if (agent_actions_extract_action_params(pool, action_obj, &type_obj, &tags_obj, &value_obj) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "unknown", "unknown", "Failed to extract action parameters") != RESULT_OK) {
            printf("Warning: Failed to log action parameter extraction failure\n");
        }
        RETURN_ERR("Failed to extract action parameters");
    }

    if (string_equal_str(type_obj->string, "working_memory_add")) {
        if (agent_actions_validate_action_params(type_obj, tags_obj, value_obj, "working_memory_add", 1) != RESULT_OK) {
            if (agent_actions_log_result(pool, config, agent, "working_memory_add", tags_obj ? tags_obj->string->data : "unknown", "Invalid parameters for working_memory_add action") != RESULT_OK) {
                printf("Warning: Failed to log working_memory_add validation failure\n");
            }
            RETURN_ERR("Invalid parameters for working_memory_add action");
        }
        return agent_actions_command_working_memory_add(pool, config, agent, action_obj);

    } else if (string_equal_str(type_obj->string, "working_memory_remove")) {
        if (agent_actions_validate_action_params(type_obj, tags_obj, value_obj, "working_memory_remove", 0) != RESULT_OK) {
            if (agent_actions_log_result(pool, config, agent, "working_memory_remove", tags_obj ? tags_obj->string->data : "unknown", "Invalid parameters for working_memory_remove action") != RESULT_OK) {
                printf("Warning: Failed to log working_memory_remove validation failure\n");
            }
            RETURN_ERR("Invalid parameters for working_memory_remove action");
        }
        return agent_actions_command_working_memory_remove(pool, config, agent, action_obj);

    } else if (string_equal_str(type_obj->string, "storage_load")) {
        if (agent_actions_validate_action_params(type_obj, tags_obj, value_obj, "storage_load", 0) != RESULT_OK) {
            if (agent_actions_log_result(pool, config, agent, "storage_load", tags_obj ? tags_obj->string->data : "unknown", "Invalid parameters for storage_load action") != RESULT_OK) {
                printf("Warning: Failed to log storage_load validation failure\n");
            }
            RETURN_ERR("Invalid parameters for storage_load action");
        }
        return agent_actions_command_storage_load(pool, config, agent, action_obj);

    } else if (string_equal_str(type_obj->string, "storage_save")) {
        if (agent_actions_validate_action_params(type_obj, tags_obj, value_obj, "storage_save", 1) != RESULT_OK) {
            if (agent_actions_log_result(pool, config, agent, "storage_save", tags_obj ? tags_obj->string->data : "unknown", "Invalid parameters for storage_save action") != RESULT_OK) {
                printf("Warning: Failed to log storage_save validation failure\n");
            }
            RETURN_ERR("Invalid parameters for storage_save action");
        }
        return agent_actions_command_storage_save(pool, config, agent, action_obj);

    } else if (string_equal_str(type_obj->string, "storage_search")) {
        if (agent_actions_validate_action_params(type_obj, tags_obj, value_obj, "storage_search", 0) != RESULT_OK) {
            if (agent_actions_log_result(pool, config, agent, "storage_search", tags_obj ? tags_obj->string->data : "unknown", "Invalid parameters for storage_search action") != RESULT_OK) {
                printf("Warning: Failed to log storage_search validation failure\n");
            }
            RETURN_ERR("Invalid parameters for storage_search action");
        }
        return agent_actions_command_storage_search(pool, config, agent, action_obj);

    } else {
        if (agent_actions_log_result(pool, config, agent,
                                           type_obj ? type_obj->string->data : "unknown",
                                           tags_obj ? tags_obj->string->data : "unknown",
                                           "Unknown action type") != RESULT_OK) {
            printf("Warning: Failed to log unknown action type failure\n");
        }
        RETURN_ERR("Unknown action type");
    }
}

// Check if key contains all query tags (exact tag matching on comma-separated tokens)
static uint64_t key_contains_all_tags(const string_t* key, string_t** tokens, size_t token_count) {
    if (!key) return 0;
    const char* kd = key->data;
    uint64_t ks = key->size;
    for (size_t t = 0; t < token_count; t++) {
        const char* td = tokens[t]->data;
        uint64_t ts = tokens[t]->size;
        uint64_t i = 0;
        uint64_t found = 0;
        if (ts == 0) return 0;
        while (i + ts <= ks) {
            if (memcmp(kd + i, td, ts) == 0) {
                if ((i == 0 || kd[i - 1] == ',') && (i + ts == ks || kd[i + ts] == ',')) {
                    found = 1; break;
                }
            }
            i++;
        }
        if (!found) return 0;
    }
    return 1;
}

result_t agent_actions_command_storage_search(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj) {
    object_t* type_obj = NULL;
    object_t* tags_obj = NULL;
    object_t* value_obj = NULL;
    object_t* storage = NULL;
    object_t* working_memory = NULL;
    string_t* normalized_query = NULL;

    if (agent_actions_ensure_storage_exists(pool, agent) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_search", "unknown", "Failed to ensure storage exists") != RESULT_OK) {
            printf("Warning: Failed to log storage_search storage existence failure\n");
        }
        RETURN_ERR("Failed to ensure storage exists for search");
    }
    if (agent_actions_get_storage(pool, agent, &storage) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_search", "unknown", "Failed to get storage") != RESULT_OK) {
            printf("Warning: Failed to log storage_search storage access failure\n");
        }
        RETURN_ERR("Failed to get storage for search");
    }
    if (agent_actions_ensure_working_memory_exists(pool, agent) != RESULT_OK) {
        printf("Warning: Failed to ensure working memory exists before storage_search logging\n");
    }
    if (agent_actions_get_working_memory(pool, agent, &working_memory) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_search", "unknown", "Failed to get working memory") != RESULT_OK) {
            printf("Warning: Failed to log storage_search working memory access failure\n");
        }
        RETURN_ERR("Failed to get working memory for search");
    }

    if (agent_actions_extract_action_params(pool, action_obj, &type_obj, &tags_obj, &value_obj) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_search", "unknown", "Failed to extract action parameters") != RESULT_OK) {
            printf("Warning: Failed to log storage_search parameter extraction failure\n");
        }
        RETURN_ERR("Failed to extract parameters for storage_search");
    }

    if (agent_actions_normalize_storage_tags(pool, tags_obj, &normalized_query) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_search", tags_obj ? tags_obj->string->data : "unknown", "Failed to normalize tags") != RESULT_OK) {
            printf("Warning: Failed to log storage_search tag normalization failure\n");
        }
        RETURN_ERR("Failed to normalize tags for storage_search");
    }

    // Tokenize normalized_query into tokens[]
    enum { MAX_Q = 64 };
    string_t* tokens[MAX_Q];
    size_t token_count = 0;
    const char* qd = normalized_query->data;
    uint64_t qs = normalized_query->size;
    uint64_t i = 0;
    while (i < qs && token_count < MAX_Q) {
        uint64_t j = i;
        while (j < qs && qd[j] != ',') j++;
        if (j > i) {
            string_t* tok = NULL;
            if (string_create(pool, &tok) != RESULT_OK) {
                destroy_string_with_warning(pool, normalized_query, "search token create failure cleanup");
                RETURN_ERR("Failed to create search token");
            }
            for (uint64_t k = i; k < j; k++) {
                if (string_append_char(pool, &tok, qd[k]) != RESULT_OK) {
                    destroy_string_with_warning(pool, tok, "search token append failure");
                    destroy_string_with_warning(pool, normalized_query, "search token append failure cleanup");
                    RETURN_ERR("Failed to append to search token");
                }
            }
            tokens[token_count++] = tok;
        }
        i = (j < qs) ? (j + 1) : j;
    }

    // Build result list of keys
    string_t* result_list = NULL;
    if (string_create(pool, &result_list) != RESULT_OK) {
        destroy_string_array_with_warning(pool, tokens, token_count, "result_list create failure cleanup");
        destroy_string_with_warning(pool, normalized_query, "result_list create failure cleanup");
        RETURN_ERR("Failed to create result list");
    }

    uint64_t matches = 0;
    for (object_t* child = storage->child; child; child = child->next) {
        if (!child->string) continue; // skip malformed
        if (key_contains_all_tags(child->string, tokens, token_count)) {
            if (matches > 0) {
                if (string_append_char(pool, &result_list, ',') != RESULT_OK) {
                    destroy_string_with_warning(pool, result_list, "append comma failure");
                    destroy_string_array_with_warning(pool, tokens, token_count, "append comma failure cleanup");
                    destroy_string_with_warning(pool, normalized_query, "append comma failure cleanup");
                    RETURN_ERR("Failed to append comma to result list");
                }
            }
            if (string_append_string(pool, &result_list, child->string) != RESULT_OK) {
                destroy_string_with_warning(pool, result_list, "append key failure");
                destroy_string_array_with_warning(pool, tokens, token_count, "append key failure cleanup");
                destroy_string_with_warning(pool, normalized_query, "append key failure cleanup");
                RETURN_ERR("Failed to append key to result list");
            }
            matches++;
        }
    }

    // Write results under working_memory: search_results.<normalized_query>
    string_t* result_path = NULL;
    if (string_create_str(pool, &result_path, "search_results") != RESULT_OK) {
        destroy_string_with_warning(pool, result_list, "result_path create failure cleanup (base)");
        destroy_string_array_with_warning(pool, tokens, token_count, "result_path create failure cleanup");
        destroy_string_with_warning(pool, normalized_query, "result_path create failure cleanup");
        RETURN_ERR("Failed to create base result path");
    }
    if (string_append_char(pool, &result_path, '.') != RESULT_OK ||
        string_append_string(pool, &result_path, normalized_query) != RESULT_OK) {
        destroy_string_with_warning(pool, result_path, "result_path append failure");
        destroy_string_with_warning(pool, result_list, "result_path append failure cleanup");
        destroy_string_array_with_warning(pool, tokens, token_count, "result_path append failure cleanup");
        destroy_string_with_warning(pool, normalized_query, "result_path append failure cleanup");
        RETURN_ERR("Failed to build result path");
    }

    if (object_set_string(pool, working_memory, result_path, result_list) != RESULT_OK) {
        destroy_string_with_warning(pool, result_path, "result_path after set failure");
        destroy_string_with_warning(pool, result_list, "result_list after set failure");
        destroy_string_array_with_warning(pool, tokens, token_count, "set failure cleanup");
        destroy_string_with_warning(pool, normalized_query, "set failure cleanup");
        RETURN_ERR("Failed to write search results to working memory");
    }

    if (agent_actions_log_result(pool, config, agent, "storage_search", normalized_query->data, matches == 0 ? "No matches" : "Search completed") != RESULT_OK) {
        printf("Warning: Failed to log storage_search result\n");
    }

    // Cleanup
    if (string_destroy(pool, result_path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy result path");
    }
    if (string_destroy(pool, result_list) != RESULT_OK) {
        RETURN_ERR("Failed to destroy result list");
    }
    destroy_string_array_with_warning(pool, tokens, token_count, "final tokens cleanup (search)");
    if (string_destroy(pool, normalized_query) != RESULT_OK) {
        RETURN_ERR("Failed to destroy normalized query");
    }

    return RESULT_OK;
}

result_t agent_actions_command_working_memory_add(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj) {
    object_t* type_obj = NULL;
    object_t* tags_obj = NULL;
    object_t* value_obj = NULL;
    object_t* working_memory = NULL;
    string_t* processed_tags = NULL;

    if (agent_actions_ensure_working_memory_exists(pool, agent) != RESULT_OK) {
        printf("Warning: Failed to ensure working memory exists before working_memory_add logging\n");
    }

    if (agent_actions_extract_action_params(pool, action_obj, &type_obj, &tags_obj, &value_obj) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "working_memory_add", "unknown", "Failed to extract action parameters") != RESULT_OK) {
            printf("Warning: Failed to log working_memory_add parameter extraction failure\n");
        }
        RETURN_ERR("Failed to extract parameters for working_memory_add");
    }

    if (agent_actions_ensure_working_memory_exists(pool, agent) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "working_memory_add", tags_obj ? tags_obj->string->data : "unknown", "Failed to ensure working memory exists") != RESULT_OK) {
            printf("Warning: Failed to log working_memory_add memory existence failure\n");
        }
        RETURN_ERR("Failed to ensure working memory exists for add operation");
    }

    if (agent_actions_get_working_memory(pool, agent, &working_memory) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "working_memory_add", tags_obj ? tags_obj->string->data : "unknown", "Failed to get working memory") != RESULT_OK) {
            printf("Warning: Failed to log working_memory_add memory access failure\n");
        }
        RETURN_ERR("Failed to get working memory for add operation");
    }

    if (agent_actions_process_tags(pool, tags_obj, &processed_tags) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "working_memory_add", tags_obj ? tags_obj->string->data : "unknown", "Failed to process tags") != RESULT_OK) {
            printf("Warning: Failed to log working_memory_add tag processing failure\n");
        }
        RETURN_ERR("Failed to process tags for working_memory_add");
    }

    if (object_set_string(pool, working_memory, processed_tags, value_obj->string) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "working_memory_add", processed_tags->data, "Failed to add item to working memory") != RESULT_OK) {
            printf("Warning: Failed to log working_memory_add set failure\n");
        }
        result_t tmp_sd = string_destroy(pool, processed_tags);
        if (tmp_sd != RESULT_OK) {
            printf("Warning: Failed to destroy processed_tags after set failure\n");
        }
        RETURN_ERR("Failed to add item to working memory");
    }

    if (agent_actions_log_result(pool, config, agent, "working_memory_add", processed_tags->data, "Successfully added item to working memory") != RESULT_OK) {
        printf("Warning: Failed to log working_memory_add result\n");
    }

    if (string_destroy(pool, processed_tags) != RESULT_OK) {
        RETURN_ERR("Failed to destroy processed tags after working_memory_add");
    }

    return RESULT_OK;
}

result_t agent_actions_command_working_memory_remove(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj) {
    object_t* type_obj = NULL;
    object_t* tags_obj = NULL;
    object_t* value_obj = NULL;
    object_t* working_memory = NULL;
    string_t* processed_tags = NULL;
    string_t* empty_string = NULL;

    if (agent_actions_ensure_working_memory_exists(pool, agent) != RESULT_OK) {
        printf("Warning: Failed to ensure working memory exists before working_memory_remove logging\n");
    }

    if (agent_actions_extract_action_params(pool, action_obj, &type_obj, &tags_obj, &value_obj) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "working_memory_remove", "unknown", "Failed to extract action parameters") != RESULT_OK) {
            printf("Warning: Failed to log working_memory_remove parameter extraction failure\n");
        }
        RETURN_ERR("Failed to extract parameters for working_memory_remove");
    }

    if (agent_actions_ensure_working_memory_exists(pool, agent) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "working_memory_remove", tags_obj ? tags_obj->string->data : "unknown", "Failed to ensure working memory exists") != RESULT_OK) {
            printf("Warning: Failed to log working_memory_remove memory existence failure\n");
        }
        RETURN_ERR("Failed to ensure working memory exists for remove operation");
    }

    if (agent_actions_get_working_memory(pool, agent, &working_memory) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "working_memory_remove", tags_obj ? tags_obj->string->data : "unknown", "Failed to get working memory") != RESULT_OK) {
            printf("Warning: Failed to log working_memory_remove memory access failure\n");
        }
        RETURN_ERR("Failed to get working memory for remove operation");
    }

    if (agent_actions_process_tags(pool, tags_obj, &processed_tags) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "working_memory_remove", tags_obj ? tags_obj->string->data : "unknown", "Failed to process tags") != RESULT_OK) {
            printf("Warning: Failed to log working_memory_remove tag processing failure\n");
        }
        RETURN_ERR("Failed to process tags for working_memory_remove");
    }

    if (string_create_str(pool, &empty_string, "") != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "working_memory_remove", processed_tags->data, "Failed to create empty string") != RESULT_OK) {
            printf("Warning: Failed to log working_memory_remove empty string creation failure\n");
        }
        result_t tmp_sd = string_destroy(pool, processed_tags);
        if (tmp_sd != RESULT_OK) {
            printf("Warning: Failed to destroy processed_tags after empty string creation failure\n");
        }
        RETURN_ERR("Failed to create empty string for working_memory_remove");
    }

    if (object_set_string(pool, working_memory, processed_tags, empty_string) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "working_memory_remove", processed_tags->data, "Failed to remove item from working memory") != RESULT_OK) {
            printf("Warning: Failed to log working_memory_remove set failure\n");
        }
        result_t tmp1 = string_destroy(pool, processed_tags);
        if (tmp1 != RESULT_OK) {
            printf("Warning: Failed to destroy processed_tags after remove failure\n");
        }
        result_t tmp2 = string_destroy(pool, empty_string);
        if (tmp2 != RESULT_OK) {
            printf("Warning: Failed to destroy empty_string after remove failure\n");
        }
        RETURN_ERR("Failed to remove item from working memory");
    }

    if (agent_actions_log_result(pool, config, agent, "working_memory_remove", processed_tags->data, "Successfully removed item from working memory") != RESULT_OK) {
        printf("Warning: Failed to log working_memory_remove result\n");
    }

    if (string_destroy(pool, processed_tags) != RESULT_OK || string_destroy(pool, empty_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy strings after working_memory_remove");
    }

    return RESULT_OK;
}

result_t agent_actions_command_storage_load(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj) {
    object_t* type_obj = NULL;
    object_t* tags_obj = NULL;
    object_t* value_obj = NULL;
    object_t* storage = NULL;
    object_t* working_memory = NULL;
    string_t* processed_tags = NULL;
    object_t* stored_item = NULL;

    if (agent_actions_ensure_working_memory_exists(pool, agent) != RESULT_OK) {
        printf("Warning: Failed to ensure working memory exists before storage_load logging\n");
    }

    if (agent_actions_extract_action_params(pool, action_obj, &type_obj, &tags_obj, &value_obj) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_load", "unknown", "Failed to extract action parameters") != RESULT_OK) {
            printf("Warning: Failed to log storage_load parameter extraction failure\n");
        }
        RETURN_ERR("Failed to extract parameters for storage_load");
    }

    if (agent_actions_ensure_storage_exists(pool, agent) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_load", tags_obj ? tags_obj->string->data : "unknown", "Failed to ensure storage exists") != RESULT_OK) {
            printf("Warning: Failed to log storage_load storage existence failure\n");
        }
        RETURN_ERR("Failed to ensure storage exists for load operation");
    }

    if (agent_actions_get_storage(pool, agent, &storage) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_load", tags_obj ? tags_obj->string->data : "unknown", "Failed to get storage") != RESULT_OK) {
            printf("Warning: Failed to log storage_load storage access failure\n");
        }
        RETURN_ERR("Failed to get storage for load operation");
    }

    if (agent_actions_ensure_working_memory_exists(pool, agent) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_load", tags_obj ? tags_obj->string->data : "unknown", "Failed to ensure working memory exists") != RESULT_OK) {
            printf("Warning: Failed to log storage_load memory existence failure\n");
        }
        RETURN_ERR("Failed to ensure working memory exists for load operation");
    }

    if (agent_actions_get_working_memory(pool, agent, &working_memory) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_load", tags_obj ? tags_obj->string->data : "unknown", "Failed to get working memory") != RESULT_OK) {
            printf("Warning: Failed to log storage_load working memory access failure\n");
        }
        RETURN_ERR("Failed to get working memory for load operation");
    }

    if (agent_actions_normalize_storage_tags(pool, tags_obj, &processed_tags) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_load", tags_obj ? tags_obj->string->data : "unknown", "Failed to process tags") != RESULT_OK) {
            printf("Warning: Failed to log storage_load tag processing failure\n");
        }
        RETURN_ERR("Failed to process tags for storage_load");
    }

    if (object_provide_string(&stored_item, storage, processed_tags) == RESULT_OK) {
        if (object_set_string(pool, working_memory, processed_tags, stored_item->string) != RESULT_OK) {
            if (agent_actions_log_result(pool, config, agent, "storage_load", processed_tags->data, "Failed to copy item from storage to working memory") != RESULT_OK) {
                printf("Warning: Failed to log storage_load copy failure\n");
            }
            result_t tmp_sd = string_destroy(pool, processed_tags);
            if (tmp_sd != RESULT_OK) {
                printf("Warning: Failed to destroy processed_tags after copy failure\n");
            }
            RETURN_ERR("Failed to copy item from storage to working memory");
        }
        if (agent_actions_log_result(pool, config, agent, "storage_load", processed_tags->data, "Successfully loaded item from storage to working memory") != RESULT_OK) {
            printf("Warning: Failed to log storage_load success\n");
        }
    } else {
        if (agent_actions_log_result(pool, config, agent, "storage_load", processed_tags->data, "Item not found in storage") != RESULT_OK) {
            printf("Warning: Failed to log storage_load not found\n");
        }
    }

    if (string_destroy(pool, processed_tags) != RESULT_OK) {
        RETURN_ERR("Failed to destroy processed tags after storage_load");
    }

    return RESULT_OK;
}

result_t agent_actions_command_storage_save(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj) {
    object_t* type_obj = NULL;
    object_t* tags_obj = NULL;
    object_t* value_obj = NULL;
    object_t* storage = NULL;
    string_t* processed_tags = NULL;

    if (agent_actions_ensure_working_memory_exists(pool, agent) != RESULT_OK) {
        printf("Warning: Failed to ensure working memory exists before storage_save logging\n");
    }

    if (agent_actions_extract_action_params(pool, action_obj, &type_obj, &tags_obj, &value_obj) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_save", "unknown", "Failed to extract action parameters") != RESULT_OK) {
            printf("Warning: Failed to log storage_save parameter extraction failure\n");
        }
        RETURN_ERR("Failed to extract parameters for storage_save");
    }

    if (agent_actions_ensure_storage_exists(pool, agent) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_save", tags_obj ? tags_obj->string->data : "unknown", "Failed to ensure storage exists") != RESULT_OK) {
            printf("Warning: Failed to log storage_save storage existence failure\n");
        }
        RETURN_ERR("Failed to ensure storage exists for save operation");
    }

    if (agent_actions_get_storage(pool, agent, &storage) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_save", tags_obj ? tags_obj->string->data : "unknown", "Failed to get storage") != RESULT_OK) {
            printf("Warning: Failed to log storage_save storage access failure\n");
        }
        RETURN_ERR("Failed to get storage for save operation");
    }

    if (agent_actions_normalize_storage_tags(pool, tags_obj, &processed_tags) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_save", tags_obj ? tags_obj->string->data : "unknown", "Failed to process tags") != RESULT_OK) {
            printf("Warning: Failed to log storage_save tag processing failure\n");
        }
        RETURN_ERR("Failed to process tags for storage_save");
    }

    if (object_set_string(pool, storage, processed_tags, value_obj->string) != RESULT_OK) {
        if (agent_actions_log_result(pool, config, agent, "storage_save", processed_tags->data, "Failed to save item to storage") != RESULT_OK) {
            printf("Warning: Failed to log storage_save save failure\n");
        }
        result_t tmp_sd = string_destroy(pool, processed_tags);
        if (tmp_sd != RESULT_OK) {
            printf("Warning: Failed to destroy processed_tags after storage save failure\n");
        }
        RETURN_ERR("Failed to save item to storage");
    }

    if (agent_actions_log_result(pool, config, agent, "storage_save", processed_tags->data, "Successfully saved item to storage") != RESULT_OK) {
        printf("Warning: Failed to log storage_save success\n");
    }

    if (string_destroy(pool, processed_tags) != RESULT_OK) {
        RETURN_ERR("Failed to destroy processed tags after storage_save");
    }

    return RESULT_OK;
}

result_t agent_actions_save_memory(pool_t* pool, agent_t* agent) {
    string_t* memory_json = NULL;

    if (agent == NULL || agent->data == NULL) {
        return RESULT_OK;
    }

    if (string_create(pool, &memory_json) != RESULT_OK) {
        return RESULT_OK;
    }

    if (object_tostring_json(pool, &memory_json, agent->data) != RESULT_OK) {
        result_t tmp = string_destroy(pool, memory_json);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy memory_json after tostring failure\n");
        }
        return RESULT_OK;
    }

    printf("memory_json: %.*s\n", (int)memory_json->size, memory_json->data);

    if (memory_json->data == NULL || memory_json->size == 0) {
        result_t tmp = string_destroy(pool, memory_json);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy empty memory_json\n");
        }
        return RESULT_OK;
    }

    if (file_write(MEMORY_PATH, memory_json) != RESULT_OK) {
        result_t tmp = string_destroy(pool, memory_json);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy memory_json after file_write failure\n");
        }
        return RESULT_OK;
    }

    if (string_destroy(pool, memory_json) != RESULT_OK) {
        printf("Warning: Failed to destroy memory_json after save\n");
    }
    return RESULT_OK;
}

result_t agent_actions_parse_response(pool_t* pool, const string_t* response_content, object_t** response_obj) {
    if (object_create(pool, response_obj) != RESULT_OK) {
        RETURN_ERR("Failed to create response object");
    }

    object_t* agent_obj = NULL;
    if (object_create(pool, &agent_obj) != RESULT_OK) {
        result_t tmp = object_destroy(pool, *response_obj);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy response_obj after agent object create failure\n");
        }
        RETURN_ERR("Failed to create agent object");
    }

    const char* content = (response_content && response_content->data) ? response_content->data : "";

    string_t* next_state_value = NULL;
    const char* next_state_start = strstr(content, "<next_state>");
    const char* next_state_end = strstr(content, "</next_state>");

    if (next_state_start && next_state_end && next_state_end > next_state_start) {
        next_state_start += strlen("<next_state>");
        size_t state_len = (size_t)(next_state_end - next_state_start);

        if (state_len > 0 && state_len < 64) {
            char state_buffer[65];
            size_t cpy = state_len < sizeof(state_buffer) - 1 ? state_len : sizeof(state_buffer) - 1;
            strncpy(state_buffer, next_state_start, cpy);
            state_buffer[cpy] = '\0';

            if (string_create_str(pool, &next_state_value, state_buffer) == RESULT_OK) {
                string_t* next_state_path = NULL;
                if (string_create_str(pool, &next_state_path, "next_state") == RESULT_OK) {
                    if (object_set_string(pool, agent_obj, next_state_path, next_state_value) != RESULT_OK) {
                        printf("Warning: Failed to set next_state on agent_obj\n");
                    }
                    if (string_destroy(pool, next_state_path) != RESULT_OK) {
                        printf("Warning: Failed to destroy next_state_path\n");
                    }
                }
                if (string_destroy(pool, next_state_value) != RESULT_OK) {
                    printf("Warning: Failed to destroy next_state_value\n");
                }
            }
        }
    }

    const char* eval_log_start = strstr(content, "<evaluation_log>");
    const char* eval_log_end = strstr(content, "</evaluation_log>");

    if (eval_log_start && eval_log_end && eval_log_end > eval_log_start) {
        eval_log_start += strlen("<evaluation_log>");
        size_t log_len = (size_t)(eval_log_end - eval_log_start);

        if (log_len > 0 && log_len < 1024) {
            string_t* eval_log_value = NULL;
            char log_buffer[1025];
            size_t cpy = log_len < sizeof(log_buffer) - 1 ? log_len : sizeof(log_buffer) - 1;
            strncpy(log_buffer, eval_log_start, cpy);
            log_buffer[cpy] = '\0';

            if (string_create_str(pool, &eval_log_value, log_buffer) == RESULT_OK) {
                string_t* eval_log_path = NULL;
                if (string_create_str(pool, &eval_log_path, "evaluation_log") == RESULT_OK) {
                    if (object_set_string(pool, agent_obj, eval_log_path, eval_log_value) != RESULT_OK) {
                        printf("Warning: Failed to set evaluation_log on agent_obj\n");
                    }
                    if (string_destroy(pool, eval_log_path) != RESULT_OK) {
                        printf("Warning: Failed to destroy eval_log_path\n");
                    }
                }
                if (string_destroy(pool, eval_log_value) != RESULT_OK) {
                    printf("Warning: Failed to destroy eval_log_value\n");
                }
            }
        }
    }

    const char* think_log_start = strstr(content, "<think_log>");
    const char* think_log_end = strstr(content, "</think_log>");

    if (think_log_start && think_log_end && think_log_end > think_log_start) {
        think_log_start += strlen("<think_log>");
        size_t log_len = (size_t)(think_log_end - think_log_start);

        if (log_len > 0 && log_len < 1024) {
            string_t* think_log_value = NULL;
            char log_buffer[1025];
            size_t cpy = log_len < sizeof(log_buffer) - 1 ? log_len : sizeof(log_buffer) - 1;
            strncpy(log_buffer, think_log_start, cpy);
            log_buffer[cpy] = '\0';

            if (string_create_str(pool, &think_log_value, log_buffer) == RESULT_OK) {
                string_t* think_log_path = NULL;
                if (string_create_str(pool, &think_log_path, "think_log") == RESULT_OK) {
                    if (object_set_string(pool, agent_obj, think_log_path, think_log_value) != RESULT_OK) {
                        printf("Warning: Failed to set think_log on agent_obj\n");
                    }
                    if (string_destroy(pool, think_log_path) != RESULT_OK) {
                        printf("Warning: Failed to destroy think_log_path\n");
                    }
                }
                if (string_destroy(pool, think_log_value) != RESULT_OK) {
                    printf("Warning: Failed to destroy think_log_value\n");
                }
            }
        }
    }

    // Support <action> and <action ...attributes...>
    const char* action_open = strstr(content, "<action");
    const char* action_end = strstr(content, "</action>");

    if (action_open && action_end && action_end > action_open) {
        // Find end of opening tag '>'
        const char* open_tag_end = strchr(action_open, '>');
        if (!open_tag_end || open_tag_end > action_end) {
            // malformed; skip action parsing
            open_tag_end = NULL;
        }

        // Content inside action is after the '>' of opening tag
        const char* action_start = open_tag_end ? (open_tag_end + 1) : NULL;
        object_t* action_obj2 = NULL;
        if (object_create(pool, &action_obj2) == RESULT_OK) {
            // Extract optional attribute type="..." from the opening tag
            if (open_tag_end) {
                const char* attrs_begin = action_open + strlen("<action");
                const char* attrs_end = open_tag_end;
                const char* type_attr = strstr(attrs_begin, "type=");
                if (type_attr && type_attr < attrs_end) {
                    // Determine quote char '"' or '\''
                    const char* q = strchr(type_attr, '"');
                    char quote = '"';
                    if (!q || q > attrs_end) {
                        q = strchr(type_attr, '\'');
                        quote = '\'';
                    }
                    if (q && q < attrs_end) {
                        const char* val_start = q + 1;
                        const char* val_end = strchr(val_start, quote);
                        if (val_end && val_end <= attrs_end && val_end > val_start) {
                            size_t len = (size_t)(val_end - val_start);
                            if (len > 0 && len <= 1024) {
                                char buf[1025];
                                size_t cpy2 = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
                                strncpy(buf, val_start, cpy2);
                                buf[cpy2] = '\0';
                                string_t* type_val_attr = NULL;
                                if (string_create_str(pool, &type_val_attr, buf) == RESULT_OK) {
                                    string_t* key = NULL;
                                    if (string_create_str(pool, &key, "type") == RESULT_OK) {
                                        if (object_set_string(pool, action_obj2, key, type_val_attr) != RESULT_OK) {
                                            printf("Warning: Failed to set action.type from attribute\n");
                                        }
                                        if (string_destroy(pool, key) != RESULT_OK) {
                                            printf("Warning: Failed to destroy key (type)\n");
                                        }
                                    }
                                    if (string_destroy(pool, type_val_attr) != RESULT_OK) {
                                        printf("Warning: Failed to destroy type_val_attr\n");
                                    }
                                }
                            }
                        }
                    }
                }
            }
            string_t* type_val = NULL;
            if (action_start) {
                char open_tag[32];
                char close_tag[32];
                snprintf(open_tag, sizeof(open_tag), "<%s>", "type");
                snprintf(close_tag, sizeof(close_tag), "</%s>", "type");
                const char* f_start = strstr(action_start, open_tag);
                const char* f_end = strstr(action_start, close_tag);
                if (f_start && f_end && f_end > f_start) {
                    f_start += strlen(open_tag);
                    size_t len = (size_t)(f_end - f_start);
                    if (len > 0 && len <= 1024) {
                        char buf[1025];
                        size_t cpy2 = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
                        strncpy(buf, f_start, cpy2);
                        buf[cpy2] = '\0';
                        if (string_create_str(pool, &type_val, buf) != RESULT_OK) {
                            type_val = NULL;
                        }
                    }
                }
            }

            string_t* tags_val = NULL;
            if (action_start) {
                char open_tag[32];
                char close_tag[32];
                snprintf(open_tag, sizeof(open_tag), "<%s>", "tags");
                snprintf(close_tag, sizeof(close_tag), "</%s>", "tags");
                const char* f_start = strstr(action_start, open_tag);
                const char* f_end = strstr(action_start, close_tag);
                if (f_start && f_end && f_end > f_start) {
                    f_start += strlen(open_tag);
                    size_t len = (size_t)(f_end - f_start);
                    if (len > 0 && len <= 1024) {
                        char buf[1025];
                        size_t cpy2 = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
                        strncpy(buf, f_start, cpy2);
                        buf[cpy2] = '\0';
                        if (string_create_str(pool, &tags_val, buf) != RESULT_OK) {
                            tags_val = NULL;
                        }
                    }
                }
            }

            string_t* value_val = NULL;
            if (action_start) {
                char open_tag[32];
                char close_tag[32];
                snprintf(open_tag, sizeof(open_tag), "<%s>", "value");
                snprintf(close_tag, sizeof(close_tag), "</%s>", "value");
                const char* f_start = strstr(action_start, open_tag);
                const char* f_end = strstr(action_start, close_tag);
                if (f_start && f_end && f_end > f_start) {
                    f_start += strlen(open_tag);
                    size_t len = (size_t)(f_end - f_start);
                    if (len > 0 && len <= 2048) {
                        char buf[2049];
                        size_t cpy2 = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
                        strncpy(buf, f_start, cpy2);
                        buf[cpy2] = '\0';
                        if (string_create_str(pool, &value_val, buf) != RESULT_OK) {
                            value_val = NULL;
                        }
                    }
                }
            }

            if (type_val) {
                string_t* key = NULL;
                if (string_create_str(pool, &key, "type") == RESULT_OK) {
                    if (object_set_string(pool, action_obj2, key, type_val) != RESULT_OK) {
                        printf("Warning: Failed to set action.type\n");
                    }
                    if (string_destroy(pool, key) != RESULT_OK) {
                        printf("Warning: Failed to destroy key (type)\n");
                    }
                }
                if (string_destroy(pool, type_val) != RESULT_OK) {
                    printf("Warning: Failed to destroy type_val\n");
                }
            }
            if (tags_val) {
                string_t* key = NULL;
                if (string_create_str(pool, &key, "tags") == RESULT_OK) {
                    if (object_set_string(pool, action_obj2, key, tags_val) != RESULT_OK) {
                        printf("Warning: Failed to set action.tags\n");
                    }
                    if (string_destroy(pool, key) != RESULT_OK) {
                        printf("Warning: Failed to destroy key (tags)\n");
                    }
                }
                if (string_destroy(pool, tags_val) != RESULT_OK) {
                    printf("Warning: Failed to destroy tags_val\n");
                }
            }
            if (value_val) {
                string_t* key = NULL;
                if (string_create_str(pool, &key, "value") == RESULT_OK) {
                    if (object_set_string(pool, action_obj2, key, value_val) != RESULT_OK) {
                        printf("Warning: Failed to set action.value\n");
                    }
                    if (string_destroy(pool, key) != RESULT_OK) {
                        printf("Warning: Failed to destroy key (value)\n");
                    }
                }
                if (string_destroy(pool, value_val) != RESULT_OK) {
                    printf("Warning: Failed to destroy value_val\n");
                }
            }

            if (action_obj2->child != NULL || action_obj2->string != NULL) {
                string_t* action_path = NULL;
                if (string_create_str(pool, &action_path, "action") == RESULT_OK) {
                    if (object_set(pool, agent_obj, action_path, action_obj2) != RESULT_OK) {
                        printf("Warning: Failed to set agent.action\n");
                    }
                    if (string_destroy(pool, action_path) != RESULT_OK) {
                        printf("Warning: Failed to destroy action_path\n");
                    }
                }
            } else {
                printf("[ACTIONS] Parsed <action> block but found no fields. Content snippet: %.*s\n", 128, response_content && response_content->data ? response_content->data : "");
                if (object_destroy(pool, action_obj2) != RESULT_OK) {
                    printf("Warning: Failed to destroy empty action_obj2\n");
                }
            }
        }
    }

    object_t* check_next_state = NULL;
    if (object_provide_str(pool, &check_next_state, agent_obj, "next_state") != RESULT_OK) {
        string_t* default_state = NULL;
        string_t* state_path = NULL;
        if (string_create_str(pool, &default_state, "thinking") == RESULT_OK &&
            string_create_str(pool, &state_path, "next_state") == RESULT_OK) {
            if (object_set_string(pool, agent_obj, state_path, default_state) != RESULT_OK) {
                printf("Warning: Failed to set default next_state\n");
            }
            if (string_destroy(pool, default_state) != RESULT_OK) {
                printf("Warning: Failed to destroy default_state\n");
            }
            if (string_destroy(pool, state_path) != RESULT_OK) {
                printf("Warning: Failed to destroy state_path\n");
            }
        }
    }

    string_t* agent_path = NULL;
    if (string_create_str(pool, &agent_path, "agent") != RESULT_OK) {
        result_t tmp1 = object_destroy(pool, *response_obj);
        if (tmp1 != RESULT_OK) {
            printf("Warning: Failed to destroy response_obj after agent_path create failure\n");
        }
        result_t tmp2 = object_destroy(pool, agent_obj);
        if (tmp2 != RESULT_OK) {
            printf("Warning: Failed to destroy agent_obj after agent_path create failure\n");
        }
        RETURN_ERR("Failed to create agent path");
    }

    if (object_set(pool, *response_obj, agent_path, agent_obj) != RESULT_OK) {
        if (string_destroy(pool, agent_path) != RESULT_OK) {
            printf("Warning: Failed to destroy agent_path after set failure\n");
        }
        if (object_destroy(pool, *response_obj) != RESULT_OK) {
            printf("Warning: Failed to destroy response_obj after set failure\n");
        }
        if (object_destroy(pool, agent_obj) != RESULT_OK) {
            printf("Warning: Failed to destroy agent_obj after set failure\n");
        }
        RETURN_ERR("Failed to set agent object in response");
    }

    if (string_destroy(pool, agent_path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy agent path");
    }

    return RESULT_OK;
}

result_t agent_actions_extract_action_params(pool_t* pool, object_t* action_obj, object_t** type_obj, object_t** tags_obj, object_t** value_obj) {
    if (object_provide_str(pool, type_obj, action_obj, "type") != RESULT_OK) {
        printf("[ACTIONS] Missing action.type in parsed response.\n");
        RETURN_ERR("Failed to extract action type");
    }

    if (object_provide_str(pool, tags_obj, action_obj, "tags") != RESULT_OK) {
        printf("[ACTIONS] Missing action.tags in parsed response.\n");
        RETURN_ERR("Failed to extract action tags");
    }

    if (object_provide_str(pool, value_obj, action_obj, "value") != RESULT_OK) {
        // optional; log only when required later
        *value_obj = NULL;
    }

    return RESULT_OK;
}

result_t agent_actions_validate_action_params(object_t* type_obj, object_t* tags_obj, object_t* value_obj, const char* expected_type, uint64_t value_required) {
    if (type_obj == NULL || type_obj->string == NULL) {
        RETURN_ERR("Action type is NULL or invalid");
    }

    if (!string_equal_str(type_obj->string, expected_type)) {
        RETURN_ERR("Action type does not match expected type");
    }

    if (tags_obj == NULL || tags_obj->string == NULL) {
        RETURN_ERR("Action tags are NULL or invalid");
    }

    if (tags_obj->string->size == 0) {
        RETURN_ERR("Action tags cannot be empty");
    }

    if (value_required) {
        if (value_obj == NULL || value_obj->string == NULL) {
            RETURN_ERR("Action value is required but not provided");
        }
    }

    return RESULT_OK;
}

result_t agent_actions_process_tags(pool_t* pool, object_t* tags_obj, string_t** processed_tags) {
    if (string_create_string(pool, processed_tags, tags_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to create copy of tags string");
    }

    for (uint64_t i = 0; i < (*processed_tags)->size; i++) {
        if ((*processed_tags)->data[i] == ' ') {
            (*processed_tags)->data[i] = '_';
        }
    }

    return RESULT_OK;
}

static int cmp_string_t_ptrs(const void* a, const void* b) {
    const string_t* const* pa = (const string_t* const*)a;
    const string_t* const* pb = (const string_t* const*)b;
    const string_t* sa = *pa;
    const string_t* sb = *pb;
    size_t minlen = sa->size < sb->size ? sa->size : sb->size;
    int c = memcmp(sa->data, sb->data, minlen);
    if (c != 0) return c;
    if (sa->size < sb->size) return -1;
    if (sa->size > sb->size) return 1;
    return 0;
}

// Helpers to destroy strings while logging failures and honoring warn_unused_result
static void destroy_string_with_warning(pool_t* pool, string_t* s, const char* context) {
    if (s) {
        result_t r = string_destroy(pool, s);
        if (r != RESULT_OK) {
            printf("Warning: Failed to destroy string (%s)\n", context ? context : "");
        }
    }
}

static void destroy_string_array_with_warning(pool_t* pool, string_t** arr, size_t count, const char* context) {
    if (!arr) return;
    for (size_t i = 0; i < count; i++) {
        if (arr[i]) {
            result_t r = string_destroy(pool, arr[i]);
            if (r != RESULT_OK) {
                printf("Warning: Failed to destroy string at index %zu (%s)\n", i, context ? context : "");
            }
        }
    }
}

result_t agent_actions_normalize_storage_tags(pool_t* pool, object_t* tags_obj, string_t** processed_tags) {
    if (!tags_obj || !tags_obj->string || !tags_obj->string->data) {
        RETURN_ERR("Invalid tags object");
    }

    // Collect normalized tokens into pool-backed strings
    enum { MAX_TAGS = 64 };
    string_t* parts[MAX_TAGS];
    size_t count = 0;

    const char* s = tags_obj->string->data;
    size_t n = tags_obj->string->size;

    size_t i = 0;
    while (i < n && count < MAX_TAGS) {
        size_t j = i;
        while (j < n && s[j] != ',') j++;
        size_t start = i;
        while (start < j && (s[start] == ' ' || s[start] == '\t' || s[start] == '\n' || s[start] == '\r')) start++;
        size_t end = j;
        while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\n' || s[end - 1] == '\r')) end--;

        if (end > start) {
            string_t* token = NULL;
            if (string_create(pool, &token) != RESULT_OK) {
                // cleanup created parts
                destroy_string_array_with_warning(pool, parts, count, "token create failure cleanup");
                RETURN_ERR("Failed to create token string");
            }

            for (size_t k = start; k < end; k++) {
                char c = s[k];
                if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
                if (c == ' ') c = '_';
                if (string_append_char(pool, &token, c) != RESULT_OK) {
                    destroy_string_array_with_warning(pool, parts, count, "append token failure cleanup");
                    destroy_string_with_warning(pool, token, "append token failure cleanup");
                    RETURN_ERR("Failed to append to token");
                }
            }

            if (token->size > 0) {
                parts[count++] = token;
            } else {
                destroy_string_with_warning(pool, token, "drop empty token");
            }
        }
        i = (j < n) ? (j + 1) : j;
    }

    if (count == 0) {
        // Fallback to underscore-normalized whole string
        if (string_create_string(pool, processed_tags, tags_obj->string) != RESULT_OK) {
            RETURN_ERR("Failed to create fallback tags string");
        }
        for (uint64_t k = 0; k < (*processed_tags)->size; k++) if ((*processed_tags)->data[k] == ' ') (*processed_tags)->data[k] = '_';
        return RESULT_OK;
    }

    // Sort and deduplicate
    qsort(parts, count, sizeof(string_t*), cmp_string_t_ptrs);
    size_t uniq = 0;
    for (size_t k = 0; k < count; k++) {
        if (uniq == 0) {
            parts[uniq++] = parts[k];
        } else {
            string_t* a = parts[uniq - 1];
            string_t* b = parts[k];
            if (!(a->size == b->size && memcmp(a->data, b->data, a->size) == 0)) {
                parts[uniq++] = parts[k];
            } else {
                // drop duplicate
                if(string_destroy(pool, parts[k]) != RESULT_OK){
                    printf("Warning: Failed to destroy duplicate string\n");
                }
            }
        }
    }
    count = uniq;

    if (string_create(pool, processed_tags) != RESULT_OK) {
        destroy_string_array_with_warning(pool, parts, count, "create processed_tags failure cleanup");
        RETURN_ERR("Failed to create processed tags");
    }

    for (size_t k = 0; k < count; k++) {
        if (k > 0) {
            if (string_append_char(pool, processed_tags, ',') != RESULT_OK) {
                destroy_string_array_with_warning(pool, parts, count, "append comma failure cleanup");
                RETURN_ERR("Failed to append comma");
            }
        }
        if (string_append_string(pool, processed_tags, parts[k]) != RESULT_OK) {
            destroy_string_array_with_warning(pool, parts, count, "append tag failure cleanup");
            RETURN_ERR("Failed to append tag");
        }
    }

    // cleanup tokens
    destroy_string_array_with_warning(pool, parts, count, "final tokens cleanup");
    return RESULT_OK;
}

result_t agent_actions_get_working_memory(pool_t* pool, agent_t* agent, object_t** working_memory) {
    if (object_provide_str(pool, working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory from agent");
    }

    return RESULT_OK;
}

result_t agent_actions_get_storage(pool_t* pool, agent_t* agent, object_t** storage) {
    if (object_provide_str(pool, storage, agent->data, "storage") != RESULT_OK) {
        RETURN_ERR("Failed to get storage from agent");
    }

    return RESULT_OK;
}

result_t agent_actions_ensure_storage_exists(pool_t* pool, agent_t* agent) {
    object_t* storage = NULL;

    if (object_provide_str(pool, &storage, agent->data, "storage") == RESULT_OK) {
        // If storage exists but is not a container (e.g., "null" or a plain string), replace it with an empty object
        if (storage->child != NULL) {
            return RESULT_OK; // already a container
        }

        // Treat any non-container (including string "null") as uninitialized
        object_t* new_storage2 = NULL;
        if (object_create(pool, &new_storage2) != RESULT_OK) {
            RETURN_ERR("Failed to create replacement storage object");
        }

        string_t* storage_path2 = NULL;
        if (string_create_str(pool, &storage_path2, "storage") != RESULT_OK) {
            if (object_destroy(pool, new_storage2) != RESULT_OK) {
                printf("Warning: Failed to destroy new_storage2 after storage_path create failure\n");
            }
            RETURN_ERR("Failed to create storage path string (replacement)");
        }

        if (object_set(pool, agent->data, storage_path2, new_storage2) != RESULT_OK) {
            if (string_destroy(pool, storage_path2) != RESULT_OK) {
                printf("Warning: Failed to destroy storage_path2 after set failure\n");
            }
            if (object_destroy(pool, new_storage2) != RESULT_OK) {
                printf("Warning: Failed to destroy new_storage2 after set failure\n");
            }
            RETURN_ERR("Failed to replace non-container storage with object");
        }

        if (string_destroy(pool, storage_path2) != RESULT_OK) {
            RETURN_ERR("Failed to destroy storage path string (replacement)");
        }

        return RESULT_OK;
    }

    object_t* new_storage = NULL;
    if (object_create(pool, &new_storage) != RESULT_OK) {
        RETURN_ERR("Failed to create new storage object");
    }

    string_t* storage_path = NULL;
    if (string_create_str(pool, &storage_path, "storage") != RESULT_OK) {
        if (object_destroy(pool, new_storage) != RESULT_OK) {
            printf("Warning: Failed to destroy new_storage after storage_path create failure\n");
        }
        RETURN_ERR("Failed to create storage path string");
    }

    if (object_set(pool, agent->data, storage_path, new_storage) != RESULT_OK) {
        if (string_destroy(pool, storage_path) != RESULT_OK) {
            printf("Warning: Failed to destroy storage_path after set failure\n");
        }
        if (object_destroy(pool, new_storage) != RESULT_OK) {
            printf("Warning: Failed to destroy new_storage after set failure\n");
        }
        RETURN_ERR("Failed to set storage object in agent data");
    }

    if (string_destroy(pool, storage_path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy storage path string");
    }

    return RESULT_OK;
}

result_t agent_actions_ensure_working_memory_exists(pool_t* pool, agent_t* agent) {
    object_t* working_memory = NULL;

    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") == RESULT_OK) {
        return RESULT_OK;
    }

    object_t* new_working_memory = NULL;
    if (object_create(pool, &new_working_memory) != RESULT_OK) {
        RETURN_ERR("Failed to create new working memory object");
    }

    string_t* working_memory_path = NULL;
    if (string_create_str(pool, &working_memory_path, "working_memory") != RESULT_OK) {
        if (object_destroy(pool, new_working_memory) != RESULT_OK) {
            printf("Warning: Failed to destroy new_working_memory after path create failure\n");
        }
        RETURN_ERR("Failed to create working memory path string");
    }

    if (object_set(pool, agent->data, working_memory_path, new_working_memory) != RESULT_OK) {
        if (string_destroy(pool, working_memory_path) != RESULT_OK) {
            printf("Warning: Failed to destroy working_memory_path after set failure\n");
        }
        if (object_destroy(pool, new_working_memory) != RESULT_OK) {
            printf("Warning: Failed to destroy new_working_memory after set failure\n");
        }
        RETURN_ERR("Failed to set working memory object in agent data");
    }

    if (string_destroy(pool, working_memory_path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy working memory path string");
    }

    return RESULT_OK;
}

result_t agent_actions_log_result(pool_t* pool, config_t* config, agent_t* agent, const char* action_type, const char* tags, const char* result_message) {
    if (agent_actions_ensure_working_memory_exists(pool, agent) != RESULT_OK) {
        printf("Warning: Failed to ensure working memory exists before logging\n");
    }

    if (agent_state_manage_command_log(pool, config, agent, action_type, tags, result_message) != RESULT_OK) {
        printf("Warning: Failed to manage command log\n");
    }

    return RESULT_OK;
}

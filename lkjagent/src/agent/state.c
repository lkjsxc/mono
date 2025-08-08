#include "agent/state.h"

// Validate that a state name exists under config.agent.state
static result_t config_has_state(pool_t* pool, config_t* config, const string_t* state_name, uint64_t* exists) {
    object_t* state_root = NULL;
    object_t* state_def = NULL;
    if (!exists) {
        RETURN_ERR("exists out param is NULL");
    }
    *exists = 0;
    if (!pool || !config || !config->data || !state_name || !state_name->data || state_name->size == 0) {
        return RESULT_OK; // treat as not existing
    }
    if (object_provide_str(pool, &state_root, config->data, "agent.state") != RESULT_OK || !state_root) {
        return RESULT_OK;
    }
    if (object_provide_string(&state_def, state_root, state_name) == RESULT_OK && state_def) {
        *exists = 1;
    }
    return RESULT_OK;
}

// Log invalid next_state into working_memory and via execution log; proceed without failing
static void log_invalid_next_state(pool_t* pool, config_t* config, agent_t* agent, const char* requested_state) {
    if (!requested_state) requested_state = "(null)";
    // Execution log entry (best-effort)
    (void)agent_state_manage_execution_log(pool, config, agent, "state_transition", requested_state, "Invalid next_state in config; defaulting to thinking");

    // Also set a lightweight key in working_memory
    object_t* working_memory = NULL;
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") == RESULT_OK && working_memory) {
        string_t* key = NULL;
        string_t* val = NULL;
        if (string_create_str(pool, &key, "state_transition_invalid") == RESULT_OK) {
            char buf[256];
            snprintf(buf, sizeof(buf), "requested='%s', fallback='thinking'", requested_state);
            if (string_create_str(pool, &val, buf) == RESULT_OK) {
                if (object_set_string(pool, working_memory, key, val) != RESULT_OK) {
                    // best-effort, ignore
                }
                if (string_destroy(pool, val) != RESULT_OK) {
                    printf("Warning: Failed to destroy state invalid log value\n");
                }
            }
            if (string_destroy(pool, key) != RESULT_OK) {
                printf("Warning: Failed to destroy state invalid log key\n");
            }
        }
    }
}

static result_t get_config_bool(pool_t* pool, config_t* config, const char* path, uint64_t* result) {
    object_t* config_obj = NULL;

    *result = 0;

    if (object_provide_str(pool, &config_obj, config->data, path) != RESULT_OK) {
        RETURN_ERR("Failed to provide config object");
    }

    if (config_obj == NULL || config_obj->string == NULL || config_obj->string->data == NULL) {
        RETURN_ERR("Invalid config object");
    }

    if (string_equal_str(config_obj->string, "true")) {
        *result = 1;
    } else {
        char temp_str[32];
        size_t copy_size = config_obj->string->size < sizeof(temp_str) - 1 ?
                          config_obj->string->size : sizeof(temp_str) - 1;
        memcpy(temp_str, config_obj->string->data, copy_size);
        temp_str[copy_size] = '\0';

        uint64_t num_val = strtoull(temp_str, NULL, 10);
        *result = (num_val != 0) ? 1 : 0;
    }

    return RESULT_OK;
}

static result_t get_config_uint64(pool_t* pool, config_t* config, const char* path, uint64_t* result, uint64_t default_value) {
    object_t* config_obj = NULL;

    *result = default_value;

    if (config == NULL || config->data == NULL) {
        return RESULT_OK;
    }

    if (object_provide_str(pool, &config_obj, config->data, path) != RESULT_OK) {
        return RESULT_OK;
    }

    if (config_obj == NULL || config_obj->string == NULL || config_obj->string->data == NULL) {
        return RESULT_OK;
    }

    char temp_str[32];
    size_t copy_size = config_obj->string->size < sizeof(temp_str) - 1 ?
                      config_obj->string->size : sizeof(temp_str) - 1;
    memcpy(temp_str, config_obj->string->data, copy_size);
    temp_str[copy_size] = '\0';

    uint64_t num_val = strtoull(temp_str, NULL, 10);
    if (num_val > 0) {
        *result = num_val;
    }

    return RESULT_OK;
}

static result_t get_config_string(pool_t* pool, config_t* config, const char* path, char* result, size_t result_size, const char* default_value) {
    object_t* config_obj = NULL;

    strncpy(result, default_value, result_size - 1);
    result[result_size - 1] = '\0';

    if (config == NULL || config->data == NULL) {
        return RESULT_OK;
    }

    if (object_provide_str(pool, &config_obj, config->data, path) != RESULT_OK) {
        return RESULT_OK;
    }

    if (config_obj == NULL || config_obj->string == NULL || config_obj->string->data == NULL) {
        return RESULT_OK;
    }

    size_t copy_size = config_obj->string->size < result_size - 1 ?
                      config_obj->string->size : result_size - 1;
    memcpy(result, config_obj->string->data, copy_size);
    result[copy_size] = '\0';

    return RESULT_OK;
}

result_t agent_state_auto_transition(pool_t* pool, config_t* config, agent_t* agent) {
    (void)config;

    if (agent_state_update_state(pool, agent, "evaluating") != RESULT_OK) {
        RETURN_ERR("Failed to transition to evaluating state");
    }

    return RESULT_OK;
}

result_t agent_state_update_and_log(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj) {
    object_t* next_state_obj = NULL;
    uint64_t successful_operations = 0;

    if (pool == NULL || agent == NULL || response_obj == NULL) {
        RETURN_ERR("Invalid parameters for state update and log");
    }

    if (agent_state_extract_next_state(pool, response_obj, &next_state_obj) == RESULT_OK && next_state_obj != NULL) {
        string_t* state_path = NULL;
        if (string_create_str(pool, &state_path, "state") != RESULT_OK) {
            RETURN_ERR("Failed to create state path string");
        }

        uint64_t exists = 0;
        if (config_has_state(pool, config, next_state_obj->string, &exists) != RESULT_OK) {
            exists = 0; // be conservative
        }

        if (!exists) {
            // Log and default to thinking
            log_invalid_next_state(pool, config, agent, next_state_obj->string ? next_state_obj->string->data : "");
            if (string_destroy(pool, state_path) != RESULT_OK) {
                RETURN_ERR("Failed to destroy state path string");
            }
            if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
                RETURN_ERR("Failed to default to thinking after invalid next_state");
            }
        } else if (object_set_string(pool, agent->data, state_path, next_state_obj->string) != RESULT_OK) {
            result_t tmp = string_destroy(pool, state_path);
            if (tmp != RESULT_OK) {
                printf("Warning: Failed to destroy state_path after failed state update\n");
            }
            RETURN_ERR("Failed to update agent state");
        }

        if (exists) {
            if (string_destroy(pool, state_path) != RESULT_OK) {
                RETURN_ERR("Failed to destroy state path string");
            }
        }

        successful_operations++;
    }

    object_t* thinking_log_obj = NULL;
    if (object_provide_str(pool, &thinking_log_obj, response_obj, "thinking_log") == RESULT_OK) {
        uint64_t thinking_log_enabled = 0;
        if (get_config_bool(pool, config, "agent.thinking_log.enable", &thinking_log_enabled) == RESULT_OK && thinking_log_enabled) {
            result_t thinking_log_result = agent_state_manage_thinking_log(pool, config, agent, response_obj);
            if (thinking_log_result == RESULT_OK) {
                successful_operations++;
            }
        }
    }

    object_t* evaluation_log_obj = NULL;
    if (object_provide_str(pool, &evaluation_log_obj, response_obj, "evaluation_log") == RESULT_OK) {
        uint64_t evaluation_log_enabled = 0;
        if (get_config_bool(pool, config, "agent.evaluation_log.enable", &evaluation_log_enabled) == RESULT_OK && evaluation_log_enabled) {
            result_t evaluation_log_result = agent_state_manage_evaluation_log(pool, config, agent, response_obj);
            if (evaluation_log_result == RESULT_OK) {
                successful_operations++;
            }
        }
    }

    if (successful_operations == 0) {
        if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
            RETURN_ERR("Failed to reset agent state to thinking after no successful operations");
        }
    }

    return RESULT_OK;
}

result_t agent_state_handle_evaluation_transition(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj) {
    uint64_t requires_paging = 0;

    if (pool == NULL || config == NULL || agent == NULL || response_obj == NULL) {
        RETURN_ERR("Invalid parameters for evaluation transition");
    }

    if (agent_state_check_memory_limits(pool, config, agent, &requires_paging) != RESULT_OK) {
        RETURN_ERR("Failed to check memory limits");
    }

    uint64_t evaluation_log_enabled = 0;
    if (get_config_bool(pool, config, "agent.evaluation_log.enable", &evaluation_log_enabled) == RESULT_OK && evaluation_log_enabled) {
        if (agent_state_manage_evaluation_log(pool, config, agent, response_obj) != RESULT_OK) {
            RETURN_ERR("Failed to manage evaluation log");
        }
    }

    if (requires_paging) {
        if (agent_state_update_state(pool, agent, "paging") != RESULT_OK) {
            RETURN_ERR("Failed to transition to paging state");
        }

        if (agent_state_execute_paging(pool, config, agent) != RESULT_OK) {
            RETURN_ERR("Failed to execute paging operation");
        }

        if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
            RETURN_ERR("Failed to transition back to thinking after paging");
        }
    } else {
        object_t* next_state_obj = NULL;
        if (agent_state_extract_next_state(pool, response_obj, &next_state_obj) == RESULT_OK && next_state_obj != NULL) {
            string_t* state_path = NULL;
            if (string_create_str(pool, &state_path, "state") != RESULT_OK) {
                RETURN_ERR("Failed to create state path string for evaluation transition");
            }

            uint64_t exists = 0;
            if (config_has_state(pool, config, next_state_obj->string, &exists) != RESULT_OK) {
                exists = 0;
            }

            if (!exists) {
                log_invalid_next_state(pool, config, agent, next_state_obj->string ? next_state_obj->string->data : "");
                result_t tmp = string_destroy(pool, state_path);
                if (tmp != RESULT_OK) {
                    printf("Warning: Failed to destroy state_path after invalid next_state (evaluation)\n");
                }
                if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
                    RETURN_ERR("Failed to default to thinking after invalid next_state (evaluation)");
                }
            } else if (object_set_string(pool, agent->data, state_path, next_state_obj->string) != RESULT_OK) {
                result_t tmp = string_destroy(pool, state_path);
                if (tmp != RESULT_OK) {
                    printf("Warning: Failed to destroy state_path after failed state update (evaluation)\n");
                }
                RETURN_ERR("Failed to update agent state from response");
            }

            if (exists) {
                if (string_destroy(pool, state_path) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy state path string");
                }
            }
        } else {
            if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
                RETURN_ERR("Failed to default to thinking state");
            }
        }
    }

    return RESULT_OK;
}

result_t agent_state_estimate_tokens(pool_t* pool, agent_t* agent, uint64_t* token_count) {
    string_t* memory_string = NULL;
    object_t* working_memory = NULL;

    if (pool == NULL || agent == NULL || token_count == NULL) {
        RETURN_ERR("Invalid parameters for token estimation");
    }

    *token_count = 0;

    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory for token estimation");
    }

    if (string_create(pool, &memory_string) != RESULT_OK) {
        RETURN_ERR("Failed to create string for token estimation");
    }

    if (object_tostring_json(pool, &memory_string, working_memory) != RESULT_OK) {
        result_t tmp = string_destroy(pool, memory_string);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy memory_string after JSON conversion error\n");
        }
        RETURN_ERR("Failed to convert working memory to JSON for token estimation");
    }

    *token_count = memory_string->size / 4;

    if (string_destroy(pool, memory_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy memory string after token estimation");
    }

    return RESULT_OK;
}

result_t agent_state_extract_next_state(pool_t* pool, object_t* response_obj, object_t** next_state_obj) {
    if (pool == NULL || response_obj == NULL || next_state_obj == NULL) {
        RETURN_ERR("Invalid parameters for next state extraction");
    }

    if (object_provide_str(pool, next_state_obj, response_obj, "next_state") != RESULT_OK) {
        *next_state_obj = NULL;
        return RESULT_ERR;
    }

    if (*next_state_obj == NULL || (*next_state_obj)->string == NULL) {
        RETURN_ERR("Next state object is invalid");
    }

    return RESULT_OK;
}

result_t agent_state_update_state(pool_t* pool, agent_t* agent, const char* new_state) {
    string_t* state_string = NULL;
    string_t* state_path = NULL;

    if (pool == NULL || agent == NULL || new_state == NULL) {
        RETURN_ERR("Invalid parameters for state update");
    }

    if (string_create_str(pool, &state_string, new_state) != RESULT_OK) {
        RETURN_ERR("Failed to create state string");
    }

    if (string_create_str(pool, &state_path, "state") != RESULT_OK) {
        result_t tmp = string_destroy(pool, state_string);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy state_string after state_path create error\n");
        }
        RETURN_ERR("Failed to create state path string");
    }

    if (object_set_string(pool, agent->data, state_path, state_string) != RESULT_OK) {
        result_t tmp1 = string_destroy(pool, state_string);
        if (tmp1 != RESULT_OK) {
            printf("Warning: Failed to destroy state_string after set error\n");
        }
        result_t tmp2 = string_destroy(pool, state_path);
        if (tmp2 != RESULT_OK) {
            printf("Warning: Failed to destroy state_path after set error\n");
        }
        RETURN_ERR("Failed to set agent state");
    }

    if (string_destroy(pool, state_string) != RESULT_OK ||
        string_destroy(pool, state_path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy strings after state update");
    }

    return RESULT_OK;
}

result_t agent_state_manage_thinking_log(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj) {
    object_t* thinking_log_obj = NULL;
    uint64_t max_entries = 10;
    char key_prefix[32] = "thinking_log_";
    uint64_t enable = 1;

    if (pool == NULL || agent == NULL || response_obj == NULL) {
        RETURN_ERR("Invalid parameters for thinking log management");
    }

    if (get_config_bool(pool, config, "agent.thinking_log.enable", &enable) != RESULT_OK || !enable) {
        return RESULT_OK;
    }

    if (object_provide_str(pool, &thinking_log_obj, response_obj, "thinking_log") != RESULT_OK) {
        return RESULT_OK;
    }

    if (thinking_log_obj == NULL || thinking_log_obj->string == NULL) {
        return RESULT_OK;
    }

    get_config_uint64(pool, config, "agent.thinking_log.max_entries", &max_entries, 10);
    get_config_string(pool, config, "agent.thinking_log.key_prefix", key_prefix, sizeof(key_prefix), "thinking_log_");

    char log_key[64];
    uint64_t next_index = 1;

    object_t* working_memory = NULL;
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory for thinking log management");
    }

    for (uint64_t i = 1; i <= max_entries; i++) {
        snprintf(log_key, sizeof(log_key), "%s%03lu", key_prefix, (unsigned long)i);
        object_t* existing_log = NULL;
        if (object_provide_str(pool, &existing_log, working_memory, log_key) == RESULT_OK &&
            existing_log != NULL && existing_log->string != NULL && existing_log->string->size > 0) {
            next_index = i + 1;
        }
    }

    if (next_index > max_entries) {
        for (uint64_t i = 1; i < max_entries; i++) {
            char old_log_key[64];
            char new_log_key[64];
            snprintf(old_log_key, sizeof(old_log_key), "%s%03lu", key_prefix, (unsigned long)(i + 1));
            snprintf(new_log_key, sizeof(new_log_key), "%s%03lu", key_prefix, (unsigned long)i);

            object_t* log_to_move = NULL;
            if (object_provide_str(pool, &log_to_move, working_memory, old_log_key) == RESULT_OK &&
                log_to_move != NULL && log_to_move->string != NULL) {
                string_t* new_key_string = NULL;
                if (string_create_str(pool, &new_key_string, new_log_key) == RESULT_OK) {
                    if (object_set_string(pool, working_memory, new_key_string, log_to_move->string) != RESULT_OK) {
                        printf("Error: Failed to rotate thinking log in working memory\n");
                    }
                    result_t tmp = string_destroy(pool, new_key_string);
                    if (tmp != RESULT_OK) {
                        printf("Warning: Failed to destroy new_key_string after thinking log rotation\n");
                    }
                }
            }
        }
        next_index = max_entries;
    }

    snprintf(log_key, sizeof(log_key), "%s%03lu", key_prefix, (unsigned long)next_index);
    string_t* log_key_string = NULL;
    if (string_create_str(pool, &log_key_string, log_key) != RESULT_OK) {
        return RESULT_OK;
    }

    if (object_set_string(pool, working_memory, log_key_string, thinking_log_obj->string) != RESULT_OK) {
        printf("Error: Failed to add thinking log to working memory\n");
    }

    {
        result_t tmp = string_destroy(pool, log_key_string);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy log_key_string after adding thinking log\n");
        }
    }
    return RESULT_OK;
}

result_t agent_state_manage_evaluation_log(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj) {
    object_t* evaluation_log_obj = NULL;
    uint64_t max_entries = 10;
    char key_prefix[32] = "evaluation_log_";
    uint64_t enable = 1;

    if (pool == NULL || agent == NULL || response_obj == NULL) {
        RETURN_ERR("Invalid parameters for evaluation log management");
    }

    if (get_config_bool(pool, config, "agent.evaluation_log.enable", &enable) != RESULT_OK || !enable) {
        return RESULT_OK;
    }

    if (object_provide_str(pool, &evaluation_log_obj, response_obj, "evaluation_log") != RESULT_OK) {
        return RESULT_OK;
    }

    if (evaluation_log_obj == NULL || evaluation_log_obj->string == NULL) {
        return RESULT_OK;
    }

    get_config_uint64(pool, config, "agent.evaluation_log.max_entries", &max_entries, 10);
    get_config_string(pool, config, "agent.evaluation_log.key_prefix", key_prefix, sizeof(key_prefix), "evaluation_log_");

    char log_key[64];
    uint64_t next_index = 1;

    object_t* working_memory = NULL;
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory for evaluation log management");
    }

    for (uint64_t i = 1; i <= max_entries; i++) {
        snprintf(log_key, sizeof(log_key), "%s%03lu", key_prefix, (unsigned long)i);
        object_t* existing_log = NULL;
        if (object_provide_str(pool, &existing_log, working_memory, log_key) == RESULT_OK &&
            existing_log != NULL && existing_log->string != NULL && existing_log->string->size > 0) {
            next_index = i + 1;
        }
    }

    if (next_index > max_entries) {
        for (uint64_t i = 1; i < max_entries; i++) {
            char old_log_key[64];
            char new_log_key[64];
            snprintf(old_log_key, sizeof(old_log_key), "%s%03lu", key_prefix, (unsigned long)(i + 1));
            snprintf(new_log_key, sizeof(new_log_key), "%s%03lu", key_prefix, (unsigned long)i);

            object_t* old_log_obj = NULL;
            if (object_provide_str(pool, &old_log_obj, working_memory, old_log_key) == RESULT_OK &&
                old_log_obj != NULL && old_log_obj->string != NULL) {
                string_t* new_log_key_string = NULL;
                if (string_create_str(pool, &new_log_key_string, new_log_key) == RESULT_OK) {
                    if (object_set_string(pool, working_memory, new_log_key_string, old_log_obj->string) != RESULT_OK) {
                        printf("Error: Failed to rotate evaluation log %s to %s\n", old_log_key, new_log_key);
                    }
                    result_t tmp = string_destroy(pool, new_log_key_string);
                    if (tmp != RESULT_OK) {
                        printf("Warning: Failed to destroy new_log_key_string after evaluation log rotation\n");
                    }
                }
            }
        }
        next_index = max_entries;
    }

    snprintf(log_key, sizeof(log_key), "%s%03lu", key_prefix, (unsigned long)next_index);
    string_t* log_key_string = NULL;
    if (string_create_str(pool, &log_key_string, log_key) != RESULT_OK) {
        RETURN_ERR("Failed to create evaluation log key string");
    }

    object_t* working_memory2 = NULL;
    if (object_provide_str(pool, &working_memory2, agent->data, "working_memory") != RESULT_OK) {
        result_t tmp = string_destroy(pool, log_key_string);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy log_key_string after working memory acquire error\n");
        }
        RETURN_ERR("Failed to get working memory for evaluation log set");
    }

    if (object_set_string(pool, working_memory2, log_key_string, evaluation_log_obj->string) != RESULT_OK) {
        result_t tmp = string_destroy(pool, log_key_string);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy log_key_string after evaluation log set error\n");
        }
        RETURN_ERR("Failed to add evaluation log to working memory");
    }

    if (string_destroy(pool, log_key_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy evaluation log key string");
    }

    return RESULT_OK;
}

result_t agent_state_manage_execution_log(pool_t* pool, config_t* config, agent_t* agent, const char* action_type, const char* tags, const char* result_message) {
    uint64_t max_entries = 4;
    char key_prefix[32] = "execution_log_";
    uint64_t enable = 1;

    if (pool == NULL || agent == NULL) {
        RETURN_ERR("Invalid parameters for execution log management");
    }

    if (get_config_bool(pool, config, "agent.execution_log.enable", &enable) != RESULT_OK || !enable) {
        return RESULT_OK;
    }

    get_config_uint64(pool, config, "agent.execution_log.max_entries", &max_entries, 4);
    get_config_string(pool, config, "agent.execution_log.key_prefix", key_prefix, sizeof(key_prefix), "execution_log_");

    char execution_log_buffer[512];
    snprintf(execution_log_buffer, sizeof(execution_log_buffer),
             "Action: %s, Tags: %s, Result: %s",
             action_type ? action_type : "unknown",
             tags ? tags : "none",
             result_message ? result_message : "no result");

    char log_key[64];
    uint64_t next_index = 1;

    object_t* working_memory = NULL;
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory for execution log management");
    }

    for (uint64_t i = 1; i <= max_entries; i++) {
        snprintf(log_key, sizeof(log_key), "%s%03lu", key_prefix, (unsigned long)i);
        object_t* existing_log = NULL;
        if (object_provide_str(pool, &existing_log, working_memory, log_key) == RESULT_OK &&
            existing_log != NULL && existing_log->string != NULL && existing_log->string->size > 0) {
            next_index = i + 1;
        }
    }

    if (next_index > max_entries) {
        for (uint64_t i = 1; i < max_entries; i++) {
            char old_log_key[64];
            char new_log_key[64];
            snprintf(old_log_key, sizeof(old_log_key), "%s%03lu", key_prefix, (unsigned long)(i + 1));
            snprintf(new_log_key, sizeof(new_log_key), "%s%03lu", key_prefix, (unsigned long)i);

            object_t* old_log_obj = NULL;
            if (object_provide_str(pool, &old_log_obj, working_memory, old_log_key) == RESULT_OK &&
                old_log_obj != NULL && old_log_obj->string != NULL) {
                string_t* new_log_key_string = NULL;
                if (string_create_str(pool, &new_log_key_string, new_log_key) == RESULT_OK) {
                    if (object_set_string(pool, working_memory, new_log_key_string, old_log_obj->string) != RESULT_OK) {
                        printf("Error: Failed to rotate execution log %s to %s\n", old_log_key, new_log_key);
                    }
                    result_t tmp = string_destroy(pool, new_log_key_string);
                    if (tmp != RESULT_OK) {
                        printf("Warning: Failed to destroy new_log_key_string after execution log rotation\n");
                    }
                }
            }
        }
        next_index = max_entries;
    }

    snprintf(log_key, sizeof(log_key), "%s%03lu", key_prefix, (unsigned long)next_index);
    string_t* log_key_string = NULL;
    string_t* log_value_string = NULL;

    if (string_create_str(pool, &log_key_string, log_key) != RESULT_OK) {
        return RESULT_OK;
    }

    if (string_create_str(pool, &log_value_string, execution_log_buffer) != RESULT_OK) {
        result_t tmp = string_destroy(pool, log_key_string);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy log_key_string after log_value_string create error\n");
        }
        return RESULT_OK;
    }

    if (object_set_string(pool, working_memory, log_key_string, log_value_string) != RESULT_OK) {
        printf("Error: Failed to add execution log to working memory\n");
    }

    {
        result_t tmp1 = string_destroy(pool, log_key_string);
        if (tmp1 != RESULT_OK) {
            printf("Warning: Failed to destroy log_key_string after execution log write\n");
        }
        result_t tmp2 = string_destroy(pool, log_value_string);
        if (tmp2 != RESULT_OK) {
            printf("Warning: Failed to destroy log_value_string after execution log write\n");
        }
    }

    return RESULT_OK;
}

result_t agent_state_check_memory_limits(pool_t* pool, config_t* config, agent_t* agent, uint64_t* requires_paging) {
    uint64_t token_count = 0;
    uint64_t paging_limit = 1024;
    uint64_t enable = 1;

    if (pool == NULL || agent == NULL || requires_paging == NULL) {
        RETURN_ERR("Invalid parameters for memory limit check");
    }

    *requires_paging = 0;

    if (get_config_bool(pool, config, "agent.paging_limit.enable", &enable) != RESULT_OK || !enable) {
        return RESULT_OK;
    }

    if (agent_state_estimate_tokens(pool, agent, &token_count) != RESULT_OK) {
        RETURN_ERR("Failed to estimate token count");
    }

    get_config_uint64(pool, config, "agent.paging_limit.max_tokens", &paging_limit, 1024);

    if (token_count >= paging_limit) {
        *requires_paging = 1;
    }

    return RESULT_OK;
}

result_t agent_state_execute_paging(pool_t* pool, config_t* config, agent_t* agent) {
    (void)pool;
    (void)config;
    (void)agent;

    printf("Paging operation executed (placeholder implementation)\n");

    return RESULT_OK;
}

result_t agent_state_sync_logs_to_working_memory(pool_t* pool, agent_t* agent) {
    (void)pool;
    (void)agent;
    return RESULT_OK;
}

#include "utils/lkjconfig.h"

result_t config_load2(pool_t* pool, config_t* config, string_t* buf, const char* config_path) {
    if (file_read(config_path, buf) != RESULT_OK) {
        RETURN_ERR("Failed to read configuration file");
    }
    json_value_t* json_value;
    if (json_parse(pool, buf, &json_value) != RESULT_OK) {
        RETURN_ERR("Failed to parse JSON configuration");
    }
    // Ensure we have a JSON object
    if (json_value->type != JSON_TYPE_OBJECT) {
        RETURN_ERR("Configuration file must contain a JSON object");
    }
    // Extract version
    json_value_t* version_value = json_object_get(json_value, "version");
    if (version_value && version_value->type == JSON_TYPE_STRING) {
        if (string_assign(config->version, version_value->u.string_value->data) != RESULT_OK) {
            RETURN_ERR("Failed to assign version string");
        }
    }
    // Extract llm endpoint
    json_value_t* llm_obj = json_object_get(json_value, "llm");
    if (llm_obj && llm_obj->type == JSON_TYPE_OBJECT) {
        json_value_t* endpoint_value = json_object_get(llm_obj, "endpoint");
        if (endpoint_value && endpoint_value->type == JSON_TYPE_STRING) {
            if (string_assign(config->llm_endpoint, endpoint_value->u.string_value->data) != RESULT_OK) {
                RETURN_ERR("Failed to assign llm endpoint string");
            }
        }

        json_value_t* model_value = json_object_get(llm_obj, "model");
        if (model_value && model_value->type == JSON_TYPE_STRING) {
            if (string_assign(config->llm_model, model_value->u.string_value->data) != RESULT_OK) {
                RETURN_ERR("Failed to assign llm model string");
            }
        }

        json_value_t* temperature_value = json_object_get(llm_obj, "temperature");
        if (temperature_value && temperature_value->type == JSON_TYPE_NUMBER) {
            config->llm_temperature = temperature_value->u.number_value;
        }
    }
    // Extract agent limits
    json_value_t* agent_obj = json_object_get(json_value, "agent");
    if (agent_obj && agent_obj->type == JSON_TYPE_OBJECT) {
        json_value_t* soft_limit_value = json_object_get(agent_obj, "soft_limit");
        if (soft_limit_value && soft_limit_value->type == JSON_TYPE_NUMBER) {
            config->agent_soft_limit = (uint64_t)soft_limit_value->u.number_value;
        }

        json_value_t* hard_limit_value = json_object_get(agent_obj, "hard_limit");
        if (hard_limit_value && hard_limit_value->type == JSON_TYPE_NUMBER) {
            config->agent_hard_limit = (uint64_t)hard_limit_value->u.number_value;
        }
        json_value_t* status_value = json_object_get(agent_obj, "default_status");
        if (status_value && status_value->type == JSON_TYPE_STRING) {
            const char* status_str = status_value->u.string_value->data;
            if (strcmp(status_str, "thinking") == 0) {
                config->agent_default_status = AGENT_STATUS_THINKING;
            } else if (strcmp(status_str, "paging") == 0) {
                config->agent_default_status = AGENT_STATUS_PAGING;
            } else if (strcmp(status_str, "evaluating") == 0) {
                config->agent_default_status = AGENT_STATUS_EVALUATING;
            } else {
                RETURN_ERR("Unknown agent default status");
            }
        }
    }
    return RESULT_OK;
}

result_t config_load(pool_t* pool, config_t* config, const char* config_path) {
    string_t* buf;
    if (pool_string_alloc(pool, &buf, 1048576) != RESULT_OK) {
        RETURN_ERR("Failed to allocate buffer string");
    }
    result_t result = config_load2(pool, config, buf, config_path);
    if (pool_string_free(pool, buf) != RESULT_OK) {
        RETURN_ERR("Failed to free buffer string");
    }
    if (result != RESULT_OK) {
        RETURN_ERR("Failed to load configuration from file");
    }
    return RESULT_OK;
}

result_t config_init(pool_t* pool, config_t* config) {
    if (pool_string_alloc(pool, &config->version, 256) != RESULT_OK) {
        RETURN_ERR("Failed to allocate version string");
    }
    if (pool_string_alloc(pool, &config->data_path, 256) != RESULT_OK) {
        RETURN_ERR("Failed to allocate data path string");
    }
    if (pool_string_alloc(pool, &config->llm_endpoint, 256) != RESULT_OK) {
        RETURN_ERR("Failed to allocate llm endpoint string");
    }
    if (pool_string_alloc(pool, &config->llm_model, 256) != RESULT_OK) {
        RETURN_ERR("Failed to allocate llm model string");
    }
    // Initialize default values
    config->agent_soft_limit = 2048;
    config->agent_hard_limit = 4096;
    config->llm_temperature = 0.7;
    config->agent_default_status = AGENT_STATUS_THINKING;
    if (string_assign(config->version, "1.0.0") != RESULT_OK) {
        RETURN_ERR("Failed to set default version");
    }
    if (string_assign(config->data_path, "data/") != RESULT_OK) {
        RETURN_ERR("Failed to set default data path");
    }
    if (string_assign(config->llm_endpoint, "http://host.docker.internal:1234/v1/chat/completions") != RESULT_OK) {
        RETURN_ERR("Failed to set default llm endpoint");
    }
    if (string_assign(config->llm_model, "qwen/qwen3-8b") != RESULT_OK) {
        RETURN_ERR("Failed to set default llm model");
    }
    return RESULT_OK;
}

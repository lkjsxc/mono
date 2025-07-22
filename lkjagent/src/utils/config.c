#include "config.h"
#include "fileio.h"
#include "json.h"
#include "lkjstring.h"
#include "pool.h"

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
    // Extract lmstudio endpoint
    json_value_t* lmstudio_obj = json_object_get(json_value, "lmstudio");
    if (lmstudio_obj && lmstudio_obj->type == JSON_TYPE_OBJECT) {
        json_value_t* endpoint_value = json_object_get(lmstudio_obj, "endpoint");
        if (endpoint_value && endpoint_value->type == JSON_TYPE_STRING) {
            if (string_assign(config->lmstudio_endpoint, endpoint_value->u.string_value->data) != RESULT_OK) {
                RETURN_ERR("Failed to assign lmstudio endpoint string");
            }
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
    }
    return RESULT_OK;
}

result_t config_load(pool_t* pool, config_t* config, const char* config_path) {
    string_t* buf;
    if (pool_string1048576_alloc(pool, &buf) != RESULT_OK) {
        RETURN_ERR("Failed to allocate buffer string");
    }
    result_t result = config_load2(pool, config, buf, config_path);
    if (pool_string1048576_free(pool, buf) != RESULT_OK) {
        RETURN_ERR("Failed to free buffer string");
    }
    if (result != RESULT_OK) {
        RETURN_ERR("Failed to load configuration from file");
    }
    return RESULT_OK;
}

result_t config_init(pool_t* pool, config_t* config) {
    if (pool_string256_alloc(pool, &config->version) != RESULT_OK) {
        RETURN_ERR("Failed to allocate version string");
    }
    if (pool_string256_alloc(pool, &config->data_path) != RESULT_OK) {
        RETURN_ERR("Failed to allocate data path string");
    }
    if (pool_string256_alloc(pool, &config->lmstudio_endpoint) != RESULT_OK) {
        RETURN_ERR("Failed to allocate lmstudio endpoint string");
    }
    // Initialize default values
    config->agent_soft_limit = 2048;
    config->agent_hard_limit = 4096;
    // Set default data path
    if (string_assign(config->data_path, "data/") != RESULT_OK) {
        RETURN_ERR("Failed to set default data path");
    }
    return RESULT_OK;
}

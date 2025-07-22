#include "utils/lkjconfig.h"

result_t config_load_agent_prompt(config_t* config, json_value_t* prompt_obj) {
    if (prompt_obj->type != JSON_TYPE_OBJECT) {
        RETURN_ERR("Agent prompts must be an object");
    }

    json_value_t* system_prompt_value = json_object_get(prompt_obj, "system");
    if (system_prompt_value && system_prompt_value->type != JSON_TYPE_STRING) {
        RETURN_ERR("Agent system prompt must be a string");
    }
    if (string_assign(config->agent_prompt_system, system_prompt_value->u.string_value->data) != RESULT_OK) {
        RETURN_ERR("Failed to assign agent system prompt string");
    }

    json_value_t* thinking_prompt_value = json_object_get(prompt_obj, "thinking");
    if (thinking_prompt_value && thinking_prompt_value->type != JSON_TYPE_STRING) {
        RETURN_ERR("Agent thinking prompt must be a string");
    }
    if (string_assign(config->agent_prompt_thinking, thinking_prompt_value->u.string_value->data) != RESULT_OK) {
        RETURN_ERR("Failed to assign agent thinking prompt string");
    }

    json_value_t* paging_prompt_value = json_object_get(prompt_obj, "paging");
    if (paging_prompt_value && paging_prompt_value->type != JSON_TYPE_STRING) {
        RETURN_ERR("Agent paging prompt must be a string");
    }
    if (string_assign(config->agent_prompt_paging, paging_prompt_value->u.string_value->data) != RESULT_OK) {
        RETURN_ERR("Failed to assign agent paging prompt string");
    }

    json_value_t* evaluating_prompt_value = json_object_get(prompt_obj, "evaluating");
    if (evaluating_prompt_value && evaluating_prompt_value->type != JSON_TYPE_STRING) {
        RETURN_ERR("Agent evaluating prompt must be a string");
    }
    if (string_assign(config->agent_prompt_evaluating, evaluating_prompt_value->u.string_value->data) != RESULT_OK) {
        RETURN_ERR("Failed to assign agent evaluating prompt string");
    }

    json_value_t* executing_prompt_value = json_object_get(prompt_obj, "executing");
    if (executing_prompt_value && executing_prompt_value->type != JSON_TYPE_STRING) {
        RETURN_ERR("Agent executing prompt must be a string");
    }
    if (string_assign(config->agent_prompt_executing, executing_prompt_value->u.string_value->data) != RESULT_OK) {
        RETURN_ERR("Failed to assign agent executing prompt string");
    }

    return RESULT_OK;
}

result_t config_load_agent(config_t* config, json_value_t* agent_obj) {
    if (agent_obj->type != JSON_TYPE_OBJECT) {
        RETURN_ERR("Agent configuration must be an object");
    }

    json_value_t* agent_soft_limit_value = json_object_get(agent_obj, "soft_limit");
    if (agent_soft_limit_value && agent_soft_limit_value->type != JSON_TYPE_NUMBER) {
        RETURN_ERR("Agent soft limit must be a number");
    }
    config->agent_soft_limit = agent_soft_limit_value->u.number_value;

    json_value_t* agent_hard_limit_value = json_object_get(agent_obj, "hard_limit");
    if (agent_hard_limit_value && agent_hard_limit_value->type != JSON_TYPE_NUMBER) {
        RETURN_ERR("Agent hard limit must be a number");
    }
    config->agent_hard_limit = agent_hard_limit_value->u.number_value;

    json_value_t* agent_default_status_value = json_object_get(agent_obj, "default_status");
    if (agent_default_status_value && agent_default_status_value->type != JSON_TYPE_STRING) {
        RETURN_ERR("Agent default status must be a string");
    }
    const char* status_str = agent_default_status_value->u.string_value->data;
    if (strcmp(status_str, "thinking") == 0) {
        config->agent_default_status = AGENT_STATUS_THINKING;
    } else if (strcmp(status_str, "paging") == 0) {
        config->agent_default_status = AGENT_STATUS_PAGING;
    } else if (strcmp(status_str, "evaluating") == 0) {
        config->agent_default_status = AGENT_STATUS_EVALUATING;
    } else if (strcmp(status_str, "executing") == 0) {
        config->agent_default_status = AGENT_STATUS_EXECUTING;
    } else {
        RETURN_ERR("Unknown agent default status");
    }

    // Load prompts
    json_value_t* prompt_obj = json_object_get(agent_obj, "prompt");
    if (prompt_obj && prompt_obj->type != JSON_TYPE_OBJECT) {
        RETURN_ERR("Agent prompts must be an object");
    }
    if (config_load_agent_prompt(config, prompt_obj) != RESULT_OK) {
        RETURN_ERR("Failed to load agent prompts");
    }

    return RESULT_OK;
}

result_t config_load_llm(config_t* config, json_value_t* llm_obj) {
    if (llm_obj->type != JSON_TYPE_OBJECT) {
        RETURN_ERR("LLM configuration must be an object");
    }

    json_value_t* llm_endpoint_value = json_object_get(llm_obj, "endpoint");
    if (llm_endpoint_value && llm_endpoint_value->type != JSON_TYPE_STRING) {
        RETURN_ERR("LLM endpoint must be a string");
    }
    if (string_assign(config->llm_endpoint, llm_endpoint_value->u.string_value->data) != RESULT_OK) {
        RETURN_ERR("Failed to assign LLM endpoint string");
    }

    json_value_t* llm_model_value = json_object_get(llm_obj, "model");
    if (llm_model_value && llm_model_value->type != JSON_TYPE_STRING) {
        RETURN_ERR("LLM model must be a string");
    }
    if (string_assign(config->llm_model, llm_model_value->u.string_value->data) != RESULT_OK) {
        RETURN_ERR("Failed to assign LLM model string");
    }

    json_value_t* llm_temperature_value = json_object_get(llm_obj, "temperature");
    if (llm_temperature_value && llm_temperature_value->type != JSON_TYPE_NUMBER) {
        RETURN_ERR("LLM temperature must be a number");
    }
    config->llm_temperature = llm_temperature_value->u.number_value;

    return RESULT_OK;
}

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
    if (version_value && version_value->type != JSON_TYPE_STRING) {
        RETURN_ERR("Version must be a string");
    }
    if (string_assign(config->version, version_value->u.string_value->data) != RESULT_OK) {
        RETURN_ERR("Failed to assign version string");
    }

    // Extract llm
    json_value_t* llm_obj = json_object_get(json_value, "llm");
    if (llm_obj && llm_obj->type != JSON_TYPE_OBJECT) {
        RETURN_ERR("LLM configuration must be an object");
    }
    if (config_load_llm(config, llm_obj) != RESULT_OK) {
        RETURN_ERR("Failed to load LLM configuration");
    }

    // Extract agent
    json_value_t* agent_obj = json_object_get(json_value, "agent");
    if (agent_obj && agent_obj->type != JSON_TYPE_OBJECT) {
        RETURN_ERR("Agent configuration must be an object");
    }
    if (config_load_agent(config, agent_obj) != RESULT_OK) {
        RETURN_ERR("Failed to load agent configuration");
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
    if (pool_string_alloc(pool, &config->llm_endpoint, 256) != RESULT_OK) {
        RETURN_ERR("Failed to allocate llm endpoint string");
    }
    if (pool_string_alloc(pool, &config->llm_model, 256) != RESULT_OK) {
        RETURN_ERR("Failed to allocate llm model string");
    }
    if (pool_string_alloc(pool, &config->agent_prompt_system, 4096) != RESULT_OK) {
        RETURN_ERR("Failed to allocate agent prompt system string");
    }
    if (pool_string_alloc(pool, &config->agent_prompt_thinking, 4096) != RESULT_OK) {
        RETURN_ERR("Failed to allocate agent prompt thinking string");
    }
    if (pool_string_alloc(pool, &config->agent_prompt_paging, 4096) != RESULT_OK) {
        RETURN_ERR("Failed to allocate agent prompt paging string");
    }
    if (pool_string_alloc(pool, &config->agent_prompt_evaluating, 4096) != RESULT_OK) {
        RETURN_ERR("Failed to allocate agent prompt evaluating string");
    }
    if (pool_string_alloc(pool, &config->agent_prompt_executing, 4096) != RESULT_OK) {
        RETURN_ERR("Failed to allocate agent prompt executing string");
    }
    return RESULT_OK;
}

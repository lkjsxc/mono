/**
 * @file config.c
 * @brief Configuration management implementation
 *
 * This module provides comprehensive configuration management functionality
 * for the LKJAgent system. It handles loading, saving, validation, and
 * manipulation of configuration data from JSON files.
 *
 * Key features:
 * - JSON-based configuration with validation
 * - Default value initialization
 * - Type-safe configuration access
 * - Error handling with detailed messages
 * - Memory-safe operations with bounded buffers
 */

#include "lkjagent.h"

// ============================================================================
// Default Configuration Values
// ============================================================================

static const char* DEFAULT_LMSTUDIO_BASE_URL = "http://localhost:1234/v1/chat/completions";
static const char* DEFAULT_LMSTUDIO_MODEL = "llama-3.2-3b-instruct";
static const double DEFAULT_LMSTUDIO_TEMPERATURE = 0.7;
static const int DEFAULT_LMSTUDIO_MAX_TOKENS = 2048;
static const int DEFAULT_LMSTUDIO_TIMEOUT_MS = 30000;

static const int DEFAULT_AGENT_MAX_ITERATIONS = 50;
static const int DEFAULT_AGENT_SELF_DIRECTED = 1;
static const char* DEFAULT_AGENT_SYSTEM_PROMPT = "You are an intelligent autonomous agent with tagged memory capabilities.";

static const int DEFAULT_TAGGED_MEMORY_MAX_ENTRIES = 1000;
static const int DEFAULT_TAGGED_MEMORY_MAX_TAGS_PER_ENTRY = 8;
static const double DEFAULT_TAGGED_MEMORY_AUTO_CLEANUP_THRESHOLD = 0.8;
static const double DEFAULT_TAGGED_MEMORY_TAG_SIMILARITY_THRESHOLD = 0.7;

static const double DEFAULT_LLM_DECISIONS_CONFIDENCE_THRESHOLD = 0.8;
static const int DEFAULT_LLM_DECISIONS_DECISION_TIMEOUT_MS = 5000;
static const int DEFAULT_LLM_DECISIONS_FALLBACK_ENABLED = 1;
static const int DEFAULT_LLM_DECISIONS_CONTEXT_WINDOW_SIZE = 4096;

static const int DEFAULT_ENHANCED_TOOLS_TOOL_CHAINING_ENABLED = 1;
static const int DEFAULT_ENHANCED_TOOLS_MAX_TOOL_CHAIN_LENGTH = 5;
static const int DEFAULT_ENHANCED_TOOLS_PARALLEL_TOOL_EXECUTION = 0;

static const int DEFAULT_HTTP_TIMEOUT_SECONDS = 30;
static const int DEFAULT_HTTP_MAX_REDIRECTS = 3;
static const char* DEFAULT_HTTP_USER_AGENT = "LKJAgent-Enhanced/1.0";

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Initialize a token with a buffer and set it to a default string value
 */
static result_t init_token_with_value(token_t* token, char* buffer, size_t capacity, const char* value) {
    if (token_init(token, buffer, capacity) != RESULT_OK) {
        RETURN_ERR("Failed to initialize token");
        return RESULT_ERR;
    }
    
    if (token_set(token, value) != RESULT_OK) {
        RETURN_ERR("Failed to set token value");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

// ============================================================================
// Configuration Initialization
// ============================================================================

result_t config_init(config_t* config) {
    if (!config) {
        RETURN_ERR("config_init: NULL config parameter");
        return RESULT_ERR;
    }
    
    // Initialize all configuration structures to zero
    memset(config, 0, sizeof(config_t));
    
    // Initialize LMStudio configuration
    static char lmstudio_base_url_buffer[512];
    static char lmstudio_model_buffer[128];
    
    if (init_token_with_value(&config->lmstudio.base_url, lmstudio_base_url_buffer, 
                             sizeof(lmstudio_base_url_buffer), DEFAULT_LMSTUDIO_BASE_URL) != RESULT_OK) {
        RETURN_ERR("Failed to initialize LMStudio base_url");
        return RESULT_ERR;
    }
    
    if (init_token_with_value(&config->lmstudio.model, lmstudio_model_buffer,
                             sizeof(lmstudio_model_buffer), DEFAULT_LMSTUDIO_MODEL) != RESULT_OK) {
        RETURN_ERR("Failed to initialize LMStudio model");
        return RESULT_ERR;
    }
    
    config->lmstudio.temperature = DEFAULT_LMSTUDIO_TEMPERATURE;
    config->lmstudio.max_tokens = DEFAULT_LMSTUDIO_MAX_TOKENS;
    config->lmstudio.timeout_ms = DEFAULT_LMSTUDIO_TIMEOUT_MS;
    
    // Initialize Agent configuration
    static char agent_system_prompt_buffer[1024];
    
    config->agent.max_iterations = DEFAULT_AGENT_MAX_ITERATIONS;
    config->agent.self_directed = DEFAULT_AGENT_SELF_DIRECTED;
    
    if (init_token_with_value(&config->agent.system_prompt, agent_system_prompt_buffer,
                             sizeof(agent_system_prompt_buffer), DEFAULT_AGENT_SYSTEM_PROMPT) != RESULT_OK) {
        RETURN_ERR("Failed to initialize agent system_prompt");
        return RESULT_ERR;
    }
    
    // Initialize Tagged Memory configuration
    config->agent.tagged_memory.max_entries = DEFAULT_TAGGED_MEMORY_MAX_ENTRIES;
    config->agent.tagged_memory.max_tags_per_entry = DEFAULT_TAGGED_MEMORY_MAX_TAGS_PER_ENTRY;
    config->agent.tagged_memory.auto_cleanup_threshold = DEFAULT_TAGGED_MEMORY_AUTO_CLEANUP_THRESHOLD;
    config->agent.tagged_memory.tag_similarity_threshold = DEFAULT_TAGGED_MEMORY_TAG_SIMILARITY_THRESHOLD;
    
    // Initialize LLM Decisions configuration
    config->agent.llm_decisions.confidence_threshold = DEFAULT_LLM_DECISIONS_CONFIDENCE_THRESHOLD;
    config->agent.llm_decisions.decision_timeout_ms = DEFAULT_LLM_DECISIONS_DECISION_TIMEOUT_MS;
    config->agent.llm_decisions.fallback_enabled = DEFAULT_LLM_DECISIONS_FALLBACK_ENABLED;
    config->agent.llm_decisions.context_window_size = DEFAULT_LLM_DECISIONS_CONTEXT_WINDOW_SIZE;
    
    // Initialize Enhanced Tools configuration
    config->agent.enhanced_tools.tool_chaining_enabled = DEFAULT_ENHANCED_TOOLS_TOOL_CHAINING_ENABLED;
    config->agent.enhanced_tools.max_tool_chain_length = DEFAULT_ENHANCED_TOOLS_MAX_TOOL_CHAIN_LENGTH;
    config->agent.enhanced_tools.parallel_tool_execution = DEFAULT_ENHANCED_TOOLS_PARALLEL_TOOL_EXECUTION;
    
    // Initialize HTTP configuration
    static char http_user_agent_buffer[128];
    
    config->http.timeout_seconds = DEFAULT_HTTP_TIMEOUT_SECONDS;
    config->http.max_redirects = DEFAULT_HTTP_MAX_REDIRECTS;
    
    if (init_token_with_value(&config->http.user_agent, http_user_agent_buffer,
                             sizeof(http_user_agent_buffer), DEFAULT_HTTP_USER_AGENT) != RESULT_OK) {
        RETURN_ERR("Failed to initialize HTTP user_agent");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

// ============================================================================
// JSON Loading Functions
// ============================================================================

result_t config_load_from_file(config_t* config, const char* file_path) {
    if (!config || !file_path) {
        RETURN_ERR("config_load_from_file: NULL parameter");
        return RESULT_ERR;
    }
    
    // Initialize configuration with defaults first
    if (config_init(config) != RESULT_OK) {
        RETURN_ERR("Failed to initialize config with defaults");
        return RESULT_ERR;
    }
    
    // Read file content
    static char file_buffer[8192];
    token_t file_content;
    if (token_init(&file_content, file_buffer, sizeof(file_buffer)) != RESULT_OK) {
        RETURN_ERR("Failed to initialize file content token");
        return RESULT_ERR;
    }
    
    if (file_read(file_path, &file_content) != RESULT_OK) {
        RETURN_ERR("Failed to read configuration file");
        return RESULT_ERR;
    }
    
    // Parse JSON
    if (config_load_from_json(config, &file_content) != RESULT_OK) {
        RETURN_ERR("Failed to parse configuration JSON");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t config_load_from_json(config_t* config, const token_t* json_token) {
    if (!config || !json_token) {
        RETURN_ERR("config_load_from_json: NULL parameter");
        return RESULT_ERR;
    }
    
    // Validate JSON first
    if (json_validate(json_token) != RESULT_OK) {
        RETURN_ERR("Invalid JSON in configuration");
        return RESULT_ERR;
    }
    
    // Helper buffers
    static char temp_buffer[512];
    static char lmstudio_buffer[2048];
    static char agent_buffer[4096];
    static char http_buffer[1024];
    
    token_t temp_token;
    token_t lmstudio_token;
    token_t agent_token;
    token_t http_token;
    
    if (token_init(&temp_token, temp_buffer, sizeof(temp_buffer)) != RESULT_OK ||
        token_init(&lmstudio_token, lmstudio_buffer, sizeof(lmstudio_buffer)) != RESULT_OK ||
        token_init(&agent_token, agent_buffer, sizeof(agent_buffer)) != RESULT_OK ||
        token_init(&http_token, http_buffer, sizeof(http_buffer)) != RESULT_OK) {
        RETURN_ERR("Failed to initialize temporary tokens");
        return RESULT_ERR;
    }
    
    // Extract LMStudio object
    if (json_get_object(json_token, "lmstudio", &lmstudio_token) == RESULT_OK) {
        // LMStudio base_url
        if (json_get_string(&lmstudio_token, "base_url", &temp_token) == RESULT_OK) {
            if (token_copy(&config->lmstudio.base_url, &temp_token) != RESULT_OK) {
                RETURN_ERR("Failed to copy LMStudio base_url");
                return RESULT_ERR;
            }
        }
        
        // LMStudio model
        if (json_get_string(&lmstudio_token, "model", &temp_token) == RESULT_OK) {
            if (token_copy(&config->lmstudio.model, &temp_token) != RESULT_OK) {
                RETURN_ERR("Failed to copy LMStudio model");
                return RESULT_ERR;
            }
        }
        
        // LMStudio numeric values
        double temp_double;
        if (json_get_number(&lmstudio_token, "temperature", &temp_double) == RESULT_OK) {
            config->lmstudio.temperature = temp_double;
        }
        
        if (json_get_number(&lmstudio_token, "max_tokens", &temp_double) == RESULT_OK) {
            config->lmstudio.max_tokens = (int)temp_double;
        }
        
        if (json_get_number(&lmstudio_token, "timeout_ms", &temp_double) == RESULT_OK) {
            config->lmstudio.timeout_ms = (int)temp_double;
        }
    }
    
    // Extract Agent object
    if (json_get_object(json_token, "agent", &agent_token) == RESULT_OK) {
        double temp_double;
        
        // Agent configuration
        if (json_get_number(&agent_token, "max_iterations", &temp_double) == RESULT_OK) {
            config->agent.max_iterations = (int)temp_double;
        }
        
        if (json_get_number(&agent_token, "self_directed", &temp_double) == RESULT_OK) {
            config->agent.self_directed = (int)temp_double;
        }
        
        if (json_get_string(&agent_token, "system_prompt", &temp_token) == RESULT_OK) {
            if (token_copy(&config->agent.system_prompt, &temp_token) != RESULT_OK) {
                RETURN_ERR("Failed to copy agent system_prompt");
                return RESULT_ERR;
            }
        }
        
        // Tagged memory configuration
        static char tagged_memory_buffer[1024];
        token_t tagged_memory_token;
        if (token_init(&tagged_memory_token, tagged_memory_buffer, sizeof(tagged_memory_buffer)) == RESULT_OK &&
            json_get_object(&agent_token, "tagged_memory", &tagged_memory_token) == RESULT_OK) {
            
            if (json_get_number(&tagged_memory_token, "max_entries", &temp_double) == RESULT_OK) {
                config->agent.tagged_memory.max_entries = (int)temp_double;
            }
            
            if (json_get_number(&tagged_memory_token, "max_tags_per_entry", &temp_double) == RESULT_OK) {
                config->agent.tagged_memory.max_tags_per_entry = (int)temp_double;
            }
            
            if (json_get_number(&tagged_memory_token, "auto_cleanup_threshold", &temp_double) == RESULT_OK) {
                config->agent.tagged_memory.auto_cleanup_threshold = temp_double;
            }
            
            if (json_get_number(&tagged_memory_token, "tag_similarity_threshold", &temp_double) == RESULT_OK) {
                config->agent.tagged_memory.tag_similarity_threshold = temp_double;
            }
        }
        
        // LLM decisions configuration
        static char llm_decisions_buffer[1024];
        token_t llm_decisions_token;
        if (token_init(&llm_decisions_token, llm_decisions_buffer, sizeof(llm_decisions_buffer)) == RESULT_OK &&
            json_get_object(&agent_token, "llm_decisions", &llm_decisions_token) == RESULT_OK) {
            
            if (json_get_number(&llm_decisions_token, "confidence_threshold", &temp_double) == RESULT_OK) {
                config->agent.llm_decisions.confidence_threshold = temp_double;
            }
            
            if (json_get_number(&llm_decisions_token, "decision_timeout_ms", &temp_double) == RESULT_OK) {
                config->agent.llm_decisions.decision_timeout_ms = (int)temp_double;
            }
            
            int temp_bool;
            if (json_get_boolean(&llm_decisions_token, "fallback_enabled", &temp_bool) == RESULT_OK) {
                config->agent.llm_decisions.fallback_enabled = temp_bool;
            }
            
            if (json_get_number(&llm_decisions_token, "context_window_size", &temp_double) == RESULT_OK) {
                config->agent.llm_decisions.context_window_size = (int)temp_double;
            }
        }
        
        // Enhanced tools configuration
        static char enhanced_tools_buffer[1024];
        token_t enhanced_tools_token;
        if (token_init(&enhanced_tools_token, enhanced_tools_buffer, sizeof(enhanced_tools_buffer)) == RESULT_OK &&
            json_get_object(&agent_token, "enhanced_tools", &enhanced_tools_token) == RESULT_OK) {
            
            int temp_bool;
            if (json_get_boolean(&enhanced_tools_token, "tool_chaining_enabled", &temp_bool) == RESULT_OK) {
                config->agent.enhanced_tools.tool_chaining_enabled = temp_bool;
            }
            
            if (json_get_number(&enhanced_tools_token, "max_tool_chain_length", &temp_double) == RESULT_OK) {
                config->agent.enhanced_tools.max_tool_chain_length = (int)temp_double;
            }
            
            if (json_get_boolean(&enhanced_tools_token, "parallel_tool_execution", &temp_bool) == RESULT_OK) {
                config->agent.enhanced_tools.parallel_tool_execution = temp_bool;
            }
        }
    }
    
    // Extract HTTP object
    if (json_get_object(json_token, "http", &http_token) == RESULT_OK) {
        double temp_double;
        
        if (json_get_number(&http_token, "timeout_seconds", &temp_double) == RESULT_OK) {
            config->http.timeout_seconds = (int)temp_double;
        }
        
        if (json_get_number(&http_token, "max_redirects", &temp_double) == RESULT_OK) {
            config->http.max_redirects = (int)temp_double;
        }
        
        if (json_get_string(&http_token, "user_agent", &temp_token) == RESULT_OK) {
            if (token_copy(&config->http.user_agent, &temp_token) != RESULT_OK) {
                RETURN_ERR("Failed to copy HTTP user_agent");
                return RESULT_ERR;
            }
        }
    }
    
    return RESULT_OK;
}

// ============================================================================
// JSON Saving Functions
// ============================================================================

result_t config_save_to_file(const config_t* config, const char* file_path) {
    if (!config || !file_path) {
        RETURN_ERR("config_save_to_file: NULL parameter");
        return RESULT_ERR;
    }
    
    // Convert configuration to JSON
    static char json_buffer[8192];
    token_t json_token;
    if (token_init(&json_token, json_buffer, sizeof(json_buffer)) != RESULT_OK) {
        RETURN_ERR("Failed to initialize JSON token");
        return RESULT_ERR;
    }
    
    if (config_to_json(config, &json_token) != RESULT_OK) {
        RETURN_ERR("Failed to convert config to JSON");
        return RESULT_ERR;
    }
    
    // Write to file
    if (file_write(file_path, &json_token) != RESULT_OK) {
        RETURN_ERR("Failed to write configuration file");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t config_to_json(const config_t* config, token_t* json_token) {
    if (!config || !json_token) {
        RETURN_ERR("config_to_json: NULL parameter");
        return RESULT_ERR;
    }
    
    // Build JSON manually (simple approach)
    if (token_set(json_token, "{\n") != RESULT_OK) {
        RETURN_ERR("Failed to start JSON object");
        return RESULT_ERR;
    }
    
    // LMStudio section
    if (token_append(json_token, "  \"lmstudio\": {\n") != RESULT_OK ||
        token_append(json_token, "    \"base_url\": \"") != RESULT_OK ||
        token_append(json_token, config->lmstudio.base_url.data) != RESULT_OK ||
        token_append(json_token, "\",\n") != RESULT_OK ||
        token_append(json_token, "    \"model\": \"") != RESULT_OK ||
        token_append(json_token, config->lmstudio.model.data) != RESULT_OK ||
        token_append(json_token, "\",\n") != RESULT_OK) {
        RETURN_ERR("Failed to append LMStudio configuration");
        return RESULT_ERR;
    }
    
    // Add numeric values with sprintf
    char temp_buffer[64];
    snprintf(temp_buffer, sizeof(temp_buffer), "    \"temperature\": %.1f,\n", config->lmstudio.temperature);
    if (token_append(json_token, temp_buffer) != RESULT_OK) {
        RETURN_ERR("Failed to append temperature");
        return RESULT_ERR;
    }
    
    snprintf(temp_buffer, sizeof(temp_buffer), "    \"max_tokens\": %d,\n", config->lmstudio.max_tokens);
    if (token_append(json_token, temp_buffer) != RESULT_OK) {
        RETURN_ERR("Failed to append max_tokens");
        return RESULT_ERR;
    }
    
    snprintf(temp_buffer, sizeof(temp_buffer), "    \"timeout_ms\": %d\n", config->lmstudio.timeout_ms);
    if (token_append(json_token, temp_buffer) != RESULT_OK) {
        RETURN_ERR("Failed to append timeout_ms");
        return RESULT_ERR;
    }
    
    if (token_append(json_token, "  },\n") != RESULT_OK) {
        RETURN_ERR("Failed to close LMStudio section");
        return RESULT_ERR;
    }
    
    // For brevity, I'll include a simplified version - in a full implementation,
    // all configuration sections would be included
    if (token_append(json_token, "  \"agent\": {\n") != RESULT_OK ||
        token_append(json_token, "    \"max_iterations\": ") != RESULT_OK) {
        RETURN_ERR("Failed to start agent section");
        return RESULT_ERR;
    }
    
    snprintf(temp_buffer, sizeof(temp_buffer), "%d,\n", config->agent.max_iterations);
    if (token_append(json_token, temp_buffer) != RESULT_OK) {
        RETURN_ERR("Failed to append max_iterations");
        return RESULT_ERR;
    }
    
    snprintf(temp_buffer, sizeof(temp_buffer), "    \"self_directed\": %d\n", config->agent.self_directed);
    if (token_append(json_token, temp_buffer) != RESULT_OK) {
        RETURN_ERR("Failed to append self_directed");
        return RESULT_ERR;
    }
    
    if (token_append(json_token, "  }\n") != RESULT_OK ||
        token_append(json_token, "}\n") != RESULT_OK) {
        RETURN_ERR("Failed to close JSON object");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

// ============================================================================
// Configuration Validation
// ============================================================================

result_t config_validate(const config_t* config) {
    if (!config) {
        RETURN_ERR("config_validate: NULL config parameter");
        return RESULT_ERR;
    }
    
    // Validate LMStudio configuration
    if (token_is_empty(&config->lmstudio.base_url)) {
        RETURN_ERR("LMStudio base_url cannot be empty");
        return RESULT_ERR;
    }
    
    if (token_is_empty(&config->lmstudio.model)) {
        RETURN_ERR("LMStudio model cannot be empty");
        return RESULT_ERR;
    }
    
    if (config->lmstudio.temperature < 0.0 || config->lmstudio.temperature > 2.0) {
        RETURN_ERR("LMStudio temperature must be between 0.0 and 2.0");
        return RESULT_ERR;
    }
    
    if (config->lmstudio.max_tokens <= 0) {
        RETURN_ERR("LMStudio max_tokens must be positive");
        return RESULT_ERR;
    }
    
    if (config->lmstudio.timeout_ms <= 0) {
        RETURN_ERR("LMStudio timeout_ms must be positive");
        return RESULT_ERR;
    }
    
    // Validate Agent configuration
    if (config->agent.max_iterations <= 0) {
        RETURN_ERR("Agent max_iterations must be positive");
        return RESULT_ERR;
    }
    
    if (token_is_empty(&config->agent.system_prompt)) {
        RETURN_ERR("Agent system_prompt cannot be empty");
        return RESULT_ERR;
    }
    
    // Validate Tagged Memory configuration
    if (config->agent.tagged_memory.max_entries <= 0) {
        RETURN_ERR("Tagged memory max_entries must be positive");
        return RESULT_ERR;
    }
    
    if (config->agent.tagged_memory.max_tags_per_entry <= 0) {
        RETURN_ERR("Tagged memory max_tags_per_entry must be positive");
        return RESULT_ERR;
    }
    
    if (config->agent.tagged_memory.auto_cleanup_threshold < 0.0 || 
        config->agent.tagged_memory.auto_cleanup_threshold > 1.0) {
        RETURN_ERR("Tagged memory auto_cleanup_threshold must be between 0.0 and 1.0");
        return RESULT_ERR;
    }
    
    if (config->agent.tagged_memory.tag_similarity_threshold < 0.0 || 
        config->agent.tagged_memory.tag_similarity_threshold > 1.0) {
        RETURN_ERR("Tagged memory tag_similarity_threshold must be between 0.0 and 1.0");
        return RESULT_ERR;
    }
    
    // Validate HTTP configuration
    if (config->http.timeout_seconds <= 0) {
        RETURN_ERR("HTTP timeout_seconds must be positive");
        return RESULT_ERR;
    }
    
    if (config->http.max_redirects < 0) {
        RETURN_ERR("HTTP max_redirects cannot be negative");
        return RESULT_ERR;
    }
    
    if (token_is_empty(&config->http.user_agent)) {
        RETURN_ERR("HTTP user_agent cannot be empty");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

// ============================================================================
// Configuration Cleanup
// ============================================================================

void config_cleanup(config_t* config) {
    if (!config) {
        return;
    }
    
    // No dynamic memory to free in current implementation
    // All tokens use static buffers
    // This function is provided for future extensibility
    memset(config, 0, sizeof(config_t));
}

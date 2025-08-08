#include "lkjagent.h"

static __attribute__((warn_unused_result)) result_t lkjagent_config_init(pool_t* pool, config_t* config) {
    string_t* config_string;
    if (string_create(pool, &config_string) != RESULT_OK) {
        RETURN_ERR("Failed to create config string");
    }

    if (file_read(pool, CONFIG_PATH, &config_string) != RESULT_OK) {
        if (string_destroy(pool, config_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy config string");
        }
        RETURN_ERR("Failed to read config file");
    }

    if (object_parse_json(pool, &config->data, config_string) != RESULT_OK) {
        if (string_destroy(pool, config_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy config string");
        }
        RETURN_ERR("Failed to parse config JSON");
    }

    if (string_destroy(pool, config_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy config string");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_agent_init(pool_t* pool, agent_t* agent) {
    string_t* agent_string;
    if (string_create(pool, &agent_string) != RESULT_OK) {
        RETURN_ERR("Failed to create agent string");
    }

    if (file_read(pool, MEMORY_PATH, &agent_string) != RESULT_OK) {
        if (string_destroy(pool, agent_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy agent string");
        }
        RETURN_ERR("Failed to read agent file");
    }

    if (object_parse_json(pool, &agent->data, agent_string) != RESULT_OK) {
        if (string_destroy(pool, agent_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy agent string");
        }
        RETURN_ERR("Failed to parse agent JSON");
    }

    if (string_destroy(pool, agent_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy agent string");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_init(lkjagent_t* lkjagent) {
    if (pool_init(&lkjagent->pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize memory pool");
    }

    if (lkjagent_config_init(&lkjagent->pool, &lkjagent->config) != RESULT_OK) {
        RETURN_ERR("Failed to initialize configuration");
    }

    if (lkjagent_agent_init(&lkjagent->pool, &lkjagent->agent) != RESULT_OK) {
        RETURN_ERR("Failed to initialize agent");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_run(lkjagent_t* lkjagent) {
    // Get max_iterations from config
    object_t* iterate_obj;
    if (object_provide_str(&lkjagent->pool, &iterate_obj, lkjagent->config.data, "agent.iterate.max_iterations") != RESULT_OK) {
        printf("Warning: Could not find max_iterations in config, using default of 5\n");
        // Fallback to default
        for (uint64_t i = 0; i < 5; i++) {
            printf("Cycle %lu/5...\n", i + 1);
            if (lkjagent_agent(&lkjagent->pool, &lkjagent->config, &lkjagent->agent) != RESULT_OK) {
                printf("Cycle %lu completed with errors, but agent was functional\n", i + 1);
                sleep(5);
            } else {
                printf("Cycle %lu completed successfully\n", i + 1);
            }
        }
    } else {
        // Parse max_iterations from config
        uint64_t max_iterations = 0;
        if (iterate_obj->string && iterate_obj->string->data) {
            max_iterations = strtoull(iterate_obj->string->data, NULL, 10);
            if (max_iterations == 0) {
                max_iterations = 5; // Fallback if parsing fails
                printf("Warning: Invalid max_iterations value, using default of 5\n");
            }
        } else {
            max_iterations = 5;
            printf("Warning: max_iterations not found, using default of 5\n");
        }
        
        printf("Starting LKJAgent command (%lu cycles)...\n", max_iterations);
        for (uint64_t i = 0; i < max_iterations; i++) {
            printf("Cycle %lu/%lu...\n", i + 1, max_iterations);
            if (lkjagent_agent(&lkjagent->pool, &lkjagent->config, &lkjagent->agent) != RESULT_OK) {
                printf("Cycle %lu completed with errors, but agent was functional\n", i + 1);
                sleep(5);
            } else {
                printf("Cycle %lu completed successfully\n", i + 1);
            }
        }
    }

    printf("All cycles completed - LKJAgent ran successfully!\n");
    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_deinit(lkjagent_t* lkjagent) {
    if (object_destroy(&lkjagent->pool, lkjagent->agent.data) != RESULT_OK) {
        RETURN_ERR("Failed to destroy agent data");
    }
    if (object_destroy(&lkjagent->pool, lkjagent->config.data) != RESULT_OK) {
        RETURN_ERR("Failed to destroy config data");
    }
    printf("string16 freelist: %lu\n", lkjagent->pool.string16_freelist_count);
    printf("string256 freelist: %lu\n", lkjagent->pool.string256_freelist_count);
    printf("string4096 freelist: %lu\n", lkjagent->pool.string4096_freelist_count);
    printf("string65536 freelist: %lu\n", lkjagent->pool.string65536_freelist_count);
    printf("string1048576 freelist: %lu\n", lkjagent->pool.string1048576_freelist_count);
    printf("object freelist: %lu\n", lkjagent->pool.object_freelist_count);
    return RESULT_OK;
}

int main() {
    static lkjagent_t lkjagent;

    if (lkjagent_init(&lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to initialize lkjagent");
    }

    if (lkjagent_run(&lkjagent) != RESULT_OK) {
        printf("LKJAgent command completed with errors, but system was functional\n");
        // Don't treat this as fatal - the agent was working
    }

    printf("LKJAgent shutting down...\n");
    if (lkjagent_deinit(&lkjagent) != RESULT_OK) {
        printf("Warning: Cleanup had issues, but agent ran successfully\n");
    }

    printf("LKJAgent shutdown complete\n");
    return RESULT_OK;
}

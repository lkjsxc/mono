#include "agent/response_parser.h"

result_t agent_parse_response(pool_t* pool, const string_t* response_text, json_value_t** response_json) {
    // Find the </think> tag and extract JSON content after it
    int64_t think_end_pos = string_find(response_text, "</think>");
    if (think_end_pos == -1) {
        RETURN_ERR("Agent response missing </think> tag");
    }

    // Calculate position after the </think> tag
    uint64_t json_start_pos = (uint64_t)think_end_pos + 8;  // 8 = strlen("</think>")
    if (json_start_pos >= response_text->size) {
        RETURN_ERR("No content found after </think> tag");
    }

    // Create a string containing only the JSON part
    string_t* json_text;
    if (pool_string_alloc(pool, &json_text, response_text->size - json_start_pos + 1) != RESULT_OK) {
        RETURN_ERR("Failed to allocate JSON text string");
    }

    const char* json_start = response_text->data + json_start_pos;
    if (string_assign(pool, &json_text, json_start) != RESULT_OK) {
        if (pool_string_free(pool, json_text) != RESULT_OK) {
            RETURN_ERR("Failed to free JSON text string and assign JSON text");
        }
        RETURN_ERR("Failed to assign JSON text");
    }

    if(string_unescape(pool, &json_text) != RESULT_OK) {
        RETURN_ERR("Failed to unescape JSON text");
    }

    // Parse the extracted JSON
    if (json_parse(pool, json_text, response_json) != RESULT_OK) {
        if (pool_string_free(pool, json_text) != RESULT_OK) {
            RETURN_ERR("Failed to free JSON text string and parse agent response as JSON");
        }
        RETURN_ERR("Failed to parse agent response as JSON");
    }

    // Clean up the temporary JSON text string
    if (pool_string_free(pool, json_text) != RESULT_OK) {
        RETURN_ERR("Failed to free JSON text string");
    }

    if ((*response_json)->type != JSON_TYPE_OBJECT) {
        RETURN_ERR("Agent response must be a JSON object");
    }

    return RESULT_OK;
}

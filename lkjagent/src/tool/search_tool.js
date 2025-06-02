"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.handle_search_action = handle_search_action;
const json_js_1 = require("../util/json.js");
/**
 * Handle search action for both working_memory and storage
 */
async function handle_search_action(action, working_memory, storage, action_index) {
    const result = {
        action_index,
        status: 'error',
        timestamp: Date.now()
    };
    if (!action.target_path) {
        result.error = 'target_path is required for search action';
        return result;
    }
    if (!action.content || typeof action.content !== 'string') {
        result.error = 'content (search query) is required for search action';
        return result;
    }
    try {
        const root = (0, json_js_1.get_path_root)(action.target_path);
        if (!root) {
            result.error = 'target_path must start with /working_memory/ or /storage/';
            return result;
        }
        const relative_path = (0, json_js_1.get_relative_path)(action.target_path);
        const target_data = root === 'working_memory' ? working_memory : storage;
        const search_results = (0, json_js_1.search_content)(target_data, relative_path, action.content);
        result.status = 'success';
        result.result = search_results;
    }
    catch (error) {
        result.error = `Search operation failed: ${error instanceof Error ? error.message : String(error)}`;
    }
    return result;
}

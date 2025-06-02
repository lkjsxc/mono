"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.handle_get_action = handle_get_action;
const json_js_1 = require("../util/json.js");
/**
 * Handle get action for both working_memory and storage
 */
async function handle_get_action(action, working_memory, storage, action_index) {
    const result = {
        action_index,
        status: 'error',
        timestamp: Date.now()
    };
    if (!action.target_path) {
        result.error = 'target_path is required for get action';
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
        const value = (0, json_js_1.get_value_at_path)(target_data, relative_path);
        if (value !== undefined) {
            result.status = 'success';
            result.result = value;
        }
        else {
            result.error = 'Path not found';
        }
    }
    catch (error) {
        result.error = `Get operation failed: ${error instanceof Error ? error.message : String(error)}`;
    }
    return result;
}

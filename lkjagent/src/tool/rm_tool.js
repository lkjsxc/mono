"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.handle_rm_action = handle_rm_action;
const json_js_1 = require("../util/json.js");
/**
 * Handle rm action for both working_memory and storage
 */
async function handle_rm_action(action, working_memory, storage, action_index) {
    const result = {
        action_index,
        status: 'error',
        timestamp: Date.now()
    };
    if (!action.target_path) {
        result.error = 'target_path is required for rm action';
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
        const success = (0, json_js_1.delete_value_at_path)(target_data, relative_path);
        if (success) {
            result.status = 'success';
            result.result = 'Data deleted successfully';
        }
        else {
            result.error = 'Failed to delete data at path (path may not exist)';
        }
    }
    catch (error) {
        result.error = `Delete operation failed: ${error instanceof Error ? error.message : String(error)}`;
    }
    return result;
}

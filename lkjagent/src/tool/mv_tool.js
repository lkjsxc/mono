"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.handle_mv_action = handle_mv_action;
const json_js_1 = require("../util/json.js");
/**
 * Handle mv action for both working_memory and storage
 */
async function handle_mv_action(action, working_memory, storage, action_index) {
    const result = {
        action_index,
        status: 'error',
        timestamp: Date.now()
    };
    if (!action.source_path) {
        result.error = 'source_path is required for mv action';
        return result;
    }
    if (!action.target_path) {
        result.error = 'target_path is required for mv action';
        return result;
    }
    try {
        const source_root = (0, json_js_1.get_path_root)(action.source_path);
        const target_root = (0, json_js_1.get_path_root)(action.target_path);
        if (!source_root || !target_root) {
            result.error = 'Both source_path and target_path must start with /working_memory/ or /storage/';
            return result;
        }
        const source_relative_path = (0, json_js_1.get_relative_path)(action.source_path);
        const target_relative_path = (0, json_js_1.get_relative_path)(action.target_path);
        const source_data = source_root === 'working_memory' ? working_memory : storage;
        const target_data = target_root === 'working_memory' ? working_memory : storage;
        // Get the value from source
        const value = (0, json_js_1.get_value_at_path)(source_data, source_relative_path);
        if (value === undefined) {
            result.error = 'Source path not found';
            return result;
        }
        // Set the value at target
        const set_success = (0, json_js_1.set_value_at_path)(target_data, target_relative_path, value);
        if (!set_success) {
            result.error = 'Failed to set data at target path';
            return result;
        }
        // Delete from source (only if source and target are different)
        if (action.source_path !== action.target_path) {
            const delete_success = (0, json_js_1.delete_value_at_path)(source_data, source_relative_path);
            if (!delete_success) {
                result.error = 'Data copied to target but failed to delete from source';
                return result;
            }
        }
        result.status = 'success';
        result.result = 'Data moved successfully';
    }
    catch (error) {
        result.error = `Move operation failed: ${error instanceof Error ? error.message : String(error)}`;
    }
    return result;
}

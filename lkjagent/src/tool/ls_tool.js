"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.handle_ls_action = handle_ls_action;
const json_js_1 = require("../util/json.js");
/**
 * Handle ls action for both working_memory and storage
 */
async function handle_ls_action(action, working_memory, storage, action_index) {
    const result = {
        action_index,
        status: 'error',
        timestamp: Date.now()
    };
    if (!action.target_path) {
        result.error = 'target_path is required for ls action';
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
        const contents = (0, json_js_1.list_contents_at_path)(target_data, relative_path);
        if (contents !== undefined) {
            result.status = 'success';
            result.result = contents;
        }
        else {
            result.error = 'Path not found or is not a directory';
        }
    }
    catch (error) {
        result.error = `List operation failed: ${error instanceof Error ? error.message : String(error)}`;
    }
    return result;
}

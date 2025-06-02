import { tool_action, action_result, json_object } from '../types/common';
import { get_path_root, get_relative_path, set_value_at_path, get_value_at_path } from '../util/json';

/**
 * Handle mkdir action for both working_memory and storage
 */
export async function handle_mkdir_action(
  action: tool_action,
  working_memory: json_object,
  storage: json_object,
  action_index: number
): Promise<action_result> {
  const result: action_result = {
    action_index,
    timestamp: Date.now(),
    kind: action.kind,
    target_path: action.target_path,
    status: 'error'
  };

  if (!action.target_path) {
    result.error = 'target_path is required for mkdir action';
    return result;
  }

  try {
    const root = get_path_root(action.target_path);
    if (!root) {
      result.error = 'target_path must start with /working_memory/ or /storage/';
      return result;
    }

    const relative_path = get_relative_path(action.target_path);
    const target_data = root === 'working_memory' ? working_memory : storage;

    // Check if directory already exists
    const existing_value = get_value_at_path(target_data, relative_path);
    if (existing_value !== undefined) {
      result.error = `Directory already exists at ${action.target_path}`;
      return result;
    }

    // Create directory with _is_directory marker
    const directory_structure = {
      _is_directory: true
    };

    const success = set_value_at_path(target_data, relative_path, directory_structure);
    
    if (success) {
      result.status = 'success';
      result.message = `Directory created successfully at ${action.target_path}`;
    } else {
      result.error = 'Failed to create directory';
    }
  } catch (error) {
    result.error = `Directory creation failed: ${error instanceof Error ? error.message : String(error)}`;
  }

  return result;
}

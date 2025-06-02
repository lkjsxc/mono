import { tool_action, action_result, json_object } from '../types/common';
import { get_path_root, get_relative_path, set_value_at_path } from '../util/json';

/**
 * Handle set action for both working_memory and storage
 */
export async function handle_set_action(
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
    result.error = 'target_path is required for set action';
    return result;
  }

  if (action.content === undefined) {
    result.error = 'content is required for set action';
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

    const success = set_value_at_path(target_data, relative_path, action.content);
    
    if (success) {
      result.status = 'success';
      result.message = `Value set successfully at ${action.target_path}`;
    } else {
      result.error = 'Failed to set data at path';
    }
  } catch (error) {
    result.error = `Set operation failed: ${error instanceof Error ? error.message : String(error)}`;
  }

  return result;
}

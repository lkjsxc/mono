import { tool_action, action_result, json_object } from '../types/common';
import { get_path_root, get_relative_path, get_value_at_path, set_value_at_path, delete_value_at_path } from '../util/json';

/**
 * Handle mv action for both working_memory and storage
 */
export async function handle_mv_action(
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
    source_path: action.source_path,
    status: 'error'
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
    const source_root = get_path_root(action.source_path);
    const target_root = get_path_root(action.target_path);
    
    if (!source_root || !target_root) {
      result.error = 'Both source_path and target_path must start with /working_memory/ or /storage/';
      return result;
    }

    const source_relative_path = get_relative_path(action.source_path);
    const target_relative_path = get_relative_path(action.target_path);
    
    const source_data = source_root === 'working_memory' ? working_memory : storage;
    const target_data = target_root === 'working_memory' ? working_memory : storage;

    // Get the value from source
    const value = get_value_at_path(source_data, source_relative_path);
    
    if (value === undefined) {
      result.error = `Source path not found: ${action.source_path}`;
      return result;
    }

    // Set the value at target
    const set_success = set_value_at_path(target_data, target_relative_path, value);
    
    if (!set_success) {
      result.error = `Failed to set data at target path: ${action.target_path}`;
      return result;
    }

    // Delete from source (only if source and target are different)
    if (action.source_path !== action.target_path) {
      const delete_success = delete_value_at_path(source_data, source_relative_path);
      
      if (!delete_success) {
        result.error = 'Data copied to target but failed to delete from source';
        return result;
      }
    }

    result.status = 'success';
    result.message = `Data moved successfully from ${action.source_path} to ${action.target_path}`;
  } catch (error) {
    result.error = `Move operation failed: ${error instanceof Error ? error.message : String(error)}`;
  }

  return result;
}

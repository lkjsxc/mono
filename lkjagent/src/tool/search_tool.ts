import { tool_action, action_result, json_object } from '../types/common';
import { get_path_root, get_relative_path, search_content } from '../util/json';

/**
 * Handle search action for both working_memory and storage
 */
export async function handle_search_action(
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
    result.error = 'target_path is required for search action';
    return result;
  }

  if (!action.content || typeof action.content !== 'string') {
    result.error = 'content (search query) is required for search action';
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

    const search_results = search_content(target_data, relative_path, action.content);
    
    result.status = 'success';
    result.data = search_results;
    const count = Object.keys(search_results).length;
    result.message = `Search for "${action.content}" in ${action.target_path} found ${count} matches`;
  } catch (error) {
    result.error = `Search operation failed: ${error instanceof Error ? error.message : String(error)}`;
  }

  return result;
}

import { tool_action, action_result, json_object } from '../types/common';
import { get_path_root, get_relative_path, list_contents_at_path } from '../util/json';

/**
 * Handle ls action for both working_memory and storage
 */
export async function handle_ls_action(
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
    result.error = 'target_path is required for ls action';
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

    const contents = list_contents_at_path(target_data, relative_path); if (contents !== undefined) {
      result.status = 'success';
      const count = Object.keys(contents).length;

      // Create formatted list of items with directory indicators
      const itemList = Object.entries(contents).map(([name, item]) => {
        const isDirectory = item && typeof item === 'object' && !Array.isArray(item) && (item as any)._is_directory === true;
        return isDirectory ? `${name}/ (dir)` : `${name} (file)`;
      });

      if (count === 0) {
        result.message = `Directory ${action.target_path} is empty`;
      } else {
        result.message = `Listed ${count} items in ${action.target_path}:\n${itemList.join('\n')}`;
      }
    } else {
      result.error = `Path not found or is not a directory: ${action.target_path}`;
    }
  } catch (error) {
    result.error = `List operation failed: ${error instanceof Error ? error.message : String(error)}`;
  }

  return result;
}

import { LogEntry, ToolAction, ToolKind } from '../types/common';
import { logAction } from '../tool/action_logger';
import { memory_set } from '../tool/memory_set';
import { memory_remove } from '../tool/memory_remove';
import { memory_mv } from '../tool/memory_mv';
import { storage_get } from '../tool/storage_get';
import { storage_remove } from '../tool/storage_remove';
import { storage_search } from '../tool/storage_search';
import { storage_set } from '../tool/storage_set';
import { storage_ls } from '../tool/storage_ls';
import { validateAction } from './action-validator';

/**
 * Execute a single tool action with error handling
 */
export async function executeAction(action: ToolAction): Promise<void> {
  // Create initial log entry
  const entry: LogEntry = {
    timestamp: Date.now(),
    actionType: action.kind as ToolKind,
    target_path: action.target_path,
    source_path: action.source_path,
    content: action.content,
    status: 'error'  // Default to error, will be updated to success if no error
  };

  try {
    // Validate action first
    const validationErrors = validateAction(action);
    if (validationErrors.length > 0) {
      console.warn(`Skipping ${action.kind} action: ${validationErrors.join(', ')}`);
      entry.error = validationErrors.join(', ');
      await logAction(entry);
      return;
    }

    // Handle each action type
    switch (action.kind) {
      case 'memory_set':
        await memory_set(action.target_path!, action.content);
        break;

      case 'memory_remove':
        await memory_remove(action.target_path!);
        break;

      case 'memory_mv':
        await memory_mv(action.source_path!, action.target_path!);
        break;

      case 'storage_set':
        await storage_set(action.source_path!, action.target_path!);
        break;

      case 'storage_remove':
        await storage_remove(action.target_path!);
        break;

      case 'storage_get':
        await memory_set('/sys/result_data', storage_get(action.target_path!));
        break;

      case 'storage_search':
        await memory_set('/sys/result_data', storage_search(action.content!));
        break;

      case 'storage_ls':
        await memory_set('/sys/result_data', storage_ls(action.target_path!));
        break;

      default:
        console.warn(`Skipping unknown action kind: ${action.kind}`);
        entry.error = `Unknown action kind: ${action.kind}`;
        await logAction(entry);
        return;
    }

    // If we get here, the action was successful
    entry.status = 'success';
    await logAction(entry);  } catch (error) {
    console.warn(`Error executing ${action.kind} action:`, error);
    entry.error = error instanceof Error ? error.message : String(error);
    await logAction(entry);
  }
}

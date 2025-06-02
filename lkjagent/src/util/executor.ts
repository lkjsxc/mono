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

// Global counter for action numbering
let actionCounter = 0;

/**
 * Get the next action number for result_data storage
 */
function getNextActionNumber(): number {
  return ++actionCounter;
}

/**
 * Reset the action counter (useful for testing or fresh starts)
 */
export function resetActionCounter(): void {
  actionCounter = 0;
}

/**
 * Get the current action counter value
 */
export function getCurrentActionCount(): number {
  return actionCounter;
}

/**
 * Execute a single tool action with error handling
 */
export async function executeAction(action: ToolAction): Promise<void> {
  const actionNumber = getNextActionNumber();
  
  // Create initial log entry
  const entry: LogEntry = {
    timestamp: Date.now(),
    actionType: action.kind as ToolKind,
    target_path: action.target_path,
    source_path: action.source_path,
    content: action.content,
    status: 'error'  // Default to error, will be updated to success if no error
  };

  try {    // Validate action first
    const validationErrors = validateAction(action);
    if (validationErrors.length > 0) {
      console.warn(`Skipping ${action.kind} action: ${validationErrors.join(', ')}`);
      entry.error = validationErrors.join(', ');
      
      // Store validation error in numbered result_data path
      const validationErrorInfo = {
        action_number: actionNumber,
        timestamp: Date.now(),
        action: action.kind,
        target_path: action.target_path,
        source_path: action.source_path,
        error: validationErrors.join(', '),
        error_type: 'validation',
        action_content: action.content
      };
        try {
        await memory_set(`/result_data/action_${actionNumber}`, validationErrorInfo);
      } catch (memoryError) {
        console.error('Failed to store validation error in result_data:', memoryError);
      }
      
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
        break;      case 'storage_get':
        const getResult = storage_get(action.target_path!);
        await memory_set(`/result_data/action_${actionNumber}`, {
          action_number: actionNumber,
          timestamp: Date.now(),
          action: action.kind,
          target_path: action.target_path,
          status: 'success',
          data: getResult
        });
        break;      case 'storage_search':
        const searchResult = storage_search(action.content!);
        await memory_set(`/result_data/action_${actionNumber}`, {
          action_number: actionNumber,
          timestamp: Date.now(),
          action: action.kind,
          search_content: action.content,
          status: 'success',
          data: searchResult
        });
        break;      case 'storage_ls':
        const lsResult = storage_ls(action.target_path!);
        await memory_set(`/result_data/action_${actionNumber}`, {
          action_number: actionNumber,
          timestamp: Date.now(),
          action: action.kind,
          target_path: action.target_path,
          status: 'success',
          data: lsResult
        });
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
    const errorMessage = error instanceof Error ? error.message : String(error);
    entry.error = errorMessage;
    
    // Store error information in numbered result_data path for continued execution
    const errorInfo = {
      action_number: actionNumber,
      timestamp: Date.now(),
      action: action.kind,
      target_path: action.target_path,
      source_path: action.source_path,
      error: errorMessage,
      error_type: 'execution',
      action_content: action.content
    };
      try {
      await memory_set(`/result_data/action_${actionNumber}`, errorInfo);
    } catch (memoryError) {
      console.error('Failed to store error in result_data:', memoryError);
    }
    
    await logAction(entry);
  }
}

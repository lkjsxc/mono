import { ToolAction } from '../types/common';

/**
 * Validate a tool action and its required fields
 * @param action The action to validate
 * @returns Array of validation errors, empty if valid
 */
export function validateAction(action: ToolAction): string[] {
  const errors: string[] = [];

  if (!action.kind) {
    errors.push('Missing action kind');
    return errors;
  }

  // Validate required fields based on action kind
  switch (action.kind) {
    case 'memory_set':
      if (!action.target_path) errors.push('Missing target_path');
      if (action.content === undefined) errors.push('Missing content');
      break;

    case 'memory_remove':
      if (!action.target_path) errors.push('Missing target_path');
      break;

    case 'memory_mv':
      if (!action.source_path) errors.push('Missing source_path');
      if (!action.target_path) errors.push('Missing target_path');
      break;

    case 'storage_get':
      if (!action.target_path) errors.push('Missing target_path');
      break;

    case 'storage_remove':
      if (!action.target_path) errors.push('Missing target_path');
      break;

    case 'storage_set':
      if (!action.source_path) errors.push('Missing source_path');
      if (!action.target_path) errors.push('Missing target_path');
      break;

    case 'storage_search':
      if (!action.content) errors.push('Missing content');
      break;

    case 'storage_ls':
      if (!action.target_path) errors.push('Missing target_path');
      break;

    default:
      errors.push(`Unknown action kind: ${action.kind}`);
      break;
  }

  return errors;
}

/**
 * Check if an action is valid
 * @param action The action to check
 * @returns True if valid, false otherwise
 */
export function isValidAction(action: ToolAction): boolean {
  return validateAction(action).length === 0;
}

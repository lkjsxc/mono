/**
 * Action validation utilities
 */

import { tool_action } from '../types/common';
import { is_valid_path } from './json';

/**
 * Validate a tool action
 */
export function validate_action(action: tool_action): { valid: boolean; error?: string } {
  // Check if kind is valid
  const valid_kinds = ['set', 'get', 'rm', 'mv', 'ls', 'search'];
  if (!valid_kinds.includes(action.kind)) {
    return { valid: false, error: `Invalid action kind: ${action.kind}` };
  }
  
  // Check if target_path is provided and valid
  if (!action.target_path) {
    return { valid: false, error: 'target_path is required' };
  }
  
  if (!is_valid_path(action.target_path)) {
    return { valid: false, error: `Invalid target_path: ${action.target_path}. Must start with /working_memory/ or /storage/` };
  }
  
  // Validate specific action requirements
  switch (action.kind) {
    case 'set':
      if (action.content === undefined) {
        return { valid: false, error: 'content is required for set action' };
      }
      break;
      
    case 'mv':
      if (!action.source_path) {
        return { valid: false, error: 'source_path is required for mv action' };
      }
      if (!is_valid_path(action.source_path)) {
        return { valid: false, error: `Invalid source_path: ${action.source_path}. Must start with /working_memory/ or /storage/` };
      }
      break;
      
    case 'search':
      if (action.content === undefined || action.content === '') {
        return { valid: false, error: 'content (search query) is required for search action' };
      }
      break;
      
    case 'get':
    case 'rm':
    case 'ls':
      // These actions only need target_path, which we already validated
      break;
      
    default:
      return { valid: false, error: `Unknown action kind: ${action.kind}` };
  }
  
  return { valid: true };
}

/**
 * Validate an array of actions
 */
export function validate_actions(actions: tool_action[]): { valid: boolean; errors: string[] } {
  const errors: string[] = [];
  
  for (let i = 0; i < actions.length; i++) {
    const validation = validate_action(actions[i]);
    if (!validation.valid) {
      errors.push(`Action ${i + 1}: ${validation.error}`);
    }
  }
  
  return {
    valid: errors.length === 0,
    errors
  };
}

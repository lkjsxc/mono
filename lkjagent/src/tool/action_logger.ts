import * as fs from 'fs-extra';
import * as path from 'path';
import { log_entry, tool_action, action_result } from '../types/common';

const DATA_DIR = path.join(process.cwd(), 'data');
const LOG_FILE = path.join(DATA_DIR, 'log.json');

/**
 * Load action log from log.json
 */
export async function load_action_log(): Promise<log_entry[]> {
  try {
    if (await fs.pathExists(LOG_FILE)) {
      return await fs.readJson(LOG_FILE);
    } else {
      return [];
    }
  } catch (error) {
    console.error('Error loading action log:', error);
    return [];
  }
}

/**
 * Save action log to log.json
 */
export async function save_action_log(log: log_entry[]): Promise<void> {
  try {
    await fs.ensureDir(DATA_DIR);
    await fs.writeJson(LOG_FILE, log, { spaces: 2 });
  } catch (error) {
    console.error('Error saving action log:', error);
    throw error;
  }
}

/**
 * Add a log entry to the action log
 */
export async function log_action(action: tool_action, result: action_result): Promise<void> {
  try {
    const log = await load_action_log();
    
    const log_entry: log_entry = {
      timestamp: Date.now(),
      total_action_index: result.action_index,
      kind: action.kind,
      target_path: action.target_path,
      source_path: action.source_path,
      content_summary: action.content ? JSON.stringify(action.content).substring(0, 100) : undefined,
      status: result.status,
      message: result.message,
      error: result.error,
      result_summary: generate_result_summary(action, result)
    };
    
    log.push(log_entry);
    
    await save_action_log(log);
  } catch (error) {
    console.error('Error logging action:', error);
    throw error;
  }
}

/**
 * Trim log to maximum number of entries
 */
export async function trim_log(max_entries: number): Promise<void> {
  try {
    const log = await load_action_log();
    
    if (log.length > max_entries) {
      const trimmed_log = log.slice(-max_entries);
      await save_action_log(trimmed_log);
    }
  } catch (error) {
    console.error('Error trimming log:', error);
    throw error;
  }
}

/**
 * Generate a human-readable summary of the action result
 */
function generate_result_summary(action: tool_action, result: action_result): string {
  const action_desc = `${action.kind}${action.target_path ? ` ${action.target_path}` : ''}`;
  
  if (result.status === 'error') {
    return `${action_desc} failed: ${result.error}`;
  }
  
  switch (action.kind) {
    case 'set':
      return `${action_desc} completed successfully`;
    case 'get':
      return `${action_desc} retrieved data`;
    case 'rm':
      return `${action_desc} deleted successfully`;
    case 'mv':
      return `${action.source_path} moved to ${action.target_path}`;
    case 'ls':
      const count = result.data ? Object.keys(result.data).length : 0;
      return `${action_desc} listed ${count} items`;
    case 'search':
      const results = result.data ? Object.keys(result.data).length : 0;
      return `${action_desc} found ${results} matches`;
    default:
      return `${action_desc} completed`;
  }
}

/**
 * Get recent log entries
 */
export async function get_recent_log_entries(count: number = 10): Promise<log_entry[]> {
  try {
    const log = await load_action_log();
    return log.slice(-count);
  } catch (error) {
    console.error('Error getting recent log entries:', error);
    return [];
  }
}

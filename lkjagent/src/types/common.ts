/**
 * Core interfaces and types for lkjagent framework
 */

// Configuration interface - all root level properties
export interface app_config {
  working_memory_character_max: number;
  key_token_max?: number;
  dir_child_max?: number;
  llm_api_url?: string;
  llm_model?: string;
  llm_max_tokens?: number;
  llm_temperature?: number;
  system_max_log_entries?: number;
  system_auto_cleanup?: boolean;
  system_debug_mode: boolean;
}

// Tool action kinds
export type tool_kind = 
  | 'set'
  | 'get'
  | 'rm'
  | 'mv'
  | 'ls'
  | 'search';

// Tool action interface
export interface tool_action {
  kind: tool_kind;
  target_path: string;     // Path for set, get, rm, ls, search (scope), mv (destination).
                           // Must start with /working_memory/ or /storage/.
  source_path?: string;   // For mv (source only). Must start with /working_memory/ or /storage/.
  content?: any;          // Data for 'set', query for 'search'. Expected to be simple json_like data if complex.
}

// Action result interface (Structure within `/working_memory/action_result/_N`)
export interface action_result extends json_object {
  action_index: number; // The N in _N
  timestamp: number;
  kind: tool_kind;
  target_path: string;
  source_path?: string; // if applicable (e.g. for mv)
  status: 'success' | 'error';
  data?: any; // For 'get', 'ls', 'search' results
  message?: string; // For 'set', 'rm', 'mv' success, or general info
  error?: string; // Error message if status is 'error'
}

// Log entry interface (Reflects cumulative numbering)
export interface log_entry {
  timestamp: number;
  total_action_index: number; // Cumulative index for this action
  kind: tool_kind;
  target_path?: string;
  source_path?: string;
  content_summary?: string;
  status: 'success' | 'error';
  message?: string;
  error?: string;
  result_summary?: string;
}

// JSON value types for type safety
export type json_value = 
  | string 
  | number 
  | boolean 
  | null 
  | undefined
  | json_object 
  | json_value[];

export interface json_object {
  [key: string]: json_value;
}

// Memory structure interfaces
export interface working_memory_structure {
  working_memory: {
    user_data: json_object;
    action_result: { [key: string]: action_result };
    system_info: json_object;
  };
}

export interface storage_structure {
  storage: json_object;
}

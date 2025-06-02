/**
 * Type definitions for lkjagent's memory architecture and tool interfaces.
 */

/**
 * Structure of a task in the todo list
 */
export interface TodoItem {
  task_description: string;
  status: 'pending' | 'in_progress' | 'completed';
  details?: string;
  sub_tasks?: Record<string, TodoItem>;
}

/**
 * Structure representing memory content
 */
export interface RamData {
  current_task?: {
    id: string;
    description: string;
    status: 'pending' | 'in_progress' | 'completed';
  };
  thinking_log: string[];
  todo: Record<string, TodoItem>;
  loaded_data?: any; // Temporary area for data loaded from storage
  [key: string]: any; // Allow additional dynamic paths
}

/**
 * Structure representing Storage content
 */
export interface StorageData {
  knowledge_base: {
    system_policy_summary: string;
    greeting_message: string;
    [key: string]: any;
  };
  archived_data: Record<string, any>;
  [key: string]: any; // Allow additional dynamic paths
}

/**
 * Complete memory state including memory
 */
export interface MemoryState {
  ram: RamData;
}

/**
 * Complete storage state
 */
export interface StorageState {
  storage: StorageData;
}

/**
 * Kinds of tools available to the agent
 */
export type ToolKind = 'memory_set' | 'memory_remove' | 'memory_mv' | 'storage_get' | 'storage_set' | 'storage_search' | 'storage_remove' | 'storage_ls' | 'think_log' | 'think_log_get' | 'think_log_get_range' | 'think_log_count';

/**
 * Structure of an action the agent can execute
 */
export interface ToolAction {
  kind: ToolKind;
  target_path?: string;        // Path in memory or Storage to operate on
  source_path?: string;        // For storage_set action
  content?: any;               // Content to add/edit/search
}

/**
 * Result of a storage search operation
 */
export interface SearchResult {
  path: string;
  content: any;
  relevance?: number;
}

/**
 * Result of a storage_ls operation
 */
export interface StorageLsResult {
  key: string;
  stringLength: number;
}

/**
 * Tool function signatures
 */
export interface ToolFunctions {
  memory_set: (target_path: string, content: any) => Promise<void>;
  memory_remove: (target_path: string) => Promise<void>;
  memory_mv: (source_path: string, target_path: string) => Promise<void>;
  storage_get: (target_path: string) => Promise<any>;
  storage_remove: (target_path: string) => Promise<void>;
  storage_search: (content: string) => Promise<SearchResult[]>;
  storage_set: (source_path: string, target_path: string) => Promise<void>;
  storage_ls: (target_path: string) => Promise<StorageLsResult[]>;
  think_log: (content: string) => Promise<void>;
  think_log_get: (index: number) => Promise<any>;
  think_log_get_range: (start: number, end?: number) => Promise<any[]>;
  think_log_count: () => Promise<number>;
}

/**
 * Helper type for JSON paths
 * Example: "ram.todo.new_task" or "storage.knowledge_base.policy"
 */
export type JsonPath = string;

/**
 * Helper type for XML string format used in LLM communication
 */
export type XmlString = string;

/**
 * Structure of a log entry
 */
export interface LogEntry {
  timestamp: number;
  actionType: ToolKind;
  target_path?: string;
  source_path?: string;
  content?: any;
  status: 'success' | 'error';
  error?: string;
}
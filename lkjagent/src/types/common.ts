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
 * Structure representing RAM content
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
 * Complete memory state including RAM
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
export type ToolKind = 'add' | 'remove' | 'edit' | 'storage_load' | 'storage_store' | 'storage_search';

/**
 * Structure of an action the agent can execute
 */
export interface ToolAction {
  kind: ToolKind;
  path?: string;        // Path in RAM or Storage to operate on
  content?: any;        // Content to add/edit/search
  source_path?: string; // For storage_store action
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
 * Tool function signatures
 */
export interface ToolFunctions {
  ram_add: (path: string, content: any) => Promise<void>;
  ram_remove: (path: string) => Promise<void>;
  storage_load: (path: string) => Promise<any>;
  storage_remove: (path: string) => Promise<void>;
  storage_search: (content: string) => Promise<SearchResult[]>;
  storage_store: (source_path: string, destination_path: string) => Promise<void>;
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
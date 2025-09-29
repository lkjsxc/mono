import type { AgentConfig } from "../config/types.js";

export interface WorkingMemory {
  readonly entries: Record<string, string>;
}

export interface StorageMemory {
  readonly entries: Record<string, string>;
}

export interface AgentMemorySnapshot {
  readonly state: string;
  readonly iteration: number;
  readonly workingMemory: WorkingMemory;
  readonly storage: StorageMemory;
}

export type AgentAction =
  | {
      readonly type: "working_memory_add";
      readonly tags: string;
      readonly value: string;
    }
  | {
      readonly type: "working_memory_remove";
      readonly tags: string;
    }
  | {
      readonly type: "storage_save";
      readonly tags: string;
      readonly value: string;
    }
  | {
      readonly type: "storage_load";
      readonly tags: string;
    }
  | {
      readonly type: "storage_search";
      readonly tags: string;
      readonly value?: string;
    };

export type AgentActions = readonly AgentAction[];

export interface AgentContext {
  readonly config: AgentConfig;
  readonly memory: AgentMemorySnapshot;
}

export interface AgentIterationResult {
  readonly memory: AgentMemorySnapshot;
  readonly actions: AgentActions;
  readonly nextState: string;
}

export interface PromptSection {
  readonly name: string;
  readonly content: string;
  readonly estimatedTokens: number;
}

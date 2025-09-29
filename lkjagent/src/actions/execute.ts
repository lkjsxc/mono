import type { AgentAction, AgentMemorySnapshot } from "../domain/types.js";
import {
  executeWorkingMemoryAdd,
  executeWorkingMemoryRemove,
} from "./workingMemory.js";
import {
  executeStorageLoad,
  executeStorageSave,
  executeStorageSearch,
} from "./storage.js";

export const executeAction = (
  memory: AgentMemorySnapshot,
  action: AgentAction,
  iteration: number,
): AgentMemorySnapshot => {
  switch (action.type) {
    case "working_memory_add":
      return executeWorkingMemoryAdd(memory, action.tags, action.value, iteration);
    case "working_memory_remove":
      return executeWorkingMemoryRemove(memory, action.tags);
    case "storage_save":
      return executeStorageSave(memory, action.tags, action.value);
    case "storage_load":
      return executeStorageLoad(memory, action.tags, iteration);
    case "storage_search":
      return executeStorageSearch(memory, action.tags, action.value, iteration);
    default: {
      const exhaustive: never = action;
      throw new Error(`Unsupported action type ${(exhaustive as { type: string }).type}`);
    }
  }
};

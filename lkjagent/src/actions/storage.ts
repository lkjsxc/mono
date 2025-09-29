import type { AgentMemorySnapshot } from "../domain/types.js";
import { loadFromStorage, saveToStorage, searchStorage } from "../memory/storage.js";

export const executeStorageSave = (
  memory: AgentMemorySnapshot,
  tags: string,
  value: string,
): AgentMemorySnapshot => saveToStorage(memory, tags, value);

export const executeStorageLoad = (
  memory: AgentMemorySnapshot,
  tags: string,
  iteration: number,
): AgentMemorySnapshot => loadFromStorage(memory, tags, iteration);

export const executeStorageSearch = (
  memory: AgentMemorySnapshot,
  tags: string,
  value: string | undefined,
  iteration: number,
): AgentMemorySnapshot => searchStorage(memory, tags, value, iteration);

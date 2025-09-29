import type { AgentMemorySnapshot } from "../domain/types.js";
import { addWorkingMemoryEntry, removeWorkingMemoryEntries } from "../memory/working.js";

export const executeWorkingMemoryAdd = (
  memory: AgentMemorySnapshot,
  tags: string,
  value: string,
  iteration: number,
): AgentMemorySnapshot => addWorkingMemoryEntry(memory, tags, value, iteration);

export const executeWorkingMemoryRemove = (
  memory: AgentMemorySnapshot,
  tags: string,
): AgentMemorySnapshot => removeWorkingMemoryEntries(memory, tags);

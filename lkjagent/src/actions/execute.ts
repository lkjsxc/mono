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

const warnOnValueFormat = (actionType: string, tags: string, value: string): void => {
  const trimmed = value.trim();
  if (!trimmed) {
    return;
  }

  const approxTokens = trimmed.split(/\s+/).filter(Boolean).length;
  if (approxTokens >= 100) {
    console.warn(
      `[lkjagent] ${actionType} on tags '${tags}' produced a value approximating ${approxTokens} tokens; consider splitting it into smaller entries.`,
    );
  }

  const looksJson = /^(\s*[{\[])/.test(trimmed);
  const looksXml = /^(\s*<[^>]+>)/.test(trimmed) && /<[^>]+>\s*$/.test(trimmed);
  const looksCsv = trimmed.includes("\n") && /,/.test(trimmed);

  if (looksJson || looksXml || looksCsv) {
    console.warn(
      `[lkjagent] ${actionType} on tags '${tags}' appears to contain structured data (JSON/XML/CSV). Rewrite it as plain Markdown text.`,
    );
  }
};

export const executeAction = (
  memory: AgentMemorySnapshot,
  action: AgentAction,
  iteration: number,
): AgentMemorySnapshot => {
  switch (action.type) {
    case "working_memory_add":
      warnOnValueFormat(action.type, action.tags, action.value);
      return executeWorkingMemoryAdd(memory, action.tags, action.value, iteration);
    case "working_memory_remove":
      return executeWorkingMemoryRemove(memory, action.tags);
    case "storage_save":
      warnOnValueFormat(action.type, action.tags, action.value);
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

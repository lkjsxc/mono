import type { AgentConfig } from "../config/types.js";
import type { AgentMemorySnapshot } from "../domain/types.js";
import { clampWorkingMemoryEntries } from "./working.js";

const sanitizeLimit = (value: unknown): number | undefined => {
  if (typeof value !== "number" || !Number.isFinite(value)) {
    return undefined;
  }
  const normalized = Math.max(0, Math.floor(value));
  return normalized;
};

export const resolveWorkingMemoryCleanupLimit = (
  config: AgentConfig,
): number | undefined => {
  const limit = config.agent.memory_system?.working_memory?.auto_cleanup_limit;
  return sanitizeLimit(limit);
};

export const applyWorkingMemoryCleanup = (
  memory: AgentMemorySnapshot,
  config: AgentConfig,
): AgentMemorySnapshot => {
  const limit = resolveWorkingMemoryCleanupLimit(config);
  if (limit === undefined) {
    return memory;
  }
  return clampWorkingMemoryEntries(memory, limit);
};

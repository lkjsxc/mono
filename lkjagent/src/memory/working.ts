import { normalizeTags, tagsMatchPrefix } from "../domain/tags.js";
import type { AgentMemorySnapshot } from "../domain/types.js";

const withUpdatedWorkingMemory = (
  memory: AgentMemorySnapshot,
  entries: Record<string, string>,
): AgentMemorySnapshot => ({
  ...memory,
  workingMemory: { entries },
});

const extractIteration = (key: string): number | undefined => {
  const marker = "iteration_";
  const index = key.lastIndexOf(marker);
  if (index < 0) return undefined;
  const slice = key.substring(index + marker.length);
  const value = Number.parseInt(slice, 10);
  return Number.isFinite(value) ? value : undefined;
};

const orderWorkingMemoryKeys = (entries: Record<string, string>): string[] =>
  Object.keys(entries).sort((a, b) => {
    const iterationA = extractIteration(a) ?? Number.MAX_SAFE_INTEGER;
    const iterationB = extractIteration(b) ?? Number.MAX_SAFE_INTEGER;
    if (iterationA !== iterationB) {
      return iterationA - iterationB;
    }
    return a.localeCompare(b);
  });

export const addWorkingMemoryEntry = (
  memory: AgentMemorySnapshot,
  rawTags: string,
  value: string,
  iteration: number,
): AgentMemorySnapshot => {
  const normalizedTags = normalizeTags(rawTags);
  const key = `${normalizedTags}${normalizedTags ? "," : ""}iteration_${iteration}`;
  return withUpdatedWorkingMemory(memory, {
    ...memory.workingMemory.entries,
    [key]: value,
  });
};

export const removeWorkingMemoryEntries = (
  memory: AgentMemorySnapshot,
  rawTags: string,
): AgentMemorySnapshot => {
  const normalizedTags = normalizeTags(rawTags);
  const entries = Object.entries(memory.workingMemory.entries).reduce<Record<string, string>>(
    (acc, [key, value]) => {
      if (!tagsMatchPrefix(normalizedTags, key)) {
        acc[key] = value;
      }
      return acc;
    },
    {},
  );

  return withUpdatedWorkingMemory(memory, entries);
};

export const clampWorkingMemoryEntries = (
  memory: AgentMemorySnapshot,
  limit: number,
): AgentMemorySnapshot => {
  if (limit <= 0) {
    return withUpdatedWorkingMemory(memory, {});
  }
  const entries = memory.workingMemory.entries;
  const keys = orderWorkingMemoryKeys(entries);
  if (keys.length <= limit) {
    return memory;
  }

  const keysToKeep = new Set(keys.slice(-limit));
  const filtered = keys.reduce<Record<string, string>>((acc, key) => {
    if (keysToKeep.has(key)) {
      acc[key] = entries[key];
    }
    return acc;
  }, {});

  return withUpdatedWorkingMemory(memory, filtered);
};

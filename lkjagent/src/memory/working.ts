import { normalizeTags, tagsMatchPrefix } from "../domain/tags.js";
import type { AgentMemorySnapshot } from "../domain/types.js";

const withUpdatedWorkingMemory = (
  memory: AgentMemorySnapshot,
  entries: Record<string, string>,
): AgentMemorySnapshot => ({
  ...memory,
  workingMemory: { entries },
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

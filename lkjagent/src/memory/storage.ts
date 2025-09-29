import { normalizeTags, searchTagsInEntry } from "../domain/tags.js";
import type { AgentMemorySnapshot } from "../domain/types.js";
import { addWorkingMemoryEntry, stripIterationSuffix } from "./working.js";

const withStorageEntries = (
  memory: AgentMemorySnapshot,
  entries: Record<string, string>,
): AgentMemorySnapshot => ({
  ...memory,
  storage: { entries },
});

export const writeStorageEntry = (
  memory: AgentMemorySnapshot,
  key: string,
  value: string,
): AgentMemorySnapshot =>
  withStorageEntries(memory, {
    ...memory.storage.entries,
    [key]: value,
  });

const valueContains = (needle: string | undefined, haystack: string): boolean => {
  if (!needle) return true;
  if (!needle.length) return true;
  return haystack.toLowerCase().includes(needle.toLowerCase());
};

export const saveToStorage = (
  memory: AgentMemorySnapshot,
  rawTags: string,
  value: string,
): AgentMemorySnapshot => {
  const normalizedTags = normalizeTags(rawTags);
  return writeStorageEntry(memory, normalizedTags, value);
};

export const loadFromStorage = (
  memory: AgentMemorySnapshot,
  rawTags: string,
  iteration: number,
): AgentMemorySnapshot => {
  const normalizedTags = normalizeTags(rawTags);
  let current = memory;
  for (const [tags, value] of Object.entries(memory.storage.entries)) {
    if (searchTagsInEntry(normalizedTags, tags)) {
      current = addWorkingMemoryEntry(current, tags, value, iteration);
    }
  }
  return current;
};

export const searchStorage = (
  memory: AgentMemorySnapshot,
  rawTags: string,
  rawValue: string | undefined,
  iteration: number,
): AgentMemorySnapshot => {
  const normalizedTags = normalizeTags(rawTags);
  let matches = 0;
  let current = memory;
  for (const [tags, value] of Object.entries(memory.storage.entries)) {
    if (!searchTagsInEntry(normalizedTags, tags)) continue;
    if (!valueContains(rawValue, value)) continue;
    matches += 1;
    current = addWorkingMemoryEntry(current, tags, value, iteration);
  }

  const summaryValue = `found ${matches} matches for tags:[${normalizedTags || "any"}] value:[${
    rawValue && rawValue.length ? rawValue : "any"
  }]`;
  return addWorkingMemoryEntry(current, "search_results,summary", summaryValue, iteration);
};

export const archiveWorkingEntries = (
  memory: AgentMemorySnapshot,
  keys: readonly string[],
): AgentMemorySnapshot => {
  if (keys.length === 0) return memory;
  let current = memory;
  for (const key of keys) {
    const storageKey = stripIterationSuffix(key) || key;
    const value = current.workingMemory.entries[key];
    if (value === undefined) continue;
    current = saveToStorage(current, storageKey, value);
  }
  return current;
};

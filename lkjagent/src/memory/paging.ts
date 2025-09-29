import type { AgentConfig } from "../config/types.js";
import type { AgentMemorySnapshot } from "../domain/types.js";
import { writeStorageEntry } from "./storage.js";

const extractIteration = (key: string): number | undefined => {
  const marker = "iteration_";
  const index = key.lastIndexOf(marker);
  if (index < 0) return undefined;
  const slice = key.substring(index + marker.length);
  const value = Number.parseInt(slice, 10);
  return Number.isFinite(value) ? value : undefined;
};

const stripIteration = (key: string): string => {
  const marker = ",iteration_";
  const index = key.lastIndexOf(marker);
  if (index < 0) return key;
  return key.slice(0, index);
};

const calculatePriority = (
  key: string,
  value: string,
  currentIteration: number,
): number => {
  const iteration = extractIteration(key);
  let priority = 0;

  if (iteration === undefined) {
    priority += 25;
  } else {
    const age = Math.max(0, currentIteration - iteration);
    if (age <= 1) priority += 50;
    else if (age <= 5) priority += 30;
    else if (age <= 15) priority += 15;
    else priority += 5;
  }

  if (key.includes("thinking_notes") || key.includes("evaluation_notes")) {
    priority += 30;
  } else if (key.includes("search_results") || key.includes("summary")) {
    priority += 15;
  } else {
    priority += 10;
  }

  if (value.length < 200) priority += 20;
  else if (value.length < 800) priority += 10;

  return Math.min(priority, 100);
};

const workingMemorySize = (memory: AgentMemorySnapshot): number =>
  JSON.stringify(memory.workingMemory.entries).length;

const getThreshold = (config: AgentConfig): number | undefined => {
  const pagingLimit = config.agent.paging_limit;
  if (pagingLimit?.enable && pagingLimit.value) {
    return pagingLimit.value;
  }
  const memoryPaging = config.agent.memory_system?.paging;
  if (memoryPaging?.enable && memoryPaging.context_threshold) {
    return memoryPaging.context_threshold;
  }
  return undefined;
};

const removeKey = (entries: Record<string, string>, key: string) => {
  const { [key]: _removed, ...rest } = entries;
  return rest;
};

const withWorkingEntries = (
  memory: AgentMemorySnapshot,
  entries: Record<string, string>,
): AgentMemorySnapshot => ({
  ...memory,
  workingMemory: { entries },
});

export const applyPaging = (
  memory: AgentMemorySnapshot,
  config: AgentConfig,
  iteration: number,
): AgentMemorySnapshot => {
  const threshold = getThreshold(config);
  if (!threshold) return memory;

  let current = memory;
  let size = workingMemorySize(current);
  if (size <= threshold) {
    return current;
  }

  const target = Math.floor(threshold * 0.8);
  const candidates = Object.entries(current.workingMemory.entries)
    .map(([key, value]) => ({ key, value, priority: calculatePriority(key, value, iteration) }))
    .sort((a, b) => a.priority - b.priority);

  for (const candidate of candidates) {
    if (size <= target) break;
    const storageKey = candidate.key;
    const storageValue = candidate.value;

  current = writeStorageEntry(current, storageKey, storageValue);
  const updatedWorking = removeKey(current.workingMemory.entries, candidate.key);
  current = withWorkingEntries(current, updatedWorking);
  current = { ...current, state: "paging" };
    size = workingMemorySize(current);
  }

  return current;
};

import { describe, expect, it } from "vitest";
import type { AgentMemorySnapshot } from "../src/domain/types.js";
import { saveToStorage, loadFromStorage, searchStorage } from "../src/memory/storage.js";
import { addWorkingMemoryEntry } from "../src/memory/working.js";

const buildMemory = (): AgentMemorySnapshot => ({
  state: "analyzing",
  iteration: 0,
  workingMemory: { entries: {} },
  storage: { entries: {} },
});

describe("storage", () => {
  it("saves and loads entries back into working memory", () => {
    let memory = buildMemory();
    memory = saveToStorage(memory, "alpha,beta", "value");
    memory = loadFromStorage(memory, "alpha", 1);
    expect(Object.keys(memory.workingMemory.entries)).toHaveLength(1);
    const [key, value] = Object.entries(memory.workingMemory.entries)[0];
    expect(key.startsWith("alpha,beta")).toBe(true);
    expect(value).toBe("value");
  });

  it("searches storage and appends summary entry", () => {
    let memory = buildMemory();
    memory = saveToStorage(memory, "alpha", "Ancient knowledge about dragons");
    memory = saveToStorage(memory, "beta", "Lore about knights");
    memory = searchStorage(memory, "alpha", "dragons", 2);
    expect(Object.keys(memory.workingMemory.entries).length).toBeGreaterThanOrEqual(1);
    const summary = Object.entries(memory.workingMemory.entries).find(([tags]) =>
      tags.startsWith("search_results,summary"),
    );
    expect(summary).toBeDefined();
    expect(summary?.[1]).toContain("found 1 matches");
  });

  it("adds working memory entries directly", () => {
    let memory = buildMemory();
    memory = addWorkingMemoryEntry(memory, "alpha", "insight", 5);
    expect(Object.keys(memory.workingMemory.entries)[0]).toContain("iteration_5");
  });
});

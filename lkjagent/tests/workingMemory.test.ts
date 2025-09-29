import { describe, expect, it } from "vitest";
import type { AgentMemorySnapshot } from "../src/domain/types.js";
import { clampWorkingMemoryEntries } from "../src/memory/working.js";

const buildMemory = (entries: Record<string, string>): AgentMemorySnapshot => ({
  state: "analyzing",
  workingMemory: { entries },
  storage: { entries: {} },
});

describe("clampWorkingMemoryEntries", () => {
  it("retains only the newest entries when the limit is exceeded", () => {
    const memory = buildMemory({
      "alpha,iteration_1": "First",
      "beta,iteration_2": "Second",
      "gamma,iteration_3": "Third",
    });

    const clamped = clampWorkingMemoryEntries(memory, 2);
    const keys = Object.keys(clamped.workingMemory.entries);
    expect(keys).toHaveLength(2);
    expect(keys.some((key) => key.includes("iteration_2"))).toBe(true);
    expect(keys.some((key) => key.includes("iteration_3"))).toBe(true);
    expect(keys.some((key) => key.includes("iteration_1"))).toBe(false);
  });

  it("clears all entries when the limit is zero", () => {
    const memory = buildMemory({
      "alpha,iteration_1": "First",
    });

    const clamped = clampWorkingMemoryEntries(memory, 0);
    expect(Object.keys(clamped.workingMemory.entries)).toHaveLength(0);
  });
});

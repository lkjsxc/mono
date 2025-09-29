import { describe, expect, it } from "vitest";
import type { AgentConfig } from "../src/config/types.js";
import type { AgentMemorySnapshot } from "../src/domain/types.js";
import { applyPaging } from "../src/memory/paging.js";

const buildConfig = (): AgentConfig => ({
  llm: {
    endpoint: "http://example.com",
    model: "test-model",
  },
  agent: {
    roles: {
      active_role: "test_role",
      available_roles: {
        test_role: {
          identity: "Test",
          creative_focus: "Testing",
          knowledge_domains: ["qa"],
        },
      },
    },
    prompts: {},
    memory_system: {
      paging: {
        enable: true,
        context_threshold: 200,
      },
    },
    paging_limit: {
      enable: true,
      value: 200,
    },
    iteration_limit: { enable: false },
  },
});

describe("applyPaging", () => {
  it("moves low-priority entries into storage when threshold exceeded", () => {
    const memory: AgentMemorySnapshot = {
      state: "creating",
      iteration: 0,
      workingMemory: {
        entries: {
          "general,iteration_0": "X".repeat(300),
          "thinking_notes,iteration_9": "Recent insight",
        },
      },
      storage: { entries: {} },
    };

    const updated = applyPaging(memory, buildConfig(), 10);
    expect(Object.keys(updated.workingMemory.entries)).toEqual(["thinking_notes,iteration_9"]);
    expect(updated.storage.entries["general,iteration_0"]).toBe("X".repeat(300));
    expect(updated.state).toBe("thinking");
  });
});

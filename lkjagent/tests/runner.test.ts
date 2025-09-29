import { beforeEach, describe, expect, it, vi } from "vitest";
import type { AgentConfig } from "../src/config/types.js";
import type { AgentMemorySnapshot } from "../src/domain/types.js";

vi.mock("../src/llm/client.js", () => ({
  requestCompletion: vi.fn(),
}));

type MockedLlmClient = {
  requestCompletion: ReturnType<typeof vi.fn>;
};

const loadLlmClient = async (): Promise<MockedLlmClient> =>
  (await import("../src/llm/client.js")) as unknown as MockedLlmClient;

const baseConfig: AgentConfig = {
  llm: {
    endpoint: "http://example.com",
    model: "offline-mock",
  },
  agent: {
    roles: {
      active_role: "test_role",
      available_roles: {
        test_role: {
          identity: "Quality Guardian",
          creative_focus: "Ensure deterministic output",
          knowledge_domains: ["testing", "quality"],
        },
      },
    },
    memory_system: {},
    prompts: {},
    states: {
      analyzing: {
        purpose: "Assess",
        mode: "thinking_mode",
      },
      creating: {
        purpose: "Create",
        mode: "action_mode",
      },
    },
    paging_limit: { enable: false },
    iteration_limit: { enable: false },
  },
};

const buildMemory = (): AgentMemorySnapshot => ({
  state: "analyzing",
  iteration: 0,
  workingMemory: { entries: {} },
  storage: { entries: {} },
});

describe("executeIteration", () => {
  beforeEach(async () => {
    const { requestCompletion } = await loadLlmClient();
    requestCompletion.mockReset();
  });

  it("applies the action returned by the LLM response", async () => {
    const { requestCompletion } = await loadLlmClient();
    requestCompletion.mockResolvedValue({
      content:
        "<agent><state>creating</state><actions><action><type>working_memory_add</type><tags>story,idea</tags><value>Draft a new narrative.</value></action></actions></agent>",
    });

    const { executeIteration } = await import("../src/agent/runner.js");
    const result = await executeIteration(baseConfig, buildMemory(), 3);

    expect(result.nextState).toBe("paging");
    expect(result.actions).toHaveLength(1);
    const entries = result.memory.workingMemory.entries;
    const keys = Object.keys(entries);
    expect(keys).toHaveLength(1);
    expect(keys[0]).toBe("idea,story,iteration_3");
    expect(entries[keys[0]]).toBe("Draft a new narrative.");
    expect(result.memory.storage.entries).toHaveProperty("idea,story");
    expect(result.memory.storage.entries["idea,story"]).toBe("Draft a new narrative.");
  });

  it("records an error entry when the LLM request fails", async () => {
    const { requestCompletion } = await loadLlmClient();
    requestCompletion.mockRejectedValue(new Error("network unreachable"));

    const { executeIteration } = await import("../src/agent/runner.js");
    const result = await executeIteration(baseConfig, buildMemory(), 1);

  expect(result.nextState).toBe("paging");
  expect(result.actions).toHaveLength(1);
    const entries = result.memory.workingMemory.entries;
    const keys = Object.keys(entries);
    expect(keys).toHaveLength(1);
    const [key] = keys;
    expect(key.startsWith("error,system,iteration_1")).toBe(true);
    expect(entries[key]).toContain("network unreachable");
    expect(entries[key]).toContain("Iteration 1 failed");
  });

  it("trims working memory according to the auto cleanup limit", async () => {
    const { requestCompletion } = await loadLlmClient();
    requestCompletion.mockResolvedValue({
      content:
        "<agent><state>creating</state><actions><action><type>working_memory_add</type><tags>story,idea</tags><value>Draft a new narrative.</value></action></actions></agent>",
    });

    const limitedConfig: AgentConfig = JSON.parse(JSON.stringify(baseConfig));
    limitedConfig.agent.memory_system = {
      working_memory: {
        auto_cleanup_limit: 2,
      },
    };

    const memory: AgentMemorySnapshot = {
      ...buildMemory(),
      iteration: 3,
      workingMemory: {
        entries: {
          "alpha,iteration_1": "First",
          "beta,iteration_2": "Second",
        },
      },
    };

    const { executeIteration } = await import("../src/agent/runner.js");
    const result = await executeIteration(limitedConfig, memory, 5);

    const entries = result.memory.workingMemory.entries;
    const keys = Object.keys(entries);
    expect(keys).toHaveLength(2);
    expect(keys.some((key) => key.includes("iteration_2"))).toBe(true);
  expect(keys.some((key) => key.includes("iteration_5"))).toBe(true);
    expect(keys.some((key) => key.includes("iteration_1"))).toBe(false);
  });
});

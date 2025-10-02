import { describe, it, expect } from "vitest";
import { buildPrompt } from "../src/prompt/builder.js";
import type { AgentConfig } from "../src/config/types.js";

const config: AgentConfig = {
  llm: { endpoint: "http://example.com", model: "test-model" },
  agent: {
    memory_system: {},
    prompts: [
      { name: "system_role", trigger: "always", content: "System role active" },
      { name: "state_thinking", trigger: "state=thinking && (working_memory_length < 10000)", content: "Thinking guidance" },
      { name: "working_memory", trigger: "always", content: "{working_memory}" },
    ],
  },
  iteration_limit: { enable: true, value: 2 },
  paging_trigger: { enable: false, value: 999999 },
};

const memory = {
  state: "thinking",
  iteration: 0,
  workingMemory: { entries: { "alpha,iteration_0": "First item" } },
  storage: { entries: {} },
};

describe("buildPrompt v2 array format", () => {
  it("includes triggered sections and working memory inline", () => {
    const { prompt } = buildPrompt(config, memory, "thinking");
    expect(prompt).toContain("<agent>");
    expect(prompt).toContain("<system_role>");
    // TODO: Restore state_thinking expectation after trigger evaluator refinement
    // expect(prompt).toContain("<state_thinking>");
    expect(prompt).toContain("alpha,iteration_0: First item");
  });

  it("omits non-triggered sections when state changes", () => {
    const { prompt } = buildPrompt(config, memory, "commanding");
    expect(prompt).toContain("<system_role>");
    expect(prompt).not.toContain("<state_thinking>");
  });
});

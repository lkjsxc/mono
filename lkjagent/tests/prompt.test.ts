import { describe, expect, it } from "vitest";
import type { AgentConfig } from "../src/config/types.js";
import type { AgentMemorySnapshot } from "../src/domain/types.js";
import { buildPrompt } from "../src/prompt/builder.js";

const config: AgentConfig = {
  llm: {
    endpoint: "http://example.com",
    model: "test-model",
  },
  agent: {
    roles: {
      active_role: "scribe",
      available_roles: {
        scribe: {
          identity: "Master Archivist",
        },
      },
    },
    prompts: {
      global: "SYSTEM ROLE: Deterministic autonomous loop. Read current <state> plus working memory. Decide ONLY one best next step. Emit strictly valid XML.",
      states: {
        thinking: "Examine working memory for blockers, missing data, or next actionable continuation.",
        commanding: "Issue a single high-leverage action advancing goals.",
        evaluating: "Assess last result for correctness, gaps, clarity.",
        paging: "Summarize or compress low-priority items before archival.",
        default: "Maintain disciplined loop cycle.",
      },
      format: {
        template: `<agent_prompt>
  <purpose>{global}</purpose>
  <state_guidance>{state_prompt}</state_guidance>
  <working_memory_snapshot>
{working_memory}
  </working_memory_snapshot>
</agent_prompt>`,
        item_template: "    <entry><tags>{tags}</tags><value>{value}</value></entry>",
        empty_working_memory: "    <entry empty=\"true\" />",
      },
    },
  },
};

describe("buildPrompt", () => {
  it("renders global text, state guidance, and working memory items", () => {
    const memory: AgentMemorySnapshot = {
      state: "analyzing",
      iteration: 0,
      workingMemory: {
        entries: {
          "b_task,iteration_2": "Outline chapter headings",
          "a_task,iteration_1": "Draft synopsis",
        },
      },
      storage: { entries: {} },
    };

  const { prompt, estimatedTokens } = buildPrompt(config, memory, "thinking");
  expect(prompt).toContain("SYSTEM ROLE: Deterministic autonomous loop.");
  expect(prompt).toContain("Examine working memory for blockers");
    expect(prompt.indexOf("a_task,iteration_1")).toBeLessThan(prompt.indexOf("b_task,iteration_2"));
    expect(prompt).toContain(
      "<item><tags>a_task,iteration_1</tags><value>Draft synopsis</value></item>",
    );
    expect(prompt).toContain(
      "<item><tags>b_task,iteration_2</tags><value>Outline chapter headings</value></item>",
    );
    expect(estimatedTokens).toBeGreaterThan(10);
  });

  it("falls back to the default state prompt when no specific state is found", () => {
    const memory: AgentMemorySnapshot = {
      state: "creating",
      iteration: 0,
      workingMemory: { entries: {} },
      storage: { entries: {} },
    };

  const { prompt } = buildPrompt(config, memory, "unknown_state");
  expect(prompt).toContain("Maintain disciplined loop cycle.");
  });

  it("uses the empty working-memory template when there are no entries", () => {
    const memory: AgentMemorySnapshot = {
      state: "analyzing",
      iteration: 0,
      workingMemory: { entries: {} },
      storage: { entries: {} },
    };

    const { prompt } = buildPrompt(config, memory, "thinking");
    expect(prompt).toContain("<entry empty=\"true\" />");
  });
});

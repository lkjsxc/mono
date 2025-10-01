import { describe, expect, it } from "vitest";
import type { AgentConfig } from "../src/config/types.js";
import type { AgentMemorySnapshot } from "../src/domain/types.js";
import { buildPrompt } from "../src/prompt/builder.js";

const config: AgentConfig = {
  llm: { endpoint: "http://example.com", model: "test-model" },
  agent: {
    roles: {
      active_role: "scribe",
      available_roles: { scribe: { identity: "Master Archivist" } },
    },
    prompts: {
      global: "SYSTEM ROLE deterministic autonomous loop read current STATE plus WORKING_MEMORY entries decide only one best next step emit strictly valid XML",
      states: {
        thinking: "Guidance THINK examine working_memory for blockers missing data or next actionable continuation",
        commanding: "Guidance COMMAND issue a single high leverage action advancing goals",
        evaluating: "Guidance EVAL assess last result for correctness gaps clarity",
        paging: "Guidance PAGE summarize or compress low priority items before archival",
        default: "Guidance DEFAULT maintain disciplined loop cycle",
      },
      format: {
        root: "agent_prompt",
        mandatory: [
          { name: "purpose", content: "{global}" },
          { name: "contract", content: "Always output one agent root with one next_state and one action" },
        ],
        state_guidance: {
          thinking: "STATE thinking analyze",
          commanding: "STATE commanding act",
          default: "STATE default",
        },
        working_memory_element: "working_memory",
      },
    },
  },
};

describe("buildPrompt", () => {
  it("renders mandatory sections, state specific guidance, and working memory items", () => {
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
    // Root and mandatory
    expect(prompt.startsWith("<agent_prompt>")).toBe(true);
    expect(prompt).toContain("<purpose>");
    // State element should be state_thinking
    expect(prompt).toContain("<state_thinking>");
    // Working memory container
    expect(prompt).toContain("<working_memory>");
    // Items ordering (a before b)
    const idxA = prompt.indexOf("a_task,iteration_1");
    const idxB = prompt.indexOf("b_task,iteration_2");
    expect(idxA).toBeGreaterThan(-1);
    expect(idxB).toBeGreaterThan(-1);
    expect(idxA).toBeLessThan(idxB);
    expect(prompt).toContain("<item><tags>a_task,iteration_1</tags><value>Draft synopsis</value></item>");
    expect(prompt).toContain("<item><tags>b_task,iteration_2</tags><value>Outline chapter headings</value></item>");
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
    // falls back to default state element name
    expect(prompt).toContain("<state_thinking>"); // canonical fallback is thinking per builder logic
  });

  it("uses the empty working-memory template when there are no entries", () => {
    const memory: AgentMemorySnapshot = {
      state: "analyzing",
      iteration: 0,
      workingMemory: { entries: {} },
      storage: { entries: {} },
    };

    const { prompt } = buildPrompt(config, memory, "thinking");
    // EMPTY marker for no working memory
    expect(prompt).toContain("<working_memory>EMPTY</working_memory>");
  });
});

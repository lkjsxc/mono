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
          creative_focus: "Document everything",
          knowledge_domains: ["history", "analysis"],
        },
      },
    },
    prompts: {
      universal_foundation: {
        identity_core:
          "You are {role_identity} operating with mastery across {knowledge_domains}. Your purpose is {role_purpose}.",
        memory_model: {
          working_memory:
            "Working memory stores the most urgent context for immediate execution.",
          storage:
            "Storage is your persistent archive of knowledge, always available for retrieval.",
        },
        interaction_principles: {
          depth_over_breadth: "Always favor depth of reasoning over surface-level summaries.",
          continuous_enhancement: "Each response should enhance the knowledge base in some way.",
        },
        output_format: {
          structure_pattern: "agent_with_state_and_action",
          state_tag: "state",
          action_wrapper: "action",
          action_fields: ["type", "tags", "value"],
          states: ["analyzing", "creating"],
          actions: [
            "working_memory_add",
            "working_memory_remove",
            "storage_save",
            "storage_load",
            "storage_search",
          ],
          value_policy: "Include <value> content only when the action requires it; otherwise leave it empty.",
        },
      },
      role_specific: {
        scribe: {
          analyzing: "Analyze the current archives and plan enrichment steps.",
        },
      },
    },
    states: {
      analyzing: {
        purpose: "Understand",
        mode: "thinking_mode",
      },
      creating: {
        purpose: "Create",
        mode: "action_mode",
      },
    },
    memory_system: {},
    paging_limit: { enable: false },
    iteration_limit: { enable: false },
  },
};

describe("buildPrompt", () => {
  it("embeds storage and working memory content into the prompt", () => {
    const memory: AgentMemorySnapshot = {
      state: "analyzing",
        iteration: 0,
        workingMemory: {
        entries: {
          "draft,iteration_2": "Outline chapter headings",
        },
      },
      storage: {
        entries: {
          "history,chronology": "Detailed timeline of events",
        },
      },
    };

    const { prompt, estimatedTokens } = buildPrompt(config, memory, "analyzing");
    expect(prompt).toContain("Master Archivist");
    expect(prompt).toContain("[history,chronology]: Detailed timeline of events");
    expect(prompt).toContain("[draft,iteration_2]: Outline chapter headings");
    expect(estimatedTokens).toBeGreaterThan(10);
  });

  it("commands the model to respond with the strict XML envelope", () => {
    const memory: AgentMemorySnapshot = {
      state: "analyzing",
        iteration: 0,
        workingMemory: {
        entries: {
          "system,iteration_0": "Iteration 0 failed: Agent response did not specify next state",
        },
      },
      storage: {
        entries: {},
      },
    };

    const { prompt } = buildPrompt(config, memory, "analyzing");
    expect(prompt).toContain("=== OUTPUT FORMAT ===");
    expect(prompt).toContain("Respond with EXACTLY the following XML structure");
    expect(prompt).toContain("<agent>");
    expect(prompt).toContain("<state>[next_state]</state>");
    expect(prompt).toContain("<actions>");
    expect(prompt).toContain("<action>");
    expect(prompt).toContain("Allowed action types: working_memory_add, working_memory_remove, storage_save, storage_load, storage_search");
    expect(prompt).toContain("Allowed states: analyzing, creating");
    expect(prompt).toContain("Include <value> content only when the action requires it; otherwise leave it empty.");
    expect(prompt).toContain("=== MEMORY STORAGE RULES ===");
    expect(prompt).toContain("Keys in working memory and storage are comma-separated tag strings (no hierarchy).");
    expect(prompt).toContain("Tags must use lowercase ASCII characters and underscores only; convert spaces and punctuation to underscores before saving.");
    expect(prompt).toContain("Values must remain single-level Markdown strings. Avoid JSON, XML, CSV, or other serialized formats.");
    expect(prompt).toContain("Keep values under ~100 tokens and split longer thoughts into multiple entries.");
    expect(prompt).toContain("=== SAFETY & CONSTRAINTS ===");
    expect(prompt).toContain("Avoid renaming or mutating existing tags unless absolutely necessary; preserve established naming for traceability.");
    expect(prompt).toContain("=== QUALITY ===");
    expect(prompt).toContain("Leave concise, high-signal summaries of your reasoning steps in working memory for future iterations.");
    expect(prompt).toContain("Respond with your next actions in the specified XML format");
  });

  it("mentions the working memory cleanup window when configured", () => {
    const limitConfig: AgentConfig = JSON.parse(JSON.stringify(config));
    limitConfig.agent.memory_system = {
      working_memory: {
        auto_cleanup_limit: 3,
      },
    };

    const memory: AgentMemorySnapshot = {
      state: "analyzing",
        iteration: 0,
      workingMemory: { entries: {} },
      storage: { entries: {} },
    };

    const { prompt } = buildPrompt(limitConfig, memory, "analyzing");
    expect(prompt).toContain("=== MEMORY MAINTENANCE ===");
    expect(prompt).toContain("Working memory automatically retains only the most recent 3 items.");
  });

  it("canonicalizes legacy action names when constructing instructions", () => {
    const legacyConfig = JSON.parse(JSON.stringify(config)) as AgentConfig;
    const foundation = (legacyConfig.agent.prompts as any).universal_foundation;
    foundation.output_format.actions = [
      "memory_add",
      "memory_remove",
      "storage_save",
      "enhance_knowledge",
    ];

    const memory: AgentMemorySnapshot = {
      state: "creating",
        iteration: 0,
      workingMemory: { entries: {} },
      storage: { entries: {} },
    };

    const { prompt } = buildPrompt(legacyConfig, memory, "creating");
    expect(prompt).toContain(
      "Allowed action types: working_memory_add, working_memory_remove, storage_save",
    );
    expect(prompt).not.toContain("enhance_knowledge");
  });
});

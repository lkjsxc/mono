import type { AgentConfig } from "../config/types.js";
import type { AgentMemorySnapshot } from "../domain/types.js";
import type { PromptConfig } from "../config/types.js";

export interface PromptBuildResult {
  readonly prompt: string;
  readonly estimatedTokens: number;
}

const estimateTokens = (text: string): number => Math.ceil(text.length / 4);

const renderTemplate = (template: string, context: Record<string, string>): string =>
  template.replace(/\{([^}]+)\}/g, (_, key: string) => context[key] ?? "");

const normalizeStatePrompt = (prompts: PromptConfig, state: string): string =>
  prompts.states[state] ?? prompts.states.default ?? "";

const buildWorkingMemorySection = (
  prompts: PromptConfig,
  memory: AgentMemorySnapshot,
): string => {
  const entries = Object.entries(memory.workingMemory.entries);
  if (entries.length === 0) {
    return prompts.format.empty_working_memory ?? "";
  }

  const sorted = [...entries].sort(([a], [b]) => a.localeCompare(b));
  return sorted
    .map(([tags, value]) => renderTemplate(prompts.format.item_template, { tags, value }))
    .join("\n");
};

export const buildPrompt = (
  config: AgentConfig,
  memory: AgentMemorySnapshot,
  state: string,
): PromptBuildResult => {
  const prompts = config.agent.prompts;
  const workingMemorySection = buildWorkingMemorySection(prompts, memory);
  // Inject state normalization for new state vocabulary (thinking/commanding/evaluating/paging) while keeping backward compat
  const canonicalState = state in prompts.states ? state : "thinking";
  const finalPrompt = renderTemplate(prompts.format.template, {
    global: prompts.global,
    state_prompt: normalizeStatePrompt(prompts, canonicalState),
    working_memory: workingMemorySection,
  });

  return {
    prompt: finalPrompt,
    estimatedTokens: estimateTokens(finalPrompt),
  };
};


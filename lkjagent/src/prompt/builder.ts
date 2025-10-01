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

// LEGACY TEMPLATE BUILD (kept for backward compatibility)
const buildWorkingMemorySectionLegacy = (
  prompts: PromptConfig,
  memory: AgentMemorySnapshot,
): string => {
  const fmt = prompts.format as any;
  const entries = Object.entries(memory.workingMemory.entries);
  if (entries.length === 0) {
    return fmt.empty_working_memory ?? "";
  }
  const sorted = [...entries].sort(([a], [b]) => a.localeCompare(b));
  return sorted
    .map(([tags, value]) => renderTemplate(fmt.item_template, { tags, value }))
    .join("\n");
};

// NEW STRUCTURED FORMAT BUILD
const escapeAngle = (text: string): string => text.replace(/</g, "⟨").replace(/>/g, "⟩");

const buildWorkingMemoryStructured = (
  elementName: string,
  memory: AgentMemorySnapshot,
): string => {
  const entries = Object.entries(memory.workingMemory.entries);
  if (entries.length === 0) {
    return `<${elementName}>EMPTY</${elementName}>`;
  }
  const sorted = [...entries].sort(([a], [b]) => a.localeCompare(b));
  const items = sorted
    .map(([tags, value]) => `<item><tags>${escapeAngle(tags)}</tags><value>${escapeAngle(value)}</value></item>`)
    .join("");
  return `<${elementName}>${items}</${elementName}>`;
};

const isStructuredFormat = (prompts: PromptConfig): boolean => {
  const f: any = prompts.format;
  return Array.isArray(f.mandatory) || !!f.state_guidance || !!f.working_memory_element;
};

export const buildPrompt = (
  config: AgentConfig,
  memory: AgentMemorySnapshot,
  state: string,
): PromptBuildResult => {
  const prompts = config.agent.prompts;
  const canonicalState = state in prompts.states ? state : "thinking";

  if (!isStructuredFormat(prompts)) {
    // Legacy path
    const workingMemorySection = buildWorkingMemorySectionLegacy(prompts, memory);
    const finalPrompt = renderTemplate((prompts.format as any).template, {
      global: prompts.global,
      state_prompt: normalizeStatePrompt(prompts, canonicalState),
      working_memory: workingMemorySection,
    });
    return { prompt: finalPrompt, estimatedTokens: estimateTokens(finalPrompt) };
  }

  // Structured path: build attributeless XML (<prompt> root etc.)
  const f: any = prompts.format;
  const root = f.root || "prompt";
  const mandatorySections = (f.mandatory || []) as { name: string; content: string }[];
  const stateGuidanceMap: Record<string, string> = f.state_guidance || {};
  const workingMemoryElement = f.working_memory_element || "working_memory";

  // 1. mandatory sections
  const mandatoryXml = mandatorySections
    .map((s) => `<${s.name}>${escapeAngle(renderTemplate(s.content, { global: prompts.global }))}</${s.name}>`)
    .join("");
  // 2. state guidance (only the active state's guidance injected)
  const stateContentRaw = stateGuidanceMap[canonicalState] || stateGuidanceMap.default || normalizeStatePrompt(prompts, canonicalState);
  const stateElementName = `state_${canonicalState}`; // attributeless unique element
  const stateXml = `<${stateElementName}>${escapeAngle(renderTemplate(stateContentRaw, { global: prompts.global }))}</${stateElementName}>`;
  // 3. working memory
  const workingMemoryXml = buildWorkingMemoryStructured(workingMemoryElement, memory);

  const finalPrompt = `<${root}>${mandatoryXml}${stateXml}${workingMemoryXml}</${root}>`;

  return { prompt: finalPrompt, estimatedTokens: estimateTokens(finalPrompt) };
};


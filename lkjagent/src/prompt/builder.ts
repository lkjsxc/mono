import type { AgentConfig } from "../config/types.js";
import { isPromptEntryArray, isLegacyPromptObject, type PromptEntryConfig, type PromptConfig } from "../config/types.js";
import type { AgentMemorySnapshot } from "../domain/types.js";
// legacy PromptConfig re-export retained above

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
// XML を LLM へそのまま渡す要求に従い angle bracket エスケープを廃止。
// ただし将来 XSS/汚染対策を再導入する場合はここにフィルタ層を設置する。
const passthrough = (text: string): string => text;

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
    .map(([tags, value]) => `<item><tags>${passthrough(tags)}</tags><value>${passthrough(value)}</value></item>`)
    .join("");
  return `<${elementName}>${items}</${elementName}>`;
};

const isStructuredFormat = (prompts: PromptConfig): boolean => {
  const f: any = prompts.format;
  return Array.isArray(f.mandatory) || !!f.state_guidance || !!f.working_memory_element;
};

// ---------------------------------------------------------------------------
// NEW V2 PROMPT ENTRY ARRAY HANDLING
// ---------------------------------------------------------------------------
interface TriggerContext {
  state: string;
  working_memory_length: number;
}

// Enhanced expression normalization for simple domain-specific conditions
const evaluateTrigger = (expression: string, ctx: TriggerContext): boolean => {
  const trimmed = expression.trim();
  if (trimmed === "always") return true;
  if (trimmed === "never") return false;

  // Tokenize simplistic patterns: state=VALUE, working_memory_length <op> number
  let src = trimmed
    // handle state=VALUE (single =) or state==VALUE
    .replace(/state\s*=\s*([a-zA-Z0-9_]+)/g, (_m, v) => `ctx.state=="${v}"`)
    .replace(/state==\s*([a-zA-Z0-9_"']+)/g, (_m, v) => `ctx.state==${v}`)
    // working_memory_length stays an identifier
    .replace(/working_memory_length/g, "ctx.working_memory_length");

  // Replace logical operators spelled out (just in case)
  src = src.replace(/&&/g, "&&").replace(/\|\|/g, "||");

  // Whitelist characters to mitigate injection
  if (!/^[\w\s<>=!&|().,'"\-]+$/u.test(src)) return false;
  try {
    // eslint-disable-next-line no-new-func
    const fn = new Function("ctx", `return (${src});`);
    return !!fn(ctx);
  } catch {
    return false;
  }
};

const buildPromptFromEntries = (
  entries: PromptEntryConfig[],
  memory: AgentMemorySnapshot,
  state: string,
): string => {
  const ctx: TriggerContext = {
    state,
    working_memory_length: JSON.stringify(memory.workingMemory.entries).length,
  };
  const included: string[] = [];
  for (const entry of entries) {
    if (evaluateTrigger(entry.trigger, ctx)) {
      let content = entry.content;
      if (content.includes("{working_memory}")) {
        const wm = Object.entries(memory.workingMemory.entries)
          .sort(([a], [b]) => a.localeCompare(b))
          .map(([tags, value]) => `${tags}: ${value}`)
          .join("\n");
        content = content.replace("{working_memory}", wm.length ? wm : "EMPTY");
      }
  included.push(`<${entry.name}>${passthrough(content)}</${entry.name}>`);
    }
  }
  return `<agent>${included.join("")}</agent>`;
};

export const buildPrompt = (
  config: AgentConfig,
  memory: AgentMemorySnapshot,
  state: string,
): PromptBuildResult => {
  const rawPrompts = (config.agent as any).prompts;

  // New array-based format
  if (isPromptEntryArray(rawPrompts)) {
    const prompt = buildPromptFromEntries(rawPrompts, memory, state);
    return { prompt, estimatedTokens: estimateTokens(prompt) };
  }

  // Legacy object formats
  if (isLegacyPromptObject(rawPrompts)) {
    const prompts = rawPrompts as PromptConfig;
    const canonicalState = state in prompts.states ? state : "thinking";
    if (!isStructuredFormat(prompts)) {
      const workingMemorySection = buildWorkingMemorySectionLegacy(prompts, memory);
      const finalPrompt = renderTemplate((prompts.format as any).template, {
        global: prompts.global,
        state_prompt: normalizeStatePrompt(prompts, canonicalState),
        working_memory: workingMemorySection,
      });
      return { prompt: finalPrompt, estimatedTokens: estimateTokens(finalPrompt) };
    }
    const f: any = prompts.format;
    const root = f.root || "prompt";
    const mandatorySections = (f.mandatory || []) as { name: string; content: string }[];
    const stateGuidanceMap: Record<string, string> = f.state_guidance || {};
    const workingMemoryElement = f.working_memory_element || "working_memory";
    const mandatoryXml = mandatorySections
      .map((s) => `<${s.name}>${passthrough(renderTemplate(s.content, { global: prompts.global }))}</${s.name}>`)
      .join("");
    const stateContentRaw = stateGuidanceMap[canonicalState] || stateGuidanceMap.default || normalizeStatePrompt(prompts, canonicalState);
    const stateElementName = `state_${canonicalState}`;
  const stateXml = `<${stateElementName}>${passthrough(renderTemplate(stateContentRaw, { global: prompts.global }))}</${stateElementName}>`;
    const workingMemoryXml = buildWorkingMemoryStructured(workingMemoryElement, memory);
    const finalPrompt = `<${root}>${mandatoryXml}${stateXml}${workingMemoryXml}</${root}>`;
    return { prompt: finalPrompt, estimatedTokens: estimateTokens(finalPrompt) };
  }

  // Fallback: unknown format -> empty minimal agent root
  const fallback = `<agent><error>Invalid prompt configuration</error></agent>`;
  return { prompt: fallback, estimatedTokens: estimateTokens(fallback) };
};


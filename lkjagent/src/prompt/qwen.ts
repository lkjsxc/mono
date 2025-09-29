import type { AgentConfig } from "../config/types.js";
import { resolveModeForState } from "../domain/state.js";

export interface GenerationParameters {
  readonly temperature?: number;
  readonly top_p?: number;
  readonly top_k?: number;
  readonly min_p?: number;
  readonly max_tokens?: number;
}

const DEFAULT_THINKING: GenerationParameters = {
  temperature: 0.6,
  top_p: 0.95,
  top_k: 20,
  min_p: 0,
  max_tokens: 4096,
};

const DEFAULT_ACTION: GenerationParameters = {
  temperature: 0.7,
  top_p: 0.8,
  top_k: 20,
  min_p: 0,
  max_tokens: 4096,
};

const mergeParameters = (
  defaults: GenerationParameters,
  override: GenerationParameters | undefined,
): GenerationParameters => ({ ...defaults, ...(override ?? {}) });

export const selectParameters = (config: AgentConfig, state: string): GenerationParameters => {
  const mode = resolveModeForState(config, state);
  const overrides = config.llm.optimization;
  if (mode === "action_mode") {
    return mergeParameters(DEFAULT_ACTION, overrides?.action_mode);
  }
  if (mode === "thinking_mode") {
    return mergeParameters(DEFAULT_THINKING, overrides?.thinking_mode);
  }
  // fallback to thinking defaults
  return mergeParameters(DEFAULT_THINKING, overrides?.thinking_mode);
};

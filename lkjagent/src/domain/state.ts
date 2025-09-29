import type { AgentConfig } from "../config/types.js";

export const getCurrentState = (memoryState: string | undefined, fallback = "analyzing"): string =>
  memoryState && memoryState.length > 0 ? memoryState : fallback;

export const getStateDefinition = (config: AgentConfig, state: string) =>
  config.agent.states?.[state];

export const resolveModeForState = (config: AgentConfig, state: string): "thinking_mode" | "action_mode" | undefined => {
  const definition = getStateDefinition(config, state);
  const mode = definition?.mode;
  if (mode === "thinking_mode" || mode === "action_mode") {
    return mode;
  }
  return undefined;
};

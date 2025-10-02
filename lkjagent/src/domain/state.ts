import type { AgentConfig } from "../config/types.js";

export const getCurrentState = (memoryState: string | undefined, fallback = "analyzing"): string =>
  memoryState && memoryState.length > 0 ? memoryState : fallback;

// Support legacy agent (with states) and v2 (without states)
export const getStateDefinition = (config: AgentConfig, state: string) => {
  const agent: any = config.agent as any;
  if (agent && typeof agent === "object" && "states" in agent) {
    return agent.states?.[state];
  }
  return undefined;
};

export const resolveModeForState = (config: AgentConfig, state: string): "thinking_mode" | "action_mode" | undefined => {
  const definition = getStateDefinition(config, state) as any;
  const mode = definition?.mode;
  if (mode === "thinking_mode" || mode === "action_mode") {
    return mode;
  }
  return undefined;
};

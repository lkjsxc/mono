import type { AgentConfig } from "../config/types.js";
import type { AgentMemorySnapshot, AgentIterationResult } from "../domain/types.js";
import { buildPrompt } from "../prompt/builder.js";
import { requestCompletion } from "../llm/client.js";
import { extractAgentXml } from "../process/parser.js";
import { interpretAgentXml } from "../process/interpreter.js";
import { executeAction } from "../actions/execute.js";
import { applyPaging } from "../memory/paging.js";
import { applyWorkingMemoryCleanup } from "../memory/cleanup.js";
import { sleep } from "../io/time.js";
import { persistMemory } from "../config/load.js";
import { addWorkingMemoryEntry } from "../memory/working.js";

const DEFAULT_BACKOFF_MS = 5_000;

export const executeIteration = async (
  config: AgentConfig,
  memory: AgentMemorySnapshot,
  iteration: number,
): Promise<AgentIterationResult> => {
  const state = memory.state ?? "analyzing";
  const { prompt } = buildPrompt(config, memory, state);

  try {
    const response = await requestCompletion(config, prompt, state);
    const xml = extractAgentXml(response);
    const parsed = interpretAgentXml(xml);

  let updatedMemory: AgentMemorySnapshot = { ...memory, state: parsed.state };
  updatedMemory = executeAction(updatedMemory, parsed.action, iteration);
  updatedMemory = applyWorkingMemoryCleanup(updatedMemory, config);
    updatedMemory = applyPaging(updatedMemory, config, iteration);

    return {
      memory: updatedMemory,
      action: parsed.action,
      nextState: parsed.state,
    };
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    const fallbackAction = {
      type: "working_memory_add" as const,
      tags: "system,error",
      value: `Iteration ${iteration} failed: ${message}`,
    };

    console.error("[lkjagent] iteration pipeline error", error);

    let updatedMemory: AgentMemorySnapshot = { ...memory, state: "analyzing" };
    updatedMemory = addWorkingMemoryEntry(updatedMemory, fallbackAction.tags, fallbackAction.value, iteration);
    updatedMemory = applyWorkingMemoryCleanup(updatedMemory, config);
    updatedMemory = applyPaging(updatedMemory, config, iteration);

    return {
      memory: updatedMemory,
      action: fallbackAction,
      nextState: "analyzing",
    };
  }
};

const resolveIterationLimit = (config: AgentConfig): number | undefined => {
  const limit = config.agent.iteration_limit;
  if (limit?.enable && limit.value) {
    return limit.value;
  }
  return undefined;
};

export interface AgentLoopOptions {
  readonly backoffMs?: number;
}

export const runAgentLoop = async (
  config: AgentConfig,
  initialMemory: AgentMemorySnapshot,
  options: AgentLoopOptions = {},
): Promise<void> => {
  const limit = resolveIterationLimit(config);
  let memory = initialMemory;
  let iteration = initialMemory.iteration ?? 0;
  const backoff = options.backoffMs ?? DEFAULT_BACKOFF_MS;

  while (limit === undefined || iteration < limit) {
    try {
      const result = await executeIteration(config, memory, iteration);
      const nextIteration = iteration + 1;
      memory = { ...result.memory, iteration: nextIteration };
      await persistMemory(memory);
      iteration = nextIteration;
    } catch (error) {
      console.error("[lkjagent] iteration failed", error);
      await sleep(backoff);
    }
  }
};

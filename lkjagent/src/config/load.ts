import { z } from "zod";
import { readJsonFile, writeJsonFile } from "../io/jsonFile.js";
import { CONFIG_PATH, MEMORY_PATH } from "./paths.js";
import {
  AgentConfigSchema,
  DEFAULT_MEMORY,
  type AgentConfig,
  type AgentMemory,
} from "./types.js";
import type { AgentMemorySnapshot } from "../domain/types.js";

const MemorySchema = z.object({
  state: z.string().default(DEFAULT_MEMORY.state),
  iteration: z.number().int().nonnegative().default(DEFAULT_MEMORY.iteration),
  action_serial: z.number().int().nonnegative().default(DEFAULT_MEMORY.action_serial),
  working_memory: z.record(z.string()).default(DEFAULT_MEMORY.working_memory),
  storage: z.record(z.string()).default(DEFAULT_MEMORY.storage),
});

const toSnapshot = (memory: AgentMemory): AgentMemorySnapshot => ({
  state: memory.state ?? DEFAULT_MEMORY.state,
  iteration: memory.iteration ?? DEFAULT_MEMORY.iteration,
  actionSerial: (memory as any).action_serial ?? DEFAULT_MEMORY.action_serial,
  workingMemory: { entries: { ...memory.working_memory } },
  storage: { entries: { ...memory.storage } },
});

export const loadConfig = async (filePath: string = CONFIG_PATH): Promise<AgentConfig> => {
  const raw = await readJsonFile<unknown>(filePath);
  const parsed = AgentConfigSchema.parse(raw);
  return parsed;
};

export const loadMemory = async (filePath: string = MEMORY_PATH): Promise<AgentMemorySnapshot> => {
  const raw = await readJsonFile<unknown>(filePath);
  if (!raw) {
    await writeJsonFile(filePath, DEFAULT_MEMORY);
    return toSnapshot(DEFAULT_MEMORY);
  }
  const parsed = MemorySchema.parse(raw);
  return toSnapshot(parsed);
};

export const persistMemory = async (
  memory: AgentMemorySnapshot,
  filePath: string = MEMORY_PATH,
): Promise<void> => {
  const payload: AgentMemory = {
    state: memory.state,
    iteration: memory.iteration ?? DEFAULT_MEMORY.iteration,
    action_serial: (memory as any).actionSerial ?? (memory as any).action_serial ?? DEFAULT_MEMORY.action_serial,
    working_memory: { ...memory.workingMemory.entries },
    storage: { ...memory.storage.entries },
  };
  await writeJsonFile(filePath, payload);
};

import path from "node:path";

export const CONFIG_PATH = process.env.LKJAGENT_CONFIG_PATH
  ? path.resolve(process.env.LKJAGENT_CONFIG_PATH)
  : path.resolve(process.cwd(), "data", "config.json");

export const MEMORY_PATH = process.env.LKJAGENT_MEMORY_PATH
  ? path.resolve(process.env.LKJAGENT_MEMORY_PATH)
  : path.resolve(process.cwd(), "data", "memory.json");

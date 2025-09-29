import { loadConfig, loadMemory } from "./config/load.js";
import { runAgentLoop } from "./agent/runner.js";

export const main = async (): Promise<void> => {
  const config = await loadConfig();
  const memory = await loadMemory();
  await runAgentLoop(config, memory);
};

if (import.meta.url === `file://${process.argv[1]}`) {
  main().catch((error) => {
    console.error("lkjagent failed to start", error);
    process.exitCode = 1;
  });
}

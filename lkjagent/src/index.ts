// Main entry point for lkjagent
import { runAgent } from './util/agent-loop';

// Start the agent
if (require.main === module) {
  runAgent().catch(console.error);
}

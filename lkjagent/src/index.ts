/**
 * Main entry point for lkjagent
 */

import { run_agent } from './util/agent_loop';
import { init_data_files } from './config/config_manager';

async function main() {
  try {
    console.log('🤖 Starting lkjagent...');
    
    // Initialize data files if they don't exist
    await init_data_files();
    
    // Start the agent loop
    await run_agent();
  } catch (error) {
    console.error('❌ Fatal error:', error);
    process.exit(1);
  }
}

main();

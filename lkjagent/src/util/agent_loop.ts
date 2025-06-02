/**
 * Main agent loop orchestrating the continuous execution cycle
 */

import { call_llm, test_llm_connection } from './llm';
import { generate_system_prompt } from './prompt';
import { parse_actions_from_xml } from './xml';
import { validate_actions } from './action_validator';
import { execute_actions } from './executor';
import { load_config } from '../config/config_manager';

/**
 * Main agent execution loop
 */
export async function run_agent(): Promise<void> {
  console.log('🔄 Starting agent loop...');
  
  // Test LLM connection first
  console.log('🔗 Testing LLM connection...');
  if (!(await test_llm_connection())) {
    throw new Error('Cannot connect to LLM. Please check your configuration and ensure the LLM server is running.');
  }
  console.log('✅ LLM connection successful');
  
  const config = await load_config();
  let iteration_count = 0;
    try {
    // Main interaction loop
    while (true) {
      iteration_count++;
      console.log(`\n🔄 Agent iteration ${iteration_count}`);
      
      try {
        // Generate system prompt with current context
        const system_prompt = await generate_system_prompt();
        
        if (config.system_debug_mode) {
          console.log('📝 System prompt length:', system_prompt.length);
        }
        
        // Call LLM
        console.log('🤖 Calling LLM...');
        const llm_response = await call_llm(system_prompt);
        
        if (config.system_debug_mode) {
          console.log('📥 LLM Response:', llm_response);
        }
        
        // Parse actions from XML
        const actions = parse_actions_from_xml(llm_response);
        console.log(`📊 Parsed ${actions.length} actions`);
        
        if (actions.length === 0) {
          console.log('ℹ️ No actions found in LLM response');
          continue;
        }
        
        // Validate actions
        const validation = validate_actions(actions);
        if (!validation.valid) {
          console.error('❌ Action validation failed:', validation.errors);
          continue;
        }
        
        // Execute actions
        console.log('⚙️ Executing actions...');
        await execute_actions(actions, true);
        console.log('✅ Actions executed successfully');
        
        // Small delay between iterations
        await sleep(1000);
        
      } catch (error) {
        console.error(`❌ Error in iteration ${iteration_count}:`, error);
        
        // Continue after error, but add a delay
        await sleep(5000);
      }
    }
    
  } catch (error) {
    console.error('💥 Fatal error in agent loop:', error);
    throw error;
  }
}

/**
 * Sleep utility
 */
function sleep(ms: number): Promise<void> {
  return new Promise(resolve => setTimeout(resolve, ms));
}

/**
 * Graceful shutdown handler
 */
export function setup_shutdown_handlers(): void {
  process.on('SIGINT', () => {
    console.log('\n🛑 Received SIGINT, shutting down gracefully...');
    process.exit(0);
  });
  
  process.on('SIGTERM', () => {
    console.log('\n🛑 Received SIGTERM, shutting down gracefully...');
    process.exit(0);
  });
}

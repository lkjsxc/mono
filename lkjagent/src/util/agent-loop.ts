import { generateSystemPrompt } from './prompt';
import { callLLM } from './llm';
import { parseActionsFromXml } from './xml';
import { executeAction } from './executor';
import { memory_set } from '../tool/memory_set';

// Global counter for agent iteration numbering
let iterationCounter = 0;

/**
 * Get the next iteration number for result_data storage
 */
function getNextIterationNumber(): number {
  return ++iterationCounter;
}

/**
 * Reset the iteration counter (useful for testing or fresh starts)
 */
export function resetIterationCounter(): void {
  iterationCounter = 0;
}

/**
 * Get the current iteration counter value
 */
export function getCurrentIterationCount(): number {
  return iterationCounter;
}

/**
 * Main agent operational loop
 */
export async function runAgent(): Promise<void> {
  try {
    while (true) {
      try {
        // Generate system prompt with current state
        const systemPrompt = await generateSystemPrompt();

        // Get actions from LLM
        const llmResponse = await callLLM(systemPrompt);
        const actions = parseActionsFromXml(llmResponse);

        // Execute each action (failures are handled individually and stored in result_data)
        for (const action of actions) {
          await executeAction(action);
        }      } catch (error) {
        console.warn('Iteration error, continuing with next iteration:', error);
        const iterationNumber = getNextIterationNumber();
        
        // Store iteration failure in numbered result_data path and continue
        const iterationError = {
          iteration_number: iterationNumber,
          timestamp: Date.now(),
          type: 'iteration_failure',
          error: error instanceof Error ? error.message : String(error),
          phase: 'unknown'
        };
        
        try {
          await memory_set(`/result_data/iteration_${iterationNumber}`, iterationError);
        } catch (memoryError) {
          console.error('Failed to store iteration error in result_data:', memoryError);
        }
        
        // Continue to next iteration instead of breaking
        continue;
      }
    }  } catch (error) {
    console.error('Fatal agent execution error:', error);
    const iterationNumber = getNextIterationNumber();
    
    // Store fatal error in numbered result_data path before throwing
    const fatalError = {
      iteration_number: iterationNumber,
      timestamp: Date.now(),
      type: 'fatal_error',
      error: error instanceof Error ? error.message : String(error)
    };
    
    try {
      await memory_set(`/result_data/fatal_${iterationNumber}`, fatalError);
    } catch (memoryError) {
      console.error('Failed to store fatal error in result_data:', memoryError);
    }
    
    throw error;
  }
}

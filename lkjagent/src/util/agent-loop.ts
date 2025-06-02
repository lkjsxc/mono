import { generateSystemPrompt } from './prompt';
import { callLLM } from './llm';
import { parseActionsFromXml } from './xml';
import { executeAction } from './executor';

/**
 * Main agent operational loop
 */
export async function runAgent(): Promise<void> {
  try {
    while (true) {
      // Generate system prompt with current state
      const systemPrompt = await generateSystemPrompt();

      // Get actions from LLM
      const llmResponse = await callLLM(systemPrompt);
      const actions = parseActionsFromXml(llmResponse);

      // Execute each action
      for (const action of actions) {
        await executeAction(action);
      }
    }
  } catch (error) {
    console.error('Agent execution error:', error);
    throw error;
  }
}

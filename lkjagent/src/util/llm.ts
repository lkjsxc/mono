/**
 * LLM communication utilities
 */

import axios from 'axios';
import { load_config } from '../config/config_manager';

/**
 * Call LLM with a prompt and get response
 */
export async function call_llm(prompt: string): Promise<string> {
  const config = await load_config();
  
  try {
    const response = await axios.post(config.llm_api_url || 'http://localhost:1234/v1/chat/completions', {
      model: config.llm_model || 'local-model',
      messages: [
        {
          role: 'system',
          content: prompt
        }
      ],
      max_tokens: config.llm_max_tokens || 1000,
      temperature: config.llm_temperature || 0.7,
      stream: false
    }, {
      headers: {
        'Content-Type': 'application/json'
      },
      timeout: 60000 // 60 second timeout
    });
    
    if (response.data && response.data.choices && response.data.choices.length > 0) {
      return response.data.choices[0].message.content || '';
    } else {
      throw new Error('Invalid response format from LLM');
    }
  } catch (error) {
    if (axios.isAxiosError(error)) {
      if (error.code === 'ECONNREFUSED') {
        throw new Error(`Could not connect to LLM server at ${config.llm_api_url}. Make sure your LLM server is running.`);
      } else if (error.response) {
        throw new Error(`LLM server error: ${error.response.status} - ${error.response.statusText}`);
      } else if (error.request) {
        throw new Error('No response from LLM server');
      }
    }
    throw new Error(`LLM communication failed: ${error instanceof Error ? error.message : String(error)}`);
  }
}

/**
 * Test LLM connection
 */
export async function test_llm_connection(): Promise<boolean> {
  try {
    await call_llm('Hello, please respond with "OK"');
    return true;
  } catch (error) {
    console.error('LLM connection test failed:', error);
    return false;
  }
}

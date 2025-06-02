import axios from 'axios';

/**
 * Make an API call to LM Studio's local API endpoint
 * @param prompt The prompt to send to the LLM
 * @returns The LLM's response
 */
export async function callLLM(prompt: string): Promise<string> {
  try {
    // Configure the API request to LM Studio
    const response = await axios.post('http://localhost:1234/v1/chat/completions', {
      messages: [
        {
          role: 'system',
          content: 'You are lkjagent, a helpful AI assistant. Reply only with XML actions in the format specified in the system prompt.'
        },
        {
          role: 'user',
          content: prompt
        }
      ],
      temperature: 0.7,
      stream: false
    });

    const llmResponse = response.data.choices[0].message.content;    // Validate that the response is in XML format
    if (!llmResponse.includes('<actions>')) {
      console.warn('LLM response is not in expected XML format, wrapping it');
      return `<actions><action><kind>memory_set</kind><target_path>/sys/response</target_path><content>${llmResponse}</content></action></actions>`;
    }

    return llmResponse;
  } catch (error: any) {
    console.error('Error calling LLM:', error);
    // Return a fallback response in case of error
    return `<actions><action><kind>memory_set</kind><target_path>/sys/error</target_path><content>Error communicating with LLM: ${error.message}</content></action></actions>`;
  }
}

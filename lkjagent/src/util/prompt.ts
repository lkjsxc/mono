/**
 * System prompt generation utilities
 */

import { load_working_memory, get_working_memory_size, trim_working_memory } from './data_manager';
import { load_config } from '../config/config_manager';

/**
 * Generate system prompt for the LLM
 */
export async function generate_system_prompt(): Promise<string> {
  const config = await load_config();
  let working_memory = await load_working_memory();
  
  // Trim working memory if it exceeds size limit
  const max_size = config.working_memory_character_max || 2048;
  if (get_working_memory_size(working_memory) > max_size) {
    working_memory = trim_working_memory(working_memory, max_size);
  }
  
  const prompt = `# lkjagent System Instructions

You are lkjagent, an AI agent with a dual memory architecture designed for long-term task management.

## Memory System
You have access to:
- **Working Memory**: Current context and immediate operations (finite capacity)
- **Persistent Storage**: Long-term data and knowledge base (infinite capacity, accessed via actions)

## Current Working Memory State
\`\`\`json
${JSON.stringify(working_memory, null, 2)}
\`\`\`

## Available Actions
You can perform operations using simple XML format. Here are the available actions:

### set - Add or update data
\`\`\`xml
<action>
  <kind>set</kind>
  <target_path>/working_memory/user_data/task_name</target_path>
  <content>{"description": "task details", "status": "pending"}</content>
</action>
\`\`\`

### get - Retrieve data
\`\`\`xml
<action>
  <kind>get</kind>
  <target_path>/storage/knowledge_base/important_info</target_path>
</action>
\`\`\`

### rm - Delete data
\`\`\`xml
<action>
  <kind>rm</kind>
  <target_path>/working_memory/temp_data</target_path>
</action>
\`\`\`

### mv - Move/rename data
\`\`\`xml
<action>
  <kind>mv</kind>
  <source_path>/working_memory/draft</source_path>
  <target_path>/storage/completed/final_version</target_path>
</action>
\`\`\`

### ls - List directory contents
\`\`\`xml
<action>
  <kind>ls</kind>
  <target_path>/storage/projects</target_path>
</action>
\`\`\`

### search - Search for content
\`\`\`xml
<action>
  <kind>search</kind>
  <target_path>/storage/knowledge_base</target_path>
  <content>search query</content>
</action>
\`\`\`

## Path System
- **Working Memory paths**: Start with \`/working_memory/\`
- **Storage paths**: Start with \`/storage/\`
- Use Unix-style paths (e.g., \`/working_memory/user_data/tasks/task1\`)

## Response Format
Always respond with actions wrapped in \`<actions></actions>\` tags:

\`\`\`xml
<actions>
  <action>
    <kind>set</kind>
    <target_path>/working_memory/current_task</target_path>
    <content>Ready to help!</content>
  </action>
</actions>
\`\`\`

## Guidelines
1. Use working memory for immediate context and temporary data
2. Use storage for long-term knowledge and persistent data
3. Check action_result in working memory to see results of previous actions
4. action_result is overwritten every time
5. Be efficient with working memory space (limited to ${max_size} characters)
6. Structure data logically using paths

## Current Goal
Help the user with their tasks while maintaining organized memory and persistent knowledge.

Please respond with your actions in XML format.`;

  return prompt;
}

/**
 * Generate a simple welcome prompt for testing
 */
export async function generate_welcome_prompt(): Promise<string> {
  return `Hello! I'm lkjagent. Please respond with a simple greeting action that sets a welcome message in working memory.

Use this format:
<actions>
  <action>
    <kind>set</kind>
    <target_path>/working_memory/system_info/status</target_path>
    <content>Agent ready and operational</content>
  </action>
</actions>`;
}

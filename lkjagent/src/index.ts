import * as fs from 'fs/promises';
import * as path from 'path';
import axios from 'axios';
import { MemoryState, StorageState, ToolAction, XmlString } from './types/common';
import { ram_add } from './tool/ram_add';
import { ram_remove } from './tool/ram_remove';
import { storage_load } from './tool/storage_load';
import { storage_remove } from './tool/storage_remove';
import { storage_search } from './tool/storage_search';
import { storage_store } from './tool/storage_store';

/**
 * Convert JSON data to XML format for LLM communication
 */
function jsonToXml(obj: any, rootTag: string, indent = ''): string {
  if (typeof obj !== 'object' || obj === null) {
    return String(obj);
  }

  const xmlParts: string[] = [`${indent}<${rootTag}>`];
  
  for (const [key, value] of Object.entries(obj)) {
    if (Array.isArray(value)) {
      for (const item of value) {
        xmlParts.push(jsonToXml(item, key, indent + '  '));
      }
    } else if (typeof value === 'object' && value !== null) {
      xmlParts.push(jsonToXml(value, key, indent + '  '));
    } else {
      xmlParts.push(`${indent}  <${key}>${value}</${key}>`);
    }
  }
  
  xmlParts.push(`${indent}</${rootTag}>`);
  return xmlParts.join('\n');
}

/**
 * Parse XML string to extract actions
 * Basic implementation - in a real system, use a proper XML parser
 */
function parseActionsFromXml(xml: string): ToolAction[] {
  const actions: ToolAction[] = [];
  const actionMatches = xml.match(/<action>.*?<\/action>/gs);

  if (!actionMatches) return actions;

  for (const actionXml of actionMatches) {
    const kind = actionXml.match(/<kind>(.*?)<\/kind>/)?.[1];
    const path = actionXml.match(/<path>(.*?)<\/path>/)?.[1];
    const content = actionXml.match(/<content>(.*?)<\/content>/)?.[1];
    const sourcePath = actionXml.match(/<source_path>(.*?)<\/source_path>/)?.[1];

    if (kind) {
      actions.push({
        kind: kind as ToolAction['kind'],
        ...(path && { path }),
        ...(content && { content }),
        ...(sourcePath && { source_path: sourcePath })
      });
    }
  }

  return actions;
}

/**
 * Generate system prompt with current memory state
 */
async function generateSystemPrompt(): Promise<string> {
  const memoryPath = path.join(__dirname, '..', 'data', 'memory.json');
  const storagePath = path.join(__dirname, '..', 'data', 'storage.json');

  const [memoryData, storageData] = await Promise.all([
    fs.readFile(memoryPath, 'utf-8').then(JSON.parse),
    fs.readFile(storagePath, 'utf-8').then(JSON.parse)
  ]);

  return `<lkjagent_system_setup>
  <persona>
    <name>lkjagent</name>
    <role>An AI agent that communicates using XML actions</role>
  </persona>

  <memory_architecture>
    ${jsonToXml(memoryData, 'ram')}
    ${jsonToXml(storageData, 'storage')}
  </memory_architecture>

  <output_format_specification>
    <format>
      You must respond with XML in this exact format:
      <actions>
        <action>
          <kind>add|remove|edit|storage_load|storage_store|storage_search</kind>
          <path>path.to.data</path>
          <content>content to store or search</content>
          <source_path>optional source path for storage operations</source_path>
        </action>
      </actions>
    </format>
    <rules>
      1. Always wrap your response in <actions> tags
      2. Each action must be wrapped in <action> tags
      3. The kind tag is required and must be one of: add, remove, edit, storage_load, storage_store, storage_search
      4. The path tag is required for all actions except storage_search
      5. The content tag is required for add, edit, and storage_search actions
      6. The source_path tag is required only for storage_store action
    </rules>
    <example>
      <actions>
        <action>
          <kind>add</kind>
          <path>ram.todo.new_task</path>
          <content>Complete the project documentation</content>
        </action>
        <action>
          <kind>storage_store</kind>
          <path>storage.completed_tasks</path>
          <source_path>ram.todo.completed</source_path>
        </action>
      </actions>
    </example>
  </output_format_specification>
</lkjagent_system_setup>`;
}

/**
 * Make an API call to LM Studio's local API endpoint
 * @param prompt The prompt to send to the LLM
 * @returns The LLM's response
 */
async function callLLM(prompt: string): Promise<string> {
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

    const llmResponse = response.data.choices[0].message.content;

    // Validate that the response is in XML format
    if (!llmResponse.includes('<actions>')) {
      console.warn('LLM response is not in expected XML format, wrapping it');
      return `<actions><action><kind>add</kind><path>ram.thinking_log</path><content>${llmResponse}</content></action></actions>`;
    }

    return llmResponse;
  } catch (error: any) {
    console.error('Error calling LLM:', error);
    // Return a fallback response in case of error
    return `<actions><action><kind>add</kind><path>ram.thinking_log</path><content>Error communicating with LLM: ${error.message}</content></action></actions>`;
  }
}

/**
 * Execute a single tool action
 */
async function executeAction(action: ToolAction): Promise<void> {
  switch (action.kind) {
    case 'add':
      if (!action.path || action.content === undefined) {
        throw new Error('Add action requires path and content');
      }
      await ram_add(action.path, action.content);
      break;
      
    case 'remove':
      if (!action.path) {
        throw new Error('Remove action requires path');
      }
      await ram_remove(action.path);
      break;
      
    case 'edit':
      if (!action.path || action.content === undefined) {
        throw new Error('Edit action requires path and content');
      }
      await ram_add(action.path, action.content); // Edit is same as add for our implementation
      break;
      
    case 'storage_load':
      if (!action.path) {
        throw new Error('Storage load action requires path');
      }
      await storage_load(action.path);
      break;
      
    case 'storage_store':
      if (!action.source_path || !action.path) {
        throw new Error('Storage store action requires source_path and path');
      }
      await storage_store(action.source_path, action.path);
      break;
      
    case 'storage_search':
      if (!action.content) {
        throw new Error('Storage search action requires content');
      }
      await storage_search(action.content);
      break;
  }
}

/**
 * Main agent operational loop
 */
async function runAgent(): Promise<void> {
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
      
      // For demonstration, we'll just run once
      // In a real implementation, we would check for completion conditions
      break;
    }
  } catch (error) {
    console.error('Agent execution error:', error);
    throw error;
  }
}

// Start the agent
if (require.main === module) {
  runAgent().catch(console.error);
}
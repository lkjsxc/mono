import * as fs from 'fs/promises';
import * as path from 'path';
import axios from 'axios';
import { LogEntry, MemoryState, StorageState, ToolAction, ToolKind, XmlString } from './types/common';
import { ConfigManager } from './config/config-manager';
import { logAction } from './tool/action_logger';
import { memory_set } from './tool/memory_set';
import { memory_remove } from './tool/memory_remove';
import { storage_get } from './tool/storage_get';
import { storage_remove } from './tool/storage_remove';
import { storage_search } from './tool/storage_search';
import { storage_set } from './tool/storage_set';
import { storage_ls } from './tool/storage_ls';

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
      // Convert XML content to JSON object if it contains XML tags
      let processedContent = content;
      if (content && content.includes('<') && content.includes('>')) {
        const contentObj: any = {};
        const xmlTags = content.match(/<(\w+)>(.*?)<\/\1>/g);
        if (xmlTags) {
          xmlTags.forEach(tag => {
            const [, name, value] = tag.match(/<(\w+)>(.*?)<\/\1>/) || [];
            if (name && value) {
              contentObj[name] = value;
            }
          });
          processedContent = contentObj;
        }
      }

      // Validate required fields based on action kind
      const action: ToolAction = {
        kind: kind as ToolAction['kind'],
        path: path || '',
        content: processedContent || {},
        source_path: sourcePath || ''
      };

      // Skip invalid actions
      switch (action.kind) {
        case 'memory_set':
          if (!action.path || action.content === undefined) {
            console.warn(`Skipping ${action.kind} action: missing path or content`);
            continue;
          }
          break;
        case 'memory_remove':
          if (!action.path) {
            console.warn('Skipping remove action: missing path');
            continue;
          }
          break;
        case 'storage_get':
          if (!action.path) {
            console.warn('Skipping storage_get action: missing path');
            continue;
          }
          break;
        case 'storage_remove':
          if (!action.path) {
            console.warn('Skipping storage_remove action: missing path');
            continue;
          }
          break;
        case 'storage_set':
          if (!action.source_path || !action.path) {
            console.warn('Skipping storage_set action: missing source_path or path');
            continue;
          }
          break;
        case 'storage_search':
          if (!action.content) {
            console.warn('Skipping storage_search action: missing content');
            continue;
          }
          break;
        case 'storage_ls':
          if (!action.path) {
            console.warn('Skipping storage_ls action: missing path');
            continue;
          }
          break;
        default:
          console.warn(`Skipping unknown action kind: ${action.kind}`);
          continue;
      }

      actions.push(action);
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

  const configManager = await ConfigManager.getInstance();
  const memoryConfig = configManager.getMemoryConfig();
  const memoryCharacterLimit = memoryConfig.memoryCharacterLimit;

  return `<lkjagent_system_setup>
  <persona>
    <name>lkjagent</name>
    <role>An AI agent with finite memory and infinite storage that communicates using XML actions</role>
  </persona>

  ${jsonToXml(memoryData, 'root')}

  <output_format_specification>
    <format>
      You must respond with XML in this exact format:
      <actions>
        <action>
          <kind>memory_set|memory_remove|storage_set|storage_remove|storage_get|storage_search|storage_ls</kind>
          <target_path>target path</target_path>
          <source_path>source path</source_path>
          <content>content</content>
        </action>
      </actions>
    </format>
    <rules>
      Always wrap your response in <actions> tags
      Each action must be wrapped in <action> tags
      must not exceed ${memoryCharacterLimit} tokens
      A directory should have no more than 8 direct children
      It is recommended that the key be 4 tokens or less
      Make proactive use of storage
    </rules>
    <example>
      <actions>
        <action>
          <kind>memory_set</kind>
          <path>/user/todo/new_task</path>
          <content>Complete the project documentation</content>
        </action>
        <action>
          <kind>storage_set</kind>
          <path>/completed_tasks</path>
          <source_path>/user/todo/completed</source_path>
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
      return `<actions><action><kind>add</kind><path>/sys/thinking_log</path><content>${llmResponse}</content></action></actions>`;
    }

    return llmResponse;
  } catch (error: any) {
    console.error('Error calling LLM:', error);
    // Return a fallback response in case of error
    return `<actions><action><kind>add</kind><path>/sys/thinking_log</path><content>Error communicating with LLM: ${error.message}</content></action></actions>`;
  }
}

/**
 * Execute a single tool action with error handling
 */
async function executeAction(action: ToolAction): Promise<void> {  // Create initial log entry
  const entry: LogEntry = {
    timestamp: Date.now(),
    actionType: action.kind as ToolKind,
    path: action.path,
    sourcePath: action.source_path,
    content: action.content,
    status: 'error'  // Default to error, will be updated to success if no error
  };

  try {
    // Validate required fields first
    if (!action.kind) {
      console.warn('Skipping action: missing kind');
      entry.error = 'Missing action kind';
      await logAction(entry);
      return;
    }

    // Handle each action type
    switch (action.kind) {
      case 'memory_set':
        if (!action.path || action.content === undefined) {
          console.warn(`Skipping ${action.kind}: missing path or content`);
          entry.error = `Missing ${!action.path ? 'path' : 'content'}`;
          await logAction(entry);
          return;
        }
        await memory_set(action.path, action.content);
        break;

      case 'memory_remove':
        if (!action.path) {
          console.warn('Skipping remove: missing path');
          entry.error = 'Missing path';
          await logAction(entry);
          return;
        }
        await memory_remove(action.path);
        break;

      case 'storage_set':
        if (!action.source_path || !action.path) {
          console.warn('Skipping storage_set: missing paths');
          entry.error = `Missing ${!action.source_path ? 'source_path' : 'path'}`;
          await logAction(entry);
          return;
        }
        await storage_set(action.source_path, action.path);
        break;

      case 'storage_remove':
        if (!action.path) {
          console.warn('Skipping storage_remove: missing path');
          entry.error = 'Missing path';
          await logAction(entry);
          return;
        }
        await storage_remove(action.path);
        break;

      case 'storage_get':
        if (!action.path) {
          console.warn('Skipping storage_get: missing path');
          entry.error = 'Missing path';
          await logAction(entry);
          return;
        }
        await memory_set('sys/loaded_data', storage_get(action.path));
        break;

      case 'storage_search':
        if (!action.content) {
          console.warn('Skipping storage_search: missing content');
          entry.error = 'Missing content';
          await logAction(entry);
          return;
        }
        await memory_set('sys/loaded_data', storage_search(action.content));
        break;

      case 'storage_ls':
        if (!action.path) {
          console.warn('Skipping storage_ls: missing path');
          entry.error = 'Missing path';
          await logAction(entry);
          return;
        }
        await memory_set('sys/loaded_data', storage_ls(action.path));
        break; default:
        console.warn(`Skipping unknown action kind: ${action.kind}`);
        entry.error = `Unknown action kind: ${action.kind}`;
        await logAction(entry);
        return;
    }

    // If we get here, the action was successful
    entry.status = 'success';
    await logAction(entry);
  } catch (error) {
    console.warn(`Error executing ${action.kind} action:`, error);
    // Add error to thinking_log and log the error
    entry.error = error instanceof Error ? error.message : String(error);
    await logAction(entry);
    await memory_set('/sys/thinking_log', `Error executing ${action.kind} action: ${error}`);
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

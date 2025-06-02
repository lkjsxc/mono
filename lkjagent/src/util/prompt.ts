import * as fs from 'fs/promises';
import * as path from 'path';
import { ConfigManager } from '../config/config-manager';
import { jsonToXml } from './xml';

/**
 * Generate system prompt with current memory state
 */
export async function generateSystemPrompt(): Promise<string> {
  const memoryPath = path.join(__dirname, '..', '..', 'data', 'memory.json');
  const storagePath = path.join(__dirname, '..', '..', 'data', 'storage.json');

  const [memoryData, storageData] = await Promise.all([
    fs.readFile(memoryPath, 'utf-8').then(JSON.parse),
    fs.readFile(storagePath, 'utf-8').then(JSON.parse)
  ]);

  const configManager = await ConfigManager.getInstance();
  const memoryConfig = configManager.getMemoryConfig();
  const MemoryCharacterMax = memoryConfig.MemoryCharacterMax;
  const DirectChildMax = memoryConfig.DirectChildMax;

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
          <kind>memory_set|memory_remove|memory_mv|storage_set|storage_remove|storage_get|storage_search|storage_ls</kind>
          <target_path>target path</target_path>
          <source_path>source path</source_path>
          <content>content</content>
        </action>
      </actions>
    </format>
    <rules>
      Always wrap your response in <actions> tags
      Each action must be wrapped in <action> tags
      must not exceed ${MemoryCharacterMax} tokens
      A directory should have no more than ${DirectChildMax} direct children
      It is recommended that the key be 4 tokens or less
      Make proactive use of storage
    </rules>
    <example>
      <actions>
        <action>
          <kind>memory_set</kind>
          <target_path>/user/todo/new_task</target_path>
          <content>Complete the project documentation</content>
        </action>
        <action>
          <kind>storage_set</kind>
          <target_path>/completed_tasks</target_path>
          <source_path>/user/todo/completed</source_path>
        </action>
      </actions>
    </example>
  </output_format_specification>
</lkjagent_system_setup>`;
}

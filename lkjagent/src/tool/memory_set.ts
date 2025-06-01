import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath } from '../types/common';
import { validatePath, setValueAtPath } from '../util/json';

/**
 * Adds or updates data at a specified path in RAM
 * @param targetPath - Dot-separated path in RAM (e.g., 'ram.todo.new_task')
 * @param content - Content to add/update at the path
 */
export async function memory_set(targetPath: JsonPath, content: any): Promise<void> {
  const memoryPath = path.join(__dirname, '..', '..', 'data', 'memory.json');
  
  // Validate path format
  validatePath(targetPath);

  // Read current memory state
  const memoryContent = await fs.readFile(memoryPath, 'utf-8');
  const memoryData = JSON.parse(memoryContent);

  // Update the value at the specified path
  setValueAtPath(memoryData, targetPath, content);

  // Write back to file
  await fs.writeFile(memoryPath, JSON.stringify(memoryData, null, 2), 'utf-8');
}
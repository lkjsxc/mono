import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath } from '../types/common';
import { validatePath, setValueAtPath } from '../util/json';

/**
 * Adds or updates data at a specified path in memory
 * @param target_path - Dot-separated path in memory (e.g., 'ram.todo.new_task')
 * @param content - Content to add/update at the path
 */
export async function memory_set(target_path: JsonPath, content: any): Promise<void> {
  const memoryPath = path.join(__dirname, '..', '..', 'data', 'memory.json');
  
  // Validate path format
  validatePath(target_path);

  // Read current memory state
  const memoryContent = await fs.readFile(memoryPath, 'utf-8');
  const memoryData = JSON.parse(memoryContent);

  // Update the value at the specified path
  setValueAtPath(memoryData, target_path, content);

  // Write back to file
  await fs.writeFile(memoryPath, JSON.stringify(memoryData, null, 2), 'utf-8');
}
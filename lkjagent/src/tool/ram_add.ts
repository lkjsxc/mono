import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath } from '../types/common';

/**
 * Adds or updates data at a specified path in RAM
 * @param targetPath - Dot-separated path in RAM (e.g., 'ram.todo.new_task')
 * @param content - Content to add/update at the path
 */
export async function ram_add(targetPath: JsonPath, content: any): Promise<void> {
  const memoryPath = path.join(__dirname, '..', '..', 'data', 'memory.json');
  
  try {
    // Read current memory state
    const memoryData = JSON.parse(await fs.readFile(memoryPath, 'utf-8'));
    
    // Split the path into parts and traverse/create the object structure
    const parts = targetPath.split('.');
    let current = memoryData;
    
    for (let i = 0; i < parts.length - 1; i++) {
      const part = parts[i];
      if (!(part in current)) {
        current[part] = {};
      }
      current = current[part];
    }
    
    // Set the value at the final path segment
    const lastPart = parts[parts.length - 1];
    current[lastPart] = content;
    
    // Write updated memory back to file
    await fs.writeFile(memoryPath, JSON.stringify(memoryData, null, 2), 'utf-8');
  } catch (error) {
    throw new Error(`Failed to add/update RAM at path ${targetPath}: ${error}`);
  }
}
import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath } from '../types/common';

/**
 * Removes data at a specified path in RAM
 * @param targetPath - Dot-separated path in RAM (e.g., 'ram.todo.task_to_remove')
 */
export async function ram_remove(targetPath: JsonPath): Promise<void> {
  const memoryPath = path.join(__dirname, '..', '..', 'data', 'memory.json');
  
  try {
    // Read current memory state
    const memoryData = JSON.parse(await fs.readFile(memoryPath, 'utf-8'));
    
    // Split the path into parts and traverse the object structure
    const parts = targetPath.split('.');
    let current = memoryData;
    
    // Traverse to the parent of the target to remove
    for (let i = 0; i < parts.length - 1; i++) {
      const part = parts[i];
      if (!(part in current)) {
        throw new Error(`Path segment '${part}' not found in RAM`);
      }
      current = current[part];
    }
    
    // Remove the target
    const lastPart = parts[parts.length - 1];
    if (lastPart in current) {
      delete current[lastPart];
      
      // Write updated memory back to file
      await fs.writeFile(memoryPath, JSON.stringify(memoryData, null, 2), 'utf-8');
    } else {
      throw new Error(`Target '${lastPart}' not found in RAM`);
    }
  } catch (error) {
    throw new Error(`Failed to remove from RAM at path ${targetPath}: ${error}`);
  }
}
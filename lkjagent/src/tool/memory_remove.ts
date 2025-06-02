import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath } from '../types/common';
import { validatePath, getValueAtPath } from '../util/json';

/**
 * Removes data at a specified path in memory
 * @param target_path - Dot-separated path in memory (e.g., 'ram.todo.task_to_remove')
 */
export async function memory_remove(target_path: JsonPath): Promise<void> {
  const memoryPath = path.join(__dirname, '..', '..', 'data', 'memory.json');
  
  try {
    // Validate path format
    validatePath(target_path);
    
    // Read current memory state
    const memoryData = JSON.parse(await fs.readFile(memoryPath, 'utf-8'));
    
    // Split the path into parts
    const parts = target_path.split('/').filter(p => p);
    if (parts.length === 0) {
      throw new Error('Cannot remove root memory node');
    }
    
    // Get the parent path and the target to remove
    const parentPath = parts.length > 1 ? '/' + parts.slice(0, -1).join('/') : '/';
    const lastPart = parts[parts.length - 1];
    
    // Get the parent object
    const parent = parentPath === '/' ? memoryData : getValueAtPath(memoryData, parentPath);
    
    if (typeof parent !== 'object' || parent === null) {
      throw new Error(`Parent path '${parentPath}' is not an object`);
    }
    
    // Remove the target
    if (lastPart in parent) {
      delete parent[lastPart];
      
      // Write updated memory back to file
      await fs.writeFile(memoryPath, JSON.stringify(memoryData, null, 2), 'utf-8');
    } else {
      throw new Error(`Target '${lastPart}' not found in memory`);
    }
  } catch (error) {
    throw new Error(`Failed to remove from memory at path ${target_path}: ${error}`);
  }
}
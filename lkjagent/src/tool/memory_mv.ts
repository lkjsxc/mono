import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath } from '../types/common';
import { validatePath, getValueAtPath, setValueAtPath } from '../util/json';

/**
 * Moves data from one path to another in memory
 * @param source_path - Path in memory where the data is currently located (e.g., '/ram/todo/old_task')
 * @param target_path - Path in memory where to move the data (e.g., '/ram/todo/new_task')
 */
export async function memory_mv(source_path: JsonPath, target_path: JsonPath): Promise<void> {
  const memoryPath = path.join(__dirname, '..', '..', 'data', 'memory.json');
  
  try {
    // Validate both path formats
    validatePath(source_path);
    validatePath(target_path);
    
    // Read current memory state
    const memoryData = JSON.parse(await fs.readFile(memoryPath, 'utf-8'));
    
    // Get the value from the source path
    const valueToMove = getValueAtPath(memoryData, source_path);
    
    // Set the value at the target path
    setValueAtPath(memoryData, target_path, valueToMove);
    
    // Remove the value from the source path
    const sourceParts = source_path.split('/').filter(p => p);
    if (sourceParts.length === 0) {
      throw new Error('Cannot move root memory node');
    }
    
    // Get the parent path and the source key to remove
    const parentPath = sourceParts.length > 1 ? '/' + sourceParts.slice(0, -1).join('/') : '/';
    const lastPart = sourceParts[sourceParts.length - 1];
    
    // Get the parent object and remove the source
    const parent = parentPath === '/' ? memoryData : getValueAtPath(memoryData, parentPath);
    
    if (typeof parent !== 'object' || parent === null) {
      throw new Error(`Parent path '${parentPath}' is not an object`);
    }
    
    if (lastPart in parent) {
      delete parent[lastPart];
    } else {
      throw new Error(`Source '${lastPart}' not found in memory at path ${source_path}`);
    }
    
    // Write updated memory back to file
    await fs.writeFile(memoryPath, JSON.stringify(memoryData, null, 2), 'utf-8');
    
  } catch (error) {
    throw new Error(`Failed to move in memory from ${source_path} to ${target_path}: ${error}`);
  }
}

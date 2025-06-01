import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath } from '../types/common';
import { ram_add } from './ram_add';

/**
 * Loads data from a specified path in Storage and places it in RAM's loaded_data area
 * @param targetPath - Dot-separated path in Storage (e.g., 'storage.knowledge_base.policy')
 * @returns The loaded data
 */
export async function storage_load(targetPath: JsonPath): Promise<any> {
  const storagePath = path.join(__dirname, '..', '..', 'data', 'storage.json');
  
  try {
    // Read current storage state
    const storageData = JSON.parse(await fs.readFile(storagePath, 'utf-8'));
    
    // Split the path into parts and traverse the object structure
    const parts = targetPath.split('.');
    let current = storageData;
    
    // Traverse to the target data
    for (const part of parts) {
      if (!(part in current)) {
        throw new Error(`Path segment '${part}' not found in Storage`);
      }
      current = current[part];
    }
    
    // Store the loaded data in RAM's loaded_data section
    await ram_add('ram.loaded_data', current);
    
    return current;
  } catch (error) {
    throw new Error(`Failed to load from Storage at path ${targetPath}: ${error}`);
  }
}
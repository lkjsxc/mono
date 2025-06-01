import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath } from '../types/common';

/**
 * Removes data at a specified path in Storage
 * @param targetPath - Dot-separated path in Storage (e.g., 'storage.archived_data.old_task')
 */
export async function storage_remove(targetPath: JsonPath): Promise<void> {
  const storagePath = path.join(__dirname, '..', '..', 'data', 'storage.json');
  
  try {
    // Read current storage state
    const storageData = JSON.parse(await fs.readFile(storagePath, 'utf-8'));
    
    // Split the path into parts and traverse the object structure
    const parts = targetPath.split('.');
    let current = storageData;
    
    // Traverse to the parent of the target to remove
    for (let i = 0; i < parts.length - 1; i++) {
      const part = parts[i];
      if (!(part in current)) {
        throw new Error(`Path segment '${part}' not found in Storage`);
      }
      current = current[part];
    }
    
    // Remove the target
    const lastPart = parts[parts.length - 1];
    if (lastPart in current) {
      delete current[lastPart];
      
      // Write updated storage back to file
      await fs.writeFile(storagePath, JSON.stringify(storageData, null, 2), 'utf-8');
    } else {
      throw new Error(`Target '${lastPart}' not found in Storage`);
    }
  } catch (error) {
    throw new Error(`Failed to remove from Storage at path ${targetPath}: ${error}`);
  }
}
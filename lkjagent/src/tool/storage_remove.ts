import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath } from '../types/common';
import { validatePath, getValueAtPath } from '../util/json';

/**
 * Removes data at a specified path in Storage
 * @param target_path - Dot-separated path in Storage (e.g., 'storage.archived_data.old_task')
 */
export async function storage_remove(target_path: JsonPath): Promise<void> {
  const storagePath = path.join(__dirname, '..', '..', 'data', 'storage.json');
    try {
    // Validate path format
    validatePath(target_path);
    
    // Read current storage state
    const storageData = JSON.parse(await fs.readFile(storagePath, 'utf-8'));
      // Split the path into parts and handle special cases
    const parts = target_path.split('/').filter(p => p);
    if (parts.length === 0) {
      throw new Error('Cannot remove root storage node');
    }
    
    // Get the parent path and the target to remove
    const parentPath = parts.length > 1 ? '/' + parts.slice(0, -1).join('/') : '/';
    const lastPart = parts[parts.length - 1];
    
    // Get the parent object
    const parent = parentPath === '/' ? storageData : getValueAtPath(storageData, parentPath);
    
    if (typeof parent !== 'object' || parent === null) {
      throw new Error(`Parent path '${parentPath}' is not an object`);
    }
    
    // Remove the target
    if (lastPart in parent) {
      delete parent[lastPart];
      
      // Write updated storage back to file
      await fs.writeFile(storagePath, JSON.stringify(storageData, null, 2), 'utf-8');
    } else {
      throw new Error(`Target '${lastPart}' not found in Storage`);
    }
  } catch (error) {
    throw new Error(`Failed to remove from Storage at path ${target_path}: ${error}`);
  }
}
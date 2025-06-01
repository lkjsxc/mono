import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath } from '../types/common';
import { validatePath, getValueAtPath } from '../util/json';

/**
 * Lists all keys at a specified path in Storage
 * @param targetPath - Dot-separated path in Storage (e.g., '/storage/archived_data')
 * @returns Array of keys found at the target path
 */
export async function storage_ls(targetPath: JsonPath): Promise<string[]> {
  const storagePath = path.join(__dirname, '..', '..', 'data', 'storage.json');
  
  try {
    // Validate path format
    validatePath(targetPath);
    
    // Read current storage state
    const storageData = JSON.parse(await fs.readFile(storagePath, 'utf-8'));
    
    // Get the object at target path
    const targetObj = getValueAtPath(storageData, targetPath);
    
    // If target is not an object or is null, return empty array
    if (typeof targetObj !== 'object' || targetObj === null) {
      return [];
    }
    
    // Return array of keys at this path
    return Object.keys(targetObj);
  } catch (error) {
    throw new Error(`Failed to list Storage contents at path ${targetPath}: ${error}`);
  }
}
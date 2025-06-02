import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath, StorageLsResult } from '../types/common';
import { validatePath, getValueAtPath } from '../util/json';

/**
 * Lists all keys at a specified path in Storage with their string lengths
 * @param target_path - Dot-separated path in Storage (e.g., '/storage/archived_data')
 * @returns Array of objects with key names and their string lengths
 */
export async function storage_ls(target_path: JsonPath): Promise<StorageLsResult[]> {
  const storagePath = path.join(__dirname, '..', '..', 'data', 'storage.json');
  
  try {
    // Validate path format
    validatePath(target_path);
    
    // Read current storage state
    const storageData = JSON.parse(await fs.readFile(storagePath, 'utf-8'));
    
    // Get the object at target path
    const targetObj = getValueAtPath(storageData, target_path);
    
    // If target is not an object or is null, return empty array
    if (typeof targetObj !== 'object' || targetObj === null) {
      return [];
    }
    
    // Return array of objects with keys and their string lengths
    return Object.keys(targetObj).map(key => {
      const value = targetObj[key];
      const stringLength = typeof value === 'string' 
        ? value.length 
        : JSON.stringify(value).length;
      
      return {
        key,
        stringLength
      };
    });
  } catch (error) {
    throw new Error(`Failed to list Storage contents at path ${target_path}: ${error}`);
  }
}
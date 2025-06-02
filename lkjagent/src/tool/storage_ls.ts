import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath, StorageLsResult } from '../types/common';

/**
 * Lists all keys at a specified path in Storage with their string lengths
 * @param target_path - Unix-style path in Storage (e.g., '/archived_data')
 * @returns Array of objects with key names and their string lengths
 */
export async function storage_ls(target_path: JsonPath): Promise<StorageLsResult[]> {
  const storagePath = path.join(__dirname, '..', '..', 'data', 'storage.json');
  
  try {
    // Read storage file
    const storageContent = await fs.readFile(storagePath, 'utf-8');
    const storageData = JSON.parse(storageContent);
    
    // Navigate to target path
    let currentObj = storageData;
    
    // Handle root path
    if (target_path === '/') {
      currentObj = storageData;
    } else {
      // Split path and navigate
      const pathParts = target_path.split('/').filter(part => part.length > 0);
      
      for (const part of pathParts) {
        if (currentObj && typeof currentObj === 'object' && part in currentObj) {
          currentObj = currentObj[part];
        } else {
          // Path not found, return empty array
          return [];
        }
      }
    }
    
    // Check if current object is a valid object to list
    if (!currentObj || typeof currentObj !== 'object' || Array.isArray(currentObj)) {
      return [];
    }
    
    // Create results array
    const results: StorageLsResult[] = [];
    
    for (const key of Object.keys(currentObj)) {
      const value = currentObj[key];
      let stringLength = 0;
      
      if (typeof value === 'string') {
        stringLength = value.length;
      } else if (value !== null && value !== undefined) {
        try {
          stringLength = JSON.stringify(value).length;
        } catch {
          stringLength = 0;
        }
      }
      
      results.push({
        key,
        stringLength
      });
    }
    
    return results;
    
  } catch (error) {
    console.error(`storage_ls error for path ${target_path}:`, error);
    return [];
  }
}
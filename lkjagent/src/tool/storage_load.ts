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
    const parts = targetPath.split('/');
    let current = storageData;
    
    // Traverse to the target data
    for (const part of parts) {
      if (Array.isArray(current)) {
        // If current is an array, try to find the item
        const item = current.find(x => {
          if (typeof x === 'string') {
            // Try case-insensitive match and replace underscores with spaces
            const normalized = part.toLowerCase().replace(/_/g, ' ');
            return x.toLowerCase() === normalized;
          }
          return x.name === part || x.id === part;
        });
        if (!item) {
          // If not found, return empty object and continue
          current = {};
          break;
        }
        current = item;
      } else if (current && typeof current === 'object') {
        if (!(part in current)) {
          // Check if we're looking for something that might be in an array
          let found = false;
          for (const [key, value] of Object.entries(current)) {
            if (Array.isArray(value)) {
              const normalized = part.toLowerCase().replace(/_/g, ' ');
              const item = value.find(x => 
                typeof x === 'string' ? x.toLowerCase() === normalized : x.name === part
              );
              if (item) {
                current = item;
                found = true;
                break;
              }
            }
          }
          if (!found) {
            // If not found, return empty object and continue
            current = {};
            break;
          }
        } else {
          current = current[part];
        }
      } else {
        // If we can't traverse further, return empty object
        current = {};
        break;
      }
    }
    
    return current;
  } catch (error) {
    throw new Error(`Failed to load from Storage at path ${targetPath}: ${error}`);
  }
}
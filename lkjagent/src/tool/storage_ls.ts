import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath } from '../types/common';

/**
 * Lists contents at a specified path in Storage
 * @param targetPath - Dot-separated path in Storage (e.g., 'storage.knowledge_base')
 * @returns Array of available keys/items at the specified path
 */
export async function storage_ls(targetPath: JsonPath): Promise<string[]> {
  const storagePath = path.join(__dirname, '..', '..', 'data', 'storage.json');
  
  try {
    // Read current storage state
    const storageData = JSON.parse(await fs.readFile(storagePath, 'utf-8'));
    
    // Split the path into parts and traverse the object structure
    const parts = targetPath.split('.');
    let current = storageData;
    
    // Traverse to the target location
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
          return []; // Path not found
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
            return []; // Path not found
          }
        } else {
          current = current[part];
        }
      } else {
        return []; // Path not found or invalid
      }
    }
    
    // Get the contents at the current path
    if (Array.isArray(current)) {
      // For arrays, return the string values or name/id properties
      return current.map(item => {
        if (typeof item === 'string') return item;
        return item.name || item.id || JSON.stringify(item);
      });
    } else if (current && typeof current === 'object') {
      // For objects, return the keys
      return Object.keys(current);
    }
    
    // If we reach here, the path exists but has no listable contents
    return [];
  } catch (error) {
    throw new Error(`Failed to list Storage contents at path ${targetPath}: ${error}`);
  }
}
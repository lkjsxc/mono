import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath } from '../types/common';

/**
 * Retrieves data from a specified path in Storage
 * @param targetPath - Optional. Dot-separated path in Storage (e.g., 'storage.user_data'). If not provided, returns the entire storage.
 * @returns The data at the specified path, or the entire storage if no path is provided.
 */
export async function storage_getdir(targetPath?: JsonPath): Promise<any> {
  const storagePath = path.join(__dirname, '..', '..', 'data', 'storage.json');

  try {
    const storageData = JSON.parse(await fs.readFile(storagePath, 'utf-8'));

    if (!targetPath) {
      return storageData; // Return entire storage if no path is specified
    }

    const parts = targetPath.split('.');
    let current = storageData;

    for (let i = 0; i < parts.length; i++) {
      const part = parts[i];
      if (typeof current !== 'object' || current === null || !(part in current)) {
        throw new Error(`Path segment '${part}' not found in Storage`);
      }
      current = current[part];
    }
    return current;
  } catch (error) {
    throw new Error(`Failed to get from Storage at path ${targetPath || 'root'}: ${error}`);
  }
}
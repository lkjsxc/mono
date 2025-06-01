import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath } from '../types/common';

/**
 * Gets the value at a specific path in an object
 */
async function getValueAtPath(obj: any, pathParts: string[]): Promise<any> {
  let current = obj;
  for (const part of pathParts) {
    if (!(part in current)) {
      throw new Error(`Path segment '${part}' not found`);
    }
    current = current[part];
  }
  return current;
}

/**
 * Sets a value at a specific path in an object, creating intermediate objects as needed
 */
function setValueAtPath(obj: any, pathParts: string[], value: any): void {
  let current = obj;
  for (let i = 0; i < pathParts.length - 1; i++) {
    const part = pathParts[i];
    if (!(part in current)) {
      current[part] = {};
    }
    current = current[part];
  }
  const lastPart = pathParts[pathParts.length - 1];
  current[lastPart] = value;
}

/**
 * Stores data from RAM into Storage
 * @param sourcePath - Path in RAM where the data is located
 * @param destinationPath - Path in Storage where to store the data
 */
export async function storage_store(sourcePath: JsonPath, destinationPath: JsonPath): Promise<void> {
  const memoryPath = path.join(__dirname, '..', '..', 'data', 'memory.json');
  const storagePath = path.join(__dirname, '..', '..', 'data', 'storage.json');
  
  try {
    // Read current memory and storage states
    const memoryData = JSON.parse(await fs.readFile(memoryPath, 'utf-8'));
    const storageData = JSON.parse(await fs.readFile(storagePath, 'utf-8'));
    
    // Get the data from RAM
    const sourcePathParts = sourcePath.split('.');
    const dataToStore = await getValueAtPath(memoryData, sourcePathParts);
    
    // Store the data in Storage
    const destinationPathParts = destinationPath.split('.');
    setValueAtPath(storageData, destinationPathParts, dataToStore);
    
    // Write updated storage back to file
    await fs.writeFile(storagePath, JSON.stringify(storageData, null, 2), 'utf-8');
  } catch (error) {
    throw new Error(`Failed to store data from ${sourcePath} to ${destinationPath}: ${error}`);
  }
}
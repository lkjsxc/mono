import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath } from '../types/common';
import { validatePath, getValueAtPath, setValueAtPath } from '../util/json';

/**
 * Stores data from memory into Storage
 * @param sourcePath - Path in memory where the data is located
 * @param destinationPath - Path in Storage where to store the data
 */
export async function storage_set(sourcePath: JsonPath, destinationPath: JsonPath): Promise<void> {
  const memoryPath = path.join(__dirname, '..', '..', 'data', 'memory.json');
  const storagePath = path.join(__dirname, '..', '..', 'data', 'storage.json');

  // Validate both paths
  validatePath(sourcePath);
  validatePath(destinationPath);

  // Read current memory and storage states
  const memoryContent = await fs.readFile(memoryPath, 'utf-8');
  const storageContent = await fs.readFile(storagePath, 'utf-8');
  
  const memoryData = JSON.parse(memoryContent);
  const storageData = JSON.parse(storageContent);

  // Get the value from memory at sourcePath
  const valueToStore = getValueAtPath(memoryData, sourcePath);

  // Store the value in storage at destinationPath
  setValueAtPath(storageData, destinationPath, valueToStore);

  // Write back to storage file
  await fs.writeFile(storagePath, JSON.stringify(storageData, null, 2), 'utf-8');
}
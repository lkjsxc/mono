import { JsonPath } from '../types/common';

/**
 * Validates that a path starts with a forward slash
 * @param path - The path to validate
 * @throws Error if path doesn't start with forward slash
 */
export function validatePath(path: JsonPath): void {
  if (path[0] !== '/') {
    throw new Error(`Path must start with forward slash (/), got: ${path}`);
  }
}

/**
 * Gets a value at a specific path in an object
 * @param obj - The object to traverse
 * @param path - The path to look up
 * @returns The value at the path
 * @throws Error if any part of the path is invalid
 */
export function getValueAtPath(obj: any, path: JsonPath): any {
  // Handle root path case
  if (path === '/') {
    return obj;
  }
  
  const parts = path.split('/').slice(1); // Remove empty string from split('/')
  let current = obj;

  for (const part of parts) {
    // Skip empty parts (can happen with trailing slashes)
    if (part === '') {
      continue;
    }
    
    if (current === null || typeof current !== 'object') {
      throw new Error(`Cannot traverse path at '${part}': parent is not an object`);
    }
    if (!(part in current)) {
      throw new Error(`Path segment '${part}' not found`);
    }
    current = current[part];
  }

  return current;
}

/**
 * Sets a value at a specific path in an object
 * @param obj - The object to modify
 * @param path - The path where to set the value
 * @param value - The value to set
 * @throws Error if any part of the path is invalid or if parent is not an object
 */
export function setValueAtPath(obj: any, path: JsonPath, value: any): void {
  // Handle root path case - replace the entire object (not recommended but handled)
  if (path === '/') {
    throw new Error('Cannot set value at root path - this would replace the entire object');
  }
  
  const parts = path.split('/').slice(1); // Remove empty string from split('/')
  // Filter out empty parts
  const filteredParts = parts.filter(part => part !== '');
  
  if (filteredParts.length === 0) {
    throw new Error('Invalid path: no valid path segments found');
  }
  
  let current = obj;

  // Traverse/create path except last part
  for (let i = 0; i < filteredParts.length - 1; i++) {
    const part = filteredParts[i];
    if (current === null || typeof current !== 'object') {
      throw new Error(`Cannot traverse path at '${part}': parent is not an object`);
    }
    if (!(part in current)) {
      current[part] = {};
    }
    current = current[part];
  }

  // Set value at final path segment
  const lastPart = filteredParts[filteredParts.length - 1];
  if (current === null || typeof current !== 'object') {
    throw new Error(`Cannot set property '${lastPart}': parent is not an object`);
  }
  current[lastPart] = value;
}

/**
 * Updates part of an object structure while preserving other properties
 * @param obj - The object to update
 * @param path - The path where to apply the update
 * @param updates - The new values to merge at the path
 */
export function updateObjectAtPath(obj: any, path: JsonPath, updates: any): void {
  const existing = getValueAtPath(obj, path);
  if (existing === null || typeof existing !== 'object') {
    throw new Error(`Cannot update at path '${path}': target is not an object`);
  }
  
  setValueAtPath(obj, path, {
    ...existing,
    ...updates
  });
}
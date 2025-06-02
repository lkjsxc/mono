import { json_value, json_object } from '../types/common';

/**
 * Get value at a specific path in a JSON object
 */
export function get_value_at_path(obj: json_object, path: string): json_value | undefined {
  if (!path || path === '/') {
    return obj;
  }

  const normalized_path = normalize_path(path);
  const parts = normalized_path.split('/').filter(part => part !== '');
  
  let current: json_value = obj;
  
  for (const part of parts) {
    if (current === null || current === undefined) {
      return undefined;
    }
    
    if (typeof current === 'object' && !Array.isArray(current)) {
      current = current[part];
    } else if (Array.isArray(current)) {
      const index = parseInt(part, 10);
      if (isNaN(index) || index < 0 || index >= current.length) {
        return undefined;
      }
      current = current[index];
    } else {
      return undefined;
    }
  }
  
  return current;
}

/**
 * Set value at a specific path in a JSON object
 */
export function set_value_at_path(obj: json_object, path: string, value: json_value): boolean {
  if (!path || path === '/') {
    return false; // Cannot replace root
  }

  const normalized_path = normalize_path(path);
  const parts = normalized_path.split('/').filter(part => part !== '');
  
  if (parts.length === 0) {
    return false;
  }

  let current: json_value = obj;
  
  // Navigate to parent
  for (let i = 0; i < parts.length - 1; i++) {
    const part = parts[i];
    
    if (typeof current !== 'object' || current === null || Array.isArray(current)) {
      return false;
    }
    
    if (!(part in current)) {
      current[part] = {};
    }
    
    current = current[part];
  }
  
  // Set the final value
  const final_key = parts[parts.length - 1];
  if (typeof current === 'object' && current !== null && !Array.isArray(current)) {
    current[final_key] = value;
    return true;
  }
  
  return false;
}

/**
 * Delete value at a specific path in a JSON object
 */
export function delete_value_at_path(obj: json_object, path: string): boolean {
  if (!path || path === '/') {
    return false; // Cannot delete root
  }

  const normalized_path = normalize_path(path);
  const parts = normalized_path.split('/').filter(part => part !== '');
  
  if (parts.length === 0) {
    return false;
  }

  let current: json_value = obj;
  
  // Navigate to parent
  for (let i = 0; i < parts.length - 1; i++) {
    const part = parts[i];
    
    if (typeof current !== 'object' || current === null || Array.isArray(current)) {
      return false;
    }
    
    if (!(part in current)) {
      return false; // Path doesn't exist
    }
    
    current = current[part];
  }
  
  // Delete the final key
  const final_key = parts[parts.length - 1];
  if (typeof current === 'object' && current !== null && !Array.isArray(current)) {
    if (final_key in current) {
      delete current[final_key];
      return true;
    }
  }
  
  return false;
}

/**
 * List contents at a specific path in a JSON object
 */
export function list_contents_at_path(obj: json_object, path: string): json_object | undefined {
  const value = get_value_at_path(obj, path);
  
  if (typeof value !== 'object' || value === null || Array.isArray(value)) {
    return undefined;
  }
  
  const result: json_object = {};
  
  for (const [key, val] of Object.entries(value)) {
    result[key] = {
      _is_directory: typeof val === 'object' && val !== null && !Array.isArray(val)
    };
  }
  
  return result;
}

/**
 * Search for content within a JSON object
 */
export function search_content(obj: json_object, path: string, query: string): json_object {
  const scope = get_value_at_path(obj, path);
  const results: json_object = {};
  
  if (typeof scope === 'object' && scope !== null) {
    search_recursive(scope, query.toLowerCase(), '', results);
  }
  
  return results;
}

/**
 * Recursive search helper
 */
function search_recursive(obj: json_value, query: string, current_path: string, results: json_object): void {
  if (typeof obj === 'string' && obj.toLowerCase().includes(query)) {
    results[current_path || 'root'] = obj;
  } else if (typeof obj === 'object' && obj !== null && !Array.isArray(obj)) {
    for (const [key, value] of Object.entries(obj)) {
      const new_path = current_path ? `${current_path}/${key}` : key;
      search_recursive(value, query, new_path, results);
    }
  } else if (Array.isArray(obj)) {
    obj.forEach((item, index) => {
      const new_path = current_path ? `${current_path}/${index}` : `${index}`;
      search_recursive(item, query, new_path, results);
    });
  }
}

/**
 * Normalize a path by removing duplicate slashes and ensuring it starts with /
 */
export function normalize_path(path: string): string {
  if (!path.startsWith('/')) {
    path = '/' + path;
  }
  
  // Remove duplicate slashes
  path = path.replace(/\/+/g, '/');
  
  // Remove trailing slash unless it's the root
  if (path.length > 1 && path.endsWith('/')) {
    path = path.slice(0, -1);
  }
  
  return path;
}

/**
 * Validate if a path is valid
 */
export function is_valid_path(path: string): boolean {
  if (!path || typeof path !== 'string') {
    return false;
  }
  
  // Must start with /working_memory/ or /storage/
  return path.startsWith('/working_memory/') || path.startsWith('/storage/') || 
         path === '/working_memory' || path === '/storage';
}

/**
 * Get the root area from a path (working_memory or storage)
 */
export function get_path_root(path: string): 'working_memory' | 'storage' | null {
  if (path.startsWith('/working_memory')) {
    return 'working_memory';
  } else if (path.startsWith('/storage')) {
    return 'storage';
  }
  return null;
}

/**
 * Convert absolute path to relative path within the root area
 */
export function get_relative_path(path: string): string {
  if (path.startsWith('/working_memory/')) {
    return path.substring('/working_memory'.length);
  } else if (path.startsWith('/storage/')) {
    return path.substring('/storage'.length);
  } else if (path === '/working_memory' || path === '/storage') {
    return '/';
  }
  return path;
}

"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.get_value_at_path = get_value_at_path;
exports.set_value_at_path = set_value_at_path;
exports.delete_value_at_path = delete_value_at_path;
exports.list_contents_at_path = list_contents_at_path;
exports.search_content = search_content;
exports.normalize_path = normalize_path;
exports.is_valid_path = is_valid_path;
exports.get_path_root = get_path_root;
exports.get_relative_path = get_relative_path;
/**
 * Get value at a specific path in a JSON object
 */
function get_value_at_path(obj, path) {
    if (!path || path === '/') {
        return obj;
    }
    const normalized_path = normalize_path(path);
    const parts = normalized_path.split('/').filter(part => part !== '');
    let current = obj;
    for (const part of parts) {
        if (current === null || current === undefined) {
            return undefined;
        }
        if (typeof current === 'object' && !Array.isArray(current)) {
            current = current[part];
        }
        else if (Array.isArray(current)) {
            const index = parseInt(part, 10);
            if (isNaN(index) || index < 0 || index >= current.length) {
                return undefined;
            }
            current = current[index];
        }
        else {
            return undefined;
        }
    }
    return current;
}
/**
 * Set value at a specific path in a JSON object
 */
function set_value_at_path(obj, path, value) {
    if (!path || path === '/') {
        return false; // Cannot replace root
    }
    const normalized_path = normalize_path(path);
    const parts = normalized_path.split('/').filter(part => part !== '');
    if (parts.length === 0) {
        return false;
    }
    let current = obj;
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
function delete_value_at_path(obj, path) {
    if (!path || path === '/') {
        return false; // Cannot delete root
    }
    const normalized_path = normalize_path(path);
    const parts = normalized_path.split('/').filter(part => part !== '');
    if (parts.length === 0) {
        return false;
    }
    let current = obj;
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
function list_contents_at_path(obj, path) {
    const value = get_value_at_path(obj, path);
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
        return undefined;
    }
    const result = {};
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
function search_content(obj, path, query) {
    const scope = get_value_at_path(obj, path);
    const results = {};
    if (typeof scope === 'object' && scope !== null) {
        search_recursive(scope, query.toLowerCase(), '', results);
    }
    return results;
}
/**
 * Recursive search helper
 */
function search_recursive(obj, query, current_path, results) {
    if (typeof obj === 'string' && obj.toLowerCase().includes(query)) {
        results[current_path || 'root'] = obj;
    }
    else if (typeof obj === 'object' && obj !== null && !Array.isArray(obj)) {
        for (const [key, value] of Object.entries(obj)) {
            const new_path = current_path ? `${current_path}/${key}` : key;
            search_recursive(value, query, new_path, results);
        }
    }
    else if (Array.isArray(obj)) {
        obj.forEach((item, index) => {
            const new_path = current_path ? `${current_path}/${index}` : `${index}`;
            search_recursive(item, query, new_path, results);
        });
    }
}
/**
 * Normalize a path by removing duplicate slashes and ensuring it starts with /
 */
function normalize_path(path) {
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
function is_valid_path(path) {
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
function get_path_root(path) {
    if (path.startsWith('/working_memory')) {
        return 'working_memory';
    }
    else if (path.startsWith('/storage')) {
        return 'storage';
    }
    return null;
}
/**
 * Convert absolute path to relative path within the root area
 */
function get_relative_path(path) {
    if (path.startsWith('/working_memory/')) {
        return path.substring('/working_memory'.length);
    }
    else if (path.startsWith('/storage/')) {
        return path.substring('/storage'.length);
    }
    else if (path === '/working_memory' || path === '/storage') {
        return '/';
    }
    return path;
}

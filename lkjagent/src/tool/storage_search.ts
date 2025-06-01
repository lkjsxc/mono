import * as fs from 'fs/promises';
import * as path from 'path';
import { JsonPath, SearchResult } from '../types/common';

/**
 * Search recursively through an object for matching content
 * @param obj - Object to search through
 * @param content - Content to search for
 * @param currentPath - Current path in the object (for building result paths)
 * @returns Array of paths and their matching values
 */
function searchRecursive(obj: any, content: string, currentPath: string): SearchResult[] {
  const results: SearchResult[] = [];
  
  if (typeof obj === 'string' && obj.toLowerCase().includes(content.toLowerCase())) {
    results.push({
      path: currentPath,
      content: obj,
      relevance: calculateRelevance(obj, content)
    });
  } else if (typeof obj === 'object' && obj !== null) {
    for (const [key, value] of Object.entries(obj)) {
      const newPath = currentPath ? `${currentPath}.${key}` : key;
      results.push(...searchRecursive(value, content, newPath));
    }
  }
  
  return results;
}

/**
 * Calculate relevance score for a match
 * Higher score means more relevant (exact match > contains > partial word match)
 */
function calculateRelevance(text: string, searchTerm: string): number {
  const normalizedText = text.toLowerCase();
  const normalizedSearch = searchTerm.toLowerCase();
  
  if (normalizedText === normalizedSearch) return 1.0;
  if (normalizedText.includes(normalizedSearch)) return 0.8;
  
  const words = normalizedText.split(/\s+/);
  if (words.some(word => word === normalizedSearch)) return 0.6;
  if (words.some(word => word.includes(normalizedSearch))) return 0.4;
  
  return 0.2;
}

/**
 * Searches data in Storage by keyword
 * @param content - Search term to look for
 * @returns Array of matching paths and their values, sorted by relevance
 */
export async function storage_search(content: string): Promise<SearchResult[]> {
  const storagePath = path.join(__dirname, '..', '..', 'data', 'storage.json');
  
  try {
    // Read current storage state
    const storageData = JSON.parse(await fs.readFile(storagePath, 'utf-8'));
    
    // Perform recursive search through the storage data
    const results = searchRecursive(storageData, content, '');
    
    // Sort results by relevance (highest first)
    return results.sort((a, b) => (b.relevance || 0) - (a.relevance || 0));
  } catch (error) {
    throw new Error(`Failed to search Storage for content '${content}': ${error}`);
  }
}
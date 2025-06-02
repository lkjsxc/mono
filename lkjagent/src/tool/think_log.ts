import * as fs from 'fs/promises';
import * as path from 'path';

/**
 * Structure of a thinking log entry
 */
export interface ThinkLogEntry {
  timestamp: number;
  content: string;
}

/**
 * Adds a thinking entry to the dedicated thinking log
 * @param content - The thinking content to log
 */
export async function think_log(content: string): Promise<void> {
  const thinkLogPath = path.join(__dirname, '..', '..', 'data', 'think_log.json');
  
  try {
    // Read current thinking log or create new one
    let thinkLogData: ThinkLogEntry[] = [];
    try {
      const logContent = await fs.readFile(thinkLogPath, 'utf-8');
      thinkLogData = JSON.parse(logContent);
    } catch (error) {
      // File doesn't exist or is invalid, start with empty array
    }
    
    // Create new thinking entry
    const newEntry: ThinkLogEntry = {
      timestamp: Date.now(),
      content: content
    };
    
    // Append new entry
    thinkLogData.push(newEntry);
    
    // Keep only last 500 entries to prevent file from growing too large
    if (thinkLogData.length > 500) {
      thinkLogData = thinkLogData.slice(-500);
    }
    
    // Write updated thinking log back to file
    await fs.writeFile(thinkLogPath, JSON.stringify(thinkLogData, null, 2), 'utf-8');
  } catch (error) {
    console.error('Failed to write to thinking log:', error);
  }
}

/**
 * Retrieves thinking log entries by index
 * @param index - Index of the entry to retrieve (negative for counting from end, e.g., -1 for last entry)
 * @returns The thinking log entry at the specified index, or null if not found
 */
export async function think_log_get(index: number): Promise<ThinkLogEntry | null> {
  const thinkLogPath = path.join(__dirname, '..', '..', 'data', 'think_log.json');
  
  try {
    // Read current thinking log
    const logContent = await fs.readFile(thinkLogPath, 'utf-8');
    const thinkLogData: ThinkLogEntry[] = JSON.parse(logContent);
    
    // Handle negative indices (count from end)
    let actualIndex = index;
    if (index < 0) {
      actualIndex = thinkLogData.length + index;
    }
    
    // Check if index is valid
    if (actualIndex < 0 || actualIndex >= thinkLogData.length) {
      return null;
    }
    
    return thinkLogData[actualIndex];
  } catch (error) {
    console.error('Failed to read thinking log:', error);
    return null;
  }
}

/**
 * Retrieves multiple thinking log entries by range
 * @param start - Start index (inclusive)
 * @param end - End index (exclusive), if not provided returns from start to end of log
 * @returns Array of thinking log entries in the specified range
 */
export async function think_log_get_range(start: number, end?: number): Promise<ThinkLogEntry[]> {
  const thinkLogPath = path.join(__dirname, '..', '..', 'data', 'think_log.json');
  
  try {
    // Read current thinking log
    const logContent = await fs.readFile(thinkLogPath, 'utf-8');
    const thinkLogData: ThinkLogEntry[] = JSON.parse(logContent);
    
    // Handle negative indices
    let actualStart = start < 0 ? thinkLogData.length + start : start;
    let actualEnd = end !== undefined ? (end < 0 ? thinkLogData.length + end : end) : thinkLogData.length;
    
    // Ensure indices are within bounds
    actualStart = Math.max(0, Math.min(actualStart, thinkLogData.length));
    actualEnd = Math.max(actualStart, Math.min(actualEnd, thinkLogData.length));
    
    return thinkLogData.slice(actualStart, actualEnd);
  } catch (error) {
    console.error('Failed to read thinking log:', error);
    return [];
  }
}

/**
 * Gets the total count of thinking log entries
 * @returns The number of entries in the thinking log
 */
export async function think_log_count(): Promise<number> {
  const thinkLogPath = path.join(__dirname, '..', '..', 'data', 'think_log.json');
  
  try {
    // Read current thinking log
    const logContent = await fs.readFile(thinkLogPath, 'utf-8');
    const thinkLogData: ThinkLogEntry[] = JSON.parse(logContent);
    
    return thinkLogData.length;
  } catch (error) {
    console.error('Failed to read thinking log:', error);
    return 0;
  }
}

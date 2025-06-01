import * as fs from 'fs/promises';
import * as path from 'path';
import { LogEntry } from '../types/common';

/**
 * Logs an action to data/log.json
 * @param entry The log entry to append
 */
export async function logAction(entry: LogEntry): Promise<void> {
  const logPath = path.join(__dirname, '..', '..', 'data', 'log.json');
  
  try {
    // Read current log or create new one
    let logData: LogEntry[] = [];
    try {
      const content = await fs.readFile(logPath, 'utf-8');
      logData = JSON.parse(content);
    } catch (error) {
      // File doesn't exist or is invalid, start with empty array
    }
    
    // Append new entry
    logData.push(entry);
    
    // Keep only last 1000 entries to prevent file from growing too large
    if (logData.length > 1000) {
      logData = logData.slice(-1000);
    }
    
    // Write updated log back to file
    await fs.writeFile(logPath, JSON.stringify(logData, null, 2), 'utf-8');
  } catch (error) {
    console.error('Failed to log action:', error);
  }
}

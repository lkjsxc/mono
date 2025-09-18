/**
 * File system utilities for zenbukko
 * Provides functional programming style file operations with proper error handling
 */

import * as fs from 'fs';
import * as path from 'path';
import { promisify } from 'util';
import { pipeline } from 'stream';
import { createWriteStream } from 'fs';
import type { AppError } from '../types';

// Promisified fs functions
const fsReadFile = promisify(fs.readFile);
const fsWriteFile = promisify(fs.writeFile);
const fsMkdir = promisify(fs.mkdir);
const fsStat = promisify(fs.stat);
const fsAccess = promisify(fs.access);
const fsReaddir = promisify(fs.readdir);
const streamPipeline = promisify(pipeline);

/**
 * Check if a path exists
 * @param filePath - Path to check
 * @returns Promise<boolean>
 */
export const exists = async (filePath: string): Promise<boolean> => {
  try {
    await fsAccess(filePath);
    return true;
  } catch {
    return false;
  }
};

/**
 * Ensure a directory exists, creating it recursively if needed
 * @param dirPath - Directory path to ensure
 * @returns Promise<void>
 */
export const ensureDir = async (dirPath: string): Promise<void> => {
  try {
    await fsMkdir(dirPath, { recursive: true });
  } catch (error) {
    throw new Error(`Failed to create directory ${dirPath}: ${error instanceof Error ? error.message : 'Unknown error'}`);
  }
};

/**
 * Ensure the parent directory of a file path exists
 * @param filePath - File path whose parent directory should be ensured
 * @returns Promise<void>
 */
export const ensureParentDir = async (filePath: string): Promise<void> => {
  const parentDir = path.dirname(filePath);
  await ensureDir(parentDir);
};

/**
 * Read file as text with error handling
 * @param filePath - Path to file
 * @param encoding - Text encoding (default: utf-8)
 * @returns Promise<string>
 */
export const readTextFile = async (filePath: string, encoding: BufferEncoding = 'utf-8'): Promise<string> => {
  try {
    return await fsReadFile(filePath, encoding);
  } catch (error) {
    throw new Error(`Failed to read file ${filePath}: ${error instanceof Error ? error.message : 'Unknown error'}`);
  }
};

/**
 * Write text to file with automatic directory creation
 * @param filePath - Path to write to
 * @param content - Text content to write
 * @param encoding - Text encoding (default: utf-8)
 * @returns Promise<void>
 */
export const writeTextFile = async (filePath: string, content: string, encoding: BufferEncoding = 'utf-8'): Promise<void> => {
  try {
    await ensureParentDir(filePath);
    await fsWriteFile(filePath, content, encoding);
  } catch (error) {
    throw new Error(`Failed to write file ${filePath}: ${error instanceof Error ? error.message : 'Unknown error'}`);
  }
};

/**
 * Read JSON file with parsing and validation
 * @param filePath - Path to JSON file
 * @returns Promise<any>
 */
export const readJsonFile = async <T = any>(filePath: string): Promise<T> => {
  try {
    const content = await readTextFile(filePath);
    return JSON.parse(content) as T;
  } catch (error) {
    if (error instanceof SyntaxError) {
      throw new Error(`Invalid JSON in file ${filePath}: ${error.message}`);
    }
    throw error;
  }
};

/**
 * Write JSON to file with formatting
 * @param filePath - Path to write to
 * @param data - Data to serialize as JSON
 * @param space - JSON formatting spaces (default: 2)
 * @returns Promise<void>
 */
export const writeJsonFile = async (filePath: string, data: any, space: number = 2): Promise<void> => {
  try {
    const content = JSON.stringify(data, null, space);
    await writeTextFile(filePath, content);
  } catch (error) {
    throw new Error(`Failed to write JSON file ${filePath}: ${error instanceof Error ? error.message : 'Unknown error'}`);
  }
};

/**
 * Get file stats safely
 * @param filePath - Path to file
 * @returns Promise<fs.Stats | null>
 */
export const getFileStats = async (filePath: string): Promise<fs.Stats | null> => {
  try {
    return await fsStat(filePath);
  } catch {
    return null;
  }
};

/**
 * Get file size in bytes
 * @param filePath - Path to file
 * @returns Promise<number | null>
 */
export const getFileSize = async (filePath: string): Promise<number | null> => {
  const stats = await getFileStats(filePath);
  return stats?.size ?? null;
};

/**
 * Check if path is a directory
 * @param dirPath - Path to check
 * @returns Promise<boolean>
 */
export const isDirectory = async (dirPath: string): Promise<boolean> => {
  const stats = await getFileStats(dirPath);
  return stats?.isDirectory() ?? false;
};

/**
 * Check if path is a file
 * @param filePath - Path to check
 * @returns Promise<boolean>
 */
export const isFile = async (filePath: string): Promise<boolean> => {
  const stats = await getFileStats(filePath);
  return stats?.isFile() ?? false;
};

/**
 * List directory contents with optional filtering
 * @param dirPath - Directory to list
 * @param filter - Optional filter function
 * @returns Promise<string[]>
 */
export const listDirectory = async (dirPath: string, filter?: (name: string) => boolean): Promise<string[]> => {
  try {
    const entries = await fsReaddir(dirPath);
    return filter ? entries.filter(filter) : entries;
  } catch (error) {
    throw new Error(`Failed to list directory ${dirPath}: ${error instanceof Error ? error.message : 'Unknown error'}`);
  }
};

/**
 * Generate safe filename by removing/replacing invalid characters
 * @param filename - Original filename
 * @param replacement - Character to replace invalid chars with (default: _)
 * @returns Safe filename
 */
export const sanitizeFilename = (filename: string, replacement: string = '_'): string => {
  // Replace invalid characters for most filesystems
  return filename
    .replace(/[<>:"/\\|?*]/g, replacement)
    .replace(/[\x00-\x1f\x80-\x9f]/g, replacement)
    .replace(/^\.+$/, replacement)
    .replace(/\s+/g, replacement)
    .trim();
};

/**
 * Generate unique filename by appending numbers if file exists
 * @param basePath - Base file path
 * @param extension - File extension (optional)
 * @returns Promise<string> - Unique file path
 */
export const generateUniqueFilename = async (basePath: string, extension?: string): Promise<string> => {
  const ext = extension || path.extname(basePath);
  const nameWithoutExt = extension ? basePath : path.parse(basePath).name;
  const dir = path.dirname(basePath);
  
  let counter = 0;
  let candidatePath = basePath;
  
  while (await exists(candidatePath)) {
    counter++;
    const newName = `${nameWithoutExt}_${counter}${ext}`;
    candidatePath = path.join(dir, newName);
  }
  
  return candidatePath;
};

/**
 * Download file from URL to local path with progress tracking
 * @param url - URL to download from
 * @param filePath - Local path to save to
 * @param options - Download options
 * @returns Promise<{filePath: string, size: number}>
 */
export interface DownloadOptions {
  onProgress?: (downloaded: number, total?: number) => void;
  timeout?: number;
  overwrite?: boolean;
}

export const downloadFile = async (
  url: string, 
  filePath: string, 
  options: DownloadOptions = {}
): Promise<{filePath: string, size: number}> => {
  const axios = (await import('axios')).default;
  
  try {
    // Check if file exists and handle overwrite
    if (!options.overwrite && await exists(filePath)) {
      throw new Error(`File already exists: ${filePath}`);
    }
    
    await ensureParentDir(filePath);
    
    const response = await axios({
      method: 'GET',
      url: url,
      responseType: 'stream',
      timeout: options.timeout || 30000,
    });
    
    const totalSize = parseInt(response.headers['content-length'] || '0');
    let downloadedSize = 0;
    
    const writer = createWriteStream(filePath);
    
    // Track download progress
    if (options.onProgress) {
      response.data.on('data', (chunk: Buffer) => {
        downloadedSize += chunk.length;
        options.onProgress!(downloadedSize, totalSize);
      });
    }
    
    await streamPipeline(response.data, writer);
    
    const finalSize = await getFileSize(filePath);
    return {
      filePath,
      size: finalSize || downloadedSize,
    };
    
  } catch (error) {
    // Clean up partial file on error
    if (await exists(filePath)) {
      try {
        fs.unlinkSync(filePath);
      } catch {
        // Ignore cleanup errors
      }
    }
    
    throw new Error(`Failed to download ${url}: ${error instanceof Error ? error.message : 'Unknown error'}`);
  }
};

/**
 * Copy file from source to destination
 * @param sourcePath - Source file path
 * @param destPath - Destination file path
 * @param overwrite - Whether to overwrite if destination exists
 * @returns Promise<void>
 */
export const copyFile = async (sourcePath: string, destPath: string, overwrite: boolean = false): Promise<void> => {
  try {
    if (!overwrite && await exists(destPath)) {
      throw new Error(`Destination file already exists: ${destPath}`);
    }
    
    await ensureParentDir(destPath);
    await streamPipeline(
      fs.createReadStream(sourcePath),
      createWriteStream(destPath)
    );
  } catch (error) {
    throw new Error(`Failed to copy ${sourcePath} to ${destPath}: ${error instanceof Error ? error.message : 'Unknown error'}`);
  }
};

/**
 * Get file extension (including the dot)
 * @param filePath - File path
 * @returns File extension with dot, or empty string if none
 */
export const getFileExtension = (filePath: string): string => {
  return path.extname(filePath);
};

/**
 * Get filename without extension
 * @param filePath - File path
 * @returns Filename without extension
 */
export const getFilenameWithoutExt = (filePath: string): string => {
  return path.parse(filePath).name;
};

/**
 * Format file size in human readable format
 * @param bytes - Size in bytes
 * @returns Formatted size string
 */
export const formatFileSize = (bytes: number): string => {
  const units = ['B', 'KB', 'MB', 'GB', 'TB'];
  let size = bytes;
  let unitIndex = 0;
  
  while (size >= 1024 && unitIndex < units.length - 1) {
    size /= 1024;
    unitIndex++;
  }
  
  return `${size.toFixed(unitIndex === 0 ? 0 : 1)} ${units[unitIndex]}`;
};

/**
 * Create file operation result type for functional programming
 */
export type FileOperationResult<T> = {
  success: boolean;
  data?: T;
  error?: AppError;
};

/**
 * Wrap file operations in result type for better error handling
 * @param operation - Async operation to wrap
 * @returns Promise<FileOperationResult<T>>
 */
export const safeFileOperation = async <T>(
  operation: () => Promise<T>
): Promise<FileOperationResult<T>> => {
  try {
    const data = await operation();
    return { success: true, data };
  } catch (error) {
    return {
      success: false,
      error: {
        type: 'FileSystemError',
        message: error instanceof Error ? error.message : 'Unknown file system error',
      },
    };
  }
};

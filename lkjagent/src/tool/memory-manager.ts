import * as fs from 'fs/promises';
import * as path from 'path';
import { ConfigManager } from '../config/config-manager';
import { JsonPath, RamData, StorageData } from '../types/common';
import { storage_store } from './storage_store';
import { ram_remove } from './ram_remove';

export class MemoryManager {
  private static instance: MemoryManager;
  private memoryPath: string;
  private storagePath: string;
  private configManager: ConfigManager;

  private constructor(configManager: ConfigManager) {
    this.configManager = configManager;
    this.memoryPath = path.join(__dirname, '..', '..', 'data', 'memory.json');
    this.storagePath = path.join(__dirname, '..', '..', 'data', 'storage.json');
  }

  public static async getInstance(): Promise<MemoryManager> {
    if (!MemoryManager.instance) {
      const configManager = await ConfigManager.getInstance();
      MemoryManager.instance = new MemoryManager(configManager);
    }
    return MemoryManager.instance;
  }

  private calculateSize(obj: any): number {
    return JSON.stringify(obj).length;
  }

  private isCriticalPath(path: string): boolean {
    const criticalPaths = this.configManager.getMemoryConfig().criticalPaths;
    return criticalPaths.some(criticalPath => path.startsWith(criticalPath));
  }

  private async getMemoryUsage(): Promise<number> {
    const memoryData = JSON.parse(await fs.readFile(this.memoryPath, 'utf-8'));
    return this.calculateSize(memoryData);
  }

  private async findLargestNonCriticalPath(): Promise<{ path: string; size: number } | null> {
    const memoryData = JSON.parse(await fs.readFile(this.memoryPath, 'utf-8'));
    let largestPath = null;
    let largestSize = 0;

    const traverse = (obj: any, currentPath: string) => {
      for (const [key, value] of Object.entries(obj)) {
        const path = currentPath ? `${currentPath}.${key}` : key;
        
        if (!this.isCriticalPath(path) && typeof value === 'object' && value !== null) {
          const size = this.calculateSize(value);
          if (size > largestSize) {
            largestPath = path;
            largestSize = size;
          }
          traverse(value, path);
        }
      }
    };

    traverse(memoryData, '');
    return largestPath ? { path: largestPath, size: largestSize } : null;
  }

  public async checkAndManageMemory(): Promise<void> {
    const currentSize = await this.getMemoryUsage();
    const limit = this.configManager.getMemoryConfig().ramCharacterLimit;

    if (currentSize > limit) {
      // Find largest non-critical data to move to storage
      const largest = await this.findLargestNonCriticalPath();
      
      if (largest) {
        // Move to storage with timestamp
        const timestamp = new Date().toISOString();
        const storagePath = `storage.archived_data.${timestamp.replace(/[:.]/g, '_')}`;
        
        await storage_store(largest.path as JsonPath, storagePath as JsonPath);
        await ram_remove(largest.path as JsonPath);
        
        // Recursively check if we need to move more data
        await this.checkAndManageMemory();
      }
    }
  }

  public async backupMemory(): Promise<void> {
    try {
      const timestamp = new Date().toISOString().replace(/[:.]/g, '_');
      const backupDir = path.join(__dirname, '..', '..', 'data', 'backups');
      
      // Ensure backup directory exists
      await fs.mkdir(backupDir, { recursive: true });
      
      // Copy current memory file to backup
      const backupPath = path.join(backupDir, `memory_${timestamp}.json`);
      await fs.copyFile(this.memoryPath, backupPath);
      
    } catch (error) {
      console.error('Failed to create memory backup:', error);
    }
  }
}

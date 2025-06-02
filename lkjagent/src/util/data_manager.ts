import * as fs from 'fs-extra';
import * as path from 'path';
import { working_memory_structure, storage_structure, json_object } from '../types/common';

const DATA_DIR = path.join(process.cwd(), 'data');
const MEMORY_FILE = path.join(DATA_DIR, 'memory.json');
const STORAGE_FILE = path.join(DATA_DIR, 'storage.json');

/**
 * Load working memory from memory.json
 */
export async function load_working_memory(): Promise<json_object> {
  try {
    if (await fs.pathExists(MEMORY_FILE)) {
      const data: working_memory_structure = await fs.readJson(MEMORY_FILE);
      return data.working_memory as json_object;
    } else {
      // Return default structure
      return {
        user_data: { todo: {} },
        action_result: {},
        system_info: {}
      };
    }
  } catch (error) {
    console.error('Error loading working memory:', error);
    return {
      user_data: { todo: {} },
      action_result: {},
      system_info: {}
    };
  }
}

/**
 * Save working memory to memory.json
 */
export async function save_working_memory(working_memory: json_object): Promise<void> {
  try {
    await fs.ensureDir(DATA_DIR);
    const data: working_memory_structure = { 
      working_memory: working_memory as any
    };
    await fs.writeJson(MEMORY_FILE, data, { spaces: 2 });
  } catch (error) {
    console.error('Error saving working memory:', error);
    throw error;
  }
}

/**
 * Load storage from storage.json
 */
export async function load_storage(): Promise<json_object> {
  try {
    if (await fs.pathExists(STORAGE_FILE)) {
      const data: storage_structure = await fs.readJson(STORAGE_FILE);
      return data.storage;
    } else {
      // Return default structure
      return {
        knowledge_base: {
          system_policy_summary: '',
          greeting_message: ''
        },
        archived_data: {}
      };
    }
  } catch (error) {
    console.error('Error loading storage:', error);
    return {
      knowledge_base: {
        system_policy_summary: '',
        greeting_message: ''
      },
      archived_data: {}
    };
  }
}

/**
 * Save storage to storage.json
 */
export async function save_storage(storage: json_object): Promise<void> {
  try {
    await fs.ensureDir(DATA_DIR);
    const data: storage_structure = { storage };
    await fs.writeJson(STORAGE_FILE, data, { spaces: 2 });
  } catch (error) {
    console.error('Error saving storage:', error);
    throw error;
  }
}

/**
 * Get the character count of working memory when serialized
 */
export function get_working_memory_size(working_memory: json_object): number {
  try {
    return JSON.stringify(working_memory).length;
  } catch (error) {
    console.error('Error calculating working memory size:', error);
    return 0;
  }
}

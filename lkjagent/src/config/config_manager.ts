import * as fs from 'fs-extra';
import * as path from 'path';
import { app_config } from '../types/common';

const DATA_DIR = path.join(process.cwd(), 'data');
const CONFIG_FILE = path.join(DATA_DIR, 'config.json');

// Default configuration
const DEFAULT_CONFIG: app_config = {
  working_memory_character_max: 2048,
  working_memory_direct_child_max: 50,
  llm_api_url: 'http://localhost:1234/v1/chat/completions',
  llm_model: 'local-model',
  llm_max_tokens: 1000,
  llm_temperature: 0.7,
  system_max_log_entries: 1000,
  system_auto_cleanup: true,
  system_debug_mode: false
};

/**
 * Load configuration from data/config.json
 */
export async function load_config(): Promise<app_config> {
  try {
    if (await fs.pathExists(CONFIG_FILE)) {
      const config_data = await fs.readJson(CONFIG_FILE);
      return { ...DEFAULT_CONFIG, ...config_data };
    } else {
      // Create default config if it doesn't exist
      await ensure_data_directory();
      await fs.writeJson(CONFIG_FILE, DEFAULT_CONFIG, { spaces: 2 });
      return DEFAULT_CONFIG;
    }
  } catch (error) {
    console.error('Error loading config:', error);
    return DEFAULT_CONFIG;
  }
}

/**
 * Save configuration to data/config.json
 */
export async function save_config(config: app_config): Promise<void> {
  try {
    await ensure_data_directory();
    await fs.writeJson(CONFIG_FILE, config, { spaces: 2 });
  } catch (error) {
    console.error('Error saving config:', error);
    throw error;
  }
}

/**
 * Ensure data directory exists
 */
export async function ensure_data_directory(): Promise<void> {
  await fs.ensureDir(DATA_DIR);
}

/**
 * Initialize default data files
 */
export async function init_data_files(): Promise<void> {
  await ensure_data_directory();
  
  const config_file = path.join(DATA_DIR, 'config.json');
  const memory_file = path.join(DATA_DIR, 'memory.json');
  const storage_file = path.join(DATA_DIR, 'storage.json');
  const log_file = path.join(DATA_DIR, 'log.json');

  // Create config.json
  if (!await fs.pathExists(config_file)) {
    await fs.writeJson(config_file, DEFAULT_CONFIG, { spaces: 2 });
  }

  // Create memory.json
  if (!await fs.pathExists(memory_file)) {
    const default_memory = {
      working_memory: {
        user_data: { todo: {} },
        action_result: {},
        system_info: {}
      }
    };
    await fs.writeJson(memory_file, default_memory, { spaces: 2 });
  }

  // Create storage.json
  if (!await fs.pathExists(storage_file)) {
    const default_storage = {
      storage: {
        knowledge_base: {
          system_policy_summary: '',
          greeting_message: ''
        },
        archived_data: {}
      }
    };
    await fs.writeJson(storage_file, default_storage, { spaces: 2 });
  }

  // Create log.json
  if (!await fs.pathExists(log_file)) {
    await fs.writeJson(log_file, [], { spaces: 2 });
  }

  console.log('Data files initialized successfully');
}

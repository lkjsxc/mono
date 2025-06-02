#!/usr/bin/env node

/**
 * Initialize data files for lkjagent
 */

const fs = require('fs-extra');
const path = require('path');

const DATA_DIR = path.join(process.cwd(), 'data');
const MEMORY_FILE = path.join(DATA_DIR, 'memory.json');
const STORAGE_FILE = path.join(DATA_DIR, 'storage.json');

async function initData() {
  try {
    console.log('🔧 Initializing lkjagent data files...');

    // Ensure data directory exists
    await fs.ensureDir(DATA_DIR);

    // Initialize memory.json
    const memoryData = {
      working_memory: {
        user: {
          goal: "Build the biggest wiki",
          current_task: "Understanding the directory structure",
          memory: {
            dir:{
              storage: "empty"
            }
          }
        },
        action_result: {},
        system_info: {}
      }
    };

    await fs.writeJson(MEMORY_FILE, memoryData, { spaces: 2 });
    console.log('✅ Created memory.json');

    // Initialize storage.json
    const storageData = {
      storage: {
      }
    };

    await fs.writeJson(STORAGE_FILE, storageData, { spaces: 2 });
    console.log('✅ Created storage.json');

    console.log('🎉 Data initialization complete!');
    console.log(`📁 Data directory: ${DATA_DIR}`);

  } catch (error) {
    console.error('❌ Failed to initialize data files:', error);
    process.exit(1);
  }
}

initData();

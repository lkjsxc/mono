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
        user_data: {
          todo: {}
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
        knowledge_base: {
          system_policy_summary: 'lkjagent is a sophisticated AI agent framework designed for small language models.',
          greeting_message: 'Hello! I am lkjagent, your AI assistant. I can help you manage data and perform various tasks using my memory system.'
        },
        archived_data: {}
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

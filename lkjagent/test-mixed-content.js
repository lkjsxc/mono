#!/usr/bin/env node
/**
 * Test script to verify enhanced ls logging with mixed files and directories
 */

const path = require('path');
const fs = require('fs-extra');

// Add src to the path
process.chdir(__dirname);

// Build the TypeScript files first
const { execSync } = require('child_process');
try {
  execSync('npx tsc', { stdio: 'inherit' });
} catch (error) {
  console.log('Note: TypeScript compilation may have warnings, continuing...');
}

const { execute_actions, clear_action_results } = require('./dist/util/executor');
const { get_recent_log_entries } = require('./dist/tool/action_logger');

async function test_mixed_files_directories() {
  console.log('🧪 Testing enhanced ls logging with mixed files and directories...');
  
  try {
    // Clear previous results
    await clear_action_results();
    
    // Create test actions with actual mixed files and directories
    const actions = [
      {
        kind: 'set',
        target_path: '/working_memory/mixed_content/simple_file.txt',
        content: 'This is just a string (file)'
      },
      {
        kind: 'set',
        target_path: '/working_memory/mixed_content/number_file',
        content: 42
      },
      {
        kind: 'set',
        target_path: '/working_memory/mixed_content/directory_with_metadata',
        content: { 
          type: 'directory',
          created: '2025-06-02',
          contents: ['file1', 'file2']
        }
      },
      {
        kind: 'set',
        target_path: '/working_memory/mixed_content/boolean_file',
        content: true
      },
      {
        kind: 'set',
        target_path: '/working_memory/mixed_content/config_directory/settings',
        content: { theme: 'dark', language: 'en' }
      },
      {
        kind: 'ls',
        target_path: '/working_memory/mixed_content'
      }
    ];
    
    // Execute actions
    await execute_actions(actions);
    
    // Get recent log entries
    const recent_logs = await get_recent_log_entries(5);
    
    // Find the ls log entry
    const ls_log = recent_logs.find(log => log.kind === 'ls');
    
    if (ls_log) {
      console.log('✅ Found ls log entry:');
      console.log(`   Target: ${ls_log.target_path}`);
      console.log(`   Status: ${ls_log.status}`);
      console.log(`   Message: ${ls_log.message}`);
      console.log(`   Result Summary: ${ls_log.result_summary}`);
      
      // Check if the enhanced summary shows directory/file breakdown
      if (ls_log.result_summary.includes('dirs') && ls_log.result_summary.includes('files')) {
        console.log('✅ Enhanced mixed content detection is working!');
        console.log('   - This should show some dirs and some files');
      } else {
        console.log('⚠️  Unexpected format:', ls_log.result_summary);
      }
    } else {
      console.log('❌ No ls log entry found');
    }
    
    console.log('\n🎉 Mixed content test completed!');
    
  } catch (error) {
    console.error('❌ Test failed:', error);
  }
}

// Run the test
test_mixed_files_directories();

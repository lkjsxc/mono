#!/usr/bin/env node
/**
 * Test script to verify enhanced ls logging with directory detection
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

async function test_enhanced_ls_logging() {
  console.log('🧪 Testing enhanced ls logging with directory detection...');
  
  try {
    // Clear previous results
    await clear_action_results();
    
    // Create test actions with mixed files and directories
    const actions = [
      {
        kind: 'set',
        target_path: '/working_memory/test_directory/file1.txt',
        content: { content: 'This is a file' }
      },
      {
        kind: 'set',
        target_path: '/working_memory/test_directory/subdirectory/info',
        content: { content: 'This is in a subdirectory' }
      },
      {
        kind: 'set',
        target_path: '/working_memory/test_directory/file2.json',
        content: { data: 'Another file' }
      },
      {
        kind: 'ls',
        target_path: '/working_memory/test_directory'
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
      if (ls_log.result_summary.includes('dirs') || ls_log.result_summary.includes('files')) {
        console.log('✅ Enhanced directory detection is working!');
      } else {
        console.log('⚠️  Basic counting only (no directories detected)');
      }
    } else {
      console.log('❌ No ls log entry found');
    }
    
    console.log('\n🎉 Test completed!');
    
  } catch (error) {
    console.error('❌ Test failed:', error);
  }
}

// Run the test
test_enhanced_ls_logging();

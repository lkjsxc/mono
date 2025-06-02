#!/usr/bin/env node

const { readFileSync, writeFileSync } = require('fs');
const { handle_ls_action } = require('./dist/tool/ls_tool.js');

async function testEnhancedLsOutput() {
  console.log('Testing enhanced ls output formatting...');

  // Create test memory with mixed content
  const testMemory = {
    working_memory: {
      test_directory: {
        folder1: {
          subfolder: {
            file1: "content1"
          }
        },
        folder2: {
          file2: "content2",
          file3: "content3"
        },
        file4: "content4",
        file5: "content5"
      }
    }
  };

  const testStorage = {
    storage: {}
  };

  // Test mixed content directory
  console.log('\n--- Test 1: Mixed content directory ---');
  const action1 = {
    kind: 'ls',
    target_path: '/working_memory/test_directory'
  };

  const result1 = await handle_ls_action(action1, testMemory.working_memory, testStorage.storage, 1);
  console.log('Status:', result1.status);
  console.log('Message:');
  console.log(result1.message);
  console.log('Data:', JSON.stringify(result1.data, null, 2));

  // Test directory with only folders
  console.log('\n--- Test 2: Directory with only folders ---');
  const action2 = {
    kind: 'ls',
    target_path: '/working_memory/test_directory/folder1'
  };

  const result2 = await handle_ls_action(action2, testMemory.working_memory, testStorage.storage, 2);
  console.log('Status:', result2.status);
  console.log('Message:');
  console.log(result2.message);
  console.log('Data:', JSON.stringify(result2.data, null, 2));

  // Test directory with only files
  console.log('\n--- Test 3: Directory with only files ---');
  const action3 = {
    kind: 'ls',
    target_path: '/working_memory/test_directory/folder2'
  };

  const result3 = await handle_ls_action(action3, testMemory.working_memory, testStorage.storage, 3);
  console.log('Status:', result3.status);
  console.log('Message:');
  console.log(result3.message);
  console.log('Data:', JSON.stringify(result3.data, null, 2));

  // Test empty directory
  console.log('\n--- Test 4: Empty directory ---');
  const emptyTestMemory = {
    working_memory: {
      empty_dir: {}
    }
  };

  const action4 = {
    kind: 'ls',
    target_path: '/working_memory/empty_dir'
  };

  const result4 = await handle_ls_action(action4, emptyTestMemory.working_memory, testStorage.storage, 4);
  console.log('Status:', result4.status);
  console.log('Message:');
  console.log(result4.message);
  console.log('Data:', JSON.stringify(result4.data, null, 2));

  console.log('\n--- Testing completed successfully! ---');
}

testEnhancedLsOutput().catch(console.error);

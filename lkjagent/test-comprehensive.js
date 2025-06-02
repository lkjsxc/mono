/**
 * Comprehensive test of all lkjagent tools
 */

const { execute_actions } = require('./dist/util/executor');
const { validate_action } = require('./dist/util/action_validator');

async function comprehensiveTest() {
  console.log('🧪 Running comprehensive lkjagent test...');
  
  try {
    // Test 1: Set operation
    console.log('\n1️⃣ Testing SET operation...');
    const setAction = {
      kind: 'set',
      target_path: '/working_memory/user_data/test_item',
      content: { message: 'Hello lkjagent!', value: 42 }
    };
    
    let validation = validate_action(setAction);
    if (!validation.valid) {
      throw new Error(`SET validation failed: ${validation.error}`);
    }
    console.log('   ✅ SET action validation passed');
    
    await execute_actions([setAction]);
    console.log('   ✅ SET action executed');
    
    // Test 2: Get operation
    console.log('\n2️⃣ Testing GET operation...');
    const getAction = {
      kind: 'get',
      target_path: '/working_memory/user_data/test_item'
    };
    
    validation = validate_action(getAction);
    if (!validation.valid) {
      throw new Error(`GET validation failed: ${validation.error}`);
    }
    console.log('   ✅ GET action validation passed');
    
    await execute_actions([getAction]);
    console.log('   ✅ GET action executed');
    
    // Test 3: List operation
    console.log('\n3️⃣ Testing LS operation...');
    const lsAction = {
      kind: 'ls',
      target_path: '/working_memory/user_data'
    };
    
    validation = validate_action(lsAction);
    if (!validation.valid) {
      throw new Error(`LS validation failed: ${validation.error}`);
    }
    console.log('   ✅ LS action validation passed');
    
    await execute_actions([lsAction]);
    console.log('   ✅ LS action executed');
    
    // Test 4: Set storage operation
    console.log('\n4️⃣ Testing STORAGE SET operation...');
    const storageSetAction = {
      kind: 'set',
      target_path: '/storage/test_data/sample',
      content: 'This is persistent storage data'
    };
    
    validation = validate_action(storageSetAction);
    if (!validation.valid) {
      throw new Error(`STORAGE SET validation failed: ${validation.error}`);
    }
    console.log('   ✅ STORAGE SET action validation passed');
    
    await execute_actions([storageSetAction]);
    console.log('   ✅ STORAGE SET action executed');
    
    // Test 5: Search operation
    console.log('\n5️⃣ Testing SEARCH operation...');
    const searchAction = {
      kind: 'search',
      target_path: '/working_memory',
      content: 'Hello'
    };
    
    validation = validate_action(searchAction);
    if (!validation.valid) {
      throw new Error(`SEARCH validation failed: ${validation.error}`);
    }
    console.log('   ✅ SEARCH action validation passed');
    
    await execute_actions([searchAction]);
    console.log('   ✅ SEARCH action executed');
    
    // Test 6: Move operation
    console.log('\n6️⃣ Testing MV operation...');
    const mvAction = {
      kind: 'mv',
      source_path: '/working_memory/user_data/test_item',
      target_path: '/storage/moved_data/test_item'
    };
    
    validation = validate_action(mvAction);
    if (!validation.valid) {
      throw new Error(`MV validation failed: ${validation.error}`);
    }
    console.log('   ✅ MV action validation passed');
    
    await execute_actions([mvAction]);
    console.log('   ✅ MV action executed');
    
    // Test 7: Remove operation
    console.log('\n7️⃣ Testing RM operation...');
    const rmAction = {
      kind: 'rm',
      target_path: '/storage/test_data/sample'
    };
    
    validation = validate_action(rmAction);
    if (!validation.valid) {
      throw new Error(`RM validation failed: ${validation.error}`);
    }
    console.log('   ✅ RM action validation passed');
    
    await execute_actions([rmAction]);
    console.log('   ✅ RM action executed');
    
    console.log('\n🎉 ALL TESTS PASSED! lkjagent framework is working correctly.');
    
  } catch (error) {
    console.error('\n❌ Test failed:', error);
    process.exit(1);
  }
}

comprehensiveTest();

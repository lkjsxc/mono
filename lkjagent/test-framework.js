/**
 * Simple test to verify lkjagent framework functionality
 */

const { execute_actions } = require('./dist/util/executor');
const { validate_action } = require('./dist/util/action_validator');

async function testFramework() {
  console.log('🧪 Testing lkjagent framework...');
  
  try {
    // Test action validation
    const testAction = {
      kind: 'set',
      target_path: '/working_memory/user_data/test',
      content: 'Hello, lkjagent!'
    };
    
    const validation = validate_action(testAction);
    if (!validation.valid) {
      console.error('❌ Action validation failed:', validation.error);
      return;
    }
    console.log('✅ Action validation passed');
    
    // Test action execution
    await execute_actions([testAction]);
    console.log('✅ Action execution completed');
    
    // Test get action
    const getAction = {
      kind: 'get',
      target_path: '/working_memory/user_data/test'
    };
    
    const getValidation = validate_action(getAction);
    if (!getValidation.valid) {
      console.error('❌ Get action validation failed:', getValidation.error);
      return;
    }
    
    await execute_actions([getAction]);
    console.log('✅ Get action completed');
    
    console.log('🎉 Framework test completed successfully!');
    
  } catch (error) {
    console.error('❌ Framework test failed:', error);
  }
}

testFramework();

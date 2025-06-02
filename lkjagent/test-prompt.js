/**
 * Test prompt generation specifically
 */

const { generate_system_prompt } = require('./dist/util/prompt');

async function testPrompt() {
  console.log('🧪 Testing prompt generation...');
  
  try {
    const prompt = await generate_system_prompt();
    console.log('✅ System prompt generated successfully');
    console.log('📝 Prompt length:', prompt.length);
    console.log('📝 Prompt preview:\n');
    console.log(prompt.substring(0, 500) + '...\n');
    
    console.log('🎉 Prompt test completed successfully!');
    
  } catch (error) {
    console.error('❌ Prompt test failed:', error);
  }
}

testPrompt();

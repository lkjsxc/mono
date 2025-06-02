/**
 * Test XML parsing and prompt generation
 */

const { parse_actions_from_xml } = require('./dist/util/xml');
const { generate_system_prompt } = require('./dist/util/prompt');

async function testXmlAndPrompt() {
  console.log('🧪 Testing XML parsing and prompt generation...');
  
  try {
    // Test XML parsing
    const xmlResponse = `
      <action>
        <kind>set</kind>
        <target_path>/working_memory/user_data/notes</target_path>
        <content>This is a test note</content>
      </action>
      <action>
        <kind>get</kind>
        <target_path>/working_memory/user_data/notes</target_path>
      </action>
    `;
    
    const actions = parse_actions_from_xml(xmlResponse);
    console.log('✅ XML parsing successful:', actions);
    
    // Test prompt generation
    const prompt = await generate_system_prompt();
    console.log('✅ System prompt generated successfully');
    console.log('📝 Prompt preview (first 200 chars):', prompt.substring(0, 200) + '...');
    
    console.log('🎉 XML and prompt tests completed successfully!');
    
  } catch (error) {
    console.error('❌ Test failed:', error);
  }
}

testXmlAndPrompt();

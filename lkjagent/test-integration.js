/**
 * Integration test simulating complete agent interaction
 */

const { parse_actions_from_xml } = require('./dist/util/xml');
const { validate_action } = require('./dist/util/action_validator');
const { execute_actions } = require('./dist/util/executor');
const { generate_system_prompt } = require('./dist/util/prompt');

async function simulateAgentInteraction() {
  console.log('🤖 Simulating complete agent interaction...');
  
  try {
    // Step 1: Generate system prompt (what would be sent to LLM)
    console.log('\n1️⃣ Generating system prompt...');
    const systemPrompt = await generate_system_prompt();
    console.log(`   ✅ System prompt generated (${systemPrompt.length} characters)`);
    
    // Step 2: Simulate LLM response with XML actions
    console.log('\n2️⃣ Simulating LLM response...');
    const simulatedLlmResponse = `
I'll help you manage your tasks. Let me check what's in your working memory and then add a new task.

<action>
  <kind>get</kind>
  <target_path>/working_memory/user_data</target_path>
</action>

<action>
  <kind>set</kind>
  <target_path>/working_memory/user_data/todo/project_planning</target_path>
  <content>{"task": "Plan lkjagent documentation", "priority": "high", "status": "pending"}</content>
</action>

<action>
  <kind>set</kind>
  <target_path>/storage/knowledge_base/agent_capabilities</target_path>
  <content>{"memory_management": true, "task_tracking": true, "data_persistence": true}</content>
</action>

<action>
  <kind>ls</kind>
  <target_path>/working_memory/user_data/todo</target_path>
</action>
`;
    
    // Step 3: Parse actions from LLM response
    console.log('\n3️⃣ Parsing actions from LLM response...');
    const actions = parse_actions_from_xml(simulatedLlmResponse);
    console.log(`   ✅ Parsed ${actions.length} actions from LLM response`);
    
    // Step 4: Validate all actions
    console.log('\n4️⃣ Validating actions...');
    let allValid = true;
    for (let i = 0; i < actions.length; i++) {
      const validation = validate_action(actions[i]);
      if (!validation.valid) {
        console.error(`   ❌ Action ${i + 1} validation failed: ${validation.error}`);
        allValid = false;
      } else {
        console.log(`   ✅ Action ${i + 1} (${actions[i].kind}) validation passed`);
      }
    }
    
    if (!allValid) {
      throw new Error('Some actions failed validation');
    }
    
    // Step 5: Execute all actions
    console.log('\n5️⃣ Executing actions...');
    await execute_actions(actions);
    console.log('   ✅ All actions executed successfully');
    
    // Step 6: Generate new system prompt with updated state
    console.log('\n6️⃣ Generating updated system prompt...');
    const updatedPrompt = await generate_system_prompt();
    console.log(`   ✅ Updated system prompt generated (${updatedPrompt.length} characters)`);
    
    // Step 7: Show the difference in prompt length (indicates state changes)
    const promptDiff = updatedPrompt.length - systemPrompt.length;
    console.log(`   📈 Prompt size change: ${promptDiff > 0 ? '+' : ''}${promptDiff} characters`);
    
    console.log('\n🎉 Complete agent interaction simulation SUCCESS!');
    console.log('\n📊 Summary:');
    console.log(`   • Actions parsed: ${actions.length}`);
    console.log(`   • Actions executed: ${actions.length}`);
    console.log(`   • Initial prompt size: ${systemPrompt.length} chars`);
    console.log(`   • Final prompt size: ${updatedPrompt.length} chars`);
    console.log('   • Memory state: Updated with new tasks and knowledge');
    
  } catch (error) {
    console.error('\n❌ Agent interaction simulation failed:', error);
    process.exit(1);
  }
}

simulateAgentInteraction();

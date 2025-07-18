# Agent Philosophy and Operation

## Continuous Operation

The LKJAgent is designed with the following operational principles:

- **Never-Ending Processing**: The agent never terminates its execution cycle
- **Perpetual Enrichment**: Continuously strives to enrich disk storage with valuable data
- **Endless Thinking Mode**: Operates in infinite iteration mode (`max_iterations: -1`)
- **Self-Sustaining**: Maintains its own memory and context through persistent storage

## Memory Architecture

Both working memory (RAM) and persistent disk memory are stored in the unified `memory.json` file, with LLM-controlled paging operations managing the flow of context between memory layers:

- **LLM-Directed Paging**: The LLM specifies context keys to move between working and disk storage
- **Context Key Management**: LLM identifies and tags important context elements for persistence
- **Automatic Context Transfer**: Context elements are moved to disk based on LLM directives
- **Seamless transitions between volatile and persistent states**
- **Comprehensive memory history preservation**
- **Context continuity across agent restarts**
- **Unified memory query interface**
- **Smart Context Paging**: LLM determines what context to preserve, archive, or retrieve
- **Context Key Directory**: Maintained in `context_keys.json` for efficient retrieval

## LLM Output Format

**IMPORTANT**: All LLM interactions must use a simple `<tag>` and `</tag>` format only. Do NOT use XML markup, HTML markup, or any complex formatting. Use only simple opening and closing tags with plain text content.

Required format for all LLM responses:

```
<thinking>
<analysis>Current situation analysis</analysis>
<planning>Next steps and strategy</planning>
<context_keys>key1,key2,key3</context_keys>
</thinking>

<action>
<type>disk_storage</type>
<context_key>specific_key</context_key>
<operation>store</operation>
<data>content to store/process</data>
</action>

<evaluation>
<progress>Assessment of current progress</progress>
<quality_score>0.85</quality_score>
<enrichment_rate>0.92</enrichment_rate>
<recommendations>rec1,rec2</recommendations>
</evaluation>

<paging>
<operation>context_management</operation>
<move_to_disk>context_key1,context_key2</move_to_disk>
<retrieve_from_disk>context_key3</retrieve_from_disk>
<archive_old>context_key4</archive_old>
<rationale>Explanation for paging decisions</rationale>
</paging>
```

**Format Rules**:
- Use only simple `<tag>content</tag>` pairs
- No attributes, namespaces, or complex structures
- Plain text content only
- No nested complex elements
- Consistent tag names as shown above

## State Machine Operation

The agent operates in four distinct states with perpetual cycling:

### 1. THINKING State
- Deep contemplative analysis using state-specific system prompt
- Context key identification for important insights
- Strategic planning with simple tag format output (`<thinking>` blocks)
- Automatic context key tagging and categorization

### 2. EXECUTING State
- Task execution using state-specific system prompt
- Disk storage operations directed by LLM in simple tag format (`<action>` blocks)
- Context key specification for storing execution results
- Quality data accumulation and organization

### 3. EVALUATING State
- Performance evaluation using state-specific system prompt
- Quality metrics and progress analysis in simple tag format (`<evaluation>` blocks)
- Success measurement and improvement identification
- Context effectiveness assessment

### 4. PAGING State
- Context analysis using specialized paging system prompt
- LLM directives for context key management in simple tag format (`<paging>` blocks)
- Automatic context transfer between working memory and disk storage
- Memory optimization and cleanup operations
- Context key archival and retrieval operations

Each state function manages context width, processes simple tag format outputs, and returns the next state, ensuring continuous operation without termination while maintaining efficient memory usage through LLM-directed paging.

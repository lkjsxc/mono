# Configuration and Data Schemas

## Configuration Schema

The `data/config.json` file configures all aspects of the agent, including state-specific system prompts:

```json
{
  "lmstudio": {
    "base_url": "http://host.docker.internal:1234/v1/chat/completions",
    "model": "qwen/qwen3-8b",
    "temperature": 0.7,
    "max_tokens": 2048,
    "timeout_ms": 30000,
    "tag_format_enforced": true
  },
  "agent": {
    "max_iterations": -1,
    "self_directed": 1,
    "state_prompts": {
      "thinking": {
        "system_prompt": "You are in THINKING state. Analyze deeply, contemplate thoroughly, and identify context keys for important insights. Always respond in simple tag format with <thinking> blocks containing analysis, planning, and context_keys.",
        "objectives": ["deep_analysis", "strategic_planning", "context_identification"],
        "tag_blocks": ["<thinking>"],
        "context_key_focus": true
      },
      "executing": {
        "system_prompt": "You are in EXECUTING state. Perform actions to enrich disk storage with valuable data. Always respond in simple tag format with <action> blocks specifying disk storage operations and context keys.",
        "objectives": ["disk_enrichment", "action_execution", "data_storage"],
        "tag_blocks": ["<action>"],
        "context_key_focus": true
      },
      "evaluating": {
        "system_prompt": "You are in EVALUATING state. Assess progress, measure quality, and provide metrics. Always respond in simple tag format with <evaluation> blocks containing progress assessments and quality metrics.",
        "objectives": ["progress_assessment", "quality_measurement", "performance_analysis"],
        "tag_blocks": ["<evaluation>"],
        "context_key_focus": false
      },
      "paging": {
        "system_prompt": "You are in PAGING state. Manage context and memory efficiently by specifying which context keys to move to disk storage, retrieve, or archive. Always respond in simple tag format with <paging> blocks containing specific directives.",
        "objectives": ["context_management", "memory_optimization", "disk_organization"],
        "tag_blocks": ["<paging>"],
        "context_key_focus": true,
        "paging_directives": {
          "move_to_disk": "Specify context keys to move from working memory to disk",
          "retrieve_from_disk": "Specify context keys to retrieve from disk to working memory",
          "archive_old": "Specify context keys to archive for long-term storage"
        }
      }
    },
    "tagged_memory": {
      "max_entries": 1000,
      "max_tags_per_entry": 8,
      "auto_cleanup_threshold": 0.8,
      "tag_similarity_threshold": 0.7,
      "context_key_integration": true
    },
    "llm_decisions": {
      "confidence_threshold": 0.8,
      "decision_timeout_ms": 5000,
      "fallback_enabled": true,
      "context_window_size": 4096,
      "tag_validation": true,
      "context_key_extraction": true
    },
    "enhanced_tools": {
      "tool_chaining_enabled": true,
      "max_tool_chain_length": 5,
      "parallel_tool_execution": false,
      "context_key_tracking": true
    },
    "paging_control": {
      "llm_controlled": true,
      "auto_paging_threshold": 0.8,
      "context_key_retention": 100,
      "disk_storage_priority": "llm_specified"
    }
  },
  "tag_format": {
    "validation_strict": true,
    "required_blocks": {
      "thinking": ["<thinking>"],
      "executing": ["<action>"],
      "evaluating": ["<evaluation>"],
      "paging": ["<paging>"]
    },
    "context_key_format": {
      "max_length": 64,
      "allowed_chars": "alphanumeric_underscore_dash",
      "case_sensitive": false
    }
  },
  "context_management": {
    "keys_file": "data/context_keys.json",
    "max_working_keys": 50,
    "disk_storage_path": "data/disk_context/",
    "compression_enabled": false,
    "encryption_enabled": false
  },
  "http": {
    "timeout_seconds": 30,
    "max_redirects": 3,
    "user_agent": "LKJAgent-Enhanced/1.0"
  }
}
```

## Memory.json Schema

The `data/memory.json` file contains unified storage for both working and disk memory with context key integration:

```json
{
  "metadata": {
    "version": "2.0",
    "created": "2025-07-18T12:00:00Z",
    "last_modified": "2025-07-18T12:00:00Z",
    "context_key_version": "1.0"
  },
  "working_memory": {
    "current_task": "Perpetual thinking and disk enrichment",
    "context": "Unified context spanning state transitions...",
    "current_state": "thinking",
    "variables": {},
    "context_window_usage": 0.75,
    "active_context_keys": ["analysis_2025_07_18", "strategy_planning", "disk_enrichment_metrics"]
  },
  "disk_memory": {
    "tagged_entries": [],
    "knowledge_base": {
      "concepts": {},
      "procedures": {},
      "facts": {}
    },
    "accumulated_insights": [],
    "enrichment_metrics": {
      "total_entries": 0,
      "quality_score": 0.0,
      "enrichment_rate": 0.0
    },
    "context_storage": {
      "archived_contexts": {},
      "context_relationships": {},
      "access_patterns": {}
    }
  },
  "context_key_directory": {
    "active_keys": {
      "analysis_2025_07_18": {
        "location": "working_memory",
        "created": "2025-07-18T12:00:00Z",
        "size_bytes": 1024,
        "access_count": 15,
        "tags": ["analysis", "current", "important"]
      }
    },
    "archived_keys": {},
    "disk_keys": {}
  },
  "unified_log": [],
  "context_management": {
    "window_size": 4096,
    "current_usage": 0,
    "trim_history": [],
    "paging_operations": [],
    "llm_directives": []
  }
}
```

## Context Keys Schema

The `data/context_keys.json` file maintains the directory of context keys for efficient management:

```json
{
  "metadata": {
    "version": "1.0",
    "last_updated": "2025-07-18T12:00:00Z",
    "total_keys": 25
  },
  "working_memory_keys": {
    "current_analysis": {
      "key": "analysis_2025_07_18",
      "content_type": "analytical_thinking",
      "size_bytes": 1024,
      "created": "2025-07-18T12:00:00Z",
      "last_accessed": "2025-07-18T12:30:00Z",
      "access_count": 15,
      "importance_score": 0.95,
      "tags": ["analysis", "current", "high_priority"]
    }
  },
  "disk_storage_keys": {
    "historical_insights": {
      "key": "insights_batch_001",
      "content_type": "accumulated_knowledge",
      "file_path": "data/disk_context/insights_batch_001.json",
      "size_bytes": 4096,
      "compressed": false,
      "created": "2025-07-17T15:00:00Z",
      "archived": "2025-07-18T09:00:00Z",
      "access_count": 5,
      "importance_score": 0.88,
      "tags": ["insights", "historical", "archived"]
    }
  },
  "archived_keys": {},
  "key_relationships": {
    "analysis_2025_07_18": {
      "related_keys": ["strategy_planning", "execution_results"],
      "dependency_type": "sequential",
      "relationship_strength": 0.9
    }
  },
  "usage_statistics": {
    "most_accessed": "analysis_2025_07_18",
    "least_accessed": "temp_calculation_001",
    "average_size_bytes": 2048,
    "total_disk_usage_bytes": 102400
  }
}
```

## Build System

The Makefile provides:
- Modular compilation with proper dependencies
- Debug symbol generation
- Warning-as-error enforcement
- Clean build targets
- Test compilation support
- Data token system compilation

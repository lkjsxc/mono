#include "lkjagent.h"
#include <string.h>

// **ULTIMATE REQUEST SYSTEM FOR QWEN3-8B**
// Maximizes 128K context window with intelligent role-based prompt assembly

typedef struct {
    const char* role_id;
    const char* role_identity; 
    const char* role_purpose;
    const char* knowledge_domains;
    data_t* role_id_buf;
    data_t* role_identity_buf;
    data_t* role_purpose_buf;
    data_t* knowledge_domains_cstr;
    object_t* knowledge_domains_obj;
    data_t* knowledge_domains_buffer;
    const char* current_state;
    uint64_t context_budget;
    uint64_t used_tokens;
} context_assembly_t;

// Forward declarations
static result_t extract_role_configuration(pool_t* pool, object_t* config, context_assembly_t* ctx);
static result_t build_prompt(pool_t* pool, context_assembly_t* ctx, object_t* working_memory, object_t* storage, data_t** prompt);
static result_t assemble_role_foundation(pool_t* pool, context_assembly_t* ctx, data_t** foundation);
static result_t assemble_contextual_knowledge(pool_t* pool, context_assembly_t* ctx, object_t* storage, data_t** knowledge);
static result_t assemble_working_context(pool_t* pool, context_assembly_t* ctx, object_t* working_memory, data_t** context);
static result_t assemble_state_specific_guidance(pool_t* pool, context_assembly_t* ctx, data_t** guidance);
static result_t build_request_payload(pool_t* pool, const data_t* prompt, object_t* config, const char* state, data_t** payload);
static result_t optimize_for_qwen3_parameters(pool_t* pool, object_t* config, const char* state, data_t** parameters);
static uint64_t estimate_token_count(const data_t* text);
static void context_cleanup(pool_t* pool, context_assembly_t* ctx);
static result_t copy_to_cstring(pool_t* pool, const data_t* src, data_t** dst, const char** view);

// **MAIN REQUEST BUILDER**
result_t lkjagent_request(pool_t* pool, lkjagent_t* lkjagent, data_t** dst) {
    if (!pool || !lkjagent || !dst) {
        RETURN_ERR("Invalid parameters for request");
    }

    context_assembly_t ctx = {0};
    ctx.context_budget = 120000;

    data_t* prompt = NULL;
    data_t* payload = NULL;
    data_t* content_type = NULL;
    data_t* response_body = NULL;

    object_t* endpoint_obj = NULL;
    if (object_provide_str(&endpoint_obj, lkjagent->config, "llm.endpoint") != RESULT_OK ||
        !endpoint_obj->data || endpoint_obj->data->size == 0) {
        RETURN_ERR("Failed to obtain llm.endpoint from config");
    }

    if (extract_role_configuration(pool, lkjagent->config, &ctx) != RESULT_OK) {
        RETURN_ERR("Failed to extract role configuration");
    }

    ctx.current_state = "analyzing";
    object_t* state_obj = NULL;
    if (object_provide_str(&state_obj, lkjagent->memory, "state") == RESULT_OK &&
        state_obj->data && state_obj->data->size > 0) {
        ctx.current_state = state_obj->data->data;
    }

    object_t* working_memory = NULL;
    object_t* storage = NULL;

    if (object_provide_str(&working_memory, lkjagent->memory, "working_memory") != RESULT_OK) {
        PRINT_ERR("Warning: No working memory found");
    }

    if (object_provide_str(&storage, lkjagent->memory, "storage") != RESULT_OK) {
        PRINT_ERR("Warning: No storage found");
    }

    if (build_prompt(pool, &ctx, working_memory, storage, &prompt) != RESULT_OK) {
        goto cleanup;
    }

    if (build_request_payload(pool, prompt, lkjagent->config, ctx.current_state, &payload) != RESULT_OK) {
        goto cleanup;
    }

    if (data_create_str(pool, &content_type, "application/json") != RESULT_OK) {
        goto cleanup;
    }

    if (http_post(pool, endpoint_obj->data, content_type, payload, &response_body) != RESULT_OK) {
        PRINT_ERR("HTTP POST to LLM endpoint failed");
        goto cleanup;
    }

    *dst = response_body;
    response_body = NULL;

cleanup:
    if (response_body && data_destroy(pool, response_body) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup response body");
    }
    if (content_type && data_destroy(pool, content_type) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup content type data");
    }
    if (payload && data_destroy(pool, payload) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup request payload");
    }
    if (prompt && data_destroy(pool, prompt) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup prompt");
    }
    context_cleanup(pool, &ctx);
    return *dst ? RESULT_OK : RESULT_ERR;
}

// **ROLE CONFIGURATION EXTRACTOR** 
static result_t extract_role_configuration(pool_t* pool, object_t* config, context_assembly_t* ctx) {
    object_t* active_role_obj = NULL;
    object_t* role_config = NULL;
    data_t* role_path = NULL;

    if (object_provide_str(&active_role_obj, config, "agent.roles.active_role") != RESULT_OK) {
        RETURN_ERR("Failed to get active role from config");
    }

    if (copy_to_cstring(pool, active_role_obj->data, &ctx->role_id_buf, &ctx->role_id) != RESULT_OK) {
        RETURN_ERR("Failed to copy role id");
    }

    if (data_create_str(pool, &role_path, "agent.roles.available_roles.") != RESULT_OK ||
        data_append_data(pool, &role_path, active_role_obj->data) != RESULT_OK) {
        if (role_path && data_destroy(pool, role_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup role_path after append error");
        }
        RETURN_ERR("Failed to build role path");
    }

    if (object_provide_data(&role_config, config, role_path) != RESULT_OK) {
        if (data_destroy(pool, role_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup role_path after access error");
        }
        RETURN_ERR("Failed to get role configuration");
    }

    if (data_destroy(pool, role_path) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup role_path");
    }

    object_t* identity_obj = NULL;
    object_t* purpose_obj = NULL;
    object_t* domains_obj = NULL;

    if (object_provide_str(&identity_obj, role_config, "identity") == RESULT_OK && identity_obj->data) {
        if (copy_to_cstring(pool, identity_obj->data, &ctx->role_identity_buf, &ctx->role_identity) != RESULT_OK) {
            RETURN_ERR("Failed to copy role identity");
        }
    }

    if (object_provide_str(&purpose_obj, role_config, "creative_focus") == RESULT_OK && purpose_obj->data) {
        if (copy_to_cstring(pool, purpose_obj->data, &ctx->role_purpose_buf, &ctx->role_purpose) != RESULT_OK) {
            RETURN_ERR("Failed to copy role purpose");
        }
    }

    if (object_provide_str(&domains_obj, role_config, "knowledge_domains") == RESULT_OK) {
        ctx->knowledge_domains_obj = domains_obj;
    } else {
        data_t* domains_key = NULL;
        if (data_create_str(pool, &domains_key, "knowledge_domains") == RESULT_OK) {
            if (object_provide_data(&domains_obj, role_config, domains_key) == RESULT_OK) {
                ctx->knowledge_domains_obj = domains_obj;
            }
            if (data_destroy(pool, domains_key) != RESULT_OK) {
                PRINT_ERR("Failed to destroy domains_key");
            }
        }
    }

    if (ctx->knowledge_domains_obj && ctx->knowledge_domains_obj->data && ctx->knowledge_domains_obj->data->size > 0) {
        if (copy_to_cstring(pool, ctx->knowledge_domains_obj->data, &ctx->knowledge_domains_cstr, &ctx->knowledge_domains) != RESULT_OK) {
            RETURN_ERR("Failed to copy knowledge domains");
        }
    }

    if (!ctx->knowledge_domains && ctx->knowledge_domains_obj && ctx->knowledge_domains_obj->child) {
        if (data_create(pool, &ctx->knowledge_domains_buffer) == RESULT_OK) {
            object_t* item = ctx->knowledge_domains_obj->child;
            while (item) {
                if (item->data && item->data->data) {
                    if (ctx->knowledge_domains_buffer->size > 0) {
                        if (data_append_str(pool, &ctx->knowledge_domains_buffer, ", ") != RESULT_OK) {
                            PRINT_ERR("Failed to append separator when joining knowledge domains");
                        }
                    }
                    if (data_append_data(pool, &ctx->knowledge_domains_buffer, item->data) != RESULT_OK) {
                        PRINT_ERR("Failed to append knowledge domain entry");
                    }
                }
                item = item->next;
            }
            ctx->knowledge_domains = ctx->knowledge_domains_buffer->data;
        }
    }

    ctx->current_state = "analyzing";
    return RESULT_OK;
}

static void context_cleanup(pool_t* pool, context_assembly_t* ctx) {
    if (ctx->role_id_buf && data_destroy(pool, ctx->role_id_buf) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup role id buffer");
    }
    if (ctx->role_identity_buf && data_destroy(pool, ctx->role_identity_buf) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup role identity buffer");
    }
    if (ctx->role_purpose_buf && data_destroy(pool, ctx->role_purpose_buf) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup role purpose buffer");
    }
    if (ctx->knowledge_domains_cstr && data_destroy(pool, ctx->knowledge_domains_cstr) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup knowledge domains string");
    }
    if (ctx->knowledge_domains_buffer && data_destroy(pool, ctx->knowledge_domains_buffer) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup knowledge domains buffer");
    }
}

static result_t copy_to_cstring(pool_t* pool, const data_t* src, data_t** dst, const char** view) {
    if (!src || src->size == 0) {
        *view = NULL;
        *dst = NULL;
        return RESULT_OK;
    }

    if (pool_data_alloc(pool, dst, src->size + 1) != RESULT_OK) {
        RETURN_ERR("Failed to allocate string buffer");
    }

    for (uint64_t i = 0; i < src->size; i++) {
        (*dst)->data[i] = src->data[i];
    }
    (*dst)->data[src->size] = '\0';
    (*dst)->size = src->size;
    *view = (*dst)->data;
    return RESULT_OK;
}

// **PROMPT BUILDER** - Constructs the prompt sent to the LLM
static result_t build_prompt(pool_t* pool, context_assembly_t* ctx, object_t* working_memory, object_t* storage, data_t** prompt) {
    if (data_create(pool, prompt) != RESULT_OK) {
        RETURN_ERR("Failed to create prompt data");
    }
    
    // **SECTION 1: ROLE FOUNDATION** (~500 tokens)
    data_t* foundation = NULL;
    if (assemble_role_foundation(pool, ctx, &foundation) == RESULT_OK) {
        if (data_append_data(pool, prompt, foundation) != RESULT_OK) {
            PRINT_ERR("Failed to append role foundation");
        } else {
            ctx->used_tokens += estimate_token_count(foundation);
        }
        if (data_destroy(pool, foundation) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup foundation");
        }
    }
    
    // **SECTION 2: CONTEXTUAL KNOWLEDGE** (~60,000 tokens)
    if (ctx->used_tokens < ctx->context_budget * 0.5) {
        data_t* knowledge = NULL;
        if (assemble_contextual_knowledge(pool, ctx, storage, &knowledge) == RESULT_OK) {
            if (data_append_str(pool, prompt, "\n\n=== KNOWLEDGE BASE ===\n") == RESULT_OK &&
                data_append_data(pool, prompt, knowledge) == RESULT_OK) {
                ctx->used_tokens += estimate_token_count(knowledge);
            }
            if (data_destroy(pool, knowledge) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup knowledge");
            }
        }
    }
    
    // **SECTION 3: WORKING CONTEXT** (~30,000 tokens)  
    if (ctx->used_tokens < ctx->context_budget * 0.75) {
        data_t* context = NULL;
        if (assemble_working_context(pool, ctx, working_memory, &context) == RESULT_OK) {
            if (data_append_str(pool, prompt, "\n\n=== CURRENT CONTEXT ===\n") == RESULT_OK &&
                data_append_data(pool, prompt, context) == RESULT_OK) {
                ctx->used_tokens += estimate_token_count(context);
            }
            if (data_destroy(pool, context) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup context");
            }
        }
    }
    
    // **SECTION 4: STATE-SPECIFIC GUIDANCE** (~1,000 tokens)
    data_t* guidance = NULL;
    if (assemble_state_specific_guidance(pool, ctx, &guidance) == RESULT_OK) {
        if (data_append_str(pool, prompt, "\n\n=== CURRENT OBJECTIVE ===\n") == RESULT_OK &&
            data_append_data(pool, prompt, guidance) == RESULT_OK) {
            ctx->used_tokens += estimate_token_count(guidance);
        }
        if (data_destroy(pool, guidance) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup guidance");
        }
    }
    
    // **FINAL INSTRUCTION**
    if (data_append_str(pool, prompt, "\n\nRespond with your next action in the specified XML format, maximizing insight and creative depth:") != RESULT_OK) {
        PRINT_ERR("Failed to append final instruction");
    }
    
    return RESULT_OK;
}

// **ROLE FOUNDATION ASSEMBLER** - Creates rich role identity
static result_t assemble_role_foundation(pool_t* pool, context_assembly_t* ctx, data_t** foundation) {
    if (data_create(pool, foundation) != RESULT_OK) {
        RETURN_ERR("Failed to create foundation");
    }
    
    // Build comprehensive role identity
    char foundation_text[2048];
    snprintf(foundation_text, sizeof(foundation_text),
        "=== AGENT IDENTITY ===\n"
        "You are %s - %s\n"
        "Purpose: %s\n" 
        "Expertise Domains: %s\n"
        "Context Capacity: 128,000 tokens (maximize utilization)\n"
        "Evolution: Continuously self-improving through each interaction\n\n"
        
        "OPERATIONAL PRINCIPLES:\n"
        "• Depth Over Breadth: Every response demonstrates profound mastery\n"
        "• Creative Synthesis: Generate unprecedented insights by connecting knowledge\n"
        "• Progressive Enhancement: Each interaction enriches your capabilities\n"
        "• Maximum Context Usage: Leverage full 128K tokens for rich understanding\n"
        "• Role Mastery: Embody complete expertise in your specialized domain\n\n"
        
        "MEMORY ARCHITECTURE:\n"
        "• Working Memory: Active context for immediate work (high priority)\n"
        "• Storage: Unlimited knowledge base (all accumulated wisdom)\n"
        "• Synthesis Engine: Cross-domain connection and insight generation\n"
        "• Enhancement System: Continuous learning and capability evolution\n",
        
        ctx->role_id ? ctx->role_id : "Synthesis",
        ctx->role_identity ? ctx->role_identity : "Creator-Librarian Hybrid",
        ctx->role_purpose ? ctx->role_purpose : "Continuously enriching knowledge while generating unprecedented content",
        ctx->knowledge_domains ? ctx->knowledge_domains : "cross-domain synthesis"
    );
    
    if (data_append_str(pool, foundation, foundation_text) != RESULT_OK) {
        RETURN_ERR("Failed to append foundation text");
    }
    
    return RESULT_OK;
}

// **CONTEXTUAL KNOWLEDGE ASSEMBLER** - Intelligently selects relevant storage content
static result_t assemble_contextual_knowledge(pool_t* pool, context_assembly_t* ctx, object_t* storage, data_t** knowledge) {
    (void)ctx; // Role-specific filtering could be added here
    
    if (data_create(pool, knowledge) != RESULT_OK) {
        RETURN_ERR("Failed to create knowledge data");
    }
    
    if (!storage || !storage->child) {
        if (data_append_str(pool, knowledge, "Knowledge base is being initialized. Ready to accumulate unlimited wisdom.") != RESULT_OK) {
            RETURN_ERR("Failed to append empty knowledge message");
        }
        return RESULT_OK;
    }
    
    // Add storage content with intelligent selection
    // TODO: Implement role-specific knowledge filtering and prioritization
    uint64_t item_count = 0;
    object_t* current = storage->child;
    
    while (current && item_count < 100) { // Reasonable limit for now
        if (current->data && current->child && current->child->data) {
            if (data_append_str(pool, knowledge, "\n[") != RESULT_OK ||
                data_append_data(pool, knowledge, current->data) != RESULT_OK ||
                data_append_str(pool, knowledge, "]: ") != RESULT_OK ||
                data_append_data(pool, knowledge, current->child->data) != RESULT_OK) {
                PRINT_ERR("Failed to append knowledge item");
            }
            item_count++;
        }
        current = current->next;
    }
    
    return RESULT_OK;
}

// **WORKING CONTEXT ASSEMBLER** - Current active information
static result_t assemble_working_context(pool_t* pool, context_assembly_t* ctx, object_t* working_memory, data_t** context) {
    (void)ctx; // State-specific filtering could be added here
    
    if (data_create(pool, context) != RESULT_OK) {
        RETURN_ERR("Failed to create context data");
    }
    
    if (!working_memory || !working_memory->child) {
        if (data_append_str(pool, context, "Fresh start - ready to begin creating and organizing knowledge.") != RESULT_OK) {
            RETURN_ERR("Failed to append empty context message");
        }
        return RESULT_OK;
    }
    
    // Add working memory content
    object_t* current = working_memory->child;
    while (current) {
        if (current->data && current->child && current->child->data) {
            if (data_append_str(pool, context, "\n[") != RESULT_OK ||
                data_append_data(pool, context, current->data) != RESULT_OK ||
                data_append_str(pool, context, "]: ") != RESULT_OK ||
                data_append_data(pool, context, current->child->data) != RESULT_OK) {
                PRINT_ERR("Failed to append context item");
            }
        }
        current = current->next;
    }
    
    return RESULT_OK;
}

// **STATE-SPECIFIC GUIDANCE ASSEMBLER** - Current objectives and methods
static result_t assemble_state_specific_guidance(pool_t* pool, context_assembly_t* ctx, data_t** guidance) {
    if (data_create(pool, guidance) != RESULT_OK) {
        RETURN_ERR("Failed to create guidance data");
    }
    
    const char* state_guidance = "";
    
    if (strcmp(ctx->current_state, "analyzing") == 0) {
        state_guidance = "ANALYZING MODE: Deeply examine the current situation. What needs to be understood, created, or organized? Consider all available information and identify the most impactful next action. Focus on comprehensive analysis that reveals insights and opportunities.";
    } else if (strcmp(ctx->current_state, "creating") == 0) {
        state_guidance = "CREATING MODE: Generate high-quality content that embodies your role expertise. Push creative boundaries while maintaining excellence. Every creation should enhance your knowledge base and demonstrate unprecedented capability.";
    } else if (strcmp(ctx->current_state, "organizing") == 0) {
        state_guidance = "ORGANIZING MODE: Structure knowledge for maximum accessibility and insight generation. Create taxonomies, cross-references, and organizational systems that reveal hidden connections and enhance future retrieval.";
    } else if (strcmp(ctx->current_state, "synthesizing") == 0) {
        state_guidance = "SYNTHESIZING MODE: Connect disparate knowledge domains to generate novel insights. Find patterns, relationships, and synthesis opportunities that create unprecedented understanding and capability.";
    } else if (strcmp(ctx->current_state, "evolving") == 0) {
        state_guidance = "EVOLVING MODE: Reflect on your capabilities and identify enhancement opportunities. How can your expertise deepen? What new approaches could amplify your effectiveness? Focus on meta-learning and self-improvement.";
    } else {
        state_guidance = "ADAPTIVE MODE: Assess the situation and determine the most appropriate approach. Consider analysis, creation, organization, synthesis, and evolution as potential pathways forward.";
    }
    
    if (data_append_str(pool, guidance, state_guidance) != RESULT_OK) {
        RETURN_ERR("Failed to append state guidance");
    }
    
    return RESULT_OK;
}

static result_t build_request_payload(pool_t* pool, const data_t* prompt, object_t* config, const char* state, data_t** payload) {
    if (!pool || !prompt || !config || !payload) {
        RETURN_ERR("Invalid parameters when building request payload");
    }

    *payload = NULL;
    data_t* tmp_payload = NULL;
    data_t* parameters = NULL;

    object_t* model_obj = NULL;
    if (object_provide_str(&model_obj, config, "llm.model") != RESULT_OK ||
        !model_obj->data || model_obj->data->size == 0) {
        RETURN_ERR("Failed to obtain llm.model from config");
    }

    if (data_create(pool, &tmp_payload) != RESULT_OK) {
        RETURN_ERR("Failed to allocate request payload buffer");
    }

    if (data_append_str(pool, &tmp_payload, "{\"model\":\"") != RESULT_OK ||
        data_append_json_escaped(pool, &tmp_payload, model_obj->data) != RESULT_OK ||
        data_append_str(pool, &tmp_payload, "\",\"messages\":[{\"role\":\"user\",\"content\":\"") != RESULT_OK ||
        data_append_json_escaped(pool, &tmp_payload, prompt) != RESULT_OK ||
        data_append_str(pool, &tmp_payload, "\"}]") != RESULT_OK) {
        goto cleanup;
    }

    if (optimize_for_qwen3_parameters(pool, config, state, &parameters) != RESULT_OK) {
        goto cleanup;
    }

    if (data_append_data(pool, &tmp_payload, parameters) != RESULT_OK) {
        goto cleanup;
    }

    if (data_append_str(pool, &tmp_payload, "}") != RESULT_OK) {
        goto cleanup;
    }

    *payload = tmp_payload;
    tmp_payload = NULL;

cleanup:
    if (parameters && data_destroy(pool, parameters) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup parameters data");
    }
    if (tmp_payload && data_destroy(pool, tmp_payload) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup temporary payload buffer");
    }

    return *payload ? RESULT_OK : RESULT_ERR;
}

// **QWEN3-8B PARAMETER OPTIMIZER** - State-specific parameter selection
static result_t optimize_for_qwen3_parameters(pool_t* pool, object_t* config, const char* state, data_t** parameters) {
    (void)config; // Could read optimization settings from config
    
    if (data_create(pool, parameters) != RESULT_OK) {
        RETURN_ERR("Failed to create parameters data");
    }
    
    // Select optimal parameters based on state
    const char* param_string = "";
    
    if (strcmp(state, "analyzing") == 0 || strcmp(state, "synthesizing") == 0 || strcmp(state, "evolving") == 0) {
        // Thinking mode parameters for complex reasoning
        param_string = ",\"temperature\":0.6,\"top_p\":0.95,\"top_k\":20,\"min_p\":0.0,\"max_tokens\":4096";
    } else {
        // Action mode parameters for direct responses
        param_string = ",\"temperature\":0.7,\"top_p\":0.8,\"top_k\":20,\"min_p\":0.0,\"max_tokens\":4096";
    }
    
    if (data_append_str(pool, parameters, param_string) != RESULT_OK) {
        RETURN_ERR("Failed to append parameters");
    }
    
    return RESULT_OK;
}

// **TOKEN ESTIMATION** - Rough approximation for context planning
static uint64_t estimate_token_count(const data_t* text) {
    if (!text || !text->data) {
        return 0;
    }

    return text->size / 4;
}

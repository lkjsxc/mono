import { z } from "zod";

const LlmOptimizationSchema = z.object({
  temperature: z.number().min(0).max(2).optional(),
  top_p: z.number().min(0).max(1).optional(),
  top_k: z.number().min(0).optional(),
  min_p: z.number().min(0).max(1).optional(),
  max_tokens: z.number().int().positive().optional(),
});

const LlmOptimizationModesSchema = z.object({
  thinking_mode: LlmOptimizationSchema.optional(),
  action_mode: LlmOptimizationSchema.optional(),
});

// Legacy prompt schema (string template with substitution)
const PromptFormatLegacySchema = z.object({
  template: z.string().min(1),
  item_template: z.string().min(1),
  empty_working_memory: z.string().min(1).optional(),
});

// New JSON driven prompt assembly schema.
// We build an attributeless XML document with ordered <prompt> root and child elements representing sections.
// Sections ordering rules (applied in builder):
// 1. mandatory sections (always present)
// 2. conditional state sections (state-specific guidance)
// 3. working memory content section (constructed at runtime)
// Each section has a name and raw text content (no angle brackets required in config; builder wraps automatically).
const PromptSectionSchema = z.object({
  name: z.string().min(1),
  content: z.string().default("")
});

const PromptFormatStructuredSchema = z.object({
  // Required always-included sections defined statically.
  mandatory: z.array(PromptSectionSchema).default([]),
  // stateGuidance maps state name -> section content (will be wrapped as <state name="STATE"> but without attributes per requirements; so we actually create <state_STATE>)
  state_guidance: z.record(z.string().min(1)).default({}),
  // Name to use for working memory container element (defaults to working_memory)
  working_memory_element: z.string().min(1).default("working_memory"),
  // Optional root element override (defaults to prompt)
  root: z.string().min(1).default("prompt"),
});

const PromptSchema = z.object({
  global: z.string().min(1),
  states: z.record(z.string().min(1)),
  // Accept legacy or structured format
  format: z.union([PromptFormatLegacySchema, PromptFormatStructuredSchema]),
});

const LlmLoggingSchema = z
  .object({
    enabled: z.boolean().optional(),
    file: z.string().min(1).optional(),
  })
  .optional();

const LlmConfigSchema = z.object({
  endpoint: z.string().min(1),
  model: z.string().min(1),
  context_window: z.number().int().positive().optional(),
  optimization: LlmOptimizationModesSchema.optional(),
  logging: LlmLoggingSchema,
});

const RoleConfigSchema = z.object({
  identity: z.string().min(1),
  specialization: z.string().optional(),
  knowledge_domains: z
    .union([
      z.array(z.string().min(1)),
      z.string().min(1),
    ])
    .optional(),
  creative_focus: z.string().optional(),
  storage_enhancement: z.string().optional(),
  thinking_patterns: z.string().optional(),
});

const RolesSchema = z.object({
  active_role: z.string().min(1),
  available_roles: z.record(RoleConfigSchema),
});

const AgentStateSchema = z.object({
  purpose: z.string().optional(),
  mode: z.string().optional(),
  context_assembly: z.string().optional(),
  token_efficiency: z.string().optional(),
  prompt_template: z.string().optional(),
});

const AgentStatesSchema = z.record(AgentStateSchema);

const MemorySystemSchema = z.object({
  working_memory: z
    .object({
      target_utilization: z.number().min(0).max(1).optional(),
      max_context_tokens: z.number().int().positive().optional(),
      auto_cleanup_limit: z.number().int().nonnegative().optional(),
    })
    .optional(),
  paging: z
    .object({
      enable: z.boolean().optional(),
      context_threshold: z.number().int().positive().optional(),
      target_reduction: z.number().min(0).max(1).optional(),
      preservation_priority: z.array(z.string()).optional(),
    })
    .optional(),
});

const IterationLimitSchema = z
  .object({
    enable: z.boolean().optional(),
    value: z.number().int().positive().optional(),
  })
  .optional();

const PagingLimitSchema = z
  .object({
    enable: z.boolean().optional(),
    value: z.number().int().positive().optional(),
  })
  .optional();

export const AgentConfigSchema = z.object({
  version: z.string().optional(),
  llm: LlmConfigSchema,
  agent: z.object({
    roles: RolesSchema,
    memory_system: MemorySystemSchema.optional(),
    prompts: PromptSchema,
    states: AgentStatesSchema.optional(),
    paging_limit: PagingLimitSchema.optional(),
    iteration_limit: IterationLimitSchema,
  }),
});

export type AgentConfig = z.infer<typeof AgentConfigSchema>;
export type PromptConfig = z.infer<typeof PromptSchema>;
export type PromptSectionConfig = z.infer<typeof PromptSectionSchema>;

export const DEFAULT_MEMORY = {
  state: "analyzing",
  iteration: 0,
  working_memory: {} as Record<string, string>,
  storage: {} as Record<string, string>,
};

export type AgentMemory = typeof DEFAULT_MEMORY;

export const DEFAULT_STATE = "analyzing";

export type AgentState = string;

export type RoleName = string;

export interface PromptFoundation {
  roleId: RoleName;
  identity?: string;
  purpose?: string;
  knowledgeDomains?: string[];
}

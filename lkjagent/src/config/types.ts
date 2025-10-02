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

// ---------------------------------------------------------------------------
// LEGACY PROMPT SCHEMA (template-based)
// ---------------------------------------------------------------------------
const PromptFormatLegacySchema = z.object({
  template: z.string().min(1),
  item_template: z.string().min(1),
  empty_working_memory: z.string().min(1).optional(),
});

// ---------------------------------------------------------------------------
// STRUCTURED PROMPT SCHEMA (legacy structured variant)
// ---------------------------------------------------------------------------
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

// Legacy object prompt configuration (v1)
const PromptSchema = z.object({
  global: z.string().min(1),
  states: z.record(z.string().min(1)),
  format: z.union([PromptFormatLegacySchema, PromptFormatStructuredSchema]),
});

// ---------------------------------------------------------------------------
// NEW PROMPT ENTRY ARRAY (v2)
// Each entry declares name, trigger expression, and content.
// trigger: "always" | "never" | expression using state, working_memory_length
// ---------------------------------------------------------------------------
export const PromptEntrySchema = z.object({
  name: z.string().min(1),
  trigger: z.string().min(1),
  content: z.string().default("")
});
export const PromptEntryArraySchema = z.array(PromptEntrySchema);

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
  storage: z
    .object({
      unlimited_capacity: z.boolean().optional(),
      auto_enrichment: z.boolean().optional(),
      knowledge_graph: z.boolean().optional(),
      cross_references: z.boolean().optional(),
      progressive_enhancement: z.boolean().optional(),
    })
    .optional(),
});

const EnableValueSchema = z
  .object({
    enable: z.boolean().optional(),
    value: z.number().int().positive().optional(),
  })
  .optional();

const IterationLimitSchema = EnableValueSchema; // legacy nested or top-level
const PagingLimitSchema = EnableValueSchema;    // legacy nested paging_limit
const PagingTriggerSchema = EnableValueSchema;  // new top-level paging_trigger

// Legacy agent block (v1)
const AgentLegacySchema = z.object({
  roles: RolesSchema,
  memory_system: MemorySystemSchema.optional(),
  prompts: PromptSchema,
  states: AgentStatesSchema.optional(),
  paging_limit: PagingLimitSchema.optional(),
  iteration_limit: IterationLimitSchema.optional(),
});

// New agent block (v2) — minimal: memory_system + prompts array
const AgentV2Schema = z.object({
  memory_system: MemorySystemSchema.optional(),
  prompts: PromptEntryArraySchema,
});

export const AgentConfigSchema = z
  .object({
    version: z.string().optional(),
    llm: LlmConfigSchema,
    agent: z.union([AgentLegacySchema, AgentV2Schema]),
    iteration_limit: IterationLimitSchema.optional(),
    paging_trigger: PagingTriggerSchema.optional(),
  })
  .superRefine((value, ctx) => {
    // Guard against conflicting iteration limits
    const top = value.iteration_limit;
    const nested: any = (value.agent as any).iteration_limit;
    if (top && nested) {
      const tVal = top.value ?? undefined;
      const nVal = nested.value ?? undefined;
      if (tVal !== nVal) {
        ctx.addIssue({
          code: z.ZodIssueCode.custom,
          path: ["iteration_limit"],
          message: "Conflicting iteration_limit definitions (top-level vs agent.iteration_limit)",
        });
      }
    }
  });

export type AgentConfig = z.infer<typeof AgentConfigSchema>;
export type PromptConfig = z.infer<typeof PromptSchema>;
export type PromptSectionConfig = z.infer<typeof PromptSectionSchema>;
export type PromptEntryConfig = z.infer<typeof PromptEntrySchema>;

export const DEFAULT_MEMORY = {
  state: "analyzing",
  iteration: 0,
  // 新規: アクション単位の一意シリアル。iteration ループとは独立し連番付与。
  action_serial: 0,
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

// Runtime helpers to discriminate prompt formats
export const isPromptEntryArray = (p: unknown): p is PromptEntryConfig[] =>
  Array.isArray(p) && p.every(e => typeof e === "object" && !!e && "name" in e && "trigger" in e);
export const isLegacyPromptObject = (p: unknown): p is PromptConfig =>
  typeof p === "object" && !!p && !Array.isArray(p) && (p as any).global && (p as any).states;

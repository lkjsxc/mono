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

const LlmConfigSchema = z.object({
  endpoint: z.string().min(1),
  model: z.string().min(1),
  context_window: z.number().int().positive().optional(),
  optimization: LlmOptimizationModesSchema.optional(),
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
    prompts: z.any().optional(),
    states: AgentStatesSchema.optional(),
    paging_limit: PagingLimitSchema.optional(),
    iteration_limit: IterationLimitSchema,
  }),
});

export type AgentConfig = z.infer<typeof AgentConfigSchema>;

export const DEFAULT_MEMORY = {
  state: "analyzing",
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

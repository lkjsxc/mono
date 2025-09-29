import type { AgentConfig } from "../config/types.js";
import type { AgentMemorySnapshot } from "../domain/types.js";
import { toTagArray } from "../domain/tags.js";

export interface PromptBuildResult {
  readonly prompt: string;
  readonly estimatedTokens: number;
}

interface RoleContext {
  readonly roleId: string;
  readonly identity?: string;
  readonly purpose?: string;
  readonly knowledgeDomains: string[];
}

const estimateTokens = (text: string): number => Math.ceil(text.length / 4);

const DEFAULT_ALLOWED_STATES = ["analyzing", "creating", "organizing", "synthesizing", "evolving"] as const;
const DEFAULT_ALLOWED_ACTIONS = [
  "working_memory_add",
  "working_memory_remove",
  "storage_save",
  "storage_load",
  "storage_search",
] as const;

const ACTION_SYNONYMS: Record<string, string> = {
  memory_add: "working_memory_add",
  memory_remove: "working_memory_remove",
};

const SUPPORTED_ACTIONS = new Set<string>(DEFAULT_ALLOWED_ACTIONS);

const canonicalizeActionName = (name: string): string | undefined => {
  const trimmed = name.trim();
  if (!trimmed) return undefined;
  const lowered = trimmed.toLowerCase();
  const candidate = ACTION_SYNONYMS[lowered] ?? lowered;
  return SUPPORTED_ACTIONS.has(candidate) ? candidate : undefined;
};

const canonicalizeActionList = (values: unknown): string[] => {
  if (!Array.isArray(values) || values.length === 0) {
    return [...DEFAULT_ALLOWED_ACTIONS];
  }
  const result: string[] = [];
  for (const raw of values) {
    if (typeof raw !== "string") continue;
    const canonical = canonicalizeActionName(raw);
    if (canonical && !result.includes(canonical)) {
      result.push(canonical);
    }
  }
  return result.length > 0 ? result : [...DEFAULT_ALLOWED_ACTIONS];
};

const canonicalizeStateList = (values: unknown): string[] => {
  if (!Array.isArray(values) || values.length === 0) {
    return [...DEFAULT_ALLOWED_STATES];
  }
  const result: string[] = [];
  for (const raw of values) {
    if (typeof raw !== "string") continue;
    const value = raw.trim();
    if (!value) continue;
    const normalized = value.toLowerCase();
    if (!result.includes(normalized)) {
      result.push(normalized);
    }
  }
  return result.length > 0 ? result : [...DEFAULT_ALLOWED_STATES];
};

const renderTemplate = (template: string, context: Record<string, string>): string =>
  template.replace(/\{([^}]+)\}/g, (_, key: string) => context[key] ?? "");

const getUniversalFoundation = (config: AgentConfig): any =>
  (config.agent.prompts as any)?.universal_foundation ?? {};

const extractRoleContext = (config: AgentConfig): RoleContext => {
  const roleId = config.agent.roles.active_role;
  const roleConfig = config.agent.roles.available_roles[roleId] ?? {};
  const knowledgeDomainsRaw = roleConfig.knowledge_domains;
  const knowledgeDomains = Array.isArray(knowledgeDomainsRaw)
    ? knowledgeDomainsRaw.map(String)
    : knowledgeDomainsRaw
    ? toTagArray(knowledgeDomainsRaw)
    : [];
  return {
    roleId,
    identity: roleConfig.identity,
    purpose: roleConfig.creative_focus,
    knowledgeDomains,
  };
};

const buildRoleFoundation = (config: AgentConfig, role: RoleContext): string => {
  const foundation = getUniversalFoundation(config);
  const identityTemplate = typeof foundation?.identity_core === "string" ? foundation.identity_core : undefined;
  const context = {
    role_identity: role.identity ?? role.roleId,
    role_purpose: role.purpose ?? "",
    knowledge_domains: role.knowledgeDomains.join(", ") || "cross-domain synthesis",
  };

  const domains = role.knowledgeDomains.length > 0 ? role.knowledgeDomains.join(", ") : "cross-domain synthesis";
  return [
    "=== AGENT IDENTITY ===",
    identityTemplate ? renderTemplate(identityTemplate, context) : `You are ${role.roleId}${role.identity ? ` - ${role.identity}` : ""}`,
    role.purpose ? `Purpose: ${role.purpose}` : undefined,
    `Expertise Domains: ${domains}`,
    "Context Capacity: 128,000 tokens (maximize utilization)",
    "Evolution: Continuously self-improving through each interaction",
    "",
    ...(foundation?.memory_model && typeof foundation.memory_model === "object"
      ? [
          "MEMORY MODEL:",
          ...Object.entries(foundation.memory_model).map(
            ([label, description]) =>
              `• ${label.replace(/_/g, " ")}: ${String(description)}`,
          ),
          "",
        ]
      : [
          "MEMORY ARCHITECTURE:",
          "• Working Memory: Active context for immediate work (high priority)",
          "• Storage: Unlimited knowledge base (all accumulated wisdom)",
          "• Synthesis Engine: Cross-domain connection and insight generation",
          "• Enhancement System: Continuous learning and capability evolution",
          "",
        ]),
    "OPERATIONAL PRINCIPLES:",
    ...(foundation?.interaction_principles && typeof foundation.interaction_principles === "object"
      ? Object.entries(foundation.interaction_principles).map(
          ([label, description]) => `• ${label.replace(/_/g, " ")}: ${String(description)}`,
        )
      : [
          "• Depth Over Breadth: Every response demonstrates profound mastery",
          "• Creative Synthesis: Generate unprecedented insights by connecting knowledge",
          "• Progressive Enhancement: Each interaction enriches your capabilities",
          "• Maximum Context Usage: Leverage full 128K tokens for rich understanding",
          "• Role Mastery: Embody complete expertise in your specialized domain",
        ]),
  ]
    .filter(Boolean)
    .join("\n");
};

const buildKnowledgeSection = (memory: AgentMemorySnapshot): string => {
  const entries = Object.entries(memory.storage.entries);
  if (entries.length === 0) {
    return "Knowledge base is being initialized. Ready to accumulate unlimited wisdom.";
  }
  return entries
    .slice(0, 100)
    .map(([tags, value]) => `[${tags}]: ${value}`)
    .join("\n");
};

const buildWorkingContext = (memory: AgentMemorySnapshot): string => {
  const entries = Object.entries(memory.workingMemory.entries);
  if (entries.length === 0) {
    return "Fresh start - ready to begin creating and organizing knowledge.";
  }
  return entries.map(([tags, value]) => `[${tags}]: ${value}`).join("\n");
};

const fallbackGuidance = (state: string): string =>
  `ADAPTIVE MODE (${state}): Assess the situation and determine the most appropriate approach. Consider analysis, creation, organization, synthesis, and evolution as potential pathways forward.`;

const buildStateGuidance = (config: AgentConfig, state: string): string => {
  const prompts: any = config.agent.prompts ?? {};
  const roleId = config.agent.roles.active_role;
  const roleSpecific = prompts?.role_specific?.[roleId]?.[state];
  if (typeof roleSpecific === "string" && roleSpecific.length > 0) {
    return roleSpecific;
  }

  switch (state) {
    case "analyzing":
      return "ANALYZING MODE: Deeply examine the current situation. Identify the most impactful next action and summarize insights.";
    case "creating":
      return "CREATING MODE: Generate high-quality content that embodies your role expertise. Push creative boundaries while remaining coherent.";
    case "organizing":
      return "ORGANIZING MODE: Structure knowledge for maximum accessibility. Build taxonomies, cross-references, and retrieval cues.";
    case "synthesizing":
      return "SYNTHESIZING MODE: Connect disparate knowledge domains to generate novel insights and relationships.";
    case "evolving":
      return "EVOLVING MODE: Reflect on capabilities and design enhancements that amplify effectiveness.";
    default:
      return fallbackGuidance(state);
  }
};

const buildCurrentStateSection = (state: string): string =>
  ["=== CURRENT STATE ===", state || "analyzing"].join("\n");

const buildOutputFormatSection = (config: AgentConfig): string => {
  const foundation = getUniversalFoundation(config);
  const format = foundation?.output_format ?? {};
  const stateTag = typeof format?.state_tag === "string" ? format.state_tag : "state";
  const actionWrapper = typeof format?.action_wrapper === "string" ? format.action_wrapper : "action";
  const actionFields = Array.isArray(format?.action_fields) && format.action_fields.length > 0
    ? format.action_fields.map(String)
    : ["type", "tags", "value"];
  const allowedStates = canonicalizeStateList(format?.states);
  const allowedActions = canonicalizeActionList(format?.actions);
  const allowedActionSet = new Set(allowedActions);
  const valuePolicy = typeof format?.value_policy === "string" ? format.value_policy : undefined;

  const xmlExample = [
    "<agent>",
    `  <${stateTag}>[next_state]</${stateTag}>`,
    `  <${actionWrapper}>`,
  ...actionFields.map((field: string) => `    <${field}>[${field}_value]</${field}>`),
    `  </${actionWrapper}>`,
    "</agent>",
  ].join("\n");

  const rules: string[] = [
    "Rules:",
    "- Do not include markdown, explanations, or additional text outside the <agent> root element.",
    "- Set <state> to the exact next state you will enter.",
    `- Allowed states: ${allowedStates.join(", ")}`,
    `- Allowed action types: ${allowedActions.join(", ")}`,
    "- Tags must be comma-separated without extraneous spaces or commentary.",
    "- Ensure the XML is well-formed and valid UTF-8.",
  ];

  if (valuePolicy) {
    rules.splice(1, 0, valuePolicy);
  }

  const actionsRequiringValue = ["working_memory_add", "storage_save", "storage_search"].filter((action) =>
    allowedActionSet.has(action),
  );
  const actionsWithOptionalValue = ["working_memory_remove", "storage_load"].filter((action) =>
    allowedActionSet.has(action),
  );
  rules.push(
    `- Actions that require <value>: ${actionsRequiringValue.length > 0 ? actionsRequiringValue.join(", ") : "none"}`,
    `- Actions that use an empty <value>: ${actionsWithOptionalValue.length > 0 ? actionsWithOptionalValue.join(", ") : "none"}`,
  );

  return [
    "=== OUTPUT FORMAT ===",
    "Respond with EXACTLY the following XML structure:",
    xmlExample,
    "",
    ...rules,
  ].join("\n");
};

export const buildPrompt = (
  config: AgentConfig,
  memory: AgentMemorySnapshot,
  state: string,
): PromptBuildResult => {
  const role = extractRoleContext(config);
  const sections: string[] = [];

  const foundation = buildRoleFoundation(config, role);
  sections.push(foundation);

  const knowledge = buildKnowledgeSection(memory);
  sections.push("\n\n=== KNOWLEDGE BASE ===\n" + knowledge);

  const workingContext = buildWorkingContext(memory);
  sections.push("\n\n=== CURRENT CONTEXT ===\n" + workingContext);

  const guidance = buildStateGuidance(config, state);
  sections.push("\n\n=== CURRENT OBJECTIVE ===\n" + guidance);

  const currentStateSection = buildCurrentStateSection(state);
  sections.push("\n\n" + currentStateSection);

  const outputFormatSection = buildOutputFormatSection(config);
  sections.push("\n\n" + outputFormatSection);

  sections.push(
    "\n\nRespond with your next action in the specified XML format, maximizing insight and creative depth.",
  );

  const prompt = sections.join("\n");
  return {
    prompt,
    estimatedTokens: estimateTokens(prompt),
  };
};

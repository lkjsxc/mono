import type { AgentConfig } from "../config/types.js";
import { postJson } from "../io/http.js";
import { selectParameters } from "../prompt/qwen.js";

export interface ChatMessage {
  readonly role: "user" | "assistant" | "system";
  readonly content: string;
}

export interface ChatCompletionRequest {
  readonly model: string;
  readonly messages: readonly ChatMessage[];
  readonly temperature?: number;
  readonly top_p?: number;
  readonly top_k?: number;
  readonly min_p?: number;
  readonly max_tokens?: number;
}

export interface ChatCompletionChoice {
  readonly message?: ChatMessage;
  readonly delta?: ChatMessage;
  readonly text?: string;
}

export interface ChatCompletionResponse {
  readonly choices?: readonly ChatCompletionChoice[];
  readonly content?: string;
}

const DISABLE_OFFLINE_VALUES = new Set(["1", "true", "yes"]);

const shouldDisableOfflineFallback = (): boolean => {
  const flag = process.env.LKJAGENT_DISABLE_OFFLINE_FALLBACK;
  if (!flag) return false;
  return DISABLE_OFFLINE_VALUES.has(flag.toLowerCase());
};

const escapeXml = (value: string): string =>
  value
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&apos;");

const summarizePrompt = (prompt: string): string => {
  const normalized = prompt.replace(/\s+/g, " ").trim();
  if (normalized.length <= 180) return normalized;
  return `${normalized.slice(0, 177)}...`;
};

const buildOfflineResponse = (state: string, prompt: string, error: unknown): ChatCompletionResponse => {
  const message = error instanceof Error ? error.message : String(error);
  const safeState = escapeXml(state || "analyzing");
  const safeMessage = escapeXml(message);
  const safeSummary = escapeXml(summarizePrompt(prompt));

  const content =
    `<agent><state>${safeState}</state><action>` +
    `<type>working_memory_add</type>` +
    `<tags>system,offline</tags>` +
    `<value>Offline fallback engaged: ${safeMessage}. Prompt summary: ${safeSummary}</value>` +
    `</action></agent>`;

  return { content };
};

export const buildRequestPayload = (
  config: AgentConfig,
  prompt: string,
  state: string,
): ChatCompletionRequest => {
  const parameters = selectParameters(config, state);
  return {
    model: config.llm.model,
    messages: [
      {
        role: "user",
        content: prompt,
      },
    ],
    ...parameters,
  };
};

export const requestCompletion = async (
  config: AgentConfig,
  prompt: string,
  state: string,
): Promise<ChatCompletionResponse> => {
  const payload = buildRequestPayload(config, prompt, state);
  try {
    return await postJson<ChatCompletionResponse>(config.llm.endpoint, payload, {
      headers: config.llm.optimization ? { "x-agent-state": state } : undefined,
    });
  } catch (error) {
    if (shouldDisableOfflineFallback()) {
      throw error;
    }
    console.warn("[lkjagent] offline LLM fallback engaged", error);
    return buildOfflineResponse(state, prompt, error);
  }
};

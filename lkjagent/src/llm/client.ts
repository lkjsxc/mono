import type { AgentConfig } from "../config/types.js";
import { appendToFile } from "../io/file.js";
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

type LogDirection = "OUTBOUND" | "INBOUND" | "ERROR";

const DEFAULT_LOG_PATH = "data/log.txt";

const resolveLoggingPath = (config: AgentConfig): string | undefined => {
  const logging = (config.llm as { logging?: { enabled?: boolean; file?: string } }).logging;
  if (!logging?.enabled) {
    return undefined;
  }
  const candidate = typeof logging.file === "string" && logging.file.trim().length > 0
    ? logging.file.trim()
    : DEFAULT_LOG_PATH;
  return candidate;
};

const formatLogEntry = (direction: LogDirection, state: string, payload: string): string => {
  const timestamp = new Date().toISOString();
  const normalizedState = state && state.length > 0 ? state : "analyzing";
  return `${timestamp} [${direction}] state=${normalizedState}\n${payload}\n\n`;
};

const appendLogEntry = async (
  filePath: string | undefined,
  direction: LogDirection,
  state: string,
  payload: string,
): Promise<void> => {
  if (!filePath) return;
  try {
    await appendToFile(filePath, formatLogEntry(direction, state, payload));
  } catch (error) {
    console.warn(`[lkjagent] failed to write LLM ${direction.toLowerCase()} log to ${filePath}`, error);
  }
};

const buildOfflineResponse = (state: string, prompt: string, error: unknown): ChatCompletionResponse => {
  const message = error instanceof Error ? error.message : String(error);
  const safeState = escapeXml(state || "analyzing");
  const safeMessage = escapeXml(message);
  const safeSummary = escapeXml(summarizePrompt(prompt));

  // interpreter が期待する構造: <agent><state>...</state><actions><action>...</action></actions></agent>
  const content =
    `<agent><state>${safeState}</state><actions><action>` +
    `<type>working_memory_add</type>` +
    `<tags>system,offline</tags>` +
    `<value>Offline fallback engaged: ${safeMessage}. Prompt summary: ${safeSummary}</value>` +
    `</action></actions></agent>`;

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
  const logPath = resolveLoggingPath(config);
  await appendLogEntry(logPath, "OUTBOUND", state, prompt);
  try {
    const response = await postJson<ChatCompletionResponse>(config.llm.endpoint, payload, {
      headers: config.llm.optimization ? { "x-agent-state": state } : undefined,
    });
    await appendLogEntry(logPath, "INBOUND", state, JSON.stringify(response, null, 2));
    return response;
  } catch (error) {
    if (shouldDisableOfflineFallback()) {
      const message = error instanceof Error ? error.message : String(error);
      await appendLogEntry(logPath, "ERROR", state, message);
      throw error;
    }
    console.warn("[lkjagent] offline LLM fallback engaged", error);
    const fallback = buildOfflineResponse(state, prompt, error);
    await appendLogEntry(logPath, "INBOUND", state, JSON.stringify(fallback, null, 2));
    return fallback;
  }
};

import { XMLParser } from "fast-xml-parser";
import type { AgentAction } from "../domain/types.js";

const parser = new XMLParser({
  ignoreAttributes: false,
  preserveOrder: false,
  trimValues: true,
});

const ACTION_ALIASES: Record<string, string> = {
  memory_add: "working_memory_add",
  memory_remove: "working_memory_remove",
};

const coerceString = (value: unknown): string => {
  if (value === null || value === undefined) return "";
  if (typeof value === "string") return value;
  if (typeof value === "number" || typeof value === "boolean") return String(value);
  return JSON.stringify(value);
};

const extractRoot = (document: any): any => {
  const candidate = document?.agent ?? document?.response ?? (() => {
    const firstKey = Object.keys(document ?? {})[0];
    return document?.[firstKey];
  })();
  return normalizeNode(candidate);
};

const normalizeNode = (node: any): any => (Array.isArray(node) ? node[0] : node);

const parseAction = (raw: any): AgentAction => {
  const rawAction = normalizeNode(raw);
  const typeRaw = coerceString(rawAction?.type).trim();
  const type = ACTION_ALIASES[typeRaw.toLowerCase()] ?? typeRaw;
  const tags = coerceString(rawAction?.tags).trim();
  const value = coerceString(rawAction?.value).trim();

  switch (type) {
    case "working_memory_add":
      if (!value) {
        throw new Error("working_memory_add requires a value");
      }
      return { type, tags, value };
    case "working_memory_remove":
      return { type, tags };
    case "storage_save":
      if (!value) {
        throw new Error("storage_save requires a value");
      }
      return { type, tags, value };
    case "storage_load":
      return { type, tags };
    case "storage_search":
      return { type, tags, value };
    default:
      throw new Error(`Unsupported action type '${type}' in agent response`);
  }
};

export interface ParsedAgentResponse {
  readonly state: string;
  readonly actions: AgentAction[];
}

export const interpretAgentXml = (xml: string): ParsedAgentResponse => {
  const document = parser.parse(xml);
  if (!document) {
    throw new Error("Failed to parse agent XML response");
  }

  const root = extractRoot(document);
  const state = coerceString(root?.state ?? root?.next_state ?? root?.nextState).trim();
  if (!state) {
    throw new Error("Agent response did not specify next state");
  }

  const actionsNode = root?.actions ?? root?.action ?? root?.agent?.actions ?? root?.agent?.action;
  if (!actionsNode) {
    throw new Error("Agent response did not include any action nodes");
  }

  const normalized = normalizeNode(actionsNode);
  let candidateNodes: unknown[];

  if (Array.isArray(normalized?.action)) {
    candidateNodes = normalized.action as unknown[];
  } else if (Array.isArray(normalized)) {
    candidateNodes = normalized as unknown[];
  } else if (normalized?.action !== undefined) {
    candidateNodes = [normalized.action];
  } else {
    candidateNodes = [normalized];
  }

  const actions = candidateNodes
    .map((candidate: unknown) => {
      try {
        return parseAction(candidate);
      } catch (error) {
        throw error instanceof Error
          ? error
          : new Error(`Failed to parse action: ${String(error)}`);
      }
    })
    .filter((action: AgentAction | undefined): action is AgentAction => Boolean(action));

  if (actions.length === 0) {
    throw new Error("Agent response did not include any parsable actions");
  }

  return { state, actions };
};

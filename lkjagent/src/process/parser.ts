import type { ChatCompletionResponse } from "../llm/client.js";

const THINK_CLOSING_TAG = "</think>";

const extractChoiceContent = (response: ChatCompletionResponse): string | undefined => {
  const choice = response.choices?.[0];
  if (!choice) return response.content;
  if (choice.message?.content) return choice.message.content;
  if (choice.delta?.content) return choice.delta.content;
  if (choice.text) return choice.text;
  return response.content;
};

const stripThink = (content: string): string => {
  const index = content.indexOf(THINK_CLOSING_TAG);
  if (index < 0) return content;
  return content.slice(index + THINK_CLOSING_TAG.length);
};

export const extractAgentXml = (response: ChatCompletionResponse): string => {
  const content = extractChoiceContent(response);
  if (!content || content.trim().length === 0) {
    throw new Error("LLM response did not include any content");
  }
  const stripped = stripThink(content);
  return stripped.trim();
};

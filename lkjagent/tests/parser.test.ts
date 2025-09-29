import { describe, expect, it } from "vitest";
import type { ChatCompletionResponse } from "../src/llm/client.js";
import { extractAgentXml } from "../src/process/parser.js";

const buildResponse = (data: Partial<ChatCompletionResponse>): ChatCompletionResponse => ({
  choices: [],
  ...data,
});

describe("extractAgentXml", () => {
  it("extracts XML content from choice message", () => {
    const response = buildResponse({
      choices: [
        {
          message: { role: "assistant", content: "<agent><state>creating</state></agent>" },
        },
      ],
    });

    const xml = extractAgentXml(response);
    expect(xml).toBe("<agent><state>creating</state></agent>");
  });

  it("falls back to top-level content when no choices are present", () => {
    const response = buildResponse({ content: "<agent><state>analyzing</state></agent>" });
    const xml = extractAgentXml(response);
    expect(xml).toBe("<agent><state>analyzing</state></agent>");
  });

  it("strips any think tags before returning the XML", () => {
    const response = buildResponse({
      choices: [
        {
          message: {
            role: "assistant",
            content: "<think>Internal reflection</think><agent><state>organizing</state></agent>",
          },
        },
      ],
    });
    const xml = extractAgentXml(response);
    expect(xml).toBe("<agent><state>organizing</state></agent>");
  });
});

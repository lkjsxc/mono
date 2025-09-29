import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import type { AgentConfig } from "../src/config/types.js";
import { extractAgentXml } from "../src/process/parser.js";
import { interpretAgentXml } from "../src/process/interpreter.js";

vi.mock("../src/io/http.js", () => ({
  postJson: vi.fn(),
}));

type MockedHttp = {
  postJson: ReturnType<typeof vi.fn>;
};

const loadHttp = async (): Promise<MockedHttp> =>
  (await import("../src/io/http.js")) as unknown as MockedHttp;

const loadClient = async () => import("../src/llm/client.js");

const config: AgentConfig = {
  llm: {
    endpoint: "http://example.com",
    model: "mock-model",
  },
  agent: {
    roles: {
      active_role: "tester",
      available_roles: {
        tester: {
          identity: "Tester",
          creative_focus: "Validate",
          knowledge_domains: ["qa"],
        },
      },
    },
    prompts: {},
    memory_system: {},
    paging_limit: { enable: false },
    iteration_limit: { enable: false },
  },
};

describe("requestCompletion", () => {
  let originalFlag: string | undefined;

  beforeEach(async () => {
    const { postJson } = await loadHttp();
    postJson.mockReset();
    originalFlag = process.env.LKJAGENT_DISABLE_OFFLINE_FALLBACK;
    delete process.env.LKJAGENT_DISABLE_OFFLINE_FALLBACK;
  });

  afterEach(() => {
    process.env.LKJAGENT_DISABLE_OFFLINE_FALLBACK = originalFlag;
  });

  it("returns a structured offline response when HTTP fails", async () => {
    const { postJson } = await loadHttp();
    postJson.mockRejectedValue(new Error("connect ECONNREFUSED"));

    const warn = vi.spyOn(console, "warn").mockImplementation(() => {});

    const { requestCompletion } = await loadClient();
    const response = await requestCompletion(config, "Prompt body", "creating");

    expect(response.content).toContain("<tags>system,offline</tags>");
    expect(response.content).toContain("Offline fallback engaged");
    expect(response.content).toContain("connect ECONNREFUSED");
    expect(warn).toHaveBeenCalled();

    const xml = extractAgentXml(response);
    const parsed = interpretAgentXml(xml);
    expect(parsed.state).toBe("creating");
    expect(parsed.action.type).toBe("working_memory_add");
    if (parsed.action.type !== "working_memory_add") {
      throw new Error("expected working_memory_add fallback action");
    }
  expect(parsed.action.tags).toBe("system,offline");
    expect(parsed.action.value).toContain("Offline fallback engaged");

    warn.mockRestore();
  });

  it("throws when offline fallback is disabled", async () => {
    const { postJson } = await loadHttp();
    postJson.mockRejectedValue(new Error("service down"));

    process.env.LKJAGENT_DISABLE_OFFLINE_FALLBACK = "true";

    const { requestCompletion } = await loadClient();
    await expect(requestCompletion(config, "Prompt", "analyzing")).rejects.toThrow(
      "service down",
    );
  });
});

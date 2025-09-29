import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import type { AgentConfig } from "../src/config/types.js";
import { extractAgentXml } from "../src/process/parser.js";
import { interpretAgentXml } from "../src/process/interpreter.js";

vi.mock("../src/io/http.js", () => ({
  postJson: vi.fn(),
}));

vi.mock("../src/io/file.js", () => ({
  appendToFile: vi.fn(),
}));

type MockedHttp = {
  postJson: ReturnType<typeof vi.fn>;
};

const loadHttp = async (): Promise<MockedHttp> =>
  (await import("../src/io/http.js")) as unknown as MockedHttp;

type MockedFile = {
  appendToFile: ReturnType<typeof vi.fn>;
};

const loadFile = async (): Promise<MockedFile> =>
  (await import("../src/io/file.js")) as unknown as MockedFile;

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
    const { appendToFile } = await loadFile();
    appendToFile.mockReset();
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
    expect(parsed.actions).toHaveLength(1);
    const [action] = parsed.actions;
    expect(action.type).toBe("working_memory_add");
    if (action.type !== "working_memory_add") {
      throw new Error("expected working_memory_add fallback action");
    }
    expect(action.tags).toBe("system,offline");
    expect(action.value).toContain("Offline fallback engaged");

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

  it("logs prompts and responses when logging is enabled", async () => {
    const { postJson } = await loadHttp();
    postJson.mockResolvedValue({
      content: "<agent><state>creating</state><action><type>working_memory_add</type><tags>story,idea</tags><value>Draft</value></action></agent>",
    });

    const { appendToFile } = await loadFile();
    appendToFile.mockResolvedValue(undefined);

    const loggingConfig: AgentConfig = {
      ...config,
      llm: {
        ...config.llm,
        logging: {
          enabled: true,
          file: "logs/log.txt",
        },
      },
    };

    const { requestCompletion } = await loadClient();
    await requestCompletion(loggingConfig, "Prompt body", "creating");

    expect(appendToFile).toHaveBeenCalledTimes(2);
    expect(appendToFile.mock.calls[0][0]).toBe("logs/log.txt");
    expect(appendToFile.mock.calls[0][1]).toContain("[OUTBOUND]");
    expect(appendToFile.mock.calls[0][1]).toContain("Prompt body");
    expect(appendToFile.mock.calls[1][0]).toBe("logs/log.txt");
    expect(appendToFile.mock.calls[1][1]).toContain("[INBOUND]");
    expect(appendToFile.mock.calls[1][1]).toContain("<agent>");
  });
});

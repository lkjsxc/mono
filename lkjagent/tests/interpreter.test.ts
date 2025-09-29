import { describe, expect, it } from "vitest";
import { interpretAgentXml } from "../src/process/interpreter.js";

const SAMPLE_XML = `
<agent>
  <state>creating</state>
  <action>
    <type>working_memory_add</type>
    <tags>alpha, beta</tags>
    <value>Forge a new tale.</value>
  </action>
</agent>`;

describe("interpreter", () => {
  it("parses agent XML into state and action", () => {
    const result = interpretAgentXml(SAMPLE_XML);
    expect(result.state).toBe("creating");
    expect(result.action.type).toBe("working_memory_add");
    expect(result.action.tags).toBe("alpha, beta");
    expect(result.action).toHaveProperty("value", "Forge a new tale.");
  });

  it("normalizes legacy memory_* action names", () => {
    const legacyXml = `
<agent>
  <state>organizing</state>
  <action>
    <type>memory_remove</type>
    <tags>draft,chapter_1</tags>
  </action>
</agent>`;

    const result = interpretAgentXml(legacyXml);
    expect(result.state).toBe("organizing");
    expect(result.action.type).toBe("working_memory_remove");
    expect(result.action).not.toHaveProperty("value");
  });
});

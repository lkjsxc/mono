import { describe, expect, it } from "vitest";
import { interpretAgentXml } from "../src/process/interpreter.js";

const SAMPLE_XML = `
<agent>
  <state>creating</state>
  <actions>
    <action>
      <type>working_memory_add</type>
      <tags>alpha, beta</tags>
      <value>Forge a new tale.</value>
    </action>
    <action>
      <type>storage_save</type>
      <tags>alpha, beta</tags>
      <value>Forge a new tale.</value>
    </action>
  </actions>
</agent>`;

describe("interpreter", () => {
  it("parses agent XML with multiple actions", () => {
    const result = interpretAgentXml(SAMPLE_XML);
    expect(result.state).toBe("creating");
    expect(result.actions).toHaveLength(2);
    expect(result.actions[0].type).toBe("working_memory_add");
    expect(result.actions[0].tags).toBe("alpha, beta");
    expect(result.actions[0]).toHaveProperty("value", "Forge a new tale.");
    expect(result.actions[1].type).toBe("storage_save");
  });

  it("handles legacy single action envelopes", () => {
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
    expect(result.actions).toHaveLength(1);
    expect(result.actions[0].type).toBe("working_memory_remove");
    expect(result.actions[0]).not.toHaveProperty("value");
  });
});

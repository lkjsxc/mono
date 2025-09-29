import { describe, expect, it } from "vitest";
import { normalizeTags, searchTagsInEntry } from "../src/domain/tags.js";

describe("tags", () => {
  it("normalizes tags by trimming, deduplicating, and sorting", () => {
    const result = normalizeTags(" beta , alpha , beta ");
    expect(result).toBe("alpha,beta");
  });

  it("matches subset of tags within entry", () => {
    const match = searchTagsInEntry("alpha", "alpha,beta,gamma");
    expect(match).toBe(true);
    const miss = searchTagsInEntry("delta", "alpha,beta");
    expect(miss).toBe(false);
  });
});

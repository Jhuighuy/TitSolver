import { expect, test } from "vitest";
import { sum } from "./sum.js";

// Temporary file, remove when we have actual tests.
test("adds 1 + 2 to equal 3", () => {
  expect(sum(1, 2)).toBe(3);
});

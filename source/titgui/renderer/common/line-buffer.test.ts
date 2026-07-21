/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { describe, expect, it } from "vitest";

import { LineBuffer } from "~/renderer/common/line-buffer";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("LineBuffer", () => {
  it("yields lines completed by a chunk", () => {
    const buffer = new LineBuffer();
    expect(buffer.append("one\ntwo\n")).toEqual(["one", "two"]);
    expect(buffer.pending).toBe("");
  });

  it("keeps a trailing partial line until completed", () => {
    const buffer = new LineBuffer();
    expect(buffer.append("par")).toEqual([]);
    expect(buffer.pending).toBe("par");
    expect(buffer.append("tial\nrest")).toEqual(["partial"]);
    expect(buffer.pending).toBe("rest");
  });

  it("flushes the pending partial line", () => {
    const buffer = new LineBuffer();
    buffer.append("no newline");
    expect(buffer.flush()).toEqual(["no newline"]);
    expect(buffer.flush()).toEqual([]);
    expect(buffer.pending).toBe("");
  });

  it("preserves empty lines", () => {
    const buffer = new LineBuffer();
    expect(buffer.append("a\n\nb\n")).toEqual(["a", "", "b"]);
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

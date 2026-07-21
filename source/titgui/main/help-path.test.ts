/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { describe, expect, it } from "vitest";

import { resolveHelpPath } from "~/main/help-path";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const ROOT = "/install/manual";

describe("resolveHelpPath", () => {
  it("resolves paths inside the manual root", () => {
    expect(resolveHelpPath(ROOT, "help://manual/index.html")).toBe(
      "/install/manual/index.html",
    );
    expect(resolveHelpPath(ROOT, "help://manual/guide/intro.html")).toBe(
      "/install/manual/guide/intro.html",
    );
    expect(resolveHelpPath(ROOT, "help://manual/a%20b.html")).toBe(
      "/install/manual/a b.html",
    );
  });

  it("collapses redundant segments that stay inside the root", () => {
    expect(resolveHelpPath(ROOT, "help://manual/guide/../index.html")).toBe(
      "/install/manual/index.html",
    );
  });

  it("stays inside the root for URL-normalized dot-dot segments", () => {
    // The URL parser clamps plain and percent-encoded dot-dot segments at
    // the host root before we ever see them, so these cannot escape.
    expect(resolveHelpPath(ROOT, "help://manual/../secret.html")).toBe(
      "/install/manual/secret.html",
    );
    expect(resolveHelpPath(ROOT, "help://manual/a/../../secret.html")).toBe(
      "/install/manual/secret.html",
    );
    expect(resolveHelpPath(ROOT, "help://manual/%2e%2e/secret.html")).toBe(
      "/install/manual/secret.html",
    );
  });

  it("rejects paths escaping the manual root", () => {
    // Percent-encoded slashes hide dot-dot segments from the URL parser's
    // normalization; only the resolve-and-contain check catches these.
    expect(() =>
      resolveHelpPath(ROOT, "help://manual/a/..%2f..%2fsecret"),
    ).toThrow();
    expect(() =>
      resolveHelpPath(ROOT, "help://manual/..%2F..%2Fsecret"),
    ).toThrow();
    expect(() => resolveHelpPath(ROOT, "help://manual/..")).toThrow();
  });

  it("rejects URLs outside the manual", () => {
    expect(() => resolveHelpPath(ROOT, "https://example.com/x")).toThrow();
    expect(() => resolveHelpPath(ROOT, "help://other/index.html")).toThrow();
    expect(() => resolveHelpPath(ROOT, "help://manual/")).toThrow();
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

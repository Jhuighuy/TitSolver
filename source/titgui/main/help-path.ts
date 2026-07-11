/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import path from "node:path";

import { ensure } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const HELP_PROTOCOL = "help";
export const HELP_HOST = "manual";
export const HELP_ORIGIN = `${HELP_PROTOCOL}://${HELP_HOST}`;
export const HOME_URL = `${HELP_ORIGIN}/index.html`;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Resolve a help URL to an absolute file path inside the manual root.
 * Throws if the URL does not belong to the manual, or if the path would
 * escape the root (e.g. through embedded `..` segments).
 */
export function resolveHelpPath(rootPath: string, url: string): string {
  const { protocol, hostname, pathname } = new URL(url, HOME_URL);
  ensure(protocol === `${HELP_PROTOCOL}:`, "Not a help URL.");
  ensure(hostname === HELP_HOST, "Not a manual URL.");

  const relativePath = decodeURIComponent(pathname).replace(/^\/+/u, "").trim();
  ensure(relativePath !== "" && relativePath !== ".", "Empty help path.");

  // Resolve and verify containment; checking the raw path for `..` is not
  // enough, since segments like `a/../../b` also escape the root.
  const resolvedPath = path.resolve(rootPath, relativePath);
  const rootRelativePath = path.relative(rootPath, resolvedPath);
  ensure(
    rootRelativePath !== "" &&
      !rootRelativePath.startsWith("..") &&
      !path.isAbsolute(rootRelativePath),
    "Help path escapes the manual root.",
  );

  return resolvedPath;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

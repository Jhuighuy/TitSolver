/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import path from "node:path/posix";
import { parse as parseYaml } from "yaml";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const PAGES_DIR = "pages";
const INDEX_MD = "index.md";
const ROOT_INDEX_SOURCE_PATH = path.join(PAGES_DIR, INDEX_MD);

type Frontmatter = {
  children?: string[];
};

// Normalize a source path imported by Astro's glob loader.
function normalizeSourcePath(sourcePath: string) {
  return path.normalize(sourcePath).replace(/^[.][/]/, "");
}

// Parse YAML frontmatter and strip it from the Markdown source.
function parseFrontmatter(sourcePath: string, source: string) {
  if (!source.startsWith("---\n")) {
    return {
      frontmatter: {} satisfies Frontmatter,
      content: source,
    };
  }

  const frontmatterEnd = source.indexOf("\n---\n", 4);
  if (frontmatterEnd < 0) {
    throw new Error(`Page '${sourcePath}' has invalid YAML frontmatter.`);
  }

  const frontmatterSource = source.slice(4, frontmatterEnd);
  const frontmatter = parseYaml(frontmatterSource) ?? {};
  if (typeof frontmatter !== "object" || Array.isArray(frontmatter)) {
    throw new Error(`Page '${sourcePath}' frontmatter must be a mapping.`);
  }

  const { children } = frontmatter as Frontmatter;
  if (
    children !== undefined &&
    (!Array.isArray(children) ||
      children.some((child) => typeof child !== "string"))
  ) {
    throw new Error(
      `Page '${sourcePath}' frontmatter field 'children' must be a string array.`,
    );
  }

  return {
    frontmatter: frontmatter as Frontmatter,
    content: source.slice(frontmatterEnd + "\n---\n".length),
  };
}

// Extract the top-level heading from a manual page.
function extractTitle(sourcePath: string, source: string) {
  const prefix = "# ";
  for (const line of source.split("\n")) {
    if (line.startsWith(prefix)) return line.slice(prefix.length).trim();
  }

  throw new Error(`Page '${sourcePath}' must define a top-level heading.`);
}

// Convert a source path to a path relative to the `pages` directory.
function toPagesRelativePath(sourcePath: string) {
  return path.relative(PAGES_DIR, normalizeSourcePath(sourcePath));
}

// Convert a source path to a page path.
function toPagePath(sourcePath: string) {
  const relativePath = toPagesRelativePath(sourcePath);
  if (relativePath === INDEX_MD) return "/index.html";

  const parsedPath = path.parse(relativePath);
  const pagePath =
    parsedPath.base === INDEX_MD
      ? parsedPath.dir
      : path.join(parsedPath.dir, parsedPath.name);

  return `/${pagePath}.html`;
}

function isIndexPage(sourcePath: string) {
  return path.basename(sourcePath) === INDEX_MD;
}

function toChildSourcePaths(
  page: ManualPage,
  pageBySourcePath: ReadonlyMap<string, ManualPage>,
) {
  if (page.children.length === 0) return [];

  if (!isIndexPage(page.sourcePath)) {
    throw new Error(
      `Page '${page.sourcePath}' declares 'children', but only index pages may do that.`,
    );
  }

  return page.children.map((child) => {
    const sourceDir = path.dirname(page.sourcePath);
    const childBasePath = path.normalize(path.join(sourceDir, child));
    const childCandidates =
      path.extname(childBasePath) === ".md"
        ? [childBasePath]
        : [`${childBasePath}.md`, path.join(childBasePath, INDEX_MD)];

    const childSourcePath = childCandidates.find((candidate) =>
      pageBySourcePath.has(candidate),
    );
    if (childSourcePath === undefined) {
      throw new Error(
        `Page '${page.sourcePath}' references unknown child '${child}'.`,
      );
    }

    return childSourcePath;
  });
}

function flattenPages(
  rootPage: ManualPage,
  pageBySourcePath: Map<string, ManualPage>,
) {
  const flattened: ManualPage[] = [];
  const visiting = new Set<string>();
  const visited = new Set<string>();

  function visit(page: ManualPage, parentPath?: string) {
    if (visiting.has(page.sourcePath)) {
      throw new Error(
        `Navigation cycle detected while visiting '${page.sourcePath}'.`,
      );
    }
    if (visited.has(page.sourcePath)) {
      throw new Error(
        `Page '${page.sourcePath}' is referenced multiple times in the manual navigation tree.`,
      );
    }

    visiting.add(page.sourcePath);
    flattened.push({
      ...page,
      parentPath,
    });

    for (const childSourcePath of toChildSourcePaths(page, pageBySourcePath)) {
      const childPage = pageBySourcePath.get(childSourcePath);
      if (childPage === undefined) {
        throw new Error(`Unknown manual page '${childSourcePath}'.`);
      }

      visit(childPage, page.path);
    }

    visiting.delete(page.sourcePath);
    visited.add(page.sourcePath);
  }

  visit(rootPage);

  const unreachablePages = [...pageBySourcePath.values()]
    .filter((page) => !visited.has(page.sourcePath))
    .map((page) => page.sourcePath);
  if (unreachablePages.length > 0) {
    throw new Error(
      `Manual pages are unreachable from '${ROOT_INDEX_SOURCE_PATH}': ${unreachablePages.join(", ")}.`,
    );
  }

  return flattened;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * A manual page.
 */
export type ManualPage = {
  title: string;
  path: string;
  sourcePath: string;
  source: string;
  children: string[];
  parentPath?: string;
};

/**
 * All the manual pages.
 */
const pageRecords: ManualPage[] = Object.entries(
  import.meta.glob("./pages/**/*.md", {
    import: "default",
    eager: true,
    query: "?raw",
  }),
)
  .sort(([leftPath], [rightPath]) => leftPath.localeCompare(rightPath))
  .map(([sourcePath, sourceText]) => {
    const normalizedSourcePath = normalizeSourcePath(sourcePath);
    const { content, frontmatter } = parseFrontmatter(
      normalizedSourcePath,
      sourceText as string,
    );

    return {
      title: extractTitle(normalizedSourcePath, content),
      path: toPagePath(normalizedSourcePath),
      source: content,
      sourcePath: normalizedSourcePath,
      children: frontmatter.children ?? [],
    };
  });

const pageBySourcePath = new Map(
  pageRecords.map((page) => [page.sourcePath, page]),
);
const rootPage = pageBySourcePath.get(ROOT_INDEX_SOURCE_PATH);
if (rootPage === undefined) {
  throw new Error(`Manual root page '${ROOT_INDEX_SOURCE_PATH}' is missing.`);
}

export const manualPages: ManualPage[] = flattenPages(
  rootPage,
  pageBySourcePath,
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Find a manual page by path.
 */
function findPage(pagePath: string) {
  return manualPages.find((page) => page.path === pagePath);
}

/**
 * Get the manual page by path.
 */
export function getPage(pagePath: string) {
  const page = findPage(pagePath);
  if (page === undefined) {
    throw new Error(`Unknown manual page: '${pagePath}'.`);
  }

  return page;
}

/**
 * Find the breadcrumbs leading to a manual page.
 */
export function getBreadcrumbs(pagePath: string) {
  const breadcrumbs: ManualPage[] = [];
  let current = findPage(pagePath);
  while (current !== undefined) {
    breadcrumbs.unshift(current);
    current =
      current.parentPath === undefined
        ? undefined
        : findPage(current.parentPath);
  }
  return breadcrumbs;
}

/**
 * Find the previous and next manual pages.
 */
export function getPrevNext(pagePath: string) {
  const index = manualPages.findIndex((page) => page.path === pagePath);
  return {
    prev: index > 0 ? manualPages[index - 1] : undefined,
    next:
      0 <= index && index < manualPages.length - 1
        ? manualPages[index + 1]
        : undefined,
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Convert a target path to a path relative to the current page.
 */
export function getRelativeHref(fromPath: string, toPath: string) {
  const relativePath = path.relative(path.dirname(fromPath), toPath);
  return relativePath === "" ? path.basename(toPath) : relativePath;
}

/**
 * Convert an Astro URL to a manual page path.
 */
export function getPagePathFromUrl(url: URL) {
  const pathname = url.pathname;
  if (pathname === "/") return "/index.html";

  const normalizedPath = path.normalize(pathname);
  return path.extname(normalizedPath) === ".html"
    ? normalizedPath
    : `${normalizedPath}.html`;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

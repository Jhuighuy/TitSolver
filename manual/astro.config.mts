// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Part of BlueTit Solver, under the MIT License.
// See /LICENSE.md for license information. SPDX-License-Identifier: MIT
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

import { unified } from "@astrojs/markdown-remark";
import starlight from "@astrojs/starlight";
import { defineConfig } from "astro/config";
import rehypeKatex from "rehype-katex";
import remarkMath from "remark-math";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export default defineConfig({
  site: "http://localhost",
  output: "static",
  build: { format: "file" },
  markdown: {
    processor: unified({
      remarkPlugins: [remarkMath],
      rehypePlugins: [rehypeKatex],
    }),
  },
  integrations: [
    starlight({
      title: "BlueTit Solver",
      customCss: ["./src/custom.css"],
      sidebar: [
        { label: "BlueTit Solver User Manual", link: "/" },
        {
          label: "User Guide",
          items: [{ autogenerate: { directory: "01-user-guide" } }],
        },
        {
          label: "Theory Guide",
          items: [
            {
              label: "SPH Foundation",
              items: [
                {
                  autogenerate: {
                    directory: "02-theory-guide/01-sph-foundation",
                  },
                },
              ],
            },
            {
              label: "Free-Surface SPH",
              items: [
                {
                  autogenerate: {
                    directory: "02-theory-guide/02-free-surface-sph",
                  },
                },
              ],
            },
          ],
        },
        {
          label: "Scripting Guide",
          items: [{ autogenerate: { directory: "03-scripting-guide" } }],
        },
      ],
    }),
  ],
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

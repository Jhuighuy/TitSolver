/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

document.addEventListener("DOMContentLoaded", () => {
  // Get the table of contents.
  const toc = document.querySelector(".toc");
  if (!toc) {
    return;
  }

  // "Unindent" the table of contents.
  const tocItems = toc.querySelector("ul");
  if (tocItems) {
    const tocInnerItems = tocItems.querySelector("li ul");
    if (tocInnerItems) {
      toc.replaceChild(tocInnerItems, tocItems);
    }
  }

  // Move the table of contents after the title.
  const titleHeading = document.querySelector(".document h1");
  if (titleHeading) {
    titleHeading.insertAdjacentElement("afterend", toc);
  }
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

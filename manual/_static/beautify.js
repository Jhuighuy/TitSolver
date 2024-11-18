/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Reposition the table of contents below the title.
 *
 * In Sphinx, the table of contents is placed in the sidebar. This function
 * moves the table of contents to the bottom of the page, after the title.
 */
function repositionTOC() {
  // Get the table of contents.
  const toc = document.querySelector(".toc");
  if (!toc) return;

  // Flatten nested TOC structure by moving nested items to the top level.
  const tocItems = toc.querySelector("ul");
  const tocInnerItems = tocItems?.querySelector("li ul");
  tocInnerItems && toc.replaceChild(tocInnerItems, tocItems);

  // Move the table of contents after the title.
  const titleHeading = document.querySelector(".content h1");
  titleHeading?.insertAdjacentElement("afterend", toc);
}

document.addEventListener("DOMContentLoaded", () => {
  try {
    repositionTOC();
  } catch (error) {
    console.error("Could not reposition the table of contents!", error);
  }
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * @brief Beautify equation numbers on the page.
 *
 * In Sphinx, there is no special support for equation numbers. We would
 * consider any parenthesized digits as an equation number (like `(123)`).
 * This function beautifies the equation numbers by adding a circle element
 * around the number with some additional styling.
 */
function beautifyEquationNumbers() {
  /** @param {Node} node */
  const processNode = (node) => {
    // Regular expression to match: `(123)`.
    const eqnoRegex = /\((\d+)\)/g;

    // Prettify the equation numbers.
    let match;
    let text = node.nodeValue;
    while ((match = eqnoRegex.exec(text)) !== null) {
      // Create a span element for the equation number. Class name is derived
      // from the number of digits in the equation number, so we can use
      // smaller font size for numbers with more digits.
      const span = document.createElement("span");
      const value = match[1];
      span.textContent = value;
      span.setAttribute("class", `eqno-circle digits-${value.length}`);

      // Replace the text node with the new nodes.
      const parentNode = node.parentNode;
      const beforeMatch = text.substring(0, match.index);
      parentNode.insertBefore(document.createTextNode(beforeMatch), node);
      parentNode.insertBefore(span, node);
      const afterMatch = text.substring(eqnoRegex.lastIndex);
      parentNode.insertBefore(document.createTextNode(afterMatch), node);

      // Remove the original text node.
      parentNode.removeChild(node);

      // Continue with the next match.
      text = afterMatch;
      eqnoRegex.lastIndex = 0;
      node = parentNode.lastChild;
    }
  };

  /** @param {Node} node */
  const traverseNode = (node) => {
    if (node.nodeType === Node.TEXT_NODE) {
      processNode(node);
    } else {
      node.childNodes.forEach(traverseNode);
    }
  };

  content = document.querySelector(".content");
  content && traverseNode(content);
}

document.addEventListener("DOMContentLoaded", () => {
  try {
    beautifyEquationNumbers();
  } catch (error) {
    console.error("Could not beautify equation numbers!", error);
  }
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

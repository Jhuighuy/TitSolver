/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

document.addEventListener("DOMContentLoaded", () => {
  /** @param {Node} node */
  const beautifyEquationNumbers = (node) => {
    // Regular expression to match: `(123)`.
    const regex = /\((\d+)\)/g;

    // Prettify the equation numbers.
    let match;
    let text = node.nodeValue;
    while ((match = regex.exec(text)) !== null) {
      // Create a span element for the equation number.
      const span = document.createElement("span");
      const value = match[1];
      span.textContent = value;
      span.setAttribute("class", `eqno-circle digits-${value.length}`);

      // Replace the text node with the new nodes.
      const parentNode = node.parentNode;
      const beforeMatch = text.substring(0, match.index);
      parentNode.insertBefore(document.createTextNode(beforeMatch), node);
      parentNode.insertBefore(span, node);
      const afterMatch = text.substring(regex.lastIndex);
      parentNode.insertBefore(document.createTextNode(afterMatch), node);

      // Remove the original text node.
      parentNode.removeChild(node);

      // Continue with the next match.
      text = afterMatch;
      regex.lastIndex = 0;
      node = parentNode.lastChild;
    }
  };

  /** @param {Node} node */
  const traverse = (node) => {
    if (node.nodeType === Node.TEXT_NODE) {
      beautifyEquationNumbers(node);
    } else {
      node.childNodes.forEach(traverse);
    }
  };

  traverse(document.body);
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

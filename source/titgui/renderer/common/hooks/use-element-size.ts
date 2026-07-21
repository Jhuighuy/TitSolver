/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useEffect, useState } from "react";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Observe the content size of an element. Pass the returned `ref` callback to
 * the element to observe.
 */
export function useElementSize<Element extends HTMLElement>() {
  const [element, setElement] = useState<Element | null>(null);
  const [size, setSize] = useState({ width: 0, height: 0 });

  useEffect(() => {
    if (element === null) return;

    const measure = () => {
      setSize({ width: element.clientWidth, height: element.clientHeight });
    };
    measure();

    const observer = new ResizeObserver(measure);
    observer.observe(element, { box: "content-box" });
    return () => {
      observer.disconnect();
    };
  }, [element]);

  return { ref: setElement, ...size };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Accumulates stream chunks and yields complete lines. A trailing partial
 * line is kept until the next chunk completes it (or `flush` is called).
 */
export class LineBuffer {
  private partial = "";

  /**
   * Append a chunk. Returns the lines it completed.
   */
  public append(chunk: string): string[] {
    const parts = (this.partial + chunk).split("\n");
    this.partial = parts.pop() ?? "";
    return parts;
  }

  /**
   * Yield the pending partial line, if any, and reset the buffer.
   */
  public flush(): string[] {
    if (this.partial === "") return [];
    const line = this.partial;
    this.partial = "";
    return [line];
  }

  /**
   * The pending partial line.
   */
  public get pending(): string {
    return this.partial;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

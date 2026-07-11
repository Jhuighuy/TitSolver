/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { describe, expect, it } from "vitest";

import { formatDuration, formatMemorySize } from "~/renderer/common/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("formatDuration", () => {
  it("formats durations as HH:MM:SS", () => {
    expect(formatDuration(0)).toBe("00:00:00");
    expect(formatDuration(999)).toBe("00:00:00");
    expect(formatDuration(61_000)).toBe("00:01:01");
    expect(formatDuration(3_600_000 + 23 * 60_000 + 45_000)).toBe("01:23:45");
    expect(formatDuration(100 * 3_600_000)).toBe("100:00:00");
  });
});

describe("formatMemorySize", () => {
  it("formats byte sizes with binary units", () => {
    expect(formatMemorySize(0)).toBe("0 B");
    expect(formatMemorySize(512)).toBe("512 B");
    expect(formatMemorySize(2048)).toBe("2.0 KiB");
    expect(formatMemorySize(1.5 * 1024 ** 3)).toBe("1.5 GiB");
    expect(formatMemorySize(200 * 1024 ** 2)).toBe("200 MiB");
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

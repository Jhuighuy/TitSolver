/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Vector2 } from "three";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class Polygon2 {
  public readonly points: readonly Vector2[];

  public constructor(points: readonly Vector2[]) {
    this.points = points.map((point) => point.clone());
  }

  public containsPoint(point: Vector2) {
    let inside = false;

    for (
      let i = 0, j = this.points.length - 1;
      i < this.points.length;
      j = i++
    ) {
      const pi = this.points[i];
      const pj = this.points[j];

      if (pi.y > point.y === pj.y > point.y) continue;

      const dx = pj.x - pi.x;
      const dy = pj.y - pi.y;
      if (
        (dy > 0 && (point.x - pi.x) * dy < dx * (point.y - pi.y)) ||
        (dy < 0 && (point.x - pi.x) * dy > dx * (point.y - pi.y))
      ) {
        inside = !inside;
      }
    }

    return inside;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

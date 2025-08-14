/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  type PropertyRecord,
  solverConfigSchema,
  type SolverConfig,
} from "~/solver-config";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const spec = {
  type: "record",
  fields: [
    {
      id: "debug",
      name: "Enable debug mode",
      spec: {
        type: "bool",
        default: false,
      },
    },
    {
      id: "workers",
      name: "Number of worker threads",
      spec: {
        type: "int",
        min: 1,
        max: 64,
        default: 4,
      },
    },
    {
      id: "threshold",
      name: "Threshold value",
      spec: {
        type: "real",
        min: 0,
        max: 1,
        default: 0.5,
      },
    },
    {
      id: "output",
      name: "Output file path",
      spec: {
        type: "string",
        default: "./output",
      },
    },
    {
      id: "mode",
      name: "Run mode",
      spec: {
        type: "enum",
        default: "fast",
        options: [
          { id: "fast", name: "Fast mode" },
          { id: "safe", name: "Safe mode" },
          { id: "debug", name: "Debug mode" },
        ],
      },
    },
    {
      id: "physics",
      name: "Physics settings",
      spec: {
        type: "record",
        fields: [
          {
            id: "gravity",
            name: "Gravitational acceleration",
            spec: {
              type: "real",
              default: 9.81,
            },
          },
          {
            id: "density",
            name: "Reference density",
            spec: {
              type: "real",
              min: 0.1,
              max: 1000,
              default: 1,
            },
          },
          {
            id: "config",
            name: "Physics sub-config",
            spec: {
              type: "record",
              fields: [
                {
                  id: "substeps",
                  name: "Substep count",
                  spec: {
                    type: "int",
                    default: 2,
                  },
                },
              ],
            },
          },
        ],
      },
    },
    {
      id: "tags",
      name: "String tags",
      spec: {
        type: "array",
        items: {
          type: "string",
        },
      },
    },
    {
      id: "tags2",
      name: "String tags 2",
      spec: {
        type: "array",
        items: {
          type: "string",
        },
      },
    },
    {
      id: "shape",
      name: "Active shape",
      spec: {
        type: "variant",
        default: "circle",
        options: {
          circle: {
            name: "Circle",
            spec: {
              type: "record",
              fields: [
                {
                  id: "radius",
                  name: "Circle radius",
                  spec: {
                    type: "real",
                    default: 1,
                  },
                },
              ],
            },
          },
          rect: {
            name: "Rectangle",
            spec: {
              type: "record",
              fields: [
                {
                  id: "width",
                  name: "Rectangle width",
                  spec: {
                    type: "real",
                    default: 2,
                  },
                },
                {
                  id: "height",
                  name: "Rectangle height",
                  spec: {
                    type: "real",
                    default: 1.5,
                  },
                },
              ],
            },
          },
        },
      },
    },
  ],
} as const;

let tree: PropertyRecord = {
  debug: true,
  workers: "bad value",
  threshold: 0.5,
  output: "runs/dam_breaking",
  mode: "safe",
  physics: {
    gravity: 19.62,
    density: 1,
    config: {
      substeps: "x",
    },
  },
  tags: ["baseline", "dam-break"],
  tags2: ["secondary"],
  shape: {
    _active: "rect",
    rect: {
      width: 4,
      height: 2,
    },
  },
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export async function fetch_solver_config(): Promise<SolverConfig> {
  await delay(160);
  return solverConfigSchema.parse({
    spec,
    tree: structuredClone(tree),
  });
}

export async function save_solver_config(
  nextTree: PropertyRecord,
): Promise<void> {
  await delay(120);
  tree = solverConfigSchema.parse({ spec, tree: nextTree }).tree;
}

function delay(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

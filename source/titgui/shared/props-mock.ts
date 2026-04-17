/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const MOCK_SPEC: unknown = {
  type: "record",
  fields: [
    {
      id: "simulation",
      name: "Simulation",
      spec: {
        type: "record",
        fields: [
          {
            id: "title",
            name: "Title",
            spec: { type: "string", default: "Untitled" },
          },
          {
            id: "gravity",
            name: "Gravity (m/s²)",
            spec: { type: "real", min: 0, default: 9.81 },
          },
          {
            id: "end_time",
            name: "End Time (s)",
            spec: { type: "real", min: 0 },
          },
          {
            id: "verbose",
            name: "Verbose",
            spec: { type: "bool", default: false },
          },
        ],
      },
    },
    {
      id: "time_integration",
      name: "Time Integration",
      spec: {
        type: "variant",
        default: "explicit_euler",
        options: [
          {
            id: "explicit_euler",
            name: "Explicit Euler",
            spec: {
              type: "record",
              fields: [
                {
                  id: "time_step",
                  name: "Time Step (s)",
                  spec: { type: "real", min: 0, default: 0.001 },
                },
              ],
            },
          },
          {
            id: "runge_kutta_2",
            name: "Runge-Kutta 2",
            spec: {
              type: "record",
              fields: [
                {
                  id: "time_step",
                  name: "Time Step (s)",
                  spec: { type: "real", min: 0, default: 0.001 },
                },
                {
                  id: "adaptive",
                  name: "Adaptive",
                  spec: { type: "bool", default: false },
                },
              ],
            },
          },
        ],
      },
    },
    {
      id: "kernel",
      name: "Kernel",
      spec: {
        type: "variant",
        default: "cubic_spline",
        options: [
          {
            id: "cubic_spline",
            name: "Cubic Spline",
            spec: {
              type: "record",
              fields: [
                {
                  id: "smoothing_length",
                  name: "Smoothing Length (m)",
                  spec: { type: "real", min: 0, default: 0.01 },
                },
              ],
            },
          },
          {
            id: "quintic_spline",
            name: "Quintic Spline",
            spec: {
              type: "record",
              fields: [
                {
                  id: "smoothing_length",
                  name: "Smoothing Length (m)",
                  spec: { type: "real", min: 0 },
                },
                {
                  id: "normalization",
                  name: "Normalization",
                  spec: { type: "bool", default: true },
                },
              ],
            },
          },
        ],
      },
    },
    {
      id: "particles",
      name: "Particles",
      spec: {
        type: "record",
        fields: [
          {
            id: "count",
            name: "Count",
            spec: { type: "int", min: 1, default: 1000 },
          },
          {
            id: "spacing",
            name: "Spacing (m)",
            spec: { type: "real", min: 0, default: 0.01 },
          },
          {
            id: "initial_velocity",
            name: "Initial Velocity (m/s)",
            spec: {
              type: "record",
              fields: [
                {
                  id: "x",
                  name: "X",
                  spec: { type: "real", default: 0 },
                },
                {
                  id: "y",
                  name: "Y",
                  spec: { type: "real", default: 0 },
                },
                {
                  id: "z",
                  name: "Z",
                  spec: { type: "real", default: 0 },
                },
              ],
            },
          },
        ],
      },
    },
    {
      id: "equation_of_state",
      name: "Equation of State",
      spec: {
        type: "variant",
        default: "weakly_compressible",
        options: [
          {
            id: "weakly_compressible",
            name: "Weakly Compressible",
            spec: {
              type: "record",
              fields: [
                {
                  id: "speed_of_sound",
                  name: "Speed of Sound (m/s)",
                  spec: { type: "real", min: 0, default: 1480 },
                },
                {
                  id: "gamma",
                  name: "Gamma",
                  spec: { type: "real", min: 1, default: 7 },
                },
              ],
            },
          },
          {
            id: "ideal_gas",
            name: "Ideal Gas",
            spec: {
              type: "record",
              fields: [
                {
                  id: "specific_heat_ratio",
                  name: "Specific Heat Ratio",
                  spec: { type: "real", min: 1, default: 1.4 },
                },
                {
                  id: "reference_pressure",
                  name: "Reference Pressure (Pa)",
                  spec: { type: "real", min: 0, default: 101325 },
                },
              ],
            },
          },
        ],
      },
    },
    {
      id: "boundary",
      name: "Boundary Conditions",
      spec: {
        type: "record",
        fields: [
          {
            id: "type",
            name: "Type",
            spec: {
              type: "enum",
              options: [
                { id: "periodic", name: "Periodic" },
                { id: "reflective", name: "Reflective" },
                { id: "absorbing", name: "Absorbing" },
              ],
              default: "reflective",
            },
          },
          {
            id: "ghost_layers",
            name: "Ghost Layers",
            spec: { type: "int", min: 1, max: 5, default: 2 },
          },
        ],
      },
    },
    {
      id: "output",
      name: "Output",
      spec: {
        type: "record",
        fields: [
          {
            id: "directory",
            name: "Directory",
            spec: { type: "string", default: "output" },
          },
          {
            id: "format",
            name: "Format",
            spec: {
              type: "enum",
              options: [
                { id: "vtk", name: "VTK" },
                { id: "hdf5", name: "HDF5" },
                { id: "csv", name: "CSV" },
              ],
            },
          },
          {
            id: "interval",
            name: "Write Interval",
            spec: { type: "int", min: 1, default: 100 },
          },
          {
            id: "compress",
            name: "Compress",
            spec: { type: "bool", default: true },
          },
          {
            id: "fields",
            name: "Output Fields",
            spec: {
              type: "array",
              item: {
                type: "enum",
                options: [
                  { id: "pressure", name: "Pressure" },
                  { id: "velocity", name: "Velocity" },
                  { id: "density", name: "Density" },
                  { id: "energy", name: "Energy" },
                ],
              },
            },
          },
        ],
      },
    },
  ],
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let MOCK_TREE: unknown = {
  simulation: {
    verbose: "yes",
  },
  time_integration: {
    _active: "runge_kutta_2",
    runge_kutta_2: {
      time_step: 0.0005,
    },
  },
  kernel: {
    _active: "cubic_spline",
    cubic_spline: {
      smoothing_length: 0.015,
    },
  },
  particles: {
    count: 5000,
    spacing: 0.005,
  },
  equation_of_state: {
    _active: "weakly_compressible",
    weakly_compressible: {
      speed_of_sound: 1480,
      gamma: "high",
    },
  },
  boundary: {
    ghost_layers: 3,
  },
  output: {
    directory: "results",
    interval: 42,
    compress: true,
    fields: ["pressure", "velocity"],
  },
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function delay(ms: number) {
  return new Promise((resolve) => {
    setTimeout(resolve, ms);
  });
}

export async function fetchSpec() {
  await delay(50);
  return MOCK_SPEC;
}

export async function fetchTree() {
  await delay(50);
  return MOCK_TREE;
}

export async function postTree(tree: unknown) {
  await delay(10);
  MOCK_TREE = tree;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

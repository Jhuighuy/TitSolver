/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type EdlSettings = {
  enabled: boolean;
  radius: number;
  strength: number;
};

export type SsaoSettings = {
  enabled: boolean;
  radius: number;
  intensity: number;
  bias: number;
};

export type SmaaSettings = {
  enabled: boolean;
};

export type EffectsSettings = {
  edl: EdlSettings;
  ssao: SsaoSettings;
  smaa: SmaaSettings;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const effectsSettingsDefault: EffectsSettings = {
  edl: {
    enabled: true,
    radius: 1.5,
    strength: 1.2,
  },
  ssao: {
    enabled: false,
    radius: 4,
    intensity: 1.5,
    bias: 0.01,
  },
  smaa: {
    enabled: true,
  },
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { z } from "zod";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export type NumericArray =
  | Int8Array
  | Uint8Array
  | Int16Array
  | Uint16Array
  | Int32Array
  | Uint32Array
  | Float32Array
  | Float64Array;

export const numericArraySchema = z.custom<NumericArray>((value) =>
  ArrayBuffer.isView(value),
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const kindSchema = z.union([
  z.literal("int8_t"),
  z.literal("uint8_t"),
  z.literal("int16_t"),
  z.literal("uint16_t"),
  z.literal("int32_t"),
  z.literal("uint32_t"),
  z.literal("float32_t"),
  z.literal("float64_t"),
]);

export type Kind = z.infer<typeof kindSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const typeSchema = z.object({
  kind: kindSchema,
  rank: z.number().int().min(0).max(2),
  dim: z.number().int().min(1).max(3),
});

export type Type = z.infer<typeof typeSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const fieldSchema = z.object({
  type: typeSchema,
  data: numericArraySchema,
});

export type Field = z.infer<typeof fieldSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const frameSchema = z.record(z.string(), fieldSchema);

export type Frame = z.infer<typeof frameSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

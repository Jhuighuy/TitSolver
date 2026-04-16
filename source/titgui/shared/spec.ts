/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { z } from "zod";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const boolSpecSchema = z.object({
  type: z.literal("bool"),
  default: z.boolean(),
});

export type BoolSpec = z.infer<typeof boolSpecSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const numberSpecSchema = z
  .union([
    z.object({
      type: z.literal("int"),
      min: z.int().optional(),
      max: z.int().optional(),
      default: z.int().optional(),
    }),
    z.object({
      type: z.literal("real"),
      min: z.number().optional(),
      max: z.number().optional(),
      default: z.number().optional(),
    }),
  ])
  .superRefine(({ min, max, default: defaultValue }, ctx) => {
    const code = "custom";

    if (min !== undefined && max !== undefined && min > max) {
      ctx.addIssue({ code, message: "`min` must be <= `max`." });
    }

    if (defaultValue !== undefined) {
      if (min !== undefined && defaultValue < min) {
        ctx.addIssue({ code, message: "`default` must be >= `min`." });
      }
      if (max !== undefined && defaultValue > max) {
        ctx.addIssue({ code, message: "`default` must be <= `max`." });
      }
    }
  });

export type NumberSpec = z.infer<typeof numberSpecSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const stringSpecSchema = z.object({
  type: z.literal("string"),
  default: z.string().optional(),
});

export type StringSpec = z.infer<typeof stringSpecSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const enumOptionSchema = z.object({
  id: z.string(),
  name: z.string(),
});

const enumSpecSchema = z
  .object({
    type: z.literal("enum"),
    options: z.array(enumOptionSchema),
    default: z.string().optional(),
  })
  .superRefine(({ options, default: defaultValue }, ctx) => {
    const code = "custom";

    const optionIds = options.map((opt) => opt.id);
    if (optionIds.some((id, index) => optionIds.indexOf(id) !== index)) {
      ctx.addIssue({ code, message: "`options` must have unique IDs." });
    }

    if (
      defaultValue !== undefined &&
      !options.some((opt) => opt.id === defaultValue)
    ) {
      ctx.addIssue({ code, message: "`default` must be one of the options." });
    }
  });

export type EnumSpec = z.infer<typeof enumSpecSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const arraySpecSchema = z.object({
  type: z.literal("array"),
  get item() {
    return specSchema;
  },
});

export type ArraySpec = z.infer<typeof arraySpecSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const recordFieldSpecSchema = z.object({
  id: z.string(),
  name: z.string(),
  get spec() {
    return specSchema;
  },
});

const recordSpecSchema = z
  .object({
    type: z.literal("record"),
    get fields() {
      return z.array(recordFieldSpecSchema);
    },
  })
  .superRefine(({ fields }, ctx) => {
    const code = "custom";

    const fieldIds = fields.map((field) => field.id);
    if (fieldIds.some((id, index) => fieldIds.indexOf(id) !== index)) {
      ctx.addIssue({ code, message: "`fields` must have unique IDs." });
    }
  });

export type RecordSpec = z.infer<typeof recordSpecSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const variantOptionSpecSchema = z.object({
  id: z.string(),
  name: z.string(),
  get spec() {
    return specSchema;
  },
});

const variantSpecSchema = z
  .object({
    type: z.literal("variant"),
    get options() {
      return z.array(variantOptionSpecSchema);
    },
    default: z.string().optional(),
  })
  .superRefine(({ options, default: defaultValue }, ctx) => {
    const code = "custom";

    const optionIds = options.map((opt) => opt.id);
    if (optionIds.some((id, index) => optionIds.indexOf(id) !== index)) {
      ctx.addIssue({ code, message: "`options` must have unique IDs." });
    }

    if (
      defaultValue !== undefined &&
      !options.some((opt) => opt.id === defaultValue)
    ) {
      ctx.addIssue({ code, message: "`default` must be one of the options." });
    }
  });

export type VariantSpec = z.infer<typeof variantSpecSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const specSchema = z.union([
  boolSpecSchema,
  numberSpecSchema,
  stringSpecSchema,
  enumSpecSchema,
  arraySpecSchema,
  recordSpecSchema,
  variantSpecSchema,
]);

export type Spec = z.infer<typeof specSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

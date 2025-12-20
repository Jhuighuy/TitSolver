/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, Text, TextField } from "@radix-ui/themes";
import { useEffect, useState } from "react";
import { z } from "zod";

import { useConnection } from "~/components/connection";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const paramSchema = z
  .object({
    id: z.number().int().nonnegative(),
    parentID: z.number().int().nonnegative().nullable(),
    name: z.string(),
    value: z.string(),
  })
  .and(
    z.union([
      z.object({
        type: z.literal("bool"),
        default: z.boolean().optional(),
      }),
      z.object({
        type: z.literal("int"),
        default: z.number().int().optional(),
        min: z.number().int().optional(),
        max: z.number().int().optional(),
      }),
      z.object({
        type: z.literal("float"),
        default: z.number().optional(),
        min: z.number().optional(),
        max: z.number().optional(),
        unit: z.string().optional(),
      }),
      z.object({
        type: z.literal("string"),
        default: z.string().optional(),
      }),
      z.object({
        type: z.literal("enum"),
        options: z.array(z.string()),
        default: z.string().optional(),
      }),
      z.object({
        type: z.literal("record"),
      }),
    ])
  );

const paramsSchema = z.array(paramSchema);

type Param = z.infer<typeof paramSchema>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type ParamEditorProps = {
  param: Param;
};

function ParamEditor({ param }: Readonly<ParamEditorProps>) {
  const [value, setValue] = useState(param.value);

  const { sendMessage } = useConnection();

  return (
    <Flex direction="row" justify="between">
      <Text>{param.name}</Text>
      <TextField.Root
        value={value}
        onChange={(e) => {
          setValue(e.target.value);
          sendMessage({
            type: "set-param",
            id: param.id,
            value: e.target.value,
          });
        }}
      />
    </Flex>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function ParamsMenu() {
  const [params, setParams] = useState<Param[]>([]);

  const { sendMessage } = useConnection();

  useEffect(() => {
    sendMessage({ type: "all-params" }, (responseRaw) => {
      setParams(paramsSchema.parse(responseRaw));
    });
  }, [sendMessage]);

  /** @todo This is a very quick and dirty prototype. */
  return (
    <Box>
      {params.map(
        (param) =>
          param.type !== "record" && (
            <ParamEditor key={param.id} param={param} />
          )
      )}
    </Box>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

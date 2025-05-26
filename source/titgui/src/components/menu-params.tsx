/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, Select, Switch, Text, TextField } from "@radix-ui/themes";
import { useCallback, useEffect, useState } from "react";
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
    ]),
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

  const sendValue = useCallback(
    (newValue: string) => {
      sendMessage({
        type: "set-param",
        id: param.id,
        value: newValue,
      });
    },
    [param.id, sendMessage],
  );

  return (
    <Flex direction="row" justify="between">
      <Text>{param.name}</Text>
      <Box width="50%" onBlur={() => sendValue(value)}>
        {(() => {
          switch (param.type) {
            case "bool":
              return (
                <Switch
                  checked={value === "true"}
                  onCheckedChange={(checked) => setValue(String(checked))}
                />
              );

            case "enum":
              return (
                <Select.Root value={value} onValueChange={setValue}>
                  <Select.Trigger />
                  <Select.Content>
                    {param.options.map((opt) => (
                      <Select.Item key={opt} value={opt}>
                        {opt}
                      </Select.Item>
                    ))}
                  </Select.Content>
                </Select.Root>
              );

            case "int":
            case "float":
              return (
                <TextField.Root
                  type="number"
                  value={value}
                  min={param.min}
                  max={param.max}
                  onChange={(e) => setValue(e.target.value)}
                />
              );

            default:
              return (
                <TextField.Root
                  value={value}
                  onChange={(e) => setValue(e.target.value)}
                />
              );
          }
        })()}
      </Box>
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
          ),
      )}
    </Box>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

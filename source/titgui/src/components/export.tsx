/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Button } from "@radix-ui/themes";
import { type ComponentProps, useState } from "react";
import { z } from "zod";

import { useConnection } from "~/components/connection";
import { assert } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function ExportButton({
  disabled,
  children,
  ...props
}: Readonly<Omit<ComponentProps<typeof Button>, "onClick">>) {
  const [isExporting, setIsExporting] = useState(false);
  const { sendMessage } = useConnection();

  return (
    <Button
      disabled={disabled || isExporting}
      onClick={() => {
        assert(!isExporting);
        setIsExporting(true);

        sendMessage({ type: "export" }, (result) => {
          setIsExporting(false);

          const fileName = z.string().parse(result);
          const linkElement = document.createElement("a");
          linkElement.href = `/export/${fileName}`;
          linkElement.download = fileName;
          linkElement.click();
        });
      }}
      {...props}
    >
      {children}
    </Button>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

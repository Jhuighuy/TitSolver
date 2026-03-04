/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Button } from "@radix-ui/themes";
import { useMutation } from "@tanstack/react-query";
import { type ComponentProps, useState } from "react";

import { backendUrl, exportStorage } from "~/backend-api";
import { assert, downloadFile } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function ExportButton({
  disabled,
  children,
  ...props
}: Readonly<Omit<ComponentProps<typeof Button>, "onClick">>) {
  const [isExporting, setIsExporting] = useState(false);
  const exportMutation = useMutation({ mutationFn: exportStorage });

  return (
    <Button
      disabled={disabled || isExporting}
      onClick={() => {
        assert(!isExporting);
        setIsExporting(true);

        exportMutation.mutate(undefined, {
          onSuccess: (fileName) => {
            setIsExporting(false);
            downloadFile(fileName, backendUrl(`/export/${fileName}`));
          },
          onError: () => {
            setIsExporting(false);
          },
        });
      }}
      {...props}
    >
      {children}
    </Button>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

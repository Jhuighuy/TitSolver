/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Button } from "@radix-ui/themes";
import { useMutation } from "@tanstack/react-query";
import { type ComponentProps, useEffect, useState } from "react";
import { z } from "zod";

import { useConnection } from "~/hooks/use-connection";
import { downloadFile } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function ExportButton({
  disabled,
  children,
  color,
  ...props
}: Readonly<Omit<ComponentProps<typeof Button>, "onClick">>) {
  // ---- Color. ---------------------------------------------------------------

  const [showSuccess, setShowSuccess] = useState(false);

  useEffect(() => {
    if (showSuccess) return;
    const timeout = setTimeout(() => setShowSuccess(false), 1000);
    return () => clearTimeout(timeout);
  }, [showSuccess]);

  // ---- Mutation. ------------------------------------------------------------

  const { sendMessageAsync } = useConnection();

  const exportMutation = useMutation({
    mutationFn: () =>
      sendMessageAsync({ type: "export" }).then((result) =>
        z.string().parse(result)
      ),
    onSuccess: (fileName) => {
      downloadFile(`/export/${fileName}`, fileName);
      setShowSuccess(true);
    },
    onError: (error) => {
      console.error("Export failed:", error);
    },
  });

  // ---- Layout. --------------------------------------------------------------

  return (
    <Button
      disabled={disabled || exportMutation.isPending}
      color={showSuccess ? "green" : color}
      onClick={() => exportMutation.mutate()}
      {...props}
    >
      {children}
    </Button>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

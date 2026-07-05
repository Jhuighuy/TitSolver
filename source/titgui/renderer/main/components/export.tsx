/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { type ComponentProps, useState } from "react";

import { Button } from "~/renderer/common/components/button";
import { ipc } from "~/renderer/common/ipc";
import { logger } from "~/renderer/common/logging";
import { assert } from "~/shared/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function ExportButton({
  disabled,
  children,
  ...props
}: Readonly<Omit<ComponentProps<typeof Button>, "onClick">>) {
  const [isExporting, setIsExporting] = useState(false);

  return (
    <Button
      disabled={(disabled ?? false) || isExporting}
      onClick={() => {
        assert(!isExporting);
        setIsExporting(true);

        void ipc.session
          .export()
          .catch((error: unknown) => {
            logger.err("Export failed.\n", error);
          })
          .finally(() => {
            setIsExporting(false);
          });
      }}
      {...props}
    >
      {children}
    </Button>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

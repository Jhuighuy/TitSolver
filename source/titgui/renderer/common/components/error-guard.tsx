/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { IconAlertTriangle } from "@tabler/icons-react";
import type { ReactNode } from "react";
import { ErrorBoundary, type FallbackProps } from "react-error-boundary";

import { Button } from "~/renderer/common/components/button";
import { Text } from "~/renderer/common/components/text";
import { logger } from "~/renderer/common/logging";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ErrorGuardProps {
  children: ReactNode;
}

/**
 * An error boundary that keeps a failing region recoverable: the rest of the
 * window stays alive, the error is logged, and the region can be retried.
 */
export function ErrorGuard({ children }: Readonly<ErrorGuardProps>) {
  return (
    <ErrorBoundary
      FallbackComponent={ErrorFallback}
      onError={(error) => {
        logger.err("Unexpected UI error.\n", error);
      }}
    >
      {children}
    </ErrorBoundary>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function ErrorFallback({ error, resetErrorBoundary }: FallbackProps) {
  return (
    <div className="flex size-full flex-col items-center justify-center gap-2 p-4">
      <IconAlertTriangle className="size-6 shrink-0 text-(--danger)" />
      <Text color="muted">This part of the window ran into an error.</Text>
      <Text mono color="subtle" className="max-w-full truncate">
        {error instanceof Error ? error.message : String(error)}
      </Text>
      <Button variant="outline" onClick={resetErrorBoundary}>
        Try Again
      </Button>
    </div>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

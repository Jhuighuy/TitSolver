/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Toast } from "@base-ui/react/toast";
import { IconAlertCircle, IconInfoSquare, IconX } from "@tabler/icons-react";

import { cn } from "~/renderer/common/components/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * The application toast manager. Usable outside React, e.g. from the logger.
 */
export const toastManager = Toast.createToastManager();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Toast viewport; mount once per window.
 */
export function AppToasts() {
  return (
    <Toast.Provider toastManager={toastManager} timeout={8000} limit={3}>
      <Toast.Portal>
        <Toast.Viewport className="fixed right-3 bottom-12 z-50 flex w-80 flex-col gap-2 outline-none">
          <ToastList />
        </Toast.Viewport>
      </Toast.Portal>
    </Toast.Provider>
  );
}

function ToastList() {
  const { toasts } = Toast.useToastManager();

  return toasts.map((toast) => (
    <Toast.Root
      key={toast.id}
      toast={toast}
      className="flex items-start gap-2 rounded border border-(--neutral-4) bg-(--neutral-1) p-2 shadow-(--shadow-popup) data-limited:hidden"
    >
      <span
        className={cn(
          "shrink-0 [&_svg]:size-4",
          toast.type === "error" ? "text-(--danger)" : "text-(--neutral-6)",
        )}
      >
        {toast.type === "error" ? <IconAlertCircle /> : <IconInfoSquare />}
      </span>

      <div className="flex min-w-0 grow flex-col gap-0.5">
        <Toast.Title className="text-(length:--text-2) leading-(--leading-2) font-medium break-words text-(--neutral-10)" />
        <Toast.Description className="text-(length:--text-1) leading-(--leading-1) break-words text-(--neutral-7)" />
      </div>

      <Toast.Close
        aria-label="Dismiss"
        className="flex size-5 shrink-0 cursor-pointer items-center justify-center rounded text-(--neutral-6) transition-colors hover:bg-(--neutral-10)/10 hover:text-(--neutral-10) [&_svg]:size-3"
      >
        <IconX />
      </Toast.Close>
    </Toast.Root>
  ));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

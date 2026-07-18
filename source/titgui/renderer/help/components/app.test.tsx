/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// @vitest-environment jsdom

import { fireEvent, screen, waitFor } from "@testing-library/react";
import { userEvent } from "@testing-library/user-event";
import {
  afterAll,
  beforeAll,
  beforeEach,
  describe,
  expect,
  it,
  vi,
} from "vitest";

import { type FakeIpc, installFakeIpc } from "~/renderer/common/fake-ipc";
import { renderWithProviders } from "~/renderer/common/testing";
import { App } from "~/renderer/help/components/app";
import type { HelpSession } from "~/shared/help";
import { WEBVIEW_OPEN_IN_TAB_CHANNEL } from "~/shared/webview";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// The help window drives a real tab/session UI over the fake IPC bridge;
// only the embedded Electron `<webview>` element is inert under jsdom.

let session: HelpSession;
let fake: FakeIpc;

const addTab = vi.fn((url?: string) => {
  const id = (session.tabs.at(-1)?.id ?? 0) + 1;
  session = {
    activeTabID: id,
    tabs: [...session.tabs, { id, url: url ?? "help://manual/" }],
  };
  fake.emit("help", "sessionChanged", session);
});
const closeTab = vi.fn();
const selectTab = vi.fn();
const navigateTab = vi.fn();

beforeAll(() => {
  fake = installFakeIpc({
    window: {
      persistGet: () => undefined,
      persistSet: () => {},
      isFullScreen: () => false,
    },
    help: {
      getSession: () => session,
      addTab: (_context, url) => {
        addTab(url);
      },
      closeTab: (_context, id) => {
        closeTab(id);
      },
      selectTab: (_context, id) => {
        selectTab(id);
      },
      navigateTab: (_context, id, url) => {
        navigateTab(id, url);
      },
    },
  });
});

afterAll(() => {
  fake.uninstall();
});

beforeEach(() => {
  vi.clearAllMocks();
  session = {
    activeTabID: 1,
    tabs: [{ id: 1, url: "help://manual/" }],
  };
});

// The `<webview>` tag is an Electron extension; under jsdom it is a plain
// unknown element. Grabbing it and faking its API is enough to exercise the
// full navigation pipeline.
function getWebview(container: HTMLElement, url = "help://manual/page") {
  const webview = container.querySelector("webview");
  expect(webview).not.toBeNull();
  return Object.assign(webview as HTMLElement, {
    getURL: () => url,
    getTitle: () => "Manual Page",
    canGoBack: () => true,
    canGoForward: () => false,
    goBack: vi.fn(),
    goForward: vi.fn(),
    reload: vi.fn(),
    findInPage: vi.fn(),
    stopFindInPage: vi.fn(),
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("help App", () => {
  it("renders the session tabs and loads the page URL", async () => {
    const { container } = renderWithProviders(<App />);

    await waitFor(() => {
      expect(container.querySelector("webview")).not.toBeNull();
    });
    // The webview src is assigned after mount.
    await waitFor(() => {
      expect(container.querySelector("webview")).toHaveAttribute(
        "src",
        "help://manual/",
      );
    });
    expect(screen.getByRole("tab")).toBeInTheDocument();
  });

  it("reflects navigation in the toolbar and tab title", async () => {
    const { container } = renderWithProviders(<App />);
    await waitFor(() => {
      expect(container.querySelector("webview")).not.toBeNull();
    });

    const webview = getWebview(container);
    fireEvent(webview, new Event("dom-ready"));

    // The page title lands on the tab, the URL in the address field, and
    // navigation state on the back/forward buttons.
    expect(await screen.findByText("Manual Page")).toBeInTheDocument();
    expect(screen.getByText("help://manual/page")).toBeInTheDocument();
    expect(screen.getByRole("button", { name: "Go back" })).toBeEnabled();
    expect(screen.getByRole("button", { name: "Go forward" })).toBeDisabled();
    expect(navigateTab).toHaveBeenCalledWith(1, "help://manual/page");

    // Toolbar actions drive the webview.
    const user = userEvent.setup();
    await user.click(screen.getByRole("button", { name: "Go back" }));
    expect(webview.goBack).toHaveBeenCalled();
    await user.click(screen.getByRole("button", { name: "Reload" }));
    expect(webview.reload).toHaveBeenCalled();
    await user.click(screen.getByRole("button", { name: "Home" }));
    expect(navigateTab).toHaveBeenCalledWith(1, undefined);
  });

  it("adds, selects, and closes tabs through the session service", async () => {
    const user = userEvent.setup();
    renderWithProviders(<App />);

    await user.click(await screen.findByRole("button", { name: "Add Tab" }));
    expect(addTab).toHaveBeenCalled();
    // The session event fans back into the UI as a second tab.
    await waitFor(() => {
      expect(screen.getAllByRole("tab")).toHaveLength(2);
    });

    await user.click(screen.getAllByRole("tab")[0]);
    expect(selectTab).toHaveBeenCalledWith(1);
  });

  it("opens link targets from the page in a new tab", async () => {
    const { container } = renderWithProviders(<App />);
    await waitFor(() => {
      expect(container.querySelector("webview")).not.toBeNull();
    });

    const webview = getWebview(container);
    const event = new Event("ipc-message");
    Object.assign(event, {
      channel: WEBVIEW_OPEN_IN_TAB_CHANNEL,
      args: ["help://manual/linked"],
    });
    fireEvent(webview, event);

    expect(addTab).toHaveBeenCalledWith("help://manual/linked");
  });

  it("searches in the page and shows the match position", async () => {
    const user = userEvent.setup();
    const { container } = renderWithProviders(<App />);
    await waitFor(() => {
      expect(container.querySelector("webview")).not.toBeNull();
    });
    const webview = getWebview(container);

    const input = screen.getByPlaceholderText("Find in page");
    await user.type(input, "sph");
    expect(webview.findInPage).toHaveBeenCalledWith("sph", {
      forward: true,
      findNext: true,
    });

    // The final found-in-page report surfaces as `current/total`.
    const found = new Event("found-in-page");
    Object.assign(found, {
      result: { activeMatchOrdinal: 2, matches: 7, finalUpdate: true },
    });
    fireEvent(webview, found);
    expect(await screen.findByText("2/7")).toBeInTheDocument();

    // Enter steps forward, escape clears.
    fireEvent.keyDown(input, { key: "Enter" });
    expect(webview.findInPage).toHaveBeenLastCalledWith("sph", {
      forward: true,
      findNext: true,
    });
    fireEvent.keyDown(input, { key: "Escape" });
    await waitFor(() => {
      expect(webview.stopFindInPage).toHaveBeenCalledWith("clearSelection");
    });
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

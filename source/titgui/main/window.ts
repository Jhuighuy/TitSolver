/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import path from "node:path";

import { BrowserWindow, nativeTheme } from "electron";
import { z } from "zod";

import { sendIpcEvent } from "~/main/ipc";
import type { PersistedState } from "~/main/persisted-state";
import { windowBackgroundColors } from "~/shared/theme";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Window kind.
 */
export type WindowKind = "main" | "help";

/**
 * Window controller.
 */
export class WindowController {
  /** Window instance. */
  public window?: BrowserWindow;

  // Title managed from the main process; page titles are ignored.
  private title = DEFAULT_WINDOW_TITLE;

  /**
   * Construct a window controller.
   */
  public constructor(
    /** Window kind. */
    public readonly kind: WindowKind,
    /** Persisted state. */
    public readonly persist: PersistedState,
  ) {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /**
   * Open the window.
   */
  public open() {
    if (this.window !== undefined) return;

    // Read persisted window state.
    const x = this.persist.get(WINDOW_X_KEY, z.int());
    const y = this.persist.get(WINDOW_Y_KEY, z.int());
    const width = this.persist.get(
      WINDOW_WIDTH_KEY,
      z.int().min(WINDOW_MIN_WIDTH),
      WINDOW_MIN_WIDTH,
    );
    const height = this.persist.get(
      WINDOW_HEIGHT_KEY,
      z.int().min(WINDOW_MIN_HEIGHT),
      WINDOW_MIN_HEIGHT,
    );
    const isMaximized = this.persist.get(
      WINDOW_MAXIMIZED_KEY,
      z.boolean(),
      false,
    );
    const isFullScreen = this.persist.get(
      WINDOW_FULLSCREEN_KEY,
      z.boolean(),
      false,
    );

    // Create the window.
    this.window = new BrowserWindow({
      show: false,
      x,
      y,
      width,
      height,
      minWidth: WINDOW_MIN_WIDTH,
      minHeight: WINDOW_MIN_HEIGHT,
      backgroundColor: getWindowBackgroundColor(),
      titleBarStyle: "hidden",
      webPreferences: {
        preload: path.join(__dirname, "preload.js"),
        contextIsolation: true,
        nodeIntegration: false,
        sandbox: true,
        // WebView tag is only used for help windows.
        webviewTag: this.kind === "help",
      },
    });

    // Save persisted window state on close.
    this.window.on("close", () => {
      if (this.window === undefined) return;

      const bounds = this.window.getNormalBounds();
      this.persist.set(WINDOW_X_KEY, bounds.x);
      this.persist.set(WINDOW_Y_KEY, bounds.y);
      this.persist.set(WINDOW_WIDTH_KEY, bounds.width);
      this.persist.set(WINDOW_HEIGHT_KEY, bounds.height);
      this.persist.set(WINDOW_MAXIMIZED_KEY, this.window.isMaximized());
      this.persist.set(WINDOW_FULLSCREEN_KEY, this.window.isFullScreen());
    });
    this.window.on("closed", () => {
      this.window = undefined;
    });

    // The main process is the source of truth for the window title.
    this.window.setTitle(this.title);
    this.window.on("page-title-updated", (event) => {
      event.preventDefault();
    });

    // Notify of full screen state changes.
    this.window.once("ready-to-show", () => {
      const window = this.window;
      if (window === undefined) return;
      sendIpcEvent(
        window,
        "window",
        "fullScreenChanged",
        window.isFullScreen(),
      );

      window.show();
      if (isMaximized) window.maximize();
      if (isFullScreen) window.setFullScreen(true);
    });
    this.window.on("enter-full-screen", () => {
      const window = this.window;
      if (window === undefined) return;
      sendIpcEvent(window, "window", "fullScreenChanged", true);
    });
    this.window.on("leave-full-screen", () => {
      const window = this.window;
      if (window === undefined) return;
      sendIpcEvent(window, "window", "fullScreenChanged", false);
    });

    // Load the content and show the window.
    const pagePath = path.join("renderer", this.kind, "index.html");
    if (RENDERER_VITE_DEV_SERVER_URL === undefined) {
      // .vite
      // |_ build <-- __dirname
      // |  |_ background.js
      // |_ renderer
      //    |_ `RENDERER_VITE_NAME`
      //       |_ <pagePath>
      const fullPagePath = path.join(
        __dirname,
        "..",
        "renderer",
        RENDERER_VITE_NAME,
        pagePath,
      );
      void this.window.loadFile(fullPagePath);
    } else {
      const fullPageUrl = new URL(
        pagePath,
        RENDERER_VITE_DEV_SERVER_URL,
      ).toString();
      void this.window.loadURL(fullPageUrl);
    }
  }

  /**
   * Close the window.
   */
  public close() {
    if (this.window === undefined) return;
    this.window.close();
  }

  /**
   * Refresh the native window background color.
   */
  public updateBackgroundColor() {
    this.window?.setBackgroundColor(getWindowBackgroundColor());
  }

  /**
   * Set the window title.
   */
  public setTitle(title: string) {
    this.title = title;
    this.window?.setTitle(title);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Window manager.
 */
export class WindowManager {
  /**
   * Window controllers.
   */
  public readonly controllers: Readonly<Record<WindowKind, WindowController>>;

  /**
   * Construct a window manager.
   */
  public constructor(persist: PersistedState) {
    // Create window controllers.
    this.controllers = {
      main: new WindowController("main", persist.withPrefix("main")),
      help: new WindowController("help", persist.withPrefix("help")),
    };

    // Open the main window.
    this.controllers.main.open();
  }

  /**
   * Find the window controller for the given window.
   */
  public find(window: BrowserWindow) {
    return Object.values(this.controllers).find(
      (controller) => controller.window === window,
    );
  }

  /**
   * Refresh native background colors for all open windows.
   */
  public updateBackgroundColors() {
    for (const controller of Object.values(this.controllers)) {
      controller.updateBackgroundColor();
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const WINDOW_MIN_WIDTH = 1280;
const WINDOW_MIN_HEIGHT = 800;

const DEFAULT_WINDOW_TITLE = "BlueTit";

const WINDOW = "window";
const WINDOW_X_KEY = `${WINDOW}.x`;
const WINDOW_Y_KEY = `${WINDOW}.y`;
const WINDOW_WIDTH_KEY = `${WINDOW}.width`;
const WINDOW_HEIGHT_KEY = `${WINDOW}.height`;
const WINDOW_MAXIMIZED_KEY = `${WINDOW}.is-maximized`;
const WINDOW_FULLSCREEN_KEY = `${WINDOW}.is-full-screen`;

function getWindowBackgroundColor() {
  return nativeTheme.shouldUseDarkColors
    ? windowBackgroundColors.dark
    : windowBackgroundColors.light;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

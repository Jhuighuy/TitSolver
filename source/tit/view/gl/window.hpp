/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <functional>
#include <map>
#include <utility>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/enum_utils.hpp"

namespace tit::view::gl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Input modifiers.
enum class Modifiers : uint8_t {
  none = 0,
  shift = GLFW_MOD_SHIFT,
  control = GLFW_MOD_CONTROL,
  alt = GLFW_MOD_ALT,
  super = GLFW_MOD_SUPER,
  caps_lock = GLFW_MOD_CAPS_LOCK,
  num_lock = GLFW_MOD_NUM_LOCK,
};

/// Keyboard key.
enum class Key : int16_t {
  space = GLFW_KEY_SPACE,
  apostrophe = GLFW_KEY_APOSTROPHE,
  comma = GLFW_KEY_COMMA,
  minus = GLFW_KEY_MINUS,
  period = GLFW_KEY_PERIOD,
  slash = GLFW_KEY_SLASH,
  _0 = GLFW_KEY_0,
  _1 = GLFW_KEY_1,
  _2 = GLFW_KEY_2,
  _3 = GLFW_KEY_3,
  _4 = GLFW_KEY_4,
  _5 = GLFW_KEY_5,
  _6 = GLFW_KEY_6,
  _7 = GLFW_KEY_7,
  _8 = GLFW_KEY_8,
  _9 = GLFW_KEY_9,
  semicolon = GLFW_KEY_SEMICOLON,
  equal = GLFW_KEY_EQUAL,
  a = GLFW_KEY_A,
  b = GLFW_KEY_B,
  c = GLFW_KEY_C,
  d = GLFW_KEY_D,
  e = GLFW_KEY_E,
  f = GLFW_KEY_F,
  g = GLFW_KEY_G,
  h = GLFW_KEY_H,
  i = GLFW_KEY_I,
  j = GLFW_KEY_J,
  k = GLFW_KEY_K,
  l = GLFW_KEY_L,
  m = GLFW_KEY_M,
  n = GLFW_KEY_N,
  o = GLFW_KEY_O,
  p = GLFW_KEY_P,
  q = GLFW_KEY_Q,
  r = GLFW_KEY_R,
  s = GLFW_KEY_S,
  t = GLFW_KEY_T,
  u = GLFW_KEY_U,
  v = GLFW_KEY_V,
  w = GLFW_KEY_W,
  x = GLFW_KEY_X,
  y = GLFW_KEY_Y,
  z = GLFW_KEY_Z,
  left_bracket = GLFW_KEY_LEFT_BRACKET,
  backslash = GLFW_KEY_BACKSLASH,
  right_bracket = GLFW_KEY_RIGHT_BRACKET,
  grave_accent = GLFW_KEY_GRAVE_ACCENT,
  world_1 = GLFW_KEY_WORLD_1,
  world_2 = GLFW_KEY_WORLD_2,
  escape = GLFW_KEY_ESCAPE,
  enter = GLFW_KEY_ENTER,
  tab = GLFW_KEY_TAB,
  backspace = GLFW_KEY_BACKSPACE,
  insert = GLFW_KEY_INSERT,
  del = GLFW_KEY_DELETE,
  right = GLFW_KEY_RIGHT,
  left = GLFW_KEY_LEFT,
  down = GLFW_KEY_DOWN,
  up = GLFW_KEY_UP,
  page_up = GLFW_KEY_PAGE_UP,
  page_down = GLFW_KEY_PAGE_DOWN,
  home = GLFW_KEY_HOME,
  end = GLFW_KEY_END,
  caps_lock = GLFW_KEY_CAPS_LOCK,
  scroll_lock = GLFW_KEY_SCROLL_LOCK,
  num_lock = GLFW_KEY_NUM_LOCK,
  print_screen = GLFW_KEY_PRINT_SCREEN,
  pause = GLFW_KEY_PAUSE,
  f1 = GLFW_KEY_F1,
  f2 = GLFW_KEY_F2,
  f3 = GLFW_KEY_F3,
  f4 = GLFW_KEY_F4,
  f5 = GLFW_KEY_F5,
  f6 = GLFW_KEY_F6,
  f7 = GLFW_KEY_F7,
  f8 = GLFW_KEY_F8,
  f9 = GLFW_KEY_F9,
  f10 = GLFW_KEY_F10,
  f11 = GLFW_KEY_F11,
  f12 = GLFW_KEY_F12,
  f13 = GLFW_KEY_F13,
  f14 = GLFW_KEY_F14,
  f15 = GLFW_KEY_F15,
  f16 = GLFW_KEY_F16,
  f17 = GLFW_KEY_F17,
  f18 = GLFW_KEY_F18,
  f19 = GLFW_KEY_F19,
  f20 = GLFW_KEY_F20,
  f21 = GLFW_KEY_F21,
  f22 = GLFW_KEY_F22,
  f23 = GLFW_KEY_F23,
  f24 = GLFW_KEY_F24,
  f25 = GLFW_KEY_F25,
  kp_0 = GLFW_KEY_KP_0,
  kp_1 = GLFW_KEY_KP_1,
  kp_2 = GLFW_KEY_KP_2,
  kp_3 = GLFW_KEY_KP_3,
  kp_4 = GLFW_KEY_KP_4,
  kp_5 = GLFW_KEY_KP_5,
  kp_6 = GLFW_KEY_KP_6,
  kp_7 = GLFW_KEY_KP_7,
  kp_8 = GLFW_KEY_KP_8,
  kp_9 = GLFW_KEY_KP_9,
  kp_decimal = GLFW_KEY_KP_DECIMAL,
  kp_divide = GLFW_KEY_KP_DIVIDE,
  kp_multiply = GLFW_KEY_KP_MULTIPLY,
  kp_subtract = GLFW_KEY_KP_SUBTRACT,
  kp_add = GLFW_KEY_KP_ADD,
  kp_enter = GLFW_KEY_KP_ENTER,
  kp_equal = GLFW_KEY_KP_EQUAL,
  left_shift = GLFW_KEY_LEFT_SHIFT,
  left_control = GLFW_KEY_LEFT_CONTROL,
  left_alt = GLFW_KEY_LEFT_ALT,
  left_super = GLFW_KEY_LEFT_SUPER,
  right_shift = GLFW_KEY_RIGHT_SHIFT,
  right_control = GLFW_KEY_RIGHT_CONTROL,
  right_alt = GLFW_KEY_RIGHT_ALT,
  right_super = GLFW_KEY_RIGHT_SUPER,
  menu = GLFW_KEY_MENU,
};

/// Key and modifiers.
using KeyMods = std::pair<Key, Modifiers>;

/// Mouse button.
enum class MouseButton : int8_t {
  _1 = GLFW_MOUSE_BUTTON_1,
  _2 = GLFW_MOUSE_BUTTON_2,
  _3 = GLFW_MOUSE_BUTTON_3,
  _4 = GLFW_MOUSE_BUTTON_4,
  _5 = GLFW_MOUSE_BUTTON_5,
  _6 = GLFW_MOUSE_BUTTON_6,
  _7 = GLFW_MOUSE_BUTTON_7,
  _8 = GLFW_MOUSE_BUTTON_8,
  left = GLFW_MOUSE_BUTTON_LEFT,
  right = GLFW_MOUSE_BUTTON_RIGHT,
  middle = GLFW_MOUSE_BUTTON_MIDDLE,
};

/// Mouse button and modifiers.
using MouseButtonMods = std::pair<MouseButton, Modifiers>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// OpenGL window session.
class WindowSession final {
public:

  /// Open the window session.
  WindowSession();

  /// Move-construct a window session.
  WindowSession(WindowSession&& other) noexcept;

  /// Move-assign the window session.
  auto operator=(WindowSession&& other) noexcept -> WindowSession&;

  /// Window session is not copy-constructible.
  WindowSession(const WindowSession&) = delete;

  /// Window session is not copyable.
  auto operator=(const WindowSession& window) -> WindowSession& = delete;

  /// Close the window session.
  ~WindowSession();

private:

  friend class Window;
  bool glfw_initialized_ = true;
  bool glew_initialized_ = false;

}; // class WindowSession

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// OpenGL window.
class Window final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a window.
  explicit Window(WindowSession& session,
                  const char* title,
                  size_t width,
                  size_t height);

  /// Move-construct a window.
  Window(Window&&) noexcept;

  /// Move-assign the window.
  auto operator=(Window&&) noexcept -> Window&;

  /// Window is not copy-constructible.
  Window(const Window&) = delete;

  /// Window is not copyable.
  auto operator=(const Window& window) -> Window& = delete;

  /// Destroy the window.
  ~Window();

  /// Underlying window pointer.
  constexpr auto underlying() const noexcept -> GLFWwindow* {
    return window_;
  }

  /// Run the window main loop.
  template<std::invocable RenderFunc>
  void main_loop(RenderFunc render_func) {
    while (!glfwWindowShouldClose(window_)) {
      render_func();
      glfwSwapBuffers(window_);
      glfwPollEvents();
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Get the current window width.
  auto width() const -> size_t;

  /// Get the current window height.
  auto height() const -> size_t;

  /// Get the current cursor position X coordinate.
  auto cursor_pos_x() const -> size_t;

  /// Get the current cursor position Y coordinate.
  auto cursor_pos_y() const -> size_t;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Close event handler.
  using OnCloseFunc = std::function<void()>;

  /// Resize event handler.
  using OnResizeFunc = std::function<void(size_t, size_t)>;

  /// Keyboard event handler.
  using OnKeyFunc = std::function<void(Key, Modifiers)>;

  /// Mouse button event handler.
  using OnMouseButtonFunc = std::function<void(MouseButton, Modifiers)>;

  /// Scroll event handler.
  using OnScrollFunc = std::function<void(ssize_t, ssize_t)>;

  /// Set cursor position event handler.
  using OnSetCursorPosFunc = std::function<void(size_t, size_t)>;

  /// Set handler for the window close event.
  void on_close(OnCloseFunc handler);

  /// Set handler for the window resize event.
  void on_resize(OnResizeFunc handler);

  /// Set handler for the key pressed event.
  /// @{
  void on_key_down(Key key, OnKeyFunc handler);
  void on_key_down(Key key, Modifiers mods, OnKeyFunc handler);
  /// @}

  /// Set handler for the key released event.
  /// @{
  void on_key_up(Key key, OnKeyFunc handler);
  void on_key_up(Key key, Modifiers mods, OnKeyFunc handler);
  /// @}

  /// Set handler for the mouse button pressed event.
  /// @{
  void on_mouse_button_down(MouseButton mouse_button,
                            OnMouseButtonFunc handler);
  void on_mouse_button_down(MouseButton mouse_button,
                            Modifiers mods,
                            OnMouseButtonFunc handler);
  /// @}

  /// Set handler for the mouse button released event.
  /// @{
  void on_mouse_button_up(MouseButton mouse_button, OnMouseButtonFunc handler);
  void on_mouse_button_up(MouseButton mouse_button,
                          Modifiers mods,
                          OnMouseButtonFunc handler);
  /// @}

  /// Set handler for the scroll event.
  void on_scroll(OnScrollFunc handler);

  /// Set handler for the set cursor position event.
  void on_set_cursor_pos(OnSetCursorPosFunc handler);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // Handle the window close event.
  static void on_close_(GLFWwindow* window);

  // Handle the window resize event.
  static void on_resize_(GLFWwindow* window, int width_int, int height_int);

  // Handle the keyboard event.
  static void on_keyboard_(GLFWwindow* window,
                           int key_int,
                           int scancode,
                           int action,
                           int mods_int);

  // Handle the mouse button event.
  static void on_mouse_button_(GLFWwindow* window,
                               int mouse_button_int,
                               int action,
                               int mods_int);

  // Handle the scroll event.
  static void on_scroll_(GLFWwindow* window,
                         double delta_x_double,
                         double delta_y_double);

  // Handle the set cursor position event.
  static void on_set_cursor_pos_(GLFWwindow* window,
                                 double pos_xd,
                                 double pos_yd);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Get the window user pointer.
  static auto get_self_(GLFWwindow* window) noexcept -> const Window&;

  // Clamp the cursor X position to the window bounds.
  auto clamp_cursor_pos_x_(double pos_xd) const -> size_t;

  // Clamp the cursor Y position to the window bounds.
  auto clamp_cursor_pos_y_(double pos_yd) const -> size_t;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  GLFWwindow* window_ = nullptr;
  std::vector<OnCloseFunc> on_close_funcs_;
  std::vector<OnResizeFunc> on_resize_funcs_;
  std::multimap<KeyMods, OnKeyFunc> on_key_down_funcs_;
  std::multimap<KeyMods, OnKeyFunc> on_key_up_funcs_;
  std::multimap<MouseButtonMods, OnMouseButtonFunc> on_mouse_button_down_funcs_;
  std::multimap<MouseButtonMods, OnMouseButtonFunc> on_mouse_button_up_funcs_;
  std::vector<OnScrollFunc> on_scroll_funcs_;
  std::vector<OnSetCursorPosFunc> on_set_cursor_pos_funcs_;

}; // class Window

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// RAII binder of an OpenGL window.
class BindWindow final {
public:

  BindWindow(BindWindow&&) = delete;
  BindWindow(const BindWindow&) = delete;
  auto operator=(BindWindow&&) -> BindWindow& = delete;
  auto operator=(const BindWindow&) -> BindWindow& = delete;

  /// Bind the @p window.
  explicit BindWindow(const Window& window) {
    glfwMakeContextCurrent(window.underlying());
  }

  /// Unbind the window.
  ~BindWindow() {
    glfwMakeContextCurrent(nullptr);
  }

}; // class BindWindow

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::view::gl

// Modifiers is a flags enum.
template<>
inline constexpr bool tit::is_flags_enum_v<tit::view::gl::Modifiers> = true;

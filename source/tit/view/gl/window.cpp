/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <bit>
#include <utility>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/log.hpp"

#include "tit/view/gl/window.hpp"

namespace tit::view::gl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Window session.
//

WindowSession::WindowSession() {
  const auto status = glfwInit();
  TIT_ASSERT(status == GLFW_TRUE, "Failed to initialize GLFW!");
  TIT_INFO("GLFW initialized, version '{}'.", glfwGetVersionString());
  // Note: GLEW needs to be initialized after the first window is created.
}

WindowSession::WindowSession(WindowSession&& other) noexcept
    : glfw_initialized_{std::exchange(other.glfw_initialized_, false)},
      glew_initialized_{std::exchange(other.glew_initialized_, false)} {}

auto WindowSession::operator=(WindowSession&& other) noexcept
    -> WindowSession& {
  glfw_initialized_ = std::exchange(other.glfw_initialized_, false);
  glew_initialized_ = std::exchange(other.glew_initialized_, false);
  return *this;
}

WindowSession::~WindowSession() {
  if (glfw_initialized_) glfwTerminate();
  // Note: GLEW does not need to be uninitialized.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Initialization.
//

Window::Window(WindowSession& session,
               const char* title,
               size_t width,
               size_t height) {
  TIT_ASSERT(title != nullptr, "Window title must be specified!");
  TIT_ASSERT(width > 0, "Window width must be positive!");
  TIT_ASSERT(height > 0, "Window hegiht must be positive!");

  // Set the window hints.
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_FALSE);

  // Create the window.
  window_ = glfwCreateWindow( // NOLINT(*-prefer-member-initializer)
      static_cast<int>(width),
      static_cast<int>(height),
      title,
      /*monitor=*/nullptr,
      /*share=*/nullptr);

  // Initialize OpenGL.
  if (!session.glew_initialized_) {
    const BindWindow bind_window{*this};
    glewExperimental = GL_TRUE;
    if (const auto status = glewInit(); status == GLEW_OK) {
      TIT_INFO("GLEW initialized!");
    } else {
      const auto* error =
          std::bit_cast<const char*>(glewGetErrorString(status));
      TIT_THROW("GLEW: failed to initialize, {:#x}: {}!", status, error);
    }
    session.glew_initialized_ = true;
  }

  // Set the event handlers.
  glfwSetWindowUserPointer(window_, this);
  glfwSetWindowCloseCallback(window_, &on_close_);
  glfwSetWindowSizeCallback(window_, &on_resize_);
  glfwSetKeyCallback(window_, &on_keyboard_);
  glfwSetMouseButtonCallback(window_, &on_mouse_button_);
  glfwSetScrollCallback(window_, &on_scroll_);
  glfwSetCursorPosCallback(window_, &on_set_cursor_pos_);
}

Window::Window(Window&& other) noexcept
    : window_{std::exchange(other.window_, nullptr)} {}

auto Window::operator=(Window&& other) noexcept -> Window& {
  window_ = std::exchange(other.window_, nullptr);
  return *this;
}

Window::~Window() {
  if (window_ != nullptr) glfwDestroyWindow(window_);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Queries.
//

auto Window::width() const -> size_t {
  TIT_ASSERT(window_ != nullptr, "Window is not loaded!");
  int width = -1;
  glfwGetWindowSize(window_, &width, nullptr);
  return static_cast<size_t>(width);
}

auto Window::height() const -> size_t {
  TIT_ASSERT(window_ != nullptr, "Window is not loaded!");
  int height = -1;
  glfwGetWindowSize(window_, nullptr, &height);
  return static_cast<size_t>(height);
}

auto Window::cursor_pos_x() const -> size_t {
  TIT_ASSERT(window_ != nullptr, "Window is not loaded!");
  double pos_xd = -1.0;
  glfwGetCursorPos(window_, &pos_xd, nullptr);
  return clamp_cursor_pos_x_(pos_xd);
}

auto Window::cursor_pos_y() const -> size_t {
  TIT_ASSERT(window_ != nullptr, "Window is not loaded!");
  double pos_yd = -1.0;
  glfwGetCursorPos(window_, nullptr, &pos_yd);
  return clamp_cursor_pos_y_(pos_yd);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Event setup.
//

void Window::on_close(OnCloseFunc handler) {
  on_close_funcs_.push_back(std::move(handler));
}

void Window::on_resize(OnResizeFunc handler) {
  on_resize_funcs_.push_back(std::move(handler));
}

void Window::on_key_down(Key key, OnKeyFunc handler) {
  on_key_down(key, Modifiers::none, std::move(handler));
}
void Window::on_key_down(Key key, Modifiers mods, OnKeyFunc handler) {
  on_key_down_funcs_.emplace(std::pair{key, mods}, std::move(handler));
}

void Window::on_key_up(Key key, OnKeyFunc handler) {
  on_key_up(key, Modifiers::none, std::move(handler));
}
void Window::on_key_up(Key key, Modifiers mods, OnKeyFunc handler) {
  on_key_up_funcs_.emplace(std::pair{key, mods}, std::move(handler));
}

void Window::on_mouse_button_down(MouseButton mouse_button,
                                  OnMouseButtonFunc handler) {
  on_mouse_button_down(mouse_button, Modifiers::none, std::move(handler));
}
void Window::on_mouse_button_down(MouseButton mouse_button,
                                  Modifiers mods,
                                  OnMouseButtonFunc handler) {
  on_mouse_button_down_funcs_.emplace(std::pair{mouse_button, mods},
                                      std::move(handler));
}

void Window::on_mouse_button_up(MouseButton mouse_button,
                                OnMouseButtonFunc handler) {
  on_mouse_button_up(mouse_button, Modifiers::none, std::move(handler));
}
void Window::on_mouse_button_up(MouseButton mouse_button,
                                Modifiers mods,
                                OnMouseButtonFunc handler) {
  on_mouse_button_up_funcs_.emplace(std::pair{mouse_button, mods},
                                    std::move(handler));
}

void Window::on_scroll(OnScrollFunc handler) {
  on_scroll_funcs_.push_back(std::move(handler));
}

void Window::on_set_cursor_pos(OnSetCursorPosFunc handler) {
  on_set_cursor_pos_funcs_.push_back(std::move(handler));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Event handlers.
//

void Window::on_close_(GLFWwindow* window) {
  const auto& self = get_self_(window);
  for (const auto& handler : self.on_close_funcs_) handler();
}

void Window::on_resize_(GLFWwindow* window, int width_int, int height_int) {
  const auto& self = get_self_(window);
  const auto width = static_cast<size_t>(width_int);
  const auto height = static_cast<size_t>(height_int);
  for (const auto& handler : self.on_resize_funcs_) handler(width, height);
}

void Window::on_keyboard_(GLFWwindow* window,
                          int key_int,
                          int /*scancode*/,
                          int action,
                          int mods_int) {
  const auto& self = get_self_(window);
  const auto key = static_cast<Key>(key_int);
  const auto mods = static_cast<Modifiers>(mods_int);
  switch (action) {
    case GLFW_PRESS:
    case GLFW_REPEAT: {
      const auto& [first, last] =
          self.on_key_down_funcs_.equal_range({key, mods});
      std::for_each(first, last, [key, mods](const auto& pair) {
        pair.second(key, mods);
      });
      break;
    }
    case GLFW_RELEASE: {
      const auto& [first, last] =
          self.on_key_up_funcs_.equal_range({key, mods});
      std::for_each(first, last, [key, mods](const auto& pair) {
        pair.second(key, mods);
      });
      break;
    }
    default: {
      TIT_WARN("GLFW: ignoring unsupported keyboard action {:#x}!", action);
      break;
    }
  }
}

void Window::on_mouse_button_(GLFWwindow* window,
                              int mouse_button_int,
                              int action,
                              int mods_int) {
  const auto& self = get_self_(window);
  const auto mouse_button = static_cast<MouseButton>(mouse_button_int);
  const auto mods = static_cast<Modifiers>(mods_int);
  switch (action) {
    case GLFW_PRESS:
    case GLFW_REPEAT: {
      const auto& [first, last] =
          self.on_mouse_button_down_funcs_.equal_range({mouse_button, mods});
      std::for_each(first, last, [mouse_button, mods](const auto& pair) {
        pair.second(mouse_button, mods);
      });
      break;
    }
    case GLFW_RELEASE: {
      const auto& [first, last] =
          self.on_mouse_button_up_funcs_.equal_range({mouse_button, mods});
      std::for_each(first, last, [mouse_button, mods](const auto& pair) {
        pair.second(mouse_button, mods);
      });
      break;
    }
    default: {
      TIT_WARN("GLFW: ignoring unsupported mouse action {:#x}!", action);
      break;
    }
  }
}

void Window::on_scroll_(GLFWwindow* window,
                        double delta_x_double,
                        double delta_y_double) {
  const auto& self = get_self_(window);
  const auto delta_x = static_cast<ssize_t>(delta_x_double);
  const auto delta_y = -static_cast<ssize_t>(delta_y_double);
  for (const auto& handler : self.on_scroll_funcs_) handler(delta_x, delta_y);
}

void Window::on_set_cursor_pos_(GLFWwindow* window,
                                double pos_xd,
                                double pos_yd) {
  const auto& self = get_self_(window);
  const auto pos_x = self.clamp_cursor_pos_x_(pos_xd);
  const auto pos_y = self.clamp_cursor_pos_y_(pos_yd);
  for (const auto& handler : self.on_set_cursor_pos_funcs_) {
    handler(pos_x, pos_y);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Miscellaneous.
//

auto Window::get_self_(GLFWwindow* window) noexcept -> const Window& {
  const auto* const user_pointer =
      static_cast<const Window*>(glfwGetWindowUserPointer(window));
  TIT_ASSERT(user_pointer != nullptr, "User pointer was not set!");
  return *user_pointer;
}

auto Window::clamp_cursor_pos_x_(double pos_xd) const -> size_t {
  return static_cast<size_t>(
      std::clamp(pos_xd, 0.0, static_cast<double>(width() - 1)));
}

auto Window::clamp_cursor_pos_y_(double pos_yd) const -> size_t {
  const auto pos_y = static_cast<size_t>(
      std::clamp(pos_yd, 0.0, static_cast<double>(height() - 1)));
  return height() - 1 - pos_y;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::view::gl

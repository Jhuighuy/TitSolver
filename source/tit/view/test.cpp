#ifdef __wasm__

#include <algorithm>

#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <emscripten/bind.h>

GLFWwindow* window = nullptr;

int initializeRenderer(int width, int height) {
  if (!glfwInit()) return -1;
  window = glfwCreateWindow(width, height, "GLFW Renderer", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  return 1;
}

void renderFrame(float /*deltaTime*/) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  double xpos = 0.0;
  double ypos = 0.0;
  glfwGetCursorPos(window, &xpos, &ypos);
  glClearColor(std::clamp(static_cast<float>(xpos) / 800.0F, 0.0F, 1.0F),
               std::clamp(static_cast<float>(ypos) / 600.0F, 0.0F, 1.0F),
               1.0F,
               1.0F);

  glfwSwapBuffers(window);
}

using namespace emscripten;

EMSCRIPTEN_BINDINGS(test_module) {
  function("initializeRenderer", &initializeRenderer);
  function("renderFrame", &renderFrame);
}

auto main() -> int {
  return 0; // In WASM you always need a main function.
}

#endif

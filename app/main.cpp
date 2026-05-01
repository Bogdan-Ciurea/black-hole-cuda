#include "core/window.h"

#include <glad/gl.h>

#include <cstdio>

extern void launchHelloKernel();

int main() {
  Window window;
  if (!window.init(1920, 1080, "BlackHoleTracer")) {
    std::fprintf(stderr, "Failed to create window\n");
    return 1;
  }

  launchHelloKernel();

  while (!window.shouldClose()) {
    window.pollEvents();

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    window.swapBuffers();
  }

  return 0;
}

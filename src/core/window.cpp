#include "core/window.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cstdio>

Window::~Window() {
  shutdown();
}

bool Window::init(int width, int height, const char* title) {
  if (!glfwInit()) {
    std::fprintf(stderr, "Failed to initialize GLFW\n");
    return false;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!window_) {
    std::fprintf(stderr, "Failed to create GLFW window\n");
    glfwTerminate();
    return false;
  }

  glfwMakeContextCurrent(window_);

  int version = gladLoadGL(glfwGetProcAddress);
  if (!version) {
    std::fprintf(stderr, "Failed to initialize GLAD\n");
    glfwDestroyWindow(window_);
    glfwTerminate();
    window_ = nullptr;
    return false;
  }

  glfwSwapInterval(1);
  glViewport(0, 0, width, height);

  glfwSetFramebufferSizeCallback(window_, [](GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
  });

  return true;
}

bool Window::shouldClose() {
  return glfwWindowShouldClose(window_);
}

void Window::pollEvents() {
  glfwPollEvents();
  if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void Window::swapBuffers() {
  glfwSwapBuffers(window_);
}

void Window::shutdown() {
  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
    glfwTerminate();
  }
}

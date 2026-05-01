#pragma once

struct GLFWwindow;

class Window {
public:
  Window() = default;
  ~Window();

  bool init(int width, int height, const char* title);
  bool shouldClose();
  void pollEvents();
  void swapBuffers();
  void shutdown();

  GLFWwindow* handle() const { return window_; }

private:
  GLFWwindow* window_ = nullptr;
};

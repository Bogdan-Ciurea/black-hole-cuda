#include "core/window.h"
#include "render/fullscreen_quad.h"
#include "render/shader.h"

#include <glad/gl.h>

#include <cstdio>

static const char* vertexShaderSrc = R"(
#version 460 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 uv;

void main() {
  uv = aUV;
  gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* fragmentShaderSrc = R"(
#version 460 core
in vec2 uv;
out vec4 fragColor;

uniform vec2 uCenter;
uniform float uRadius;
uniform vec2 uResolution;

void main() {
  // Correct for aspect ratio
  vec2 coord = uv;
  coord.x *= uResolution.x / uResolution.y;

  vec2 center = uCenter;
  center.x *= uResolution.x / uResolution.y;

  float dist = distance(coord, center);
  if (dist < uRadius)
    fragColor = vec4(1.0, 1.0, 1.0, 1.0);
  else
    fragColor = vec4(0.05, 0.05, 0.08, 1.0);
}
)";

int main() {
  Window window;
  if (!window.init(800, 600, "BlackHoleTracer - Circle")) {
    std::fprintf(stderr, "Failed to create window\n");
    return 1;
  }

  bht::render::Shader shader;
  if (!shader.compile(vertexShaderSrc, fragmentShaderSrc)) {
    std::fprintf(stderr, "Failed to compile shaders\n");
    return 1;
  }

  bht::render::FullscreenQuad quad;
  quad.init();

  while (!window.shouldClose()) {
    window.pollEvents();

    glClear(GL_COLOR_BUFFER_BIT);

    shader.use();
    shader.setVec2("uCenter", 0.5f, 0.5f);
    shader.setFloat("uRadius", 0.2f);
    shader.setVec2("uResolution", 800.0f, 600.0f);
    quad.draw();

    window.swapBuffers();
  }

  return 0;
}

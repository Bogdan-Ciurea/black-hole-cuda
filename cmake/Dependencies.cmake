include(FetchContent)

# GLM — header-only math library (CUDA compatible)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1
  GIT_SHALLOW    TRUE
)

# GLFW — windowing and input
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
  GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# GLAD v2 — OpenGL loader (generated for GL 4.6 Core)
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG        v2.0.8
  GIT_SHALLOW    TRUE
)

# Google Test — unit testing
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
  GIT_SHALLOW    TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# stb — header-only image loading (stb_image.h)
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(glm glfw googletest stb)

# GLAD v2 — has no top-level CMakeLists.txt, need to add its cmake/ subdirectory
FetchContent_GetProperties(glad)
if(NOT glad_POPULATED)
  FetchContent_Populate(glad)
endif()
add_subdirectory("${glad_SOURCE_DIR}/cmake" "${glad_BINARY_DIR}")
glad_add_library(glad_gl REPRODUCIBLE API gl:core=4.6)

# Download skybox asset if not present
set(SKYBOX_PATH "${CMAKE_SOURCE_DIR}/assets/skybox.jpg")
if(NOT EXISTS "${SKYBOX_PATH}")
  message(STATUS "Downloading skybox star map...")
  file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/assets")
  file(DOWNLOAD
    "https://www.solarsystemscope.com/textures/download/8k_stars_milky_way.jpg"
    "${SKYBOX_PATH}"
    SHOW_PROGRESS
    STATUS SKYBOX_DL_STATUS
  )
  list(GET SKYBOX_DL_STATUS 0 SKYBOX_DL_CODE)
  if(NOT SKYBOX_DL_CODE EQUAL 0)
    message(WARNING "Failed to download skybox image. Procedural starfield will be used.")
    file(REMOVE "${SKYBOX_PATH}")
  else()
    message(STATUS "Skybox downloaded to ${SKYBOX_PATH}")
  endif()
endif()

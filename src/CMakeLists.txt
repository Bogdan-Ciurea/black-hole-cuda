# Automatically collect all .cpp and .cu source files
file(GLOB_RECURSE ENGINE_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cu"
)

add_library(BlackHoleEngine STATIC ${ENGINE_SOURCES})

target_include_directories(BlackHoleEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(BlackHoleEngine
  PUBLIC
    OpenGL::GL
    glfw
    glad_gl
    glm::glm
)

target_compile_definitions(BlackHoleEngine
  PUBLIC
    GLM_FORCE_CUDA
    GLM_ENABLE_EXPERIMENTAL
)

set_target_properties(BlackHoleEngine PROPERTIES
  CUDA_SEPARABLE_COMPILATION ON
  CUDA_RESOLVE_DEVICE_SYMBOLS ON
)

# Auto-generate one executable per .cpp file in this directory
file(GLOB APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(source ${APP_SOURCES})
  get_filename_component(name ${source} NAME_WE)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE BlackHoleEngine)
  target_compile_definitions(${name} PRIVATE
    BHT_PROJECT_DIR="${CMAKE_SOURCE_DIR}")
endforeach()

cmake_minimum_required(VERSION 3.10)

project(myproject)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

include_directories(src/fmt/include)
include_directories(src/synapse/synapse/include)
include_directories(src/freetype/include)
include_directories(src/glfw/include)
include_directories(src/cgtb/include)

add_subdirectory(src/fmt)
add_subdirectory(src/glfw)
add_subdirectory(src/synapse)
add_subdirectory(src/freetype)
add_subdirectory(src/cgtb)

link_libraries(fmt)
link_libraries(synapse)
link_libraries(freetype)
link_libraries(glfw)
link_libraries(cgtb)

add_subdirectory(src/app)
cmake_minimum_required(VERSION 3.10)
project(gorgon)
add_subdirectory(sandfox)
add_executable(
	dash
	src/main.cpp
)
set_property(TARGET dash PROPERTY CXX_STANDARD 17)
target_include_directories(dash PRIVATE sandfox)
target_link_libraries(dash PRIVATE sandfox)
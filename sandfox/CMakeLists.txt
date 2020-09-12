cmake_minimum_required(VERSION 3.10)
project(sandfox)
add_library(
	${PROJECT_NAME}
	STATIC
	src/sandfox/core.cpp
	src/sandfox/impl.cpp
	src/sandfox/ui.cpp
)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
add_subdirectory(extern)
target_include_directories(${PROJECT_NAME} PUBLIC extern include)
target_link_libraries(${PROJECT_NAME} PUBLIC extern)
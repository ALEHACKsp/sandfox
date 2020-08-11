project(cgtb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(cgtb/third-party/glfw)

add_library(

	cgtb

	STATIC

	cgtb/ui.cpp
	cgtb/ui/canvas.cpp
	cgtb/ui/wrapper.cpp
	cgtb/icons.cpp
	cgtb/third-party/nanovg.c
	cgtb/third-party/glad.c
)

target_link_libraries(cgtb glfw)
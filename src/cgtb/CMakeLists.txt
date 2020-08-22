project(cgtb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("${CMAKE_CURRENT_SOURCE_DIR}")

add_library(

	cgtb

	STATIC

	src/ui.cpp
	src/ui/canvas.cpp
	src/ui/reactor.cpp
	src/third-party/nanovg.c
	src/third-party/glad.c
)
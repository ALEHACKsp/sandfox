cmake_minimum_required(VERSION 3.10)

project(extern)

if(NOT EMSCRIPTEN)
	add_library(
		${PROJECT_NAME} STATIC
		src/glad.c
		src/nanovg.c
		src/synapse/src/block.cpp
		src/synapse/src/connect.cpp
		src/synapse/src/thread_local_queue.cpp
	)
else()
	message("-- Building for the web.")
	add_library(
		${PROJECT_NAME} STATIC
		src/nanovg.c
		src/synapse/src/block.cpp
		src/synapse/src/connect.cpp
		src/synapse/src/thread_local_queue.cpp
	)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(BUILD_SHARED_LIBS OFF)

# UV

add_subdirectory(src/uv)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/uv/include)

# SYNAPSE

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/synapse/include)

# FREETYPE

add_subdirectory(src/freetype)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/freetype/include)

# SPDLOG

add_subdirectory(src/spdlog)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/spdlog/include)

# ASSIMP

option(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
option(ASSIMP_BUILD_TESTS OFF)
option(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
option(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
option(ASSIMP_BUILD_COLLADA_IMPORTER ON)
add_subdirectory(src/assimp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/assimp/include)

# GLFW

if(NOT EMSCRIPTEN)
	option(GLFW_BUILD_EXAMPLES OFF)
	option(GLFW_BUILD_TESTS OFF)
	option(GLFW_BUILD_DOCS OFF)
	option(GLFW_INSTALL OFF)
	add_subdirectory(src/glfw)
	target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/glfw/include)
else()
	message("-- Skipping building GLFW because web build has been indicated.")
endif()

# NNG

if(NOT EMSCRIPTEN)
	option(NNG_TESTS OFF)
	option(NNG_TOOLS OFF)
	add_subdirectory(src/nng)
	target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/nng/include)
else()
	message("-- Skipping building NNG because web build has been indicated.")
endif()

# PLATFORM SPECIFICS

if(EMSCRIPTEN)
	target_link_libraries(${PROJECT_NAME} freetype assimp spdlog uv_a)
	# set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS "-s USE_GLFW=3 -s USE_WEBGL2=1 --shell-file shell_raylib.html")
	# set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
	if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
		target_link_libraries(${PROJECT_NAME} -lopengl32 -lwinmm -lws2_32)
		#if (HIDE_WINDOWS_TERMINAL)
		#	set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS "-mwindows")
		#	message("-- There will be no terminal window allocated. You may need to use WinMain() as your entry point instead of main().")
		#endif()
	elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
		# lsb_release -is
		find_program(LSB_RELEASE_EXEC lsb_release)
		execute_process(COMMAND ${LSB_RELEASE_EXEC} -is OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE)
		if(${LSB_RELEASE_ID_SHORT} STREQUAL "Ubuntu")
			target_link_libraries(${PROJECT_NAME} -lGL)
		endif()
	endif()
	target_link_libraries(${PROJECT_NAME} glfw freetype assimp nng spdlog uv_a)
endif()
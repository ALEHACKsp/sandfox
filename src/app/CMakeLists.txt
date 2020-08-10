project(app)

include(generate_product_version)

generate_product_version(
   VERSION_INFORMATION
   NAME "My App"
   ICON "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.ico"
   VERSION_MAJOR 1
   VERSION_MINOR 0
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
	${PROJECT_NAME}
	${VERSION_INFORMATION}
	main.cpp
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

	message("")
	message("Detected Windows. Expecting MSYS2/GCC.")
	message("Required packages: mingw-w64-x86_64-glfw mingw-w64-x86_64-glew mingw-w64-x86_64-fmt")
	message("In case of link errors. Not that those must be available.")
	message("")

	# glfw3 - https://packages.msys2.org/package/mingw-w64-x86_64-glfw
	# glew32 - https://packages.msys2.org/package/mingw-w64-x86_64-glew
	# fmt - https://packages.msys2.org/package/mingw-w64-x86_64-fmt

	target_link_libraries(${PROJECT_NAME} -lglfw3 -lglew32 -lopengl32 -lfmt)
	# set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS "-mwindows") # use WinMain, console window will no be created

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

	find_program(LSB_RELEASE_EXEC lsb_release)
	execute_process(COMMAND ${LSB_RELEASE_EXEC} -is
		OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT # this will be populated with a specific distro name such as "Ubuntu"
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if(${LSB_RELEASE_ID_SHORT} STREQUAL "Ubuntu")

		message("")
		message("Detected Ubuntu.")
		message("Required packages: libglfw3-dev libglew-dev libfmt-dev")
		message("In case of link errors. Not that those must be available.")
		message("")

		# glfw - https://packages.ubuntu.com/focal/libglfw3-dev
		# GLEW - https://packages.ubuntu.com/focal/libglew-dev
		# fmt - https://packages.ubuntu.com/focal/libfmt-dev

		target_link_libraries(${PROJECT_NAME} -lglfw -lGLEW -lGL -lfmt)

	endif()
endif()
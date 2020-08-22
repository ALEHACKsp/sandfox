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

add_subdirectory(emico)

add_executable(
	${PROJECT_NAME}
	${VERSION_INFORMATION}
	main.cpp
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

	target_link_libraries(${PROJECT_NAME} -lopengl32)
	# set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS "-mwindows") # use WinMain, console window will no be created

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

	find_program(LSB_RELEASE_EXEC lsb_release)
	execute_process(COMMAND ${LSB_RELEASE_EXEC} -is
		OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT # this will be populated with a specific distro name such as "Ubuntu"
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if(${LSB_RELEASE_ID_SHORT} STREQUAL "Ubuntu")

		target_link_libraries(${PROJECT_NAME} -lGL)

	endif()
endif()

target_link_libraries(${PROJECT_NAME} emico freetype)
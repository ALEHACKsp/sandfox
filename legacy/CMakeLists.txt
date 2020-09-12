cmake_minimum_required(VERSION 3.10)
project(app)
add_subdirectory(sandfox)

add_executable(mongoose mongoose.cpp)
set_property(TARGET mongoose PROPERTY CXX_STANDARD 17)
target_include_directories(mongoose PRIVATE sandfox)
target_link_libraries(mongoose PRIVATE sandfox)

add_executable(monkey monkey.cpp)
set_property(TARGET monkey PROPERTY CXX_STANDARD 17)
target_include_directories(monkey PRIVATE sandfox)
target_link_libraries(monkey PRIVATE sandfox)
project(synapse)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(

	synapse

	STATIC

	synapse/src/block.cpp
	synapse/src/connect.cpp
	synapse/src/thread_local_queue.cpp
)

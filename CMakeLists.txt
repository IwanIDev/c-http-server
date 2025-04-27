cmake_minimum_required(VERSION 3.10)

# Project name
project(c-http-server)

# Set the C standard
set(CMAKE_C_STANDARD 99)

file(GLOB_RECURSE SRC src/*.c)
# Set the output directory for the executable

# Add the executable
add_executable(${PROJECT_NAME} ${SRC})

# Include directories (if any)
include_directories(include)

# Link libraries (if any)
# target_link_libraries(c_http_server <library_name>)

cmake_minimum_required(VERSION 3.10)

# Project name
project(c_http_server)

# Set the C standard
set(CMAKE_C_STANDARD 99)

# Add the executable
add_executable(c_http_server src/main.c)

# Include directories (if any)
include_directories(include)

# Link libraries (if any)
# target_link_libraries(c_http_server <library_name>)
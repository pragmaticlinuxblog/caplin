# Specify the minimum version
cmake_minimum_required(VERSION 3.7)

# Specify the project name
project(canapp)

# Set the required C standard.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add sources
set(
  PROG_SRCS
  source/${PROJECT_NAME}.c
  source/lib/caplin.c
  source/lib/can.c
  source/lib/timer.c
  source/lib/keys.c
  source/lib/util.c
)

# Specify what is needed to create the main target.
add_executable(${PROJECT_NAME} ${PROG_SRCS})

# Set include directories.
target_include_directories(${PROJECT_NAME} PUBLIC
  source
  source/lib
)

# Specify the libraries that should be linked.
target_link_libraries(${PROJECT_NAME} pthread)

# Specify how to install the binary.
install (TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

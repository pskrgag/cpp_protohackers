# -----------------------------------------------------------------------------
# Executable sources
# -----------------------------------------------------------------------------
add_executable(task0 src/main.cpp)
target_link_libraries(task0 PRIVATE cocur)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")

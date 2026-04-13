# -----------------------------------------------------------------------------
# Executable sources
# -----------------------------------------------------------------------------
add_executable(task2 src/main.cpp)
target_link_libraries(task2 PRIVATE cocur)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -O2")

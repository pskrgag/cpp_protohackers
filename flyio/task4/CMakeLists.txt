# -----------------------------------------------------------------------------
# Executable sources
# -----------------------------------------------------------------------------
add_executable(task4 src/main.cpp)
target_link_libraries(task4 PRIVATE cocur)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -O2")

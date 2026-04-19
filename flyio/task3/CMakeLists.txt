# -----------------------------------------------------------------------------
# Executable sources
# -----------------------------------------------------------------------------
add_executable(task3 src/main.cpp)
target_link_libraries(task3 PRIVATE cocur)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -O2")

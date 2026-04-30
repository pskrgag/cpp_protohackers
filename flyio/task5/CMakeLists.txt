# -----------------------------------------------------------------------------
# Executable sources
# -----------------------------------------------------------------------------
add_executable(task5 src/main.cpp)
target_link_libraries(task5 PRIVATE cocur)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -O2")

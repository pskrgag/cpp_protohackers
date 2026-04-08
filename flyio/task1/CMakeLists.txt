# -----------------------------------------------------------------------------
# Executable sources
# -----------------------------------------------------------------------------
find_package(RapidJSON REQUIRED)

add_executable(task1 src/main.cpp)
target_link_libraries(task1 PRIVATE cocur)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")

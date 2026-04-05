include(GoogleTest)

add_executable(tests
	bounded_spsc.cpp
	unbounded_spsc.cpp
	net/helpers.cpp
	engine.cpp
)

set_target_properties(tests PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
)

target_link_libraries(tests
	PRIVATE
	cocur
	gtest_main
	gtest
)
target_compile_options(tests PRIVATE -ggdb)

add_test(NAME tests COMMAND tests)

set_tests_properties(tests PROPERTIES
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

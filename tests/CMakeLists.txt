find_package(GTest REQUIRED)

add_executable(graphics_engine_tests
	graphics/test_child_pool.cpp
	graphics/test_frame_stats.cpp
	graphics/test_pan_zoom_camera.cpp
	graphics/test_projection.cpp
	graphics/test_rect.cpp
	graphics/test_scene_graph.cpp
	physics/test_physics_world.cpp
)

target_link_libraries(graphics_engine_tests PRIVATE graphics_engine GTest::gtest_main)

# Less strict than the library — tests trade off some warning hygiene for
# brevity (e.g. literal integer narrowing in EXPECT_EQ arguments).
target_compile_options(graphics_engine_tests PRIVATE -Wall -Wextra -Wpedantic)

include(GoogleTest)
gtest_discover_tests(graphics_engine_tests)

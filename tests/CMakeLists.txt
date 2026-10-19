cmake_minimum_required(VERSION 3.10)

find_package(GTest QUIET)
if (NOT GTEST_FOUND)
    message("Unable to find GTest on the system, downloading it...")
    add_subdirectory(deps/GTest)
endif()
enable_testing()

# The GTest package on some distributions (e.g. Arch Linux) exposes GMock
# under the GTest:: namespace; alias the expected GMock:: target names.
include(${CMAKE_CURRENT_SOURCE_DIR}/GMockAliases.cmake)

set(TESTS
        src/test_morkparser.cpp
        src/test_utils.cpp
        src/test_windowtools_x11_state.cpp
        src/test_quit_authorization.cpp
        )

add_executable(tests src/TestResources.cpp src/TestResources.h ${TESTS})
target_include_directories(tests PRIVATE src ../src)
target_link_libraries(tests GTest::GTest GMock::GMock GMock::Main birdtray_lib)
gtest_discover_tests(tests)

add_executable(native_messaging_startup_test
        src/test_native_messaging_startup.cpp
        ../src/native_messaging.cpp)
target_include_directories(native_messaging_startup_test PRIVATE ../src)
target_link_libraries(native_messaging_startup_test Qt6::Core Qt6::Network)
add_test(NAME NativeMessaging.EnsureRunningStartsHelper
        COMMAND native_messaging_startup_test --self-test)

add_custom_target(run_tests COMMENT "Run the Birdtray tests")
add_custom_command(TARGET run_tests
        POST_BUILD COMMAND $<TARGET_FILE:tests>)

cmake_minimum_required(VERSION 3.14)

project(Stream)


include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_compile_options(-std=c++2b)
enable_testing()

add_executable(test_stream test_stream.cpp)
target_link_libraries(test_stream GTest::gtest_main)

add_executable(stream stream.cpp)

include(GoogleTest)
gtest_discover_tests(test_stream)
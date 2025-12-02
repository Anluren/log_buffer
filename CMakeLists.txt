cmake_minimum_required(VERSION 3.14)
project(log_buffer VERSION 1.0.0 LANGUAGES CXX)

# Check if Ninja is available and use it as the generator
find_program(NINJA_EXECUTABLE ninja)
if(NINJA_EXECUTABLE AND NOT CMAKE_GENERATOR MATCHES "Ninja")
    message(STATUS "Ninja found at ${NINJA_EXECUTABLE}")
    message(WARNING "Ninja is available but not being used. Reconfigure with: cmake -G Ninja ..")
endif()

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiled library (static by default, can be shared with -DBUILD_SHARED_LIBS=ON)
add_library(log_buffer src/logger.cpp)
target_include_directories(log_buffer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(log_buffer PUBLIC cxx_std_17)

# Optional: Add compile options for smaller code size
target_compile_options(log_buffer PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Os>
    $<$<CXX_COMPILER_ID:MSVC>:/Os>
)

# Examples
add_executable(basic_usage examples/basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE log_buffer)

# Enable testing
enable_testing()

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.10.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# Disable GMock - we only need GTest
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
# Disable warnings as errors for GoogleTest (v1.10.0 has deprecated-copy warnings with newer compilers)
set(CMAKE_CXX_FLAGS_BACKUP "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")
FetchContent_MakeAvailable(googletest)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_BACKUP}")

# Test executable
add_executable(test_logger tests/test_logger.cpp)
target_link_libraries(test_logger PRIVATE log_buffer gtest_main)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(test_logger)

cmake_minimum_required(VERSION 3.24)
project(timed-worker LANGUAGES CXX VERSION 1.0.0)

# Force C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Check compiler support
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
        message(FATAL_ERROR "GCC 13 or later is required for C++23 support")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17)
        message(FATAL_ERROR "Clang 17 or later is required for C++23 support")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.30)
        message(FATAL_ERROR "MSVC 19.30 or later is required for C++23 support")
    endif()
endif()

# Header-only interface target
add_library(timed_worker INTERFACE)
target_include_directories(timed_worker INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(timed_worker INTERFACE cxx_std_23)

# Example
add_executable(example_basic examples/basic.cpp)
target_link_libraries(example_basic PRIVATE timed_worker)

# ---------- Tests with GoogleTest ----------
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(example_tests test/timed_worker_tests.cpp)
target_link_libraries(example_tests PRIVATE
    timed_worker
    GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(example_tests)

# ---------- install / export ----------
include(GNUInstallDirs)
install(TARGETS timed_worker EXPORT twTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT twTargets NAMESPACE tw:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/timed_worker)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/timed_workerConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)

install(FILES
    "${PROJECT_BINARY_DIR}/timed_workerConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/timed_worker) 
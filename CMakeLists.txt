cmake_minimum_required(VERSION 3.24)
project(timed-worker LANGUAGES CXX VERSION 1.0.0)

# === C++ standard ===
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# === Compiler checks ===
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10)
    message(FATAL_ERROR "GCC 10+ is required")
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 18)
    message(FATAL_ERROR "Clang 18+ is required (17 is buggy)")
  endif()
elseif(MSVC)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.29)
    message(FATAL_ERROR "MSVC 19.29+ is required")
  endif()
endif()

# === Build options ===
# This is the standard CTest toggle; off by default.
include(CTest)
# BUILD_TESTING is defined by include(CTest), default=OFF
# You can also override with -DBUILD_TESTING=ON

# === Library target ===
add_library(timed_worker INTERFACE)
target_include_directories(timed_worker INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_features(timed_worker INTERFACE cxx_std_20)

# === Example binary (always built) ===
add_executable(example_basic examples/basic.cpp)
target_link_libraries(example_basic PRIVATE timed_worker)

# === Tests (only if BUILD_TESTING=ON) ===
if(BUILD_TESTING)
  message(STATUS "Configuring tests…")

  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
  )
  FetchContent_MakeAvailable(googletest)

  enable_testing()    # turn on CTest machinery

  add_executable(example_tests test/timed_worker_tests.cpp)
  target_link_libraries(example_tests PRIVATE
    timed_worker
    GTest::gtest_main
  )

  include(GoogleTest)
  gtest_discover_tests(example_tests)
endif()

# === Install / export ===
include(GNUInstallDirs)

# Library headers & interface-target
install(TARGETS timed_worker EXPORT twTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Exported CMake targets file
install(EXPORT twTargets
  NAMESPACE tw::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/timed_worker
)

# Generate and install a Config file so that downstream users can:
#   find_package(timed_worker CONFIG REQUIRED)
# and link to tw::timed_worker
include(CMakePackageConfigHelpers)
# Path to a small template—we’ll create this next…
set(_config_in "${CMAKE_CURRENT_LIST_DIR}/cmake/timed_workerConfig.cmake.in")
set(_config_out "${PROJECT_BINARY_DIR}/timed_workerConfig.cmake")

configure_package_config_file(
  ${_config_in}
  ${_config_out}
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/timed_worker
)

write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/timed_workerConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${_config_out}"
  "${PROJECT_BINARY_DIR}/timed_workerConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/timed_worker
)

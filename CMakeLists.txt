cmake_minimum_required(VERSION 3.20)
project(iora_sdp VERSION 0.1.0 LANGUAGES CXX
  DESCRIPTION "Standalone, header-only C++17 SDP parser/serializer with WebRTC offer/answer utilities")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==========================================================================
# iora_sdp (header-only interface library — zero external dependencies)
# ==========================================================================
add_library(iora_sdp INTERFACE)
target_include_directories(iora_sdp INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# ==========================================================================
# Install rules
# ==========================================================================
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS iora_sdp EXPORT iora_sdpTargets)

install(DIRECTORY include/iora DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT iora_sdpTargets
  NAMESPACE iora_sdp::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iora_sdp
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/iora_sdpConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/iora_sdpConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iora_sdp
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/iora_sdpConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/iora_sdpConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/iora_sdpConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iora_sdp
)

# ==========================================================================
# Fuzz testing (requires clang with libFuzzer or AFL)
# ==========================================================================
option(IORA_SDP_BUILD_FUZZ "Build fuzz testing targets" OFF)
if(IORA_SDP_BUILD_FUZZ)
  add_executable(fuzz_parser tests/fuzz_parser.cpp)
  target_link_libraries(fuzz_parser PRIVATE iora_sdp)
endif()

# ==========================================================================
# Tests
# ==========================================================================
option(IORA_SDP_BUILD_TESTS "Build iora_sdp tests" ON)
if(IORA_SDP_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

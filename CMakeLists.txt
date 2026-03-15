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
  "${CMAKE_SOURCE_DIR}/include"
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

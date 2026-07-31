# tests/CMakeLists.txt — Tests for iora_sdp

# Catch2 v2.13.10 via the standardized find-or-fetch resolver: reuse a system
# Catch2 ONLY if it is exactly 2.13.10 AND provides Catch2::Catch2, else fetch the
# pinned tag. Behaviourally identical across iora-family repos (cmake/Dependencies.cmake).
# (Previously iora_sdp fetched Catch2 nowhere and sdp_tests linked no Catch2 target,
# so the suite could not build standalone — fixed here.) test_main.cpp defines
# CATCH_CONFIG_MAIN, so link plain Catch2::Catch2 (header-only) — NOT WithMain — to
# avoid a duplicate main().
include(${PROJECT_SOURCE_DIR}/cmake/Dependencies.cmake)
find_or_fetch_dependency(
  NAME Catch2
  PACKAGE_NAME Catch2
  TARGET_NAME Catch2::Catch2
  VERSION 2.13.10
  EXACT
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v2.13.10
)

add_executable(sdp_tests
  test_main.cpp
  test_types.cpp
  test_attributes.cpp
  test_session_description.cpp
  test_parser.cpp
  test_serializer.cpp
  test_offer_answer.cpp
  test_round_trip.cpp
  test_webrtc_sdp.cpp
  test_effective_direction.cpp
)

target_link_libraries(sdp_tests PRIVATE
  iora_sdp
  Catch2::Catch2
)

enable_testing()
add_test(NAME sdp_tests COMMAND sdp_tests)

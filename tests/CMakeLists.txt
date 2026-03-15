# tests/CMakeLists.txt — Tests for iora_sdp

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
)

target_link_libraries(sdp_tests PRIVATE
  iora_sdp
)

enable_testing()
add_test(NAME sdp_tests COMMAND sdp_tests)

#include <catch2/catch.hpp>
#include <string>

#include "iora/sdp/sdp_parser.hpp"
#include "iora/sdp/sdp_serializer.hpp"

using namespace iora::sdp;

namespace {

void roundTrip(const std::string& sdp, const SdpParserOptions& opts = SdpParserOptions{})
{
  auto r1 = SdpParser::parse(sdp, opts);
  REQUIRE(r1.hasValue());
  REQUIRE_FALSE(r1.hasError());

  auto serialized = SdpSerializer::serialize(*r1.value);

  auto r2 = SdpParser::parse(serialized, opts);
  REQUIRE(r2.hasValue());
  REQUIRE_FALSE(r2.hasError());

  REQUIRE(*r1.value == *r2.value);
}

} // namespace

// ---------------------------------------------------------------------------
// T1: Test Infrastructure and Minimal Round-Trips
// ---------------------------------------------------------------------------

TEST_CASE("Round-trip: minimal audio SDP", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
  );
}

TEST_CASE("Round-trip: minimal audio with explicit rtpmap", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
  );
}

TEST_CASE("Round-trip: minimal video-only SDP", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 RTP/AVP 96\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
  );
}

TEST_CASE("Round-trip: session-level fields only", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
  );
}

// ---------------------------------------------------------------------------
// T2: Full WebRTC SDP Round-Trip
// ---------------------------------------------------------------------------

TEST_CASE("Round-trip: full WebRTC audio+video SDP", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 4962location303286location9location IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=ice-ufrag:abcd1234\r\n"
    "a=ice-pwd:aabbccddee11223344556677\r\n"
    "a=ice-options:trickle\r\n"
    "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
    "a=extmap-allow-mixed\r\n"
    "a=group:BUNDLE 0 1\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111 0\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=sendrecv\r\n"
    "a=mid:0\r\n"
    "a=msid:stream0 track0\r\n"
    "a=ice-ufrag:abcd1234\r\n"
    "a=ice-pwd:aabbccddee11223344556677\r\n"
    "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
    "a=setup:actpass\r\n"
    "a=rtcp-mux\r\n"
    "a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\n"
    "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\n"
    "a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\n"
    "a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
    "a=fmtp:111 minptime=10;useinbandfec=1\r\n"
    "a=rtcp-fb:111 transport-cc\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=ssrc:1234567890 cname:user@example.com\r\n"
    "a=ssrc:1234567890 msid:stream0 track0\r\n"
    "a=candidate:1 1 udp 2113937151 192.168.1.100 5000 typ host\r\n"
    "a=candidate:2 1 udp 1845501695 203.0.113.1 6000 typ srflx raddr 192.168.1.100 rport 5000\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=sendrecv\r\n"
    "a=mid:1\r\n"
    "a=msid:stream0 track1\r\n"
    "a=ice-ufrag:abcd1234\r\n"
    "a=ice-pwd:aabbccddee11223344556677\r\n"
    "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
    "a=setup:actpass\r\n"
    "a=rtcp-mux\r\n"
    "a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid\r\n"
    "a=extmap:5 urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=rtcp-fb:96 nack\r\n"
    "a=rtcp-fb:96 nack pli\r\n"
    "a=rtcp-fb:96 transport-cc\r\n"
    "a=rtcp-fb:96 goog-remb\r\n"
    "a=ssrc:9876543210 cname:user@example.com\r\n"
    "a=candidate:1 1 udp 2113937151 192.168.1.100 5002 typ host\r\n"
    "a=candidate:2 1 udp 1845501695 203.0.113.1 6002 typ srflx raddr 192.168.1.100 rport 5002\r\n"
  );
}

TEST_CASE("Round-trip: large SDP with 50+ ICE candidates", "[round_trip]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=sendrecv\r\n"
    "a=mid:0\r\n"
    "a=rtcp-mux\r\n"
    "a=rtpmap:111 opus/48000/2\r\n";

  for (int i = 0; i < 55; ++i)
  {
    sdp += "a=candidate:" + std::to_string(i) + " 1 udp " + std::to_string(2000000000 - i)
        + " 192.168.1." + std::to_string(i % 256) + " " + std::to_string(5000 + i) + " typ host\r\n";
  }

  auto r1 = SdpParser::parse(sdp);
  REQUIRE(r1.hasValue());
  REQUIRE(r1.value->mediaDescriptions[0].candidates.size() == 55);

  auto serialized = SdpSerializer::serialize(*r1.value);
  auto r2 = SdpParser::parse(serialized);
  REQUIRE(r2.hasValue());
  REQUIRE(*r1.value == *r2.value);
}

TEST_CASE("Round-trip: ICE candidate with IPv6 address", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
    "a=candidate:1 1 udp 2113937151 fd12:3456:789a::1 5000 typ host\r\n"
  );
}

// ---------------------------------------------------------------------------
// T3: Multi-media, Data Channel, and SIP Round-Trips
// ---------------------------------------------------------------------------

TEST_CASE("Round-trip: audio + video + data channel", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 1234567890 1 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=group:BUNDLE 0 1 2\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111 0\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=sendrecv\r\n"
    "a=mid:0\r\n"
    "a=rtcp-mux\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
    "a=fmtp:111 minptime=10;useinbandfec=1\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96 97\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=sendrecv\r\n"
    "a=mid:1\r\n"
    "a=rtcp-mux\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=rtpmap:97 H264/90000\r\n"
    "a=fmtp:97 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f\r\n"
    "m=application 9 UDP/DTLS/SCTP webrtc-datachannel\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=sendrecv\r\n"
    "a=mid:2\r\n"
    "a=sctp-port:5000\r\n"
    "a=max-message-size:262144\r\n"
  );
}

TEST_CASE("Round-trip: SIP SDP without WebRTC extensions", "[round_trip]")
{
  auto sdp =
    "v=0\r\n"
    "o=user 53655765 2353687637 IN IP4 192.168.1.10\r\n"
    "s=SIP Call\r\n"
    "t=0 0\r\n"
    "m=audio 49170 RTP/AVP 0 8 101\r\n"
    "c=IN IP4 192.168.1.10\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtpmap:8 PCMA/8000\r\n"
    "a=rtpmap:101 telephone-event/8000\r\n"
    "a=fmtp:101 0-16\r\n"
    "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:d0RmdmcmVCspeEc3QGZiNWpVLFJhQX1cfHAwJSoj\r\n";

  auto r1 = SdpParser::parse(sdp);
  REQUIRE(r1.hasValue());

  auto serialized = SdpSerializer::serialize(*r1.value);
  auto r2 = SdpParser::parse(serialized);
  REQUIRE(r2.hasValue());
  REQUIRE(*r1.value == *r2.value);

  // Verify no WebRTC attributes injected
  REQUIRE(serialized.find("a=ice-ufrag") == std::string::npos);
  REQUIRE(serialized.find("a=ice-pwd") == std::string::npos);
  REQUIRE(serialized.find("a=fingerprint") == std::string::npos);
  REQUIRE(serialized.find("a=rtcp-mux") == std::string::npos);
  REQUIRE(serialized.find("a=group:BUNDLE") == std::string::npos);

  // Verify codec order preserved: 0, 8, 101
  auto& media = r2.value->mediaDescriptions[0];
  REQUIRE(media.formats.size() == 3);
  REQUIRE(media.formats[0] == "0");
  REQUIRE(media.formats[1] == "8");
  REQUIRE(media.formats[2] == "101");
}

TEST_CASE("Round-trip: disabled media section (port 0)", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=sendrecv\r\n"
    "m=video 0 RTP/AVP 96\r\n"
    "a=inactive\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
  );

  auto r = SdpParser::parse(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 0 RTP/AVP 96\r\n"
    "a=inactive\r\n"
  );
  REQUIRE(r.hasValue());
  REQUIRE(r.value->mediaDescriptions[0].port == 0);
  REQUIRE(r.value->mediaDescriptions[0].direction == Direction::INACTIVE);
}

TEST_CASE("Round-trip: media section with zero ICE candidates", "[round_trip]")
{
  auto sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=sendrecv\r\n"
    "a=mid:0\r\n"
    "a=rtcp-mux\r\n"
    "a=rtpmap:111 opus/48000/2\r\n";

  auto r1 = SdpParser::parse(sdp);
  REQUIRE(r1.hasValue());
  REQUIRE(r1.value->mediaDescriptions[0].candidates.empty());

  auto serialized = SdpSerializer::serialize(*r1.value);
  auto r2 = SdpParser::parse(serialized);
  REQUIRE(r2.hasValue());
  REQUIRE(r2.value->mediaDescriptions[0].candidates.empty());
  REQUIRE(*r1.value == *r2.value);
}

TEST_CASE("Round-trip: fmtp with whitespace around delimiters", "[round_trip]")
{
  // The raw parameters string is preserved exactly
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 RTP/AVP 96\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=fmtp:96 apt = 97 ; something = else\r\n"
  );
}

// ---------------------------------------------------------------------------
// T4: Optional Fields, Timing, and Connection Round-Trips
// ---------------------------------------------------------------------------

TEST_CASE("Round-trip: all optional session-level fields", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=admin 1234 5678 IN IP4 10.0.0.1\r\n"
    "s=Test Session\r\n"
    "i=A session for testing\r\n"
    "u=http://example.com/session\r\n"
    "e=admin@example.com\r\n"
    "p=+1-555-0100\r\n"
    "c=IN IP4 10.0.0.1\r\n"
    "t=0 0\r\n"
    "k=base64:encodedkey\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
  );
}

TEST_CASE("Round-trip: missing optional fields remain absent", "[round_trip]")
{
  auto sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n";

  auto r1 = SdpParser::parse(sdp);
  REQUIRE(r1.hasValue());
  REQUIRE_FALSE(r1.value->sessionInfo.has_value());
  REQUIRE_FALSE(r1.value->uri.has_value());
  REQUIRE_FALSE(r1.value->emailAddress.has_value());
  REQUIRE_FALSE(r1.value->phoneNumber.has_value());
  REQUIRE_FALSE(r1.value->connection.has_value());
  REQUIRE_FALSE(r1.value->encryptionKey.has_value());

  auto serialized = SdpSerializer::serialize(*r1.value);
  auto r2 = SdpParser::parse(serialized);
  REQUIRE(r2.hasValue());
  REQUIRE_FALSE(r2.value->sessionInfo.has_value());
  REQUIRE_FALSE(r2.value->uri.has_value());
  REQUIRE_FALSE(r2.value->emailAddress.has_value());
  REQUIRE_FALSE(r2.value->phoneNumber.has_value());
  REQUIRE_FALSE(r2.value->connection.has_value());
  REQUIRE_FALSE(r2.value->encryptionKey.has_value());
  REQUIRE(*r1.value == *r2.value);
}

TEST_CASE("Round-trip: non-ASCII UTF-8 in session name and info", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=T\xc3\xabst S\xc3\xa9ssion\r\n"
    "i=Sessi\xc3\xb3n informaci\xc3\xb3n\r\n"
    "t=0 0\r\n"
  );
}

TEST_CASE("Round-trip: repeat time lines", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "r=7d 1h 0 25h\r\n"
  );
}

TEST_CASE("Round-trip: multiple TimeDescriptions", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=2873397496 2873404696\r\n"
    "t=2873480896 2873488096\r\n"
  );
}

TEST_CASE("Round-trip: zone adjustments", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "z=2882844526 -1h 2898848070 0\r\n"
  );
}

TEST_CASE("Round-trip: encryption key at session and media level", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "k=base64:sessionkey\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "k=base64:mediakey\r\n"
  );
}

TEST_CASE("Round-trip: IPv6 connection", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP6 ::1\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "c=IN IP6 ::1\r\n"
  );
}

TEST_CASE("Round-trip: IPv4 multicast TTL", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 5004 RTP/AVP 0\r\n"
    "c=IN IP4 224.2.36.42/127\r\n"
  );
}

TEST_CASE("Round-trip: IPv4 multicast TTL and numberOfAddresses", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 5004 RTP/AVP 0\r\n"
    "c=IN IP4 224.2.36.42/127/3\r\n"
  );
}

TEST_CASE("Round-trip: multiple bandwidth lines", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 RTP/AVP 96\r\n"
    "b=AS:128\r\n"
    "b=TIAS:128000\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
  );
}

TEST_CASE("Round-trip: ptime, maxptime, framerate", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=ptime:20\r\n"
    "a=maxptime:40\r\n"
  );
}

TEST_CASE("Round-trip: fractional framerate", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 RTP/AVP 96\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=framerate:29.97\r\n"
  );
}

TEST_CASE("Round-trip: integer framerate", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 RTP/AVP 96\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=framerate:30\r\n"
  );
}

// ---------------------------------------------------------------------------
// T5: Attribute Preservation and Edge Cases
// ---------------------------------------------------------------------------

TEST_CASE("Round-trip: unknown/proprietary attributes", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=sendrecv\r\n"
    "a=x-custom:value\r\n"
    "a=x-flag\r\n"
    "a=x-another:data\r\n"
  );

  // Verify insertion order
  auto r = SdpParser::parse(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=sendrecv\r\n"
    "a=x-custom:value\r\n"
    "a=x-flag\r\n"
    "a=x-another:data\r\n"
  );
  REQUIRE(r.hasValue());
  auto& ga = r.value->mediaDescriptions[0].genericAttributes;
  REQUIRE(ga.size() == 3);
  REQUIRE(ga[0].name == "x-custom");
  REQUIRE(ga[0].value == "value");
  REQUIRE(ga[1].name == "x-flag");
  REQUIRE_FALSE(ga[1].value.has_value());
  REQUIRE(ga[2].name == "x-another");
  REQUIRE(ga[2].value == "data");
}

TEST_CASE("Round-trip: simulcast with rid", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=simulcast:send h;m;l\r\n"
    "a=rid:h send pt=96;max-fps=30;max-height=720;max-width=1280\r\n"
    "a=rid:m send pt=96;max-fps=30;max-height=360;max-width=640\r\n"
    "a=rid:l send pt=96;max-fps=15;max-height=180;max-width=320\r\n"
  );
}

TEST_CASE("Round-trip: simulcast with paused streams", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=simulcast:send h;~m;l\r\n"
    "a=rid:h send\r\n"
    "a=rid:m send\r\n"
    "a=rid:l send\r\n"
  );

  auto r = SdpParser::parse(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=simulcast:send h;~m;l\r\n"
    "a=rid:h send\r\n"
    "a=rid:m send\r\n"
    "a=rid:l send\r\n"
  );
  REQUIRE(r.hasValue());
  auto& sim = r.value->mediaDescriptions[0].simulcast;
  REQUIRE(sim.has_value());
  REQUIRE(sim->sendStreams.size() == 3);
  REQUIRE_FALSE(sim->sendStreams[0][0].paused);
  REQUIRE(sim->sendStreams[1][0].paused);
  REQUIRE_FALSE(sim->sendStreams[2][0].paused);
}

TEST_CASE("Round-trip: simulcast with alternatives", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=simulcast:send h,h2;m\r\n"
    "a=rid:h send\r\n"
    "a=rid:h2 send\r\n"
    "a=rid:m send\r\n"
  );

  auto r = SdpParser::parse(
    "v=0\r\no=- 0 0 IN IP4 0.0.0.0\r\ns=-\r\nt=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=sendrecv\r\na=rtpmap:96 VP8/90000\r\n"
    "a=simulcast:send h,h2;m\r\n"
  );
  REQUIRE(r.hasValue());
  auto& sim = r.value->mediaDescriptions[0].simulcast;
  REQUIRE(sim->sendStreams.size() == 2);
  REQUIRE(sim->sendStreams[0].size() == 2); // h,h2 are alternatives
  REQUIRE(sim->sendStreams[0][0].rid == "h");
  REQUIRE(sim->sendStreams[0][1].rid == "h2");
}

TEST_CASE("Round-trip: simulcast recv-only", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=simulcast:recv h;m;l\r\n"
  );
}

TEST_CASE("Round-trip: simulcast send+recv", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=simulcast:send h;m recv h;m\r\n"
  );
}

TEST_CASE("Round-trip: rid with restrictions (sorted keys)", "[round_trip]")
{
  // Restrictions are unordered_map; serializer sorts keys alphabetically.
  // Round-trip compares parsed models via operator== (order-independent).
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=rid:h send pt=96;max-width=1280;max-height=720;max-fps=30\r\n"
  );
}

TEST_CASE("Round-trip: ssrc-group entries", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=ssrc:1111 cname:test\r\n"
    "a=ssrc:2222 cname:test\r\n"
    "a=ssrc-group:FID 1111 2222\r\n"
  );
}

TEST_CASE("Round-trip: ICE candidate TCP with tcptype", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
    "a=candidate:1 1 tcp 1518280447 192.168.1.100 9 typ host tcptype passive\r\n"
  );
}

TEST_CASE("Round-trip: ICE candidate with extension attributes", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
    "a=candidate:1 1 udp 2113937151 192.168.1.100 5000 typ host generation 0 network-id 1\r\n"
  );
}

TEST_CASE("Round-trip: ICE candidate relay with raddr/rport", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
    "a=candidate:1 1 udp 100 10.0.0.1 3478 typ relay raddr 192.168.1.100 rport 5000\r\n"
  );
}

TEST_CASE("Round-trip: multiple fingerprints at session level", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=fingerprint:sha-1 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD\r\n"
    "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
  );
}

TEST_CASE("Round-trip: generic attribute with = in value", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=sendrecv\r\n"
    "a=foo:bar=baz\r\n"
  );

  auto r = SdpParser::parse(
    "v=0\r\no=- 0 0 IN IP4 0.0.0.0\r\ns=-\r\nt=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\na=sendrecv\r\na=foo:bar=baz\r\n"
  );
  REQUIRE(r.hasValue());
  auto& ga = r.value->mediaDescriptions[0].genericAttributes;
  REQUIRE(ga.size() == 1);
  REQUIRE(ga[0].name == "foo");
  REQUIRE(ga[0].value == "bar=baz");
}

TEST_CASE("Round-trip: all boolean flag attributes", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=ice-lite\r\n"
    "a=extmap-allow-mixed\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=sendrecv\r\n"
    "a=rtcp-mux\r\n"
    "a=rtcp-mux-only\r\n"
    "a=rtcp-rsize\r\n"
    "a=extmap-allow-mixed\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
    "a=end-of-candidates\r\n"
  );
}

TEST_CASE("Round-trip: media section with numberOfPorts", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 5004/2 RTP/AVP 0\r\n"
  );

  auto r = SdpParser::parse(
    "v=0\r\no=- 0 0 IN IP4 0.0.0.0\r\ns=-\r\nt=0 0\r\n"
    "m=audio 5004/2 RTP/AVP 0\r\n"
  );
  REQUIRE(r.hasValue());
  REQUIRE(r.value->mediaDescriptions[0].port == 5004);
  REQUIRE(r.value->mediaDescriptions[0].numberOfPorts == 2);
}

TEST_CASE("Round-trip: multiple formats order preserved", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111 0 8\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtpmap:8 PCMA/8000\r\n"
  );
}

TEST_CASE("Round-trip: crypto with and without sessionParams", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/SAVP 0\r\n"
    "a=sendrecv\r\n"
    "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:base64key\r\n"
    "a=crypto:2 AES_CM_128_HMAC_SHA1_32 inline:anotherkey UNENCRYPTED_SRTP\r\n"
  );
}

TEST_CASE("Round-trip: group with LS semantics", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=group:LS 0 1\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=sendrecv\r\n"
    "a=mid:0\r\n"
    "m=video 9 RTP/AVP 96\r\n"
    "a=sendrecv\r\n"
    "a=mid:1\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
  );
}

TEST_CASE("Round-trip: bare LF line endings (lenient mode)", "[round_trip]")
{
  // Input uses bare LF; default options (strict=false) handle it
  auto sdp =
    "v=0\n"
    "o=- 0 0 IN IP4 0.0.0.0\n"
    "s=-\n"
    "t=0 0\n"
    "m=audio 9 RTP/AVP 0\n";

  SdpParserOptions opts;
  auto r1 = SdpParser::parse(sdp, opts);
  REQUIRE(r1.hasValue());

  // Serialized output uses CRLF
  auto serialized = SdpSerializer::serialize(*r1.value);
  REQUIRE(serialized.find("\r\n") != std::string::npos);

  auto r2 = SdpParser::parse(serialized, opts);
  REQUIRE(r2.hasValue());
  REQUIRE(*r1.value == *r2.value);
}

TEST_CASE("Round-trip: rtcp port-only and full form", "[round_trip]")
{
  // Port-only
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=sendrecv\r\n"
    "a=rtcp:9\r\n"
  );

  // Full form
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=sendrecv\r\n"
    "a=rtcp:9 IN IP4 0.0.0.0\r\n"
  );
}

TEST_CASE("Round-trip: msid with and without trackId", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=sendrecv\r\n"
    "a=msid:stream track\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
  );

  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=sendrecv\r\n"
    "a=msid:stream\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
  );
}

TEST_CASE("Round-trip: multiple msid lines per media", "[round_trip]")
{
  roundTrip(
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=sendrecv\r\n"
    "a=msid:stream1 track1\r\n"
    "a=msid:stream2 track2\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
  );
}

TEST_CASE("Round-trip: duplicate setup attribute (last-wins)", "[round_trip]")
{
  // Parser in non-strict mode uses last-wins for duplicate unique attrs.
  // parse(serialize(parse(input))) == parse(input) still holds.
  auto sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=sendrecv\r\n"
    "a=setup:actpass\r\n"
    "a=setup:active\r\n"
    "a=rtpmap:111 opus/48000/2\r\n";

  SdpParserOptions opts;
  auto r1 = SdpParser::parse(sdp, opts);
  REQUIRE(r1.hasValue());
  REQUIRE(r1.value->mediaDescriptions[0].setup == SetupRole::ACTIVE);

  auto serialized = SdpSerializer::serialize(*r1.value);
  auto r2 = SdpParser::parse(serialized, opts);
  REQUIRE(r2.hasValue());
  REQUIRE(*r1.value == *r2.value);
}

TEST_CASE("Round-trip: legacy sctpmap as GenericAttribute", "[round_trip]")
{
  // Parser has no typed handler for sctpmap; round-trips as GenericAttribute
  auto sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=application 9 DTLS/SCTP 5000\r\n"
    "a=sendrecv\r\n"
    "a=sctpmap:5000 webrtc-datachannel 1024\r\n";

  auto r1 = SdpParser::parse(sdp);
  REQUIRE(r1.hasValue());
  auto& ga = r1.value->mediaDescriptions[0].genericAttributes;
  REQUIRE(ga.size() == 1);
  REQUIRE(ga[0].name == "sctpmap");
  REQUIRE(ga[0].value == "5000 webrtc-datachannel 1024");

  auto serialized = SdpSerializer::serialize(*r1.value);
  auto r2 = SdpParser::parse(serialized);
  REQUIRE(r2.hasValue());
  REQUIRE(*r1.value == *r2.value);
}

TEST_CASE("Round-trip: very long attribute line", "[round_trip]")
{
  // Default maxLineLength is 4096; build a line under that
  std::string longVal(3000, 'x');
  auto sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=sendrecv\r\n"
    "a=x-long-value:" + longVal + "\r\n";

  auto r1 = SdpParser::parse(sdp);
  REQUIRE(r1.hasValue());
  auto& ga = r1.value->mediaDescriptions[0].genericAttributes;
  REQUIRE(ga.size() == 1);
  REQUIRE(ga[0].name == "x-long-value");
  REQUIRE(ga[0].value == longVal);

  auto serialized = SdpSerializer::serialize(*r1.value);
  auto r2 = SdpParser::parse(serialized);
  REQUIRE(r2.hasValue());
  REQUIRE(*r1.value == *r2.value);
}

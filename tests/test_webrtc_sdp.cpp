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
// T1: Chrome M120+ Unified Plan offer SDP
// ---------------------------------------------------------------------------

static const char* kChromeSdp =
  "v=0\r\n"
  "o=- 4611731400430051336 2 IN IP4 127.0.0.1\r\n"
  "s=-\r\n"
  "t=0 0\r\n"
  "a=ice-options:trickle\r\n"
  "a=fingerprint:sha-256 D1:A2:B3:C4:E5:F6:07:18:29:3A:4B:5C:6D:7E:8F:90:A1:B2:C3:D4:E5:F6:07:18:29:3A:4B:5C:6D:7E:8F:90\r\n"
  "a=extmap-allow-mixed\r\n"
  "a=group:BUNDLE 0 1\r\n"
  "m=audio 9 UDP/TLS/RTP/SAVPF 111 0 110\r\n"
  "c=IN IP4 0.0.0.0\r\n"
  "a=sendrecv\r\n"
  "a=mid:0\r\n"
  "a=msid:stream0 audio0\r\n"
  "a=ice-ufrag:abcd\r\n"
  "a=ice-pwd:aabbccddee11223344556677\r\n"
  "a=fingerprint:sha-256 D1:A2:B3:C4:E5:F6:07:18:29:3A:4B:5C:6D:7E:8F:90:A1:B2:C3:D4:E5:F6:07:18:29:3A:4B:5C:6D:7E:8F:90\r\n"
  "a=setup:actpass\r\n"
  "a=rtcp-mux\r\n"
  "a=rtcp-rsize\r\n"
  "a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\n"
  "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\n"
  "a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\n"
  "a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid\r\n"
  "a=rtpmap:111 opus/48000/2\r\n"
  "a=fmtp:111 minptime=10;useinbandfec=1\r\n"
  "a=rtcp-fb:111 transport-cc\r\n"
  "a=rtpmap:0 PCMU/8000\r\n"
  "a=rtpmap:110 telephone-event/8000\r\n"
  "a=fmtp:110 0-16\r\n"
  "a=ssrc:3456789012 cname:user@example.com\r\n"
  "a=candidate:1 1 udp 2113937151 192.168.1.100 5000 typ host\r\n"
  "a=candidate:2 1 udp 1845501695 203.0.113.1 6000 typ srflx raddr 192.168.1.100 rport 5000\r\n"
  "m=video 9 UDP/TLS/RTP/SAVPF 96 97 98 99\r\n"
  "c=IN IP4 0.0.0.0\r\n"
  "a=sendrecv\r\n"
  "a=mid:1\r\n"
  "a=msid:stream0 video0\r\n"
  "a=ice-ufrag:abcd\r\n"
  "a=ice-pwd:aabbccddee11223344556677\r\n"
  "a=fingerprint:sha-256 D1:A2:B3:C4:E5:F6:07:18:29:3A:4B:5C:6D:7E:8F:90:A1:B2:C3:D4:E5:F6:07:18:29:3A:4B:5C:6D:7E:8F:90\r\n"
  "a=setup:actpass\r\n"
  "a=rtcp-mux\r\n"
  "a=rtcp-rsize\r\n"
  "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\n"
  "a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\n"
  "a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid\r\n"
  "a=extmap:5 urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id\r\n"
  "a=extmap:6 urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id\r\n"
  "a=rtpmap:96 VP8/90000\r\n"
  "a=rtcp-fb:96 goog-remb\r\n"
  "a=rtcp-fb:96 transport-cc\r\n"
  "a=rtcp-fb:96 ccm fir\r\n"
  "a=rtcp-fb:96 nack\r\n"
  "a=rtcp-fb:96 nack pli\r\n"
  "a=rtpmap:97 VP9/90000\r\n"
  "a=fmtp:97 profile-id=0\r\n"
  "a=rtcp-fb:97 goog-remb\r\n"
  "a=rtcp-fb:97 transport-cc\r\n"
  "a=rtcp-fb:97 ccm fir\r\n"
  "a=rtcp-fb:97 nack\r\n"
  "a=rtcp-fb:97 nack pli\r\n"
  "a=rtpmap:98 H264/90000\r\n"
  "a=fmtp:98 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f\r\n"
  "a=rtcp-fb:98 goog-remb\r\n"
  "a=rtcp-fb:98 transport-cc\r\n"
  "a=rtcp-fb:98 ccm fir\r\n"
  "a=rtcp-fb:98 nack\r\n"
  "a=rtcp-fb:98 nack pli\r\n"
  "a=rtpmap:99 H264/90000\r\n"
  "a=fmtp:99 level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42e01f\r\n"
  "a=rtcp-fb:99 goog-remb\r\n"
  "a=rtcp-fb:99 transport-cc\r\n"
  "a=rtcp-fb:99 ccm fir\r\n"
  "a=rtcp-fb:99 nack\r\n"
  "a=rtcp-fb:99 nack pli\r\n"
  "a=ssrc:1234567890 cname:user@example.com\r\n"
  "a=candidate:1 1 udp 2113937151 192.168.1.100 5002 typ host\r\n"
  "a=candidate:2 1 udp 1845501695 203.0.113.1 6002 typ srflx raddr 192.168.1.100 rport 5002\r\n";

TEST_CASE("WebRTC: Chrome M120+ Unified Plan offer", "[webrtc_sdp]")
{
  auto r = SdpParser::parse(kChromeSdp);
  REQUIRE(r.hasValue());
  REQUIRE_FALSE(r.hasError());
  REQUIRE(r.warnings.empty());

  auto& sd = *r.value;

  SECTION("session level")
  {
    REQUIRE(sd.sessionName == "-");
    REQUIRE(sd.extmapAllowMixed);
    REQUIRE(sd.iceOptions.size() == 1);
    REQUIRE(sd.iceOptions[0] == "trickle");
    REQUIRE(sd.fingerprints.size() == 1);
    REQUIRE(sd.fingerprints[0].hashFunction == "sha-256");
    REQUIRE(sd.groups.size() == 1);
    REQUIRE(sd.groups[0].semantics == "BUNDLE");
    REQUIRE(sd.groups[0].mids == std::vector<std::string>{"0", "1"});
  }

  SECTION("audio section")
  {
    REQUIRE(sd.mediaDescriptions.size() == 2);
    auto& audio = sd.mediaDescriptions[0];
    REQUIRE(audio.mediaType == MediaType::AUDIO);
    REQUIRE(audio.protocol == TransportProtocol::UDP_TLS_RTP_SAVPF);
    REQUIRE(audio.formats == std::vector<std::string>{"111", "0", "110"});
    REQUIRE(audio.mid == "0");
    REQUIRE(audio.direction == Direction::SENDRECV);
    REQUIRE(audio.rtcpMux);
    REQUIRE(audio.rtcpRsize);
    REQUIRE(audio.setup == SetupRole::ACTPASS);
    REQUIRE(audio.iceUfrag == "abcd");
    REQUIRE(audio.icePwd == "aabbccddee11223344556677");
    REQUIRE(audio.rtpMaps.size() == 3);
    REQUIRE(audio.rtpMaps[0].encodingName == "opus");
    REQUIRE(audio.rtpMaps[0].clockRate == 48000);
    REQUIRE(audio.rtpMaps[0].channels == 2);
    REQUIRE(audio.fmtps.size() == 2);
    REQUIRE(audio.rtcpFeedbacks.size() == 1);
    REQUIRE(audio.extMaps.size() == 4);
    REQUIRE(audio.ssrcs.size() == 1);
    REQUIRE(audio.ssrcs[0].attributeName == "cname");
    REQUIRE(audio.candidates.size() == 2);
    REQUIRE(audio.msid.size() == 1);
    REQUIRE(audio.msid[0].streamId == "stream0");
    REQUIRE(audio.msid[0].trackId == "audio0");
  }

  SECTION("video section")
  {
    auto& video = sd.mediaDescriptions[1];
    REQUIRE(video.mediaType == MediaType::VIDEO);
    REQUIRE(video.formats == std::vector<std::string>{"96", "97", "98", "99"});
    REQUIRE(video.mid == "1");
    REQUIRE(video.direction == Direction::SENDRECV);
    REQUIRE(video.rtcpMux);
    REQUIRE(video.rtcpRsize);
    REQUIRE(video.setup == SetupRole::ACTPASS);
    REQUIRE(video.iceUfrag == "abcd");
    REQUIRE(video.icePwd == "aabbccddee11223344556677");
    REQUIRE(video.fingerprints.size() == 1);
    REQUIRE(video.rtpMaps.size() == 4);
    REQUIRE(video.rtpMaps[0].encodingName == "VP8");
    REQUIRE(video.rtpMaps[1].encodingName == "VP9");
    REQUIRE(video.rtpMaps[2].encodingName == "H264");
    REQUIRE(video.rtpMaps[3].encodingName == "H264");
    REQUIRE(video.fmtps.size() == 3);
    REQUIRE(video.rtcpFeedbacks.size() == 20);
    REQUIRE(video.extMaps.size() == 5);
    REQUIRE(video.extMaps[0].uri == "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time");
    REQUIRE(video.extMaps[3].uri == "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id");
    REQUIRE(video.extMaps[4].uri == "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id");
    REQUIRE(video.ssrcs.size() == 1);
    REQUIRE(video.ssrcs[0].attributeName == "cname");
    REQUIRE(video.candidates.size() == 2);
  }

  SECTION("fmtp parameter values")
  {
    auto& audio = sd.mediaDescriptions[0];
    // opus fmtp
    REQUIRE(audio.fmtps[0].payloadType == 111);
    REQUIRE(audio.fmtps[0].parameterMap.at("useinbandfec") == "1");
    REQUIRE(audio.fmtps[0].parameterMap.at("minptime") == "10");

    auto& video = sd.mediaDescriptions[1];
    // VP9 fmtp
    REQUIRE(video.fmtps[0].payloadType == 97);
    REQUIRE(video.fmtps[0].parameterMap.at("profile-id") == "0");
    // H264 profile 42001f
    REQUIRE(video.fmtps[1].payloadType == 98);
    REQUIRE(video.fmtps[1].parameterMap.at("profile-level-id") == "42001f");
    REQUIRE(video.fmtps[1].parameterMap.at("packetization-mode") == "1");
    REQUIRE(video.fmtps[1].parameterMap.at("level-asymmetry-allowed") == "1");
    // H264 profile 42e01f
    REQUIRE(video.fmtps[2].payloadType == 99);
    REQUIRE(video.fmtps[2].parameterMap.at("profile-level-id") == "42e01f");
    REQUIRE(video.fmtps[2].parameterMap.at("packetization-mode") == "0");
  }

  SECTION("round-trip")
  {
    roundTrip(kChromeSdp);
  }
}

// ---------------------------------------------------------------------------
// T2: Firefox offer SDP
// ---------------------------------------------------------------------------

static const char* kFirefoxSdp =
  "v=0\r\n"
  "o=mozilla...THIS_IS_SDPARTA-99.0 820location969135location3 0 IN IP4 0.0.0.0\r\n"
  "s=-\r\n"
  "t=0 0\r\n"
  "a=fingerprint:sha-256 A1:B2:C3:D4:E5:F6:07:18:29:3A:4B:5C:6D:7E:8F:90:A1:B2:C3:D4:E5:F6:07:18:29:3A:4B:5C:6D:7E:8F:90\r\n"
  "a=ice-options:trickle\r\n"
  "a=extmap-allow-mixed\r\n"
  "a=group:BUNDLE 0 1\r\n"
  "m=audio 9 UDP/TLS/RTP/SAVPF 109 9 0 8 101\r\n"
  "c=IN IP4 0.0.0.0\r\n"
  "a=ice-ufrag:ffufrag1\r\n"
  "a=ice-pwd:ffpwd12345678901234567890\r\n"
  "a=fingerprint:sha-256 A1:B2:C3:D4:E5:F6:07:18:29:3A:4B:5C:6D:7E:8F:90:A1:B2:C3:D4:E5:F6:07:18:29:3A:4B:5C:6D:7E:8F:90\r\n"
  "a=setup:actpass\r\n"
  "a=sendrecv\r\n"
  "a=mid:0\r\n"
  "a=msid:{stream-id-001} {track-id-audio}\r\n"
  "a=rtcp-mux\r\n"
  "a=rtcp-rsize\r\n"
  "a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\n"
  "a=extmap:2/recvonly urn:ietf:params:rtp-hdrext:csrc-audio-level\r\n"
  "a=extmap:3 urn:ietf:params:rtp-hdrext:sdes:mid\r\n"
  "a=rtpmap:109 opus/48000/2\r\n"
  "a=fmtp:109 maxplaybackrate=48000;stereo=1;useinbandfec=1\r\n"
  "a=rtpmap:9 G722/8000\r\n"
  "a=rtpmap:0 PCMU/8000\r\n"
  "a=rtpmap:8 PCMA/8000\r\n"
  "a=rtpmap:101 telephone-event/8000\r\n"
  "a=fmtp:101 0-15\r\n"
  "a=ssrc:1111111111 cname:{cname-001}\r\n"
  "a=candidate:0 1 udp 2130706431 10.0.0.1 50000 typ host\r\n"
  "m=video 9 UDP/TLS/RTP/SAVPF 120 124 121 125\r\n"
  "c=IN IP4 0.0.0.0\r\n"
  "a=ice-ufrag:ffufrag1\r\n"
  "a=ice-pwd:ffpwd12345678901234567890\r\n"
  "a=fingerprint:sha-256 A1:B2:C3:D4:E5:F6:07:18:29:3A:4B:5C:6D:7E:8F:90:A1:B2:C3:D4:E5:F6:07:18:29:3A:4B:5C:6D:7E:8F:90\r\n"
  "a=setup:actpass\r\n"
  "a=sendrecv\r\n"
  "a=mid:1\r\n"
  "a=msid:{stream-id-001} {track-id-video}\r\n"
  "a=rtcp-mux\r\n"
  "a=rtcp-rsize\r\n"
  "a=extmap:3 urn:ietf:params:rtp-hdrext:sdes:mid\r\n"
  "a=extmap:4 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\n"
  "a=extmap:5 urn:ietf:params:rtp-hdrext:toffset\r\n"
  "a=extmap:6 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\n"
  "a=extmap:7 urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id\r\n"
  "a=rtpmap:120 VP8/90000\r\n"
  "a=rtcp-fb:120 nack\r\n"
  "a=rtcp-fb:120 nack pli\r\n"
  "a=rtcp-fb:120 ccm fir\r\n"
  "a=rtcp-fb:120 goog-remb\r\n"
  "a=rtcp-fb:120 transport-cc\r\n"
  "a=rtpmap:124 rtx/90000\r\n"
  "a=fmtp:124 apt=120\r\n"
  "a=rtpmap:121 H264/90000\r\n"
  "a=fmtp:121 profile-level-id=42e01f;level-asymmetry-allowed=1;packetization-mode=1\r\n"
  "a=rtcp-fb:121 nack\r\n"
  "a=rtcp-fb:121 nack pli\r\n"
  "a=rtcp-fb:121 ccm fir\r\n"
  "a=rtcp-fb:121 goog-remb\r\n"
  "a=rtcp-fb:121 transport-cc\r\n"
  "a=rtpmap:125 rtx/90000\r\n"
  "a=fmtp:125 apt=121\r\n"
  "a=ssrc:2222222222 cname:{cname-001}\r\n"
  "a=ssrc:3333333333 cname:{cname-001}\r\n"
  "a=ssrc-group:FID 2222222222 3333333333\r\n"
  "a=candidate:0 1 udp 2130706431 10.0.0.1 50002 typ host\r\n";

TEST_CASE("WebRTC: Firefox offer", "[webrtc_sdp]")
{
  auto r = SdpParser::parse(kFirefoxSdp);
  REQUIRE(r.hasValue());
  REQUIRE_FALSE(r.hasError());
  REQUIRE(r.warnings.empty());

  auto& sd = *r.value;

  SECTION("session level")
  {
    REQUIRE(sd.groups.size() == 1);
    REQUIRE(sd.groups[0].semantics == "BUNDLE");
    REQUIRE(sd.groups[0].mids == std::vector<std::string>{"0", "1"});
    REQUIRE(sd.extmapAllowMixed);
  }

  SECTION("audio section")
  {
    auto& audio = sd.mediaDescriptions[0];
    REQUIRE(audio.mediaType == MediaType::AUDIO);
    REQUIRE(audio.formats == std::vector<std::string>{"109", "9", "0", "8", "101"});
    REQUIRE(audio.mid == "0");
    REQUIRE(audio.setup == SetupRole::ACTPASS);
    REQUIRE(audio.rtcpMux);
    REQUIRE(audio.rtcpRsize);
    REQUIRE(audio.iceUfrag == "ffufrag1");
    REQUIRE(audio.icePwd == "ffpwd12345678901234567890");
    REQUIRE(audio.msid.size() == 1);
    REQUIRE(audio.msid[0].streamId == "{stream-id-001}");
    REQUIRE(audio.msid[0].trackId == "{track-id-audio}");
    REQUIRE(audio.rtpMaps.size() == 5);
    REQUIRE(audio.rtpMaps[0].encodingName == "opus");
    REQUIRE(audio.extMaps.size() == 3);
    // Firefox extmap with direction
    REQUIRE(audio.extMaps[1].direction == Direction::RECVONLY);
    REQUIRE(audio.extMaps[1].uri == "urn:ietf:params:rtp-hdrext:csrc-audio-level");
  }

  SECTION("video section with RTX and ssrc-group")
  {
    auto& video = sd.mediaDescriptions[1];
    REQUIRE(video.mediaType == MediaType::VIDEO);
    REQUIRE(video.formats == std::vector<std::string>{"120", "124", "121", "125"});
    REQUIRE(video.mid == "1");
    REQUIRE(video.setup == SetupRole::ACTPASS);
    REQUIRE(video.rtcpMux);
    REQUIRE(video.rtcpRsize);
    REQUIRE(video.iceUfrag == "ffufrag1");
    REQUIRE(video.rtpMaps.size() == 4);
    // RTX codecs
    REQUIRE(video.rtpMaps[1].encodingName == "rtx");
    REQUIRE(video.rtpMaps[3].encodingName == "rtx");
    REQUIRE(video.fmtps.size() == 3);
    // ssrc-group:FID
    REQUIRE(video.ssrcGroups.size() == 1);
    REQUIRE(video.ssrcGroups[0].semantics == "FID");
    REQUIRE(video.ssrcGroups[0].ssrcs.size() == 2);
    REQUIRE(video.ssrcs.size() == 2);
    REQUIRE(video.extMaps.size() == 5);
  }

  SECTION("round-trip")
  {
    roundTrip(kFirefoxSdp);
  }
}

// ---------------------------------------------------------------------------
// T3: Safari offer SDP
// ---------------------------------------------------------------------------

static const char* kSafariSdp =
  "v=0\r\n"
  "o=- 2846374location985 2 IN IP4 127.0.0.1\r\n"
  "s=-\r\n"
  "t=0 0\r\n"
  "a=ice-options:trickle\r\n"
  "a=fingerprint:sha-256 E1:F2:03:14:25:36:47:58:69:7A:8B:9C:AD:BE:CF:D0:E1:F2:03:14:25:36:47:58:69:7A:8B:9C:AD:BE:CF:D0\r\n"
  "a=extmap-allow-mixed\r\n"
  "a=group:BUNDLE 0 1\r\n"
  "m=audio 9 UDP/TLS/RTP/SAVPF 111 0\r\n"
  "c=IN IP4 0.0.0.0\r\n"
  "a=sendrecv\r\n"
  "a=mid:0\r\n"
  "a=msid:safari-stream safari-audio-track\r\n"
  "a=ice-ufrag:sfufrag1\r\n"
  "a=ice-pwd:sfpwd12345678901234567890\r\n"
  "a=fingerprint:sha-256 E1:F2:03:14:25:36:47:58:69:7A:8B:9C:AD:BE:CF:D0:E1:F2:03:14:25:36:47:58:69:7A:8B:9C:AD:BE:CF:D0\r\n"
  "a=setup:actpass\r\n"
  "a=rtcp-mux\r\n"
  "a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\n"
  "a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid\r\n"
  "a=rtpmap:111 opus/48000/2\r\n"
  "a=fmtp:111 minptime=10;useinbandfec=1\r\n"
  "a=rtpmap:0 PCMU/8000\r\n"
  "a=ssrc:4444444444 cname:safari-cname\r\n"
  "a=candidate:1 1 udp 2113937151 192.168.1.50 50000 typ host\r\n"
  "m=video 9 UDP/TLS/RTP/SAVPF 96 97\r\n"
  "c=IN IP4 0.0.0.0\r\n"
  "a=sendrecv\r\n"
  "a=mid:1\r\n"
  "a=msid:safari-stream safari-video-track\r\n"
  "a=ice-ufrag:sfufrag1\r\n"
  "a=ice-pwd:sfpwd12345678901234567890\r\n"
  "a=fingerprint:sha-256 E1:F2:03:14:25:36:47:58:69:7A:8B:9C:AD:BE:CF:D0:E1:F2:03:14:25:36:47:58:69:7A:8B:9C:AD:BE:CF:D0\r\n"
  "a=setup:actpass\r\n"
  "a=rtcp-mux\r\n"
  "a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid\r\n"
  "a=extmap:12 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\n"
  "a=rtpmap:96 H264/90000\r\n"
  "a=fmtp:96 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=640c1f\r\n"
  "a=rtcp-fb:96 nack\r\n"
  "a=rtcp-fb:96 nack pli\r\n"
  "a=rtcp-fb:96 ccm fir\r\n"
  "a=rtpmap:97 VP8/90000\r\n"
  "a=rtcp-fb:97 nack\r\n"
  "a=rtcp-fb:97 nack pli\r\n"
  "a=rtcp-fb:97 ccm fir\r\n"
  "a=ssrc:5555555555 cname:safari-cname\r\n"
  "a=candidate:1 1 udp 2113937151 192.168.1.50 50002 typ host\r\n";

TEST_CASE("WebRTC: Safari offer", "[webrtc_sdp]")
{
  auto r = SdpParser::parse(kSafariSdp);
  REQUIRE(r.hasValue());
  REQUIRE_FALSE(r.hasError());

  auto& sd = *r.value;
  REQUIRE(sd.mediaDescriptions.size() == 2);

  SECTION("session level")
  {
    REQUIRE(sd.groups.size() == 1);
    REQUIRE(sd.groups[0].semantics == "BUNDLE");
    REQUIRE(sd.groups[0].mids == std::vector<std::string>{"0", "1"});
    REQUIRE(sd.extmapAllowMixed);
    REQUIRE(sd.iceOptions == std::vector<std::string>{"trickle"});
    REQUIRE(sd.fingerprints.size() == 1);
    REQUIRE(sd.fingerprints[0].hashFunction == "sha-256");
  }

  SECTION("audio section")
  {
    auto& audio = sd.mediaDescriptions[0];
    REQUIRE(audio.mediaType == MediaType::AUDIO);
    REQUIRE(audio.formats == std::vector<std::string>{"111", "0"});
    REQUIRE(audio.mid == "0");
    REQUIRE(audio.setup == SetupRole::ACTPASS);
    REQUIRE(audio.rtcpMux);
    REQUIRE(audio.iceUfrag == "sfufrag1");
    REQUIRE(audio.icePwd == "sfpwd12345678901234567890");
    REQUIRE(audio.rtpMaps.size() == 2);
    REQUIRE(audio.rtpMaps[0].encodingName == "opus");
    REQUIRE(audio.rtpMaps[0].clockRate == 48000);
    REQUIRE(audio.rtpMaps[1].encodingName == "PCMU");
    REQUIRE(audio.extMaps.size() == 2);
    REQUIRE(audio.extMaps[0].uri == "urn:ietf:params:rtp-hdrext:ssrc-audio-level");
    REQUIRE(audio.extMaps[1].uri == "urn:ietf:params:rtp-hdrext:sdes:mid");
    REQUIRE(audio.fmtps.size() == 1);
    REQUIRE(audio.msid.size() == 1);
    REQUIRE(audio.msid[0].streamId == "safari-stream");
    REQUIRE(audio.msid[0].trackId == "safari-audio-track");
    REQUIRE(audio.ssrcs.size() == 1);
    REQUIRE(audio.ssrcs[0].attributeValue == "safari-cname");
  }

  SECTION("Safari prefers H264 over VP8 in video")
  {
    auto& video = sd.mediaDescriptions[1];
    REQUIRE(video.mediaType == MediaType::VIDEO);
    REQUIRE(video.formats[0] == "96");
    REQUIRE(video.rtpMaps[0].encodingName == "H264");
    REQUIRE(video.rtpMaps[1].encodingName == "VP8");
    REQUIRE(video.setup == SetupRole::ACTPASS);
    REQUIRE(video.rtcpMux);
    REQUIRE(video.extMaps.size() == 2);
    REQUIRE(video.rtcpFeedbacks.size() == 6);
    REQUIRE(video.msid.size() == 1);
    REQUIRE(video.msid[0].trackId == "safari-video-track");
  }

  SECTION("round-trip")
  {
    roundTrip(kSafariSdp);
  }
}

// ---------------------------------------------------------------------------
// T3: SIP phone SDPs (plain RTP/AVP and SDES)
// ---------------------------------------------------------------------------

TEST_CASE("WebRTC: SIP phone SDP (plain RTP/AVP)", "[webrtc_sdp]")
{
  auto sdp =
    "v=0\r\n"
    "o=user 53655765 2353687637 IN IP4 192.168.1.10\r\n"
    "s=SIP Call\r\n"
    "c=IN IP4 192.168.1.10\r\n"
    "t=0 0\r\n"
    "m=audio 49170 RTP/AVP 0 8 18 101\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtpmap:8 PCMA/8000\r\n"
    "a=rtpmap:18 G729/8000\r\n"
    "a=fmtp:18 annexb=no\r\n"
    "a=rtpmap:101 telephone-event/8000\r\n"
    "a=fmtp:101 0-16\r\n"
    "a=ptime:20\r\n";

  auto r = SdpParser::parse(sdp);
  REQUIRE(r.hasValue());

  auto& sd = *r.value;
  REQUIRE(sd.sessionName == "SIP Call");
  REQUIRE(sd.connection.has_value());
  REQUIRE(sd.connection->address == "192.168.1.10");
  REQUIRE(sd.mediaDescriptions.size() == 1);

  auto& audio = sd.mediaDescriptions[0];
  REQUIRE(audio.protocol == TransportProtocol::RTP_AVP);
  REQUIRE(audio.direction == Direction::SENDRECV);
  REQUIRE(audio.formats == std::vector<std::string>{"0", "8", "18", "101"});
  REQUIRE(audio.rtpMaps.size() == 4);
  REQUIRE(audio.rtpMaps[0].encodingName == "PCMU");
  REQUIRE(audio.rtpMaps[1].encodingName == "PCMA");
  REQUIRE(audio.rtpMaps[2].encodingName == "G729");
  REQUIRE(audio.rtpMaps[3].encodingName == "telephone-event");
  REQUIRE(audio.ptime == 20);

  // No WebRTC attributes
  REQUIRE_FALSE(audio.iceUfrag.has_value());
  REQUIRE_FALSE(audio.icePwd.has_value());
  REQUIRE(audio.fingerprints.empty());
  REQUIRE_FALSE(audio.rtcpMux);
  REQUIRE(sd.groups.empty());
  REQUIRE(audio.candidates.empty());

  // Verify no WebRTC injection on round-trip
  auto serialized = SdpSerializer::serialize(sd);
  REQUIRE(serialized.find("a=ice-ufrag") == std::string::npos);
  REQUIRE(serialized.find("a=rtcp-mux") == std::string::npos);
  REQUIRE(serialized.find("a=group:BUNDLE") == std::string::npos);

  roundTrip(sdp);
}

TEST_CASE("WebRTC: SIP phone SDP (SDES-SRTP)", "[webrtc_sdp]")
{
  auto sdp =
    "v=0\r\n"
    "o=user 53655765 2353687637 IN IP4 10.0.0.5\r\n"
    "s=SIP Call\r\n"
    "c=IN IP4 10.0.0.5\r\n"
    "t=0 0\r\n"
    "m=audio 49170 RTP/SAVP 0 8 101\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtpmap:8 PCMA/8000\r\n"
    "a=rtpmap:101 telephone-event/8000\r\n"
    "a=fmtp:101 0-16\r\n"
    "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:d0RmdmcmVCspeEc3QGZiNWpVLFJhQX1cfHAwJSoj\r\n"
    "a=crypto:2 AES_CM_128_HMAC_SHA1_32 inline:d0RmdmcmVCspeEc3QGZiNWpVLFJhQX1cfHAwJSoa\r\n"
    "a=ptime:20\r\n";

  auto r = SdpParser::parse(sdp);
  REQUIRE(r.hasValue());

  auto& audio = r.value->mediaDescriptions[0];
  REQUIRE(audio.protocol == TransportProtocol::RTP_SAVP);
  REQUIRE(audio.crypto.size() == 2);
  REQUIRE(audio.crypto[0].tag == 1);
  REQUIRE(audio.crypto[0].suite == "AES_CM_128_HMAC_SHA1_80");
  REQUIRE(audio.crypto[1].tag == 2);
  REQUIRE(audio.crypto[1].suite == "AES_CM_128_HMAC_SHA1_32");

  // No WebRTC attributes
  REQUIRE_FALSE(audio.iceUfrag.has_value());
  REQUIRE(audio.fingerprints.empty());

  roundTrip(sdp);
}

// ---------------------------------------------------------------------------
// T4: Minimal valid SDP
// ---------------------------------------------------------------------------

TEST_CASE("WebRTC: minimal valid SDP", "[webrtc_sdp]")
{
  auto sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n";

  auto r = SdpParser::parse(sdp);
  REQUIRE(r.hasValue());
  REQUIRE(r.value->mediaDescriptions.size() == 1);
  REQUIRE(r.value->mediaDescriptions[0].formats == std::vector<std::string>{"0"});

  roundTrip(sdp);
}

TEST_CASE("WebRTC: minimal WebRTC SDP", "[webrtc_sdp]")
{
  auto sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=group:BUNDLE 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=sendrecv\r\n"
    "a=mid:0\r\n"
    "a=ice-ufrag:abcd1234\r\n"
    "a=ice-pwd:aabbccddee11223344556677\r\n"
    "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
    "a=setup:actpass\r\n"
    "a=rtcp-mux\r\n"
    "a=rtpmap:111 opus/48000/2\r\n";

  auto r = SdpParser::parse(sdp);
  REQUIRE(r.hasValue());

  auto& audio = r.value->mediaDescriptions[0];
  REQUIRE(audio.mid == "0");
  REQUIRE(audio.iceUfrag == "abcd1234");
  REQUIRE(audio.rtcpMux);
  REQUIRE(audio.fingerprints.size() == 1);
  REQUIRE(audio.setup == SetupRole::ACTPASS);
  REQUIRE(r.value->groups.size() == 1);

  roundTrip(sdp);
}

// ---------------------------------------------------------------------------
// T4: Maximal SDP with every supported attribute
// ---------------------------------------------------------------------------

static const char* kMaximalSdp =
  "v=0\r\n"
  "o=admin 9876543210 1 IN IP4 10.0.0.1\r\n"
  "s=Maximal Test Session\r\n"
  "i=A session with every attribute type\r\n"
  "u=http://example.com/session\r\n"
  "e=admin@example.com\r\n"
  "p=+1-555-0100\r\n"
  "c=IN IP4 10.0.0.1\r\n"
  "b=CT:1000\r\n"
  "t=2873397496 2873404696\r\n"
  "r=7d 1h 0 25h\r\n"
  "t=0 0\r\n"
  "z=2882844526 -1h 2898848070 0\r\n"
  "k=base64:sessionkey123\r\n"
  "a=ice-ufrag:sessufrag\r\n"
  "a=ice-pwd:sesspwd1234567890123456\r\n"
  "a=ice-options:trickle ice2\r\n"
  "a=ice-lite\r\n"
  "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
  "a=fingerprint:sha-1 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD\r\n"
  "a=setup:actpass\r\n"
  "a=extmap-allow-mixed\r\n"
  "a=group:BUNDLE 0 1 2 3 4\r\n"
  "a=group:LS 0 1\r\n"
  "a=x-session-custom:session-value\r\n"
  // Audio section — SENDRECV with full features
  "m=audio 5004/2 UDP/TLS/RTP/SAVPF 111 0\r\n"
  "c=IN IP4 192.168.1.100\r\n"
  "b=AS:128\r\n"
  "b=TIAS:128000\r\n"
  "k=base64:mediakey456\r\n"
  "a=sendrecv\r\n"
  "a=mid:0\r\n"
  "a=msid:stream0 track0\r\n"
  "a=msid:stream1\r\n"
  "a=ice-ufrag:mediaufrag\r\n"
  "a=ice-pwd:mediapwd123456789012345\r\n"
  "a=ice-options:trickle\r\n"
  "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
  "a=setup:actpass\r\n"
  "a=extmap-allow-mixed\r\n"
  "a=rtcp-mux\r\n"
  "a=rtcp-mux-only\r\n"
  "a=rtcp-rsize\r\n"
  "a=rtcp:9 IN IP4 0.0.0.0\r\n"
  "a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\n"
  "a=extmap:2/sendonly http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\n"
  "a=rtpmap:111 opus/48000/2\r\n"
  "a=fmtp:111 minptime=10;useinbandfec=1\r\n"
  "a=rtcp-fb:111 transport-cc\r\n"
  "a=rtcp-fb:* nack\r\n"
  "a=rtpmap:0 PCMU/8000\r\n"
  "a=ptime:20\r\n"
  "a=maxptime:40\r\n"
  "a=ssrc:1111 cname:cname1\r\n"
  "a=ssrc:2222 cname:cname1\r\n"
  "a=ssrc-group:FID 1111 2222\r\n"
  "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:base64key1\r\n"
  "a=candidate:1 1 udp 2113937151 192.168.1.100 5004 typ host\r\n"
  "a=candidate:2 1 udp 1845501695 203.0.113.1 6004 typ srflx raddr 192.168.1.100 rport 5004\r\n"
  "a=candidate:3 1 udp 100 10.0.0.1 3478 typ relay raddr 203.0.113.1 rport 6004\r\n"
  "a=candidate:4 1 tcp 1518280447 192.168.1.100 9 typ host tcptype passive\r\n"
  "a=end-of-candidates\r\n"
  "a=x-media-custom:audio-value\r\n"
  // Video section — SENDONLY with simulcast
  "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
  "c=IN IP6 ::1\r\n"
  "a=sendonly\r\n"
  "a=mid:1\r\n"
  "a=msid:stream0 videotrack\r\n"
  "a=ice-ufrag:mediaufrag\r\n"
  "a=ice-pwd:mediapwd123456789012345\r\n"
  "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
  "a=setup:actpass\r\n"
  "a=rtcp-mux\r\n"
  "a=extmap:3 urn:ietf:params:rtp-hdrext:sdes:mid\r\n"
  "a=rtpmap:96 VP8/90000\r\n"
  "a=rtcp-fb:96 nack\r\n"
  "a=rtcp-fb:96 nack pli\r\n"
  "a=framerate:29.97\r\n"
  "a=simulcast:send h;m;l\r\n"
  "a=rid:h send pt=96;max-height=720;max-width=1280\r\n"
  "a=rid:m send pt=96;max-height=360;max-width=640\r\n"
  "a=rid:l send pt=96;max-height=180;max-width=320\r\n"
  // Audio section — RECVONLY
  "m=audio 9 UDP/TLS/RTP/SAVPF 0\r\n"
  "c=IN IP4 0.0.0.0\r\n"
  "a=recvonly\r\n"
  "a=mid:3\r\n"
  "a=ice-ufrag:mediaufrag\r\n"
  "a=ice-pwd:mediapwd123456789012345\r\n"
  "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
  "a=setup:actpass\r\n"
  "a=rtcp-mux\r\n"
  "a=rtpmap:0 PCMU/8000\r\n"
  // Audio section — INACTIVE
  "m=audio 0 UDP/TLS/RTP/SAVPF 0\r\n"
  "c=IN IP4 0.0.0.0\r\n"
  "a=inactive\r\n"
  "a=mid:4\r\n"
  "a=ice-ufrag:mediaufrag\r\n"
  "a=ice-pwd:mediapwd123456789012345\r\n"
  "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
  "a=setup:actpass\r\n"
  "a=rtcp-mux\r\n"
  "a=rtpmap:0 PCMU/8000\r\n"
  // Data channel section — APPLICATION
  "m=application 9 UDP/DTLS/SCTP webrtc-datachannel\r\n"
  "c=IN IP4 0.0.0.0\r\n"
  "a=sendrecv\r\n"
  "a=mid:2\r\n"
  "a=ice-ufrag:mediaufrag\r\n"
  "a=ice-pwd:mediapwd123456789012345\r\n"
  "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
  "a=setup:actpass\r\n"
  "a=sctp-port:5000\r\n"
  "a=max-message-size:262144\r\n";

TEST_CASE("WebRTC: maximal SDP with every attribute", "[webrtc_sdp]")
{
  auto r = SdpParser::parse(kMaximalSdp);
  REQUIRE(r.hasValue());
  REQUIRE_FALSE(r.hasError());

  auto& sd = *r.value;

  SECTION("session-level optional fields")
  {
    REQUIRE(sd.sessionName == "Maximal Test Session");
    REQUIRE(sd.sessionInfo == "A session with every attribute type");
    REQUIRE(sd.uri == "http://example.com/session");
    REQUIRE(sd.emailAddress == "admin@example.com");
    REQUIRE(sd.phoneNumber == "+1-555-0100");
    REQUIRE(sd.connection.has_value());
    REQUIRE(sd.connection->address == "10.0.0.1");
    REQUIRE(sd.bandwidths.size() == 1);
    REQUIRE(sd.bandwidths[0].type == BandwidthType::CT);
    REQUIRE(sd.encryptionKey == "base64:sessionkey123");
  }

  SECTION("session-level timing")
  {
    REQUIRE(sd.timeDescriptions.size() == 2);
    REQUIRE(sd.timeDescriptions[0].startTime == 2873397496);
    REQUIRE(sd.timeDescriptions[0].stopTime == 2873404696);
    REQUIRE(sd.timeDescriptions[0].repeatTimes.size() == 1);
    REQUIRE(sd.timeDescriptions[0].repeatTimes[0].repeatInterval == "7d");
    REQUIRE(sd.timeDescriptions[1].startTime == 0);
    REQUIRE(sd.zoneAdjustments.size() == 2);
  }

  SECTION("session-level ICE/DTLS")
  {
    REQUIRE(sd.iceUfrag == "sessufrag");
    REQUIRE(sd.icePwd == "sesspwd1234567890123456");
    REQUIRE(sd.iceOptions == std::vector<std::string>{"trickle", "ice2"});
    REQUIRE(sd.iceLite);
    REQUIRE(sd.fingerprints.size() == 2);
    REQUIRE(sd.setup == SetupRole::ACTPASS);
    REQUIRE(sd.extmapAllowMixed);
  }

  SECTION("session-level groups and generic attrs")
  {
    REQUIRE(sd.groups.size() == 2);
    REQUIRE(sd.groups[0].semantics == "BUNDLE");
    REQUIRE(sd.groups[0].mids == std::vector<std::string>{"0", "1", "2", "3", "4"});
    REQUIRE(sd.groups[1].semantics == "LS");
    REQUIRE(sd.groups[1].mids == std::vector<std::string>{"0", "1"});
    REQUIRE(sd.attributes.size() == 1);
    REQUIRE(sd.attributes[0].name == "x-session-custom");
  }

  SECTION("audio section — full features")
  {
    REQUIRE(sd.mediaDescriptions.size() == 5);
    auto& a = sd.mediaDescriptions[0];
    REQUIRE(a.mediaType == MediaType::AUDIO);
    REQUIRE(a.port == 5004);
    REQUIRE(a.numberOfPorts == 2);
    REQUIRE(a.mid == "0");
    REQUIRE(a.direction == Direction::SENDRECV);
    REQUIRE(a.connection.has_value());
    REQUIRE(a.connection->address == "192.168.1.100");
    REQUIRE(a.bandwidths.size() == 2);
    REQUIRE(a.encryptionKey == "base64:mediakey456");
    REQUIRE(a.iceUfrag == "mediaufrag");
    REQUIRE(a.icePwd == "mediapwd123456789012345");
    REQUIRE(a.iceOptions == std::vector<std::string>{"trickle"});
    REQUIRE(a.fingerprints.size() == 1);
    REQUIRE(a.setup == SetupRole::ACTPASS);
    REQUIRE(a.extmapAllowMixed);
    REQUIRE(a.rtcpMux);
    REQUIRE(a.rtcpMuxOnly);
    REQUIRE(a.rtcpRsize);
    REQUIRE(a.rtcp.has_value());
    REQUIRE(a.rtcp->port == 9);
    REQUIRE(a.rtcp->address == "0.0.0.0");
    REQUIRE(a.extMaps.size() == 2);
    REQUIRE(a.extMaps[1].direction == Direction::SENDONLY);
    REQUIRE(a.rtpMaps.size() == 2);
    REQUIRE(a.fmtps.size() == 1);
    REQUIRE(a.rtcpFeedbacks.size() == 2); // 111 transport-cc + * nack
    REQUIRE(a.rtcpFeedbacks[1].payloadType == "*");
    REQUIRE(a.ptime == 20);
    REQUIRE(a.maxptime == 40);
    REQUIRE(a.msid.size() == 2);
    REQUIRE(a.msid[0].trackId == "track0");
    REQUIRE_FALSE(a.msid[1].trackId.has_value());
    REQUIRE(a.ssrcs.size() == 2);
    REQUIRE(a.ssrcGroups.size() == 1);
    REQUIRE(a.crypto.size() == 1);
    REQUIRE(a.candidates.size() == 4);
    REQUIRE(a.candidates[2].type == IceCandidateType::RELAY);
    REQUIRE(a.candidates[2].relAddr == "203.0.113.1");
    REQUIRE(a.candidates[3].transport == IceTransportType::TCP);
    REQUIRE(a.candidates[3].tcpType == "passive");
    REQUIRE(a.endOfCandidates);
    REQUIRE(a.genericAttributes.size() == 1);
    REQUIRE(a.genericAttributes[0].name == "x-media-custom");
  }

  SECTION("video section — simulcast")
  {
    auto& v = sd.mediaDescriptions[1];
    REQUIRE(v.mediaType == MediaType::VIDEO);
    REQUIRE(v.direction == Direction::SENDONLY);
    REQUIRE(v.rtcpMux);
    REQUIRE(v.setup == SetupRole::ACTPASS);
    REQUIRE(v.iceUfrag == "mediaufrag");
    REQUIRE(v.fingerprints.size() == 1);
    REQUIRE(v.connection.has_value());
    REQUIRE(v.connection->addrType == AddressType::IP6);
    REQUIRE(v.connection->address == "::1");
    REQUIRE(v.framerate.has_value());
    REQUIRE(*v.framerate == Approx(29.97));
    REQUIRE(v.simulcast.has_value());
    REQUIRE(v.simulcast->sendStreams.size() == 3);
    REQUIRE(v.rids.size() == 3);
    REQUIRE(v.rids[0].id == "h");
  }

  SECTION("recvonly section")
  {
    auto& rv = sd.mediaDescriptions[2];
    REQUIRE(rv.mediaType == MediaType::AUDIO);
    REQUIRE(rv.direction == Direction::RECVONLY);
    REQUIRE(rv.mid == "3");
    REQUIRE(rv.rtcpMux);
    REQUIRE(rv.setup == SetupRole::ACTPASS);
  }

  SECTION("inactive section")
  {
    auto& in = sd.mediaDescriptions[3];
    REQUIRE(in.mediaType == MediaType::AUDIO);
    REQUIRE(in.direction == Direction::INACTIVE);
    REQUIRE(in.mid == "4");
    REQUIRE(in.port == 0);
    REQUIRE(in.rtcpMux);
    REQUIRE(in.setup == SetupRole::ACTPASS);
  }

  SECTION("data channel section")
  {
    auto& d = sd.mediaDescriptions[4];
    REQUIRE(d.mediaType == MediaType::APPLICATION);
    REQUIRE(d.protocol == TransportProtocol::UDP_DTLS_SCTP);
    REQUIRE(d.formats == std::vector<std::string>{"webrtc-datachannel"});
    REQUIRE(d.sctpPort == 5000);
    REQUIRE(d.maxMessageSize == 262144);
    REQUIRE(d.mid == "2");
    REQUIRE(d.direction == Direction::SENDRECV);
    REQUIRE(d.iceUfrag == "mediaufrag");
    REQUIRE(d.icePwd == "mediapwd123456789012345");
    REQUIRE(d.fingerprints.size() == 1);
    REQUIRE(d.setup == SetupRole::ACTPASS);
  }

  SECTION("round-trip")
  {
    roundTrip(kMaximalSdp);
  }
}

// ---------------------------------------------------------------------------
// T5: Data channel only SDP
// ---------------------------------------------------------------------------

TEST_CASE("WebRTC: data channel only SDP (modern)", "[webrtc_sdp]")
{
  auto sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=group:BUNDLE 0\r\n"
    "m=application 9 UDP/DTLS/SCTP webrtc-datachannel\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=sendrecv\r\n"
    "a=mid:0\r\n"
    "a=ice-ufrag:dcufrag1\r\n"
    "a=ice-pwd:dcpwd12345678901234567890\r\n"
    "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
    "a=setup:actpass\r\n"
    "a=sctp-port:5000\r\n"
    "a=max-message-size:262144\r\n";

  auto r = SdpParser::parse(sdp);
  REQUIRE(r.hasValue());
  REQUIRE(r.value->mediaDescriptions.size() == 1);

  auto& dc = r.value->mediaDescriptions[0];
  REQUIRE(dc.mediaType == MediaType::APPLICATION);
  REQUIRE(dc.protocol == TransportProtocol::UDP_DTLS_SCTP);
  REQUIRE(dc.formats == std::vector<std::string>{"webrtc-datachannel"});
  REQUIRE(dc.sctpPort == 5000);
  REQUIRE(dc.maxMessageSize == 262144);
  REQUIRE(dc.mid == "0");
  REQUIRE(dc.iceUfrag == "dcufrag1");
  REQUIRE(dc.fingerprints.size() == 1);
  REQUIRE(dc.setup == SetupRole::ACTPASS);

  // No audio or video
  REQUIRE(r.value->groups.size() == 1);
  REQUIRE(r.value->groups[0].mids == std::vector<std::string>{"0"});

  roundTrip(sdp);
}

TEST_CASE("WebRTC: data channel only SDP (legacy DTLS/SCTP)", "[webrtc_sdp]")
{
  auto sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=group:BUNDLE data\r\n"
    "m=application 9 DTLS/SCTP 5000\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=sendrecv\r\n"
    "a=mid:data\r\n"
    "a=ice-ufrag:dcufrag2\r\n"
    "a=ice-pwd:dcpwd12345678901234567890\r\n"
    "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
    "a=setup:actpass\r\n"
    "a=sctpmap:5000 webrtc-datachannel 1024\r\n";

  auto r = SdpParser::parse(sdp);
  REQUIRE(r.hasValue());
  REQUIRE(r.value->mediaDescriptions.size() == 1);

  auto& dc = r.value->mediaDescriptions[0];
  REQUIRE(dc.mediaType == MediaType::APPLICATION);
  REQUIRE(dc.protocol == TransportProtocol::DTLS_SCTP);
  REQUIRE(dc.formats == std::vector<std::string>{"5000"});
  REQUIRE(dc.mid == "data");

  // sctpmap parsed as GenericAttribute (no typed handler)
  REQUIRE(dc.genericAttributes.size() == 1);
  REQUIRE(dc.genericAttributes[0].name == "sctpmap");
  REQUIRE(dc.genericAttributes[0].value == "5000 webrtc-datachannel 1024");

  roundTrip(sdp);
}

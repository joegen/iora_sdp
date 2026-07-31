#include <catch2/catch.hpp>
#include "iora/sdp/sdp_parser.hpp"

using namespace iora::sdp;

// =========================================================================
// Helper: build a minimal valid SDP string
// =========================================================================

static const char* kMinimalSdp =
  "v=0\r\n"
  "o=- 0 0 IN IP4 0.0.0.0\r\n"
  "s=-\r\n"
  "t=0 0\r\n";

// =========================================================================
// T1 — ParseResult, ParseError, ParseWarning, SdpParserOptions
// =========================================================================

TEST_CASE("ParseError: equality", "[parser][ParseError]")
{
  ParseError a{1, "bad"};
  ParseError b{1, "bad"};
  ParseError c{2, "bad"};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

TEST_CASE("ParseWarning: equality", "[parser][ParseWarning]")
{
  ParseWarning a{1, "warn"};
  ParseWarning b{1, "warn"};
  ParseWarning c{1, "other"};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

TEST_CASE("ParseResult: hasValue and hasError", "[parser][ParseResult]")
{
  ParseResult<int> r;
  REQUIRE(!r.hasValue());
  REQUIRE(!r.hasError());

  r.value = 42;
  REQUIRE(r.hasValue());

  r.error = ParseError{1, "oops"};
  REQUIRE(r.hasError());
}

TEST_CASE("SdpParserOptions: default values", "[parser][SdpParserOptions]")
{
  SdpParserOptions opts;
  REQUIRE(opts.strict == false);
  REQUIRE(opts.requireVersion == true);
  REQUIRE(opts.requireOrigin == true);
  REQUIRE(opts.requireSessionName == true);
  REQUIRE(opts.requireTiming == false);
  REQUIRE(opts.maxLineLength == 4096);
  REQUIRE(opts.maxMediaSections == 64);
}

TEST_CASE("SdpParserOptions: equality", "[parser][SdpParserOptions]")
{
  SdpParserOptions a;
  SdpParserOptions b;
  REQUIRE(a == b);

  SdpParserOptions c;
  c.strict = true;
  REQUIRE(a != c);
}

// =========================================================================
// T3 — Session-level field parsing
// =========================================================================

TEST_CASE("Parser: minimal valid SDP", "[parser]")
{
  auto result = SdpParser::parse(kMinimalSdp);
  REQUIRE(result.hasValue());
  REQUIRE(!result.hasError());
  REQUIRE(result.warnings.empty());

  auto& sd = *result.value;
  REQUIRE(sd.version == 0);
  REQUIRE(sd.origin.username == "-");
  REQUIRE(sd.origin.sessId == "0");
  REQUIRE(sd.origin.sessVersion == "0");
  REQUIRE(sd.origin.netType == NetworkType::IN);
  REQUIRE(sd.origin.addrType == AddressType::IP4);
  REQUIRE(sd.origin.address == "0.0.0.0");
  REQUIRE(sd.sessionName == "-");
  REQUIRE(sd.timeDescriptions.size() == 1);
  REQUIRE(sd.timeDescriptions[0].startTime == 0);
  REQUIRE(sd.timeDescriptions[0].stopTime == 0);
}

TEST_CASE("Parser: session-level optional fields (i, u, e, p, k)", "[parser]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=Test Session\r\n"
    "i=Session information\r\n"
    "u=http://example.com\r\n"
    "e=user@example.com\r\n"
    "p=+1-555-0100\r\n"
    "k=prompt\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());

  auto& sd = *result.value;
  REQUIRE(sd.sessionName == "Test Session");
  REQUIRE(sd.sessionInfo == "Session information");
  REQUIRE(sd.uri == "http://example.com");
  REQUIRE(sd.emailAddress == "user@example.com");
  REQUIRE(sd.phoneNumber == "+1-555-0100");
  REQUIRE(sd.encryptionKey == "prompt");
}

TEST_CASE("Parser: session-level c= line", "[parser]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "c=IN IP4 192.168.1.1\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->connection.has_value());
  REQUIRE(result.value->connection->netType == NetworkType::IN);
  REQUIRE(result.value->connection->addrType == AddressType::IP4);
  REQUIRE(result.value->connection->address == "192.168.1.1");
}

TEST_CASE("Parser: session-level b= lines", "[parser]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "b=AS:1000\r\n"
    "b=TIAS:500000\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->bandwidths.size() == 2);
  REQUIRE(result.value->bandwidths[0].type == BandwidthType::AS);
  REQUIRE(result.value->bandwidths[0].bandwidth == 1000);
  REQUIRE(result.value->bandwidths[1].type == BandwidthType::TIAS);
  REQUIRE(result.value->bandwidths[1].bandwidth == 500000);
}

TEST_CASE("Parser: t= and r= lines", "[parser]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=3034423619 3034430819\r\n"
    "r=604800 3600 0 90000\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& tds = result.value->timeDescriptions;
  REQUIRE(tds.size() == 2);
  REQUIRE(tds[0].startTime == 3034423619);
  REQUIRE(tds[0].stopTime == 3034430819);
  REQUIRE(tds[0].repeatTimes.size() == 1);
  REQUIRE(tds[0].repeatTimes[0].repeatInterval == "604800");
  REQUIRE(tds[0].repeatTimes[0].activeDuration == "3600");
  REQUIRE(tds[0].repeatTimes[0].offsets.size() == 2);
  REQUIRE(tds[0].repeatTimes[0].offsets[0] == "0");
  REQUIRE(tds[0].repeatTimes[0].offsets[1] == "90000");
  REQUIRE(tds[1].startTime == 0);
}

TEST_CASE("Parser: z= line (zone adjustments)", "[parser]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "z=2882844526 -1h 2898848070 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->zoneAdjustments.size() == 2);
  REQUIRE(result.value->zoneAdjustments[0].adjustmentTime == "2882844526");
  REQUIRE(result.value->zoneAdjustments[0].offset == "-1h");
  REQUIRE(result.value->zoneAdjustments[1].adjustmentTime == "2898848070");
  REQUIRE(result.value->zoneAdjustments[1].offset == "0");
}

TEST_CASE("Parser: origin parsing edge cases — large numeric ids as strings", "[parser]")
{
  std::string sdp =
    "v=0\r\n"
    "o=jdoe 2890844526 2890842807 IN IP4 10.47.16.5\r\n"
    "s=-\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& o = result.value->origin;
  REQUIRE(o.username == "jdoe");
  REQUIRE(o.sessId == "2890844526");
  REQUIRE(o.sessVersion == "2890842807");
  REQUIRE(o.address == "10.47.16.5");
}

// =========================================================================
// T4 — Session-level attribute parsing
// =========================================================================

TEST_CASE("Parser: session-level attributes (group, ICE, fingerprint, setup, extmap-allow-mixed)", "[parser]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=group:BUNDLE 0 1 2\r\n"
    "a=ice-ufrag:abc123\r\n"
    "a=ice-pwd:secretpassword\r\n"
    "a=ice-options:trickle ice2\r\n"
    "a=ice-lite\r\n"
    "a=fingerprint:sha-256 AA:BB:CC:DD\r\n"
    "a=setup:actpass\r\n"
    "a=extmap-allow-mixed\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());

  auto& sd = *result.value;
  REQUIRE(sd.groups.size() == 1);
  REQUIRE(sd.groups[0].semantics == "BUNDLE");
  REQUIRE(sd.groups[0].mids.size() == 3);
  REQUIRE(sd.groups[0].mids[0] == "0");
  REQUIRE(sd.groups[0].mids[2] == "2");

  REQUIRE(sd.iceUfrag == "abc123");
  REQUIRE(sd.icePwd == "secretpassword");
  REQUIRE(sd.iceOptions.size() == 2);
  REQUIRE(sd.iceOptions[0] == "trickle");
  REQUIRE(sd.iceOptions[1] == "ice2");
  REQUIRE(sd.iceLite == true);

  REQUIRE(sd.fingerprints.size() == 1);
  REQUIRE(sd.fingerprints[0].hashFunction == "sha-256");
  REQUIRE(sd.fingerprints[0].fingerprint == "AA:BB:CC:DD");

  REQUIRE(sd.setup == SetupRole::ACTPASS);
  REQUIRE(sd.extmapAllowMixed == true);
}

TEST_CASE("Parser: unknown session-level attributes stored as GenericAttribute", "[parser]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=x-custom:some-value\r\n"
    "a=tool:iora-sdp\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->attributes.size() == 2);
  REQUIRE(result.value->attributes[0].name == "x-custom");
  REQUIRE(result.value->attributes[0].value == "some-value");
  REQUIRE(result.value->attributes[1].name == "tool");
  REQUIRE(result.value->attributes[1].value == "iora-sdp");
}

TEST_CASE("Parser: session-level direction captured as typed field, not GenericAttribute", "[parser][direction]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=sendonly\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  // A session-level direction lands in the typed session.direction field and is
  // NOT double-stored as a GenericAttribute (RFC 4566 §5/§6 fold-down source).
  REQUIRE(result.value->direction.has_value());
  REQUIRE(*result.value->direction == Direction::SENDONLY);
  REQUIRE(result.value->attributes.empty());
}

// =========================================================================
// T5 — m= line parsing and media section state machine
// =========================================================================

TEST_CASE("Parser: m= line basic parsing", "[parser]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->mediaDescriptions.size() == 1);

  auto& md = result.value->mediaDescriptions[0];
  REQUIRE(md.mediaType == MediaType::AUDIO);
  REQUIRE(md.port == 9);
  REQUIRE(!md.numberOfPorts.has_value());
  REQUIRE(md.protocol == TransportProtocol::UDP_TLS_RTP_SAVPF);
  REQUIRE(md.formats.size() == 2);
  REQUIRE(md.formats[0] == "111");
  REQUIRE(md.formats[1] == "0");
}

TEST_CASE("Parser: m= line with port/numberOfPorts", "[parser]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 5004/2 RTP/AVP 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& md = result.value->mediaDescriptions[0];
  REQUIRE(md.port == 5004);
  REQUIRE(md.numberOfPorts == 2);
}

TEST_CASE("Parser: rejected media (port 0)", "[parser]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 0 RTP/AVP 96\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->mediaDescriptions[0].port == 0);
}

TEST_CASE("Parser: media-level c=, b=, k= lines", "[parser]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "c=IN IP6 ::1\r\n"
    "b=AS:64\r\n"
    "k=base64:keydata\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& md = result.value->mediaDescriptions[0];
  REQUIRE(md.connection.has_value());
  REQUIRE(md.connection->addrType == AddressType::IP6);
  REQUIRE(md.connection->address == "::1");
  REQUIRE(md.bandwidths.size() == 1);
  REQUIRE(md.bandwidths[0].type == BandwidthType::AS);
  REQUIRE(md.bandwidths[0].bandwidth == 64);
  REQUIRE(md.encryptionKey == "base64:keydata");
}

// =========================================================================
// T6 — Media-level RTP attributes
// =========================================================================

TEST_CASE("Parser: a=rtpmap", "[parser][rtpmap]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111 0\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
    "a=rtpmap:0 PCMU/8000\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& md = result.value->mediaDescriptions[0];
  REQUIRE(md.rtpMaps.size() == 2);

  REQUIRE(md.rtpMaps[0].payloadType == 111);
  REQUIRE(md.rtpMaps[0].encodingName == "opus");
  REQUIRE(md.rtpMaps[0].clockRate == 48000);
  REQUIRE(md.rtpMaps[0].channels == 2);

  REQUIRE(md.rtpMaps[1].payloadType == 0);
  REQUIRE(md.rtpMaps[1].encodingName == "PCMU");
  REQUIRE(md.rtpMaps[1].clockRate == 8000);
  REQUIRE(!md.rtpMaps[1].channels.has_value());
}

TEST_CASE("Parser: a=fmtp with parameterMap", "[parser][fmtp]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=fmtp:111 minptime=10;useinbandfec=1\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& fmtp = result.value->mediaDescriptions[0].fmtps[0];
  REQUIRE(fmtp.payloadType == 111);
  REQUIRE(fmtp.parameters == "minptime=10;useinbandfec=1");
  REQUIRE(fmtp.parameterMap.size() == 2);
  REQUIRE(fmtp.parameterMap.at("minptime") == "10");
  REQUIRE(fmtp.parameterMap.at("useinbandfec") == "1");
}

TEST_CASE("Parser: a=rtcp-fb", "[parser][rtcpfb]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=rtcp-fb:96 nack pli\r\n"
    "a=rtcp-fb:* transport-cc\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& fbs = result.value->mediaDescriptions[0].rtcpFeedbacks;
  REQUIRE(fbs.size() == 2);
  REQUIRE(fbs[0].payloadType == "96");
  REQUIRE(fbs[0].type == "nack");
  REQUIRE(fbs[0].subtype == "pli");
  REQUIRE(fbs[1].payloadType == "*");
  REQUIRE(fbs[1].type == "transport-cc");
  REQUIRE(!fbs[1].subtype.has_value());
}

TEST_CASE("Parser: a=extmap", "[parser][extmap]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\n"
    "a=extmap:2/sendonly urn:ietf:params:rtp-hdrext:some-ext extra-attrs\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& exts = result.value->mediaDescriptions[0].extMaps;
  REQUIRE(exts.size() == 2);

  REQUIRE(exts[0].id == 1);
  REQUIRE(!exts[0].direction.has_value());
  REQUIRE(exts[0].uri == "urn:ietf:params:rtp-hdrext:ssrc-audio-level");

  REQUIRE(exts[1].id == 2);
  REQUIRE(exts[1].direction == Direction::SENDONLY);
  REQUIRE(exts[1].uri == "urn:ietf:params:rtp-hdrext:some-ext");
  REQUIRE(exts[1].extensionAttributes == "extra-attrs");
}

TEST_CASE("Parser: a=ssrc and a=ssrc-group", "[parser][ssrc]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=ssrc:12345 cname:user@example.com\r\n"
    "a=ssrc:12345 msid:stream1 track1\r\n"
    "a=ssrc-group:FID 12345 67890\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& md = result.value->mediaDescriptions[0];

  REQUIRE(md.ssrcs.size() == 2);
  REQUIRE(md.ssrcs[0].ssrc == 12345);
  REQUIRE(md.ssrcs[0].attributeName == "cname");
  REQUIRE(md.ssrcs[0].attributeValue == "user@example.com");
  REQUIRE(md.ssrcs[1].attributeName == "msid");
  REQUIRE(md.ssrcs[1].attributeValue == "stream1 track1");

  REQUIRE(md.ssrcGroups.size() == 1);
  REQUIRE(md.ssrcGroups[0].semantics == "FID");
  REQUIRE(md.ssrcGroups[0].ssrcs.size() == 2);
  REQUIRE(md.ssrcGroups[0].ssrcs[0] == 12345);
  REQUIRE(md.ssrcGroups[0].ssrcs[1] == 67890);
}

TEST_CASE("Parser: a=rtcp", "[parser][rtcp]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=rtcp:9 IN IP4 0.0.0.0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& rtcp = result.value->mediaDescriptions[0].rtcp;
  REQUIRE(rtcp.has_value());
  REQUIRE(rtcp->port == 9);
  REQUIRE(rtcp->netType == NetworkType::IN);
  REQUIRE(rtcp->addrType == AddressType::IP4);
  REQUIRE(rtcp->address == "0.0.0.0");
}

TEST_CASE("Parser: a=ptime, a=maxptime, a=framerate", "[parser][media-timing]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=ptime:20\r\n"
    "a=maxptime:60\r\n"
    "m=video 9 RTP/AVP 96\r\n"
    "a=framerate:30.0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->mediaDescriptions[0].ptime == 20);
  REQUIRE(result.value->mediaDescriptions[0].maxptime == 60);
  REQUIRE(result.value->mediaDescriptions[1].framerate == 30.0);
}

// =========================================================================
// T7 — Media-level ICE, DTLS, WebRTC attributes
// =========================================================================

TEST_CASE("Parser: a=mid, a=msid", "[parser][mid-msid]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=mid:0\r\n"
    "a=msid:stream1 track1\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& md = result.value->mediaDescriptions[0];
  REQUIRE(md.mid == "0");
  REQUIRE(md.msid.size() == 1);
  REQUIRE(md.msid[0].streamId == "stream1");
  REQUIRE(md.msid[0].trackId == "track1");
}

TEST_CASE("Parser: media-level ICE attributes", "[parser][ice]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=ice-ufrag:localufrag\r\n"
    "a=ice-pwd:localpassword\r\n"
    "a=ice-options:trickle\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& md = result.value->mediaDescriptions[0];
  REQUIRE(md.iceUfrag == "localufrag");
  REQUIRE(md.icePwd == "localpassword");
  REQUIRE(md.iceOptions.size() == 1);
  REQUIRE(md.iceOptions[0] == "trickle");
}

TEST_CASE("Parser: ICE candidate — host", "[parser][candidate]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=candidate:1 1 udp 2130706431 192.168.1.1 5000 typ host generation 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& cand = result.value->mediaDescriptions[0].candidates[0];
  REQUIRE(cand.foundation == "1");
  REQUIRE(cand.component == 1);
  REQUIRE(cand.transport == IceTransportType::UDP);
  REQUIRE(cand.priority == 2130706431);
  REQUIRE(cand.address == "192.168.1.1");
  REQUIRE(cand.port == 5000);
  REQUIRE(cand.type == IceCandidateType::HOST);
  REQUIRE(!cand.relAddr.has_value());
  REQUIRE(!cand.relPort.has_value());
  REQUIRE(cand.extensions.size() == 1);
  REQUIRE(cand.extensions[0].first == "generation");
  REQUIRE(cand.extensions[0].second == "0");
}

TEST_CASE("Parser: ICE candidate — srflx with raddr/rport", "[parser][candidate]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=candidate:2 1 udp 1694498815 203.0.113.1 6000 typ srflx raddr 192.168.1.1 rport 5000\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& cand = result.value->mediaDescriptions[0].candidates[0];
  REQUIRE(cand.type == IceCandidateType::SRFLX);
  REQUIRE(cand.relAddr == "192.168.1.1");
  REQUIRE(cand.relPort == 5000);
}

TEST_CASE("Parser: ICE candidate — relay with raddr/rport", "[parser][candidate]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=candidate:3 1 udp 16777215 203.0.113.2 7000 typ relay raddr 203.0.113.1 rport 6000\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& cand = result.value->mediaDescriptions[0].candidates[0];
  REQUIRE(cand.type == IceCandidateType::RELAY);
  REQUIRE(cand.relAddr == "203.0.113.1");
  REQUIRE(cand.relPort == 6000);
}

TEST_CASE("Parser: ICE candidate — TCP with tcptype", "[parser][candidate]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=candidate:4 1 tcp 2105524223 192.168.1.1 9 typ host tcptype passive\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& cand = result.value->mediaDescriptions[0].candidates[0];
  REQUIRE(cand.transport == IceTransportType::TCP);
  REQUIRE(cand.type == IceCandidateType::HOST);
  REQUIRE(cand.tcpType == "passive");
}

TEST_CASE("Parser: media-level fingerprint and setup", "[parser][dtls]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF\r\n"
    "a=setup:active\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& md = result.value->mediaDescriptions[0];
  REQUIRE(md.fingerprints.size() == 1);
  REQUIRE(md.fingerprints[0].hashFunction == "sha-256");
  REQUIRE(md.fingerprints[0].fingerprint == "AA:BB:CC:DD:EE:FF");
  REQUIRE(md.setup == SetupRole::ACTIVE);
}

TEST_CASE("Parser: a=crypto (SDES)", "[parser][crypto]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 5004 RTP/SAVP 0\r\n"
    "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:base64keyvalue\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& cr = result.value->mediaDescriptions[0].crypto;
  REQUIRE(cr.size() == 1);
  REQUIRE(cr[0].tag == 1);
  REQUIRE(cr[0].suite == "AES_CM_128_HMAC_SHA1_80");
  REQUIRE(cr[0].keyParams == "inline:base64keyvalue");
  REQUIRE(!cr[0].sessionParams.has_value());
}

TEST_CASE("Parser: flag attributes (rtcp-mux, rtcp-mux-only, rtcp-rsize, end-of-candidates, extmap-allow-mixed)", "[parser][flags]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=rtcp-mux\r\n"
    "a=rtcp-mux-only\r\n"
    "a=rtcp-rsize\r\n"
    "a=end-of-candidates\r\n"
    "a=extmap-allow-mixed\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& md = result.value->mediaDescriptions[0];
  REQUIRE(md.rtcpMux == true);
  REQUIRE(md.rtcpMuxOnly == true);
  REQUIRE(md.rtcpRsize == true);
  REQUIRE(md.endOfCandidates == true);
  REQUIRE(md.extmapAllowMixed == true);
}

TEST_CASE("Parser: direction attributes at media level", "[parser][direction]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=sendonly\r\n"
    "m=video 9 RTP/AVP 96\r\n"
    "a=recvonly\r\n"
    "m=audio 0 RTP/AVP 0\r\n"
    "a=inactive\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->mediaDescriptions[0].direction == Direction::SENDONLY);
  REQUIRE(result.value->mediaDescriptions[1].direction == Direction::RECVONLY);
  REQUIRE(result.value->mediaDescriptions[2].direction == Direction::INACTIVE);
}

TEST_CASE("Parser: a=sctp-port and a=max-message-size", "[parser][sctp]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=application 9 UDP/DTLS/SCTP webrtc-datachannel\r\n"
    "a=sctp-port:5000\r\n"
    "a=max-message-size:262144\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& md = result.value->mediaDescriptions[0];
  REQUIRE(md.mediaType == MediaType::APPLICATION);
  REQUIRE(md.protocol == TransportProtocol::UDP_DTLS_SCTP);
  REQUIRE(md.sctpPort == 5000);
  REQUIRE(md.maxMessageSize == 262144);
}

TEST_CASE("Parser: a=sctpmap stored as GenericAttribute (not dispatched)", "[parser][sctpmap]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=application 9 DTLS/SCTP 5000\r\n"
    "a=sctpmap:5000 webrtc-datachannel 1024\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& md = result.value->mediaDescriptions[0];
  REQUIRE(md.genericAttributes.size() == 1);
  REQUIRE(md.genericAttributes[0].name == "sctpmap");
  REQUIRE(md.genericAttributes[0].value == "5000 webrtc-datachannel 1024");
}

// =========================================================================
// Simulcast and RID
// =========================================================================

TEST_CASE("Parser: a=simulcast — send and recv", "[parser][simulcast]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=simulcast:send h;m;~l recv h\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& sim = result.value->mediaDescriptions[0].simulcast;
  REQUIRE(sim.has_value());
  REQUIRE(sim->sendStreams.size() == 3);
  REQUIRE(sim->sendStreams[0][0].rid == "h");
  REQUIRE(sim->sendStreams[0][0].paused == false);
  REQUIRE(sim->sendStreams[1][0].rid == "m");
  REQUIRE(sim->sendStreams[2][0].rid == "l");
  REQUIRE(sim->sendStreams[2][0].paused == true);
  REQUIRE(sim->recvStreams.size() == 1);
  REQUIRE(sim->recvStreams[0][0].rid == "h");
}

TEST_CASE("Parser: a=simulcast — alternatives with comma", "[parser][simulcast]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=simulcast:send h,m;l\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& sim = result.value->mediaDescriptions[0].simulcast;
  REQUIRE(sim.has_value());
  REQUIRE(sim->sendStreams.size() == 2);
  REQUIRE(sim->sendStreams[0].size() == 2);
  REQUIRE(sim->sendStreams[0][0].rid == "h");
  REQUIRE(sim->sendStreams[0][1].rid == "m");
  REQUIRE(sim->sendStreams[1].size() == 1);
  REQUIRE(sim->sendStreams[1][0].rid == "l");
}

TEST_CASE("Parser: a=rid", "[parser][rid]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96 97\r\n"
    "a=rid:h send pt=96,97;max-width=1280;max-height=720\r\n"
    "a=rid:l recv\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& rids = result.value->mediaDescriptions[0].rids;
  REQUIRE(rids.size() == 2);

  REQUIRE(rids[0].id == "h");
  REQUIRE(rids[0].direction == RidDirection::SEND);
  REQUIRE(rids[0].payloadTypes.size() == 2);
  REQUIRE(rids[0].payloadTypes[0] == 96);
  REQUIRE(rids[0].payloadTypes[1] == 97);
  REQUIRE(rids[0].restrictions.at("max-width") == "1280");
  REQUIRE(rids[0].restrictions.at("max-height") == "720");

  REQUIRE(rids[1].id == "l");
  REQUIRE(rids[1].direction == RidDirection::RECV);
  REQUIRE(rids[1].payloadTypes.empty());
}

TEST_CASE("Parser: unknown media-level attributes preserved as GenericAttribute", "[parser]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=x-custom:some-value\r\n"
    "a=label:main-audio\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& gas = result.value->mediaDescriptions[0].genericAttributes;
  REQUIRE(gas.size() == 2);
  REQUIRE(gas[0].name == "x-custom");
  REQUIRE(gas[0].value == "some-value");
  REQUIRE(gas[1].name == "label");
  REQUIRE(gas[1].value == "main-audio");
}

// =========================================================================
// Full SDP scenarios
// =========================================================================

TEST_CASE("Parser: WebRTC offer SDP", "[parser][webrtc]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 4962303333179871722 2 IN IP4 127.0.0.1\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=group:BUNDLE 0 1\r\n"
    "a=extmap-allow-mixed\r\n"
    "a=ice-ufrag:abcd\r\n"
    "a=ice-pwd:efghijklmnopqrstuvwx\r\n"
    "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
    "a=setup:actpass\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111 0\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=mid:0\r\n"
    "a=rtcp-mux\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
    "a=fmtp:111 minptime=10;useinbandfec=1\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtcp-fb:111 transport-cc\r\n"
    "a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\n"
    "a=ssrc:1001 cname:stream1\r\n"
    "a=msid:stream1 audio-track\r\n"
    "a=candidate:1 1 udp 2130706431 192.168.1.1 5000 typ host\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=mid:1\r\n"
    "a=rtcp-mux\r\n"
    "a=sendrecv\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=rtcp-fb:96 nack\r\n"
    "a=rtcp-fb:96 nack pli\r\n"
    "a=ssrc:2001 cname:stream1\r\n"
    "a=msid:stream1 video-track\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(!result.hasError());

  auto& sd = *result.value;
  REQUIRE(sd.origin.sessId == "4962303333179871722");
  REQUIRE(sd.groups.size() == 1);
  REQUIRE(sd.groups[0].semantics == "BUNDLE");
  REQUIRE(sd.groups[0].mids.size() == 2);
  REQUIRE(sd.extmapAllowMixed == true);
  REQUIRE(sd.iceUfrag == "abcd");
  REQUIRE(sd.icePwd == "efghijklmnopqrstuvwx");
  REQUIRE(sd.fingerprints.size() == 1);
  REQUIRE(sd.setup == SetupRole::ACTPASS);
  REQUIRE(sd.mediaDescriptions.size() == 2);

  auto& audio = sd.mediaDescriptions[0];
  REQUIRE(audio.mediaType == MediaType::AUDIO);
  REQUIRE(audio.mid == "0");
  REQUIRE(audio.rtcpMux == true);
  REQUIRE(audio.direction == Direction::SENDRECV);
  REQUIRE(audio.rtpMaps.size() == 2);
  REQUIRE(audio.rtpMaps[0].encodingName == "opus");
  REQUIRE(audio.fmtps.size() == 1);
  REQUIRE(audio.rtcpFeedbacks.size() == 1);
  REQUIRE(audio.extMaps.size() == 1);
  REQUIRE(audio.ssrcs.size() == 1);
  REQUIRE(audio.msid.size() == 1);
  REQUIRE(audio.candidates.size() == 1);

  auto& video = sd.mediaDescriptions[1];
  REQUIRE(video.mediaType == MediaType::VIDEO);
  REQUIRE(video.mid == "1");
  REQUIRE(video.rtpMaps.size() == 1);
  REQUIRE(video.rtpMaps[0].encodingName == "VP8");
  REQUIRE(video.rtcpFeedbacks.size() == 2);
}

TEST_CASE("Parser: WebRTC answer SDP (direction flipping, codec subset)", "[parser][webrtc]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 1234567890 1 IN IP4 127.0.0.1\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=group:BUNDLE 0 1\r\n"
    "a=setup:active\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=mid:0\r\n"
    "a=rtcp-mux\r\n"
    "a=recvonly\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=mid:1\r\n"
    "a=rtcp-mux\r\n"
    "a=recvonly\r\n"
    "a=rtpmap:96 VP8/90000\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& sd = *result.value;
  REQUIRE(sd.setup == SetupRole::ACTIVE);
  REQUIRE(sd.mediaDescriptions.size() == 2);
  REQUIRE(sd.mediaDescriptions[0].direction == Direction::RECVONLY);
  REQUIRE(sd.mediaDescriptions[0].formats.size() == 1);
  REQUIRE(sd.mediaDescriptions[1].direction == Direction::RECVONLY);
}

TEST_CASE("Parser: data channel SDP", "[parser][datachannel]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=group:BUNDLE 0\r\n"
    "a=fingerprint:sha-256 AA:BB:CC\r\n"
    "a=setup:actpass\r\n"
    "m=application 9 UDP/DTLS/SCTP webrtc-datachannel\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=mid:0\r\n"
    "a=sctp-port:5000\r\n"
    "a=max-message-size:262144\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());

  auto& md = result.value->mediaDescriptions[0];
  REQUIRE(md.mediaType == MediaType::APPLICATION);
  REQUIRE(md.protocol == TransportProtocol::UDP_DTLS_SCTP);
  REQUIRE(md.formats.size() == 1);
  REQUIRE(md.formats[0] == "webrtc-datachannel");
  REQUIRE(md.sctpPort == 5000);
  REQUIRE(md.maxMessageSize == 262144);
}

TEST_CASE("Parser: SIP SDP (RTP/AVP with SDES crypto, no ICE)", "[parser][sip]")
{
  std::string sdp =
    "v=0\r\n"
    "o=alice 2890844526 2890844526 IN IP4 10.0.0.1\r\n"
    "s=SIP Call\r\n"
    "c=IN IP4 10.0.0.1\r\n"
    "t=0 0\r\n"
    "m=audio 49170 RTP/SAVP 0 8 97\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtpmap:8 PCMA/8000\r\n"
    "a=rtpmap:97 iLBC/8000\r\n"
    "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:base64key\r\n"
    "a=sendrecv\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());

  auto& sd = *result.value;
  REQUIRE(sd.origin.username == "alice");
  REQUIRE(sd.sessionName == "SIP Call");
  REQUIRE(sd.connection.has_value());
  REQUIRE(sd.connection->address == "10.0.0.1");
  REQUIRE(sd.mediaDescriptions.size() == 1);

  auto& md = sd.mediaDescriptions[0];
  REQUIRE(md.port == 49170);
  REQUIRE(md.protocol == TransportProtocol::RTP_SAVP);
  REQUIRE(md.formats.size() == 3);
  REQUIRE(md.rtpMaps.size() == 3);
  REQUIRE(md.crypto.size() == 1);
  REQUIRE(md.direction == Direction::SENDRECV);
  REQUIRE(!md.iceUfrag.has_value());
}

TEST_CASE("Parser: multicast SDP with c= line TTL and numberOfAddresses", "[parser][multicast]")
{
  // The multi-address form is MEDIA-level: RFC 4566 §5.7 permits multiple
  // addresses only per-media, and "They MUST NOT be specified for a
  // session-level 'c=' field."
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 49170 RTP/AVP 0\r\n"
    "c=IN IP4 224.2.36.42/127/3\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(!result.value->mediaDescriptions.empty());
  auto& c = result.value->mediaDescriptions[0].connection;
  REQUIRE(c.has_value());
  REQUIRE(c->address == "224.2.36.42");
  REQUIRE(c->ttl == 127);
  REQUIRE(c->numberOfAddresses == 3);
}

TEST_CASE("Parser: a session-level c= may carry a TTL but not an address count",
          "[parser][multicast][connection]")
{
  // RFC 4566 §5.7: "Multiple addresses or 'c=' lines MAY be specified on a
  // per-media basis only if they provide multicast addresses for different
  // layers ... They MUST NOT be specified for a session-level 'c=' field."
  // A <count> group IS multiple addresses; a bare <base>/<ttl> is not (RFC 4566
  // §5's own example is c=IN IP4 224.2.17.12/127).
  SECTION("session-level <base>/<ttl> is legal and preserved")
  {
    auto result = SdpParser::parse(
      "v=0\r\no=- 0 0 IN IP4 0.0.0.0\r\ns=-\r\nc=IN IP4 224.2.17.12/127\r\nt=0 0\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(result.value->connection.has_value());
    REQUIRE(result.value->connection->ttl == 127);
    REQUIRE(!result.value->connection->numberOfAddresses.has_value());
  }

  SECTION("session-level address count is dropped with a warning in lenient mode")
  {
    auto result = SdpParser::parse(
      "v=0\r\no=- 0 0 IN IP4 0.0.0.0\r\ns=-\r\nc=IN IP4 224.2.36.42/127/3\r\nt=0 0\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(result.value->connection.has_value());
    REQUIRE(result.value->connection->ttl == 127);
    REQUIRE(!result.value->connection->numberOfAddresses.has_value());
    REQUIRE_THAT(result.warnings.back().message,
                 Catch::Matchers::Contains("multiple addresses"));
  }

  SECTION("session-level address count is a strict-mode error")
  {
    SdpParserOptions opts;
    opts.strict = true;
    auto result = SdpParser::parse(
      "v=0\r\no=- 0 0 IN IP4 0.0.0.0\r\ns=-\r\nc=IN IP4 224.2.36.42/127/3\r\nt=0 0\r\n",
      opts);
    REQUIRE(!result.hasValue());
    REQUIRE(result.hasError());
  }

  SECTION("any IPv6 session-level slash group is a count, so it is forbidden")
  {
    SdpParserOptions opts;
    opts.strict = true;
    auto result = SdpParser::parse(
      "v=0\r\no=- 0 0 IN IP6 ::1\r\ns=-\r\nc=IN IP6 FF15::101/3\r\nt=0 0\r\n", opts);
    REQUIRE(!result.hasValue());
    REQUIRE(result.hasError());
  }
}

// =========================================================================
// Line ending variations
// =========================================================================

TEST_CASE("Parser: bare LF line endings", "[parser][line-endings]")
{
  std::string sdp =
    "v=0\n"
    "o=- 0 0 IN IP4 0.0.0.0\n"
    "s=-\n"
    "t=0 0\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->version == 0);
}

TEST_CASE("Parser: CRLF line endings", "[parser][line-endings]")
{
  auto result = SdpParser::parse(kMinimalSdp);
  REQUIRE(result.hasValue());
}

TEST_CASE("Parser: mixed line endings", "[parser][line-endings]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\n"
    "s=-\r\n"
    "t=0 0\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->sessionName == "-");
}

// =========================================================================
// Lenient vs strict mode
// =========================================================================

TEST_CASE("Parser: lenient mode — malformed lines skipped with warning", "[parser][lenient]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "GARBAGE LINE\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.warnings.size() == 1);
  REQUIRE(result.warnings[0].line == 4);
}

TEST_CASE("Parser: strict mode — malformed line produces error", "[parser][strict]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "GARBAGE LINE\r\n"
    "t=0 0\r\n";

  SdpParserOptions opts;
  opts.strict = true;
  auto result = SdpParser::parse(sdp, opts);
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
  REQUIRE(result.error->line == 4);
}

TEST_CASE("Parser: lenient mode — leading/trailing whitespace trimmed", "[parser][lenient]")
{
  std::string sdp =
    "  v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->version == 0);
}

TEST_CASE("Parser: strict mode — leading/trailing whitespace is error", "[parser][strict]")
{
  std::string sdp =
    "  v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n";

  SdpParserOptions opts;
  opts.strict = true;
  auto result = SdpParser::parse(sdp, opts);
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
}

TEST_CASE("Parser: duplicate unique attributes — lenient overwrite with warning", "[parser][lenient]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=ice-ufrag:first\r\n"
    "a=ice-ufrag:second\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->iceUfrag == "second");
  REQUIRE(!result.warnings.empty());
}

TEST_CASE("Parser: duplicate unique attributes — strict produces error", "[parser][strict]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=ice-ufrag:first\r\n"
    "a=ice-ufrag:second\r\n";

  SdpParserOptions opts;
  opts.strict = true;
  auto result = SdpParser::parse(sdp, opts);
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
}

// =========================================================================
// Error cases
// =========================================================================

TEST_CASE("Parser: empty SDP returns error", "[parser][error]")
{
  auto result = SdpParser::parse("");
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
  REQUIRE(result.error->message == "Empty SDP");
}

TEST_CASE("Parser: missing v= line (requireVersion=true)", "[parser][error]")
{
  std::string sdp =
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
}

TEST_CASE("Parser: missing o= line (requireOrigin=true)", "[parser][error]")
{
  std::string sdp =
    "v=0\r\n"
    "s=-\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
}

TEST_CASE("Parser: missing s= line (requireSessionName=true)", "[parser][error]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
}

TEST_CASE("Parser: missing t= line with requireTiming=true", "[parser][error]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n";

  SdpParserOptions opts;
  opts.requireTiming = true;
  auto result = SdpParser::parse(sdp, opts);
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
}

TEST_CASE("Parser: missing required fields can be relaxed with options", "[parser]")
{
  std::string sdp =
    "m=audio 9 RTP/AVP 0\r\n";

  SdpParserOptions opts;
  opts.requireVersion = false;
  opts.requireOrigin = false;
  opts.requireSessionName = false;
  auto result = SdpParser::parse(sdp, opts);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->mediaDescriptions.size() == 1);
}

TEST_CASE("Parser: maxLineLength exceeded — lenient skips with warning", "[parser][limits]")
{
  SdpParserOptions opts;
  opts.maxLineLength = 20;

  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp, opts);
  // o= line exceeds 20 chars, so it's skipped, then requireOrigin fails
  REQUIRE(!result.hasValue());
}

TEST_CASE("Parser: maxLineLength exceeded — strict error", "[parser][limits]")
{
  SdpParserOptions opts;
  opts.strict = true;
  opts.maxLineLength = 10;

  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp, opts);
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
}

TEST_CASE("Parser: maxMediaSections exceeded", "[parser][limits]")
{
  SdpParserOptions opts;
  opts.maxMediaSections = 1;

  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "m=video 9 RTP/AVP 96\r\n";

  auto result = SdpParser::parse(sdp, opts);
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
}

// =========================================================================
// T8 — Public API: parseMediaSection and parseAttribute
// =========================================================================

TEST_CASE("Parser: parseMediaSection", "[parser][api]")
{
  std::string section =
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=mid:0\r\n"
    "a=rtcp-mux\r\n"
    "a=rtpmap:111 opus/48000/2\r\n"
    "a=sendrecv\r\n";

  auto result = SdpParser::parseMediaSection(section);
  REQUIRE(result.hasValue());

  auto& md = *result.value;
  REQUIRE(md.mediaType == MediaType::AUDIO);
  REQUIRE(md.port == 9);
  REQUIRE(md.mid == "0");
  REQUIRE(md.rtcpMux == true);
  REQUIRE(md.rtpMaps.size() == 1);
  REQUIRE(md.rtpMaps[0].encodingName == "opus");
  REQUIRE(md.direction == Direction::SENDRECV);
}

TEST_CASE("Parser: parseMediaSection — error on non-m= first line", "[parser][api]")
{
  auto result = SdpParser::parseMediaSection("a=mid:0\r\n");
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
}

TEST_CASE("Parser: parseMediaSection — empty input", "[parser][api]")
{
  auto result = SdpParser::parseMediaSection("");
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
}

TEST_CASE("Parser: parseAttribute — rtpmap", "[parser][api]")
{
  auto ga = SdpParser::parseAttribute("a=rtpmap:111 opus/48000/2");
  REQUIRE(ga.name == "rtpmap");
  REQUIRE(ga.value == "111 opus/48000/2");
}

TEST_CASE("Parser: parseAttribute — candidate", "[parser][api]")
{
  auto ga = SdpParser::parseAttribute("a=candidate:1 1 udp 2130706431 192.168.1.1 5000 typ host");
  REQUIRE(ga.name == "candidate");
  REQUIRE(ga.value.has_value());
}

TEST_CASE("Parser: parseAttribute — flag attribute (no value)", "[parser][api]")
{
  auto ga = SdpParser::parseAttribute("a=rtcp-mux");
  REQUIRE(ga.name == "rtcp-mux");
  REQUIRE(!ga.value.has_value());
}

TEST_CASE("Parser: parseAttribute — unknown attribute", "[parser][api]")
{
  auto ga = SdpParser::parseAttribute("a=x-custom:hello world");
  REQUIRE(ga.name == "x-custom");
  REQUIRE(ga.value == "hello world");
}

TEST_CASE("Parser: parseAttribute — without a= prefix", "[parser][api]")
{
  auto ga = SdpParser::parseAttribute("mid:0");
  REQUIRE(ga.name == "mid");
  REQUIRE(ga.value == "0");
}

// =========================================================================
// Round-trip: parse and verify all fields match expected values
// =========================================================================

TEST_CASE("Parser: round-trip verify — all session and media fields populated", "[parser][round-trip]")
{
  std::string sdp =
    "v=0\r\n"
    "o=jdoe 2890844526 2890842807 IN IP4 10.47.16.5\r\n"
    "s=SDP Workshop\r\n"
    "i=A workshop session\r\n"
    "u=http://example.com/sdp\r\n"
    "e=jdoe@example.com\r\n"
    "p=+1-555-0199\r\n"
    "c=IN IP4 224.2.36.42/127\r\n"
    "b=AS:512\r\n"
    "t=2873397496 2873404696\r\n"
    "r=604800 3600 0 90000\r\n"
    "z=2882844526 -1h\r\n"
    "k=clear:password\r\n"
    "a=group:BUNDLE audio video\r\n"
    "a=ice-ufrag:testufrag\r\n"
    "a=ice-pwd:testpassword\r\n"
    "a=ice-options:trickle\r\n"
    "a=ice-lite\r\n"
    "a=fingerprint:sha-256 AA:BB\r\n"
    "a=setup:actpass\r\n"
    "a=extmap-allow-mixed\r\n"
    "a=x-custom:session-level\r\n"
    "m=audio 49170 RTP/AVP 0 8\r\n"
    "c=IN IP4 10.0.0.1\r\n"
    "b=AS:64\r\n"
    "a=mid:audio\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtpmap:8 PCMA/8000\r\n"
    "a=sendrecv\r\n"
    "a=ptime:20\r\n"
    "m=video 51372 RTP/AVP 96\r\n"
    "a=mid:video\r\n"
    "a=rtpmap:96 VP8/90000\r\n"
    "a=recvonly\r\n"
    "a=framerate:30.0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(!result.hasError());

  auto& sd = *result.value;

  // Session level
  REQUIRE(sd.version == 0);
  REQUIRE(sd.origin.username == "jdoe");
  REQUIRE(sd.origin.sessId == "2890844526");
  REQUIRE(sd.origin.sessVersion == "2890842807");
  REQUIRE(sd.sessionName == "SDP Workshop");
  REQUIRE(sd.sessionInfo == "A workshop session");
  REQUIRE(sd.uri == "http://example.com/sdp");
  REQUIRE(sd.emailAddress == "jdoe@example.com");
  REQUIRE(sd.phoneNumber == "+1-555-0199");
  REQUIRE(sd.connection.has_value());
  REQUIRE(sd.connection->address == "224.2.36.42");
  REQUIRE(sd.connection->ttl == 127);
  REQUIRE(sd.bandwidths.size() == 1);
  REQUIRE(sd.bandwidths[0].type == BandwidthType::AS);
  REQUIRE(sd.bandwidths[0].bandwidth == 512);
  REQUIRE(sd.timeDescriptions.size() == 1);
  REQUIRE(sd.timeDescriptions[0].startTime == 2873397496);
  REQUIRE(sd.timeDescriptions[0].stopTime == 2873404696);
  REQUIRE(sd.timeDescriptions[0].repeatTimes.size() == 1);
  REQUIRE(sd.zoneAdjustments.size() == 1);
  REQUIRE(sd.encryptionKey == "clear:password");
  REQUIRE(sd.groups.size() == 1);
  REQUIRE(sd.groups[0].semantics == "BUNDLE");
  REQUIRE(sd.groups[0].mids.size() == 2);
  REQUIRE(sd.iceUfrag == "testufrag");
  REQUIRE(sd.icePwd == "testpassword");
  REQUIRE(sd.iceOptions.size() == 1);
  REQUIRE(sd.iceLite == true);
  REQUIRE(sd.fingerprints.size() == 1);
  REQUIRE(sd.setup == SetupRole::ACTPASS);
  REQUIRE(sd.extmapAllowMixed == true);
  REQUIRE(sd.attributes.size() == 1);
  REQUIRE(sd.attributes[0].name == "x-custom");

  // Media sections
  REQUIRE(sd.mediaDescriptions.size() == 2);

  auto& audio = sd.mediaDescriptions[0];
  REQUIRE(audio.mediaType == MediaType::AUDIO);
  REQUIRE(audio.port == 49170);
  REQUIRE(audio.protocol == TransportProtocol::RTP_AVP);
  REQUIRE(audio.formats.size() == 2);
  REQUIRE(audio.connection.has_value());
  REQUIRE(audio.connection->address == "10.0.0.1");
  REQUIRE(audio.bandwidths.size() == 1);
  REQUIRE(audio.mid == "audio");
  REQUIRE(audio.rtpMaps.size() == 2);
  REQUIRE(audio.direction == Direction::SENDRECV);
  REQUIRE(audio.ptime == 20);

  auto& video = sd.mediaDescriptions[1];
  REQUIRE(video.mediaType == MediaType::VIDEO);
  REQUIRE(video.port == 51372);
  REQUIRE(video.mid == "video");
  REQUIRE(video.rtpMaps.size() == 1);
  REQUIRE(video.direction == Direction::RECVONLY);
  REQUIRE(video.framerate == 30.0);
}

// =========================================================================
// Review findings — additional test coverage
// =========================================================================

TEST_CASE("Parser: v= version validation — v=1 rejected in strict mode", "[parser][version]")
{
  std::string sdp =
    "v=1\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n";

  SdpParserOptions opts;
  opts.strict = true;
  auto result = SdpParser::parse(sdp, opts);
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
}

TEST_CASE("Parser: v= version validation — v=1 warning in lenient mode", "[parser][version]")
{
  std::string sdp =
    "v=1\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(!result.warnings.empty());
  REQUIRE(result.value->version == 1);
}

TEST_CASE("Parser: r= line with typed time format", "[parser][repeat-time]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "r=7d 1h 0 25h\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& rt = result.value->timeDescriptions[0].repeatTimes[0];
  REQUIRE(rt.repeatInterval == "7d");
  REQUIRE(rt.activeDuration == "1h");
  REQUIRE(rt.offsets.size() == 2);
  REQUIRE(rt.offsets[0] == "0");
  REQUIRE(rt.offsets[1] == "25h");
}

TEST_CASE("Parser: a=msid without trackId", "[parser][msid]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=msid:stream-only\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& msid = result.value->mediaDescriptions[0].msid[0];
  REQUIRE(msid.streamId == "stream-only");
  REQUIRE(!msid.trackId.has_value());
}

TEST_CASE("Parser: a=rtcp with port only (no netType/addrType/address)", "[parser][rtcp]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=rtcp:9\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& rtcp = result.value->mediaDescriptions[0].rtcp;
  REQUIRE(rtcp.has_value());
  REQUIRE(rtcp->port == 9);
  REQUIRE(!rtcp->netType.has_value());
  REQUIRE(!rtcp->addrType.has_value());
  REQUIRE(!rtcp->address.has_value());
}

TEST_CASE("Parser: a=crypto with sessionParams", "[parser][crypto]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 5004 RTP/SAVP 0\r\n"
    "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:base64key KDR=1 UNENCRYPTED_SRTP\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& cr = result.value->mediaDescriptions[0].crypto[0];
  REQUIRE(cr.tag == 1);
  REQUIRE(cr.suite == "AES_CM_128_HMAC_SHA1_80");
  REQUIRE(cr.keyParams == "inline:base64key");
  REQUIRE(cr.sessionParams.has_value());
  REQUIRE(cr.sessionParams == "KDR=1 UNENCRYPTED_SRTP");
}

TEST_CASE("Parser: media-level duplicate a=mid — lenient overwrite with warning", "[parser][lenient]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=mid:first\r\n"
    "a=mid:second\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->mediaDescriptions[0].mid == "second");
  REQUIRE(!result.warnings.empty());
}

TEST_CASE("Parser: media-level duplicate a=ice-ufrag — strict error", "[parser][strict]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=ice-ufrag:first\r\n"
    "a=ice-ufrag:second\r\n";

  SdpParserOptions opts;
  opts.strict = true;
  auto result = SdpParser::parse(sdp, opts);
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
}

TEST_CASE("Parser: multiple a=group lines at session level", "[parser][group]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=group:BUNDLE 0 1\r\n"
    "a=group:LS 0 1\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->groups.size() == 2);
  REQUIRE(result.value->groups[0].semantics == "BUNDLE");
  REQUIRE(result.value->groups[1].semantics == "LS");
}

TEST_CASE("Parser: a=rid with only id and direction — empty restrictions", "[parser][rid]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=rid:m send\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& rid = result.value->mediaDescriptions[0].rids[0];
  REQUIRE(rid.id == "m");
  REQUIRE(rid.direction == RidDirection::SEND);
  REQUIRE(rid.payloadTypes.empty());
  REQUIRE(rid.restrictions.empty());
}

TEST_CASE("Parser: default direction is sendrecv when no direction attribute present", "[parser][direction]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  // No a= direction line -> the raw media-level field is absent, and the
  // effective direction defaults to sendrecv (RFC 3264 §6.1 / RFC 4566 §6).
  REQUIRE(!result.value->mediaDescriptions[0].direction.has_value());
  REQUIRE(result.value->effectiveDirection(0) == Direction::SENDRECV);
}

TEST_CASE("Parser: a=ssrc without attributeValue (just attributeName)", "[parser][ssrc]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=ssrc:12345 mute\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& ssrc = result.value->mediaDescriptions[0].ssrcs[0];
  REQUIRE(ssrc.ssrc == 12345);
  REQUIRE(ssrc.attributeName == "mute");
  REQUIRE(!ssrc.attributeValue.has_value());
}

TEST_CASE("Parser: multiple b= lines in a media section", "[parser][bandwidth]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 RTP/AVP 96\r\n"
    "b=AS:1000\r\n"
    "b=TIAS:500000\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& bws = result.value->mediaDescriptions[0].bandwidths;
  REQUIRE(bws.size() == 2);
  REQUIRE(bws[0].type == BandwidthType::AS);
  REQUIRE(bws[0].bandwidth == 1000);
  REQUIRE(bws[1].type == BandwidthType::TIAS);
  REQUIRE(bws[1].bandwidth == 500000);
}

TEST_CASE("Parser: connection data without TTL — unicast address", "[parser][connection]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "c=IN IP4 192.168.1.100\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& c = result.value->connection;
  REQUIRE(c.has_value());
  REQUIRE(c->address == "192.168.1.100");
  REQUIRE(!c->ttl.has_value());
  REQUIRE(!c->numberOfAddresses.has_value());
}

TEST_CASE("Parser: a=ice-options with multiple values at media level", "[parser][ice]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=ice-options:trickle ice2 renomination\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& opts = result.value->mediaDescriptions[0].iceOptions;
  REQUIRE(opts.size() == 3);
  REQUIRE(opts[0] == "trickle");
  REQUIRE(opts[1] == "ice2");
  REQUIRE(opts[2] == "renomination");
}

TEST_CASE("Parser: ICE candidate with multiple extension attributes", "[parser][candidate]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=candidate:1 1 udp 2130706431 192.168.1.1 5000 typ host"
    " generation 0 ufrag abc network-id 1 network-cost 10\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& cand = result.value->mediaDescriptions[0].candidates[0];
  REQUIRE(cand.type == IceCandidateType::HOST);
  REQUIRE(cand.extensions.size() == 4);
  REQUIRE(cand.extensions[0].first == "generation");
  REQUIRE(cand.extensions[0].second == "0");
  REQUIRE(cand.extensions[1].first == "ufrag");
  REQUIRE(cand.extensions[1].second == "abc");
  REQUIRE(cand.extensions[2].first == "network-id");
  REQUIRE(cand.extensions[2].second == "1");
  REQUIRE(cand.extensions[3].first == "network-cost");
  REQUIRE(cand.extensions[3].second == "10");
}

TEST_CASE("Parser: a=extmap without extensionAttributes", "[parser][extmap]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& ext = result.value->mediaDescriptions[0].extMaps[0];
  REQUIRE(ext.id == 1);
  REQUIRE(!ext.direction.has_value());
  REQUIRE(ext.uri == "urn:ietf:params:rtp-hdrext:ssrc-audio-level");
  REQUIRE(!ext.extensionAttributes.has_value());
}

TEST_CASE("Parser: boundary values — port 65535 and max SSRC", "[parser][boundary]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 65535 RTP/AVP 0\r\n"
    "a=ssrc:4294967295 cname:max\r\n"
    "a=ptime:0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& md = result.value->mediaDescriptions[0];
  REQUIRE(md.port == 65535);
  REQUIRE(md.ssrcs[0].ssrc == 4294967295u);
  REQUIRE(md.ptime == 0);
}

TEST_CASE("Parser: empty session name (s=)", "[parser][edge-case]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->sessionName.empty());
}

TEST_CASE("Parser: connection data with TTL only (no numberOfAddresses)", "[parser][connection]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "c=IN IP4 224.2.36.42/127\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& c = result.value->connection;
  REQUIRE(c.has_value());
  REQUIRE(c->address == "224.2.36.42");
  REQUIRE(c->ttl == 127);
  REQUIRE(!c->numberOfAddresses.has_value());
}

// =========================================================================
// Final review findings — additional edge case tests
// =========================================================================

TEST_CASE("Parser: a=rtpmap with no clock rate (encoding name only)", "[parser][rtpmap][edge]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 RTP/AVP 96\r\n"
    "a=rtpmap:96 VP8\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& rm = result.value->mediaDescriptions[0].rtpMaps[0];
  REQUIRE(rm.payloadType == 96);
  REQUIRE(rm.encodingName == "VP8");
  REQUIRE(rm.clockRate == 0);
  REQUIRE(!rm.channels.has_value());
}

TEST_CASE("Parser: a=fmtp with parameters lacking = signs (standalone flags)", "[parser][fmtp][edge]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 RTP/AVP 96\r\n"
    "a=fmtp:96 profile-level-id=42e01f;level-asymmetry-allowed\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& fmtp = result.value->mediaDescriptions[0].fmtps[0];
  REQUIRE(fmtp.payloadType == 96);
  REQUIRE(fmtp.parameterMap.size() == 1);
  REQUIRE(fmtp.parameterMap.at("profile-level-id") == "42e01f");
}

TEST_CASE("Parser: a=ssrc-group with a single SSRC", "[parser][ssrc-group][edge]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 RTP/AVP 96\r\n"
    "a=ssrc-group:FID 12345\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& sg = result.value->mediaDescriptions[0].ssrcGroups[0];
  REQUIRE(sg.semantics == "FID");
  REQUIRE(sg.ssrcs.size() == 1);
  REQUIRE(sg.ssrcs[0] == 12345);
}

TEST_CASE("Parser: ICE candidate — prflx type", "[parser][candidate][edge]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=candidate:5 1 udp 1845501695 203.0.113.5 9000 typ prflx raddr 192.168.1.1 rport 5000\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& cand = result.value->mediaDescriptions[0].candidates[0];
  REQUIRE(cand.type == IceCandidateType::PRFLX);
  REQUIRE(cand.relAddr == "192.168.1.1");
  REQUIRE(cand.relPort == 5000);
}

TEST_CASE("Parser: a=fingerprint with sha-1 hash algorithm", "[parser][fingerprint][edge]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "a=fingerprint:sha-1 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& fp = result.value->mediaDescriptions[0].fingerprints[0];
  REQUIRE(fp.hashFunction == "sha-1");
  REQUIRE(fp.fingerprint == "AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99");
}

TEST_CASE("Parser: a=simulcast with recv only (no send)", "[parser][simulcast][edge]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
    "a=simulcast:recv h;m;l\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  auto& sim = result.value->mediaDescriptions[0].simulcast;
  REQUIRE(sim.has_value());
  REQUIRE(sim->sendStreams.empty());
  REQUIRE(sim->recvStreams.size() == 3);
  REQUIRE(sim->recvStreams[0][0].rid == "h");
  REQUIRE(sim->recvStreams[1][0].rid == "m");
  REQUIRE(sim->recvStreams[2][0].rid == "l");
}

TEST_CASE("Parser: SDP with only whitespace/blank lines", "[parser][error][edge]")
{
  auto result = SdpParser::parse("  \r\n  \r\n\r\n");
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
}

TEST_CASE("Parser: SDP with no trailing newline", "[parser][edge]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->timeDescriptions.size() == 1);
  REQUIRE(result.value->timeDescriptions[0].startTime == 0);
}

TEST_CASE("Parser: a= line with nothing after = (empty attribute name)", "[parser][edge]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "a=\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->attributes.size() == 1);
  REQUIRE(result.value->attributes[0].name.empty());
  REQUIRE(!result.value->attributes[0].value.has_value());
}

TEST_CASE("Parser: b= line with unknown bandwidth type (silently dropped)", "[parser][bandwidth][edge]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "b=X-CUSTOM:1000\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->bandwidths.empty());
}

TEST_CASE("Parser: orphan r= line before any t= line (silently ignored)", "[parser][edge]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "r=604800 3600 0\r\n"
    "t=0 0\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->timeDescriptions.size() == 1);
  REQUIRE(result.value->timeDescriptions[0].repeatTimes.empty());
}

TEST_CASE("Parser: z= line with odd number of tokens (incomplete pair)", "[parser][edge]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "z=2882844526\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->zoneAdjustments.empty());
}

TEST_CASE("Parser: media-level i= line generates warning", "[parser][edge]")
{
  std::string sdp =
    "v=0\r\n"
    "o=- 0 0 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 9 RTP/AVP 0\r\n"
    "i=Audio stream description\r\n";

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(!result.warnings.empty());
}

// =========================================================================
// c= line hardening + RFC 4566 §5.7 fold-down over parsed SDP
// =========================================================================

namespace {

/// A session-level c= plus one audio section, with extra media-level lines
/// spliced in.
std::string sdpWithMediaLines(const std::string& sessionConnection,
                              const std::string& mediaLines)
{
  return "v=0\r\n"
         "o=- 1 1 IN IP4 198.51.100.9\r\n"
         "s=-\r\n"
       + sessionConnection
       + "t=0 0\r\n"
         "m=audio 49170 RTP/AVP 0 8 101\r\n"
       + mediaLines;
}

} // namespace

TEST_CASE("Parser: a malformed media c= does not shadow the session c=",
          "[parser][connection]")
{
  // A c= that cannot be parsed must leave the media-level value UNSET so the
  // session-level address folds down. Yielding a default-constructed
  // ConnectionData would present an engaged-but-empty override and destroy the
  // only usable address in the description.
  const std::string sdp =
    sdpWithMediaLines("c=IN IP4 198.51.100.9\r\n", "c=IN IP4\r\n");

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(!result.value->mediaDescriptions.empty());
  REQUIRE(!result.value->mediaDescriptions[0].connection.has_value());
  REQUIRE(!result.warnings.empty());

  const auto resolved = result.value->effectiveConnection(0);
  REQUIRE(resolved.has_value());
  REQUIRE(resolved->address == "198.51.100.9");
}

TEST_CASE("Parser: a malformed c= is an error in strict mode",
          "[parser][connection][strict]")
{
  const std::string sdp =
    sdpWithMediaLines("c=IN IP4 198.51.100.9\r\n", "c=IN IP4\r\n");

  SdpParserOptions opts;
  opts.strict = true;
  auto result = SdpParser::parse(sdp, opts);
  REQUIRE(!result.hasValue());
  REQUIRE(result.hasError());
}

TEST_CASE("Parser: a c= with a surplus token is rejected", "[parser][connection]")
{
  // §5.7 defines exactly three subfields.
  const std::string sdp =
    sdpWithMediaLines("c=IN IP4 198.51.100.9\r\n",
                      "c=IN IP4 203.0.113.7 surplus\r\n");

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(!result.value->mediaDescriptions[0].connection.has_value());
  REQUIRE_THAT(result.warnings.back().message,
               Catch::Matchers::Contains("Malformed c="));

  const auto resolved = result.value->effectiveConnection(0);
  REQUIRE(resolved.has_value());
  REQUIRE(resolved->address == "198.51.100.9");
}

TEST_CASE("Parser: IPv6 multicast /N is an address count, not a TTL",
          "[parser][connection]")
{
  // RFC 4566 §5.7: "the TTL value MUST NOT be present for IPv6 multicast";
  // c=IN IP6 FF15::101/3 denotes THREE addresses.
  const std::string sdp =
    sdpWithMediaLines("", "c=IN IP6 FF15::101/3\r\n");

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());

  const auto resolved = result.value->effectiveConnection(0);
  REQUIRE(resolved.has_value());
  REQUIRE(resolved->address == "FF15::101");
  REQUIRE(resolved->addrType == AddressType::IP6);
  REQUIRE(!resolved->ttl.has_value());
  REQUIRE(resolved->numberOfAddresses.has_value());
  REQUIRE(*resolved->numberOfAddresses == 3);
}

TEST_CASE("Parser: IPv4 multicast keeps ttl and count", "[parser][connection]")
{
  const std::string sdp = sdpWithMediaLines("", "c=IN IP4 224.2.1.1/127/3\r\n");

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());

  const auto resolved = result.value->effectiveConnection(0);
  REQUIRE(resolved.has_value());
  REQUIRE(resolved->address == "224.2.1.1");
  REQUIRE(resolved->ttl.has_value());
  REQUIRE(*resolved->ttl == 127);
  REQUIRE(resolved->numberOfAddresses.has_value());
  REQUIRE(*resolved->numberOfAddresses == 3);
}

TEST_CASE("Parser: an IPv6 c= carrying a TTL group is rejected",
          "[parser][connection]")
{
  // Not merely a semantic MUST NOT: the §5.7 grammar admits only one slash
  // group for IPv6 (IP6-multicast = hexpart [ "/" integer ]), and the two
  // groups are not disambiguable, so guessing would invent a layer count.
  const std::string sdp = sdpWithMediaLines("", "c=IN IP6 FF15::101/16/3\r\n");

  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(!result.value->mediaDescriptions[0].connection.has_value());
  REQUIRE(!result.warnings.empty());
}

TEST_CASE("Parser: a repeated c= is reported, not silently discarded",
          "[parser][connection]")
{
  // §5.7 permits repeated media-level c= lines ONLY for multicast layers, so a
  // fixture for that permission must actually be multicast — 224.0.0.0/4 here.
  // A unicast pair would assert strict-mode acceptance of an input §5.7 does not
  // permit, while claiming to protect one it does.
  const std::string layeredMulticast = sdpWithMediaLines(
    "", "c=IN IP4 224.2.1.1/127\r\nc=IN IP4 224.2.1.2/127\r\n");

  SECTION("media level keeps the FIRST line — the layered-multicast base")
  {
    // §5.7's slash form is <base multicast address>[/<ttl>]/<count>, and its
    // stated multi-line equivalent lists the base first. An enhancement layer
    // alone is undecodable, so the base is the one to keep. (The RFC states no
    // ordering for separate c= lines; keeping the first follows the slash form.)
    auto result = SdpParser::parse(layeredMulticast);
    REQUIRE(result.hasValue());
    REQUIRE(result.value->mediaDescriptions[0].connection.has_value());
    REQUIRE(result.value->mediaDescriptions[0].connection.has_value());
    REQUIRE(result.value->mediaDescriptions[0].connection->address == "224.2.1.1");
    REQUIRE_THAT(result.warnings.back().message,
                 Catch::Matchers::Contains("layered multicast"));
  }

  SECTION("media level is legal SDP, so strict mode must NOT reject it")
  {
    // Rejecting here would declare a conformant layered-multicast description
    // malformed (§5.7: multiple per-media c= lines MAY be specified).
    SdpParserOptions opts;
    opts.strict = true;
    auto result = SdpParser::parse(layeredMulticast, opts);
    REQUIRE(result.hasValue());
    REQUIRE(!result.hasError());
    REQUIRE(result.value->mediaDescriptions[0].connection.has_value());
    REQUIRE(result.value->mediaDescriptions[0].connection->address == "224.2.1.1");
  }

  SECTION("a repeated UNICAST media c= is also accepted — a known limitation")
  {
    // §5.7's permission is conditional on multicast, but the parser treats the
    // address as an opaque string (FQDNs are legal), so it cannot classify the
    // base address and does not enforce the condition. Pinned so the limitation
    // is deliberate and visible rather than assumed.
    const std::string unicastPair =
      sdpWithMediaLines("", "c=IN IP4 203.0.113.7\r\nc=IN IP4 203.0.113.8\r\n");

    SdpParserOptions opts;
    opts.strict = true;
    auto result = SdpParser::parse(unicastPair, opts);
    REQUIRE(result.hasValue());
    REQUIRE(result.value->mediaDescriptions[0].connection->address == "203.0.113.7");
  }

  SECTION("session level is forbidden outright by §5.7")
  {
    const std::string sdp =
      "v=0\r\n"
      "o=- 1 1 IN IP4 198.51.100.9\r\n"
      "s=-\r\n"
      "c=IN IP4 198.51.100.9\r\n"
      "c=IN IP4 203.0.113.7\r\n"
      "t=0 0\r\n"
      "m=audio 49170 RTP/AVP 0\r\n";

    SECTION("strict mode errors")
    {
      SdpParserOptions opts;
      opts.strict = true;
      auto result = SdpParser::parse(sdp, opts);
      REQUIRE(!result.hasValue());
      REQUIRE(result.hasError());
    }

    SECTION("lenient mode warns and keeps the first")
    {
      auto result = SdpParser::parse(sdp);
      REQUIRE(result.hasValue());
      REQUIRE(result.value->connection.has_value());
      REQUIRE(result.value->connection->address == "198.51.100.9");
      REQUIRE_THAT(result.warnings.back().message,
                   Catch::Matchers::Contains("Duplicate session-level"));
    }
  }

  SECTION("a valid c= followed by a malformed one is reported as MALFORMED")
  {
    // The duplicate check runs after the parse, so the diagnostic names the
    // real defect rather than claiming an overwrite that never happened.
    const std::string sdp =
      sdpWithMediaLines("", "c=IN IP4 203.0.113.7\r\nc=IN IP4\r\n");

    auto result = SdpParser::parse(sdp);
    REQUIRE(result.hasValue());
    REQUIRE(result.value->mediaDescriptions[0].connection.has_value());
    REQUIRE(result.value->mediaDescriptions[0].connection->address == "203.0.113.7");
    REQUIRE_THAT(result.warnings.back().message,
                 Catch::Matchers::Contains("Malformed c="));
  }

  SECTION("the same ordering holds at the SESSION site")
  {
    // The session site carries its own copy of the parse-before-repeat order.
    const std::string sdp =
      "v=0\r\n"
      "o=- 1 1 IN IP4 198.51.100.9\r\n"
      "s=-\r\n"
      "c=IN IP4 198.51.100.9\r\n"
      "c=IN IP4\r\n"
      "t=0 0\r\n"
      "m=audio 49170 RTP/AVP 0\r\n";

    auto lenient = SdpParser::parse(sdp);
    REQUIRE(lenient.hasValue());
    REQUIRE(lenient.value->connection.has_value());
    REQUIRE(lenient.value->connection->address == "198.51.100.9");
    REQUIRE_THAT(lenient.warnings.back().message,
                 Catch::Matchers::Contains("Malformed c="));

    SdpParserOptions opts;
    opts.strict = true;
    auto strict = SdpParser::parse(sdp, opts);
    REQUIRE(!strict.hasValue());
    REQUIRE(strict.hasError());
    REQUIRE(strict.error->message == "Malformed c= line");
  }

  SECTION("a malformed c= followed by a valid one keeps the first USABLE value")
  {
    const std::string sdp =
      sdpWithMediaLines("", "c=IN IP4\r\nc=IN IP4 203.0.113.8\r\n");

    auto result = SdpParser::parse(sdp);
    REQUIRE(result.hasValue());
    REQUIRE(result.value->mediaDescriptions[0].connection.has_value());
    REQUIRE(result.value->mediaDescriptions[0].connection->address == "203.0.113.8");
  }
}

TEST_CASE("Parser: a=rtcp stores no address without a recognised family",
          "[parser][rtcp]")
{
  // RFC 3605 §2.1 — the address is meaningful only with its nettype/addrtype.
  // A half-populated attribute names an RTCP destination the serializer then
  // drops silently.
  SECTION("unknown addrtype leaves the whole address unset")
  {
    auto result = SdpParser::parseMediaSection(
      "m=audio 49170 RTP/AVP 0\r\na=rtcp:53020 IN IP9 198.51.100.9\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(result.value->rtcp.has_value());
    REQUIRE(result.value->rtcp->port == 53020);
    REQUIRE(!result.value->rtcp->address.has_value());
    REQUIRE(!result.value->rtcp->addrType.has_value());
  }

  SECTION("unknown nettype leaves the whole address unset")
  {
    // The other half of the `nt && at` guard — without this, dropping the
    // nettype check would leave the suite green.
    auto result = SdpParser::parseMediaSection(
      "m=audio 49170 RTP/AVP 0\r\na=rtcp:53020 XX IP4 198.51.100.9\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(result.value->rtcp.has_value());
    REQUIRE(!result.value->rtcp->address.has_value());
    REQUIRE(!result.value->rtcp->netType.has_value());
  }

  SECTION("a truncated address group leaves the address unset")
  {
    auto result = SdpParser::parseMediaSection(
      "m=audio 49170 RTP/AVP 0\r\na=rtcp:53020 IN IP4\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(result.value->rtcp.has_value());
    REQUIRE(result.value->rtcp->port == 53020);
    REQUIRE(!result.value->rtcp->address.has_value());
  }

  SECTION("a well-formed a=rtcp keeps address and family together")
  {
    auto result = SdpParser::parseMediaSection(
      "m=audio 49170 RTP/AVP 0\r\na=rtcp:53020 IN IP4 198.51.100.9\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(result.value->rtcp.has_value());
    REQUIRE(result.value->rtcp->address.has_value());
    REQUIRE(*result.value->rtcp->address == "198.51.100.9");
    REQUIRE(result.value->rtcp->addrType.has_value());
    REQUIRE(*result.value->rtcp->addrType == AddressType::IP4);
  }
}

TEST_CASE("Parser: a=rtcp with no usable port is not stored", "[parser][rtcp]")
{
  // RFC 3605 §2.1 makes the port mandatory. RtcpAttribute::port is a plain
  // uint16_t, so leaving it defaulted would fabricate port 0 — indistinguishable
  // from a legitimate a=rtcp:0, and the serializer would emit it as fact.
  SECTION("an unparseable port drops the whole attribute")
  {
    auto result = SdpParser::parseMediaSection(
      "m=audio 49170 RTP/AVP 0\r\na=rtcp:abc IN IP4 198.51.100.9\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(!result.value->rtcp.has_value());
  }

  SECTION("an out-of-range port drops the whole attribute")
  {
    auto result = SdpParser::parseMediaSection(
      "m=audio 49170 RTP/AVP 0\r\na=rtcp:70000\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(!result.value->rtcp.has_value());
  }

  SECTION("a bare a=rtcp drops the whole attribute")
  {
    auto result = SdpParser::parseMediaSection(
      "m=audio 49170 RTP/AVP 0\r\na=rtcp\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(!result.value->rtcp.has_value());
  }

  SECTION("a legitimate a=rtcp:0 IS still stored — the discriminating case")
  {
    auto result = SdpParser::parseMediaSection(
      "m=audio 49170 RTP/AVP 0\r\na=rtcp:0 IN IP4 198.51.100.9\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(result.value->rtcp.has_value());
    REQUIRE(result.value->rtcp->port == 0);
  }
}

TEST_CASE("Parser: an unrecognised nettype or addrtype is not coerced to IN/IP4",
          "[parser][connection]")
{
  // Silently defaulting would both mislabel the address family AND pick the
  // IPv4 slash grammar, so c=IN IP9 FF15::101/3 would yield an IPv6 literal
  // tagged IP4 carrying a bogus TTL — and re-serialize as c=IN IP4 FF15::101/3.
  SECTION("unknown addrtype: the media c= is dropped, the session c= folds down")
  {
    const std::string sdp =
      sdpWithMediaLines("c=IN IP4 198.51.100.9\r\n", "c=IN IP9 FF15::101/3\r\n");

    auto result = SdpParser::parse(sdp);
    REQUIRE(result.hasValue());
    REQUIRE(!result.value->mediaDescriptions[0].connection.has_value());

    const auto resolved = result.value->effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->addrType == AddressType::IP4);
    REQUIRE(resolved->address == "198.51.100.9");
  }

  SECTION("unknown nettype is likewise rejected")
  {
    const std::string sdp = sdpWithMediaLines("", "c=TN RFC2543 +1-201-406-4090\r\n");

    auto result = SdpParser::parse(sdp);
    REQUIRE(result.hasValue());
    REQUIRE(!result.value->mediaDescriptions[0].connection.has_value());
    REQUIRE(!result.value->effectiveConnection(0).has_value());
  }

  SECTION("strict mode errors on an unknown addrtype")
  {
    const std::string sdp = sdpWithMediaLines("", "c=IN IP9 2001:db8::1\r\n");

    SdpParserOptions opts;
    opts.strict = true;
    auto result = SdpParser::parse(sdp, opts);
    REQUIRE(!result.hasValue());
    REQUIRE(result.hasError());
  }
}

TEST_CASE("Parser: parseMediaSection resolves c= lines", "[parser][connection]")
{
  // The third parseConnection call site. It is always lenient — there is no
  // strict mode for a standalone media section.
  SECTION("a well-formed media-level c= is stored")
  {
    auto result = SdpParser::parseMediaSection(
      "m=audio 49170 RTP/AVP 0\r\nc=IN IP4 203.0.113.7\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(result.value->connection.has_value());
    REQUIRE(result.value->connection->address == "203.0.113.7");
  }

  SECTION("a malformed c= is dropped with a warning, never left empty")
  {
    auto result = SdpParser::parseMediaSection(
      "m=audio 49170 RTP/AVP 0\r\nc=IN IP4\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(!result.value->connection.has_value());
    REQUIRE(!result.warnings.empty());
  }

  SECTION("a repeated c= keeps the first and warns")
  {
    auto result = SdpParser::parseMediaSection(
      "m=audio 49170 RTP/AVP 0\r\nc=IN IP4 224.2.1.1/127\r\nc=IN IP4 224.2.1.2/127\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(result.value->connection.has_value());
    REQUIRE(result.value->connection->address == "224.2.1.1");
    REQUIRE_THAT(result.warnings.back().message,
                 Catch::Matchers::Contains("layered multicast"));
  }

  SECTION("parse-before-repeat ordering holds here too")
  {
    auto result = SdpParser::parseMediaSection(
      "m=audio 49170 RTP/AVP 0\r\nc=IN IP4 203.0.113.7\r\nc=IN IP4\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(result.value->connection.has_value());
    REQUIRE(result.value->connection->address == "203.0.113.7");
    REQUIRE_THAT(result.warnings.back().message,
                 Catch::Matchers::Contains("Malformed c="));
  }

  SECTION("IPv6 multicast /N is an address count here too")
  {
    auto result = SdpParser::parseMediaSection(
      "m=audio 49170 RTP/AVP 0\r\nc=IN IP6 FF15::101/3\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(result.value->connection.has_value());
    REQUIRE(!result.value->connection->ttl.has_value());
    REQUIRE(result.value->connection->numberOfAddresses.has_value());
    REQUIRE(*result.value->connection->numberOfAddresses == 3);
  }
}

TEST_CASE("Parser: effectiveConnection over realistic parsed offers",
          "[parser][connection]")
{
  SECTION("a plain SIP offer folds the session c= down")
  {
    const std::string sdp = sdpWithMediaLines("c=IN IP4 198.51.100.9\r\n", "");

    auto result = SdpParser::parse(sdp);
    REQUIRE(result.hasValue());
    const auto resolved = result.value->effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->address == "198.51.100.9");
  }

  SECTION("an unspecified session address survives the fold-down")
  {
    // Deliberately NOT labelled "hold": a bare session-level 0.0.0.0 with
    // default sendrecv is equally RFC 3264 §8.4's address-not-yet-known case.
    // The resolver is indifferent either way — that is the point.
    const std::string sdp = sdpWithMediaLines("c=IN IP4 0.0.0.0\r\n", "");

    auto result = SdpParser::parse(sdp);
    REQUIRE(result.hasValue());
    const auto resolved = result.value->effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->address == "0.0.0.0");
  }

  SECTION("an a=inactive hold offer resolves its real address unchanged")
  {
    // Modern hold (RFC 3264 §8.4) keeps a real connection address; the hold
    // signal is the direction attribute, which this resolver does not read.
    const std::string sdp =
      sdpWithMediaLines("c=IN IP4 198.51.100.9\r\n", "a=inactive\r\n");

    auto result = SdpParser::parse(sdp);
    REQUIRE(result.hasValue());
    const auto resolved = result.value->effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->address == "198.51.100.9");
  }

  SECTION("a trickle-ICE BUNDLE offer keeps its 0.0.0.0 placeholder per section")
  {
    // RFC 8840 §4.1.1 — the placeholder is NOT hold; the real address arrives
    // via candidates. A real offer of this shape carries a=mid on every grouped
    // section (RFC 5888 §5 / RFC 8843), DTLS-SRTP over UDP/TLS/RTP/SAVPF with
    // a=fingerprint + a=setup, and a=ice-options:trickle.
    const std::string sdp =
      "v=0\r\n"
      "o=- 1 1 IN IP4 0.0.0.0\r\n"
      "s=-\r\n"
      "t=0 0\r\n"
      "a=group:BUNDLE 0 1\r\n"
      "a=ice-ufrag:4ZcD\r\n"
      "a=ice-pwd:2/1muCWoOi3uLifh0NuRHlZ\r\n"
      "a=ice-options:trickle\r\n"
      "a=fingerprint:sha-256 "
      "12:5A:31:9C:7B:2E:44:8D:6F:01:AB:CD:EF:23:45:67:"
      "89:0A:BC:DE:F1:23:45:67:89:AB:CD:EF:01:23:45:67\r\n"
      "a=setup:actpass\r\n"
      "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
      "c=IN IP4 0.0.0.0\r\n"
      "a=mid:0\r\n"
      "a=rtcp-mux\r\n"
      "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
      "c=IN IP4 0.0.0.0\r\n"
      "a=mid:1\r\n"
      "a=rtcp-mux\r\n";

    auto result = SdpParser::parse(sdp);
    REQUIRE(result.hasValue());
    REQUIRE(result.value->mediaDescriptions.size() == 2);
    // The BUNDLE group references these mids, so they must actually be present.
    REQUIRE(result.value->mediaDescriptions[0].mid == "0");
    REQUIRE(result.value->mediaDescriptions[1].mid == "1");

    const auto audio = result.value->effectiveConnection(0);
    const auto video = result.value->effectiveConnection(1);
    REQUIRE(audio.has_value());
    REQUIRE(video.has_value());
    REQUIRE(audio->address == "0.0.0.0");
    REQUIRE(video->address == "0.0.0.0");
  }

  SECTION("no c= at any level: every index is nullopt (the malformed contract)")
  {
    // This is the shape the consumer must reject; it was only ever asserted at
    // model level, never over the parser.
    const std::string sdp = sdpWithMediaLines("", "");

    auto result = SdpParser::parse(sdp);
    REQUIRE(result.hasValue());
    REQUIRE(!result.value->connection.has_value());
    REQUIRE(!result.value->effectiveConnection(0).has_value());
  }

  SECTION("mixed-family fold-down: a media-level IPv6 c= over a session IPv4 c=")
  {
    // A dual-stack offer — the caller must not infer the family from the
    // session level.
    const std::string sdp =
      sdpWithMediaLines("c=IN IP4 198.51.100.9\r\n", "c=IN IP6 2001:db8::1\r\n");

    auto result = SdpParser::parse(sdp);
    REQUIRE(result.hasValue());
    const auto resolved = result.value->effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->addrType == AddressType::IP6);
    REQUIRE(resolved->address == "2001:db8::1");
  }

  SECTION("a rejected stream still resolves its address")
  {
    const std::string sdp =
      "v=0\r\n"
      "o=- 1 1 IN IP4 198.51.100.9\r\n"
      "s=-\r\n"
      "c=IN IP4 198.51.100.9\r\n"
      "t=0 0\r\n"
      "m=audio 49170 RTP/AVP 0\r\n"
      "m=video 0 RTP/AVP 31\r\n";

    auto result = SdpParser::parse(sdp);
    REQUIRE(result.hasValue());
    REQUIRE(result.value->mediaDescriptions.size() == 2);
    REQUIRE(result.value->mediaDescriptions[1].port == 0);

    const auto rejected = result.value->effectiveConnection(1);
    REQUIRE(rejected.has_value());
    REQUIRE(rejected->address == "198.51.100.9");
  }
}

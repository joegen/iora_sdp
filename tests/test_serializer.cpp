#include <catch2/catch.hpp>
#include <string>
#include <vector>

#include "iora/sdp/sdp_serializer.hpp"

using namespace iora::sdp;

// ============================================================================
// T8: Field ordering and format tests
// ============================================================================

TEST_CASE("Serializer: Minimal SessionDescription", "[serializer]")
{
  SessionDescription session;
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("v=0\r\n") != std::string::npos);
  CHECK(sdp.find("o=- 0 0 IN IP4 0.0.0.0\r\n") != std::string::npos);
  CHECK(sdp.find("s=-\r\n") != std::string::npos);
  CHECK(sdp.find("t=0 0\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Session-level fields in RFC order", "[serializer]")
{
  SessionDescription session;
  session.version = 0;
  session.origin = {"alice", "12345", "2", NetworkType::IN, AddressType::IP4, "192.168.1.1"};
  session.sessionName = "Test Session";
  session.sessionInfo = "A test";
  session.uri = "http://example.com";
  session.emailAddress = "alice@example.com";
  session.phoneNumber = "+1-555-1234";
  session.connection = ConnectionData{NetworkType::IN, AddressType::IP4, "192.168.1.1", std::nullopt, std::nullopt};
  session.bandwidths = {{BandwidthType::AS, 1000}, {BandwidthType::CT, 500}};

  TimeDescription td;
  td.startTime = 3034423619;
  td.stopTime = 3034423619;
  RepeatTime rt;
  rt.repeatInterval = "7d";
  rt.activeDuration = "1h";
  rt.offsets = {"0", "25h"};
  td.repeatTimes.push_back(rt);
  session.timeDescriptions.push_back(td);

  session.zoneAdjustments = {{"2882844526", "-1h"}, {"2898848070", "0"}};
  session.encryptionKey = "base64:key123";
  session.iceUfrag = "ufrag1";
  session.icePwd = "pwd1";
  session.iceOptions = {"trickle", "ice2"};
  session.iceLite = true;
  session.fingerprints.push_back({"sha-256", "AA:BB:CC"});
  session.setup = SetupRole::ACTPASS;
  session.extmapAllowMixed = true;
  session.groups.push_back({"BUNDLE", {"0", "1"}});
  session.attributes.push_back({"tool", std::string("test")});

  auto sdp = SdpSerializer::serialize(session);

  auto posV = sdp.find("v=0");
  auto posO = sdp.find("o=alice");
  auto posS = sdp.find("s=Test Session");
  auto posI = sdp.find("i=A test");
  auto posU = sdp.find("u=http://example.com");
  auto posE = sdp.find("e=alice@example.com");
  auto posP = sdp.find("p=+1-555-1234");
  auto posC = sdp.find("c=IN IP4 192.168.1.1");
  auto posB = sdp.find("b=AS:1000");
  auto posT = sdp.find("t=3034423619 3034423619");
  auto posR = sdp.find("r=7d 1h 0 25h");
  auto posZ = sdp.find("z=2882844526 -1h 2898848070 0");
  auto posK = sdp.find("k=base64:key123");
  auto posA = sdp.find("a=ice-ufrag:ufrag1");

  CHECK(posV < posO);
  CHECK(posO < posS);
  CHECK(posS < posI);
  CHECK(posI < posU);
  CHECK(posU < posE);
  CHECK(posE < posP);
  CHECK(posP < posC);
  CHECK(posC < posB);
  CHECK(posB < posT);
  CHECK(posT < posR);
  CHECK(posR < posZ);
  CHECK(posZ < posK);
  CHECK(posK < posA);
}

TEST_CASE("Serializer: Origin serialization", "[serializer]")
{
  SessionDescription session;
  session.origin = {"bob", "9876543210", "42", NetworkType::IN, AddressType::IP6, "::1"};
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("o=bob 9876543210 42 IN IP6 ::1\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Connection with TTL and numberOfAddresses", "[serializer]")
{
  SessionDescription session;
  ConnectionData conn;
  conn.netType = NetworkType::IN;
  conn.addrType = AddressType::IP4;
  conn.address = "224.2.36.42";
  conn.ttl = 127;
  conn.numberOfAddresses = 3;
  session.connection = conn;
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("c=IN IP4 224.2.36.42/127/3\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Connection with TTL only", "[serializer]")
{
  SessionDescription session;
  ConnectionData conn;
  conn.netType = NetworkType::IN;
  conn.addrType = AddressType::IP4;
  conn.address = "224.2.36.42";
  conn.ttl = 127;
  session.connection = conn;
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("c=IN IP4 224.2.36.42/127\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Connection without TTL", "[serializer]")
{
  SessionDescription session;
  ConnectionData conn;
  conn.address = "192.168.1.1";
  session.connection = conn;
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("c=IN IP4 192.168.1.1\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Multiple b= lines", "[serializer]")
{
  SessionDescription session;
  session.bandwidths = {{BandwidthType::AS, 1000}, {BandwidthType::TIAS, 900000}};
  auto sdp = SdpSerializer::serialize(session);
  auto posAS = sdp.find("b=AS:1000\r\n");
  auto posTIAS = sdp.find("b=TIAS:900000\r\n");
  CHECK(posAS != std::string::npos);
  CHECK(posTIAS != std::string::npos);
  CHECK(posAS < posTIAS);
}

TEST_CASE("Serializer: TimeDescription with RepeatTime", "[serializer]")
{
  SessionDescription session;
  TimeDescription td;
  td.startTime = 3034423619;
  td.stopTime = 3034423619;
  RepeatTime rt;
  rt.repeatInterval = "604800";
  rt.activeDuration = "3600";
  rt.offsets = {"0", "90000"};
  td.repeatTimes.push_back(rt);
  session.timeDescriptions.push_back(td);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("t=3034423619 3034423619\r\n") != std::string::npos);
  CHECK(sdp.find("r=604800 3600 0 90000\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Zone adjustments on single z= line", "[serializer]")
{
  SessionDescription session;
  session.zoneAdjustments = {{"2882844526", "-1h"}, {"2898848070", "0"}};
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("z=2882844526 -1h 2898848070 0\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Encryption key", "[serializer]")
{
  SessionDescription session;
  session.encryptionKey = "base64:somekey";
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("k=base64:somekey\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Bare LF line ending option", "[serializer]")
{
  SdpSerializerOptions opts;
  opts.lineEnding = "\n";
  SessionDescription session;
  auto sdp = SdpSerializer::serialize(session, opts);
  CHECK(sdp.find("\r\n") == std::string::npos);
  CHECK(sdp.find("v=0\n") != std::string::npos);
  CHECK(sdp.find("t=0 0\n") != std::string::npos);
}

TEST_CASE("Serializer: omitSessionName=true with empty sessionName", "[serializer]")
{
  SdpSerializerOptions opts;
  opts.omitSessionName = true;
  SessionDescription session;
  session.sessionName = "";
  auto sdp = SdpSerializer::serialize(session, opts);
  CHECK(sdp.find("s=") == std::string::npos);
}

TEST_CASE("Serializer: omitSessionName=true with non-empty sessionName", "[serializer]")
{
  SdpSerializerOptions opts;
  opts.omitSessionName = true;
  SessionDescription session;
  session.sessionName = "My Session";
  auto sdp = SdpSerializer::serialize(session, opts);
  CHECK(sdp.find("s=My Session\r\n") != std::string::npos);
}

TEST_CASE("Serializer: omitSessionName=false with empty sessionName", "[serializer]")
{
  SessionDescription session;
  session.sessionName = "";
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("s=\r\n") != std::string::npos);
}

TEST_CASE("Serializer: omitTiming=true with empty timeDescriptions", "[serializer]")
{
  SdpSerializerOptions opts;
  opts.omitTiming = true;
  SessionDescription session;
  auto sdp = SdpSerializer::serialize(session, opts);
  CHECK(sdp.find("t=") == std::string::npos);
}

TEST_CASE("Serializer: omitTiming=false with empty timeDescriptions", "[serializer]")
{
  SessionDescription session;
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("t=0 0\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Session info (i=) line", "[serializer]")
{
  SessionDescription session;
  session.sessionInfo = "Session description text";
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("i=Session description text\r\n") != std::string::npos);
}

TEST_CASE("Serializer: URI (u=) line", "[serializer]")
{
  SessionDescription session;
  session.uri = "http://example.com/session";
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("u=http://example.com/session\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Email (e=) line", "[serializer]")
{
  SessionDescription session;
  session.emailAddress = "test@example.com";
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("e=test@example.com\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Phone (p=) line", "[serializer]")
{
  SessionDescription session;
  session.phoneNumber = "+1-555-0000";
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("p=+1-555-0000\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Optional fields omitted when not set", "[serializer]")
{
  SessionDescription session;
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("i=") == std::string::npos);
  CHECK(sdp.find("u=") == std::string::npos);
  CHECK(sdp.find("e=") == std::string::npos);
  CHECK(sdp.find("p=") == std::string::npos);
  CHECK(sdp.find("c=") == std::string::npos);
  CHECK(sdp.find("b=") == std::string::npos);
  CHECK(sdp.find("z=") == std::string::npos);
  CHECK(sdp.find("k=") == std::string::npos);
}

// ============================================================================
// T9: Attribute serialization and round-trip tests
// ============================================================================

TEST_CASE("Serializer: Session-level attributes in order", "[serializer]")
{
  SessionDescription session;
  session.iceUfrag = "abc";
  session.icePwd = "def";
  session.iceOptions = {"trickle"};
  session.iceLite = true;
  session.fingerprints.push_back({"sha-256", "AA:BB"});
  session.setup = SetupRole::ACTPASS;
  session.extmapAllowMixed = true;
  session.groups.push_back({"BUNDLE", {"0", "1"}});
  session.attributes.push_back({"tool", std::string("test")});

  auto sdp = SdpSerializer::serialize(session);

  auto posUfrag = sdp.find("a=ice-ufrag:abc");
  auto posPwd = sdp.find("a=ice-pwd:def");
  auto posOpts = sdp.find("a=ice-options:trickle");
  auto posLite = sdp.find("a=ice-lite");
  auto posFp = sdp.find("a=fingerprint:sha-256 AA:BB");
  auto posSetup = sdp.find("a=setup:actpass");
  auto posEAM = sdp.find("a=extmap-allow-mixed");
  auto posGroup = sdp.find("a=group:BUNDLE 0 1");
  auto posTool = sdp.find("a=tool:test");

  CHECK(posUfrag < posPwd);
  CHECK(posPwd < posOpts);
  CHECK(posOpts < posLite);
  CHECK(posLite < posFp);
  CHECK(posFp < posSetup);
  CHECK(posSetup < posEAM);
  CHECK(posEAM < posGroup);
  CHECK(posGroup < posTool);
}

TEST_CASE("Serializer: Session-level iceOptions space-separated", "[serializer]")
{
  SessionDescription session;
  session.iceOptions = {"trickle", "ice2"};
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=ice-options:trickle ice2\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Empty iceOptions vector omits line", "[serializer]")
{
  SessionDescription session;
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=ice-options") == std::string::npos);
}

TEST_CASE("Serializer: Multiple GroupAttribute at session level", "[serializer]")
{
  SessionDescription session;
  session.groups.push_back({"BUNDLE", {"0", "1", "2"}});
  session.groups.push_back({"LS", {"0", "1"}});
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=group:BUNDLE 0 1 2\r\n") != std::string::npos);
  CHECK(sdp.find("a=group:LS 0 1\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Multiple FingerprintAttribute at session and media level", "[serializer]")
{
  SessionDescription session;
  session.fingerprints.push_back({"sha-256", "AA:BB:CC"});
  session.fingerprints.push_back({"sha-1", "DD:EE:FF"});
  MediaDescription media;
  media.formats = {"0"};
  media.fingerprints.push_back({"sha-256", "11:22:33"});
  media.fingerprints.push_back({"sha-384", "44:55:66"});
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=fingerprint:sha-256 AA:BB:CC") != std::string::npos);
  CHECK(sdp.find("a=fingerprint:sha-1 DD:EE:FF") != std::string::npos);
  CHECK(sdp.find("a=fingerprint:sha-256 11:22:33") != std::string::npos);
  CHECK(sdp.find("a=fingerprint:sha-384 44:55:66") != std::string::npos);
}

TEST_CASE("Serializer: Media section m= line format", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.mediaType = MediaType::AUDIO;
  media.port = 9;
  media.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
  media.formats = {"111", "0"};
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("m=audio 9 UDP/TLS/RTP/SAVPF 111 0\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Media without numberOfPorts", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.port = 5004;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  auto mLine = sdp.find("m=audio 5004 RTP/AVP");
  CHECK(mLine != std::string::npos);
  CHECK(sdp.find("m=audio 5004/") == std::string::npos);
}

TEST_CASE("Serializer: Media with numberOfPorts", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.port = 5004;
  media.numberOfPorts = 2;
  media.formats = {"0"};
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("m=audio 5004/2 RTP/AVP 0\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Direction attribute serialized", "[serializer]")
{
  SessionDescription session;

  SECTION("sendrecv")
  {
    MediaDescription m;
    m.direction = Direction::SENDRECV;
    session.mediaDescriptions.push_back(m);
    auto sdp = SdpSerializer::serialize(session);
    CHECK(sdp.find("a=sendrecv\r\n") != std::string::npos);
  }
  SECTION("sendonly")
  {
    MediaDescription m;
    m.direction = Direction::SENDONLY;
    session.mediaDescriptions.push_back(m);
    auto sdp = SdpSerializer::serialize(session);
    CHECK(sdp.find("a=sendonly\r\n") != std::string::npos);
  }
  SECTION("recvonly")
  {
    MediaDescription m;
    m.direction = Direction::RECVONLY;
    session.mediaDescriptions.push_back(m);
    auto sdp = SdpSerializer::serialize(session);
    CHECK(sdp.find("a=recvonly\r\n") != std::string::npos);
  }
  SECTION("inactive")
  {
    MediaDescription m;
    m.direction = Direction::INACTIVE;
    session.mediaDescriptions.push_back(m);
    auto sdp = SdpSerializer::serialize(session);
    CHECK(sdp.find("a=inactive\r\n") != std::string::npos);
  }
}

TEST_CASE("Serializer: mid, msid, ice-ufrag, ice-pwd, ice-options at media level", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.mid = "0";
  media.msid.push_back({"stream1", std::string("track1")});
  media.iceUfrag = "uf1";
  media.icePwd = "pw1";
  media.iceOptions = {"trickle", "ice2"};
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=mid:0\r\n") != std::string::npos);
  CHECK(sdp.find("a=msid:stream1 track1\r\n") != std::string::npos);
  CHECK(sdp.find("a=ice-ufrag:uf1\r\n") != std::string::npos);
  CHECK(sdp.find("a=ice-pwd:pw1\r\n") != std::string::npos);
  CHECK(sdp.find("a=ice-options:trickle ice2\r\n") != std::string::npos);
}

TEST_CASE("Serializer: msid without trackId", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.msid.push_back({"stream1", std::nullopt});
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=msid:stream1\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Flag attributes", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.rtcpMux = true;
  media.rtcpMuxOnly = true;
  media.rtcpRsize = true;
  media.extmapAllowMixed = true;
  media.endOfCandidates = true;
  session.mediaDescriptions.push_back(media);
  session.iceLite = true;
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=rtcp-mux\r\n") != std::string::npos);
  CHECK(sdp.find("a=rtcp-mux-only\r\n") != std::string::npos);
  CHECK(sdp.find("a=rtcp-rsize\r\n") != std::string::npos);
  CHECK(sdp.find("a=extmap-allow-mixed\r\n") != std::string::npos);
  CHECK(sdp.find("a=end-of-candidates\r\n") != std::string::npos);
  CHECK(sdp.find("a=ice-lite\r\n") != std::string::npos);
}

TEST_CASE("Serializer: rtpmap with channels", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.formats = {"111"};
  RtpMapAttribute rm;
  rm.payloadType = 111;
  rm.encodingName = "opus";
  rm.clockRate = 48000;
  rm.channels = 2;
  media.rtpMaps.push_back(rm);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=rtpmap:111 opus/48000/2\r\n") != std::string::npos);
}

TEST_CASE("Serializer: rtpmap without channels", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.formats = {"96"};
  RtpMapAttribute rm;
  rm.payloadType = 96;
  rm.encodingName = "VP8";
  rm.clockRate = 90000;
  media.rtpMaps.push_back(rm);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=rtpmap:96 VP8/90000\r\n") != std::string::npos);
}

TEST_CASE("Serializer: fmtp raw parameters preserved", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.formats = {"111"};
  RtpMapAttribute rm;
  rm.payloadType = 111;
  rm.encodingName = "opus";
  rm.clockRate = 48000;
  media.rtpMaps.push_back(rm);
  FmtpAttribute fmtp;
  fmtp.payloadType = 111;
  fmtp.parameters = "minptime=10;useinbandfec=1";
  media.fmtps.push_back(fmtp);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=fmtp:111 minptime=10;useinbandfec=1\r\n") != std::string::npos);
}

TEST_CASE("Serializer: rtcp-fb with and without subtype", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.formats = {"111"};
  RtpMapAttribute rm;
  rm.payloadType = 111;
  rm.encodingName = "opus";
  rm.clockRate = 48000;
  media.rtpMaps.push_back(rm);
  media.rtcpFeedbacks.push_back({"111", "nack", std::string("pli")});
  media.rtcpFeedbacks.push_back({"111", "transport-cc", std::nullopt});
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=rtcp-fb:111 nack pli\r\n") != std::string::npos);
  CHECK(sdp.find("a=rtcp-fb:111 transport-cc\r\n") != std::string::npos);
}

TEST_CASE("Serializer: rtcp-fb wildcard payload type", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.rtcpFeedbacks.push_back({"*", "transport-cc", std::nullopt});
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=rtcp-fb:* transport-cc\r\n") != std::string::npos);
}

TEST_CASE("Serializer: extmap with direction", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  ExtMapAttribute em;
  em.id = 1;
  em.direction = Direction::SENDONLY;
  em.uri = "urn:ietf:params:rtp-hdrext:ssrc-audio-level";
  media.extMaps.push_back(em);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=extmap:1/sendonly urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\n") != std::string::npos);
}

TEST_CASE("Serializer: extmap without direction", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  ExtMapAttribute em;
  em.id = 1;
  em.uri = "urn:ietf:params:rtp-hdrext:ssrc-audio-level";
  media.extMaps.push_back(em);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\n") != std::string::npos);
}

TEST_CASE("Serializer: extmap with extensionAttributes", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  ExtMapAttribute em;
  em.id = 2;
  em.uri = "urn:example";
  em.extensionAttributes = "some-attr";
  media.extMaps.push_back(em);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=extmap:2 urn:example some-attr\r\n") != std::string::npos);
}

TEST_CASE("Serializer: ssrc with and without attributeValue", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.ssrcs.push_back({3735928559, "cname", std::string("user@host")});
  media.ssrcs.push_back({3735928559, "label", std::nullopt});
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=ssrc:3735928559 cname:user@host\r\n") != std::string::npos);
  CHECK(sdp.find("a=ssrc:3735928559 label\r\n") != std::string::npos);
}

TEST_CASE("Serializer: ssrc-group", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  SsrcGroupAttribute sg;
  sg.semantics = "FID";
  sg.ssrcs = {12345, 67890};
  media.ssrcGroups.push_back(sg);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=ssrc-group:FID 12345 67890\r\n") != std::string::npos);
}

TEST_CASE("Serializer: rtcp port only", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  RtcpAttribute rtcp;
  rtcp.port = 9;
  media.rtcp = rtcp;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=rtcp:9\r\n") != std::string::npos);
}

TEST_CASE("Serializer: rtcp full form", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  RtcpAttribute rtcp;
  rtcp.port = 53020;
  rtcp.netType = NetworkType::IN;
  rtcp.addrType = AddressType::IP4;
  rtcp.address = "203.0.113.1";
  media.rtcp = rtcp;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=rtcp:53020 IN IP4 203.0.113.1\r\n") != std::string::npos);
}

TEST_CASE("Serializer: ICE candidate host", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  IceCandidate cand;
  cand.foundation = "1";
  cand.component = 1;
  cand.transport = IceTransportType::UDP;
  cand.priority = 2130706431;
  cand.address = "192.168.1.1";
  cand.port = 50000;
  cand.type = IceCandidateType::HOST;
  media.candidates.push_back(cand);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=candidate:1 1 udp 2130706431 192.168.1.1 50000 typ host\r\n") != std::string::npos);
}

TEST_CASE("Serializer: ICE candidate srflx with raddr/rport", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  IceCandidate cand;
  cand.foundation = "2";
  cand.component = 1;
  cand.transport = IceTransportType::UDP;
  cand.priority = 1694498815;
  cand.address = "203.0.113.1";
  cand.port = 50000;
  cand.type = IceCandidateType::SRFLX;
  cand.relAddr = "192.168.1.1";
  cand.relPort = 50000;
  media.candidates.push_back(cand);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=candidate:2 1 udp 1694498815 203.0.113.1 50000 typ srflx raddr 192.168.1.1 rport 50000\r\n") != std::string::npos);
}

TEST_CASE("Serializer: ICE candidate relay with raddr/rport", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  IceCandidate cand;
  cand.foundation = "3";
  cand.component = 1;
  cand.transport = IceTransportType::UDP;
  cand.priority = 16777215;
  cand.address = "198.51.100.1";
  cand.port = 50000;
  cand.type = IceCandidateType::RELAY;
  cand.relAddr = "192.168.1.1";
  cand.relPort = 50000;
  media.candidates.push_back(cand);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("typ relay raddr 192.168.1.1 rport 50000\r\n") != std::string::npos);
}

TEST_CASE("Serializer: ICE candidate TCP with tcptype", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  IceCandidate cand;
  cand.foundation = "4";
  cand.component = 1;
  cand.transport = IceTransportType::TCP;
  cand.priority = 2105524223;
  cand.address = "192.168.1.1";
  cand.port = 9;
  cand.type = IceCandidateType::HOST;
  cand.tcpType = "passive";
  media.candidates.push_back(cand);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=candidate:4 1 tcp 2105524223 192.168.1.1 9 typ host tcptype passive\r\n") != std::string::npos);
}

TEST_CASE("Serializer: ICE candidate with extension attributes", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  IceCandidate cand;
  cand.foundation = "1";
  cand.component = 1;
  cand.transport = IceTransportType::UDP;
  cand.priority = 2130706431;
  cand.address = "192.168.1.1";
  cand.port = 50000;
  cand.type = IceCandidateType::HOST;
  cand.extensions = {{"generation", "0"}, {"network-id", "1"}};
  media.candidates.push_back(cand);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("typ host generation 0 network-id 1\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Fingerprint sha-256 and sha-1", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.fingerprints.push_back({"sha-256", "AA:BB:CC:DD"});
  media.fingerprints.push_back({"sha-1", "EE:FF:00:11"});
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=fingerprint:sha-256 AA:BB:CC:DD\r\n") != std::string::npos);
  CHECK(sdp.find("a=fingerprint:sha-1 EE:FF:00:11\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Crypto with sessionParams", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  CryptoAttribute crypto;
  crypto.tag = 1;
  crypto.suite = "AES_CM_128_HMAC_SHA1_80";
  crypto.keyParams = "inline:base64key";
  crypto.sessionParams = "KDR=1 UNENCRYPTED_SRTP";
  media.crypto.push_back(crypto);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:base64key KDR=1 UNENCRYPTED_SRTP\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Crypto without sessionParams", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  CryptoAttribute crypto;
  crypto.tag = 1;
  crypto.suite = "AES_CM_128_HMAC_SHA1_80";
  crypto.keyParams = "inline:base64key";
  media.crypto.push_back(crypto);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:base64key\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Simulcast send+recv", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  SimulcastAttribute sc;
  sc.sendStreams = {{{"h", false}, {"m", false}}, {{"l", false}}};
  sc.recvStreams = {{{"h", false}}};
  media.simulcast = sc;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=simulcast:send h,m;l recv h\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Simulcast send-only", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  SimulcastAttribute sc;
  sc.sendStreams = {{{"h", false}, {"m", false}}};
  media.simulcast = sc;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=simulcast:send h,m\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Simulcast recv-only", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  SimulcastAttribute sc;
  sc.recvStreams = {{{"h", false}}, {{"l", false}}};
  media.simulcast = sc;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=simulcast:recv h;l\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Simulcast with paused streams", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  SimulcastAttribute sc;
  sc.sendStreams = {{{"h", false}, {"m", true}}, {{"l", true}}};
  media.simulcast = sc;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=simulcast:send h,~m;~l\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Rid with pt=", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  RidAttribute rid;
  rid.id = "h";
  rid.direction = RidDirection::SEND;
  rid.payloadTypes = {96, 97};
  media.rids.push_back(rid);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=rid:h send pt=96,97\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Rid with restrictions sorted alphabetically", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  RidAttribute rid;
  rid.id = "h";
  rid.direction = RidDirection::SEND;
  rid.restrictions["max-width"] = "1280";
  rid.restrictions["max-height"] = "720";
  rid.restrictions["max-fps"] = "30";
  media.rids.push_back(rid);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=rid:h send max-fps=30;max-height=720;max-width=1280\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Rid minimal (id direction only)", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  RidAttribute rid;
  rid.id = "l";
  rid.direction = RidDirection::RECV;
  media.rids.push_back(rid);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=rid:l recv\r\n") != std::string::npos);
}

TEST_CASE("Serializer: sctp-port and max-message-size", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.mediaType = MediaType::APPLICATION;
  media.protocol = TransportProtocol::UDP_DTLS_SCTP;
  media.formats = {"webrtc-datachannel"};
  media.sctpPort = 5000;
  media.maxMessageSize = 262144;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=sctp-port:5000\r\n") != std::string::npos);
  CHECK(sdp.find("a=max-message-size:262144\r\n") != std::string::npos);
}

TEST_CASE("Serializer: ptime and maxptime as integers", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.ptime = 20;
  media.maxptime = 60;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=ptime:20\r\n") != std::string::npos);
  CHECK(sdp.find("a=maxptime:60\r\n") != std::string::npos);
}

TEST_CASE("Serializer: framerate integer value", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.framerate = 30.0;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=framerate:30\r\n") != std::string::npos);
}

TEST_CASE("Serializer: framerate fractional value", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.framerate = 29.97;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  auto pos = sdp.find("a=framerate:");
  REQUIRE(pos != std::string::npos);
  auto lineEnd = sdp.find("\r\n", pos);
  auto val = sdp.substr(pos + 12, lineEnd - pos - 12);
  CHECK(val == "29.97");
}

TEST_CASE("Serializer: framerate no trailing zeros", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.framerate = 15.0;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=framerate:15\r\n") != std::string::npos);
  CHECK(sdp.find("a=framerate:15.0") == std::string::npos);
}

TEST_CASE("Serializer: Generic attributes with value and flag", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.genericAttributes.push_back({"x-custom", std::string("value1")});
  media.genericAttributes.push_back({"x-flag", std::nullopt});
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=x-custom:value1\r\n") != std::string::npos);
  CHECK(sdp.find("a=x-flag\r\n") != std::string::npos);
}

TEST_CASE("Serializer: GenericAttribute with '=' in value", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.genericAttributes.push_back({"foo", std::string("bar=baz")});
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=foo:bar=baz\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Codec grouping by payload type", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.mediaType = MediaType::AUDIO;
  media.port = 9;
  media.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
  media.formats = {"111", "0"};

  RtpMapAttribute rm1;
  rm1.payloadType = 111;
  rm1.encodingName = "opus";
  rm1.clockRate = 48000;
  rm1.channels = 2;
  media.rtpMaps.push_back(rm1);

  RtpMapAttribute rm2;
  rm2.payloadType = 0;
  rm2.encodingName = "PCMU";
  rm2.clockRate = 8000;
  media.rtpMaps.push_back(rm2);

  FmtpAttribute fmtp;
  fmtp.payloadType = 111;
  fmtp.parameters = "minptime=10;useinbandfec=1";
  media.fmtps.push_back(fmtp);

  media.rtcpFeedbacks.push_back({"111", "transport-cc", std::nullopt});
  media.rtcpFeedbacks.push_back({"111", "nack", std::nullopt});

  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);

  auto posRm1 = sdp.find("a=rtpmap:111 opus/48000/2");
  auto posFmtp = sdp.find("a=fmtp:111 minptime=10;useinbandfec=1");
  auto posFb1 = sdp.find("a=rtcp-fb:111 transport-cc");
  auto posFb2 = sdp.find("a=rtcp-fb:111 nack");
  auto posRm2 = sdp.find("a=rtpmap:0 PCMU/8000");

  REQUIRE(posRm1 != std::string::npos);
  REQUIRE(posFmtp != std::string::npos);
  REQUIRE(posFb1 != std::string::npos);
  REQUIRE(posFb2 != std::string::npos);
  REQUIRE(posRm2 != std::string::npos);

  CHECK(posRm1 < posFmtp);
  CHECK(posFmtp < posFb1);
  CHECK(posFb1 < posFb2);
  CHECK(posFb2 < posRm2);
}

TEST_CASE("Serializer: Format with no rtpmap in m= line", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.formats = {"111", "8"};
  RtpMapAttribute rm;
  rm.payloadType = 111;
  rm.encodingName = "opus";
  rm.clockRate = 48000;
  media.rtpMaps.push_back(rm);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("m=audio 0 RTP/AVP 111 8\r\n") != std::string::npos);
  CHECK(sdp.find("a=rtpmap:111 opus/48000\r\n") != std::string::npos);
  CHECK(sdp.find("a=rtpmap:8") == std::string::npos);
}

TEST_CASE("Serializer: No empty lines in output", "[serializer]")
{
  SessionDescription session;
  session.iceUfrag = "uf";
  session.icePwd = "pw";
  MediaDescription media;
  media.mid = "0";
  media.rtcpMux = true;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("\r\n\r\n") == std::string::npos);
}

TEST_CASE("Serializer: No trailing whitespace on any line", "[serializer]")
{
  SessionDescription session;
  session.iceUfrag = "uf";
  session.icePwd = "pw";
  session.fingerprints.push_back({"sha-256", "AA:BB"});
  MediaDescription media;
  media.mid = "0";
  media.formats = {"111"};
  RtpMapAttribute rm;
  rm.payloadType = 111;
  rm.encodingName = "opus";
  rm.clockRate = 48000;
  media.rtpMaps.push_back(rm);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);

  std::size_t pos = 0;
  while (pos < sdp.size())
  {
    auto crlf = sdp.find("\r\n", pos);
    if (crlf == std::string::npos) break;
    if (crlf > pos)
    {
      CHECK(sdp[crlf - 1] != ' ');
      CHECK(sdp[crlf - 1] != '\t');
    }
    pos = crlf + 2;
  }
}

TEST_CASE("Serializer: SdpSerializerOptions default construction", "[serializer]")
{
  SdpSerializerOptions opts;
  CHECK(opts.lineEnding == "\r\n");
  CHECK(opts.omitSessionName == false);
  CHECK(opts.omitTiming == false);
}

TEST_CASE("Serializer: SdpSerializerOptions equality", "[serializer]")
{
  SdpSerializerOptions a;
  SdpSerializerOptions b;
  CHECK(a == b);
  b.lineEnding = "\n";
  CHECK(a != b);
}

TEST_CASE("Serializer: serializeMediaSection", "[serializer]")
{
  MediaDescription media;
  media.mediaType = MediaType::AUDIO;
  media.port = 9;
  media.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
  media.formats = {"111"};
  media.mid = "0";
  auto sdp = SdpSerializer::serializeMediaSection(media);
  CHECK(sdp.find("m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n") == 0);
  CHECK(sdp.find("a=mid:0\r\n") != std::string::npos);
}

TEST_CASE("Serializer: serializeMediaSection with options (bare LF)", "[serializer]")
{
  SdpSerializerOptions opts;
  opts.lineEnding = "\n";
  MediaDescription media;
  media.port = 9;
  media.formats = {"0"};
  auto sdp = SdpSerializer::serializeMediaSection(media, opts);
  CHECK(sdp.find("\r\n") == std::string::npos);
  CHECK(sdp.find("m=audio 9 RTP/AVP 0\n") == 0);
}

TEST_CASE("Serializer: serializeAttribute with value", "[serializer]")
{
  GenericAttribute attr;
  attr.name = "tool";
  attr.value = "test-tool";
  auto line = SdpSerializer::serializeAttribute(attr);
  CHECK(line == "a=tool:test-tool");
  CHECK(line.find("\r\n") == std::string::npos);
  CHECK(line.find("\n") == std::string::npos);
}

TEST_CASE("Serializer: serializeAttribute flag (no value)", "[serializer]")
{
  GenericAttribute attr;
  attr.name = "rtcp-mux";
  auto line = SdpSerializer::serializeAttribute(attr);
  CHECK(line == "a=rtcp-mux");
}

TEST_CASE("Serializer: Media-level attribute order", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.direction = Direction::SENDRECV;
  media.mid = "0";
  media.msid.push_back({"stream1", std::string("track1")});
  media.iceUfrag = "uf";
  media.icePwd = "pw";
  media.fingerprints.push_back({"sha-256", "AA:BB"});
  media.setup = SetupRole::ACTPASS;
  media.extmapAllowMixed = true;
  media.rtcpMux = true;
  media.rtcpMuxOnly = true;
  media.rtcpRsize = true;
  RtcpAttribute rtcp;
  rtcp.port = 9;
  media.rtcp = rtcp;
  ExtMapAttribute em;
  em.id = 1;
  em.uri = "urn:example";
  media.extMaps.push_back(em);
  media.formats = {"111"};
  RtpMapAttribute rm;
  rm.payloadType = 111;
  rm.encodingName = "opus";
  rm.clockRate = 48000;
  media.rtpMaps.push_back(rm);
  media.ptime = 20;
  media.ssrcs.push_back({1234, "cname", std::string("c1")});
  media.ssrcGroups.push_back({"FID", {1234, 5678}});
  media.crypto.push_back({1, "AES_CM_128_HMAC_SHA1_80", "inline:key", std::nullopt});
  media.sctpPort = 5000;
  media.maxMessageSize = 262144;
  SimulcastAttribute sc;
  sc.sendStreams = {{{"h", false}}};
  media.simulcast = sc;
  RidAttribute rid;
  rid.id = "h";
  rid.direction = RidDirection::SEND;
  media.rids.push_back(rid);
  IceCandidate cand;
  cand.foundation = "1";
  cand.component = 1;
  cand.transport = IceTransportType::UDP;
  cand.priority = 2130706431;
  cand.address = "192.168.1.1";
  cand.port = 50000;
  cand.type = IceCandidateType::HOST;
  media.candidates.push_back(cand);
  media.endOfCandidates = true;
  media.genericAttributes.push_back({"x-custom", std::string("val")});

  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);

  auto posDir = sdp.find("a=sendrecv");
  auto posMid = sdp.find("a=mid:0");
  auto posMsid = sdp.find("a=msid:stream1");
  auto posIceU = sdp.find("a=ice-ufrag:uf");
  auto posIceP = sdp.find("a=ice-pwd:pw");
  auto posFp = sdp.find("a=fingerprint:sha-256");
  auto posSetup = sdp.find("a=setup:actpass");
  auto posEAM = sdp.find("a=extmap-allow-mixed");
  auto posMux = sdp.find("a=rtcp-mux\r\n");
  auto posMuxO = sdp.find("a=rtcp-mux-only");
  auto posRsize = sdp.find("a=rtcp-rsize");
  auto posRtcp = sdp.find("a=rtcp:9");
  auto posExtmap = sdp.find("a=extmap:1");
  auto posRtpmap = sdp.find("a=rtpmap:111");
  auto posPtime = sdp.find("a=ptime:20");
  auto posSsrc = sdp.find("a=ssrc:1234");
  auto posSsrcG = sdp.find("a=ssrc-group:FID");
  auto posCrypto = sdp.find("a=crypto:1");
  auto posSctp = sdp.find("a=sctp-port:5000");
  auto posMms = sdp.find("a=max-message-size:262144");
  auto posSim = sdp.find("a=simulcast:");
  auto posRid = sdp.find("a=rid:h");
  auto posCand = sdp.find("a=candidate:1");
  auto posEoc = sdp.find("a=end-of-candidates");
  auto posGen = sdp.find("a=x-custom:val");

  CHECK(posDir < posMid);
  CHECK(posMid < posMsid);
  CHECK(posMsid < posIceU);
  CHECK(posIceU < posIceP);
  CHECK(posIceP < posFp);
  CHECK(posFp < posSetup);
  CHECK(posSetup < posEAM);
  CHECK(posEAM < posMux);
  CHECK(posMux < posMuxO);
  CHECK(posMuxO < posRsize);
  CHECK(posRsize < posRtcp);
  CHECK(posRtcp < posExtmap);
  CHECK(posExtmap < posRtpmap);
  CHECK(posRtpmap < posPtime);
  CHECK(posPtime < posSsrc);
  CHECK(posSsrc < posSsrcG);
  CHECK(posSsrcG < posCrypto);
  CHECK(posCrypto < posSctp);
  CHECK(posSctp < posMms);
  CHECK(posMms < posSim);
  CHECK(posSim < posRid);
  CHECK(posRid < posCand);
  CHECK(posCand < posEoc);
  CHECK(posEoc < posGen);
}

// ============================================================================
// Round-trip tests
// ============================================================================

TEST_CASE("Serializer: Round-trip minimal session", "[serializer][roundtrip]")
{
  SessionDescription session;
  session.sessionName = "-";
  session.origin = {"-", "0", "0", NetworkType::IN, AddressType::IP4, "0.0.0.0"};
  TimeDescription td;
  td.startTime = 0;
  td.stopTime = 0;
  session.timeDescriptions.push_back(td);

  auto sdp = SdpSerializer::serialize(session);
  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  CHECK(*result.value == session);
}

TEST_CASE("Serializer: Round-trip WebRTC audio+video offer", "[serializer][roundtrip]")
{
  SessionDescription session;
  session.version = 0;
  session.origin = {"-", "4567890123", "2", NetworkType::IN, AddressType::IP4, "127.0.0.1"};
  session.sessionName = "-";
  TimeDescription td;
  session.timeDescriptions.push_back(td);
  session.iceUfrag = "u123";
  session.icePwd = "p456abcdef";
  session.iceOptions = {"trickle"};
  session.fingerprints.push_back({"sha-256", "AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99"});
  session.setup = SetupRole::ACTPASS;
  session.extmapAllowMixed = true;
  session.groups.push_back({"BUNDLE", {"0", "1"}});

  // Audio
  MediaDescription audio;
  audio.mediaType = MediaType::AUDIO;
  audio.port = 9;
  audio.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
  audio.formats = {"111", "0"};
  audio.direction = Direction::SENDRECV;
  audio.mid = "0";
  audio.msid.push_back({"stream1", std::string("audiotrack1")});
  audio.rtcpMux = true;

  RtpMapAttribute rmOpus;
  rmOpus.payloadType = 111;
  rmOpus.encodingName = "opus";
  rmOpus.clockRate = 48000;
  rmOpus.channels = 2;
  audio.rtpMaps.push_back(rmOpus);

  FmtpAttribute fmtpOpus;
  fmtpOpus.payloadType = 111;
  fmtpOpus.parameters = "minptime=10;useinbandfec=1";
  fmtpOpus.parameterMap["minptime"] = "10";
  fmtpOpus.parameterMap["useinbandfec"] = "1";
  audio.fmtps.push_back(fmtpOpus);

  audio.rtcpFeedbacks.push_back({"111", "transport-cc", std::nullopt});

  RtpMapAttribute rmPcmu;
  rmPcmu.payloadType = 0;
  rmPcmu.encodingName = "PCMU";
  rmPcmu.clockRate = 8000;
  audio.rtpMaps.push_back(rmPcmu);

  ExtMapAttribute emAudio;
  emAudio.id = 1;
  emAudio.uri = "urn:ietf:params:rtp-hdrext:ssrc-audio-level";
  audio.extMaps.push_back(emAudio);

  audio.ssrcs.push_back({1234567890, "cname", std::string("user@example")});

  session.mediaDescriptions.push_back(audio);

  // Video
  MediaDescription video;
  video.mediaType = MediaType::VIDEO;
  video.port = 9;
  video.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
  video.formats = {"96"};
  video.direction = Direction::SENDRECV;
  video.mid = "1";
  video.msid.push_back({"stream1", std::string("videotrack1")});
  video.rtcpMux = true;
  video.rtcpRsize = true;

  RtpMapAttribute rmVp8;
  rmVp8.payloadType = 96;
  rmVp8.encodingName = "VP8";
  rmVp8.clockRate = 90000;
  video.rtpMaps.push_back(rmVp8);

  video.rtcpFeedbacks.push_back({"96", "nack", std::nullopt});
  video.rtcpFeedbacks.push_back({"96", "nack", std::string("pli")});
  video.rtcpFeedbacks.push_back({"96", "ccm", std::string("fir")});

  ExtMapAttribute emVideo;
  emVideo.id = 2;
  emVideo.uri = "urn:ietf:params:rtp-hdrext:toffset";
  video.extMaps.push_back(emVideo);

  video.ssrcs.push_back({987654321, "cname", std::string("user@example")});

  session.mediaDescriptions.push_back(video);

  auto sdp = SdpSerializer::serialize(session);
  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  CHECK(*result.value == session);
}

TEST_CASE("Serializer: Round-trip WebRTC offer audio+video+data channel", "[serializer][roundtrip]")
{
  SessionDescription session;
  session.version = 0;
  session.origin = {"-", "1234567890", "1", NetworkType::IN, AddressType::IP4, "127.0.0.1"};
  session.sessionName = "-";
  TimeDescription td;
  session.timeDescriptions.push_back(td);
  session.iceUfrag = "abcd";
  session.icePwd = "efghijklmnop";
  session.iceOptions = {"trickle"};
  session.fingerprints.push_back({"sha-256", "AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99"});
  session.setup = SetupRole::ACTPASS;
  session.extmapAllowMixed = true;
  session.groups.push_back({"BUNDLE", {"0", "1", "2"}});

  // Audio
  MediaDescription audio;
  audio.mediaType = MediaType::AUDIO;
  audio.port = 9;
  audio.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
  audio.formats = {"111"};
  audio.direction = Direction::SENDRECV;
  audio.mid = "0";
  audio.rtcpMux = true;
  RtpMapAttribute rmOpus;
  rmOpus.payloadType = 111;
  rmOpus.encodingName = "opus";
  rmOpus.clockRate = 48000;
  rmOpus.channels = 2;
  audio.rtpMaps.push_back(rmOpus);
  FmtpAttribute fmtpOpus;
  fmtpOpus.payloadType = 111;
  fmtpOpus.parameters = "minptime=10;useinbandfec=1";
  fmtpOpus.parameterMap["minptime"] = "10";
  fmtpOpus.parameterMap["useinbandfec"] = "1";
  audio.fmtps.push_back(fmtpOpus);
  session.mediaDescriptions.push_back(audio);

  // Video
  MediaDescription video;
  video.mediaType = MediaType::VIDEO;
  video.port = 9;
  video.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
  video.formats = {"96"};
  video.direction = Direction::SENDRECV;
  video.mid = "1";
  video.rtcpMux = true;
  video.rtcpRsize = true;
  RtpMapAttribute rmVp8;
  rmVp8.payloadType = 96;
  rmVp8.encodingName = "VP8";
  rmVp8.clockRate = 90000;
  video.rtpMaps.push_back(rmVp8);
  video.rtcpFeedbacks.push_back({"96", "nack", std::string("pli")});
  session.mediaDescriptions.push_back(video);

  // Data channel
  MediaDescription data;
  data.mediaType = MediaType::APPLICATION;
  data.port = 9;
  data.protocol = TransportProtocol::UDP_DTLS_SCTP;
  data.formats = {"webrtc-datachannel"};
  data.direction = Direction::SENDRECV;
  data.mid = "2";
  data.sctpPort = 5000;
  data.maxMessageSize = 262144;
  session.mediaDescriptions.push_back(data);

  auto sdp = SdpSerializer::serialize(session);
  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(result.value->mediaDescriptions.size() == 3);
  CHECK(*result.value == session);
}

TEST_CASE("Serializer: Round-trip data channel", "[serializer][roundtrip]")
{
  SessionDescription session;
  session.origin = {"-", "1", "1", NetworkType::IN, AddressType::IP4, "0.0.0.0"};
  session.sessionName = "-";
  TimeDescription td;
  session.timeDescriptions.push_back(td);
  session.groups.push_back({"BUNDLE", {"2"}});
  session.fingerprints.push_back({"sha-256", "AA:BB:CC:DD"});
  session.setup = SetupRole::ACTPASS;
  session.iceUfrag = "dc-uf";
  session.icePwd = "dc-pwd";

  MediaDescription data;
  data.mediaType = MediaType::APPLICATION;
  data.port = 9;
  data.protocol = TransportProtocol::UDP_DTLS_SCTP;
  data.formats = {"webrtc-datachannel"};
  data.direction = Direction::SENDRECV;
  data.mid = "2";
  data.sctpPort = 5000;
  data.maxMessageSize = 262144;
  data.rtcpMux = true;
  session.mediaDescriptions.push_back(data);

  auto sdp = SdpSerializer::serialize(session);
  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  CHECK(*result.value == session);
}

TEST_CASE("Serializer: Round-trip SIP SDP with crypto", "[serializer][roundtrip]")
{
  SessionDescription session;
  session.origin = {"alice", "2890844526", "2890844526", NetworkType::IN, AddressType::IP4, "192.168.1.1"};
  session.sessionName = "SIP Call";
  TimeDescription td;
  session.timeDescriptions.push_back(td);

  ConnectionData conn;
  conn.address = "192.168.1.1";
  session.connection = conn;

  MediaDescription audio;
  audio.mediaType = MediaType::AUDIO;
  audio.port = 49170;
  audio.protocol = TransportProtocol::RTP_SAVP;
  audio.formats = {"0", "8"};
  audio.direction = Direction::SENDRECV;

  RtpMapAttribute rmPcmu;
  rmPcmu.payloadType = 0;
  rmPcmu.encodingName = "PCMU";
  rmPcmu.clockRate = 8000;
  audio.rtpMaps.push_back(rmPcmu);

  RtpMapAttribute rmPcma;
  rmPcma.payloadType = 8;
  rmPcma.encodingName = "PCMA";
  rmPcma.clockRate = 8000;
  audio.rtpMaps.push_back(rmPcma);

  CryptoAttribute crypto;
  crypto.tag = 1;
  crypto.suite = "AES_CM_128_HMAC_SHA1_80";
  crypto.keyParams = "inline:base64encodedkey";
  audio.crypto.push_back(crypto);

  audio.ptime = 20;
  session.mediaDescriptions.push_back(audio);

  auto sdp = SdpSerializer::serialize(session);
  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  CHECK(*result.value == session);
}

TEST_CASE("Serializer: Numeric fields without leading zeros", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.port = 9;
  media.formats = {"0"};
  RtpMapAttribute rm;
  rm.payloadType = 0;
  rm.encodingName = "PCMU";
  rm.clockRate = 8000;
  media.rtpMaps.push_back(rm);
  media.ssrcs.push_back({100, "cname", std::string("c")});
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=rtpmap:0 PCMU/8000") != std::string::npos);
  CHECK(sdp.find("a=ssrc:100 cname:c") != std::string::npos);
}

TEST_CASE("Serializer: Wildcard rtcp-fb after per-PT rtcp-fb", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.formats = {"111"};
  RtpMapAttribute rm;
  rm.payloadType = 111;
  rm.encodingName = "opus";
  rm.clockRate = 48000;
  media.rtpMaps.push_back(rm);
  media.rtcpFeedbacks.push_back({"111", "nack", std::nullopt});
  media.rtcpFeedbacks.push_back({"*", "transport-cc", std::nullopt});
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);

  auto posPerPT = sdp.find("a=rtcp-fb:111 nack");
  auto posWild = sdp.find("a=rtcp-fb:* transport-cc");
  REQUIRE(posPerPT != std::string::npos);
  REQUIRE(posWild != std::string::npos);
  CHECK(posPerPT < posWild);
}

TEST_CASE("Serializer: Rid with pt= and restrictions", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  RidAttribute rid;
  rid.id = "h";
  rid.direction = RidDirection::SEND;
  rid.payloadTypes = {96, 97};
  rid.restrictions["max-width"] = "1280";
  rid.restrictions["max-fps"] = "30";
  media.rids.push_back(rid);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=rid:h send pt=96,97;max-fps=30;max-width=1280\r\n") != std::string::npos);
}

TEST_CASE("Serializer: SdpSerializer is non-instantiable", "[serializer]")
{
  CHECK(std::is_constructible<SdpSerializer>::value == false);
}

TEST_CASE("Serializer: Media-level iceOptions with multiple values", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.iceOptions = {"trickle", "ice2"};
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=ice-options:trickle ice2\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Empty media-level iceOptions omits line", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  // Should only have session-level content and media content — no ice-options at media level
  auto mPos = sdp.find("m=");
  REQUIRE(mPos != std::string::npos);
  auto mediaSdp = sdp.substr(mPos);
  CHECK(mediaSdp.find("a=ice-options") == std::string::npos);
}

TEST_CASE("Serializer: Round-trip with all session-level optional fields", "[serializer][roundtrip]")
{
  SessionDescription session;
  session.origin = {"user", "123", "456", NetworkType::IN, AddressType::IP6, "::1"};
  session.sessionName = "Full Session";
  session.sessionInfo = "Detailed information";
  session.uri = "http://example.com";
  session.emailAddress = "admin@example.com";
  session.phoneNumber = "+1-800-555-0123";
  ConnectionData conn;
  conn.netType = NetworkType::IN;
  conn.addrType = AddressType::IP4;
  conn.address = "192.168.1.1";
  session.connection = conn;
  session.bandwidths = {{BandwidthType::AS, 2000}};
  TimeDescription td;
  td.startTime = 100;
  td.stopTime = 200;
  session.timeDescriptions.push_back(td);
  session.encryptionKey = "prompt";

  auto sdp = SdpSerializer::serialize(session);
  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  CHECK(*result.value == session);
}

TEST_CASE("Serializer: Round-trip connection with TTL and numberOfAddresses", "[serializer][roundtrip]")
{
  SessionDescription session;
  session.sessionName = "-";
  session.origin = {"-", "0", "0", NetworkType::IN, AddressType::IP4, "0.0.0.0"};
  ConnectionData conn;
  conn.address = "224.2.36.42";
  conn.ttl = 127;
  conn.numberOfAddresses = 3;
  session.connection = conn;
  TimeDescription td;
  session.timeDescriptions.push_back(td);

  auto sdp = SdpSerializer::serialize(session);
  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  CHECK(result.value->connection.has_value());
  CHECK(result.value->connection->address == "224.2.36.42");
  CHECK(result.value->connection->ttl == 127);
  CHECK(result.value->connection->numberOfAddresses == 3);
}

TEST_CASE("Serializer: Round-trip TimeDescription with RepeatTime", "[serializer][roundtrip]")
{
  SessionDescription session;
  session.sessionName = "-";
  session.origin = {"-", "0", "0", NetworkType::IN, AddressType::IP4, "0.0.0.0"};
  TimeDescription td;
  td.startTime = 3034423619;
  td.stopTime = 3034423619;
  RepeatTime rt;
  rt.repeatInterval = "604800";
  rt.activeDuration = "3600";
  rt.offsets = {"0", "90000"};
  td.repeatTimes.push_back(rt);
  session.timeDescriptions.push_back(td);

  auto sdp = SdpSerializer::serialize(session);
  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  CHECK(*result.value == session);
}

TEST_CASE("Serializer: Round-trip zone adjustments", "[serializer][roundtrip]")
{
  SessionDescription session;
  session.sessionName = "-";
  session.origin = {"-", "0", "0", NetworkType::IN, AddressType::IP4, "0.0.0.0"};
  TimeDescription td;
  session.timeDescriptions.push_back(td);
  session.zoneAdjustments = {{"2882844526", "-1h"}, {"2898848070", "0"}};

  auto sdp = SdpSerializer::serialize(session);
  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  CHECK(*result.value == session);
}

TEST_CASE("Serializer: Round-trip ICE candidate with all fields", "[serializer][roundtrip]")
{
  SessionDescription session;
  session.sessionName = "-";
  session.origin = {"-", "0", "0", NetworkType::IN, AddressType::IP4, "0.0.0.0"};
  TimeDescription td;
  session.timeDescriptions.push_back(td);

  MediaDescription media;
  media.port = 9;
  media.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
  media.formats = {"111"};
  media.direction = Direction::SENDRECV;

  IceCandidate cand;
  cand.foundation = "1";
  cand.component = 1;
  cand.transport = IceTransportType::TCP;
  cand.priority = 2105524223;
  cand.address = "192.168.1.1";
  cand.port = 9;
  cand.type = IceCandidateType::HOST;
  cand.tcpType = "passive";
  cand.extensions = {{"generation", "0"}};
  media.candidates.push_back(cand);

  session.mediaDescriptions.push_back(media);

  auto sdp = SdpSerializer::serialize(session);
  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(!result.value->mediaDescriptions.empty());
  REQUIRE(!result.value->mediaDescriptions[0].candidates.empty());
  auto& parsed = result.value->mediaDescriptions[0].candidates[0];
  CHECK(parsed.foundation == "1");
  CHECK(parsed.component == 1);
  CHECK(parsed.transport == IceTransportType::TCP);
  CHECK(parsed.priority == 2105524223);
  CHECK(parsed.address == "192.168.1.1");
  CHECK(parsed.port == 9);
  CHECK(parsed.type == IceCandidateType::HOST);
  CHECK(parsed.tcpType == "passive");
  CHECK(parsed.extensions.size() == 1);
  CHECK(parsed.extensions[0].first == "generation");
  CHECK(parsed.extensions[0].second == "0");
}

TEST_CASE("Serializer: Round-trip simulcast", "[serializer][roundtrip]")
{
  SessionDescription session;
  session.sessionName = "-";
  session.origin = {"-", "0", "0", NetworkType::IN, AddressType::IP4, "0.0.0.0"};
  TimeDescription td;
  session.timeDescriptions.push_back(td);

  MediaDescription media;
  media.port = 9;
  media.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
  media.formats = {"96"};
  media.direction = Direction::SENDRECV;

  SimulcastAttribute sc;
  sc.sendStreams = {{{"h", false}, {"m", false}}, {{"l", false}}};
  sc.recvStreams = {{{"h", false}}};
  media.simulcast = sc;

  RidAttribute ridH;
  ridH.id = "h";
  ridH.direction = RidDirection::SEND;
  media.rids.push_back(ridH);

  RidAttribute ridM;
  ridM.id = "m";
  ridM.direction = RidDirection::SEND;
  media.rids.push_back(ridM);

  RidAttribute ridL;
  ridL.id = "l";
  ridL.direction = RidDirection::SEND;
  media.rids.push_back(ridL);

  session.mediaDescriptions.push_back(media);

  auto sdp = SdpSerializer::serialize(session);
  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  REQUIRE(!result.value->mediaDescriptions.empty());
  auto& parsedMedia = result.value->mediaDescriptions[0];
  REQUIRE(parsedMedia.simulcast.has_value());
  CHECK(parsedMedia.simulcast->sendStreams.size() == 2);
  CHECK(parsedMedia.simulcast->recvStreams.size() == 1);
  CHECK(parsedMedia.rids.size() == 3);
}

TEST_CASE("Serializer: Fingerprint and setup serialized", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.fingerprints.push_back({"sha-256", "AA:BB:CC"});
  media.setup = SetupRole::ACTIVE;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("a=fingerprint:sha-256 AA:BB:CC\r\n") != std::string::npos);
  CHECK(sdp.find("a=setup:active\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Media-level connection", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  ConnectionData conn;
  conn.address = "10.0.0.1";
  media.connection = conn;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  auto mPos = sdp.find("m=");
  auto mediaSdp = sdp.substr(mPos);
  CHECK(mediaSdp.find("c=IN IP4 10.0.0.1\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Media-level bandwidth", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.bandwidths = {{BandwidthType::TIAS, 500000}};
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  auto mPos = sdp.find("m=");
  auto mediaSdp = sdp.substr(mPos);
  CHECK(mediaSdp.find("b=TIAS:500000\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Media-level encryption key", "[serializer]")
{
  SessionDescription session;
  MediaDescription media;
  media.encryptionKey = "base64:mediakey";
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session);
  auto mPos = sdp.find("m=");
  auto mediaSdp = sdp.substr(mPos);
  CHECK(mediaSdp.find("k=base64:mediakey\r\n") != std::string::npos);
}

TEST_CASE("Serializer: Multiple media sections", "[serializer]")
{
  SessionDescription session;
  MediaDescription audio;
  audio.mediaType = MediaType::AUDIO;
  audio.port = 9;
  audio.formats = {"111"};
  session.mediaDescriptions.push_back(audio);

  MediaDescription video;
  video.mediaType = MediaType::VIDEO;
  video.port = 9;
  video.formats = {"96"};
  session.mediaDescriptions.push_back(video);

  auto sdp = SdpSerializer::serialize(session);
  auto posAudio = sdp.find("m=audio");
  auto posVideo = sdp.find("m=video");
  REQUIRE(posAudio != std::string::npos);
  REQUIRE(posVideo != std::string::npos);
  CHECK(posAudio < posVideo);
}

// ============================================================================
// Edge case tests
// ============================================================================

TEST_CASE("Serializer: Multiple TimeDescriptions", "[serializer][edge]")
{
  SessionDescription session;
  TimeDescription td1;
  td1.startTime = 100;
  td1.stopTime = 200;
  TimeDescription td2;
  td2.startTime = 300;
  td2.stopTime = 400;
  session.timeDescriptions.push_back(td1);
  session.timeDescriptions.push_back(td2);
  auto sdp = SdpSerializer::serialize(session);
  auto pos1 = sdp.find("t=100 200\r\n");
  auto pos2 = sdp.find("t=300 400\r\n");
  REQUIRE(pos1 != std::string::npos);
  REQUIRE(pos2 != std::string::npos);
  CHECK(pos1 < pos2);
}

TEST_CASE("Serializer: No empty lines with bare LF", "[serializer][edge]")
{
  SdpSerializerOptions opts;
  opts.lineEnding = "\n";
  SessionDescription session;
  session.iceUfrag = "uf";
  session.icePwd = "pw";
  MediaDescription media;
  media.mid = "0";
  media.rtcpMux = true;
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session, opts);
  CHECK(sdp.find("\n\n") == std::string::npos);
}

TEST_CASE("Serializer: No trailing whitespace with bare LF", "[serializer][edge]")
{
  SdpSerializerOptions opts;
  opts.lineEnding = "\n";
  SessionDescription session;
  session.iceUfrag = "uf";
  session.icePwd = "pw";
  session.fingerprints.push_back({"sha-256", "AA:BB"});
  MediaDescription media;
  media.mid = "0";
  media.formats = {"111"};
  RtpMapAttribute rm;
  rm.payloadType = 111;
  rm.encodingName = "opus";
  rm.clockRate = 48000;
  media.rtpMaps.push_back(rm);
  session.mediaDescriptions.push_back(media);
  auto sdp = SdpSerializer::serialize(session, opts);

  std::size_t pos = 0;
  while (pos < sdp.size())
  {
    auto lf = sdp.find('\n', pos);
    if (lf == std::string::npos) break;
    if (lf > pos)
    {
      CHECK(sdp[lf - 1] != ' ');
      CHECK(sdp[lf - 1] != '\t');
    }
    pos = lf + 1;
  }
}

TEST_CASE("Serializer: Connection numberOfAddresses omitted when ttl absent", "[serializer][edge]")
{
  SessionDescription session;
  ConnectionData conn;
  conn.address = "224.2.36.42";
  conn.numberOfAddresses = 3;
  // ttl is NOT set
  session.connection = conn;
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("c=IN IP4 224.2.36.42\r\n") != std::string::npos);
  CHECK(sdp.find("/3") == std::string::npos);
}

TEST_CASE("Serializer: RepeatTime with empty offsets", "[serializer][edge]")
{
  SessionDescription session;
  TimeDescription td;
  td.startTime = 0;
  td.stopTime = 0;
  RepeatTime rt;
  rt.repeatInterval = "3600";
  rt.activeDuration = "1800";
  // offsets is empty
  td.repeatTimes.push_back(rt);
  session.timeDescriptions.push_back(td);
  auto sdp = SdpSerializer::serialize(session);
  CHECK(sdp.find("r=3600 1800\r\n") != std::string::npos);
}

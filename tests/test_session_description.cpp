#include <catch2/catch.hpp>
#include "iora/sdp/session_description.hpp"

using namespace iora::sdp;

// ---------------------------------------------------------------------------
// Origin
// ---------------------------------------------------------------------------

TEST_CASE("Origin: default construction", "[session][Origin]")
{
  Origin o;
  REQUIRE(o.username == "-");
  REQUIRE(o.sessId == "0");
  REQUIRE(o.sessVersion == "0");
  REQUIRE(o.netType == NetworkType::IN);
  REQUIRE(o.addrType == AddressType::IP4);
  REQUIRE(o.address == "0.0.0.0");
}

TEST_CASE("Origin: equality", "[session][Origin]")
{
  Origin a;
  Origin b;
  REQUIRE(a == b);

  Origin c;
  c.sessId = "12345";
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// ConnectionData
// ---------------------------------------------------------------------------

TEST_CASE("ConnectionData: default construction", "[session][ConnectionData]")
{
  ConnectionData c;
  REQUIRE(c.netType == NetworkType::IN);
  REQUIRE(c.addrType == AddressType::IP4);
  REQUIRE(c.address.empty());
  REQUIRE(!c.ttl.has_value());
  REQUIRE(!c.numberOfAddresses.has_value());
}

TEST_CASE("ConnectionData: with multicast fields", "[session][ConnectionData]")
{
  ConnectionData c;
  c.address = "224.2.36.42";
  c.ttl = 127;
  c.numberOfAddresses = 3;

  REQUIRE(c.ttl == 127);
  REQUIRE(c.numberOfAddresses == 3);
}

TEST_CASE("ConnectionData: without multicast fields", "[session][ConnectionData]")
{
  ConnectionData c;
  c.addrType = AddressType::IP6;
  c.address = "::1";

  REQUIRE(!c.ttl.has_value());
  REQUIRE(!c.numberOfAddresses.has_value());
}

TEST_CASE("ConnectionData: equality", "[session][ConnectionData]")
{
  ConnectionData a;
  a.address = "192.168.1.1";

  ConnectionData b = a;
  REQUIRE(a == b);

  ConnectionData c = a;
  c.address = "10.0.0.1";
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// BandwidthInfo
// ---------------------------------------------------------------------------

TEST_CASE("BandwidthInfo: default construction", "[session][BandwidthInfo]")
{
  BandwidthInfo b;
  REQUIRE(b.type == BandwidthType::AS);
  REQUIRE(b.bandwidth == 0);
}

TEST_CASE("BandwidthInfo: equality", "[session][BandwidthInfo]")
{
  BandwidthInfo a{BandwidthType::TIAS, 1000000};
  BandwidthInfo b{BandwidthType::TIAS, 1000000};
  BandwidthInfo c{BandwidthType::AS, 1000};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// RepeatTime
// ---------------------------------------------------------------------------

TEST_CASE("RepeatTime: default construction", "[session][RepeatTime]")
{
  RepeatTime r;
  REQUIRE(r.repeatInterval.empty());
  REQUIRE(r.activeDuration.empty());
  REQUIRE(r.offsets.empty());
}

TEST_CASE("RepeatTime: with typed time format", "[session][RepeatTime]")
{
  RepeatTime r{"7d", "1h", {"0", "25h"}};
  REQUIRE(r.repeatInterval == "7d");
  REQUIRE(r.activeDuration == "1h");
  REQUIRE(r.offsets.size() == 2);
}

TEST_CASE("RepeatTime: equality", "[session][RepeatTime]")
{
  RepeatTime a{"604800", "3600", {"0", "90000"}};
  RepeatTime b{"604800", "3600", {"0", "90000"}};
  RepeatTime c{"604800", "3600", {"0"}};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// ZoneAdjustment
// ---------------------------------------------------------------------------

TEST_CASE("ZoneAdjustment: default construction", "[session][ZoneAdjustment]")
{
  ZoneAdjustment z;
  REQUIRE(z.adjustmentTime.empty());
  REQUIRE(z.offset.empty());
}

TEST_CASE("ZoneAdjustment: equality", "[session][ZoneAdjustment]")
{
  ZoneAdjustment a{"2882844526", "-1h"};
  ZoneAdjustment b{"2882844526", "-1h"};
  ZoneAdjustment c{"2882844526", "+1h"};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// TimeDescription
// ---------------------------------------------------------------------------

TEST_CASE("TimeDescription: default construction", "[session][TimeDescription]")
{
  TimeDescription t;
  REQUIRE(t.startTime == 0);
  REQUIRE(t.stopTime == 0);
  REQUIRE(t.repeatTimes.empty());
}

TEST_CASE("TimeDescription: with repeat times", "[session][TimeDescription]")
{
  TimeDescription t;
  t.startTime = 3034423619;
  t.stopTime = 3034430819;
  t.repeatTimes.push_back({"604800", "3600", {"0", "90000"}});

  REQUIRE(t.repeatTimes.size() == 1);
  REQUIRE(t.repeatTimes[0].repeatInterval == "604800");
}

TEST_CASE("TimeDescription: without repeat times", "[session][TimeDescription]")
{
  TimeDescription t{0, 0, {}};
  REQUIRE(t.startTime == 0);
  REQUIRE(t.stopTime == 0);
  REQUIRE(t.repeatTimes.empty());
}

TEST_CASE("TimeDescription: equality", "[session][TimeDescription]")
{
  TimeDescription a{0, 0, {}};
  TimeDescription b{0, 0, {}};
  REQUIRE(a == b);

  TimeDescription c{100, 0, {}};
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// MediaDescription
// ---------------------------------------------------------------------------

TEST_CASE("MediaDescription: default construction", "[session][MediaDescription]")
{
  MediaDescription md;
  REQUIRE(md.mediaType == MediaType::AUDIO);
  REQUIRE(md.port == 0);
  REQUIRE(!md.numberOfPorts.has_value());
  REQUIRE(md.protocol == TransportProtocol::RTP_AVP);
  REQUIRE(md.formats.empty());
  REQUIRE(!md.connection.has_value());
  REQUIRE(md.bandwidths.empty());
  REQUIRE(!md.encryptionKey.has_value());
  REQUIRE(!md.mid.has_value());
  REQUIRE(md.direction == Direction::SENDRECV);
  REQUIRE(md.rtpMaps.empty());
  REQUIRE(md.fmtps.empty());
  REQUIRE(md.rtcpFeedbacks.empty());
  REQUIRE(md.extMaps.empty());
  REQUIRE(md.ssrcs.empty());
  REQUIRE(md.ssrcGroups.empty());
  REQUIRE(md.candidates.empty());
  REQUIRE(md.fingerprints.empty());
  REQUIRE(!md.setup.has_value());
  REQUIRE(!md.iceUfrag.has_value());
  REQUIRE(!md.icePwd.has_value());
  REQUIRE(md.iceOptions.empty());
  REQUIRE(md.rtcpMux == false);
  REQUIRE(md.rtcpMuxOnly == false);
  REQUIRE(md.rtcpRsize == false);
  REQUIRE(!md.rtcp.has_value());
  REQUIRE(md.msid.empty());
  REQUIRE(md.crypto.empty());
  REQUIRE(!md.sctpPort.has_value());
  REQUIRE(!md.maxMessageSize.has_value());
  REQUIRE(!md.simulcast.has_value());
  REQUIRE(md.rids.empty());
  REQUIRE(!md.ptime.has_value());
  REQUIRE(!md.maxptime.has_value());
  REQUIRE(!md.framerate.has_value());
  REQUIRE(md.endOfCandidates == false);
  REQUIRE(md.extmapAllowMixed == false);
  REQUIRE(md.genericAttributes.empty());
}

TEST_CASE("MediaDescription: add rtpMaps and fmtps", "[session][MediaDescription]")
{
  MediaDescription md;
  md.mediaType = MediaType::AUDIO;
  md.port = 9;
  md.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
  md.formats = {"111", "0"};

  md.rtpMaps.push_back({111, "opus", 48000, 2});
  md.rtpMaps.push_back({0, "PCMU", 8000, 1});

  FmtpAttribute fmtp;
  fmtp.payloadType = 111;
  fmtp.parameters = "minptime=10;useinbandfec=1";
  md.fmtps.push_back(fmtp);

  REQUIRE(md.rtpMaps.size() == 2);
  REQUIRE(md.rtpMaps[0].encodingName == "opus");
  REQUIRE(md.fmtps.size() == 1);
}

TEST_CASE("MediaDescription: add candidates", "[session][MediaDescription]")
{
  MediaDescription md;

  IceCandidate cand;
  cand.foundation = "1";
  cand.component = 1;
  cand.transport = IceTransportType::UDP;
  cand.priority = 2130706431;
  cand.address = "192.168.1.1";
  cand.port = 5000;
  cand.type = IceCandidateType::HOST;
  md.candidates.push_back(cand);

  IceCandidate relay;
  relay.foundation = "2";
  relay.component = 1;
  relay.transport = IceTransportType::UDP;
  relay.priority = 16777215;
  relay.address = "203.0.113.1";
  relay.port = 6000;
  relay.type = IceCandidateType::RELAY;
  relay.relAddr = "192.168.1.1";
  relay.relPort = 5000;
  md.candidates.push_back(relay);

  REQUIRE(md.candidates.size() == 2);
  REQUIRE(md.candidates[1].type == IceCandidateType::RELAY);
  REQUIRE(md.candidates[1].relAddr == "192.168.1.1");
}

TEST_CASE("MediaDescription: all fields accessible", "[session][MediaDescription]")
{
  MediaDescription md;
  md.mid = "0";
  md.direction = Direction::SENDONLY;
  md.rtcpMux = true;
  md.rtcpMuxOnly = true;
  md.rtcpRsize = true;
  md.endOfCandidates = true;
  md.extmapAllowMixed = true;
  md.iceUfrag = "user";
  md.icePwd = "pass";
  md.iceOptions = {"trickle", "ice2"};
  md.setup = SetupRole::ACTPASS;
  md.ptime = 20;
  md.maxptime = 60;
  md.framerate = 30.0;
  md.sctpPort = 5000;
  md.maxMessageSize = 262144;
  md.fingerprints.push_back({"sha-256", "AA:BB:CC"});
  md.msid.push_back({"stream1", "track1"});
  md.crypto.push_back({1, "AES_CM_128_HMAC_SHA1_80", "inline:key", std::nullopt});
  md.genericAttributes.push_back({"x-custom", "value"});

  REQUIRE(md.mid == "0");
  REQUIRE(md.direction == Direction::SENDONLY);
  REQUIRE(md.rtcpMux == true);
  REQUIRE(md.rtcpMuxOnly == true);
  REQUIRE(md.iceOptions.size() == 2);
  REQUIRE(md.setup == SetupRole::ACTPASS);
  REQUIRE(md.ptime == 20);
  REQUIRE(md.framerate == 30.0);
  REQUIRE(md.sctpPort == 5000);
  REQUIRE(md.fingerprints.size() == 1);
  REQUIRE(md.msid.size() == 1);
  REQUIRE(md.crypto.size() == 1);
  REQUIRE(md.genericAttributes.size() == 1);
}

TEST_CASE("MediaDescription: equality", "[session][MediaDescription]")
{
  MediaDescription a;
  a.mediaType = MediaType::VIDEO;
  a.port = 9;
  a.rtcpMux = true;

  MediaDescription b = a;
  REQUIRE(a == b);

  MediaDescription c = a;
  c.rtcpMux = false;
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// SessionDescription
// ---------------------------------------------------------------------------

TEST_CASE("SessionDescription: default construction", "[session][SessionDescription]")
{
  SessionDescription sd;
  REQUIRE(sd.version == 0);
  REQUIRE(sd.sessionName == "-");
  REQUIRE(!sd.sessionInfo.has_value());
  REQUIRE(!sd.uri.has_value());
  REQUIRE(!sd.emailAddress.has_value());
  REQUIRE(!sd.phoneNumber.has_value());
  REQUIRE(!sd.connection.has_value());
  REQUIRE(sd.bandwidths.empty());
  REQUIRE(sd.timeDescriptions.empty());
  REQUIRE(sd.zoneAdjustments.empty());
  REQUIRE(!sd.encryptionKey.has_value());
  REQUIRE(sd.attributes.empty());
  REQUIRE(sd.mediaDescriptions.empty());
  REQUIRE(sd.groups.empty());
  REQUIRE(!sd.iceUfrag.has_value());
  REQUIRE(!sd.icePwd.has_value());
  REQUIRE(sd.iceOptions.empty());
  REQUIRE(sd.iceLite == false);
  REQUIRE(sd.fingerprints.empty());
  REQUIRE(!sd.setup.has_value());
  REQUIRE(sd.extmapAllowMixed == false);
}

TEST_CASE("SessionDescription: add multiple media descriptions", "[session][SessionDescription]")
{
  SessionDescription sd;
  sd.origin.sessId = "1234567890";
  sd.origin.sessVersion = "1";

  MediaDescription audio;
  audio.mediaType = MediaType::AUDIO;
  audio.port = 9;
  audio.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
  audio.mid = "0";
  audio.rtcpMux = true;
  sd.mediaDescriptions.push_back(audio);

  MediaDescription video;
  video.mediaType = MediaType::VIDEO;
  video.port = 9;
  video.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
  video.mid = "1";
  video.rtcpMux = true;
  sd.mediaDescriptions.push_back(video);

  MediaDescription data;
  data.mediaType = MediaType::APPLICATION;
  data.port = 9;
  data.protocol = TransportProtocol::UDP_DTLS_SCTP;
  data.mid = "2";
  data.sctpPort = 5000;
  data.maxMessageSize = 262144;
  sd.mediaDescriptions.push_back(data);

  sd.groups.push_back({"BUNDLE", {"0", "1", "2"}});

  REQUIRE(sd.mediaDescriptions.size() == 3);
  REQUIRE(sd.groups.size() == 1);
  REQUIRE(sd.groups[0].mids.size() == 3);
}

TEST_CASE("SessionDescription: session-level ICE", "[session][SessionDescription]")
{
  SessionDescription sd;
  sd.iceUfrag = "abc123";
  sd.icePwd = "secretpassword";
  sd.iceOptions = {"trickle"};
  sd.iceLite = true;

  REQUIRE(sd.iceUfrag == "abc123");
  REQUIRE(sd.icePwd == "secretpassword");
  REQUIRE(sd.iceOptions.size() == 1);
  REQUIRE(sd.iceOptions[0] == "trickle");
  REQUIRE(sd.iceLite == true);
}

TEST_CASE("SessionDescription: session-level fingerprints and setup", "[session][SessionDescription]")
{
  SessionDescription sd;
  sd.fingerprints.push_back({"sha-256", "AA:BB:CC:DD"});
  sd.fingerprints.push_back({"sha-384", "11:22:33:44"});
  sd.setup = SetupRole::ACTPASS;
  sd.extmapAllowMixed = true;

  REQUIRE(sd.fingerprints.size() == 2);
  REQUIRE(sd.setup == SetupRole::ACTPASS);
  REQUIRE(sd.extmapAllowMixed == true);
}

TEST_CASE("SessionDescription: equality — identical", "[session][SessionDescription]")
{
  SessionDescription a;
  a.origin.sessId = "100";
  a.sessionName = "Test";
  a.iceUfrag = "user";
  a.icePwd = "pass";

  MediaDescription md;
  md.mediaType = MediaType::AUDIO;
  md.port = 9;
  md.rtcpMux = true;
  a.mediaDescriptions.push_back(md);

  SessionDescription b = a;
  REQUIRE(a == b);
}

TEST_CASE("SessionDescription: inequality — any field difference", "[session][SessionDescription]")
{
  SessionDescription base;
  base.origin.sessId = "100";
  base.sessionName = "Test";

  SessionDescription diffVersion = base;
  diffVersion.version = 1;
  REQUIRE(base != diffVersion);

  SessionDescription diffName = base;
  diffName.sessionName = "Other";
  REQUIRE(base != diffName);

  SessionDescription diffIceLite = base;
  diffIceLite.iceLite = true;
  REQUIRE(base != diffIceLite);

  SessionDescription diffExtmap = base;
  diffExtmap.extmapAllowMixed = true;
  REQUIRE(base != diffExtmap);

  SessionDescription diffMedia = base;
  diffMedia.mediaDescriptions.push_back(MediaDescription{});
  REQUIRE(base != diffMedia);
}

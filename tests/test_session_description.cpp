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
  // Defaults are the fail-safe UNKNOWN sentinel, not a silent AUDIO/RTP_AVP: a
  // default-constructed section must never masquerade as recognized audio/RTP.
  REQUIRE(md.mediaType == MediaType::UNKNOWN);
  REQUIRE(md.port == 0);
  REQUIRE(!md.numberOfPorts.has_value());
  REQUIRE(md.protocol == TransportProtocol::UNKNOWN);
  REQUIRE(md.rawMediaType.empty());
  REQUIRE(md.rawProtocol.empty());
  REQUIRE(md.formats.empty());
  REQUIRE(!md.connection.has_value());
  REQUIRE(md.bandwidths.empty());
  REQUIRE(!md.encryptionKey.has_value());
  REQUIRE(!md.mid.has_value());
  // A default-constructed media section carries NO direction line; the effective
  // direction (session/default sendrecv) is resolved via effectiveDirection().
  REQUIRE(!md.direction.has_value());
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
  sd.origin.emplace();
  sd.origin->sessId = "1234567890";
  sd.origin->sessVersion = "1";

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
  a.origin.emplace();
  a.origin->sessId = "100";
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
  base.origin.emplace();
  base.origin->sessId = "100";
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

// ---------------------------------------------------------------------------
// SessionDescription::effectiveConnection — RFC 4566 §5.7 c= fold-down
// ---------------------------------------------------------------------------

namespace {

/// Builds an audio media section with no media-level c=.
MediaDescription audioSection()
{
  MediaDescription m;
  m.mediaType = MediaType::AUDIO;
  m.port = 49170;
  m.protocol = TransportProtocol::RTP_AVP;
  return m;
}

ConnectionData ip4(const std::string& address)
{
  ConnectionData c;
  c.netType = NetworkType::IN;
  c.addrType = AddressType::IP4;
  c.address = address;
  return c;
}

ConnectionData ip6(const std::string& address)
{
  ConnectionData c;
  c.netType = NetworkType::IN;
  c.addrType = AddressType::IP6;
  c.address = address;
  return c;
}

} // namespace

TEST_CASE("effectiveConnection: precedence cube", "[session][effectiveConnection]")
{
  // Session and media addresses are DISTINCT throughout, so each case
  // discriminates media-precedence from session-fallback rather than merely
  // observing "some value came back".
  const ConnectionData sessionConn = ip4("192.0.2.1");
  const ConnectionData mediaConn = ip4("198.51.100.9");
  REQUIRE(sessionConn != mediaConn);

  SECTION("session-only: a real media section exists but omits its own c=")
  {
    // A REAL section at index 0 — NOT an empty media list, which would resolve
    // through the out-of-range path and prove nothing about the fold-down.
    SessionDescription sd;
    sd.connection = sessionConn;
    sd.mediaDescriptions.push_back(audioSection());
    REQUIRE(!sd.mediaDescriptions[0].connection.has_value());

    const auto resolved = sd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(*resolved == sessionConn);
  }

  SECTION("media-only: no session c=")
  {
    SessionDescription sd;
    REQUIRE(!sd.connection.has_value());
    MediaDescription m = audioSection();
    m.connection = mediaConn;
    sd.mediaDescriptions.push_back(m);

    const auto resolved = sd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(*resolved == mediaConn);
  }

  SECTION("both present: the media-level c= wins")
  {
    SessionDescription sd;
    sd.connection = sessionConn;
    MediaDescription m = audioSection();
    m.connection = mediaConn;
    sd.mediaDescriptions.push_back(m);

    const auto resolved = sd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(*resolved == mediaConn);
    REQUIRE(*resolved != sessionConn);
  }

  SECTION("neither present: nullopt for an in-range index")
  {
    SessionDescription sd;
    sd.mediaDescriptions.push_back(audioSection());

    REQUIRE(!sd.effectiveConnection(0).has_value());
  }

  SECTION("callable through a const reference")
  {
    // The pinned signature is a const member so a consumer can resolve through
    // a `const SessionDescription&`; without this, dropping const still passes.
    SessionDescription sd;
    sd.connection = sessionConn;
    sd.mediaDescriptions.push_back(audioSection());

    const SessionDescription& constSd = sd;
    const auto resolved = constSd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(*resolved == sessionConn);
  }
}

TEST_CASE("effectiveConnection: value preserved verbatim",
          "[session][effectiveConnection]")
{
  SECTION("IPv6 address and addrType survive the fold-down")
  {
    SessionDescription sd;
    sd.connection = ip6("2001:db8::1");
    sd.mediaDescriptions.push_back(audioSection());

    const auto resolved = sd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->addrType == AddressType::IP6);
    REQUIRE(resolved->address == "2001:db8::1");
  }

  SECTION("multicast ttl and address count are returned intact")
  {
    ConnectionData multicast = ip4("224.2.1.1");
    multicast.ttl = 127;
    multicast.numberOfAddresses = 3;

    SessionDescription sd;
    MediaDescription m = audioSection();
    m.connection = multicast;
    sd.mediaDescriptions.push_back(m);

    const auto resolved = sd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(*resolved == multicast);
  }

  SECTION("multicast fields survive the session-level fold-down too")
  {
    // The accessor has two distinct return statements; the media-level one is
    // covered above. Without this, a session branch that rebuilt a partial
    // ConnectionData would pass every other assertion.
    ConnectionData multicast = ip4("224.2.1.1");
    multicast.ttl = 127;
    multicast.numberOfAddresses = 3;

    SessionDescription sd;
    sd.connection = multicast;
    sd.mediaDescriptions.push_back(audioSection());

    const auto resolved = sd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(*resolved == multicast);
  }

  SECTION("a present-but-EMPTY address is returned present and empty")
  {
    // Pins the documented address-usability contract: the resolver returns
    // model state and validates nothing, so has_value() is NOT the acceptance
    // test. A "helpful" empty-address -> nullopt normalisation would silently
    // invert the contract the RTP-1.g adapter is written against.
    SessionDescription sd;
    MediaDescription m = audioSection();
    ConnectionData blank;
    blank.address = "";
    m.connection = blank;
    sd.mediaDescriptions.push_back(m);

    const auto resolved = sd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->address.empty());
  }

  SECTION("an empty session-level address folds down present and empty too")
  {
    SessionDescription sd;
    ConnectionData blank;
    blank.address = "";
    sd.connection = blank;
    sd.mediaDescriptions.push_back(audioSection());

    const auto resolved = sd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->address.empty());
  }

  SECTION("an FQDN address passes through unresolved")
  {
    SessionDescription sd;
    sd.connection = ip4("host.example.com");
    sd.mediaDescriptions.push_back(audioSection());

    const auto resolved = sd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->address == "host.example.com");
  }

  SECTION("multi-m= mixed placement resolves each index independently")
  {
    const ConnectionData sessionConn = ip4("192.0.2.1");
    const ConnectionData videoConn = ip4("198.51.100.9");

    SessionDescription sd;
    sd.connection = sessionConn;

    sd.mediaDescriptions.push_back(audioSection());  // index 0: session-level

    MediaDescription video = audioSection();
    video.mediaType = MediaType::VIDEO;
    video.connection = videoConn;
    sd.mediaDescriptions.push_back(video);           // index 1: media-level

    sd.mediaDescriptions.push_back(audioSection());  // index 2: session-level

    const auto first = sd.effectiveConnection(0);
    const auto second = sd.effectiveConnection(1);
    const auto third = sd.effectiveConnection(2);

    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    REQUIRE(third.has_value());
    REQUIRE(*first == sessionConn);
    REQUIRE(*second == videoConn);
    REQUIRE(*third == sessionConn);
  }
}

TEST_CASE("effectiveConnection: out-of-range never falls back to the session c=",
          "[session][effectiveConnection]")
{
  const ConnectionData sessionConn = ip4("192.0.2.1");

  SECTION("one past the end of a non-empty media list")
  {
    // Two sections, queried at index 2 — the exact one-past boundary. A `<=`
    // mis-guard would read one past the end here; a session-fallback bug would
    // return sessionConn.
    SessionDescription sd;
    sd.connection = sessionConn;
    sd.mediaDescriptions.push_back(audioSection());
    sd.mediaDescriptions.push_back(audioSection());
    REQUIRE(sd.mediaDescriptions.size() == 2);

    REQUIRE_NOTHROW(sd.effectiveConnection(2));
    REQUIRE(!sd.effectiveConnection(2).has_value());

    // The last valid index still resolves, so the guard is not simply refusing
    // everything.
    const auto lastValid = sd.effectiveConnection(1);
    REQUIRE(lastValid.has_value());
    REQUIRE(*lastValid == sessionConn);
  }

  SECTION("empty media list with a session c= present")
  {
    SessionDescription sd;
    sd.connection = sessionConn;
    REQUIRE(sd.mediaDescriptions.empty());

    REQUIRE_NOTHROW(sd.effectiveConnection(0));
    REQUIRE(!sd.effectiveConnection(0).has_value());
  }

  SECTION("a far out-of-range index does not throw")
  {
    SessionDescription sd;
    sd.connection = sessionConn;
    sd.mediaDescriptions.push_back(audioSection());

    REQUIRE_NOTHROW(sd.effectiveConnection(1000));
    REQUIRE(!sd.effectiveConnection(1000).has_value());
  }
}

TEST_CASE("effectiveConnection: unspecified-address sentinels fold down "
          "byte-identical",
          "[session][effectiveConnection]")
{
  // The resolver is deliberately address-blind. 0.0.0.0 and :: carry different
  // meanings depending on context (RFC 3264 §8.4 legacy hold vs RFC 8840 §4.1.1
  // trickle-ICE placeholder), and disambiguating them is the caller's job — so
  // these assert only that the bytes survive, not what they mean.
  SECTION("a real media-level c= overrides a session-level 0.0.0.0")
  {
    const ConnectionData mediaConn = ip4("198.51.100.9");

    SessionDescription sd;
    sd.connection = ip4("0.0.0.0");
    MediaDescription m = audioSection();
    m.connection = mediaConn;
    sd.mediaDescriptions.push_back(m);

    const auto resolved = sd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(*resolved == mediaConn);
    REQUIRE(resolved->address == "198.51.100.9");
  }

  SECTION("a media-level 0.0.0.0 overrides a real session c=")
  {
    SessionDescription sd;
    sd.connection = ip4("192.0.2.1");
    MediaDescription m = audioSection();
    m.connection = ip4("0.0.0.0");
    sd.mediaDescriptions.push_back(m);

    const auto resolved = sd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    // Byte-identical: no special-casing of the hold address into nullopt.
    REQUIRE(resolved->address == "0.0.0.0");
    REQUIRE(resolved->addrType == AddressType::IP4);
  }

  SECTION("the IPv6 :: sentinel is likewise preserved")
  {
    SessionDescription sd;
    sd.connection = ip6("2001:db8::1");
    MediaDescription m = audioSection();
    m.connection = ip6("::");
    sd.mediaDescriptions.push_back(m);

    const auto resolved = sd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->address == "::");
    REQUIRE(resolved->addrType == AddressType::IP6);
  }
}

TEST_CASE("effectiveConnection: a rejected (port 0) stream resolves normally",
          "[session][effectiveConnection]")
{
  // Stream rejection is signalled by the m= port, never by the connection
  // address (RFC 3264 §6 to reject, §8.2 to remove). The fold-down must not special-
  // case a port-0 section, or a caller loses the address of a rejected stream.
  const ConnectionData sessionConn = ip4("192.0.2.1");
  const ConnectionData mediaConn = ip4("198.51.100.9");

  SECTION("port 0 with no media-level c= still folds the session c= down")
  {
    SessionDescription sd;
    sd.connection = sessionConn;
    MediaDescription rejected = audioSection();
    rejected.port = 0;
    sd.mediaDescriptions.push_back(rejected);

    const auto resolved = sd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(*resolved == sessionConn);
  }

  SECTION("port 0 with its own c= still returns the media-level value")
  {
    SessionDescription sd;
    sd.connection = sessionConn;
    MediaDescription rejected = audioSection();
    rejected.port = 0;
    rejected.connection = mediaConn;
    sd.mediaDescriptions.push_back(rejected);

    const auto resolved = sd.effectiveConnection(0);
    REQUIRE(resolved.has_value());
    REQUIRE(*resolved == mediaConn);
  }
}

#include <catch2/catch.hpp>
#include "iora/sdp/attributes.hpp"

using namespace iora::sdp;

// ---------------------------------------------------------------------------
// RtpMapAttribute
// ---------------------------------------------------------------------------

TEST_CASE("RtpMapAttribute: default construction", "[attributes][RtpMapAttribute]")
{
  RtpMapAttribute attr;
  REQUIRE(attr.payloadType == 0);
  REQUIRE(attr.encodingName.empty());
  REQUIRE(attr.clockRate == 0);
  REQUIRE(!attr.channels.has_value());
}

TEST_CASE("RtpMapAttribute: aggregate initialization", "[attributes][RtpMapAttribute]")
{
  RtpMapAttribute attr{111, "opus", 48000, 2};
  REQUIRE(attr.payloadType == 111);
  REQUIRE(attr.encodingName == "opus");
  REQUIRE(attr.clockRate == 48000);
  REQUIRE(attr.channels == 2);
}

TEST_CASE("RtpMapAttribute: equality", "[attributes][RtpMapAttribute]")
{
  RtpMapAttribute a{96, "VP8", 90000, std::nullopt};
  RtpMapAttribute b{96, "VP8", 90000, std::nullopt};
  RtpMapAttribute c{97, "VP8", 90000, std::nullopt};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// FmtpAttribute
// ---------------------------------------------------------------------------

TEST_CASE("FmtpAttribute: default construction", "[attributes][FmtpAttribute]")
{
  FmtpAttribute attr;
  REQUIRE(attr.payloadType == 0);
  REQUIRE(attr.parameters.empty());
  REQUIRE(attr.parameterMap.empty());
}

TEST_CASE("FmtpAttribute: parameterMap stores key=value pairs", "[attributes][FmtpAttribute]")
{
  FmtpAttribute attr;
  attr.payloadType = 111;
  attr.parameters = "minptime=10;useinbandfec=1";
  attr.parameterMap["minptime"] = "10";
  attr.parameterMap["useinbandfec"] = "1";

  REQUIRE(attr.parameterMap.size() == 2);
  REQUIRE(attr.parameterMap.at("minptime") == "10");
  REQUIRE(attr.parameterMap.at("useinbandfec") == "1");
}

TEST_CASE("FmtpAttribute: equality", "[attributes][FmtpAttribute]")
{
  FmtpAttribute a;
  a.payloadType = 111;
  a.parameters = "minptime=10";
  a.parameterMap["minptime"] = "10";

  FmtpAttribute b = a;
  REQUIRE(a == b);

  FmtpAttribute c = a;
  c.parameters = "minptime=20";
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// RtcpFbAttribute
// ---------------------------------------------------------------------------

TEST_CASE("RtcpFbAttribute: default construction", "[attributes][RtcpFbAttribute]")
{
  RtcpFbAttribute attr;
  REQUIRE(attr.payloadType.empty());
  REQUIRE(attr.type.empty());
  REQUIRE(!attr.subtype.has_value());
}

TEST_CASE("RtcpFbAttribute: wildcard payload type", "[attributes][RtcpFbAttribute]")
{
  RtcpFbAttribute attr{"*", "transport-cc", std::nullopt};
  REQUIRE(attr.payloadType == "*");
}

TEST_CASE("RtcpFbAttribute: equality", "[attributes][RtcpFbAttribute]")
{
  RtcpFbAttribute a{"96", "nack", "pli"};
  RtcpFbAttribute b{"96", "nack", "pli"};
  RtcpFbAttribute c{"96", "nack", std::nullopt};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// ExtMapAttribute
// ---------------------------------------------------------------------------

TEST_CASE("ExtMapAttribute: default construction", "[attributes][ExtMapAttribute]")
{
  ExtMapAttribute attr;
  REQUIRE(attr.id == 0);
  REQUIRE(!attr.direction.has_value());
  REQUIRE(attr.uri.empty());
  REQUIRE(!attr.extensionAttributes.has_value());
}

TEST_CASE("ExtMapAttribute: aggregate initialization", "[attributes][ExtMapAttribute]")
{
  ExtMapAttribute attr{1, Direction::SENDONLY,
    "urn:ietf:params:rtp-hdrext:ssrc-audio-level", std::nullopt};
  REQUIRE(attr.id == 1);
  REQUIRE(attr.direction == Direction::SENDONLY);
}

TEST_CASE("ExtMapAttribute: equality", "[attributes][ExtMapAttribute]")
{
  ExtMapAttribute a{1, std::nullopt, "urn:test", std::nullopt};
  ExtMapAttribute b{1, std::nullopt, "urn:test", std::nullopt};
  ExtMapAttribute c{2, std::nullopt, "urn:test", std::nullopt};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// SsrcAttribute
// ---------------------------------------------------------------------------

TEST_CASE("SsrcAttribute: default construction", "[attributes][SsrcAttribute]")
{
  SsrcAttribute attr;
  REQUIRE(attr.ssrc == 0);
  REQUIRE(attr.attributeName.empty());
  REQUIRE(!attr.attributeValue.has_value());
}

TEST_CASE("SsrcAttribute: equality", "[attributes][SsrcAttribute]")
{
  SsrcAttribute a{12345, "cname", "user@example.com"};
  SsrcAttribute b{12345, "cname", "user@example.com"};
  SsrcAttribute c{12345, "cname", "other@example.com"};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// SsrcGroupAttribute
// ---------------------------------------------------------------------------

TEST_CASE("SsrcGroupAttribute: default construction", "[attributes][SsrcGroupAttribute]")
{
  SsrcGroupAttribute attr;
  REQUIRE(attr.semantics.empty());
  REQUIRE(attr.ssrcs.empty());
}

TEST_CASE("SsrcGroupAttribute: equality", "[attributes][SsrcGroupAttribute]")
{
  SsrcGroupAttribute a{"FID", {1000, 2000}};
  SsrcGroupAttribute b{"FID", {1000, 2000}};
  SsrcGroupAttribute c{"FID", {1000, 3000}};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// IceCandidate
// ---------------------------------------------------------------------------

TEST_CASE("IceCandidate: default construction", "[attributes][IceCandidate]")
{
  IceCandidate cand;
  REQUIRE(cand.foundation.empty());
  REQUIRE(cand.component == 0);
  REQUIRE(cand.transport == IceTransportType::UDP);
  REQUIRE(cand.priority == 0);
  REQUIRE(cand.address.empty());
  REQUIRE(cand.port == 0);
  REQUIRE(cand.type == IceCandidateType::HOST);
  REQUIRE(!cand.relAddr.has_value());
  REQUIRE(!cand.relPort.has_value());
  REQUIRE(!cand.tcpType.has_value());
  REQUIRE(cand.extensions.empty());
}

TEST_CASE("IceCandidate: aggregate initialization", "[attributes][IceCandidate]")
{
  IceCandidate cand{
    "1",                             // foundation
    1,                               // component
    IceTransportType::UDP,           // transport
    2130706431,                      // priority
    "192.168.1.1",                   // address
    5000,                            // port
    IceCandidateType::SRFLX,         // type
    std::string("10.0.0.1"),         // relAddr
    std::optional<std::uint16_t>{3478}, // relPort
    std::nullopt,                    // tcpType
    {{"generation", "0"}}            // extensions
  };

  REQUIRE(cand.foundation == "1");
  REQUIRE(cand.component == 1);
  REQUIRE(cand.transport == IceTransportType::UDP);
  REQUIRE(cand.priority == 2130706431);
  REQUIRE(cand.address == "192.168.1.1");
  REQUIRE(cand.port == 5000);
  REQUIRE(cand.type == IceCandidateType::SRFLX);
  REQUIRE(cand.relAddr == "10.0.0.1");
  REQUIRE(cand.relPort == 3478);
  REQUIRE(!cand.tcpType.has_value());
  REQUIRE(cand.extensions.size() == 1);
  REQUIRE(cand.extensions[0].first == "generation");
}

TEST_CASE("IceCandidate: extensions preserve insertion order", "[attributes][IceCandidate]")
{
  IceCandidate cand;
  cand.extensions.push_back({"generation", "0"});
  cand.extensions.push_back({"network-id", "1"});
  cand.extensions.push_back({"network-cost", "10"});

  REQUIRE(cand.extensions.size() == 3);
  REQUIRE(cand.extensions[0].first == "generation");
  REQUIRE(cand.extensions[1].first == "network-id");
  REQUIRE(cand.extensions[2].first == "network-cost");
}

TEST_CASE("IceCandidate: equality", "[attributes][IceCandidate]")
{
  IceCandidate a;
  a.foundation = "1";
  a.component = 1;
  a.transport = IceTransportType::UDP;
  a.priority = 2130706431;
  a.address = "192.168.1.1";
  a.port = 5000;
  a.type = IceCandidateType::HOST;

  IceCandidate b = a;
  REQUIRE(a == b);

  IceCandidate c = a;
  c.type = IceCandidateType::SRFLX;
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// FingerprintAttribute
// ---------------------------------------------------------------------------

TEST_CASE("FingerprintAttribute: default construction", "[attributes][FingerprintAttribute]")
{
  FingerprintAttribute attr;
  REQUIRE(attr.hashFunction.empty());
  REQUIRE(attr.fingerprint.empty());
}

TEST_CASE("FingerprintAttribute: equality", "[attributes][FingerprintAttribute]")
{
  FingerprintAttribute a{"sha-256", "AB:CD:EF:01:23"};
  FingerprintAttribute b{"sha-256", "AB:CD:EF:01:23"};
  FingerprintAttribute c{"sha-256", "FF:FF:FF:FF:FF"};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// CryptoAttribute
// ---------------------------------------------------------------------------

TEST_CASE("CryptoAttribute: default construction", "[attributes][CryptoAttribute]")
{
  CryptoAttribute attr;
  REQUIRE(attr.tag == 0);
  REQUIRE(attr.suite.empty());
  REQUIRE(attr.keyParams.empty());
  REQUIRE(!attr.sessionParams.has_value());
}

TEST_CASE("CryptoAttribute: equality", "[attributes][CryptoAttribute]")
{
  CryptoAttribute a{1, "AES_CM_128_HMAC_SHA1_80", "inline:base64key", std::nullopt};
  CryptoAttribute b{1, "AES_CM_128_HMAC_SHA1_80", "inline:base64key", std::nullopt};
  CryptoAttribute c{1, "AES_CM_128_HMAC_SHA1_80", "inline:otherkey", std::nullopt};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// RtcpAttribute
// ---------------------------------------------------------------------------

TEST_CASE("RtcpAttribute: default construction", "[attributes][RtcpAttribute]")
{
  RtcpAttribute attr;
  REQUIRE(attr.port == 0);
  REQUIRE(!attr.netType.has_value());
  REQUIRE(!attr.addrType.has_value());
  REQUIRE(!attr.address.has_value());
}

TEST_CASE("RtcpAttribute: equality", "[attributes][RtcpAttribute]")
{
  RtcpAttribute a{9, NetworkType::IN, AddressType::IP4, "0.0.0.0"};
  RtcpAttribute b{9, NetworkType::IN, AddressType::IP4, "0.0.0.0"};
  RtcpAttribute c{9, std::nullopt, std::nullopt, std::nullopt};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// MsidAttribute
// ---------------------------------------------------------------------------

TEST_CASE("MsidAttribute: default construction", "[attributes][MsidAttribute]")
{
  MsidAttribute attr;
  REQUIRE(attr.streamId.empty());
  REQUIRE(!attr.trackId.has_value());
}

TEST_CASE("MsidAttribute: equality", "[attributes][MsidAttribute]")
{
  MsidAttribute a{"stream1", "track1"};
  MsidAttribute b{"stream1", "track1"};
  MsidAttribute c{"stream1", std::nullopt};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// GroupAttribute
// ---------------------------------------------------------------------------

TEST_CASE("GroupAttribute: default construction", "[attributes][GroupAttribute]")
{
  GroupAttribute attr;
  REQUIRE(attr.semantics.empty());
  REQUIRE(attr.mids.empty());
}

TEST_CASE("GroupAttribute: equality", "[attributes][GroupAttribute]")
{
  GroupAttribute a{"BUNDLE", {"0", "1", "2"}};
  GroupAttribute b{"BUNDLE", {"0", "1", "2"}};
  GroupAttribute c{"BUNDLE", {"0", "1"}};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// SimulcastStream
// ---------------------------------------------------------------------------

TEST_CASE("SimulcastStream: default paused is false", "[attributes][SimulcastStream]")
{
  SimulcastStream s;
  REQUIRE(s.rid.empty());
  REQUIRE(s.paused == false);
}

TEST_CASE("SimulcastStream: equality", "[attributes][SimulcastStream]")
{
  SimulcastStream a{"h", false};
  SimulcastStream b{"h", false};
  SimulcastStream c{"h", true};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// SimulcastAttribute
// ---------------------------------------------------------------------------

TEST_CASE("SimulcastAttribute: default construction", "[attributes][SimulcastAttribute]")
{
  SimulcastAttribute attr;
  REQUIRE(attr.sendStreams.empty());
  REQUIRE(attr.recvStreams.empty());
}

TEST_CASE("SimulcastAttribute: nested vector structure for alternatives", "[attributes][SimulcastAttribute]")
{
  SimulcastAttribute attr;
  attr.sendStreams = {
    {{"h", false}, {"m", false}},
    {{"l", true}}
  };
  attr.recvStreams = {
    {{"h", false}}
  };

  REQUIRE(attr.sendStreams.size() == 2);
  REQUIRE(attr.sendStreams[0].size() == 2);
  REQUIRE(attr.sendStreams[0][0].rid == "h");
  REQUIRE(attr.sendStreams[1][0].paused == true);
  REQUIRE(attr.recvStreams.size() == 1);
}

TEST_CASE("SimulcastAttribute: equality", "[attributes][SimulcastAttribute]")
{
  SimulcastAttribute a;
  a.sendStreams = {{{"h", false}}};

  SimulcastAttribute b = a;
  REQUIRE(a == b);

  SimulcastAttribute c;
  c.sendStreams = {{{"m", false}}};
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// RidAttribute
// ---------------------------------------------------------------------------

TEST_CASE("RidAttribute: default construction", "[attributes][RidAttribute]")
{
  RidAttribute attr;
  REQUIRE(attr.id.empty());
  REQUIRE(attr.direction == RidDirection::SEND);
  REQUIRE(attr.payloadTypes.empty());
  REQUIRE(attr.restrictions.empty());
}

TEST_CASE("RidAttribute: with restrictions", "[attributes][RidAttribute]")
{
  RidAttribute attr;
  attr.id = "h";
  attr.direction = RidDirection::SEND;
  attr.payloadTypes = {96, 97};
  attr.restrictions["max-width"] = "1280";
  attr.restrictions["max-height"] = "720";

  REQUIRE(attr.payloadTypes.size() == 2);
  REQUIRE(attr.restrictions.at("max-width") == "1280");
}

TEST_CASE("RidAttribute: equality", "[attributes][RidAttribute]")
{
  RidAttribute a;
  a.id = "h";
  a.direction = RidDirection::SEND;

  RidAttribute b = a;
  REQUIRE(a == b);

  RidAttribute c = a;
  c.direction = RidDirection::RECV;
  REQUIRE(a != c);
}

// ---------------------------------------------------------------------------
// GenericAttribute
// ---------------------------------------------------------------------------

TEST_CASE("GenericAttribute: default construction", "[attributes][GenericAttribute]")
{
  GenericAttribute attr;
  REQUIRE(attr.name.empty());
  REQUIRE(!attr.value.has_value());
}

TEST_CASE("GenericAttribute: flag attribute (no value)", "[attributes][GenericAttribute]")
{
  GenericAttribute attr{"rtcp-mux", std::nullopt};
  REQUIRE(attr.name == "rtcp-mux");
  REQUIRE(!attr.value.has_value());
}

TEST_CASE("GenericAttribute: value attribute", "[attributes][GenericAttribute]")
{
  GenericAttribute attr{"x-custom", "some-value"};
  REQUIRE(attr.name == "x-custom");
  REQUIRE(attr.value == "some-value");
}

TEST_CASE("GenericAttribute: equality", "[attributes][GenericAttribute]")
{
  GenericAttribute a{"rtcp-mux", std::nullopt};
  GenericAttribute b{"rtcp-mux", std::nullopt};
  GenericAttribute c{"rtcp-mux", "yes"};

  REQUIRE(a == b);
  REQUIRE(a != c);
}

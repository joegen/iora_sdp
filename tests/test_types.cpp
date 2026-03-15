#include <catch2/catch.hpp>
#include "iora/sdp/types.hpp"

using namespace iora::sdp;

// ---------------------------------------------------------------------------
// MediaType
// ---------------------------------------------------------------------------

TEST_CASE("MediaType: all values are distinct", "[types][MediaType]")
{
  REQUIRE(MediaType::AUDIO != MediaType::VIDEO);
  REQUIRE(MediaType::VIDEO != MediaType::APPLICATION);
  REQUIRE(MediaType::APPLICATION != MediaType::TEXT);
  REQUIRE(MediaType::TEXT != MediaType::MESSAGE);
}

TEST_CASE("MediaType: toString produces correct wire strings", "[types][MediaType]")
{
  REQUIRE(toString(MediaType::AUDIO) == "audio");
  REQUIRE(toString(MediaType::VIDEO) == "video");
  REQUIRE(toString(MediaType::APPLICATION) == "application");
  REQUIRE(toString(MediaType::TEXT) == "text");
  REQUIRE(toString(MediaType::MESSAGE) == "message");
}

TEST_CASE("MediaType: fromString round-trip", "[types][MediaType]")
{
  auto values = {"audio", "video", "application", "text", "message"};
  for (auto s : values)
  {
    auto v = mediaTypeFromString(s);
    REQUIRE(v.has_value());
    REQUIRE(toString(*v) == s);
  }
}

TEST_CASE("MediaType: fromString is case-insensitive", "[types][MediaType]")
{
  REQUIRE(mediaTypeFromString("AUDIO") == MediaType::AUDIO);
  REQUIRE(mediaTypeFromString("Audio") == MediaType::AUDIO);
  REQUIRE(mediaTypeFromString("VIDEO") == MediaType::VIDEO);
}

TEST_CASE("MediaType: fromString invalid returns nullopt", "[types][MediaType]")
{
  REQUIRE(!mediaTypeFromString("invalid").has_value());
  REQUIRE(!mediaTypeFromString("").has_value());
  REQUIRE(!mediaTypeFromString("aud").has_value());
}

// ---------------------------------------------------------------------------
// TransportProtocol
// ---------------------------------------------------------------------------

TEST_CASE("TransportProtocol: all values are distinct", "[types][TransportProtocol]")
{
  REQUIRE(TransportProtocol::UDP != TransportProtocol::RTP_AVP);
  REQUIRE(TransportProtocol::RTP_AVP != TransportProtocol::RTP_SAVP);
  REQUIRE(TransportProtocol::RTP_SAVP != TransportProtocol::RTP_SAVPF);
  REQUIRE(TransportProtocol::RTP_SAVPF != TransportProtocol::UDP_TLS_RTP_SAVPF);
  REQUIRE(TransportProtocol::UDP_TLS_RTP_SAVPF != TransportProtocol::DTLS_SCTP);
  REQUIRE(TransportProtocol::DTLS_SCTP != TransportProtocol::UDP_DTLS_SCTP);
  REQUIRE(TransportProtocol::UDP_DTLS_SCTP != TransportProtocol::TCP_DTLS_SCTP);
}

TEST_CASE("TransportProtocol: underscore-to-slash mapping", "[types][TransportProtocol]")
{
  REQUIRE(toString(TransportProtocol::UDP) == "UDP");
  REQUIRE(toString(TransportProtocol::RTP_AVP) == "RTP/AVP");
  REQUIRE(toString(TransportProtocol::RTP_SAVP) == "RTP/SAVP");
  REQUIRE(toString(TransportProtocol::RTP_SAVPF) == "RTP/SAVPF");
  REQUIRE(toString(TransportProtocol::UDP_TLS_RTP_SAVPF) == "UDP/TLS/RTP/SAVPF");
  REQUIRE(toString(TransportProtocol::DTLS_SCTP) == "DTLS/SCTP");
  REQUIRE(toString(TransportProtocol::UDP_DTLS_SCTP) == "UDP/DTLS/SCTP");
  REQUIRE(toString(TransportProtocol::TCP_DTLS_SCTP) == "TCP/DTLS/SCTP");
}

TEST_CASE("TransportProtocol: fromString round-trip", "[types][TransportProtocol]")
{
  auto values = {"UDP", "RTP/AVP", "RTP/SAVP", "RTP/SAVPF",
                 "UDP/TLS/RTP/SAVPF", "DTLS/SCTP", "UDP/DTLS/SCTP", "TCP/DTLS/SCTP"};
  for (auto s : values)
  {
    auto v = transportProtocolFromString(s);
    REQUIRE(v.has_value());
    REQUIRE(toString(*v) == s);
  }
}

TEST_CASE("TransportProtocol: fromString is case-insensitive", "[types][TransportProtocol]")
{
  REQUIRE(transportProtocolFromString("rtp/avp") == TransportProtocol::RTP_AVP);
  REQUIRE(transportProtocolFromString("udp/tls/rtp/savpf") == TransportProtocol::UDP_TLS_RTP_SAVPF);
}

TEST_CASE("TransportProtocol: fromString invalid returns nullopt", "[types][TransportProtocol]")
{
  REQUIRE(!transportProtocolFromString("invalid").has_value());
  REQUIRE(!transportProtocolFromString("RTP_AVP").has_value());
  REQUIRE(!transportProtocolFromString("").has_value());
}

// ---------------------------------------------------------------------------
// NetworkType
// ---------------------------------------------------------------------------

TEST_CASE("NetworkType: toString", "[types][NetworkType]")
{
  REQUIRE(toString(NetworkType::IN) == "IN");
}

TEST_CASE("NetworkType: fromString round-trip", "[types][NetworkType]")
{
  auto v = networkTypeFromString("IN");
  REQUIRE(v.has_value());
  REQUIRE(*v == NetworkType::IN);
  REQUIRE(toString(*v) == "IN");
}

TEST_CASE("NetworkType: fromString is case-insensitive", "[types][NetworkType]")
{
  REQUIRE(networkTypeFromString("in") == NetworkType::IN);
  REQUIRE(networkTypeFromString("In") == NetworkType::IN);
}

TEST_CASE("NetworkType: fromString invalid returns nullopt", "[types][NetworkType]")
{
  REQUIRE(!networkTypeFromString("OUT").has_value());
  REQUIRE(!networkTypeFromString("").has_value());
}

// ---------------------------------------------------------------------------
// AddressType
// ---------------------------------------------------------------------------

TEST_CASE("AddressType: toString", "[types][AddressType]")
{
  REQUIRE(toString(AddressType::IP4) == "IP4");
  REQUIRE(toString(AddressType::IP6) == "IP6");
}

TEST_CASE("AddressType: fromString round-trip", "[types][AddressType]")
{
  REQUIRE(toString(*addressTypeFromString("IP4")) == "IP4");
  REQUIRE(toString(*addressTypeFromString("IP6")) == "IP6");
}

TEST_CASE("AddressType: fromString is case-insensitive", "[types][AddressType]")
{
  REQUIRE(addressTypeFromString("ip4") == AddressType::IP4);
  REQUIRE(addressTypeFromString("ip6") == AddressType::IP6);
}

TEST_CASE("AddressType: fromString invalid returns nullopt", "[types][AddressType]")
{
  REQUIRE(!addressTypeFromString("IP8").has_value());
}

// ---------------------------------------------------------------------------
// Direction
// ---------------------------------------------------------------------------

TEST_CASE("Direction: all values are distinct", "[types][Direction]")
{
  REQUIRE(Direction::SENDRECV != Direction::SENDONLY);
  REQUIRE(Direction::SENDONLY != Direction::RECVONLY);
  REQUIRE(Direction::RECVONLY != Direction::INACTIVE);
}

TEST_CASE("Direction: toString produces lowercase wire strings", "[types][Direction]")
{
  REQUIRE(toString(Direction::SENDRECV) == "sendrecv");
  REQUIRE(toString(Direction::SENDONLY) == "sendonly");
  REQUIRE(toString(Direction::RECVONLY) == "recvonly");
  REQUIRE(toString(Direction::INACTIVE) == "inactive");
}

TEST_CASE("Direction: fromString round-trip", "[types][Direction]")
{
  for (auto s : {"sendrecv", "sendonly", "recvonly", "inactive"})
  {
    auto v = directionFromString(s);
    REQUIRE(v.has_value());
    REQUIRE(toString(*v) == s);
  }
}

TEST_CASE("Direction: fromString is case-insensitive", "[types][Direction]")
{
  REQUIRE(directionFromString("SENDRECV") == Direction::SENDRECV);
  REQUIRE(directionFromString("SendOnly") == Direction::SENDONLY);
}

TEST_CASE("Direction: SENDRECV is default per RFC 3264", "[types][Direction]")
{
  REQUIRE(Direction::SENDRECV == Direction{});
}

TEST_CASE("Direction: fromString invalid returns nullopt", "[types][Direction]")
{
  REQUIRE(!directionFromString("send").has_value());
  REQUIRE(!directionFromString("").has_value());
}

// ---------------------------------------------------------------------------
// SetupRole
// ---------------------------------------------------------------------------

TEST_CASE("SetupRole: toString", "[types][SetupRole]")
{
  REQUIRE(toString(SetupRole::ACTIVE) == "active");
  REQUIRE(toString(SetupRole::PASSIVE) == "passive");
  REQUIRE(toString(SetupRole::ACTPASS) == "actpass");
  REQUIRE(toString(SetupRole::HOLDCONN) == "holdconn");
}

TEST_CASE("SetupRole: fromString round-trip", "[types][SetupRole]")
{
  for (auto s : {"active", "passive", "actpass", "holdconn"})
  {
    auto v = setupRoleFromString(s);
    REQUIRE(v.has_value());
    REQUIRE(toString(*v) == s);
  }
}

TEST_CASE("SetupRole: fromString is case-insensitive", "[types][SetupRole]")
{
  REQUIRE(setupRoleFromString("ACTPASS") == SetupRole::ACTPASS);
}

TEST_CASE("SetupRole: fromString invalid returns nullopt", "[types][SetupRole]")
{
  REQUIRE(!setupRoleFromString("pass").has_value());
}

// ---------------------------------------------------------------------------
// BandwidthType
// ---------------------------------------------------------------------------

TEST_CASE("BandwidthType: toString", "[types][BandwidthType]")
{
  REQUIRE(toString(BandwidthType::CT) == "CT");
  REQUIRE(toString(BandwidthType::AS) == "AS");
  REQUIRE(toString(BandwidthType::TIAS) == "TIAS");
  REQUIRE(toString(BandwidthType::RS) == "RS");
  REQUIRE(toString(BandwidthType::RR) == "RR");
}

TEST_CASE("BandwidthType: fromString round-trip", "[types][BandwidthType]")
{
  for (auto s : {"CT", "AS", "TIAS", "RS", "RR"})
  {
    auto v = bandwidthTypeFromString(s);
    REQUIRE(v.has_value());
    REQUIRE(toString(*v) == s);
  }
}

TEST_CASE("BandwidthType: fromString is case-insensitive", "[types][BandwidthType]")
{
  REQUIRE(bandwidthTypeFromString("tias") == BandwidthType::TIAS);
  REQUIRE(bandwidthTypeFromString("ct") == BandwidthType::CT);
}

TEST_CASE("BandwidthType: fromString invalid returns nullopt", "[types][BandwidthType]")
{
  REQUIRE(!bandwidthTypeFromString("X-BW").has_value());
}

// ---------------------------------------------------------------------------
// IceCandidateType
// ---------------------------------------------------------------------------

TEST_CASE("IceCandidateType: toString", "[types][IceCandidateType]")
{
  REQUIRE(toString(IceCandidateType::HOST) == "host");
  REQUIRE(toString(IceCandidateType::SRFLX) == "srflx");
  REQUIRE(toString(IceCandidateType::PRFLX) == "prflx");
  REQUIRE(toString(IceCandidateType::RELAY) == "relay");
}

TEST_CASE("IceCandidateType: fromString round-trip", "[types][IceCandidateType]")
{
  for (auto s : {"host", "srflx", "prflx", "relay"})
  {
    auto v = iceCandidateTypeFromString(s);
    REQUIRE(v.has_value());
    REQUIRE(toString(*v) == s);
  }
}

TEST_CASE("IceCandidateType: fromString is case-insensitive", "[types][IceCandidateType]")
{
  REQUIRE(iceCandidateTypeFromString("HOST") == IceCandidateType::HOST);
  REQUIRE(iceCandidateTypeFromString("Relay") == IceCandidateType::RELAY);
}

TEST_CASE("IceCandidateType: fromString invalid returns nullopt", "[types][IceCandidateType]")
{
  REQUIRE(!iceCandidateTypeFromString("local").has_value());
}

// ---------------------------------------------------------------------------
// IceTransportType
// ---------------------------------------------------------------------------

TEST_CASE("IceTransportType: toString", "[types][IceTransportType]")
{
  REQUIRE(toString(IceTransportType::UDP) == "udp");
  REQUIRE(toString(IceTransportType::TCP) == "tcp");
}

TEST_CASE("IceTransportType: fromString round-trip", "[types][IceTransportType]")
{
  REQUIRE(toString(*iceTransportTypeFromString("udp")) == "udp");
  REQUIRE(toString(*iceTransportTypeFromString("tcp")) == "tcp");
}

TEST_CASE("IceTransportType: fromString is case-insensitive", "[types][IceTransportType]")
{
  REQUIRE(iceTransportTypeFromString("UDP") == IceTransportType::UDP);
  REQUIRE(iceTransportTypeFromString("Tcp") == IceTransportType::TCP);
}

TEST_CASE("IceTransportType: fromString invalid returns nullopt", "[types][IceTransportType]")
{
  REQUIRE(!iceTransportTypeFromString("sctp").has_value());
}

// ---------------------------------------------------------------------------
// RidDirection
// ---------------------------------------------------------------------------

TEST_CASE("RidDirection: toString", "[types][RidDirection]")
{
  REQUIRE(toString(RidDirection::SEND) == "send");
  REQUIRE(toString(RidDirection::RECV) == "recv");
}

TEST_CASE("RidDirection: fromString round-trip", "[types][RidDirection]")
{
  REQUIRE(toString(*ridDirectionFromString("send")) == "send");
  REQUIRE(toString(*ridDirectionFromString("recv")) == "recv");
}

TEST_CASE("RidDirection: fromString is case-insensitive", "[types][RidDirection]")
{
  REQUIRE(ridDirectionFromString("SEND") == RidDirection::SEND);
  REQUIRE(ridDirectionFromString("Recv") == RidDirection::RECV);
}

TEST_CASE("RidDirection: fromString invalid returns nullopt", "[types][RidDirection]")
{
  REQUIRE(!ridDirectionFromString("sendrecv").has_value());
}

// ---------------------------------------------------------------------------
// AttributeConstants
// ---------------------------------------------------------------------------

TEST_CASE("AttributeConstants: all values match expected SDP wire strings", "[types][AttributeConstants]")
{
  REQUIRE(AttributeConstants::kRtpmap == "rtpmap");
  REQUIRE(AttributeConstants::kFmtp == "fmtp");
  REQUIRE(AttributeConstants::kRtcp == "rtcp");
  REQUIRE(AttributeConstants::kRtcpMux == "rtcp-mux");
  REQUIRE(AttributeConstants::kRtcpMuxOnly == "rtcp-mux-only");
  REQUIRE(AttributeConstants::kRtcpRsize == "rtcp-rsize");
  REQUIRE(AttributeConstants::kRtcpFb == "rtcp-fb");
  REQUIRE(AttributeConstants::kExtmap == "extmap");
  REQUIRE(AttributeConstants::kExtmapAllowMixed == "extmap-allow-mixed");
  REQUIRE(AttributeConstants::kSsrc == "ssrc");
  REQUIRE(AttributeConstants::kSsrcGroup == "ssrc-group");
  REQUIRE(AttributeConstants::kMid == "mid");
  REQUIRE(AttributeConstants::kMsid == "msid");
  REQUIRE(AttributeConstants::kGroup == "group");
  REQUIRE(AttributeConstants::kIceUfrag == "ice-ufrag");
  REQUIRE(AttributeConstants::kIcePwd == "ice-pwd");
  REQUIRE(AttributeConstants::kIceOptions == "ice-options");
  REQUIRE(AttributeConstants::kIceLite == "ice-lite");
  REQUIRE(AttributeConstants::kCandidate == "candidate");
  REQUIRE(AttributeConstants::kEndOfCandidates == "end-of-candidates");
  REQUIRE(AttributeConstants::kFingerprint == "fingerprint");
  REQUIRE(AttributeConstants::kSetup == "setup");
  REQUIRE(AttributeConstants::kCrypto == "crypto");
  REQUIRE(AttributeConstants::kSendrecv == "sendrecv");
  REQUIRE(AttributeConstants::kSendonly == "sendonly");
  REQUIRE(AttributeConstants::kRecvonly == "recvonly");
  REQUIRE(AttributeConstants::kInactive == "inactive");
  REQUIRE(AttributeConstants::kSctpmap == "sctpmap");
  REQUIRE(AttributeConstants::kSctpPort == "sctp-port");
  REQUIRE(AttributeConstants::kMaxMessageSize == "max-message-size");
  REQUIRE(AttributeConstants::kSimulcast == "simulcast");
  REQUIRE(AttributeConstants::kRid == "rid");
  REQUIRE(AttributeConstants::kFramerate == "framerate");
  REQUIRE(AttributeConstants::kPtime == "ptime");
  REQUIRE(AttributeConstants::kMaxptime == "maxptime");
}

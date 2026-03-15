#pragma once

/// @file attributes.hpp
/// @brief Typed attribute structs for all known SDP attributes.

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "iora/sdp/types.hpp"

namespace iora {
namespace sdp {

/// Parsed a=rtpmap line — maps payload type to encoding name, clock rate,
/// and optional channel count (RFC 4566 section 6, RFC 3551).
struct RtpMapAttribute
{
  std::uint8_t payloadType{0};     ///< RTP payload type number (0-127)
  std::string encodingName;        ///< Codec name (e.g., "opus", "VP8", "H264")
  std::uint32_t clockRate{0};      ///< RTP clock rate in Hz
  std::optional<std::uint8_t> channels; ///< Channel count (audio); omitted for video

  bool operator==(const RtpMapAttribute& other) const noexcept
  {
    return payloadType == other.payloadType
        && encodingName == other.encodingName
        && clockRate == other.clockRate
        && channels == other.channels;
  }

  bool operator!=(const RtpMapAttribute& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Parsed a=fmtp line — format-specific parameters (RFC 4566 section 6).
struct FmtpAttribute
{
  std::uint8_t payloadType{0};     ///< RTP payload type this fmtp applies to
  std::string parameters;          ///< Raw parameter string
  std::unordered_map<std::string, std::string> parameterMap; ///< Parsed key=value pairs

  bool operator==(const FmtpAttribute& other) const noexcept
  {
    return payloadType == other.payloadType
        && parameters == other.parameters
        && parameterMap == other.parameterMap;
  }

  bool operator!=(const FmtpAttribute& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Parsed a=rtcp-fb line — RTCP feedback capability (RFC 4585 section 4.2).
struct RtcpFbAttribute
{
  std::string payloadType;         ///< Payload type number or "*" for all
  std::string type;                ///< Feedback type (e.g., "nack", "transport-cc")
  std::optional<std::string> subtype; ///< Feedback subtype (e.g., "pli", "fir")

  bool operator==(const RtcpFbAttribute& other) const noexcept
  {
    return payloadType == other.payloadType
        && type == other.type
        && subtype == other.subtype;
  }

  bool operator!=(const RtcpFbAttribute& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Parsed a=extmap line — RTP header extension mapping (RFC 5285 / RFC 8285).
struct ExtMapAttribute
{
  std::uint16_t id{0};             ///< Local identifier (1-14 one-byte, 1-255 two-byte)
  std::optional<Direction> direction; ///< Optional direction restriction
  std::string uri;                 ///< Extension URI
  std::optional<std::string> extensionAttributes; ///< Optional extension-specific attributes

  bool operator==(const ExtMapAttribute& other) const noexcept
  {
    return id == other.id
        && direction == other.direction
        && uri == other.uri
        && extensionAttributes == other.extensionAttributes;
  }

  bool operator!=(const ExtMapAttribute& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Parsed a=ssrc line — source-specific attribute (RFC 5576 section 4.1).
struct SsrcAttribute
{
  std::uint32_t ssrc{0};           ///< Synchronization source identifier
  std::string attributeName;       ///< Attribute name (e.g., "cname", "msid")
  std::optional<std::string> attributeValue; ///< Attribute value

  bool operator==(const SsrcAttribute& other) const noexcept
  {
    return ssrc == other.ssrc
        && attributeName == other.attributeName
        && attributeValue == other.attributeValue;
  }

  bool operator!=(const SsrcAttribute& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Parsed a=ssrc-group line — grouping of SSRCs (RFC 5576 section 4.2).
struct SsrcGroupAttribute
{
  std::string semantics;           ///< Grouping semantics (e.g., "FID", "SIM")
  std::vector<std::uint32_t> ssrcs; ///< SSRCs in the group

  bool operator==(const SsrcGroupAttribute& other) const noexcept
  {
    return semantics == other.semantics
        && ssrcs == other.ssrcs;
  }

  bool operator!=(const SsrcGroupAttribute& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Parsed a=candidate line — ICE candidate (RFC 8445 section 5.1).
struct IceCandidate
{
  std::string foundation;          ///< Candidate foundation
  std::uint8_t component{0};      ///< Component ID (1 = RTP, 2 = RTCP)
  IceTransportType transport{IceTransportType::UDP}; ///< Transport protocol
  std::uint32_t priority{0};      ///< Candidate priority
  std::string address;             ///< Candidate IP address
  std::uint16_t port{0};          ///< Candidate port number
  IceCandidateType type{IceCandidateType::HOST}; ///< Candidate type
  std::optional<std::string> relAddr;   ///< Related address (srflx/relay base)
  std::optional<std::uint16_t> relPort; ///< Related port
  std::optional<std::string> tcpType;   ///< TCP candidate type (RFC 6544)
  std::vector<std::pair<std::string, std::string>> extensions; ///< Extension attributes

  bool operator==(const IceCandidate& other) const noexcept
  {
    return foundation == other.foundation
        && component == other.component
        && transport == other.transport
        && priority == other.priority
        && address == other.address
        && port == other.port
        && type == other.type
        && relAddr == other.relAddr
        && relPort == other.relPort
        && tcpType == other.tcpType
        && extensions == other.extensions;
  }

  bool operator!=(const IceCandidate& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Parsed a=fingerprint line — DTLS certificate fingerprint (RFC 4572 section 5).
struct FingerprintAttribute
{
  std::string hashFunction;        ///< Hash algorithm (e.g., "sha-256")
  std::string fingerprint;         ///< Hex-encoded fingerprint with colons

  bool operator==(const FingerprintAttribute& other) const noexcept
  {
    return hashFunction == other.hashFunction
        && fingerprint == other.fingerprint;
  }

  bool operator!=(const FingerprintAttribute& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Parsed a=crypto line — SDES-SRTP key exchange (RFC 4568).
struct CryptoAttribute
{
  std::uint32_t tag{0};           ///< Crypto attribute tag
  std::string suite;               ///< Crypto suite name
  std::string keyParams;           ///< Key parameters
  std::optional<std::string> sessionParams; ///< Optional session parameters

  bool operator==(const CryptoAttribute& other) const noexcept
  {
    return tag == other.tag
        && suite == other.suite
        && keyParams == other.keyParams
        && sessionParams == other.sessionParams;
  }

  bool operator!=(const CryptoAttribute& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Parsed a=rtcp line — explicit RTCP transport address (RFC 3605).
struct RtcpAttribute
{
  std::uint16_t port{0};          ///< RTCP port number
  std::optional<NetworkType> netType;    ///< Network type
  std::optional<AddressType> addrType;   ///< Address type
  std::optional<std::string> address;    ///< RTCP address

  bool operator==(const RtcpAttribute& other) const noexcept
  {
    return port == other.port
        && netType == other.netType
        && addrType == other.addrType
        && address == other.address;
  }

  bool operator!=(const RtcpAttribute& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Parsed a=msid line — WebRTC MediaStream identification (RFC 8830).
struct MsidAttribute
{
  std::string streamId;            ///< MediaStream identifier
  std::optional<std::string> trackId; ///< MediaStreamTrack identifier

  bool operator==(const MsidAttribute& other) const noexcept
  {
    return streamId == other.streamId
        && trackId == other.trackId;
  }

  bool operator!=(const MsidAttribute& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Parsed a=group line — SDP media grouping (RFC 5888, RFC 8843).
struct GroupAttribute
{
  std::string semantics;           ///< Grouping semantics (e.g., "BUNDLE", "LS")
  std::vector<std::string> mids;   ///< Mid values in the group

  bool operator==(const GroupAttribute& other) const noexcept
  {
    return semantics == other.semantics
        && mids == other.mids;
  }

  bool operator!=(const GroupAttribute& other) const noexcept
  {
    return !(*this == other);
  }
};

/// A single simulcast stream entry referencing an a=rid line.
struct SimulcastStream
{
  std::string rid;                 ///< Restriction identifier
  bool paused{false};              ///< Whether this stream starts paused ('~' prefix)

  bool operator==(const SimulcastStream& other) const noexcept
  {
    return rid == other.rid
        && paused == other.paused;
  }

  bool operator!=(const SimulcastStream& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Parsed a=simulcast line — simulcast negotiation (RFC 8853).
/// Inner vectors represent alternatives (separated by ',').
struct SimulcastAttribute
{
  std::vector<std::vector<SimulcastStream>> sendStreams; ///< Send direction streams
  std::vector<std::vector<SimulcastStream>> recvStreams; ///< Recv direction streams

  bool operator==(const SimulcastAttribute& other) const noexcept
  {
    return sendStreams == other.sendStreams
        && recvStreams == other.recvStreams;
  }

  bool operator!=(const SimulcastAttribute& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Parsed a=rid line — restriction identifier for simulcast (RFC 8851).
struct RidAttribute
{
  std::string id;                  ///< Restriction identifier
  RidDirection direction{RidDirection::SEND}; ///< Send or recv
  std::vector<std::uint8_t> payloadTypes; ///< Applicable payload types
  std::unordered_map<std::string, std::string> restrictions; ///< Key=value restrictions

  bool operator==(const RidAttribute& other) const noexcept
  {
    return id == other.id
        && direction == other.direction
        && payloadTypes == other.payloadTypes
        && restrictions == other.restrictions;
  }

  bool operator!=(const RidAttribute& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Catch-all for unrecognized a= lines — preserves name and optional value
/// as raw strings for round-trip fidelity.
struct GenericAttribute
{
  std::string name;                ///< Attribute name
  std::optional<std::string> value; ///< Attribute value (absent for flag attributes)

  bool operator==(const GenericAttribute& other) const noexcept
  {
    return name == other.name
        && value == other.value;
  }

  bool operator!=(const GenericAttribute& other) const noexcept
  {
    return !(*this == other);
  }
};

} // namespace sdp
} // namespace iora

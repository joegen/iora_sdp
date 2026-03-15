#pragma once

/// @file types.hpp
/// @brief Foundational enums, constants, and conversion helpers for SDP parsing/serialization.

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace iora {
namespace sdp {

namespace detail {

inline std::string toLower(std::string_view s)
{
  std::string result(s);
  std::transform(result.begin(), result.end(), result.begin(),
    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return result;
}

inline bool iequals(std::string_view a, std::string_view b)
{
  if (a.size() != b.size())
  {
    return false;
  }
  for (std::size_t i = 0; i < a.size(); ++i)
  {
    if (std::tolower(static_cast<unsigned char>(a[i])) !=
        std::tolower(static_cast<unsigned char>(b[i])))
    {
      return false;
    }
  }
  return true;
}

} // namespace detail

/// SDP media types from the m= line.
enum class MediaType
{
  AUDIO,
  VIDEO,
  APPLICATION,
  TEXT,
  MESSAGE
};

/// Returns the SDP wire-format string for a MediaType.
inline std::string_view toString(MediaType v) noexcept
{
  switch (v)
  {
    case MediaType::AUDIO:       return "audio";
    case MediaType::VIDEO:       return "video";
    case MediaType::APPLICATION: return "application";
    case MediaType::TEXT:        return "text";
    case MediaType::MESSAGE:     return "message";
  }
  return "audio";
}

/// Parses a media type string (case-insensitive).
inline std::optional<MediaType> mediaTypeFromString(std::string_view s)
{
  auto lower = detail::toLower(s);
  if (lower == "audio")       return MediaType::AUDIO;
  if (lower == "video")       return MediaType::VIDEO;
  if (lower == "application") return MediaType::APPLICATION;
  if (lower == "text")        return MediaType::TEXT;
  if (lower == "message")     return MediaType::MESSAGE;
  return std::nullopt;
}

/// SDP transport protocols from the m= line proto field.
/// Enum values use underscores where SDP uses slashes.
enum class TransportProtocol
{
  UDP,
  RTP_AVP,
  RTP_SAVP,
  RTP_SAVPF,
  UDP_TLS_RTP_SAVPF,
  DTLS_SCTP,
  UDP_DTLS_SCTP,
  TCP_DTLS_SCTP
};

/// Returns the SDP wire-format string (slashes, not underscores).
inline std::string_view toString(TransportProtocol v) noexcept
{
  switch (v)
  {
    case TransportProtocol::UDP:                return "UDP";
    case TransportProtocol::RTP_AVP:            return "RTP/AVP";
    case TransportProtocol::RTP_SAVP:           return "RTP/SAVP";
    case TransportProtocol::RTP_SAVPF:          return "RTP/SAVPF";
    case TransportProtocol::UDP_TLS_RTP_SAVPF:  return "UDP/TLS/RTP/SAVPF";
    case TransportProtocol::DTLS_SCTP:          return "DTLS/SCTP";
    case TransportProtocol::UDP_DTLS_SCTP:      return "UDP/DTLS/SCTP";
    case TransportProtocol::TCP_DTLS_SCTP:      return "TCP/DTLS/SCTP";
  }
  return "UDP";
}

/// Parses a transport protocol string with slashes (case-insensitive).
inline std::optional<TransportProtocol> transportProtocolFromString(std::string_view s)
{
  auto lower = detail::toLower(s);
  if (lower == "udp")                return TransportProtocol::UDP;
  if (lower == "rtp/avp")           return TransportProtocol::RTP_AVP;
  if (lower == "rtp/savp")          return TransportProtocol::RTP_SAVP;
  if (lower == "rtp/savpf")         return TransportProtocol::RTP_SAVPF;
  if (lower == "udp/tls/rtp/savpf") return TransportProtocol::UDP_TLS_RTP_SAVPF;
  if (lower == "dtls/sctp")         return TransportProtocol::DTLS_SCTP;
  if (lower == "udp/dtls/sctp")     return TransportProtocol::UDP_DTLS_SCTP;
  if (lower == "tcp/dtls/sctp")     return TransportProtocol::TCP_DTLS_SCTP;
  return std::nullopt;
}

/// SDP network type (c= and o= lines). Only IN (Internet) is defined.
enum class NetworkType
{
  IN
};

/// Returns the SDP wire-format string for a NetworkType.
inline std::string_view toString(NetworkType v) noexcept
{
  switch (v)
  {
    case NetworkType::IN: return "IN";
  }
  return "IN";
}

/// Parses a network type string (case-insensitive).
inline std::optional<NetworkType> networkTypeFromString(std::string_view s)
{
  if (detail::iequals(s, "IN"))
  {
    return NetworkType::IN;
  }
  return std::nullopt;
}

/// SDP address type (c= and o= lines).
enum class AddressType
{
  IP4,
  IP6
};

/// Returns the SDP wire-format string for an AddressType.
inline std::string_view toString(AddressType v) noexcept
{
  switch (v)
  {
    case AddressType::IP4: return "IP4";
    case AddressType::IP6: return "IP6";
  }
  return "IP4";
}

/// Parses an address type string (case-insensitive).
inline std::optional<AddressType> addressTypeFromString(std::string_view s)
{
  if (detail::iequals(s, "IP4")) return AddressType::IP4;
  if (detail::iequals(s, "IP6")) return AddressType::IP6;
  return std::nullopt;
}

/// SDP direction attributes. Default is SENDRECV per RFC 3264 section 6.1.
enum class Direction
{
  SENDRECV,
  SENDONLY,
  RECVONLY,
  INACTIVE
};

/// Returns the SDP wire-format string for a Direction.
inline std::string_view toString(Direction v) noexcept
{
  switch (v)
  {
    case Direction::SENDRECV: return "sendrecv";
    case Direction::SENDONLY: return "sendonly";
    case Direction::RECVONLY: return "recvonly";
    case Direction::INACTIVE: return "inactive";
  }
  return "sendrecv";
}

/// Parses a direction string (case-insensitive).
inline std::optional<Direction> directionFromString(std::string_view s)
{
  auto lower = detail::toLower(s);
  if (lower == "sendrecv") return Direction::SENDRECV;
  if (lower == "sendonly") return Direction::SENDONLY;
  if (lower == "recvonly") return Direction::RECVONLY;
  if (lower == "inactive") return Direction::INACTIVE;
  return std::nullopt;
}

/// DTLS setup role (RFC 4145).
enum class SetupRole
{
  ACTIVE,
  PASSIVE,
  ACTPASS,
  HOLDCONN
};

/// Returns the SDP wire-format string for a SetupRole.
inline std::string_view toString(SetupRole v) noexcept
{
  switch (v)
  {
    case SetupRole::ACTIVE:   return "active";
    case SetupRole::PASSIVE:  return "passive";
    case SetupRole::ACTPASS:  return "actpass";
    case SetupRole::HOLDCONN: return "holdconn";
  }
  return "active";
}

/// Parses a setup role string (case-insensitive).
inline std::optional<SetupRole> setupRoleFromString(std::string_view s)
{
  auto lower = detail::toLower(s);
  if (lower == "active")   return SetupRole::ACTIVE;
  if (lower == "passive")  return SetupRole::PASSIVE;
  if (lower == "actpass")  return SetupRole::ACTPASS;
  if (lower == "holdconn") return SetupRole::HOLDCONN;
  return std::nullopt;
}

/// SDP bandwidth modifier types (b= line).
enum class BandwidthType
{
  CT,   ///< Conference Total
  AS,   ///< Application Specific
  TIAS, ///< Transport Independent Application Specific (RFC 3890)
  RS,   ///< RTCP bandwidth — sender (RFC 3556)
  RR    ///< RTCP bandwidth — receiver (RFC 3556)
};

/// Returns the SDP wire-format string for a BandwidthType.
inline std::string_view toString(BandwidthType v) noexcept
{
  switch (v)
  {
    case BandwidthType::CT:   return "CT";
    case BandwidthType::AS:   return "AS";
    case BandwidthType::TIAS: return "TIAS";
    case BandwidthType::RS:   return "RS";
    case BandwidthType::RR:   return "RR";
  }
  return "CT";
}

/// Parses a bandwidth type string (case-insensitive).
inline std::optional<BandwidthType> bandwidthTypeFromString(std::string_view s)
{
  if (detail::iequals(s, "CT"))   return BandwidthType::CT;
  if (detail::iequals(s, "AS"))   return BandwidthType::AS;
  if (detail::iequals(s, "TIAS")) return BandwidthType::TIAS;
  if (detail::iequals(s, "RS"))   return BandwidthType::RS;
  if (detail::iequals(s, "RR"))   return BandwidthType::RR;
  return std::nullopt;
}

/// ICE candidate types (RFC 8445).
enum class IceCandidateType
{
  HOST,  ///< Local address
  SRFLX, ///< Server reflexive (STUN)
  PRFLX, ///< Peer reflexive
  RELAY  ///< TURN relay
};

/// Returns the SDP wire-format string for an IceCandidateType.
inline std::string_view toString(IceCandidateType v) noexcept
{
  switch (v)
  {
    case IceCandidateType::HOST:  return "host";
    case IceCandidateType::SRFLX: return "srflx";
    case IceCandidateType::PRFLX: return "prflx";
    case IceCandidateType::RELAY: return "relay";
  }
  return "host";
}

/// Parses an ICE candidate type string (case-insensitive).
inline std::optional<IceCandidateType> iceCandidateTypeFromString(std::string_view s)
{
  auto lower = detail::toLower(s);
  if (lower == "host")  return IceCandidateType::HOST;
  if (lower == "srflx") return IceCandidateType::SRFLX;
  if (lower == "prflx") return IceCandidateType::PRFLX;
  if (lower == "relay") return IceCandidateType::RELAY;
  return std::nullopt;
}

/// ICE candidate transport protocol.
enum class IceTransportType
{
  UDP,
  TCP
};

/// Returns the SDP wire-format string for an IceTransportType.
inline std::string_view toString(IceTransportType v) noexcept
{
  switch (v)
  {
    case IceTransportType::UDP: return "udp";
    case IceTransportType::TCP: return "tcp";
  }
  return "udp";
}

/// Parses an ICE transport type string (case-insensitive).
inline std::optional<IceTransportType> iceTransportTypeFromString(std::string_view s)
{
  auto lower = detail::toLower(s);
  if (lower == "udp") return IceTransportType::UDP;
  if (lower == "tcp") return IceTransportType::TCP;
  return std::nullopt;
}

/// Direction qualifier for a=rid restrictions (RFC 8851).
enum class RidDirection
{
  SEND,
  RECV
};

/// Returns the SDP wire-format string for a RidDirection.
inline std::string_view toString(RidDirection v) noexcept
{
  switch (v)
  {
    case RidDirection::SEND: return "send";
    case RidDirection::RECV: return "recv";
  }
  return "send";
}

/// Parses a rid direction string (case-insensitive).
inline std::optional<RidDirection> ridDirectionFromString(std::string_view s)
{
  auto lower = detail::toLower(s);
  if (lower == "send") return RidDirection::SEND;
  if (lower == "recv") return RidDirection::RECV;
  return std::nullopt;
}

/// Compile-time string constants for all known SDP attribute names.
/// Used by the parser for attribute dispatch and by the serializer
/// for canonical attribute name output.
struct AttributeConstants
{
  static constexpr std::string_view kRtpmap = "rtpmap";
  static constexpr std::string_view kFmtp = "fmtp";
  static constexpr std::string_view kRtcp = "rtcp";
  static constexpr std::string_view kRtcpMux = "rtcp-mux";
  static constexpr std::string_view kRtcpMuxOnly = "rtcp-mux-only";
  static constexpr std::string_view kRtcpRsize = "rtcp-rsize";
  static constexpr std::string_view kRtcpFb = "rtcp-fb";

  static constexpr std::string_view kExtmap = "extmap";
  static constexpr std::string_view kExtmapAllowMixed = "extmap-allow-mixed";

  static constexpr std::string_view kSsrc = "ssrc";
  static constexpr std::string_view kSsrcGroup = "ssrc-group";

  static constexpr std::string_view kMid = "mid";
  static constexpr std::string_view kMsid = "msid";

  static constexpr std::string_view kGroup = "group";

  static constexpr std::string_view kIceUfrag = "ice-ufrag";
  static constexpr std::string_view kIcePwd = "ice-pwd";
  static constexpr std::string_view kIceOptions = "ice-options";
  static constexpr std::string_view kIceLite = "ice-lite";

  static constexpr std::string_view kCandidate = "candidate";
  static constexpr std::string_view kEndOfCandidates = "end-of-candidates";

  static constexpr std::string_view kFingerprint = "fingerprint";
  static constexpr std::string_view kSetup = "setup";
  static constexpr std::string_view kCrypto = "crypto";

  static constexpr std::string_view kSendrecv = "sendrecv";
  static constexpr std::string_view kSendonly = "sendonly";
  static constexpr std::string_view kRecvonly = "recvonly";
  static constexpr std::string_view kInactive = "inactive";

  static constexpr std::string_view kSctpmap = "sctpmap";
  static constexpr std::string_view kSctpPort = "sctp-port";
  static constexpr std::string_view kMaxMessageSize = "max-message-size";

  static constexpr std::string_view kSimulcast = "simulcast";
  static constexpr std::string_view kRid = "rid";

  static constexpr std::string_view kFramerate = "framerate";
  static constexpr std::string_view kPtime = "ptime";
  static constexpr std::string_view kMaxptime = "maxptime";
};

} // namespace sdp
} // namespace iora

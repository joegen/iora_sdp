#pragma once

/// @file session_description.hpp
/// @brief Core data model — SessionDescription, MediaDescription, and supporting structs.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "iora/sdp/types.hpp"
#include "iora/sdp/attributes.hpp"

namespace iora {
namespace sdp {

/// Represents the o= line — session originator and session identifier.
struct Origin
{
  std::string username = "-";          ///< User's login or '-'
  std::string sessId = "0";           ///< Numeric session identifier (as string)
  std::string sessVersion = "0";      ///< Numeric session version (as string)
  NetworkType netType = NetworkType::IN;
  AddressType addrType = AddressType::IP4;
  std::string address = "0.0.0.0";    ///< Originating host address

  bool operator==(const Origin& other) const noexcept
  {
    return username == other.username
        && sessId == other.sessId
        && sessVersion == other.sessVersion
        && netType == other.netType
        && addrType == other.addrType
        && address == other.address;
  }

  bool operator!=(const Origin& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Represents a c= line — connection information.
struct ConnectionData
{
  NetworkType netType = NetworkType::IN;
  AddressType addrType = AddressType::IP4;
  std::string address;                 ///< Connection address
  std::optional<std::uint8_t> ttl;     ///< Multicast TTL (IPv4 only)
  std::optional<std::uint16_t> numberOfAddresses; ///< Number of multicast addresses

  bool operator==(const ConnectionData& other) const noexcept
  {
    return netType == other.netType
        && addrType == other.addrType
        && address == other.address
        && ttl == other.ttl
        && numberOfAddresses == other.numberOfAddresses;
  }

  bool operator!=(const ConnectionData& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Represents a b= line — bandwidth information.
struct BandwidthInfo
{
  BandwidthType type = BandwidthType::AS;
  std::uint32_t bandwidth = 0;        ///< Bandwidth in kbps (CT/AS) or bps (TIAS)

  bool operator==(const BandwidthInfo& other) const noexcept
  {
    return type == other.type
        && bandwidth == other.bandwidth;
  }

  bool operator!=(const BandwidthInfo& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Represents an r= line — repeat time using SDP typed time format.
struct RepeatTime
{
  std::string repeatInterval;          ///< Repeat interval (e.g., "7d")
  std::string activeDuration;          ///< Active duration (e.g., "1h")
  std::vector<std::string> offsets;    ///< Offsets from start time

  bool operator==(const RepeatTime& other) const noexcept
  {
    return repeatInterval == other.repeatInterval
        && activeDuration == other.activeDuration
        && offsets == other.offsets;
  }

  bool operator!=(const RepeatTime& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Represents a single z= line entry — time zone adjustment.
struct ZoneAdjustment
{
  std::string adjustmentTime;          ///< NTP time of adjustment
  std::string offset;                  ///< Offset amount

  bool operator==(const ZoneAdjustment& other) const noexcept
  {
    return adjustmentTime == other.adjustmentTime
        && offset == other.offset;
  }

  bool operator!=(const ZoneAdjustment& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Represents a t= line — timing information with optional r= repeat times.
struct TimeDescription
{
  std::uint64_t startTime = 0;        ///< NTP start time (0 = permanent)
  std::uint64_t stopTime = 0;         ///< NTP stop time (0 = unbounded)
  std::vector<RepeatTime> repeatTimes; ///< Associated r= lines

  bool operator==(const TimeDescription& other) const noexcept
  {
    return startTime == other.startTime
        && stopTime == other.stopTime
        && repeatTimes == other.repeatTimes;
  }

  bool operator!=(const TimeDescription& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Represents a single m= block — one media section with its associated attributes.
struct MediaDescription
{
  MediaType mediaType = MediaType::AUDIO;
  std::uint16_t port = 0;             ///< Transport port (0 = rejected)
  std::optional<std::uint16_t> numberOfPorts; ///< Number of ports (multicast)
  TransportProtocol protocol = TransportProtocol::RTP_AVP;
  std::vector<std::string> formats;    ///< Payload type numbers or format strings
  std::optional<ConnectionData> connection; ///< Media-level c= line
  std::vector<BandwidthInfo> bandwidths;
  std::optional<std::string> encryptionKey; ///< k= line (deprecated)
  std::optional<std::string> mid;      ///< a=mid (RFC 5888)
  Direction direction = Direction::SENDRECV;
  std::vector<RtpMapAttribute> rtpMaps;
  std::vector<FmtpAttribute> fmtps;
  std::vector<RtcpFbAttribute> rtcpFeedbacks;
  std::vector<ExtMapAttribute> extMaps;
  std::vector<SsrcAttribute> ssrcs;
  std::vector<SsrcGroupAttribute> ssrcGroups;
  std::vector<IceCandidate> candidates;
  std::vector<FingerprintAttribute> fingerprints;
  std::optional<SetupRole> setup;      ///< a=setup (DTLS role)
  std::optional<std::string> iceUfrag;
  std::optional<std::string> icePwd;
  std::vector<std::string> iceOptions;
  bool rtcpMux = false;               ///< a=rtcp-mux (RFC 5761)
  bool rtcpMuxOnly = false;           ///< a=rtcp-mux-only (RFC 8858)
  bool rtcpRsize = false;             ///< a=rtcp-rsize (RFC 5506)
  std::optional<RtcpAttribute> rtcp;   ///< a=rtcp (RFC 3605)
  std::vector<MsidAttribute> msid;
  std::vector<CryptoAttribute> crypto;
  std::optional<std::uint16_t> sctpPort; ///< a=sctp-port (RFC 8841)
  std::optional<std::uint32_t> maxMessageSize; ///< a=max-message-size (RFC 8841)
  std::optional<SimulcastAttribute> simulcast;
  std::vector<RidAttribute> rids;
  std::optional<std::uint32_t> ptime;  ///< a=ptime (ms)
  std::optional<std::uint32_t> maxptime; ///< a=maxptime (ms)
  std::optional<double> framerate;     ///< a=framerate
  bool endOfCandidates = false;        ///< a=end-of-candidates
  bool extmapAllowMixed = false;       ///< a=extmap-allow-mixed (RFC 8285)
  std::vector<GenericAttribute> genericAttributes; ///< Unrecognized a= lines

  bool operator==(const MediaDescription& other) const noexcept
  {
    return mediaType == other.mediaType
        && port == other.port
        && numberOfPorts == other.numberOfPorts
        && protocol == other.protocol
        && formats == other.formats
        && connection == other.connection
        && bandwidths == other.bandwidths
        && encryptionKey == other.encryptionKey
        && mid == other.mid
        && direction == other.direction
        && rtpMaps == other.rtpMaps
        && fmtps == other.fmtps
        && rtcpFeedbacks == other.rtcpFeedbacks
        && extMaps == other.extMaps
        && ssrcs == other.ssrcs
        && ssrcGroups == other.ssrcGroups
        && candidates == other.candidates
        && fingerprints == other.fingerprints
        && setup == other.setup
        && iceUfrag == other.iceUfrag
        && icePwd == other.icePwd
        && iceOptions == other.iceOptions
        && rtcpMux == other.rtcpMux
        && rtcpMuxOnly == other.rtcpMuxOnly
        && rtcpRsize == other.rtcpRsize
        && rtcp == other.rtcp
        && msid == other.msid
        && crypto == other.crypto
        && sctpPort == other.sctpPort
        && maxMessageSize == other.maxMessageSize
        && simulcast == other.simulcast
        && rids == other.rids
        && ptime == other.ptime
        && maxptime == other.maxptime
        && framerate == other.framerate
        && endOfCandidates == other.endOfCandidates
        && extmapAllowMixed == other.extmapAllowMixed
        && genericAttributes == other.genericAttributes;
  }

  bool operator!=(const MediaDescription& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Top-level container representing a complete SDP document.
struct SessionDescription
{
  std::uint8_t version = 0;           ///< SDP protocol version (always 0)
  Origin origin;
  std::string sessionName = "-";       ///< s= line
  std::optional<std::string> sessionInfo; ///< i= line
  std::optional<std::string> uri;      ///< u= line
  std::optional<std::string> emailAddress; ///< e= line
  std::optional<std::string> phoneNumber;  ///< p= line
  std::optional<ConnectionData> connection; ///< Session-level c= line
  std::vector<BandwidthInfo> bandwidths;
  std::vector<TimeDescription> timeDescriptions;
  std::vector<ZoneAdjustment> zoneAdjustments;
  std::optional<std::string> encryptionKey; ///< k= line (deprecated)
  std::vector<GenericAttribute> attributes; ///< Session-level generic a= lines
  std::vector<MediaDescription> mediaDescriptions;
  std::vector<GroupAttribute> groups;  ///< a=group (BUNDLE, LS, etc.)
  std::optional<std::string> iceUfrag;
  std::optional<std::string> icePwd;
  std::vector<std::string> iceOptions;
  bool iceLite = false;                ///< a=ice-lite
  std::vector<FingerprintAttribute> fingerprints;
  std::optional<SetupRole> setup;
  bool extmapAllowMixed = false;       ///< a=extmap-allow-mixed (RFC 8285)

  bool operator==(const SessionDescription& other) const noexcept
  {
    return version == other.version
        && origin == other.origin
        && sessionName == other.sessionName
        && sessionInfo == other.sessionInfo
        && uri == other.uri
        && emailAddress == other.emailAddress
        && phoneNumber == other.phoneNumber
        && connection == other.connection
        && bandwidths == other.bandwidths
        && timeDescriptions == other.timeDescriptions
        && zoneAdjustments == other.zoneAdjustments
        && encryptionKey == other.encryptionKey
        && attributes == other.attributes
        && mediaDescriptions == other.mediaDescriptions
        && groups == other.groups
        && iceUfrag == other.iceUfrag
        && icePwd == other.icePwd
        && iceOptions == other.iceOptions
        && iceLite == other.iceLite
        && fingerprints == other.fingerprints
        && setup == other.setup
        && extmapAllowMixed == other.extmapAllowMixed;
  }

  bool operator!=(const SessionDescription& other) const noexcept
  {
    return !(*this == other);
  }
};

} // namespace sdp
} // namespace iora

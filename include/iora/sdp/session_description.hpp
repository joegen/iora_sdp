#pragma once

/// @file session_description.hpp
/// @brief Core data model — SessionDescription, MediaDescription, and supporting structs.

#include <cstddef>
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
  /// Multicast TTL. IPv4 only — RFC 4566 §5.7 forbids a TTL for IPv6 multicast,
  /// so the parser rejects a c= carrying one and the serializer omits it when
  /// addrType is IP6.
  std::optional<std::uint8_t> ttl;
  /// Number of contiguous multicast addresses. For IPv4 the wire grammar is
  /// `<base>/<ttl>/<count>`, so this is only serialisable when @ref ttl is also
  /// set; for IPv6 it is the only slash group and stands alone.
  std::optional<std::uint16_t> numberOfAddresses;

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
  /// Media-level direction (a=sendrecv/sendonly/recvonly/inactive). Absent means
  /// no media-level direction line was present — the session-level value applies,
  /// or the RFC 3264 §6.1 / RFC 4566 §6 default of sendrecv. Distinguishing
  /// absent from an explicit a=sendrecv is required for session-level fold-down;
  /// resolve the effective value with SessionDescription::effectiveDirection().
  std::optional<Direction> direction;
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
  /// Session-level direction (a=sendrecv/sendonly/recvonly/inactive). Absent
  /// means no session-level direction line was present. Applies to every media
  /// section that omits its own direction (RFC 4566 §5/§6); resolve the effective
  /// per-media value with effectiveDirection().
  std::optional<Direction> direction;
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

  /// Resolves the effective connection data for one media section (RFC 4566
  /// §5.7): a media-level c= overrides the session-level c=, and a session-level
  /// c= applies to every media section that omits its own.
  ///
  /// @param mediaIndex Index into @ref mediaDescriptions.
  /// @return The media-level c= when @p mediaIndex is in range and carries one;
  ///         otherwise the session-level c= when @p mediaIndex is in range;
  ///         otherwise `std::nullopt`.
  ///
  /// The call is total: an out-of-range index yields `std::nullopt` — never the
  /// session-level c= — and no exception is thrown (only the returned copy can
  /// propagate an allocation failure). RFC 4566 §5 scopes session-level
  /// information to the media OF THAT SESSION, and §5.7's per-media values
  /// override it "for the respective media"; an index past the last section
  /// names no stream, so answering with the session address would fabricate a
  /// connection.
  ///
  /// @note The returned ConnectionData is never inspected or normalized. Every
  ///   value — the unspecified-address sentinels `0.0.0.0` and `::`, FQDNs,
  ///   multicast groups — is returned verbatim. Collapsing a sentinel into
  ///   `std::nullopt` here would destroy the signal callers depend on.
  ///
  /// @note Direction: this folds down the c= of a *received* description. An
  ///   outbound description takes its connection address from the local session
  ///   identity, never from here.
  ///
  /// @note A `std::nullopt` for an in-range index means the description is
  ///   malformed and unusable — §5.7 makes a session-level c= mandatory unless
  ///   every media section supplies its own. Callers reject such a description
  ///   rather than defaulting or fabricating an empty address. A *present*
  ///   value is not automatically a usable one either: ConnectionData is
  ///   returned verbatim and validated against nothing, so a programmatically
  ///   built description can carry a present-but-empty or ill-formed address.
  ///   Callers check the address is non-empty and well formed for its addrType
  ///   and reject it exactly as they reject `std::nullopt` — `has_value()`
  ///   alone is not the acceptance test.
  ///
  /// @note Two things this value is NOT. It is not necessarily the media
  ///   transport — ICE and BUNDLE can put that elsewhere, and `a=rtcp`
  ///   (RFC 3605) can move the RTCP destination alone — and an unspecified
  ///   address (`0.0.0.0` or `::`) is
  ///   not classifiable from the address alone, so it does not by itself mean
  ///   hold. After a lenient parse the value may also be the session-level c=
  ///   standing in for a media-level override that was dropped as malformed.
  ///   The RFC citations and the rules a caller must follow for all three are
  ///   in docs/programmers_manual.md §4, "effectiveConnection".
  ///
  /// @note Model limitation: one optional ConnectionData per level. The slash
  ///   notation `<base>/<ttl>/<count>`, which §5.7 calls semantically identical
  ///   to multiple c= lines, IS carried verbatim; what cannot be represented is
  ///   a media section carrying several *separate* c= lines with distinct base
  ///   addresses.
  std::optional<ConnectionData> effectiveConnection(std::size_t mediaIndex) const
  {
    if (mediaIndex >= mediaDescriptions.size())
    {
      return std::nullopt;
    }

    const std::optional<ConnectionData>& mediaLevel =
      mediaDescriptions[mediaIndex].connection;
    if (mediaLevel.has_value())
    {
      return mediaLevel;
    }

    return connection;
  }

  /// Resolves the effective direction for one media section (RFC 4566 §5/§6):
  /// a media-level direction attribute overrides the session-level one, and a
  /// session-level direction applies to every media section that omits its own.
  /// When neither level carries a direction attribute the RFC 3264 §6.1 /
  /// RFC 4566 §6 default of sendrecv is applied HERE, so no caller re-homes it.
  ///
  /// @param mediaIndex Index into @ref mediaDescriptions.
  /// @return For an in-range index, always a concrete Direction: the media-level
  ///         value if present, else the session-level value if present, else
  ///         Direction::SENDRECV. `std::nullopt` iff @p mediaIndex is OUT OF
  ///         RANGE — nothing else.
  ///
  /// @note The `std::nullopt` here is NARROWER than effectiveConnection's: c= is
  ///   mandatory, so its absence at both levels is malformed and yields nullopt;
  ///   direction has an RFC-defined default, so its absence is never malformed
  ///   and never yields nullopt for an in-range index. The full RFC prose is in
  ///   docs/programmers_manual.md, "effectiveDirection".
  ///
  /// @note A port-0 (rejected) section still has its direction folded here; this
  ///   resolver is direction-only and port-agnostic. Rejection dominates for
  ///   hold classification — that precedence belongs to the consumer / the
  ///   offer/answer answer path, not this resolver.
  ///
  /// The call is total: an out-of-range index yields `std::nullopt` and no
  /// exception is thrown.
  std::optional<Direction> effectiveDirection(std::size_t mediaIndex) const
  {
    if (mediaIndex >= mediaDescriptions.size())
    {
      return std::nullopt;
    }

    const std::optional<Direction>& mediaLevel =
      mediaDescriptions[mediaIndex].direction;
    if (mediaLevel.has_value())
    {
      return mediaLevel;
    }
    if (direction.has_value())
    {
      return direction;
    }
    return Direction::SENDRECV;
  }

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
        && direction == other.direction
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

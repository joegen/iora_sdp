#pragma once

/// @file sdp_serializer.hpp
/// @brief SDP serializer — converts a SessionDescription data model into conformant SDP text.

#include <algorithm>
#include <cstdio>
#include <string>

#include "iora/sdp/sdp_parser.hpp"

namespace iora {
namespace sdp {

/// Controls serializer behavior.
struct SdpSerializerOptions
{
  std::string lineEnding = "\r\n";   ///< Line ending sequence (CRLF or LF)
  bool omitSessionName = false;      ///< Omit s= line when sessionName is empty
  bool omitTiming = false;           ///< Omit t= lines when timeDescriptions is empty

  bool operator==(const SdpSerializerOptions& other) const noexcept
  {
    return lineEnding == other.lineEnding
        && omitSessionName == other.omitSessionName
        && omitTiming == other.omitTiming;
  }

  bool operator!=(const SdpSerializerOptions& other) const noexcept
  {
    return !(*this == other);
  }
};

namespace detail {

inline std::string serializeDouble(double v)
{
  auto intVal = static_cast<std::int64_t>(v);
  if (static_cast<double>(intVal) == v)
  {
    return std::to_string(intVal);
  }
  char buf[64];
  int len = std::snprintf(buf, sizeof(buf), "%.10f", v);
  std::string result(buf, static_cast<std::size_t>(len));
  for (auto& c : result)
  {
    if (c == ',') c = '.';
  }
  auto dot = result.find('.');
  if (dot != std::string::npos)
  {
    while (result.back() == '0') result.pop_back();
    if (result.back() == '.') result.pop_back();
  }
  return result;
}

inline std::string serializeOrigin(const Origin& o, const std::string& le)
{
  return "o=" + o.username + " " + o.sessId + " " + o.sessVersion + " "
      + std::string(toString(o.netType)) + " "
      + std::string(toString(o.addrType)) + " "
      + o.address + le;
}

inline std::string serializeConnection(const ConnectionData& c, const std::string& le)
{
  std::string result = "c=" + std::string(toString(c.netType)) + " "
      + std::string(toString(c.addrType)) + " " + c.address;
  if (c.ttl.has_value())
  {
    result += "/" + std::to_string(*c.ttl);
    if (c.numberOfAddresses.has_value())
    {
      result += "/" + std::to_string(*c.numberOfAddresses);
    }
  }
  result += le;
  return result;
}

inline std::string serializeBandwidth(const BandwidthInfo& b, const std::string& le)
{
  return "b=" + std::string(toString(b.type)) + ":" + std::to_string(b.bandwidth) + le;
}

inline std::string serializeRepeatTime(const RepeatTime& rt, const std::string& le)
{
  std::string result = "r=" + rt.repeatInterval + " " + rt.activeDuration;
  for (const auto& offset : rt.offsets)
  {
    result += " " + offset;
  }
  result += le;
  return result;
}

inline std::string serializeTimeDescription(const TimeDescription& td, const std::string& le)
{
  std::string result = "t=" + std::to_string(td.startTime) + " "
      + std::to_string(td.stopTime) + le;
  for (const auto& rt : td.repeatTimes)
  {
    result += serializeRepeatTime(rt, le);
  }
  return result;
}

inline std::string serializeZoneAdjustments(
    const std::vector<ZoneAdjustment>& za, const std::string& le)
{
  if (za.empty()) return {};
  std::string result = "z=";
  for (std::size_t i = 0; i < za.size(); ++i)
  {
    if (i > 0) result += " ";
    result += za[i].adjustmentTime + " " + za[i].offset;
  }
  result += le;
  return result;
}

inline std::string serializeRtpMap(const RtpMapAttribute& attr, const std::string& le)
{
  std::string result = "a=rtpmap:" + std::to_string(attr.payloadType) + " "
      + attr.encodingName + "/" + std::to_string(attr.clockRate);
  if (attr.channels.has_value())
  {
    result += "/" + std::to_string(*attr.channels);
  }
  result += le;
  return result;
}

inline std::string serializeFmtp(const FmtpAttribute& attr, const std::string& le)
{
  return "a=fmtp:" + std::to_string(attr.payloadType) + " " + attr.parameters + le;
}

inline std::string serializeRtcpFb(const RtcpFbAttribute& attr, const std::string& le)
{
  std::string result = "a=rtcp-fb:" + attr.payloadType + " " + attr.type;
  if (attr.subtype.has_value())
  {
    result += " " + *attr.subtype;
  }
  result += le;
  return result;
}

inline std::string serializeExtMap(const ExtMapAttribute& attr, const std::string& le)
{
  std::string result = "a=extmap:" + std::to_string(attr.id);
  if (attr.direction.has_value())
  {
    result += "/" + std::string(toString(*attr.direction));
  }
  result += " " + attr.uri;
  if (attr.extensionAttributes.has_value())
  {
    result += " " + *attr.extensionAttributes;
  }
  result += le;
  return result;
}

inline std::string serializeSsrc(const SsrcAttribute& attr, const std::string& le)
{
  std::string result = "a=ssrc:" + std::to_string(attr.ssrc) + " " + attr.attributeName;
  if (attr.attributeValue.has_value())
  {
    result += ":" + *attr.attributeValue;
  }
  result += le;
  return result;
}

inline std::string serializeSsrcGroup(const SsrcGroupAttribute& attr, const std::string& le)
{
  std::string result = "a=ssrc-group:" + attr.semantics;
  for (const auto& ssrc : attr.ssrcs)
  {
    result += " " + std::to_string(ssrc);
  }
  result += le;
  return result;
}

inline std::string serializeRtcp(const RtcpAttribute& attr, const std::string& le)
{
  std::string result = "a=rtcp:" + std::to_string(attr.port);
  if (attr.netType.has_value() && attr.addrType.has_value() && attr.address.has_value())
  {
    result += " " + std::string(toString(*attr.netType))
        + " " + std::string(toString(*attr.addrType))
        + " " + *attr.address;
  }
  result += le;
  return result;
}

inline std::string serializeCandidate(const IceCandidate& cand, const std::string& le)
{
  std::string result = "a=candidate:" + cand.foundation
      + " " + std::to_string(cand.component)
      + " " + std::string(toString(cand.transport))
      + " " + std::to_string(cand.priority)
      + " " + cand.address
      + " " + std::to_string(cand.port)
      + " typ " + std::string(toString(cand.type));
  if (cand.relAddr.has_value() && cand.relPort.has_value())
  {
    result += " raddr " + *cand.relAddr + " rport " + std::to_string(*cand.relPort);
  }
  if (cand.tcpType.has_value())
  {
    result += " tcptype " + *cand.tcpType;
  }
  for (const auto& ext : cand.extensions)
  {
    result += " " + ext.first + " " + ext.second;
  }
  result += le;
  return result;
}

inline std::string serializeFingerprint(const FingerprintAttribute& attr, const std::string& le)
{
  return "a=fingerprint:" + attr.hashFunction + " " + attr.fingerprint + le;
}

inline std::string serializeCrypto(const CryptoAttribute& attr, const std::string& le)
{
  std::string result = "a=crypto:" + std::to_string(attr.tag) + " "
      + attr.suite + " " + attr.keyParams;
  if (attr.sessionParams.has_value())
  {
    result += " " + *attr.sessionParams;
  }
  result += le;
  return result;
}

inline std::string serializeMsid(const MsidAttribute& attr, const std::string& le)
{
  std::string result = "a=msid:" + attr.streamId;
  if (attr.trackId.has_value())
  {
    result += " " + *attr.trackId;
  }
  result += le;
  return result;
}

inline std::string serializeGroup(const GroupAttribute& attr, const std::string& le)
{
  std::string result = "a=group:" + attr.semantics;
  for (const auto& mid : attr.mids)
  {
    result += " " + mid;
  }
  result += le;
  return result;
}

inline std::string serializeSimulcastStreams(
    const std::vector<std::vector<SimulcastStream>>& streams)
{
  std::string result;
  for (std::size_t i = 0; i < streams.size(); ++i)
  {
    if (i > 0) result += ';';
    for (std::size_t j = 0; j < streams[i].size(); ++j)
    {
      if (j > 0) result += ',';
      if (streams[i][j].paused) result += '~';
      result += streams[i][j].rid;
    }
  }
  return result;
}

inline std::string serializeSimulcast(const SimulcastAttribute& attr, const std::string& le)
{
  std::string result = "a=simulcast:";
  bool hasSend = !attr.sendStreams.empty();
  bool hasRecv = !attr.recvStreams.empty();
  if (hasSend)
  {
    result += "send " + serializeSimulcastStreams(attr.sendStreams);
  }
  if (hasSend && hasRecv)
  {
    result += " ";
  }
  if (hasRecv)
  {
    result += "recv " + serializeSimulcastStreams(attr.recvStreams);
  }
  result += le;
  return result;
}

inline std::string serializeRid(const RidAttribute& attr, const std::string& le)
{
  std::string result = "a=rid:" + attr.id + " " + std::string(toString(attr.direction));

  std::vector<std::string> parts;
  if (!attr.payloadTypes.empty())
  {
    std::string pt = "pt=";
    for (std::size_t i = 0; i < attr.payloadTypes.size(); ++i)
    {
      if (i > 0) pt += ',';
      pt += std::to_string(attr.payloadTypes[i]);
    }
    parts.push_back(std::move(pt));
  }

  if (!attr.restrictions.empty())
  {
    std::vector<std::string> keys;
    keys.reserve(attr.restrictions.size());
    for (const auto& kv : attr.restrictions)
    {
      keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());
    for (const auto& key : keys)
    {
      parts.push_back(key + "=" + attr.restrictions.at(key));
    }
  }

  if (!parts.empty())
  {
    result += " ";
    for (std::size_t i = 0; i < parts.size(); ++i)
    {
      if (i > 0) result += ';';
      result += parts[i];
    }
  }

  result += le;
  return result;
}

inline std::string serializeGenericAttribute(const GenericAttribute& attr, const std::string& le)
{
  std::string result = "a=" + attr.name;
  if (attr.value.has_value())
  {
    result += ":" + *attr.value;
  }
  result += le;
  return result;
}

inline std::string serializeSessionLevel(
    const SessionDescription& session, const SdpSerializerOptions& options)
{
  const auto& le = options.lineEnding;
  std::string result;

  // v=
  result += "v=" + std::to_string(session.version) + le;

  // o=
  result += serializeOrigin(session.origin, le);

  // s=
  if (options.omitSessionName && session.sessionName.empty())
  {
    // Skip s= line
  }
  else
  {
    result += "s=" + session.sessionName + le;
  }

  // i= (optional)
  if (session.sessionInfo.has_value())
  {
    result += "i=" + *session.sessionInfo + le;
  }

  // u= (optional)
  if (session.uri.has_value())
  {
    result += "u=" + *session.uri + le;
  }

  // e= (optional)
  if (session.emailAddress.has_value())
  {
    result += "e=" + *session.emailAddress + le;
  }

  // p= (optional)
  if (session.phoneNumber.has_value())
  {
    result += "p=" + *session.phoneNumber + le;
  }

  // c= (optional session-level connection)
  if (session.connection.has_value())
  {
    result += serializeConnection(*session.connection, le);
  }

  // b= (multiple)
  for (const auto& bw : session.bandwidths)
  {
    result += serializeBandwidth(bw, le);
  }

  // t= (with r=)
  if (!session.timeDescriptions.empty())
  {
    for (const auto& td : session.timeDescriptions)
    {
      result += serializeTimeDescription(td, le);
    }
  }
  else if (!options.omitTiming)
  {
    result += "t=0 0" + le;
  }

  // z=
  result += serializeZoneAdjustments(session.zoneAdjustments, le);

  // k=
  if (session.encryptionKey.has_value())
  {
    result += "k=" + *session.encryptionKey + le;
  }

  // Session-level a= attributes in conventional order:
  // ice-ufrag, ice-pwd, ice-options, ice-lite, fingerprint, setup,
  // extmap-allow-mixed, group, generic

  if (session.iceUfrag.has_value())
  {
    result += "a=ice-ufrag:" + *session.iceUfrag + le;
  }
  if (session.icePwd.has_value())
  {
    result += "a=ice-pwd:" + *session.icePwd + le;
  }
  if (!session.iceOptions.empty())
  {
    result += "a=ice-options:";
    for (std::size_t i = 0; i < session.iceOptions.size(); ++i)
    {
      if (i > 0) result += " ";
      result += session.iceOptions[i];
    }
    result += le;
  }
  if (session.iceLite)
  {
    result += "a=ice-lite" + le;
  }
  for (const auto& fp : session.fingerprints)
  {
    result += serializeFingerprint(fp, le);
  }
  if (session.setup.has_value())
  {
    result += "a=setup:" + std::string(toString(*session.setup)) + le;
  }
  if (session.extmapAllowMixed)
  {
    result += "a=extmap-allow-mixed" + le;
  }
  for (const auto& group : session.groups)
  {
    result += serializeGroup(group, le);
  }
  for (const auto& attr : session.attributes)
  {
    result += serializeGenericAttribute(attr, le);
  }

  return result;
}

inline std::string serializeMediaLevel(
    const MediaDescription& media, const SdpSerializerOptions& options)
{
  const auto& le = options.lineEnding;
  std::string result;

  // m= line
  result += "m=" + std::string(toString(media.mediaType)) + " "
      + std::to_string(media.port);
  if (media.numberOfPorts.has_value())
  {
    result += "/" + std::to_string(*media.numberOfPorts);
  }
  result += " " + std::string(toString(media.protocol));
  for (const auto& fmt : media.formats)
  {
    result += " " + fmt;
  }
  result += le;

  // c= (media-level connection)
  if (media.connection.has_value())
  {
    result += serializeConnection(*media.connection, le);
  }

  // b= (multiple)
  for (const auto& bw : media.bandwidths)
  {
    result += serializeBandwidth(bw, le);
  }

  // k= (media-level encryption key)
  if (media.encryptionKey.has_value())
  {
    result += "k=" + *media.encryptionKey + le;
  }

  // Media-level a= attributes in conventional order

  // 1. direction
  result += "a=" + std::string(toString(media.direction)) + le;

  // 2. mid
  if (media.mid.has_value())
  {
    result += "a=mid:" + *media.mid + le;
  }

  // 3. msid (multiple)
  for (const auto& m : media.msid)
  {
    result += serializeMsid(m, le);
  }

  // 4. ice-ufrag, ice-pwd, ice-options
  if (media.iceUfrag.has_value())
  {
    result += "a=ice-ufrag:" + *media.iceUfrag + le;
  }
  if (media.icePwd.has_value())
  {
    result += "a=ice-pwd:" + *media.icePwd + le;
  }
  if (!media.iceOptions.empty())
  {
    result += "a=ice-options:";
    for (std::size_t i = 0; i < media.iceOptions.size(); ++i)
    {
      if (i > 0) result += " ";
      result += media.iceOptions[i];
    }
    result += le;
  }

  // 5. fingerprint (multiple), setup
  for (const auto& fp : media.fingerprints)
  {
    result += serializeFingerprint(fp, le);
  }
  if (media.setup.has_value())
  {
    result += "a=setup:" + std::string(toString(*media.setup)) + le;
  }

  // 6. extmap-allow-mixed
  if (media.extmapAllowMixed)
  {
    result += "a=extmap-allow-mixed" + le;
  }

  // 7. rtcp-mux, rtcp-mux-only, rtcp-rsize
  if (media.rtcpMux)
  {
    result += "a=rtcp-mux" + le;
  }
  if (media.rtcpMuxOnly)
  {
    result += "a=rtcp-mux-only" + le;
  }
  if (media.rtcpRsize)
  {
    result += "a=rtcp-rsize" + le;
  }

  // 8. rtcp
  if (media.rtcp.has_value())
  {
    result += serializeRtcp(*media.rtcp, le);
  }

  // 9. extmap (multiple)
  for (const auto& em : media.extMaps)
  {
    result += serializeExtMap(em, le);
  }

  // 10. Codec grouping by payload type
  for (const auto& fmt : media.formats)
  {
    const RtpMapAttribute* rtpmap = nullptr;
    for (const auto& rm : media.rtpMaps)
    {
      if (std::to_string(rm.payloadType) == fmt)
      {
        rtpmap = &rm;
        break;
      }
    }
    if (!rtpmap)
    {
      continue;
    }

    result += serializeRtpMap(*rtpmap, le);

    for (const auto& fp : media.fmtps)
    {
      if (std::to_string(fp.payloadType) == fmt)
      {
        result += serializeFmtp(fp, le);
        break;
      }
    }

    for (const auto& fb : media.rtcpFeedbacks)
    {
      if (fb.payloadType == fmt)
      {
        result += serializeRtcpFb(fb, le);
      }
    }
  }

  // 11. Wildcard rtcp-fb (payloadType='*')
  for (const auto& fb : media.rtcpFeedbacks)
  {
    if (fb.payloadType == "*")
    {
      result += serializeRtcpFb(fb, le);
    }
  }

  // 12. ptime, maxptime, framerate
  if (media.ptime.has_value())
  {
    result += "a=ptime:" + std::to_string(*media.ptime) + le;
  }
  if (media.maxptime.has_value())
  {
    result += "a=maxptime:" + std::to_string(*media.maxptime) + le;
  }
  if (media.framerate.has_value())
  {
    result += "a=framerate:" + serializeDouble(*media.framerate) + le;
  }

  // 13. ssrc (multiple), ssrc-group (multiple)
  for (const auto& s : media.ssrcs)
  {
    result += serializeSsrc(s, le);
  }
  for (const auto& sg : media.ssrcGroups)
  {
    result += serializeSsrcGroup(sg, le);
  }

  // 14. crypto (multiple)
  for (const auto& c : media.crypto)
  {
    result += serializeCrypto(c, le);
  }

  // 15. sctp-port, max-message-size
  if (media.sctpPort.has_value())
  {
    result += "a=sctp-port:" + std::to_string(*media.sctpPort) + le;
  }
  if (media.maxMessageSize.has_value())
  {
    result += "a=max-message-size:" + std::to_string(*media.maxMessageSize) + le;
  }

  // 16. simulcast, rid (multiple)
  if (media.simulcast.has_value())
  {
    result += serializeSimulcast(*media.simulcast, le);
  }
  for (const auto& r : media.rids)
  {
    result += serializeRid(r, le);
  }

  // 17. candidate (multiple), end-of-candidates
  for (const auto& c : media.candidates)
  {
    result += serializeCandidate(c, le);
  }
  if (media.endOfCandidates)
  {
    result += "a=end-of-candidates" + le;
  }

  // 18. generic attributes (preserve insertion order)
  for (const auto& ga : media.genericAttributes)
  {
    result += serializeGenericAttribute(ga, le);
  }

  return result;
}

} // namespace detail

/// SDP serializer. All methods are static; the class is not instantiable.
struct SdpSerializer
{
  SdpSerializer() = delete;

  /// Serialize a SessionDescription to SDP text with default options.
  static std::string serialize(const SessionDescription& session)
  {
    return serialize(session, SdpSerializerOptions{});
  }

  /// Serialize a SessionDescription to SDP text with explicit options.
  static std::string serialize(
      const SessionDescription& session, const SdpSerializerOptions& options)
  {
    std::string result = detail::serializeSessionLevel(session, options);
    for (const auto& media : session.mediaDescriptions)
    {
      result += detail::serializeMediaLevel(media, options);
    }
    return result;
  }

  /// Serialize a single media section with default options.
  static std::string serializeMediaSection(const MediaDescription& media)
  {
    return serializeMediaSection(media, SdpSerializerOptions{});
  }

  /// Serialize a single media section with explicit options.
  static std::string serializeMediaSection(
      const MediaDescription& media, const SdpSerializerOptions& options)
  {
    return detail::serializeMediaLevel(media, options);
  }

  /// Serialize a GenericAttribute to 'a=name[:value]' (no line ending appended).
  static std::string serializeAttribute(const GenericAttribute& attr)
  {
    std::string result = "a=" + attr.name;
    if (attr.value.has_value())
    {
      result += ":" + *attr.value;
    }
    return result;
  }
};

} // namespace sdp
} // namespace iora

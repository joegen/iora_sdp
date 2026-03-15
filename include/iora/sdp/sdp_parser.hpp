#pragma once

/// @file sdp_parser.hpp
/// @brief SDP text parser — converts raw SDP into a SessionDescription data model.

#include <charconv>
#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "iora/sdp/session_description.hpp"

namespace iora {
namespace sdp {

/// Describes a fatal parse error.
struct ParseError
{
  std::size_t line = 0;            ///< Line number (1-based)
  std::string message;             ///< Human-readable description

  bool operator==(const ParseError& other) const noexcept
  {
    return line == other.line && message == other.message;
  }

  bool operator!=(const ParseError& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Describes a non-fatal parse warning.
struct ParseWarning
{
  std::size_t line = 0;            ///< Line number (1-based)
  std::string message;             ///< Human-readable description

  bool operator==(const ParseWarning& other) const noexcept
  {
    return line == other.line && message == other.message;
  }

  bool operator!=(const ParseWarning& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Result type returned by parse operations.
template<typename T>
struct ParseResult
{
  std::optional<T> value;                  ///< Parsed result (present on success)
  std::optional<ParseError> error;         ///< Fatal error (present on failure)
  std::vector<ParseWarning> warnings;      ///< Non-fatal issues

  bool hasValue() const noexcept { return value.has_value(); }
  bool hasError() const noexcept { return error.has_value(); }
};

/// Controls parser behavior.
struct SdpParserOptions
{
  bool strict = false;                     ///< Reject on any malformed line
  bool requireVersion = true;              ///< Require v=0 as first line
  bool requireOrigin = true;               ///< Require o= line
  bool requireSessionName = true;          ///< Require s= line
  bool requireTiming = false;              ///< Require at least one t= line
  std::size_t maxLineLength = 4096;        ///< Max line length (DoS prevention)
  std::size_t maxMediaSections = 64;       ///< Max m= sections (resource exhaustion prevention)

  bool operator==(const SdpParserOptions& other) const noexcept
  {
    return strict == other.strict
        && requireVersion == other.requireVersion
        && requireOrigin == other.requireOrigin
        && requireSessionName == other.requireSessionName
        && requireTiming == other.requireTiming
        && maxLineLength == other.maxLineLength
        && maxMediaSections == other.maxMediaSections;
  }

  bool operator!=(const SdpParserOptions& other) const noexcept
  {
    return !(*this == other);
  }
};

namespace detail {

inline std::string_view trimView(std::string_view s)
{
  while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
  {
    s.remove_prefix(1);
  }
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t'))
  {
    s.remove_suffix(1);
  }
  return s;
}

inline std::vector<std::string_view> splitLines(std::string_view text)
{
  std::vector<std::string_view> lines;
  std::size_t pos = 0;
  while (pos < text.size())
  {
    auto cr = text.find('\r', pos);
    auto lf = text.find('\n', pos);

    std::size_t eol;
    std::size_t next;
    if (cr != std::string_view::npos && lf == cr + 1)
    {
      eol = cr;
      next = lf + 1;
    }
    else if (lf != std::string_view::npos)
    {
      eol = lf;
      next = lf + 1;
    }
    else if (cr != std::string_view::npos)
    {
      eol = cr;
      next = cr + 1;
    }
    else
    {
      lines.push_back(text.substr(pos));
      break;
    }
    lines.push_back(text.substr(pos, eol - pos));
    pos = next;
  }
  return lines;
}

inline bool parseLineTypeValue(std::string_view line, char& type, std::string_view& value)
{
  if (line.size() < 2 || line[1] != '=')
  {
    return false;
  }
  type = line[0];
  value = line.substr(2);
  return true;
}

inline std::string_view firstToken(std::string_view s)
{
  auto sp = s.find(' ');
  return sp != std::string_view::npos ? s.substr(0, sp) : s;
}

inline std::string_view restAfterSpace(std::string_view s)
{
  auto sp = s.find(' ');
  return sp != std::string_view::npos ? s.substr(sp + 1) : std::string_view{};
}

inline std::optional<std::uint32_t> parseUint32(std::string_view s)
{
  std::uint32_t val = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc{} || ptr != s.data() + s.size())
  {
    return std::nullopt;
  }
  return val;
}

inline std::optional<std::uint16_t> parseUint16(std::string_view s)
{
  std::uint16_t val = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc{} || ptr != s.data() + s.size())
  {
    return std::nullopt;
  }
  return val;
}

inline std::optional<std::uint8_t> parseUint8(std::string_view s)
{
  std::uint8_t val = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc{} || ptr != s.data() + s.size())
  {
    return std::nullopt;
  }
  return val;
}

inline std::optional<std::uint64_t> parseUint64(std::string_view s)
{
  std::uint64_t val = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc{} || ptr != s.data() + s.size())
  {
    return std::nullopt;
  }
  return val;
}

inline std::optional<double> parseDouble(std::string_view s)
{
  if (s.empty())
  {
    return std::nullopt;
  }
  double val = 0.0;
  double frac = 0.0;
  double divisor = 1.0;
  bool hasDot = false;
  bool negative = false;
  std::size_t i = 0;
  if (s[0] == '-')
  {
    negative = true;
    ++i;
  }
  else if (s[0] == '+')
  {
    ++i;
  }
  if (i >= s.size())
  {
    return std::nullopt;
  }
  bool hasDigit = false;
  for (; i < s.size(); ++i)
  {
    if (s[i] == '.')
    {
      if (hasDot) return std::nullopt;
      hasDot = true;
    }
    else if (s[i] >= '0' && s[i] <= '9')
    {
      hasDigit = true;
      if (hasDot)
      {
        divisor *= 10.0;
        frac += static_cast<double>(s[i] - '0') / divisor;
      }
      else
      {
        val = val * 10.0 + static_cast<double>(s[i] - '0');
      }
    }
    else
    {
      return std::nullopt;
    }
  }
  if (!hasDigit)
  {
    return std::nullopt;
  }
  val += frac;
  return negative ? -val : val;
}

inline std::pair<std::string_view, std::string_view> splitAttrNameValue(std::string_view attrLine)
{
  auto colon = attrLine.find(':');
  if (colon == std::string_view::npos)
  {
    return {attrLine, {}};
  }
  return {attrLine.substr(0, colon), attrLine.substr(colon + 1)};
}

inline Origin parseOrigin(std::string_view value)
{
  Origin o;
  std::istringstream iss{std::string(value)};
  std::string username, sessId, sessVersion, netTypeStr, addrTypeStr, address;
  if (iss >> username >> sessId >> sessVersion >> netTypeStr >> addrTypeStr >> address)
  {
    o.username = username;
    o.sessId = sessId;
    o.sessVersion = sessVersion;
    if (auto nt = networkTypeFromString(netTypeStr))
    {
      o.netType = *nt;
    }
    if (auto at = addressTypeFromString(addrTypeStr))
    {
      o.addrType = *at;
    }
    o.address = address;
  }
  return o;
}

inline ConnectionData parseConnection(std::string_view value)
{
  ConnectionData c;
  std::istringstream iss{std::string(value)};
  std::string netTypeStr, addrTypeStr, address;
  if (iss >> netTypeStr >> addrTypeStr >> address)
  {
    if (auto nt = networkTypeFromString(netTypeStr))
    {
      c.netType = *nt;
    }
    if (auto at = addressTypeFromString(addrTypeStr))
    {
      c.addrType = *at;
    }
    auto slash = address.find('/');
    if (slash != std::string::npos)
    {
      auto rest = address.substr(slash + 1);
      address = address.substr(0, slash);
      auto slash2 = rest.find('/');
      if (slash2 != std::string::npos)
      {
        if (auto ttl = parseUint8(std::string_view(rest).substr(0, slash2)))
        {
          c.ttl = *ttl;
        }
        if (auto num = parseUint16(std::string_view(rest).substr(slash2 + 1)))
        {
          c.numberOfAddresses = *num;
        }
      }
      else
      {
        if (auto ttl = parseUint8(rest))
        {
          c.ttl = *ttl;
        }
      }
    }
    c.address = address;
  }
  return c;
}

inline std::optional<BandwidthInfo> parseBandwidth(std::string_view value)
{
  auto colon = value.find(':');
  if (colon == std::string_view::npos)
  {
    return std::nullopt;
  }
  auto typeStr = value.substr(0, colon);
  auto bwStr = value.substr(colon + 1);
  auto bt = bandwidthTypeFromString(typeStr);
  auto bw = parseUint32(bwStr);
  if (!bt || !bw)
  {
    return std::nullopt;
  }
  return BandwidthInfo{*bt, *bw};
}

inline RtpMapAttribute parseRtpMap(std::string_view value)
{
  RtpMapAttribute attr;
  auto sp = value.find(' ');
  if (sp == std::string_view::npos)
  {
    return attr;
  }
  if (auto pt = parseUint8(value.substr(0, sp)))
  {
    attr.payloadType = *pt;
  }
  auto rest = value.substr(sp + 1);
  auto slash1 = rest.find('/');
  if (slash1 == std::string_view::npos)
  {
    attr.encodingName = std::string(rest);
    return attr;
  }
  attr.encodingName = std::string(rest.substr(0, slash1));
  auto afterName = rest.substr(slash1 + 1);
  auto slash2 = afterName.find('/');
  if (slash2 == std::string_view::npos)
  {
    if (auto cr = parseUint32(afterName))
    {
      attr.clockRate = *cr;
    }
  }
  else
  {
    if (auto cr = parseUint32(afterName.substr(0, slash2)))
    {
      attr.clockRate = *cr;
    }
    if (auto ch = parseUint8(afterName.substr(slash2 + 1)))
    {
      attr.channels = *ch;
    }
  }
  return attr;
}

inline FmtpAttribute parseFmtp(std::string_view value)
{
  FmtpAttribute attr;
  auto sp = value.find(' ');
  if (sp == std::string_view::npos)
  {
    return attr;
  }
  if (auto pt = parseUint8(value.substr(0, sp)))
  {
    attr.payloadType = *pt;
  }
  attr.parameters = std::string(value.substr(sp + 1));

  auto params = std::string_view(attr.parameters);
  while (!params.empty())
  {
    auto semi = params.find(';');
    auto token = semi != std::string_view::npos ? params.substr(0, semi) : params;
    token = trimView(token);
    if (!token.empty())
    {
      auto eq = token.find('=');
      if (eq != std::string_view::npos)
      {
        auto key = std::string(trimView(token.substr(0, eq)));
        auto val = std::string(trimView(token.substr(eq + 1)));
        if (!key.empty())
        {
          attr.parameterMap[key] = val;
        }
      }
    }
    if (semi == std::string_view::npos)
    {
      break;
    }
    params = params.substr(semi + 1);
  }
  return attr;
}

inline RtcpFbAttribute parseRtcpFb(std::string_view value)
{
  RtcpFbAttribute attr;
  auto sp = value.find(' ');
  if (sp == std::string_view::npos)
  {
    return attr;
  }
  attr.payloadType = std::string(value.substr(0, sp));
  auto rest = value.substr(sp + 1);
  auto sp2 = rest.find(' ');
  if (sp2 == std::string_view::npos)
  {
    attr.type = std::string(rest);
  }
  else
  {
    attr.type = std::string(rest.substr(0, sp2));
    auto sub = trimView(rest.substr(sp2 + 1));
    if (!sub.empty())
    {
      attr.subtype = std::string(sub);
    }
  }
  return attr;
}

inline ExtMapAttribute parseExtMap(std::string_view value)
{
  ExtMapAttribute attr;
  auto sp = value.find(' ');
  if (sp == std::string_view::npos)
  {
    return attr;
  }
  auto idPart = value.substr(0, sp);
  auto rest = value.substr(sp + 1);

  auto slash = idPart.find('/');
  if (slash != std::string_view::npos)
  {
    if (auto id = parseUint16(idPart.substr(0, slash)))
    {
      attr.id = *id;
    }
    if (auto dir = directionFromString(idPart.substr(slash + 1)))
    {
      attr.direction = *dir;
    }
  }
  else
  {
    if (auto id = parseUint16(idPart))
    {
      attr.id = *id;
    }
  }

  auto sp2 = rest.find(' ');
  if (sp2 == std::string_view::npos)
  {
    attr.uri = std::string(rest);
  }
  else
  {
    attr.uri = std::string(rest.substr(0, sp2));
    auto ext = trimView(rest.substr(sp2 + 1));
    if (!ext.empty())
    {
      attr.extensionAttributes = std::string(ext);
    }
  }
  return attr;
}

inline SsrcAttribute parseSsrc(std::string_view value)
{
  SsrcAttribute attr;
  auto sp = value.find(' ');
  if (sp == std::string_view::npos)
  {
    return attr;
  }
  if (auto ssrc = parseUint32(value.substr(0, sp)))
  {
    attr.ssrc = *ssrc;
  }
  auto rest = value.substr(sp + 1);
  auto colon = rest.find(':');
  if (colon == std::string_view::npos)
  {
    attr.attributeName = std::string(rest);
  }
  else
  {
    attr.attributeName = std::string(rest.substr(0, colon));
    attr.attributeValue = std::string(rest.substr(colon + 1));
  }
  return attr;
}

inline SsrcGroupAttribute parseSsrcGroup(std::string_view value)
{
  SsrcGroupAttribute attr;
  std::istringstream iss{std::string(value)};
  std::string token;
  if (iss >> token)
  {
    attr.semantics = token;
  }
  while (iss >> token)
  {
    if (auto ssrc = parseUint32(token))
    {
      attr.ssrcs.push_back(*ssrc);
    }
  }
  return attr;
}

inline IceCandidate parseCandidate(std::string_view value)
{
  IceCandidate cand;
  std::istringstream iss{std::string(value)};
  std::string token;

  if (!(iss >> token)) return cand;
  cand.foundation = token;

  if (!(iss >> token)) return cand;
  if (auto c = parseUint8(token)) cand.component = *c;

  if (!(iss >> token)) return cand;
  if (auto t = iceTransportTypeFromString(token)) cand.transport = *t;

  if (!(iss >> token)) return cand;
  if (auto p = parseUint32(token)) cand.priority = *p;

  if (!(iss >> token)) return cand;
  cand.address = token;

  if (!(iss >> token)) return cand;
  if (auto p = parseUint16(token)) cand.port = *p;

  if (!(iss >> token)) return cand; // "typ"

  if (!(iss >> token)) return cand;
  if (auto t = iceCandidateTypeFromString(token)) cand.type = *t;

  while (iss >> token)
  {
    if (token == "raddr")
    {
      if (iss >> token) cand.relAddr = token;
    }
    else if (token == "rport")
    {
      if (iss >> token)
      {
        if (auto p = parseUint16(token)) cand.relPort = *p;
      }
    }
    else if (token == "tcptype")
    {
      if (iss >> token) cand.tcpType = token;
    }
    else
    {
      std::string extVal;
      if (iss >> extVal)
      {
        cand.extensions.emplace_back(token, extVal);
      }
    }
  }
  return cand;
}

inline FingerprintAttribute parseFingerprint(std::string_view value)
{
  FingerprintAttribute attr;
  auto sp = value.find(' ');
  if (sp == std::string_view::npos)
  {
    return attr;
  }
  attr.hashFunction = std::string(value.substr(0, sp));
  attr.fingerprint = std::string(trimView(value.substr(sp + 1)));
  return attr;
}

inline CryptoAttribute parseCrypto(std::string_view value)
{
  CryptoAttribute attr;
  std::istringstream iss{std::string(value)};
  std::string token;
  if (iss >> token)
  {
    if (auto t = parseUint32(token)) attr.tag = *t;
  }
  if (iss >> token) attr.suite = token;
  if (iss >> token) attr.keyParams = token;
  std::string rest;
  if (std::getline(iss >> std::ws, rest) && !rest.empty())
  {
    attr.sessionParams = rest;
  }
  return attr;
}

inline RtcpAttribute parseRtcp(std::string_view value)
{
  RtcpAttribute attr;
  std::istringstream iss{std::string(value)};
  std::string token;
  if (iss >> token)
  {
    if (auto p = parseUint16(token)) attr.port = *p;
  }
  std::string ntStr, atStr, addrStr;
  if (iss >> ntStr >> atStr >> addrStr)
  {
    if (auto nt = networkTypeFromString(ntStr)) attr.netType = *nt;
    if (auto at = addressTypeFromString(atStr)) attr.addrType = *at;
    attr.address = addrStr;
  }
  return attr;
}

inline MsidAttribute parseMsid(std::string_view value)
{
  MsidAttribute attr;
  auto sp = value.find(' ');
  if (sp == std::string_view::npos)
  {
    attr.streamId = std::string(value);
  }
  else
  {
    attr.streamId = std::string(value.substr(0, sp));
    auto track = trimView(value.substr(sp + 1));
    if (!track.empty())
    {
      attr.trackId = std::string(track);
    }
  }
  return attr;
}

inline GroupAttribute parseGroup(std::string_view value)
{
  GroupAttribute attr;
  std::istringstream iss{std::string(value)};
  std::string token;
  if (iss >> token)
  {
    attr.semantics = token;
  }
  while (iss >> token)
  {
    attr.mids.push_back(token);
  }
  return attr;
}

inline std::vector<std::vector<SimulcastStream>> parseSimulcastStreams(std::string_view str)
{
  std::vector<std::vector<SimulcastStream>> result;
  while (!str.empty())
  {
    auto semi = str.find(';');
    auto group = semi != std::string_view::npos ? str.substr(0, semi) : str;
    std::vector<SimulcastStream> alts;
    while (!group.empty())
    {
      auto comma = group.find(',');
      auto token = comma != std::string_view::npos ? group.substr(0, comma) : group;
      token = trimView(token);
      if (!token.empty())
      {
        SimulcastStream s;
        if (token[0] == '~')
        {
          s.paused = true;
          token.remove_prefix(1);
        }
        s.rid = std::string(token);
        alts.push_back(std::move(s));
      }
      if (comma == std::string_view::npos) break;
      group = group.substr(comma + 1);
    }
    if (!alts.empty())
    {
      result.push_back(std::move(alts));
    }
    if (semi == std::string_view::npos) break;
    str = str.substr(semi + 1);
  }
  return result;
}

inline SimulcastAttribute parseSimulcast(std::string_view value)
{
  SimulcastAttribute attr;
  auto v = trimView(value);
  while (!v.empty())
  {
    auto token = firstToken(v);
    auto rest = restAfterSpace(v);
    if (token == "send" && !rest.empty())
    {
      auto nextKeyword = rest.find(" recv");
      auto streams = nextKeyword != std::string_view::npos
        ? rest.substr(0, nextKeyword) : rest;
      attr.sendStreams = parseSimulcastStreams(trimView(streams));
      if (nextKeyword != std::string_view::npos)
      {
        v = rest.substr(nextKeyword + 1);
      }
      else
      {
        break;
      }
    }
    else if (token == "recv" && !rest.empty())
    {
      auto nextKeyword = rest.find(" send");
      auto streams = nextKeyword != std::string_view::npos
        ? rest.substr(0, nextKeyword) : rest;
      attr.recvStreams = parseSimulcastStreams(trimView(streams));
      if (nextKeyword != std::string_view::npos)
      {
        v = rest.substr(nextKeyword + 1);
      }
      else
      {
        break;
      }
    }
    else
    {
      break;
    }
  }
  return attr;
}

inline RidAttribute parseRid(std::string_view value)
{
  RidAttribute attr;
  auto sp = value.find(' ');
  if (sp == std::string_view::npos)
  {
    attr.id = std::string(value);
    return attr;
  }
  attr.id = std::string(value.substr(0, sp));
  auto rest = value.substr(sp + 1);

  auto sp2 = rest.find(' ');
  auto dirStr = sp2 != std::string_view::npos ? rest.substr(0, sp2) : rest;
  if (auto d = ridDirectionFromString(dirStr))
  {
    attr.direction = *d;
  }

  if (sp2 == std::string_view::npos)
  {
    return attr;
  }
  auto params = rest.substr(sp2 + 1);

  while (!params.empty())
  {
    auto semi = params.find(';');
    auto token = semi != std::string_view::npos ? params.substr(0, semi) : params;
    token = trimView(token);
    if (!token.empty())
    {
      if (token.substr(0, 3) == "pt=")
      {
        auto ptList = token.substr(3);
        while (!ptList.empty())
        {
          auto comma = ptList.find(',');
          auto ptStr = comma != std::string_view::npos ? ptList.substr(0, comma) : ptList;
          if (auto pt = parseUint8(trimView(ptStr)))
          {
            attr.payloadTypes.push_back(*pt);
          }
          if (comma == std::string_view::npos) break;
          ptList = ptList.substr(comma + 1);
        }
      }
      else
      {
        auto eq = token.find('=');
        if (eq != std::string_view::npos)
        {
          attr.restrictions[std::string(token.substr(0, eq))] = std::string(token.substr(eq + 1));
        }
      }
    }
    if (semi == std::string_view::npos) break;
    params = params.substr(semi + 1);
  }
  return attr;
}

inline void dispatchSessionAttribute(
    std::string_view name, std::string_view value,
    SessionDescription& session,
    std::vector<ParseWarning>& warnings,
    std::size_t lineNum, bool strict, std::optional<ParseError>& error)
{
  if (name == AttributeConstants::kGroup)
  {
    session.groups.push_back(parseGroup(value));
  }
  else if (name == AttributeConstants::kIceUfrag)
  {
    if (session.iceUfrag.has_value() && strict)
    {
      error = ParseError{lineNum, "Duplicate a=ice-ufrag at session level"};
      return;
    }
    if (session.iceUfrag.has_value())
    {
      warnings.push_back({lineNum, "Duplicate a=ice-ufrag at session level, overwriting"});
    }
    session.iceUfrag = std::string(value);
  }
  else if (name == AttributeConstants::kIcePwd)
  {
    if (session.icePwd.has_value() && strict)
    {
      error = ParseError{lineNum, "Duplicate a=ice-pwd at session level"};
      return;
    }
    if (session.icePwd.has_value())
    {
      warnings.push_back({lineNum, "Duplicate a=ice-pwd at session level, overwriting"});
    }
    session.icePwd = std::string(value);
  }
  else if (name == AttributeConstants::kIceOptions)
  {
    std::istringstream iss{std::string(value)};
    std::string token;
    session.iceOptions.clear();
    while (iss >> token)
    {
      session.iceOptions.push_back(token);
    }
  }
  else if (name == AttributeConstants::kIceLite)
  {
    session.iceLite = true;
  }
  else if (name == AttributeConstants::kFingerprint)
  {
    session.fingerprints.push_back(parseFingerprint(value));
  }
  else if (name == AttributeConstants::kSetup)
  {
    if (session.setup.has_value() && strict)
    {
      error = ParseError{lineNum, "Duplicate a=setup at session level"};
      return;
    }
    if (session.setup.has_value())
    {
      warnings.push_back({lineNum, "Duplicate a=setup at session level, overwriting"});
    }
    if (auto role = setupRoleFromString(value))
    {
      session.setup = *role;
    }
  }
  else if (name == AttributeConstants::kExtmapAllowMixed)
  {
    session.extmapAllowMixed = true;
  }
  else
  {
    GenericAttribute ga;
    ga.name = std::string(name);
    if (!value.empty())
    {
      ga.value = std::string(value);
    }
    session.attributes.push_back(std::move(ga));
  }
}

inline void dispatchMediaAttribute(
    std::string_view name, std::string_view value,
    MediaDescription& media,
    std::vector<ParseWarning>& warnings,
    std::size_t lineNum, bool strict, std::optional<ParseError>& error)
{
  if (name == AttributeConstants::kRtpmap)
  {
    media.rtpMaps.push_back(parseRtpMap(value));
  }
  else if (name == AttributeConstants::kFmtp)
  {
    media.fmtps.push_back(parseFmtp(value));
  }
  else if (name == AttributeConstants::kRtcpFb)
  {
    media.rtcpFeedbacks.push_back(parseRtcpFb(value));
  }
  else if (name == AttributeConstants::kExtmap)
  {
    media.extMaps.push_back(parseExtMap(value));
  }
  else if (name == AttributeConstants::kSsrc)
  {
    media.ssrcs.push_back(parseSsrc(value));
  }
  else if (name == AttributeConstants::kSsrcGroup)
  {
    media.ssrcGroups.push_back(parseSsrcGroup(value));
  }
  else if (name == AttributeConstants::kRtcp)
  {
    media.rtcp = parseRtcp(value);
  }
  else if (name == AttributeConstants::kPtime)
  {
    if (auto v = parseUint32(value)) media.ptime = *v;
  }
  else if (name == AttributeConstants::kMaxptime)
  {
    if (auto v = parseUint32(value)) media.maxptime = *v;
  }
  else if (name == AttributeConstants::kFramerate)
  {
    if (auto v = parseDouble(value)) media.framerate = *v;
  }
  else if (name == AttributeConstants::kMid)
  {
    if (media.mid.has_value() && strict)
    {
      error = ParseError{lineNum, "Duplicate a=mid"};
      return;
    }
    if (media.mid.has_value())
    {
      warnings.push_back({lineNum, "Duplicate a=mid, overwriting"});
    }
    media.mid = std::string(value);
  }
  else if (name == AttributeConstants::kMsid)
  {
    media.msid.push_back(parseMsid(value));
  }
  else if (name == AttributeConstants::kIceUfrag)
  {
    if (media.iceUfrag.has_value() && strict)
    {
      error = ParseError{lineNum, "Duplicate a=ice-ufrag"};
      return;
    }
    if (media.iceUfrag.has_value())
    {
      warnings.push_back({lineNum, "Duplicate a=ice-ufrag, overwriting"});
    }
    media.iceUfrag = std::string(value);
  }
  else if (name == AttributeConstants::kIcePwd)
  {
    if (media.icePwd.has_value() && strict)
    {
      error = ParseError{lineNum, "Duplicate a=ice-pwd"};
      return;
    }
    if (media.icePwd.has_value())
    {
      warnings.push_back({lineNum, "Duplicate a=ice-pwd, overwriting"});
    }
    media.icePwd = std::string(value);
  }
  else if (name == AttributeConstants::kIceOptions)
  {
    std::istringstream iss{std::string(value)};
    std::string token;
    media.iceOptions.clear();
    while (iss >> token)
    {
      media.iceOptions.push_back(token);
    }
  }
  else if (name == AttributeConstants::kCandidate)
  {
    media.candidates.push_back(parseCandidate(value));
  }
  else if (name == AttributeConstants::kEndOfCandidates)
  {
    media.endOfCandidates = true;
  }
  else if (name == AttributeConstants::kFingerprint)
  {
    media.fingerprints.push_back(parseFingerprint(value));
  }
  else if (name == AttributeConstants::kSetup)
  {
    if (media.setup.has_value() && strict)
    {
      error = ParseError{lineNum, "Duplicate a=setup"};
      return;
    }
    if (media.setup.has_value())
    {
      warnings.push_back({lineNum, "Duplicate a=setup, overwriting"});
    }
    if (auto role = setupRoleFromString(value))
    {
      media.setup = *role;
    }
  }
  else if (name == AttributeConstants::kCrypto)
  {
    media.crypto.push_back(parseCrypto(value));
  }
  else if (name == AttributeConstants::kRtcpMux)
  {
    media.rtcpMux = true;
  }
  else if (name == AttributeConstants::kRtcpMuxOnly)
  {
    media.rtcpMuxOnly = true;
  }
  else if (name == AttributeConstants::kRtcpRsize)
  {
    media.rtcpRsize = true;
  }
  else if (name == AttributeConstants::kExtmapAllowMixed)
  {
    media.extmapAllowMixed = true;
  }
  else if (name == AttributeConstants::kSendrecv)
  {
    media.direction = Direction::SENDRECV;
  }
  else if (name == AttributeConstants::kSendonly)
  {
    media.direction = Direction::SENDONLY;
  }
  else if (name == AttributeConstants::kRecvonly)
  {
    media.direction = Direction::RECVONLY;
  }
  else if (name == AttributeConstants::kInactive)
  {
    media.direction = Direction::INACTIVE;
  }
  else if (name == AttributeConstants::kSctpPort)
  {
    if (auto v = parseUint16(value)) media.sctpPort = *v;
  }
  else if (name == AttributeConstants::kMaxMessageSize)
  {
    if (auto v = parseUint32(value)) media.maxMessageSize = *v;
  }
  else if (name == AttributeConstants::kSimulcast)
  {
    media.simulcast = parseSimulcast(value);
  }
  else if (name == AttributeConstants::kRid)
  {
    media.rids.push_back(parseRid(value));
  }
  else
  {
    GenericAttribute ga;
    ga.name = std::string(name);
    if (!value.empty())
    {
      ga.value = std::string(value);
    }
    media.genericAttributes.push_back(std::move(ga));
  }
}

inline MediaDescription parseMediaLine(std::string_view value)
{
  MediaDescription md;
  std::istringstream iss{std::string(value)};
  std::string mediaStr, portStr, protoStr;
  if (!(iss >> mediaStr >> portStr >> protoStr))
  {
    return md;
  }

  if (auto mt = mediaTypeFromString(mediaStr))
  {
    md.mediaType = *mt;
  }

  auto slash = portStr.find('/');
  if (slash != std::string::npos)
  {
    if (auto p = parseUint16(std::string_view(portStr).substr(0, slash)))
    {
      md.port = *p;
    }
    if (auto n = parseUint16(std::string_view(portStr).substr(slash + 1)))
    {
      md.numberOfPorts = *n;
    }
  }
  else
  {
    if (auto p = parseUint16(portStr))
    {
      md.port = *p;
    }
  }

  if (auto tp = transportProtocolFromString(protoStr))
  {
    md.protocol = *tp;
  }

  std::string fmt;
  while (iss >> fmt)
  {
    md.formats.push_back(fmt);
  }
  return md;
}

inline ParseResult<SessionDescription> doParse(
    std::string_view sdpText, const SdpParserOptions& options)
{
  ParseResult<SessionDescription> result;
  result.value = SessionDescription{};
  auto& session = *result.value;

  auto lines = splitLines(sdpText);
  if (lines.empty())
  {
    result.value.reset();
    result.error = ParseError{0, "Empty SDP"};
    return result;
  }

  bool hasVersion = false;
  bool hasOrigin = false;
  bool hasSessionName = false;
  bool hasTiming = false;
  bool inMedia = false;

  for (std::size_t i = 0; i < lines.size(); ++i)
  {
    std::size_t lineNum = i + 1;
    auto line = lines[i];

    if (line.empty())
    {
      continue;
    }

    if (!options.strict)
    {
      line = trimView(line);
    }
    else if (line != trimView(line))
    {
      result.value.reset();
      result.error = ParseError{lineNum, "Leading/trailing whitespace in strict mode"};
      return result;
    }

    if (line.empty())
    {
      continue;
    }

    if (line.size() > options.maxLineLength)
    {
      if (options.strict)
      {
        result.value.reset();
        result.error = ParseError{lineNum, "Line exceeds maxLineLength"};
        return result;
      }
      result.warnings.push_back({lineNum, "Line exceeds maxLineLength, skipping"});
      continue;
    }

    char type;
    std::string_view val;
    if (!parseLineTypeValue(line, type, val))
    {
      if (options.strict)
      {
        result.value.reset();
        result.error = ParseError{lineNum, "Malformed line: missing type=value format"};
        return result;
      }
      result.warnings.push_back({lineNum, "Malformed line, skipping"});
      continue;
    }

    if (type == 'm')
    {
      if (session.mediaDescriptions.size() >= options.maxMediaSections)
      {
        result.value.reset();
        result.error = ParseError{lineNum, "Exceeded maxMediaSections limit"};
        return result;
      }
      session.mediaDescriptions.push_back(parseMediaLine(val));
      inMedia = true;
      continue;
    }

    if (inMedia)
    {
      auto& media = session.mediaDescriptions.back();
      switch (type)
      {
        case 'c':
          media.connection = parseConnection(val);
          break;
        case 'b':
          if (auto bw = parseBandwidth(val))
          {
            media.bandwidths.push_back(*bw);
          }
          break;
        case 'k':
          media.encryptionKey = std::string(val);
          break;
        case 'a':
        {
          auto [aName, aValue] = splitAttrNameValue(val);
          dispatchMediaAttribute(aName, aValue, media,
              result.warnings, lineNum, options.strict, result.error);
          if (result.error)
          {
            result.value.reset();
            return result;
          }
          break;
        }
        default:
          if (options.strict)
          {
            result.value.reset();
            result.error = ParseError{lineNum,
                std::string("Unexpected line type '") + type + "' in media section"};
            return result;
          }
          result.warnings.push_back({lineNum,
              std::string("Unexpected line type '") + type + "' in media section, skipping"});
          break;
      }
    }
    else
    {
      switch (type)
      {
        case 'v':
          hasVersion = true;
          if (auto v = parseUint8(val))
          {
            session.version = *v;
            if (options.requireVersion && *v != 0)
            {
              if (options.strict)
              {
                result.value.reset();
                result.error = ParseError{lineNum, "SDP version must be 0"};
                return result;
              }
              result.warnings.push_back({lineNum, "SDP version is not 0"});
            }
          }
          break;
        case 'o':
          hasOrigin = true;
          session.origin = parseOrigin(val);
          break;
        case 's':
          hasSessionName = true;
          session.sessionName = std::string(val);
          break;
        case 'i':
          session.sessionInfo = std::string(val);
          break;
        case 'u':
          session.uri = std::string(val);
          break;
        case 'e':
          session.emailAddress = std::string(val);
          break;
        case 'p':
          session.phoneNumber = std::string(val);
          break;
        case 'c':
          session.connection = parseConnection(val);
          break;
        case 'b':
          if (auto bw = parseBandwidth(val))
          {
            session.bandwidths.push_back(*bw);
          }
          break;
        case 't':
        {
          hasTiming = true;
          TimeDescription td;
          auto sp = val.find(' ');
          if (sp != std::string_view::npos)
          {
            if (auto st = parseUint64(val.substr(0, sp))) td.startTime = *st;
            if (auto et = parseUint64(trimView(val.substr(sp + 1)))) td.stopTime = *et;
          }
          session.timeDescriptions.push_back(td);
          break;
        }
        case 'r':
        {
          if (!session.timeDescriptions.empty())
          {
            RepeatTime rt;
            std::istringstream riss{std::string(val)};
            std::string token;
            if (riss >> token) rt.repeatInterval = token;
            if (riss >> token) rt.activeDuration = token;
            while (riss >> token) rt.offsets.push_back(token);
            session.timeDescriptions.back().repeatTimes.push_back(std::move(rt));
          }
          break;
        }
        case 'z':
        {
          std::istringstream ziss{std::string(val)};
          std::string timeStr, offsetStr;
          while (ziss >> timeStr >> offsetStr)
          {
            session.zoneAdjustments.push_back({timeStr, offsetStr});
          }
          break;
        }
        case 'k':
          session.encryptionKey = std::string(val);
          break;
        case 'a':
        {
          auto [aName, aValue] = splitAttrNameValue(val);
          dispatchSessionAttribute(aName, aValue, session,
              result.warnings, lineNum, options.strict, result.error);
          if (result.error)
          {
            result.value.reset();
            return result;
          }
          break;
        }
        default:
          if (options.strict)
          {
            result.value.reset();
            result.error = ParseError{lineNum,
                std::string("Unexpected line type '") + type + "'"};
            return result;
          }
          result.warnings.push_back({lineNum,
              std::string("Unexpected line type '") + type + "', skipping"});
          break;
      }
    }
  }

  if (options.requireVersion && !hasVersion)
  {
    result.value.reset();
    result.error = ParseError{0, "Missing required v= line"};
    return result;
  }
  if (options.requireOrigin && !hasOrigin)
  {
    result.value.reset();
    result.error = ParseError{0, "Missing required o= line"};
    return result;
  }
  if (options.requireSessionName && !hasSessionName)
  {
    result.value.reset();
    result.error = ParseError{0, "Missing required s= line"};
    return result;
  }
  if (options.requireTiming && !hasTiming)
  {
    result.value.reset();
    result.error = ParseError{0, "Missing required t= line"};
    return result;
  }

  return result;
}

} // namespace detail

/// SDP text parser. All methods are static; the class is not instantiable.
struct SdpParser
{
  SdpParser() = delete;

  /// Parse SDP text with default options.
  static ParseResult<SessionDescription> parse(std::string_view sdpText)
  {
    return detail::doParse(sdpText, SdpParserOptions{});
  }

  /// Parse SDP text with explicit options.
  static ParseResult<SessionDescription> parse(
      std::string_view sdpText, const SdpParserOptions& options)
  {
    return detail::doParse(sdpText, options);
  }

  /// Parse a single media section (m= line and subsequent lines).
  static ParseResult<MediaDescription> parseMediaSection(std::string_view text)
  {
    ParseResult<MediaDescription> result;
    auto lines = detail::splitLines(text);
    if (lines.empty())
    {
      result.error = ParseError{0, "Empty media section"};
      return result;
    }

    char type;
    std::string_view val;
    if (!detail::parseLineTypeValue(lines[0], type, val) || type != 'm')
    {
      result.error = ParseError{1, "First line is not an m= line"};
      return result;
    }

    auto media = detail::parseMediaLine(val);
    std::optional<ParseError> dispatchError;

    for (std::size_t i = 1; i < lines.size(); ++i)
    {
      auto line = detail::trimView(lines[i]);
      if (line.empty()) continue;

      if (!detail::parseLineTypeValue(line, type, val))
      {
        result.warnings.push_back({i + 1, "Malformed line, skipping"});
        continue;
      }

      switch (type)
      {
        case 'c':
          media.connection = detail::parseConnection(val);
          break;
        case 'b':
          if (auto bw = detail::parseBandwidth(val))
          {
            media.bandwidths.push_back(*bw);
          }
          break;
        case 'i':
          result.warnings.push_back({i + 1, "Media-level i= line not stored, skipping"});
          break;
        case 'k':
          media.encryptionKey = std::string(val);
          break;
        case 'a':
        {
          auto [aName, aValue] = detail::splitAttrNameValue(val);
          detail::dispatchMediaAttribute(aName, aValue, media,
              result.warnings, i + 1, false, dispatchError);
          if (dispatchError)
          {
            result.error = std::move(dispatchError);
            return result;
          }
          break;
        }
        default:
          result.warnings.push_back({i + 1,
              std::string("Unexpected line type '") + type + "', skipping"});
          break;
      }
    }

    result.value = std::move(media);
    return result;
  }

  /// Parse a single a= line into a GenericAttribute (name + optional value).
  static GenericAttribute parseAttribute(std::string_view line)
  {
    auto trimmed = detail::trimView(line);
    if (trimmed.size() >= 2 && trimmed[0] == 'a' && trimmed[1] == '=')
    {
      trimmed = trimmed.substr(2);
    }
    auto [name, value] = detail::splitAttrNameValue(trimmed);
    GenericAttribute ga;
    ga.name = std::string(name);
    if (!value.empty())
    {
      ga.value = std::string(value);
    }
    return ga;
  }
};

} // namespace sdp
} // namespace iora

#pragma once

/// @file offer_answer.hpp
/// @brief Stateless offer/answer utility functions for SDP operations.

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "iora/sdp/session_description.hpp"

namespace iora {
namespace sdp {

/// Options for creating an SDP offer.
struct OfferOptions
{
  bool audio = true;
  bool video = true;
  bool dataChannel = false;
  std::optional<std::string> iceUfrag;
  std::optional<std::string> icePwd;
  std::optional<FingerprintAttribute> fingerprint;
  bool bundlePolicy = true;
  bool rtcpMux = true;
  SetupRole dtlsSetup = SetupRole::ACTPASS;

  bool operator==(const OfferOptions& other) const noexcept
  {
    return audio == other.audio
        && video == other.video
        && dataChannel == other.dataChannel
        && iceUfrag == other.iceUfrag
        && icePwd == other.icePwd
        && fingerprint == other.fingerprint
        && bundlePolicy == other.bundlePolicy
        && rtcpMux == other.rtcpMux
        && dtlsSetup == other.dtlsSetup;
  }

  bool operator!=(const OfferOptions& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Options for creating an SDP answer.
struct AnswerOptions
{
  std::optional<std::string> iceUfrag;
  std::optional<std::string> icePwd;
  std::optional<FingerprintAttribute> fingerprint;
  SetupRole dtlsSetup = SetupRole::ACTIVE;
  /// The answerer's OWN media capabilities. When set, createAnswer negotiates
  /// against the offer and emits the ANSWERER's rtpMaps / fmtps / rtcpFeedbacks
  /// (intersected with the offer) plus the URI-intersected extMaps — never a
  /// verbatim echo of the offerer's attributes (RFC 3264 §6; RFC 7587 §7.1).
  std::optional<MediaDescription> supportedMedia;
  std::vector<MediaType> rejectMediaTypes;

  bool operator==(const AnswerOptions& other) const noexcept
  {
    return iceUfrag == other.iceUfrag
        && icePwd == other.icePwd
        && fingerprint == other.fingerprint
        && dtlsSetup == other.dtlsSetup
        && supportedMedia == other.supportedMedia
        && rejectMediaTypes == other.rejectMediaTypes;
  }

  bool operator!=(const AnswerOptions& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Options for creating a transceiver media section.
struct TransceiverOptions
{
  std::string mid;
  Direction direction = Direction::SENDRECV;
  TransportProtocol protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
  std::optional<std::string> iceUfrag;
  std::optional<std::string> icePwd;
  std::optional<FingerprintAttribute> fingerprint;
  SetupRole setup = SetupRole::ACTPASS;
  std::optional<MsidAttribute> msid;

  bool operator==(const TransceiverOptions& other) const noexcept
  {
    return mid == other.mid
        && direction == other.direction
        && protocol == other.protocol
        && iceUfrag == other.iceUfrag
        && icePwd == other.icePwd
        && fingerprint == other.fingerprint
        && setup == other.setup
        && msid == other.msid;
  }

  bool operator!=(const TransceiverOptions& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Options for creating a data channel media section.
struct DataChannelOptions
{
  std::string mid;
  std::uint16_t sctpPort = 5000;
  std::uint32_t maxMessageSize = 262144;
  std::optional<std::string> iceUfrag;
  std::optional<std::string> icePwd;
  std::optional<FingerprintAttribute> fingerprint;
  SetupRole setup = SetupRole::ACTPASS;

  bool operator==(const DataChannelOptions& other) const noexcept
  {
    return mid == other.mid
        && sctpPort == other.sctpPort
        && maxMessageSize == other.maxMessageSize
        && iceUfrag == other.iceUfrag
        && icePwd == other.icePwd
        && fingerprint == other.fingerprint
        && setup == other.setup;
  }

  bool operator!=(const DataChannelOptions& other) const noexcept
  {
    return !(*this == other);
  }
};

/// Stateless offer/answer utility functions for SDP operations.
/// All methods are static — the struct is non-instantiable.
struct OfferAnswer
{
  OfferAnswer() = delete;

  /// Flip an SDP direction for offer/answer per RFC 3264.
  static Direction flipDirection(Direction direction) noexcept
  {
    switch (direction)
    {
      case Direction::SENDRECV: return Direction::SENDRECV;
      case Direction::SENDONLY: return Direction::RECVONLY;
      case Direction::RECVONLY: return Direction::SENDONLY;
      case Direction::INACTIVE: return Direction::INACTIVE;
    }
    return Direction::SENDRECV;
  }

  /// Reverse an a=extmap direction qualifier for the answer (RFC 8285 §7:
  /// sendonly<->recvonly; sendrecv/inactive unchanged; absent stays absent ==
  /// sendrecv). Reuses flipDirection — no second direction reversal.
  static std::optional<Direction> flipExtmapDirection(std::optional<Direction> d) noexcept
  {
    if (!d.has_value())
    {
      return std::nullopt;
    }
    return flipDirection(*d);
  }

  /// Intersect two direction qualifiers as send/recv capability sets (absent ==
  /// sendrecv). Returns {keep, direction}: keep=false when the intersection is
  /// empty (no shared send or recv) — the caller drops the extension (RFC 8285
  /// §7). A full sendrecv result is reported as absent (nullopt) so the answer
  /// omits the redundant qualifier (RFC 8285 §5 default).
  static std::pair<bool, std::optional<Direction>>
  intersectDirections(std::optional<Direction> a, std::optional<Direction> b) noexcept
  {
    auto caps = [](std::optional<Direction> d) -> std::pair<bool, bool>
    {
      switch (d.value_or(Direction::SENDRECV))
      {
        case Direction::SENDRECV: return {true, true};
        case Direction::SENDONLY: return {true, false};
        case Direction::RECVONLY: return {false, true};
        case Direction::INACTIVE: return {false, false};
      }
      return {true, true};
    };
    const auto ca = caps(a);
    const auto cb = caps(b);
    const bool send = ca.first && cb.first;
    const bool recv = ca.second && cb.second;
    if (send && recv) { return {true, std::nullopt}; }
    if (send) { return {true, Direction::SENDONLY}; }
    if (recv) { return {true, Direction::RECVONLY}; }
    return {false, std::nullopt};
  }

  /// True when `media` advertises rtcp-fb (type, subtype) for payload type `pt`,
  /// counting a wildcard `a=rtcp-fb:* ...` entry as applying to every PT
  /// (RFC 4585 §5.1).
  static bool hasRtcpFb(const MediaDescription& media, std::uint8_t pt,
                        const std::string& type,
                        const std::optional<std::string>& subtype)
  {
    const std::string ptStr = std::to_string(pt);
    for (const auto& fb : media.rtcpFeedbacks)
    {
      if ((fb.payloadType == ptStr || fb.payloadType == "*")
          && fb.type == type && fb.subtype == subtype)
      {
        return true;
      }
    }
    return false;
  }

  /// True when `media` advertises rtcp-fb (type, subtype) as an explicit
  /// wildcard `*` entry (RFC 4585 §5.1).
  static bool hasRtcpFbWildcard(const MediaDescription& media,
                                const std::string& type,
                                const std::optional<std::string>& subtype)
  {
    for (const auto& fb : media.rtcpFeedbacks)
    {
      if (fb.payloadType == "*" && fb.type == type && fb.subtype == subtype)
      {
        return true;
      }
    }
    return false;
  }

  /// Build the answer's a=extmap set for one media section (RFC 8285 §7).
  /// Shared by the matched and mirror createAnswer branches. `offerStreamDir` is
  /// the offer m= section's effective direction: an extmap with NO direction
  /// qualifier inherits it (RFC 8285 §7), so it is resolved BEFORE the flip.
  /// The offer's numeric id is always reused (ids MUST NOT be remapped). When
  /// `answererMedia` is null (capability-blind fallback) every offered extension
  /// is kept, direction = flipped effective; otherwise the set is intersected by
  /// CASE-SENSITIVE URI (RFC 3986 — exact ==, never detail::iequals) and the
  /// direction is flip(offer) ∩ answerer, dropping an incompatible extension.
  /// A resolved direction equal to the answer's stream direction is emitted as
  /// absent (RFC 8285 §5 — the qualifier then inherits the stream direction).
  static std::vector<ExtMapAttribute>
  answerExtMaps(const MediaDescription& offerMedia, Direction offerStreamDir,
                const MediaDescription* answererMedia)
  {
    const Direction answerStreamDir = flipDirection(offerStreamDir);
    std::vector<ExtMapAttribute> result;
    for (const auto& oext : offerMedia.extMaps)
    {
      const Direction offerEff = oext.direction.value_or(offerStreamDir);
      const Direction flipped = flipDirection(offerEff);

      std::optional<Direction> outDir;
      if (answererMedia == nullptr)
      {
        outDir = flipped;
      }
      else
      {
        const ExtMapAttribute* aext = nullptr;
        for (const auto& ae : answererMedia->extMaps)
        {
          if (ae.uri == oext.uri)
          {
            aext = &ae;
            break;
          }
        }
        if (aext == nullptr)
        {
          continue;
        }
        const auto dir = intersectDirections(flipped, aext->direction);
        if (!dir.first)
        {
          continue;
        }
        outDir = dir.second; // nullopt == sendrecv
      }
      // Emit absent when it matches the stream direction the extmap would
      // otherwise inherit (keeps the wire minimal; RFC 8285 §5).
      if (outDir.has_value() && *outDir == answerStreamDir)
      {
        outDir = std::nullopt;
      }
      ExtMapAttribute out = oext;
      out.direction = outDir;
      result.push_back(std::move(out));
    }
    return result;
  }

  /// Set the direction attribute of a media section.
  static void setMediaDirection(MediaDescription& media, Direction direction) noexcept
  {
    media.direction = direction;
  }

  /// Disable a media section by setting port to 0 and direction to INACTIVE.
  static void disableMedia(MediaDescription& media) noexcept
  {
    media.port = 0;
    media.direction = Direction::INACTIVE;
  }

  /// Check if a media section is disabled (port == 0).
  static bool isMediaDisabled(const MediaDescription& media) noexcept
  {
    return media.port == 0;
  }

  /// Find a media section by its a=mid value. Returns nullptr if not found.
  static MediaDescription* findMediaByMid(SessionDescription& session,
                                           const std::string& mid) noexcept
  {
    for (auto& media : session.mediaDescriptions)
    {
      if (media.mid.has_value() && media.mid.value() == mid)
      {
        return &media;
      }
    }
    return nullptr;
  }

  /// Find a media section by its a=mid value (const overload).
  static const MediaDescription* findMediaByMid(const SessionDescription& session,
                                                  const std::string& mid) noexcept
  {
    for (const auto& media : session.mediaDescriptions)
    {
      if (media.mid.has_value() && media.mid.value() == mid)
      {
        return &media;
      }
    }
    return nullptr;
  }

  /// Generate random ICE credentials per RFC 8445 section 5.3.
  static std::pair<std::string, std::string> generateIceCredentials()
  {
    static constexpr char kChars[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+/";
    static constexpr std::size_t kCharCount = sizeof(kChars) - 1;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> dist(0, kCharCount - 1);

    std::string ufrag(8, '\0');
    for (auto& c : ufrag)
    {
      c = kChars[dist(gen)];
    }

    std::string pwd(24, '\0');
    for (auto& c : pwd)
    {
      c = kChars[dist(gen)];
    }

    return {std::move(ufrag), std::move(pwd)};
  }

  /// Append a trickle ICE candidate to a media section.
  static void addIceCandidate(MediaDescription& media, const IceCandidate& candidate)
  {
    media.candidates.push_back(candidate);
  }

  /// Clear all ICE candidates from a media section.
  static void removeIceCandidates(MediaDescription& media) noexcept
  {
    media.candidates.clear();
    media.endOfCandidates = false;
  }

  /// Parse an m= format token to a payload type (0-127), non-throwing.
  /// Returns std::nullopt for non-numeric tokens (e.g. "webrtc-datachannel"),
  /// out-of-range values, or over-long digit strings. Uses std::from_chars so
  /// untrusted SDP can never throw std::out_of_range (unlike std::stoul).
  static std::optional<std::uint8_t> formatToPayloadType(const std::string& s) noexcept
  {
    // std::from_chars on an empty range returns errc::invalid_argument, which the
    // ec check below rejects — no separate empty guard needed.
    unsigned value = 0;
    const char* begin = s.data();
    const char* end = begin + s.size();
    auto res = std::from_chars(begin, end, value);
    if (res.ec != std::errc() || res.ptr != end || value > 127)
    {
      return std::nullopt;
    }
    return static_cast<std::uint8_t>(value);
  }

  /// The implied a=rtpmap for a payload type that arrived WITHOUT one, for the
  /// RFC 3551 static assignments iora negotiates. RFC 4566 section 6 makes
  /// a=rtpmap optional for statically-assigned PTs; real endpoints (CUCM, older
  /// Avaya/Polycom, carrier SBCs) omit it. Scope is bounded to the codecs
  /// iora_media decodes — the authority for this set is the iora_media decoder
  /// registry (payload_format_registry). G.729 (18) and CN (13) are deliberately
  /// excluded (no G.729 decoder; CN is not a media codec, RFC 3389). channels is
  /// left unset so a synthesized entry is byte-identical to a parsed bare
  /// "PCMU/8000" (RFC 4566 section 6: an omitted channel count means one).
  /// G.722 (9) uses the RFC 3551 section 4.5.2 8000 RTP clock despite 16 kHz
  /// sampling. See tracker 2026-07-23-2 design decision D4.
  /// Not noexcept: constructs a std::string encodingName (SSO-safe for these
  /// literals, but the return type owns heap storage in general).
  static std::optional<RtpMapAttribute> impliedStaticRtpMap(std::uint8_t pt)
  {
    switch (pt)
    {
      case 0: return RtpMapAttribute{0, "PCMU", 8000, std::nullopt};
      case 8: return RtpMapAttribute{8, "PCMA", 8000, std::nullopt};
      case 9: return RtpMapAttribute{9, "G722", 8000, std::nullopt};
      default: return std::nullopt;
    }
  }

  /// Return the media section's rtpMaps AUGMENTED with the implied rtpmap for
  /// each rtpmap-less RFC 3551 static PT listed on the m= line (never a
  /// formats-only projection that discards rtpMaps). Non-mutating: the caller's
  /// MediaDescription is untouched. Two-pass union (tracker 2026-07-23-2 C-1):
  ///  - PASS 1 walks media.formats IN ORDER so the m= fmt list drives preference
  ///    order; a listed PT contributes its existing rtpMap entry if present, else
  ///    a synthesized static entry, else (dynamic/unknown with no rtpmap) nothing.
  ///  - PASS 2 appends every rtpMaps entry whose PT the fmt list did not cover
  ///    (formats-less operands such as the createAnswer answerer or a
  ///    reconstructed local offer, and orphan rtpmaps) so no entry is ever
  ///    dropped and the legacy join-all-rtpMaps behavior is preserved.
  /// Duplicate-PT rtpmaps in a malformed SDP are deduped by first occurrence.
  static std::vector<RtpMapAttribute> normalizeStaticFormats(const MediaDescription& media)
  {
    std::vector<RtpMapAttribute> result;
    result.reserve(media.rtpMaps.size() + media.formats.size());
    std::vector<std::uint8_t> seen;

    // The RFC 3551 static assignments 0/8/9 are AUDIO. Only synthesize for an
    // audio m= line — a malformed video/application line listing 0/8/9 without
    // an a=rtpmap must not gain a phantom PCMU/PCMA/G722 entry. Existing rtpMaps
    // are still preserved for every media type (the union passes below).
    const bool synthesizeStatics = (media.mediaType == MediaType::AUDIO);

    auto isSeen = [&](std::uint8_t pt)
    {
      return std::find(seen.begin(), seen.end(), pt) != seen.end();
    };
    auto findRtpMap = [&](std::uint8_t pt) -> const RtpMapAttribute*
    {
      for (const auto& r : media.rtpMaps)
      {
        if (r.payloadType == pt)
        {
          return &r;
        }
      }
      return nullptr;
    };

    for (const auto& token : media.formats)
    {
      auto pt = formatToPayloadType(token);
      if (!pt.has_value() || isSeen(*pt))
      {
        continue;
      }
      if (const RtpMapAttribute* existing = findRtpMap(*pt))
      {
        result.push_back(*existing);
        seen.push_back(*pt);
      }
      else if (synthesizeStatics)
      {
        if (auto implied = impliedStaticRtpMap(*pt))
        {
          result.push_back(*implied);
          seen.push_back(*pt);
        }
      }
    }

    for (const auto& r : media.rtpMaps)
    {
      if (!isSeen(r.payloadType))
      {
        result.push_back(r);
        seen.push_back(r.payloadType);
      }
    }
    return result;
  }

  /// Intersect codec lists between offer and answer media sections.
  /// Returns codecs present in both, ordered by the answerer's preference.
  /// Both operands are first normalised (normalizeStaticFormats) so a
  /// conformant rtpmap-less static offer still negotiates (RFC 4566 section 6).
  static std::vector<RtpMapAttribute> matchCodecs(const MediaDescription& offer,
                                                    const MediaDescription& answer)
  {
    const std::vector<RtpMapAttribute> offerCodecs = normalizeStaticFormats(offer);
    const std::vector<RtpMapAttribute> answerCodecs = normalizeStaticFormats(answer);

    std::vector<RtpMapAttribute> result;
    for (const auto& answerCodec : answerCodecs)
    {
      for (const auto& offerCodec : offerCodecs)
      {
        if (detail::iequals(answerCodec.encodingName, offerCodec.encodingName)
            && answerCodec.clockRate == offerCodec.clockRate)
        {
          result.push_back(answerCodec);
          break;
        }
      }
    }
    return result;
  }

  /// Remove rtpmap, fmtp, and rtcp-fb entries for payload types not in the retain list.
  static void pruneUnusedCodecs(MediaDescription& media,
                                 const std::vector<std::uint8_t>& retainPayloadTypes)
  {
    auto isRetained = [&](std::uint8_t pt)
    {
      return std::find(retainPayloadTypes.begin(), retainPayloadTypes.end(), pt)
          != retainPayloadTypes.end();
    };

    // Non-throwing parse (formatToPayloadType uses std::from_chars): an
    // over-long all-digit fmt/rtcp-fb token from untrusted SDP can no longer
    // throw std::out_of_range out of pruneUnusedCodecs (tracker 2026-07-23-2
    // task 1.4).
    auto toPayloadType = [](const std::string& s) { return formatToPayloadType(s); };

    media.rtpMaps.erase(
      std::remove_if(media.rtpMaps.begin(), media.rtpMaps.end(),
        [&](const RtpMapAttribute& r) { return !isRetained(r.payloadType); }),
      media.rtpMaps.end());

    media.fmtps.erase(
      std::remove_if(media.fmtps.begin(), media.fmtps.end(),
        [&](const FmtpAttribute& f) { return !isRetained(f.payloadType); }),
      media.fmtps.end());

    media.rtcpFeedbacks.erase(
      std::remove_if(media.rtcpFeedbacks.begin(), media.rtcpFeedbacks.end(),
        [&](const RtcpFbAttribute& fb)
        {
          if (fb.payloadType == "*")
          {
            return false;
          }
          auto pt = toPayloadType(fb.payloadType);
          if (!pt.has_value())
          {
            return false;
          }
          return !isRetained(pt.value());
        }),
      media.rtcpFeedbacks.end());

    media.formats.erase(
      std::remove_if(media.formats.begin(), media.formats.end(),
        [&](const std::string& fmt)
        {
          auto pt = toPayloadType(fmt);
          if (!pt.has_value())
          {
            return false;
          }
          return !isRetained(pt.value());
        }),
      media.formats.end());
  }

  /// Create a MediaDescription with WebRTC defaults.
  static MediaDescription addTransceiver(MediaType type, const TransceiverOptions& options)
  {
    MediaDescription media;
    media.mediaType = type;
    media.port = 9;
    media.protocol = options.protocol;
    media.direction = options.direction;
    media.mid = options.mid;
    media.rtcpMux = true;
    media.setup = options.setup;

    if (options.iceUfrag.has_value())
    {
      media.iceUfrag = options.iceUfrag;
    }
    if (options.icePwd.has_value())
    {
      media.icePwd = options.icePwd;
    }
    if (options.fingerprint.has_value())
    {
      media.fingerprints.push_back(options.fingerprint.value());
    }
    if (options.msid.has_value())
    {
      media.msid.push_back(options.msid.value());
    }

    return media;
  }

  /// Create a MediaDescription for SCTP data channels per RFC 8841.
  static MediaDescription addDataChannel(const DataChannelOptions& options)
  {
    MediaDescription media;
    media.mediaType = MediaType::APPLICATION;
    media.port = 9;
    media.protocol = TransportProtocol::UDP_DTLS_SCTP;
    media.formats = {"webrtc-datachannel"};
    media.direction = Direction::SENDRECV;
    media.mid = options.mid;
    media.sctpPort = options.sctpPort;
    media.maxMessageSize = options.maxMessageSize;
    media.setup = options.setup;

    if (options.iceUfrag.has_value())
    {
      media.iceUfrag = options.iceUfrag;
    }
    if (options.icePwd.has_value())
    {
      media.icePwd = options.icePwd;
    }
    if (options.fingerprint.has_value())
    {
      media.fingerprints.push_back(options.fingerprint.value());
    }

    return media;
  }

  /// Scaffold a new SessionDescription for a WebRTC or SIP offer.
  ///
  /// The connection address is the caller's to supply: no c= is set at either
  /// level, so the result is incomplete per RFC 4566 §5.7 until the caller
  /// assigns one (see SessionDescription::effectiveConnection).
  ///
  /// Likewise the RTP media sections (audio/video) are scaffolded WITHOUT any
  /// formats/rtpMaps: the caller MUST populate each active RTP section's `formats`
  /// (and matching `a=rtpmap` for dynamic PTs) before serializing. The data-channel
  /// section is exempt — it is self-complete (`formats = {"webrtc-datachannel"}`).
  /// If an active RTP section is serialized with an empty formats list, the
  /// serializer renders it as a REJECTED stream (port 0) rather than advertising a
  /// placeholder codec (RFC 3264 §5.1/§8.2/§6) — a SILENT coercion, since
  /// serialize() gives no error signal. In particular, serializing an unpopulated
  /// createOffer() with `bundlePolicy = true` (the default) yields a NON-CONFORMANT
  /// BUNDLE offer (RFC 8843 §7.2.1: the suggested offerer-tagged m= section ends up
  /// zero-port). Contrast with
  /// createAnswer, whose contract differs: it ECHOES the offer's fmt list rather
  /// than requiring the caller to populate — but feeding it an unpopulated
  /// createOffer scaffold propagates the empty formats and is likewise rendered
  /// rejected.
  static SessionDescription createOffer(const OfferOptions& options = {})
  {
    SessionDescription session;
    session.version = 0;

    {
      std::random_device rd;
      std::mt19937_64 gen(rd());
      std::uniform_int_distribution<std::uint64_t> dist;
      session.origin.sessId = std::to_string(dist(gen));
    }
    session.origin.sessVersion = "1";

    session.sessionName = "-";

    session.timeDescriptions.push_back({0, 0, {}});

    if (options.iceUfrag.has_value() && options.icePwd.has_value())
    {
      session.iceUfrag = options.iceUfrag;
      session.icePwd = options.icePwd;
    }
    else
    {
      auto creds = generateIceCredentials();
      session.iceUfrag = std::move(creds.first);
      session.icePwd = std::move(creds.second);
    }

    if (options.fingerprint.has_value())
    {
      session.fingerprints.push_back(options.fingerprint.value());
    }

    session.setup = options.dtlsSetup;
    session.iceOptions = {"trickle"};

    int midIndex = 0;

    if (options.audio)
    {
      MediaDescription audio;
      audio.mediaType = MediaType::AUDIO;
      audio.port = 9;
      audio.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
      audio.direction = Direction::SENDRECV;
      audio.mid = std::to_string(midIndex++);
      audio.rtcpMux = options.rtcpMux;
      session.mediaDescriptions.push_back(std::move(audio));
    }

    if (options.video)
    {
      MediaDescription video;
      video.mediaType = MediaType::VIDEO;
      video.port = 9;
      video.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
      video.direction = Direction::SENDRECV;
      video.mid = std::to_string(midIndex++);
      video.rtcpMux = options.rtcpMux;
      session.mediaDescriptions.push_back(std::move(video));
    }

    if (options.dataChannel)
    {
      DataChannelOptions dcOpts;
      dcOpts.mid = std::to_string(midIndex++);
      session.mediaDescriptions.push_back(addDataChannel(dcOpts));
    }

    if (options.bundlePolicy && !session.mediaDescriptions.empty())
    {
      GroupAttribute bundle;
      bundle.semantics = "BUNDLE";
      for (const auto& media : session.mediaDescriptions)
      {
        if (media.mid.has_value())
        {
          bundle.mids.push_back(media.mid.value());
        }
      }
      session.groups.push_back(std::move(bundle));
    }

    return session;
  }

  /// Generate an SDP answer from a received offer.
  ///
  /// As with createOffer, no c= is set at either level — the caller supplies
  /// the answer's connection address (RFC 4566 §5.7).
  static SessionDescription createAnswer(const SessionDescription& offer,
                                          const AnswerOptions& answerOptions = {})
  {
    SessionDescription session;
    session.version = 0;

    {
      std::random_device rd;
      std::mt19937_64 gen(rd());
      std::uniform_int_distribution<std::uint64_t> dist;
      session.origin.sessId = std::to_string(dist(gen));
    }
    session.origin.sessVersion = "1";

    session.sessionName = "-";

    session.timeDescriptions.push_back({0, 0, {}});

    if (answerOptions.iceUfrag.has_value() && answerOptions.icePwd.has_value())
    {
      session.iceUfrag = answerOptions.iceUfrag;
      session.icePwd = answerOptions.icePwd;
    }
    else
    {
      auto creds = generateIceCredentials();
      session.iceUfrag = std::move(creds.first);
      session.icePwd = std::move(creds.second);
    }

    if (answerOptions.fingerprint.has_value())
    {
      session.fingerprints.push_back(answerOptions.fingerprint.value());
    }

    session.setup = answerOptions.dtlsSetup;
    session.iceOptions = {"trickle"};

    // createAnswer is deliberately NON-BUNDLE-AWARE: it does NOT copy the offer's
    // a=group attributes into the answer. Echoing a=group:BUNDLE would name the
    // mids of sections this function rejects at port 0 (RFC 8843 §7.3.3(a) MUST
    // NOT), and echoing a=group:LS would assert lip-sync across a port-0/INACTIVE
    // rejected stream; more fundamentally this function implements none of RFC
    // 8843 §7.3's positive answerer obligations (no answerer-tagged section, no
    // a=bundle-only, every accepted section gets its own port), so advertising
    // any a=group would be a false claim. Per RFC 5888 §9.2 an answerer that does
    // not support a group's semantics returns an answer WITHOUT the group line
    // (mids are retained). A non-BUNDLE answer (no a=group, per-m= ports) is the
    // RFC 8843 §1.2 backward-compat baseline and is fully §7.3-conformant. A
    // BUNDLE/WebRTC consumer must NOT route BUNDLE offers through this generic
    // primitive (see tasks 2026-07-25-2 disposition C, 2026-09-02-5/6).

    // A port-0 INACTIVE reject that echoes the offer's <media>/<proto> (incl. the
    // raw wire token for an UNKNOWN profile — empty for a recognized one), its fmt
    // list, and mid. The fmt list is copied verbatim: usually >=1 (RFC 4566 §5.14),
    // but a fmt-less port-0 removal offer legitimately carries zero — the
    // serializer supplies the mandatory placeholder fmt on output. Shared by the
    // UNKNOWN fail-safe and the normal decline: both produce the identical
    // rejected-stream wire shape (RFC 3264 §6/§8.2).
    const auto rejectEcho = [](const MediaDescription& off)
    {
      MediaDescription a;
      a.mediaType = off.mediaType;
      a.rawMediaType = off.rawMediaType;
      a.protocol = off.protocol;
      a.rawProtocol = off.rawProtocol;
      a.port = 0;
      a.formats = off.formats;
      a.direction = Direction::INACTIVE;
      a.mid = off.mid;
      return a;
    };

    // Indexed loop so the offer's effective (session-level-folded) direction can
    // be resolved per media via offer.effectiveDirection(i).
    for (std::size_t i = 0; i < offer.mediaDescriptions.size(); ++i)
    {
      const auto& offerMedia = offer.mediaDescriptions[i];
      MediaDescription answerMedia;

      // FAIL-SAFE: an offer whose <media> or <proto> token is unrecognized/
      // malformed (UNKNOWN) is rejected with port 0 BEFORE the declined/
      // APPLICATION/RTP dispatch — no arm may accept (port=9) a media type or
      // transport profile we cannot faithfully model. Placed pre-dispatch so it
      // covers the APPLICATION arm too (RFC 3264 §6/§8.2: reject an unsupported
      // stream with port 0). Echo the exact wire token so the rejected m= line
      // round-trips (never a "" toString).
      if (offerMedia.hasUnknownProfile())
      {
        session.mediaDescriptions.push_back(rejectEcho(offerMedia));
        continue;
      }

      // FAIL-SAFE (2026-09-01-1 PART 2d): a RECOGNIZED but unmodelable MEDIA type
      // (T.38 fax, m=image) is likewise port-0-rejected before dispatch. Recognizing
      // IMAGE (types.hpp) removes it from the hasUnknownProfile() guard above, so
      // without this clause an m=image offer would fall through to the RTP else-arm
      // and be accepted at port 9 — a fax stream this SDP layer cannot answer. This
      // is a MEDIA-type reject ONLY: it inspects no <proto>, so createAnswer stays
      // transport-profile-agnostic and keeps answering DTLS-SRTP (UDP/TLS/RTP/SAVP[F])
      // with a fingerprint (that serviceability decision belongs to the media plane,
      // not this generic primitive).
      if (offerMedia.mediaType == MediaType::IMAGE)
      {
        session.mediaDescriptions.push_back(rejectEcho(offerMedia));
        continue;
      }

      // A section is DECLINED when offered with port 0 (RFC 3264 §6/§8.2 — the
      // answer MUST also be port 0) or when its media type is locally rejected.
      // The port-0 test is first so it short-circuits the reject-list search.
      // A declined answer echoes the offer's media/proto and keeps >=1 format
      // token (RFC 4566 m= grammar), is marked INACTIVE, and dominates any
      // inherited session-level direction.
      const bool declined =
        isMediaDisabled(offerMedia)
        || std::find(answerOptions.rejectMediaTypes.begin(),
                     answerOptions.rejectMediaTypes.end(),
                     offerMedia.mediaType)
             != answerOptions.rejectMediaTypes.end();

      if (declined)
      {
        answerMedia = rejectEcho(offerMedia);
      }
      else if (offerMedia.mediaType == MediaType::APPLICATION)
      {
        answerMedia.mediaType = MediaType::APPLICATION;
        answerMedia.port = 9;
        answerMedia.protocol = offerMedia.protocol;
        answerMedia.formats = offerMedia.formats;
        answerMedia.direction = Direction::SENDRECV;
        answerMedia.mid = offerMedia.mid;
        answerMedia.rtcpMux = offerMedia.rtcpMux;
        answerMedia.sctpPort = offerMedia.sctpPort;
        answerMedia.maxMessageSize = offerMedia.maxMessageSize;
      }
      else
      {
        answerMedia.mediaType = offerMedia.mediaType;
        answerMedia.port = 9;
        answerMedia.protocol = offerMedia.protocol;
        // Answer the offer's EFFECTIVE direction (session-level folded down per
        // RFC 4566 §5/§6), not the raw media-level field which is nullopt when
        // only a session-level direction was present. In-range index → Option B
        // guarantees a concrete value, so the deref is safe (RFC 3264 §6.1).
        answerMedia.direction = flipDirection(*offer.effectiveDirection(i));
        answerMedia.mid = offerMedia.mid;
        answerMedia.rtcpMux = offerMedia.rtcpMux;

        if (answerOptions.supportedMedia.has_value())
        {
          // Option A: the answerer declares its OWN capabilities. Emit the
          // answerer's rtpMaps / fmtps / rtcpFeedbacks (intersected with the
          // offer, keyed to the answerer PT) and the URI-intersected extMaps —
          // never a verbatim echo of the offerer's attributes (RFC 3264 §6;
          // RFC 7587 §7.1; RFC 4585 §4.2; RFC 8285 §7).
          const MediaDescription& answererMedia = answerOptions.supportedMedia.value();
          auto matched = matchCodecs(offerMedia, answererMedia);

          if (matched.empty())
          {
            answerMedia.port = 0;
            answerMedia.direction = Direction::INACTIVE;
            answerMedia.formats = offerMedia.formats;
          }
          else
          {
            // The offer's codecs normalized so rtpmap-less RFC 3551 statics also
            // resolve — used for the answerer-PT ↔ offer-PT correspondence.
            const std::vector<RtpMapAttribute> offerCodecs =
              normalizeStaticFormats(offerMedia);

            // Dedup guard shared by both rtcp-fb emit paths.
            auto pushUniqueFb = [&](RtcpFbAttribute e)
            {
              if (std::find(answerMedia.rtcpFeedbacks.begin(),
                            answerMedia.rtcpFeedbacks.end(), e)
                  == answerMedia.rtcpFeedbacks.end())
              {
                answerMedia.rtcpFeedbacks.push_back(std::move(e));
              }
            };

            answerMedia.rtpMaps = std::move(matched);
            for (const auto& codec : answerMedia.rtpMaps)
            {
              const std::uint8_t answererPt = codec.payloadType;
              const std::string answererPtStr = std::to_string(answererPt);
              answerMedia.formats.push_back(answererPtStr);

              // FMTP: the answerer's own fmtp for this PT (absent → codec
              // defaults; never the offerer's).
              for (const auto& fmtp : answererMedia.fmtps)
              {
                if (fmtp.payloadType == answererPt)
                {
                  answerMedia.fmtps.push_back(fmtp);
                }
              }

              // Offer PT for this codec: first name+clock match in offer m=
              // FMT-LIST order (deterministic tie-break; does not disambiguate
              // fmtp-differentiated same-name codecs — see tracker 2026-09-02-3).
              std::optional<std::uint8_t> offerPt;
              for (const auto& oc : offerCodecs)
              {
                if (detail::iequals(oc.encodingName, codec.encodingName)
                    && oc.clockRate == codec.clockRate)
                {
                  offerPt = oc.payloadType;
                  break;
                }
              }

              // RTCP-FB: intersection on the (type, subtype) tuple, keyed to the
              // answerer PT. An answerer specific-PT entry is kept when the offer
              // advertises the same feedback for the offer PT (specific or '*').
              // An answerer '*' entry is emitted verbatim only when the offer
              // also declares '*'; otherwise it is expanded to this PT when the
              // offer supports it (RFC 4585 §4.2 / §5.1).
              if (offerPt.has_value())
              {
                for (const auto& afb : answererMedia.rtcpFeedbacks)
                {
                  const bool specific = (afb.payloadType == answererPtStr);
                  const bool wildcard = (afb.payloadType == "*");
                  if (!specific && !wildcard)
                  {
                    continue;
                  }
                  if (!hasRtcpFb(offerMedia, *offerPt, afb.type, afb.subtype))
                  {
                    continue;
                  }
                  if (wildcard && hasRtcpFbWildcard(offerMedia, afb.type, afb.subtype))
                  {
                    // Both sides declare '*' — emit '*' once (section-scoped).
                    pushUniqueFb({"*", afb.type, afb.subtype});
                  }
                  else
                  {
                    // Emit / expand to this specific answerer PT.
                    pushUniqueFb({answererPtStr, afb.type, afb.subtype});
                  }
                }
              }
            }

            // A section-scoped '*' subsumes any specific-PT entry with the same
            // (type, subtype) — drop the redundant specifics (RFC 4585 §5.1).
            // Snapshot the wildcard tuples first so the remove_if predicate does
            // not read the vector it is compacting (order-independent).
            std::vector<std::pair<std::string, std::optional<std::string>>> wildFbs;
            for (const auto& fb : answerMedia.rtcpFeedbacks)
            {
              if (fb.payloadType == "*")
              {
                wildFbs.emplace_back(fb.type, fb.subtype);
              }
            }
            if (!wildFbs.empty())
            {
              answerMedia.rtcpFeedbacks.erase(
                std::remove_if(answerMedia.rtcpFeedbacks.begin(),
                               answerMedia.rtcpFeedbacks.end(),
                               [&](const RtcpFbAttribute& fb)
                               {
                                 return fb.payloadType != "*"
                                   && std::any_of(wildFbs.begin(), wildFbs.end(),
                                        [&](const auto& w)
                                        {
                                          return w.first == fb.type
                                            && w.second == fb.subtype;
                                        });
                               }),
                answerMedia.rtcpFeedbacks.end());
            }

            // PTIME / MAXPTIME: the answerer's own packetization preference
            // (RFC 3264 §6.1). Absent → omitted; never the offerer's.
            answerMedia.ptime = answererMedia.ptime;
            answerMedia.maxptime = answererMedia.maxptime;

            // EXTMAP (media-level): URI-intersected against the answerer, offer
            // id reused, direction reconciled (RFC 8285 §7 — see answerExtMaps).
            answerMedia.extMaps = answerExtMaps(
              offerMedia, *offer.effectiveDirection(i), &answererMedia);
          }
        }
        else
        {
          // Capability-blind fallback (no answerer template): accept the offer's
          // codecs as-offered and echo the offerer's fmtp / rtcp-fb. Echoing fmtp
          // is required for the RFC 4867 §8.3.1 MUST-echo framing params (AMR
          // octet-align et al.) and de-facto for H.264 packetization-mode; it
          // over-advertises the orthogonal opus/telephone-event params (RFC 7587
          // §7.1), and echoing rtcp-fb over-advertises feedback (RFC 4585 §4.2).
          // A correct capability-blind policy must be CODEC-AWARE (MUST-echo vs
          // MUST-NOT-echo) and is deferred to its own tracker (2026-09-02-4) — a
          // caller needing conformant per-codec negotiation supplies supportedMedia
          // and takes the matched branch above. Only the a=extmap direction flip
          // (RFC 8285 §7 — id reused) is applied here, a pure correctness fix.
          answerMedia.rtpMaps = offerMedia.rtpMaps;
          answerMedia.fmtps = offerMedia.fmtps;
          answerMedia.rtcpFeedbacks = offerMedia.rtcpFeedbacks;
          answerMedia.extMaps = answerExtMaps(
            offerMedia, *offer.effectiveDirection(i), nullptr);
          // Mirror the offer's m= fmt list directly (RFC 3264 section 6.1).
          // Building formats from rtpMaps would drop the offer's rtpmap-less
          // static PTs and emit an m= line with no format tokens — invalid SDP
          // per RFC 4566 section 5.14 (tracker 2026-07-23-2 task 1.3 / H-C).
          answerMedia.formats = offerMedia.formats;
        }
      }

      session.mediaDescriptions.push_back(std::move(answerMedia));
    }

    return session;
  }
};

} // namespace sdp
} // namespace iora

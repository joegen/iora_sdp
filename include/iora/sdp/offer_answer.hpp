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
  std::optional<std::vector<RtpMapAttribute>> supportedCodecs;
  std::vector<MediaType> rejectMediaTypes;

  bool operator==(const AnswerOptions& other) const noexcept
  {
    return iceUfrag == other.iceUfrag
        && icePwd == other.icePwd
        && fingerprint == other.fingerprint
        && dtlsSetup == other.dtlsSetup
        && supportedCodecs == other.supportedCodecs
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

    session.groups = offer.groups;

    for (const auto& offerMedia : offer.mediaDescriptions)
    {
      MediaDescription answerMedia;

      bool rejected = std::find(answerOptions.rejectMediaTypes.begin(),
                                 answerOptions.rejectMediaTypes.end(),
                                 offerMedia.mediaType)
                      != answerOptions.rejectMediaTypes.end();

      if (rejected)
      {
        answerMedia.mediaType = offerMedia.mediaType;
        answerMedia.port = 0;
        answerMedia.protocol = offerMedia.protocol;
        answerMedia.formats = offerMedia.formats;
        answerMedia.direction = Direction::INACTIVE;
        answerMedia.mid = offerMedia.mid;
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
        answerMedia.direction = flipDirection(offerMedia.direction);
        answerMedia.mid = offerMedia.mid;
        answerMedia.rtcpMux = offerMedia.rtcpMux;

        if (answerOptions.supportedCodecs.has_value())
        {
          MediaDescription answererMedia;
          answererMedia.rtpMaps = answerOptions.supportedCodecs.value();
          auto matched = matchCodecs(offerMedia, answererMedia);

          if (matched.empty())
          {
            answerMedia.port = 0;
            answerMedia.direction = Direction::INACTIVE;
            answerMedia.formats = offerMedia.formats;
          }
          else
          {
            answerMedia.rtpMaps = matched;
            for (const auto& codec : matched)
            {
              answerMedia.formats.push_back(std::to_string(codec.payloadType));
            }
          }
        }
        else
        {
          answerMedia.rtpMaps = offerMedia.rtpMaps;
          answerMedia.fmtps = offerMedia.fmtps;
          answerMedia.rtcpFeedbacks = offerMedia.rtcpFeedbacks;
          answerMedia.extMaps = offerMedia.extMaps;
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

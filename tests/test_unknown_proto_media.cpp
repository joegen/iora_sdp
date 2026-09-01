// Tests for the unrecognized/malformed SDP m= <proto>/<media> fail-safe:
// unrecognized or truncated tokens must resolve to the fail-safe UNKNOWN
// sentinel (never the RTP_AVP/AUDIO struct default), preserve the exact raw
// wire token, round-trip faithfully, and be rejected by every answerer/gate
// that trusts the parsed enum.
// Tracker: coding_trackers/tasks/iora_sdp/backlog/2026-07-31-1_*_P0.json

#include <catch2/catch.hpp>

#include "iora/sdp/offer_answer.hpp"
#include "iora/sdp/sdp_parser.hpp"
#include "iora/sdp/sdp_serializer.hpp"
#include "iora/sdp/session_description.hpp"
#include "iora/sdp/types.hpp"

using namespace iora::sdp;

namespace
{
// A minimal, well-formed session header so parse() yields a value; the caller
// appends the m= line under test.
std::string withHeader(const std::string& mLineAndAttrs)
{
  return "v=0\r\n"
         "o=- 1 1 IN IP4 192.0.2.1\r\n"
         "s=-\r\n"
         "t=0 0\r\n"
         + mLineAndAttrs;
}
} // namespace

// ---------------------------------------------------------------------------
// Phase 1 — parser: unrecognized token -> UNKNOWN + raw preserved byte-for-byte
// ---------------------------------------------------------------------------

TEST_CASE("parser: unrecognized proto -> UNKNOWN, raw token preserved",
          "[unknown][parser][proto]")
{
  // RFC 5764 UDP/TLS/RTP/SAVP (no trailing F) is a real secure profile that the
  // current recognized table misses — it MUST NOT silently default to RTP/AVP.
  auto r = SdpParser::parse(withHeader("m=audio 5004 UDP/TLS/RTP/SAVP 0\r\n"));
  REQUIRE(r.value.has_value());
  REQUIRE(r.value->mediaDescriptions.size() == 1);
  const auto& m = r.value->mediaDescriptions[0];
  REQUIRE(m.protocol == TransportProtocol::UNKNOWN);
  REQUIRE(m.rawProtocol == "UDP/TLS/RTP/SAVP"); // exact spelling & case
  REQUIRE(m.mediaType == MediaType::AUDIO);       // media token IS recognized
  REQUIRE(m.rawMediaType.empty());
}

TEST_CASE("parser: unrecognized media -> UNKNOWN, raw token preserved",
          "[unknown][parser][media]")
{
  auto r = SdpParser::parse(withHeader("m=image 49172 udptl t38\r\n"));
  REQUIRE(r.value.has_value());
  REQUIRE(r.value->mediaDescriptions.size() == 1);
  const auto& m = r.value->mediaDescriptions[0];
  REQUIRE(m.mediaType == MediaType::UNKNOWN);
  REQUIRE(m.rawMediaType == "image");
  REQUIRE(m.protocol == TransportProtocol::UNKNOWN);
  REQUIRE(m.rawProtocol == "udptl");
}

TEST_CASE("parser: exact-case preservation of an unknown proto",
          "[unknown][parser][proto]")
{
  auto r = SdpParser::parse(withHeader("m=audio 5004 RtP/FoO/SaVp 0\r\n"));
  REQUIRE(r.value.has_value());
  const auto& m = r.value->mediaDescriptions[0];
  REQUIRE(m.protocol == TransportProtocol::UNKNOWN);
  REQUIRE(m.rawProtocol == "RtP/FoO/SaVp");
}

TEST_CASE("parser: RTP/AVPF (RFC 4585, not in the recognized table) -> UNKNOWN",
          "[unknown][parser][proto][regression]")
{
  // Real-world spelling that fail-safe-rejects TODAY (recognition is the split-out
  // tracker 2026-09-01-1); it must parse to UNKNOWN, not silently to RTP/AVP.
  auto r = SdpParser::parse(withHeader("m=audio 5004 RTP/AVPF 0\r\n"));
  REQUIRE(r.value.has_value());
  const auto& m = r.value->mediaDescriptions[0];
  REQUIRE(m.protocol == TransportProtocol::UNKNOWN);
  REQUIRE(m.rawProtocol == "RTP/AVPF");
}

TEST_CASE("parser: recognized secure profiles still map (no fail-safe over-reach)",
          "[unknown][parser][proto][regression]")
{
  struct Case { const char* proto; TransportProtocol expected; };
  const Case cases[] = {
    {"RTP/AVP", TransportProtocol::RTP_AVP},
    {"RTP/SAVP", TransportProtocol::RTP_SAVP},
    {"RTP/SAVPF", TransportProtocol::RTP_SAVPF},
    {"UDP/TLS/RTP/SAVPF", TransportProtocol::UDP_TLS_RTP_SAVPF},
  };
  for (const auto& c : cases)
  {
    auto r = SdpParser::parse(
      withHeader(std::string("m=audio 5004 ") + c.proto + " 0\r\n"));
    REQUIRE(r.value.has_value());
    const auto& m = r.value->mediaDescriptions[0];
    REQUIRE(m.protocol == c.expected);
    REQUIRE(m.rawProtocol.empty()); // recognized -> no raw token retained
  }
}

// ---------------------------------------------------------------------------
// Phase 1 — malformed/truncated m= line must NOT become a serializable section
// ---------------------------------------------------------------------------

TEST_CASE("parser: truncated m= line is a HARD ERROR in lenient mode too",
          "[unknown][parser][malformed]")
{
  // "m=audio 5004" — missing <proto> (and <fmt>). A malformed m= line is a
  // structural RFC 4566 §5.14 failure, not a salvageable attribute: it fails the
  // whole parse in BOTH modes so an answerer never sees a partial offer.
  auto r = SdpParser::parse(withHeader("m=audio 5004\r\n"));
  REQUIRE_FALSE(r.value.has_value());
  REQUIRE(r.error.has_value());
}

TEST_CASE("parser (strict): truncated m= line is a hard error",
          "[unknown][parser][malformed][strict]")
{
  SdpParserOptions strict;
  strict.strict = true;
  auto r = SdpParser::parse(withHeader("m=audio 5004\r\n"), strict);
  REQUIRE_FALSE(r.value.has_value());
  REQUIRE(r.error.has_value());
}

TEST_CASE("parser: a zero-<fmt> ACTIVE (port != 0) m= line is malformed",
          "[unknown][parser][malformed]")
{
  // Three tokens but NO fmt on an active stream: it would otherwise serialize to
  // an RFC-invalid fmt-less m= line (and, on the reject path, echo an empty format
  // list). RFC 4566 §5.14 requires 1*(SP fmt) for a live stream.
  auto r = SdpParser::parse(withHeader("m=audio 5004 SOME/VENDOR/SECURE\r\n"));
  REQUIRE_FALSE(r.value.has_value());
  REQUIRE(r.error.has_value());
}

TEST_CASE("parser: a zero-<fmt> port-0 m= line is a TOLERATED stream removal",
          "[unknown][parser][malformed][removal]")
{
  // RFC 3264 §8.2 stream removal: some endpoints emit `m=<media> 0 <proto>` with
  // no fmt. It must NOT fail the parse (that would silently defeat the removal);
  // it is kept as a port-0 section and round-trips as-is.
  auto r = SdpParser::parse(withHeader("m=video 0 RTP/AVP\r\n"));
  REQUIRE(r.value.has_value());
  REQUIRE(r.value->mediaDescriptions.size() == 1);
  const auto& m = r.value->mediaDescriptions[0];
  REQUIRE(m.mediaType == MediaType::VIDEO);
  REQUIRE(m.protocol == TransportProtocol::RTP_AVP);
  REQUIRE(m.port == 0);
  REQUIRE(m.formats.empty());

  // Emit-conservative: the fmt-less removal is re-serialized with a placeholder
  // "0" fmt so the wire line is RFC 4566 §5.14-valid (never a corrupt fmt-less or
  // "m= 0 " line); it still reparses as the same port-0 removal.
  auto out = SdpSerializer::serialize(*r.value);
  REQUIRE(out.find("m=video 0 RTP/AVP 0") != std::string::npos);
  auto reparsed = SdpParser::parse(out);
  REQUIRE(reparsed.value.has_value());
  REQUIRE(reparsed.value->mediaDescriptions.size() == 1);
  REQUIRE(reparsed.value->mediaDescriptions[0].port == 0);
}

TEST_CASE("parser: a zero-<fmt> port-0 UNKNOWN-proto removal is tolerated + echoes raw",
          "[unknown][parser][malformed][removal]")
{
  // A removal under an unrecognized proto: kept (port 0), UNKNOWN with the raw
  // token preserved, and round-trips the raw proto (never downgraded).
  auto r = SdpParser::parse(withHeader("m=audio 0 SOME/VENDOR/SAVP\r\n"));
  REQUIRE(r.value.has_value());
  REQUIRE(r.value->mediaDescriptions.size() == 1);
  const auto& m = r.value->mediaDescriptions[0];
  REQUIRE(m.protocol == TransportProtocol::UNKNOWN);
  REQUIRE(m.rawProtocol == "SOME/VENDOR/SAVP");
  REQUIRE(m.port == 0);

  auto out = SdpSerializer::serialize(*r.value);
  REQUIRE(out.find("m=audio 0 SOME/VENDOR/SAVP 0") != std::string::npos); // placeholder fmt
}

TEST_CASE("parser: a malformed m= section fails the WHOLE offer (no m-line desync)",
          "[unknown][parser][malformed]")
{
  // RFC 3264 §6 positional correspondence: an N-section offer with one malformed
  // section must NOT parse to N-1 sections (which would desync the answer and
  // break BUNDLE mid association). The whole parse fails.
  auto r = SdpParser::parse(withHeader(
    "m=audio 5004\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "m=audio 5006 RTP/AVP 0\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"));
  REQUIRE_FALSE(r.value.has_value());
  REQUIRE(r.error.has_value());
}

// ---------------------------------------------------------------------------
// Phase 2 — serializer round-trip + validity
// ---------------------------------------------------------------------------

TEST_CASE("serializer: UNKNOWN proto/media round-trips the original raw token",
          "[unknown][serializer][round-trip]")
{
  const std::string in = withHeader("m=image 49172 udptl t38\r\n");
  auto parsed = SdpParser::parse(in);
  REQUIRE(parsed.value.has_value());
  auto out = SdpSerializer::serialize(*parsed.value);

  // The exact wire tokens, never a "" / "audio" / "UDP" fallback.
  REQUIRE(out.find("m=image 49172 udptl t38") != std::string::npos);
  REQUIRE(out.find("m= ") == std::string::npos);   // no empty media token
  REQUIRE(out.find(" 49172 \r") == std::string::npos); // no empty proto token

  // parse -> serialize -> parse is a fixed point (validity).
  auto reparsed = SdpParser::parse(out);
  REQUIRE(reparsed.value.has_value());
  REQUIRE(reparsed.value->mediaDescriptions.size() == 1);
  const auto& m = reparsed.value->mediaDescriptions[0];
  REQUIRE(m.mediaType == MediaType::UNKNOWN);
  REQUIRE(m.rawMediaType == "image");
  REQUIRE(m.protocol == TransportProtocol::UNKNOWN);
  REQUIRE(m.rawProtocol == "udptl");
}

TEST_CASE("serializer: an UNKNOWN section survives a B2BUA field mutation (proxy "
          "rewrites c=, re-emits the raw proto — no forward-leg downgrade)",
          "[unknown][serializer][round-trip][b2bua]")
{
  auto parsed = SdpParser::parse(withHeader(
    "m=audio 5004 UDP/TLS/RTP/SAVP 0\r\n"
    "c=IN IP4 203.0.113.9\r\n"));
  REQUIRE(parsed.value.has_value());

  // A B2BUA rewrites the media connection address, then re-serializes.
  auto& md = parsed.value->mediaDescriptions[0];
  md.connection = ConnectionData{NetworkType::IN, AddressType::IP4, "198.51.100.7"};
  auto out = SdpSerializer::serialize(*parsed.value);

  // The secure proto is re-emitted verbatim, NEVER downgraded to RTP/AVP or "".
  REQUIRE(out.find("m=audio 5004 UDP/TLS/RTP/SAVP 0") != std::string::npos);
  REQUIRE(out.find("c=IN IP4 198.51.100.7") != std::string::npos);

  auto reparsed = SdpParser::parse(out);
  REQUIRE(reparsed.value.has_value());
  const auto& m = reparsed.value->mediaDescriptions[0];
  REQUIRE(m.protocol == TransportProtocol::UNKNOWN);
  REQUIRE(m.rawProtocol == "UDP/TLS/RTP/SAVP");
}

// ---------------------------------------------------------------------------
// Phase 2 — operator== includes the raw tokens
// ---------------------------------------------------------------------------

TEST_CASE("operator==: two DIFFERENT unknown tokens are NOT equal",
          "[unknown][operator==]")
{
  MediaDescription a;
  a.protocol = TransportProtocol::UNKNOWN;
  a.rawProtocol = "VENDOR/A";
  MediaDescription b;
  b.protocol = TransportProtocol::UNKNOWN;
  b.rawProtocol = "VENDOR/B";
  REQUIRE(a != b); // enum equal, raw token differs -> unequal

  MediaDescription c;
  c.protocol = TransportProtocol::UNKNOWN;
  c.rawProtocol = "VENDOR/A";
  REQUIRE(a == c);
}

TEST_CASE("UNKNOWN compares unequal to every recognized enum value",
          "[unknown][operator==]")
{
  REQUIRE(TransportProtocol::UNKNOWN != TransportProtocol::RTP_AVP);
  REQUIRE(TransportProtocol::UNKNOWN != TransportProtocol::UDP_TLS_RTP_SAVPF);
  REQUIRE(MediaType::UNKNOWN != MediaType::AUDIO);
  REQUIRE(MediaType::UNKNOWN != MediaType::APPLICATION);
}

// ---------------------------------------------------------------------------
// Phase 3 — createAnswer fail-safe for UNKNOWN (all three dispatch arms)
// ---------------------------------------------------------------------------

TEST_CASE("createAnswer: UNKNOWN proto on an audio offer -> port-0 INACTIVE",
          "[unknown][offer_answer][createAnswer]")
{
  auto offer = SdpParser::parse(
    withHeader("m=audio 5004 SOME/VENDOR/SECURE 0\r\n"));
  REQUIRE(offer.value.has_value());
  AnswerOptions opts;
  opts.supportedCodecs = std::vector<RtpMapAttribute>{{0, "PCMU", 8000, {}}};

  auto ans = OfferAnswer::createAnswer(*offer.value, opts);
  REQUIRE(ans.mediaDescriptions.size() == 1);
  const auto& m = ans.mediaDescriptions[0];
  REQUIRE(m.port == 0); // rejected, NOT port 9
  REQUIRE(m.direction == Direction::INACTIVE);
  REQUIRE(m.protocol == TransportProtocol::UNKNOWN);
  REQUIRE(m.rawProtocol == "SOME/VENDOR/SECURE"); // echoed, not ""
}

TEST_CASE("createAnswer: UNKNOWN proto without supportedCodecs -> port-0",
          "[unknown][offer_answer][createAnswer]")
{
  auto offer = SdpParser::parse(
    withHeader("m=audio 5004 SOME/VENDOR/SECURE 0\r\n"));
  REQUIRE(offer.value.has_value());
  auto ans = OfferAnswer::createAnswer(*offer.value, AnswerOptions{});
  REQUIRE(ans.mediaDescriptions.size() == 1);
  REQUIRE(ans.mediaDescriptions[0].port == 0);
}

TEST_CASE("createAnswer: APPLICATION media with UNKNOWN proto -> port-0 (not port 9)",
          "[unknown][offer_answer][createAnswer][application]")
{
  // Regression for the round-3 cpp17 finding: the APPLICATION arm was a second
  // port=9 accept path that bypassed the else-branch UNKNOWN gate.
  auto offer = SdpParser::parse(
    withHeader("m=application 9 SOME/VENDOR/PROTO webrtc-datachannel\r\n"));
  REQUIRE(offer.value.has_value());
  REQUIRE(offer.value->mediaDescriptions[0].mediaType == MediaType::APPLICATION);
  REQUIRE(offer.value->mediaDescriptions[0].protocol == TransportProtocol::UNKNOWN);

  auto ans = OfferAnswer::createAnswer(*offer.value, AnswerOptions{});
  REQUIRE(ans.mediaDescriptions.size() == 1);
  REQUIRE(ans.mediaDescriptions[0].port == 0);
  REQUIRE(ans.mediaDescriptions[0].direction == Direction::INACTIVE);
}

TEST_CASE("createAnswer: UNKNOWN media type -> port-0",
          "[unknown][offer_answer][createAnswer]")
{
  auto offer = SdpParser::parse(withHeader("m=image 49172 udptl t38\r\n"));
  REQUIRE(offer.value.has_value());
  auto ans = OfferAnswer::createAnswer(*offer.value, AnswerOptions{});
  REQUIRE(ans.mediaDescriptions.size() == 1);
  REQUIRE(ans.mediaDescriptions[0].port == 0);
  REQUIRE(ans.mediaDescriptions[0].mediaType == MediaType::UNKNOWN);
  REQUIRE(ans.mediaDescriptions[0].rawMediaType == "image");
}

TEST_CASE("createAnswer: a mixed offer (valid audio + UNKNOWN section) preserves "
          "m-line count and order (RFC 3264 §6 positional correspondence)",
          "[unknown][offer_answer][createAnswer][positional]")
{
  auto offer = SdpParser::parse(
    "v=0\r\n"
    "o=- 1 1 IN IP4 192.0.2.1\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=audio 5004 RTP/AVP 0\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "m=image 49172 udptl t38\r\n"); // second section is UNKNOWN (media + proto)
  REQUIRE(offer.value.has_value());
  REQUIRE(offer.value->mediaDescriptions.size() == 2);

  AnswerOptions opts;
  opts.supportedCodecs = std::vector<RtpMapAttribute>{{0, "PCMU", 8000, {}}};
  auto ans = OfferAnswer::createAnswer(*offer.value, opts);

  // Both sections answered, in order: section 0 accepted, section 1 port-0 reject.
  REQUIRE(ans.mediaDescriptions.size() == 2);
  CHECK(ans.mediaDescriptions[0].mediaType == MediaType::AUDIO);
  CHECK(ans.mediaDescriptions[0].port != 0); // audio answered
  CHECK(ans.mediaDescriptions[1].mediaType == MediaType::UNKNOWN);
  CHECK(ans.mediaDescriptions[1].rawMediaType == "image");
  CHECK(ans.mediaDescriptions[1].port == 0); // UNKNOWN rejected
  CHECK(ans.mediaDescriptions[1].direction == Direction::INACTIVE);
}

TEST_CASE("createAnswer: the UNKNOWN-proto reject section round-trips as VALID SDP",
          "[unknown][offer_answer][createAnswer][round-trip]")
{
  // Parse an UNKNOWN-proto offer, answer it (port-0 reject), serialize the answer,
  // and re-parse: the rejected m= line must carry a non-empty <media>, non-empty
  // <proto> (the echoed raw token), and >=1 <fmt> (RFC 4566 §5.14) — never a
  // corrupt "m= 0 " / fmt-less line.
  auto offer = SdpParser::parse(
    withHeader("m=audio 5004 SOME/VENDOR/SECURE 0 8\r\n"));
  REQUIRE(offer.value.has_value());

  auto ans = OfferAnswer::createAnswer(*offer.value, AnswerOptions{});
  REQUIRE(ans.mediaDescriptions.size() == 1);
  REQUIRE(ans.mediaDescriptions[0].port == 0);

  // Give the answer a connection so it serializes as a complete session.
  ans.connection = ConnectionData{NetworkType::IN, AddressType::IP4, "192.0.2.9"};
  auto wire = SdpSerializer::serialize(ans);
  REQUIRE(wire.find("m=audio 0 SOME/VENDOR/SECURE 0 8") != std::string::npos);
  REQUIRE(wire.find("m= ") == std::string::npos); // no empty media token

  auto reparsed = SdpParser::parse(wire);
  REQUIRE(reparsed.value.has_value()); // valid SDP: re-parses cleanly
  REQUIRE(reparsed.value->mediaDescriptions.size() == 1);
  const auto& m = reparsed.value->mediaDescriptions[0];
  REQUIRE(m.port == 0);
  REQUIRE(m.protocol == TransportProtocol::UNKNOWN);
  REQUIRE(m.rawProtocol == "SOME/VENDOR/SECURE");
  REQUIRE_FALSE(m.formats.empty()); // >=1 fmt survived
}

TEST_CASE("createAnswer: answering a fmt-less port-0 UNKNOWN removal emits a VALID "
          "(placeholder-fmt) rejected m= line",
          "[unknown][offer_answer][createAnswer][removal]")
{
  // The rejectEcho path copies the offer's (empty) formats for a port-0 removal;
  // the serializer must still emit a mandatory placeholder fmt (RFC 4566 §5.14),
  // so the generated answer is valid SDP and reparses with a non-empty fmt list.
  auto offer = SdpParser::parse(withHeader("m=audio 0 SOME/VENDOR/SAVP\r\n"));
  REQUIRE(offer.value.has_value());
  REQUIRE(offer.value->mediaDescriptions[0].formats.empty()); // fmt-less removal

  auto ans = OfferAnswer::createAnswer(*offer.value, AnswerOptions{});
  REQUIRE(ans.mediaDescriptions.size() == 1);
  CHECK(ans.mediaDescriptions[0].port == 0);

  ans.connection = ConnectionData{NetworkType::IN, AddressType::IP4, "192.0.2.9"};
  auto wire = SdpSerializer::serialize(ans);
  REQUIRE(wire.find("m=audio 0 SOME/VENDOR/SAVP 0") != std::string::npos); // placeholder
  auto reparsed = SdpParser::parse(wire);
  REQUIRE(reparsed.value.has_value());
  REQUIRE_FALSE(reparsed.value->mediaDescriptions[0].formats.empty());
}

// ---------------------------------------------------------------------------
// Phase 4 — createOffer never emits UNKNOWN (pins the "never offer UNKNOWN"
// invariant the iora_media gate asserts on its offer operand).
// ---------------------------------------------------------------------------

TEST_CASE("createOffer never emits UNKNOWN proto/media",
          "[unknown][offer_answer][createOffer]")
{
  OfferOptions opts;
  opts.audio = true;
  opts.video = true;
  opts.dataChannel = true;
  auto offer = OfferAnswer::createOffer(opts);
  REQUIRE_FALSE(offer.mediaDescriptions.empty());
  for (const auto& m : offer.mediaDescriptions)
  {
    REQUIRE(m.mediaType != MediaType::UNKNOWN);
    REQUIRE(m.protocol != TransportProtocol::UNKNOWN);
    REQUIRE(m.rawMediaType.empty());
    REQUIRE(m.rawProtocol.empty());
  }
}

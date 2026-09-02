#include <catch2/catch.hpp>
#include "iora/sdp/sdp_parser.hpp"

using namespace iora::sdp;

// =========================================================================
// parseMediaLine <port> / <number of ports> parse-failure rejection
// (tracker 2026-09-01-2). A non-numeric / out-of-range / missing <port>
// (and a present-but-unparseable /<n> count) is a malformed m= line and
// MUST reject the whole parse (RFC 4566 §5.14), never silently coerce to
// the port-0 default (which masquerades as an RFC 3264 §8.2 removal).
// =========================================================================

namespace
{
// Wrap an m= line body in a minimal valid session so only the m= line is
// under test. requireTiming defaults false, so v=/o=/s= suffice; t= added
// for realism.
std::string withMLine(const std::string& mline)
{
  return std::string("v=0\r\n"
                     "o=- 0 0 IN IP4 0.0.0.0\r\n"
                     "s=-\r\n"
                     "t=0 0\r\n") +
         mline + "\r\n";
}
} // namespace

TEST_CASE("parseMediaLine: single-port garbage rejects (a)", "[parser][mport]")
{
  auto r = SdpParser::parse(withMLine("m=audio notaport RTP/AVP 0"));
  REQUIRE(!r.hasValue());
  // Attribute the rejection to the malformed m= port cause, not an unrelated
  // reject — so a future edit cannot make this pass for the wrong reason.
  REQUIRE(r.error.has_value());
  REQUIRE(r.error->message == detail::kMalformedMLineError);
}

TEST_CASE("parseMediaLine: explicit port-0 removal preserved (b)",
          "[parser][mport]")
{
  auto r = SdpParser::parse(withMLine("m=audio 0 RTP/AVP 0"));
  REQUIRE(r.hasValue());
  REQUIRE(r.value->mediaDescriptions.size() == 1);
  REQUIRE(r.value->mediaDescriptions[0].port == 0);
}

TEST_CASE("parseMediaLine: out-of-range single port rejects (c)",
          "[parser][mport]")
{
  auto r = SdpParser::parse(withMLine("m=audio 65536 RTP/AVP 0"));
  REQUIRE(!r.hasValue());
}

TEST_CASE("parseMediaLine: fmt-less garbage port rejects (d)",
          "[parser][mport]")
{
  // Widened-blast-radius headline: garbage port + empty fmt must NOT be
  // tolerated as a port-0 removal.
  auto r = SdpParser::parse(withMLine("m=audio notaport RTP/AVP"));
  REQUIRE(!r.hasValue());
}

TEST_CASE("parseMediaLine: token-shift / missing port rejects (e)",
          "[parser][mport]")
{
  // Missing port shifts tokens: portStr becomes "RTP/AVP" -> slash branch,
  // port half "RTP" fails. Must reject, not become a spurious removal.
  auto r = SdpParser::parse(withMLine("m=audio RTP/AVP 0"));
  REQUIRE(!r.hasValue());
}

TEST_CASE("parseMediaLine: slash form, garbage port half rejects (f)",
          "[parser][mport]")
{
  auto r = SdpParser::parse(withMLine("m=video notaport/2 RTP/AVP 96"));
  REQUIRE(!r.hasValue());
}

TEST_CASE("parseMediaLine: slash form, garbage count half rejects (g)",
          "[parser][mport]")
{
  // The count must not be silently dropped (would round-trip as a bare port).
  auto r = SdpParser::parse(withMLine("m=video 49170/notanum RTP/AVP 96"));
  REQUIRE(!r.hasValue());
}

TEST_CASE("parseMediaLine: slash form, empty count rejects (h)",
          "[parser][mport]")
{
  auto r = SdpParser::parse(withMLine("m=video 49170/ RTP/AVP 96"));
  REQUIRE(!r.hasValue());
}

TEST_CASE("parseMediaLine: slash form, empty port half rejects (h2)",
          "[parser][mport]")
{
  // Mirror of (h): the port half before '/' is empty -> parseUint16("") fails.
  auto r = SdpParser::parse(withMLine("m=video /2 RTP/AVP 96"));
  REQUIRE(!r.hasValue());
}

TEST_CASE("parseMediaLine: slash form valid regression (i)", "[parser][mport]")
{
  auto r = SdpParser::parse(withMLine("m=video 49170/2 RTP/AVP 96"));
  REQUIRE(r.hasValue());
  auto& md = r.value->mediaDescriptions[0];
  REQUIRE(md.port == 49170);
  REQUIRE(md.numberOfPorts == 2);
}

TEST_CASE("parseMediaLine: trailing junk port rejects (j)", "[parser][mport]")
{
  auto r = SdpParser::parse(withMLine("m=audio 5060x RTP/AVP 0"));
  REQUIRE(!r.hasValue());
}

TEST_CASE("parseMediaLine: negative port rejects (k)", "[parser][mport]")
{
  auto r = SdpParser::parse(withMLine("m=audio -1 RTP/AVP 0"));
  REQUIRE(!r.hasValue());
}

TEST_CASE("parseMediaLine: boundary port 65535 accepted (l)",
          "[parser][mport]")
{
  auto r = SdpParser::parse(withMLine("m=audio 65535 RTP/AVP 0"));
  REQUIRE(r.hasValue());
  REQUIRE(r.value->mediaDescriptions[0].port == 65535);
}

TEST_CASE("parseMediaLine: leading-zero leniency preserved (m)",
          "[parser][mport]")
{
  auto r = SdpParser::parse(withMLine("m=audio 05060 RTP/AVP 0"));
  REQUIRE(r.hasValue());
  REQUIRE(r.value->mediaDescriptions[0].port == 5060);
}

TEST_CASE("parseMediaLine: over-range-many-digits rejects, no wrap (n)",
          "[parser][mport]")
{
  // from_chars reports result_out_of_range; must NOT wrap to a small value.
  auto r = SdpParser::parse(withMLine("m=audio 999999999999999999999 RTP/AVP 0"));
  REQUIRE(!r.hasValue());
}

TEST_CASE("parseMediaLine: normal nonzero port not a removal (o)",
          "[parser][mport]")
{
  auto r = SdpParser::parse(withMLine("m=audio 9 RTP/AVP 0"));
  REQUIRE(r.hasValue());
  REQUIRE(r.value->mediaDescriptions[0].port == 9);
}

TEST_CASE("parseMediaLine: positive full regression (p)", "[parser][mport]")
{
  auto r = SdpParser::parse(withMLine("m=audio 49170 RTP/AVP 0 8"));
  REQUIRE(r.hasValue());
  auto& md = r.value->mediaDescriptions[0];
  REQUIRE(md.port == 49170);
  REQUIRE(md.formats.size() == 2);
}

TEST_CASE("parseMediaLine: port-0 + empty-fmt tolerated removal preserved (q)",
          "[parser][mport]")
{
  // Positive twin of (d): the discriminator's other side. An explicit port-0
  // fmt-less line IS the tolerated RFC 3264 §8.2 removal and must still parse.
  auto r = SdpParser::parse(withMLine("m=audio 0 RTP/AVP"));
  REQUIRE(r.hasValue());
  auto& md = r.value->mediaDescriptions[0];
  REQUIRE(md.port == 0);
  REQUIRE(md.formats.empty());
}

TEST_CASE("parseMediaLine: garbage port rejects in BOTH strict and lenient (r)",
          "[parser][mport]")
{
  const std::string sdp = withMLine("m=audio notaport RTP/AVP 0");

  SdpParserOptions lenient;
  lenient.strict = false;
  auto lr = SdpParser::parse(sdp, lenient);
  REQUIRE(!lr.hasValue());
  REQUIRE(lr.error.has_value());
  REQUIRE(lr.error->message == detail::kMalformedMLineError);

  SdpParserOptions strict;
  strict.strict = true;
  auto sr = SdpParser::parse(sdp, strict);
  REQUIRE(!sr.hasValue());
  REQUIRE(sr.error.has_value());
  REQUIRE(sr.error->message == detail::kMalformedMLineError);
}

TEST_CASE("parseMediaLine: whole-SDP rejection, not per-stream skip (s)",
          "[parser][mport]")
{
  // A valid section followed by a garbage-port section must fail the ENTIRE
  // parse (never drop just the bad section, which would desync RFC 3264 §6
  // positional correspondence).
  const std::string sdp = withMLine("m=audio 5004 RTP/AVP 0\r\n"
                                    "m=video notaport RTP/AVP 96");
  auto r = SdpParser::parse(sdp);
  REQUIRE(!r.hasValue());
  REQUIRE(r.error.has_value());
  REQUIRE(r.error->message == detail::kMalformedMLineError);
  // The failure must be attributed to the 2nd (video) m= line, proving the
  // whole-SDP rejection is caused by the bad section, not the valid audio one.
  // Lines: 1=v 2=o 3=s 4=t 5=m=audio 6=m=video.
  REQUIRE(r.error->line == 6);
}

TEST_CASE("parseMediaLine: slash form, multi-slash rejects (t)",
          "[parser][mport]")
{
  // RFC 4566 §5.14 allows only one '/<integer>'. "2/3" fails parseUint16.
  auto r = SdpParser::parse(withMLine("m=video 5004/2/3 RTP/AVP 96"));
  REQUIRE(!r.hasValue());
}

TEST_CASE("parseMediaLine: slash form, over-range count rejects (u)",
          "[parser][mport]")
{
  auto r = SdpParser::parse(withMLine("m=video 49170/70000 RTP/AVP 96"));
  REQUIRE(!r.hasValue());
}

// -------------------------------------------------------------------------
// Second entry point: SdpParser::parseMediaSection (trickle-ICE / incremental
// parsing) also rejects a malformed port. Per rtp-media HR-13, each parse
// entry point that reaches parseMediaLine needs its own regression, not just
// the doParse path exercised above.
// -------------------------------------------------------------------------

TEST_CASE("parseMediaSection: garbage port rejects (M1 entry point)",
          "[parser][mport]")
{
  auto r = SdpParser::parseMediaSection("m=video notaport RTP/AVP 96");
  REQUIRE(!r.hasValue());
  REQUIRE(r.error.has_value());
  REQUIRE(r.error->message == detail::kMalformedMLineError);
}

TEST_CASE("parseMediaSection: valid slash form regression (M1 entry point)",
          "[parser][mport]")
{
  auto r = SdpParser::parseMediaSection("m=video 49170/2 RTP/AVP 96");
  REQUIRE(r.hasValue());
  REQUIRE(r.value->port == 49170);
  REQUIRE(r.value->numberOfPorts == 2);
}

// -------------------------------------------------------------------------
// Documented current behavior (NOT a coercion defect): a `/0` count and a
// leading-zero count PARSE (parseUint16 succeeds). The count>=1 / no-leading-
// zero semantic validation (RFC 4566 §5.14 `integer = POS-DIGIT *DIGIT`) is
// deliberately out of scope for this coercion fix and tracked as backlog
// 2026-09-02-2. This pins the tolerance as a conscious decision; 2026-09-02-2
// will flip these to reject.
// -------------------------------------------------------------------------

TEST_CASE("parseMediaLine: slash form /0 count TOLERATED (tracked 2026-09-02-2)",
          "[parser][mport]")
{
  auto r = SdpParser::parse(withMLine("m=video 49170/0 RTP/AVP 96"));
  REQUIRE(r.hasValue());
  REQUIRE(r.value->mediaDescriptions[0].numberOfPorts == 0);
}

TEST_CASE("parseMediaLine: slash form leading-zero count TOLERATED "
          "(tracked 2026-09-02-2)",
          "[parser][mport]")
{
  // parseUint16 (from_chars) accepts a leading-zero count, against the ABNF
  // `integer = POS-DIGIT *DIGIT`. Pinned as a conscious decision; 2026-09-02-2
  // will flip it to reject.
  auto r = SdpParser::parse(withMLine("m=video 49170/02 RTP/AVP 96"));
  REQUIRE(r.hasValue());
  REQUIRE(r.value->mediaDescriptions[0].numberOfPorts == 2);
}

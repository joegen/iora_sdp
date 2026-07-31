// test_effective_direction.cpp — RFC 4566 §5/§6 session-level direction
// fold-down: SessionDescription::effectiveDirection(), the optional<Direction>
// model change, serializer conditional emit, and createAnswer direction/port-0
// handling. Tracker: tasks/iora_sdp/ongoing/2026-07-25-3.
//
// test_main.cpp defines CATCH_CONFIG_MAIN for the aggregated sdp_tests binary,
// so this translation unit contributes TEST_CASEs only.
//
// Test IDs mirror the tracker's phase_5 matrix (a,b,c,d,f,g,h,i,j,k,l,n +
// m2,m3,m4). Gaps at (e) and (m1) are intentional reservations from folding.

#include <catch2/catch.hpp>

#include <string>

#include "iora/sdp/offer_answer.hpp"
#include "iora/sdp/sdp_parser.hpp"
#include "iora/sdp/sdp_serializer.hpp"
#include "iora/sdp/session_description.hpp"
#include "iora/sdp/types.hpp"

using namespace iora::sdp;

namespace
{
/// Parse helper — asserts success and returns the SessionDescription.
SessionDescription parseOk(const std::string& sdp)
{
  auto result = SdpParser::parse(sdp);
  REQUIRE(result.hasValue());
  return *result.value;
}

const std::string kHdr =
  "v=0\r\n"
  "o=- 0 0 IN IP4 0.0.0.0\r\n"
  "s=-\r\n"
  "t=0 0\r\n";
} // namespace

// ---------------------------------------------------------------------------
// Resolver + model
// ---------------------------------------------------------------------------

// (a) session-level a=inactive, no media-level attribute, two m= sections ->
//     both resolve INACTIVE.
TEST_CASE("effectiveDirection (a): session-level inactive folds to all media",
          "[session][effectiveDirection]")
{
  auto sd = parseOk(kHdr + "a=inactive\r\n" +
                    "m=audio 9 RTP/AVP 0\r\n" +
                    "m=video 9 RTP/AVP 96\r\n");
  REQUIRE(sd.direction.has_value());
  REQUIRE(*sd.direction == Direction::INACTIVE);
  REQUIRE(!sd.mediaDescriptions[0].direction.has_value());
  REQUIRE(!sd.mediaDescriptions[1].direction.has_value());
  REQUIRE(sd.effectiveDirection(0) == Direction::INACTIVE);
  REQUIRE(sd.effectiveDirection(1) == Direction::INACTIVE);
}

// (b) session-level a=sendonly with a media-level a=recvonly on section 1 ->
//     section 0 SENDONLY (inherited), section 1 RECVONLY (override). Distinct.
TEST_CASE("effectiveDirection (b): media-level overrides session-level per section",
          "[session][effectiveDirection]")
{
  auto sd = parseOk(kHdr + "a=sendonly\r\n" +
                    "m=audio 9 RTP/AVP 0\r\n" +
                    "m=video 9 RTP/AVP 96\r\n" +
                    "a=recvonly\r\n");
  REQUIRE(sd.effectiveDirection(0) == Direction::SENDONLY);
  REQUIRE(sd.effectiveDirection(1) == Direction::RECVONLY);
  // The override is stored at media level; section 0 inherits (absent).
  REQUIRE(!sd.mediaDescriptions[0].direction.has_value());
  REQUIRE(sd.mediaDescriptions[1].direction.has_value());
  REQUIRE(*sd.mediaDescriptions[1].direction == Direction::RECVONLY);
}

// (c) RFC 4566 §5's own worked example: a session-level a=recvonly applies to
//     both an audio and a video section.
TEST_CASE("effectiveDirection (c): RFC 4566 §5 worked example (session recvonly)",
          "[session][effectiveDirection]")
{
  auto sd = parseOk(kHdr + "a=recvonly\r\n" +
                    "m=audio 49170 RTP/AVP 0\r\n" +
                    "m=video 51372 RTP/AVP 99\r\n");
  REQUIRE(sd.effectiveDirection(0) == Direction::RECVONLY);
  REQUIRE(sd.effectiveDirection(1) == Direction::RECVONLY);
}

// (f) neither level carries a direction -> both resolve SENDRECV (NOT nullopt).
//     Mutation guard for Option B.
TEST_CASE("effectiveDirection (f): neither-present defaults to SENDRECV, not nullopt",
          "[session][effectiveDirection]")
{
  auto sd = parseOk(kHdr + "m=audio 9 RTP/AVP 0\r\n" +
                    "m=video 9 RTP/AVP 96\r\n");
  REQUIRE(!sd.direction.has_value());
  REQUIRE(!sd.mediaDescriptions[0].direction.has_value());
  auto d0 = sd.effectiveDirection(0);
  auto d1 = sd.effectiveDirection(1);
  REQUIRE(d0.has_value());
  REQUIRE(*d0 == Direction::SENDRECV);
  REQUIRE(d1.has_value());
  REQUIRE(*d1 == Direction::SENDRECV);
}

// (g) explicit a=sendrecv is distinguishable from absent, and explicit override
//     beats an inherited session-level direction.
TEST_CASE("effectiveDirection (g): explicit sendrecv distinguishable from absent",
          "[session][effectiveDirection]")
{
  SECTION("explicit media-level a=sendrecv is present in the model")
  {
    auto sd = parseOk(kHdr + "m=audio 9 RTP/AVP 0\r\n" + "a=sendrecv\r\n");
    REQUIRE(sd.mediaDescriptions[0].direction.has_value());
    REQUIRE(*sd.mediaDescriptions[0].direction == Direction::SENDRECV);
  }
  SECTION("absent media-level direction is nullopt")
  {
    auto sd = parseOk(kHdr + "m=audio 9 RTP/AVP 0\r\n");
    REQUIRE(!sd.mediaDescriptions[0].direction.has_value());
  }
  SECTION("explicit media sendrecv overrides an inherited session sendonly")
  {
    auto sd = parseOk(kHdr + "a=sendonly\r\n" + "m=audio 9 RTP/AVP 0\r\n" +
                      "a=sendrecv\r\n");
    REQUIRE(sd.effectiveDirection(0) == Direction::SENDRECV);
  }
}

// (i) media upgrades a restricted session: session a=inactive + media a=sendrecv
//     -> media wins -> SENDRECV (legal per RFC 4566 §5).
TEST_CASE("effectiveDirection (i): media sendrecv upgrades an inactive session",
          "[session][effectiveDirection]")
{
  auto sd = parseOk(kHdr + "a=inactive\r\n" + "m=audio 9 RTP/AVP 0\r\n" +
                    "a=sendrecv\r\n");
  REQUIRE(sd.effectiveDirection(0) == Direction::SENDRECV);
}

// (l) session-level a=sendonly + a port-0 section: the resolver folds sendonly
//     onto every in-range index (direction-only, port-agnostic). Rejection
//     precedence belongs to the consumer / answer path, not the resolver.
TEST_CASE("effectiveDirection (l): folds onto a port-0 section (resolver is port-agnostic)",
          "[session][effectiveDirection]")
{
  auto sd = parseOk(kHdr + "a=sendonly\r\n" + "m=audio 0 RTP/AVP 0\r\n");
  REQUIRE(sd.mediaDescriptions[0].port == 0);
  REQUIRE(sd.effectiveDirection(0) == Direction::SENDONLY);
}

// Out-of-range index -> nullopt (the ONLY nullopt case). Precedence matrix.
TEST_CASE("effectiveDirection: out-of-range is the only nullopt; precedence matrix",
          "[session][effectiveDirection]")
{
  SECTION("out-of-range yields nullopt")
  {
    auto sd = parseOk(kHdr + "a=sendonly\r\n" + "m=audio 9 RTP/AVP 0\r\n");
    REQUIRE(!sd.effectiveDirection(1).has_value());
    REQUIRE(!sd.effectiveDirection(99).has_value());
  }
  SECTION("empty description: index 0 is out of range -> nullopt")
  {
    SessionDescription sd;
    REQUIRE(!sd.effectiveDirection(0).has_value());
  }
  SECTION("precedence: media beats session beats default")
  {
    SessionDescription sd;
    sd.mediaDescriptions.resize(3);
    sd.direction = Direction::SENDONLY;          // session-level
    sd.mediaDescriptions[0].direction = Direction::RECVONLY; // media override
    // [1] and [2] inherit / default
    REQUIRE(sd.effectiveDirection(0) == Direction::RECVONLY);
    REQUIRE(sd.effectiveDirection(1) == Direction::SENDONLY);
    sd.direction.reset();                        // no session-level now
    REQUIRE(sd.effectiveDirection(2) == Direction::SENDRECV);
  }
}

// const-correctness: effectiveDirection is usable on a const reference.
TEST_CASE("effectiveDirection: callable on a const SessionDescription",
          "[session][effectiveDirection]")
{
  const SessionDescription sd = parseOk(kHdr + "a=inactive\r\n" +
                                        "m=audio 9 RTP/AVP 0\r\n");
  REQUIRE(sd.effectiveDirection(0) == Direction::INACTIVE);
}

// ---------------------------------------------------------------------------
// Offer / answer
// ---------------------------------------------------------------------------

// (d) createAnswer on a session-level-sendonly offer -> answer is EXACTLY
//     recvonly (mutation-tested against the pre-fix SENDRECV behavior).
TEST_CASE("createAnswer (d): session-level sendonly offer answered exactly recvonly",
          "[offer_answer][effectiveDirection]")
{
  auto offer = parseOk(kHdr + "a=sendonly\r\n" + "m=audio 9 RTP/AVP 0\r\n");
  auto answer = OfferAnswer::createAnswer(offer);
  REQUIRE(answer.mediaDescriptions.size() == 1);
  REQUIRE(answer.mediaDescriptions[0].direction.has_value());
  REQUIRE(*answer.mediaDescriptions[0].direction == Direction::RECVONLY);
}

// (h) multi-section answer flip: session a=sendonly + media[1] a=recvonly ->
//     answer [0] RECVONLY, [1] SENDONLY (each flipped from its EFFECTIVE value).
TEST_CASE("createAnswer (h): multi-section independent flip of effective direction",
          "[offer_answer][effectiveDirection]")
{
  auto offer = parseOk(kHdr + "a=sendonly\r\n" +
                       "m=audio 9 RTP/AVP 0\r\n" +
                       "m=video 9 RTP/AVP 96\r\n" +
                       "a=recvonly\r\n");
  auto answer = OfferAnswer::createAnswer(offer);
  REQUIRE(answer.mediaDescriptions.size() == 2);
  REQUIRE(*answer.mediaDescriptions[0].direction == Direction::RECVONLY);
  REQUIRE(*answer.mediaDescriptions[1].direction == Direction::SENDONLY);
}

// (n) port-0 offered section -> answered port 0 + INACTIVE, with a valid m=
//     line (media/proto echoed, >=1 format token). Mutation-tested against the
//     pre-fix active-answer behavior; the guard dominates inherited direction.
TEST_CASE("createAnswer (n): port-0 offered section answered port-0 + INACTIVE, valid m=",
          "[offer_answer][effectiveDirection]")
{
  auto offer = parseOk(kHdr + "a=sendonly\r\n" + "m=audio 0 RTP/AVP 0\r\n");
  auto answer = OfferAnswer::createAnswer(offer);
  REQUIRE(answer.mediaDescriptions.size() == 1);
  const auto& am = answer.mediaDescriptions[0];
  REQUIRE(am.port == 0);
  REQUIRE(am.direction.has_value());
  REQUIRE(*am.direction == Direction::INACTIVE);        // dominates inherited sendonly
  REQUIRE(am.mediaType == MediaType::AUDIO);             // media echoed
  REQUIRE(am.protocol == TransportProtocol::RTP_AVP);    // proto echoed
  REQUIRE(!am.formats.empty());                          // RFC 4566 m= grammar: >=1 <fmt>
  // The serialized m= line is syntactically valid (port 0, proto, >=1 fmt).
  std::string wire = SdpSerializer::serialize(answer);
  REQUIRE(wire.find("m=audio 0 RTP/AVP 0") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Round-trip + hold
// ---------------------------------------------------------------------------

// (m2) parser double-store guard: a direction line lands ONLY in the typed
//      field, never also as a GenericAttribute (session AND media level).
TEST_CASE("parse (m2): direction not double-stored as a GenericAttribute",
          "[parser][effectiveDirection]")
{
  SECTION("session level")
  {
    auto sd = parseOk(kHdr + "a=sendonly\r\n" + "m=audio 9 RTP/AVP 0\r\n");
    REQUIRE(sd.direction.has_value());
    REQUIRE(*sd.direction == Direction::SENDONLY);
    for (const auto& a : sd.attributes)
    {
      REQUIRE(a.name != "sendonly");
    }
    REQUIRE(sd.attributes.empty());
  }
  SECTION("media level")
  {
    auto sd = parseOk(kHdr + "m=audio 9 RTP/AVP 0\r\n" + "a=recvonly\r\n");
    REQUIRE(*sd.mediaDescriptions[0].direction == Direction::RECVONLY);
    for (const auto& a : sd.mediaDescriptions[0].genericAttributes)
    {
      REQUIRE(a.name != "recvonly");
    }
  }
}

// (m3) session-level round-trip: a session-level direction survives at session
//      scope, un-duplicated, no media section gains a fabricated a=sendrecv, the
//      model round-trips, and the a= line sits after t= and before the first m=.
TEST_CASE("round-trip (m3): session-level direction survives at session scope",
          "[serializer][round_trip][effectiveDirection]")
{
  auto sd = parseOk(kHdr + "a=recvonly\r\n" +
                    "m=audio 9 RTP/AVP 0\r\n" +
                    "m=video 9 RTP/AVP 96\r\n");
  std::string wire = SdpSerializer::serialize(sd);

  // Exactly one session-level direction line; no fabricated media a=sendrecv.
  REQUIRE(wire.find("a=recvonly") != std::string::npos);
  REQUIRE(wire.find("a=sendrecv") == std::string::npos);
  // Only one direction line total (the session-level recvonly).
  REQUIRE(wire.find("a=recvonly") == wire.rfind("a=recvonly"));

  // Textual order: a=recvonly after t=, before the first m= (RFC 4566 §5).
  auto posT = wire.find("t=");
  auto posDir = wire.find("a=recvonly");
  auto posM = wire.find("m=");
  REQUIRE(posT != std::string::npos);
  REQUIRE(posDir != std::string::npos);
  REQUIRE(posM != std::string::npos);
  REQUIRE(posT < posDir);
  REQUIRE(posDir < posM);

  // Model round-trips (guards the SessionDescription::operator== direction term).
  auto reparsed = parseOk(wire);
  REQUIRE(reparsed == sd);
  REQUIRE(reparsed.direction.has_value());
  REQUIRE(*reparsed.direction == Direction::RECVONLY);
  REQUIRE(!reparsed.mediaDescriptions[0].direction.has_value());
}

// (m4) duplicate session-level direction: last-wins in lenient mode; a strict
//      parse rejects it (mirroring the ice-ufrag/setup duplicate pattern).
TEST_CASE("parse (m4): duplicate session-level direction — last-wins / strict-reject",
          "[parser][effectiveDirection]")
{
  const std::string dup = kHdr + "a=sendonly\r\n" + "a=inactive\r\n" +
                          "m=audio 9 RTP/AVP 0\r\n";
  SECTION("lenient: last wins, with a warning")
  {
    auto result = SdpParser::parse(dup);
    REQUIRE(result.hasValue());
    REQUIRE(result.value->direction.has_value());
    REQUIRE(*result.value->direction == Direction::INACTIVE);
    REQUIRE(!result.warnings.empty());
  }
  SECTION("strict: rejected")
  {
    SdpParserOptions opts;
    opts.strict = true;
    auto result = SdpParser::parse(dup, opts);
    REQUIRE(result.hasError());
  }
}

// (j) combined hold form: a live (non-zero) c= with a session-level a=inactive
//     -> effectiveConnection is a live address while effectiveDirection is
//     INACTIVE (RFC 3264 §8.4 modern hold; direction is authoritative).
TEST_CASE("(j): live c= + session a=inactive — direction authoritative for hold",
          "[session][effectiveDirection][effectiveConnection]")
{
  auto sd = parseOk(
    "v=0\r\n"
    "o=- 0 0 IN IP4 192.0.2.1\r\n"
    "s=-\r\n"
    "c=IN IP4 192.0.2.1\r\n"
    "t=0 0\r\n"
    "a=inactive\r\n"
    "m=audio 9 RTP/AVP 0\r\n");
  auto conn = sd.effectiveConnection(0);
  REQUIRE(conn.has_value());
  REQUIRE(conn->address == "192.0.2.1");          // address looks live
  REQUIRE(sd.effectiveDirection(0) == Direction::INACTIVE); // yet held by direction
}

// (k) resume: zero direction attributes anywhere -> effectiveDirection is
//     SENDRECV for every in-range index (the hold->resume transition; only
//     passes under Option B).
TEST_CASE("(k): zero direction attributes -> SENDRECV resume for all media",
          "[session][effectiveDirection]")
{
  auto sd = parseOk(kHdr + "m=audio 9 RTP/AVP 0\r\n" +
                    "m=video 9 RTP/AVP 96\r\n");
  for (std::size_t i = 0; i < sd.mediaDescriptions.size(); ++i)
  {
    REQUIRE(sd.effectiveDirection(i) == Direction::SENDRECV);
  }
}

// operator== directly discriminates the session-level direction term (guards the
// added `direction == other.direction` term, which m3's round-trip cannot — both
// its operands carry the same value). Steps-4-8 cpp17 MEDIUM-1.
TEST_CASE("SessionDescription::operator== discriminates the direction term",
          "[session][effectiveDirection]")
{
  SessionDescription a;
  SessionDescription b;
  REQUIRE(a == b);                       // both nullopt
  a.direction = Direction::SENDONLY;
  REQUIRE(a != b);                       // set vs nullopt
  b.direction = Direction::INACTIVE;
  REQUIRE(a != b);                       // differing values
  b.direction = Direction::SENDONLY;
  REQUIRE(a == b);                       // equal values
}

// createAnswer active-branch identity flips: inactive->inactive and
// (absent=)sendrecv->sendrecv must resolve through the ACTIVE path (port 9),
// not the port-0/reject INACTIVE paths. Steps-4-8 sip #3.
TEST_CASE("createAnswer: active-branch identity flips (inactive, sendrecv)",
          "[offer_answer][effectiveDirection]")
{
  SECTION("session a=inactive offer -> answer INACTIVE on the active branch")
  {
    auto offer = parseOk(kHdr + "a=inactive\r\n" + "m=audio 9 RTP/AVP 0\r\n");
    auto answer = OfferAnswer::createAnswer(offer);
    REQUIRE(answer.mediaDescriptions[0].port == 9);   // active path, not port-0
    REQUIRE(*answer.mediaDescriptions[0].direction == Direction::INACTIVE);
  }
  SECTION("absent-direction offer -> answer SENDRECV")
  {
    auto offer = parseOk(kHdr + "m=audio 9 RTP/AVP 0\r\n");
    auto answer = OfferAnswer::createAnswer(offer);
    REQUIRE(answer.mediaDescriptions[0].port == 9);
    REQUIRE(*answer.mediaDescriptions[0].direction == Direction::SENDRECV);
  }
}

// Media-level direction parsing is case-insensitive (parity with the session
// level via directionFromString) and detects duplicates. Steps-4-8 cpp17 LOW-1 /
// sip #1 / #2.
TEST_CASE("parse: media-level direction is case-insensitive and duplicate-guarded",
          "[parser][effectiveDirection]")
{
  SECTION("mixed-case a=SendOnly parses as SENDONLY at media level")
  {
    auto sd = parseOk(kHdr + "m=audio 9 RTP/AVP 0\r\n" + "a=SendOnly\r\n");
    REQUIRE(sd.mediaDescriptions[0].direction.has_value());
    REQUIRE(*sd.mediaDescriptions[0].direction == Direction::SENDONLY);
  }
  SECTION("duplicate media-level direction: lenient last-wins + warning")
  {
    auto result = SdpParser::parse(kHdr + "m=audio 9 RTP/AVP 0\r\n" +
                                   "a=sendonly\r\n" + "a=inactive\r\n");
    REQUIRE(result.hasValue());
    REQUIRE(*result.value->mediaDescriptions[0].direction == Direction::INACTIVE);
    REQUIRE(!result.warnings.empty());
  }
  SECTION("duplicate media-level direction: strict rejects")
  {
    SdpParserOptions opts;
    opts.strict = true;
    auto result = SdpParser::parse(kHdr + "m=audio 9 RTP/AVP 0\r\n" +
                                   "a=sendonly\r\n" + "a=inactive\r\n", opts);
    REQUIRE(result.hasError());
  }
}

#include <catch2/catch.hpp>
#include <string>

#include "iora/sdp/offer_answer.hpp"
#include "iora/sdp/sdp_parser.hpp"
#include "iora/sdp/sdp_serializer.hpp"

using namespace iora::sdp;

TEST_CASE("Re-offer answer test scaffold compiles", "[reoffer_answer][scaffold]")
{
  REQUIRE(true);
}

// ---------------------------------------------------------------------------
// RA-T-23: nextSessionVersion (RA-SC-6)
// ---------------------------------------------------------------------------

TEST_CASE("nextSessionVersion adds one with carry and rejects non-digits", "[reoffer_answer][next_session_version]")
{
  REQUIRE(OfferAnswer::nextSessionVersion("0") == std::optional<std::string>("1"));
  REQUIRE(OfferAnswer::nextSessionVersion("9") == std::optional<std::string>("10"));
  REQUIRE(OfferAnswer::nextSessionVersion("099") == std::optional<std::string>("100"));
  REQUIRE(OfferAnswer::nextSessionVersion("0099") == std::optional<std::string>("0100"));
  REQUIRE(OfferAnswer::nextSessionVersion("18446744073709551615")
          == std::optional<std::string>("18446744073709551616"));
  REQUIRE_FALSE(OfferAnswer::nextSessionVersion("").has_value());
  REQUIRE_FALSE(OfferAnswer::nextSessionVersion("12a").has_value());
  REQUIRE_FALSE(OfferAnswer::nextSessionVersion("-1").has_value());
  REQUIRE_FALSE(OfferAnswer::nextSessionVersion(" 1").has_value());
}

// ---------------------------------------------------------------------------
// RA-T-24: equalIgnoringOriginLine (RA-SC-12)
// ---------------------------------------------------------------------------

namespace {

const std::string LF_TEXT =
    "v=0\n"
    "o=- 1 1 IN IP4 192.0.2.1\n"
    "s=-\n"
    "c=IN IP4 192.0.2.1\n"
    "t=0 0\n"
    "m=audio 49170 RTP/AVP 0\n";

std::string replaceAll(std::string text, const std::string& from, const std::string& to)
{
  std::size_t pos = 0;
  while ((pos = text.find(from, pos)) != std::string::npos)
  {
    text.replace(pos, from.size(), to);
    pos += to.size();
  }
  return text;
}

} // namespace

TEST_CASE("equalIgnoringOriginLine compares SDP text without its o= line", "[reoffer_answer][equal_ignoring_origin]")
{
  SECTION("CRLF text equals the same text with LF")
  {
    REQUIRE(OfferAnswer::equalIgnoringOriginLine(replaceAll(LF_TEXT, "\n", "\r\n"), LF_TEXT));
  }
  SECTION("texts that differ only in the o= line are equal")
  {
    auto other = replaceAll(LF_TEXT, "o=- 1 1 IN IP4 192.0.2.1", "o=peer 77 2 IN IP4 198.51.100.7");
    REQUIRE(OfferAnswer::equalIgnoringOriginLine(LF_TEXT, other));
  }
  SECTION("a moved line makes the texts different")
  {
    const std::string moved =
        "v=0\n"
        "o=- 1 1 IN IP4 192.0.2.1\n"
        "s=-\n"
        "t=0 0\n"
        "c=IN IP4 192.0.2.1\n"
        "m=audio 49170 RTP/AVP 0\n";
    REQUIRE_FALSE(OfferAnswer::equalIgnoringOriginLine(LF_TEXT, moved));
  }
  SECTION("bare CR only equals LF")
  {
    REQUIRE(OfferAnswer::equalIgnoringOriginLine(replaceAll(LF_TEXT, "\n", "\r"), LF_TEXT));
  }
  SECTION("extra empty lines in the middle and at the end are skipped")
  {
    auto extra = replaceAll(LF_TEXT, "s=-\n", "s=-\n\n") + "\n";
    REQUIRE(OfferAnswer::equalIgnoringOriginLine(extra, LF_TEXT));
  }
  SECTION("a bare CR inside a line with a later LF stays in the line (splitLines rule)")
  {
    auto mixed = replaceAll(LF_TEXT, "s=-\n", "s=-\rx\n");
    auto lfOnly = replaceAll(LF_TEXT, "s=-\n", "s=-\nx\n");
    REQUIRE_FALSE(OfferAnswer::equalIgnoringOriginLine(mixed, lfOnly));
  }
  SECTION("a text with no o= line equals one with an o= line (no validity check)")
  {
    auto noOrigin = replaceAll(LF_TEXT, "o=- 1 1 IN IP4 192.0.2.1\n", "");
    REQUIRE(OfferAnswer::equalIgnoringOriginLine(noOrigin, LF_TEXT));
  }
}

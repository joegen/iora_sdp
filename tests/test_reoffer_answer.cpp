#include <catch2/catch.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "iora/sdp/offer_answer.hpp"
#include "iora/sdp/sdp_parser.hpp"
#include "iora/sdp/sdp_serializer.hpp"

using namespace iora::sdp;

// ---------------------------------------------------------------------------
// RA-T-23: nextSessionVersion (RA-SC-6)
// ---------------------------------------------------------------------------

TEST_CASE("nextSessionVersion adds one with carry and rejects non-digits",
          "[reoffer_answer][next_session_version]")
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

const std::string LF_TEXT
    = "v=0\n"
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

TEST_CASE("equalIgnoringOriginLine compares SDP text without its o= line",
          "[reoffer_answer][equal_ignoring_origin]")
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
    const std::string moved
        = "v=0\n"
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

// ---------------------------------------------------------------------------
// answerReofferFromOwnOffer helpers
// ---------------------------------------------------------------------------

namespace {

const std::string OWN_ORIGIN = "- 1 9 IN IP4 192.0.2.1";
const std::string PEER_ORIGIN = "peer 2 3 IN IP4 198.51.100.7";

std::string sdpWith(const std::string& origin, const std::string& body)
{
  return "v=0\r\no=" + origin + "\r\ns=-\r\nc=IN IP4 192.0.2.1\r\nt=0 0\r\n" + body;
}

SessionDescription mustParse(const std::string& text)
{
  auto result = SdpParser::parse(text);
  INFO(text);
  REQUIRE(result.hasValue());
  return *result.value;
}

std::optional<ReofferAnswer> answerFor(const std::string& reofferBody, const std::string& ownBody)
{
  auto reoffer = mustParse(sdpWith(PEER_ORIGIN, reofferBody));
  auto own = mustParse(sdpWith(OWN_ORIGIN, ownBody));
  return OfferAnswer::answerReofferFromOwnOffer(reoffer, own);
}

SessionDescription ownFrom(const std::string& ownBody)
{
  return mustParse(sdpWith(OWN_ORIGIN, ownBody));
}

void requireUnchanged(const std::optional<ReofferAnswer>& result, const std::string& ownBody)
{
  REQUIRE(result.has_value());
  REQUIRE_FALSE(result->differsFromOwnOffer);
  REQUIRE(result->answer == ownFrom(ownBody));
}

void requireChangedVersion(const std::optional<ReofferAnswer>& result)
{
  REQUIRE(result.has_value());
  REQUIRE(result->differsFromOwnOffer);
  REQUIRE(result->answer.origin.has_value());
  REQUIRE(result->answer.origin->sessVersion == "10");
}

bool hasRtpMap(const MediaDescription& media, int pt)
{
  for (const auto& map : media.rtpMaps)
  {
    if (map.payloadType == pt)
    {
      return true;
    }
  }
  return false;
}

bool hasFmtp(const MediaDescription& media, int pt)
{
  for (const auto& fmtp : media.fmtps)
  {
    if (fmtp.payloadType == pt)
    {
      return true;
    }
  }
  return false;
}

using Formats = std::vector<std::string>;

} // namespace

// ---------------------------------------------------------------------------
// RA-T-1 (NR-9, RA-SC-9), RA-T-2 (NR-1), RA-T-5 (RA-SC-2)
// ---------------------------------------------------------------------------

TEST_CASE("A BUNDLE re-offer is not answered", "[reoffer_answer][nr9]")
{
  const std::string own = "m=audio 5000 RTP/AVP 0\na=mid:0\nm=audio 5002 RTP/AVP 0\na=mid:1\n";
  REQUIRE_FALSE(answerFor("a=group:BUNDLE 0 1\n" + own, own).has_value());
  REQUIRE_FALSE(answerFor("a=group:bundle 0 1\n" + own, own).has_value());
}

TEST_CASE("The m-line shape must match (NR-1)", "[reoffer_answer][nr1]")
{
  const std::string two
      = "m=audio 5000 RTP/AVP 0\nm=video 5002 RTP/AVP 96\na=rtpmap:96 VP8/90000\n";
  REQUIRE_FALSE(answerFor("m=audio 5000 RTP/AVP 0\n", two).has_value());
  REQUIRE_FALSE(answerFor("m=audio 5000 RTP/AVP 0\nm=audio 5002 RTP/AVP 0\n", two).has_value());

  const std::string unknown = "m=foo 5000 RTP/AVP 0\n";
  requireUnchanged(answerFor(unknown, unknown), unknown);
  REQUIRE_FALSE(answerFor("m=bar 5000 RTP/AVP 0\n", unknown).has_value());
}

TEST_CASE("An own line on port 0 with a live re-offer line has no answer (RA-SC-2)",
          "[reoffer_answer][rasc2]")
{
  REQUIRE_FALSE(answerFor("m=audio 5000 RTP/AVP 0\n", "m=audio 0 RTP/AVP 0\n").has_value());
}

// ---------------------------------------------------------------------------
// RA-T-3 (NR-2) and RA-T-4 (hoist regression)
// ---------------------------------------------------------------------------

TEST_CASE("A re-offer line on port 0 gets the reject shape (NR-2)", "[reoffer_answer][nr2]")
{
  const std::string audio = "m=audio 5000 RTP/AVP 0\na=mid:a\n";
  const std::string ownVideo
      = "m=video 5002 RTP/SAVP 96\n"
        "a=mid:v\n"
        "a=rtpmap:96 H264/90000\n"
        "a=fmtp:96 packetization-mode=1\n"
        "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:WVNfX19zZW1jdGwgKCkgewkyMjA7fQp9CnVubGVz\n"
        "a=setup:actpass\n"
        "a=candidate:1 1 udp 2130706431 192.0.2.1 5002 typ host\n";
  const std::string reVideo = "m=video 0 RTP/SAVP 96\na=mid:v\n";

  auto result = answerFor(audio + reVideo, audio + ownVideo);
  requireChangedVersion(result);

  MediaDescription expected;
  expected.mediaType = MediaType::VIDEO;
  expected.protocol = TransportProtocol::RTP_SAVP;
  expected.port = 0;
  expected.formats = {"96"};
  expected.direction = Direction::INACTIVE;
  expected.mid = "v";
  REQUIRE(result->answer.mediaDescriptions[1] == expected);
  REQUIRE(result->answer.mediaDescriptions[0] == ownFrom(audio + ownVideo).mediaDescriptions[0]);
}

TEST_CASE("createAnswer still rejects with the same shape after the hoist",
          "[reoffer_answer][hoist]")
{
  const std::string offerText
      = sdpWith(PEER_ORIGIN,
                "m=audio 5000 RTP/AVP 0\na=mid:0\n"
                "m=audio 5002 FOO/BAR 0\na=mid:1\n"
                "m=image 5004 udptl t38\na=mid:2\n"
                "m=audio 0 RTP/AVP 8\na=mid:3\n"
                "m=video 5006 RTP/AVP 96\na=mid:4\na=rtpmap:96 VP8/90000\n");
  auto offer = mustParse(offerText);
  AnswerOptions options;
  options.rejectMediaTypes = {MediaType::VIDEO};
  auto answer = OfferAnswer::createAnswer(offer, options);
  REQUIRE(answer.mediaDescriptions.size() == 5);

  for (std::size_t i = 1; i < 5; ++i)
  {
    const auto& off = offer.mediaDescriptions[i];
    MediaDescription expected;
    expected.mediaType = off.mediaType;
    expected.rawMediaType = off.rawMediaType;
    expected.protocol = off.protocol;
    expected.rawProtocol = off.rawProtocol;
    expected.port = 0;
    expected.formats = off.formats;
    expected.direction = Direction::INACTIVE;
    expected.mid = off.mid;
    INFO("line " << i);
    REQUIRE(answer.mediaDescriptions[i] == expected);
  }
}

// ---------------------------------------------------------------------------
// RA-T-6 .. RA-T-12 (NR-3, RA-SC-1, RA-SC-10, RA-SC-20)
// ---------------------------------------------------------------------------

TEST_CASE("Asymmetric payload numbers keep ownOffer's numbers (NR-3)", "[reoffer_answer][nr3]")
{
  const std::string own
      = "m=audio 5000 RTP/AVP 111 101\n"
        "a=rtpmap:111 opus/48000/2\na=fmtp:111 minptime=10;useinbandfec=1\n"
        "a=rtpmap:101 telephone-event/48000\na=fmtp:101 0-15\n";
  const std::string re
      = "m=audio 6000 RTP/AVP 96 100\n"
        "a=rtpmap:96 opus/48000/2\na=rtpmap:100 telephone-event/48000\n";
  requireUnchanged(answerFor(re, own), own);
}

TEST_CASE("Dropped formats lose their codec lines (NR-3)", "[reoffer_answer][nr3]")
{
  const std::string own
      = "m=audio 5000 RTP/AVP 0 8 18 101\n"
        "a=rtpmap:18 G729/8000\na=fmtp:18 annexb=no\n"
        "a=rtpmap:101 telephone-event/8000\na=fmtp:101 0-15\n"
        "a=rtcp-fb:* nack\n";
  const std::string re
      = "m=audio 6000 RTP/AVP 8 101\na=rtpmap:101 telephone-event/8000\na=rtcp-fb:* nack\n";
  auto result = answerFor(re, own);
  requireChangedVersion(result);
  const auto& line = result->answer.mediaDescriptions[0];
  REQUIRE(line.formats == Formats{"8", "101"});
  REQUIRE_FALSE(hasRtpMap(line, 18));
  REQUIRE_FALSE(hasFmtp(line, 18));
  REQUIRE(line.rtcpFeedbacks.size() == 1);
  REQUIRE(line.rtcpFeedbacks[0].payloadType == "*");
}

TEST_CASE("Several candidates are resolved by number, else no answer (NR-3)",
          "[reoffer_answer][nr3]")
{
  const std::string own
      = "m=audio 5000 RTP/AVP 96 97\n"
        "a=rtpmap:96 AMR-WB/16000/1\na=fmtp:96 octet-align=0\n"
        "a=rtpmap:97 AMR-WB/16000/1\na=fmtp:97 octet-align=1\n";
  auto kept = answerFor(
      "m=audio 6000 RTP/AVP 97\na=rtpmap:97 AMR-WB/16000/1\na=fmtp:97 octet-align=1\n", own);
  requireChangedVersion(kept);
  REQUIRE(kept->answer.mediaDescriptions[0].formats == Formats{"97"});
  REQUIRE(hasFmtp(kept->answer.mediaDescriptions[0], 97));
  REQUIRE_FALSE(hasFmtp(kept->answer.mediaDescriptions[0], 96));

  REQUIRE_FALSE(
      answerFor("m=audio 6000 RTP/AVP 100\na=rtpmap:100 AMR-WB/16000/1\n", own).has_value());

  const std::string h264
      = "m=video 5002 RTP/AVP 97 98\n"
        "a=rtpmap:97 H264/90000\na=fmtp:97 packetization-mode=0\n"
        "a=rtpmap:98 H264/90000\na=fmtp:98 packetization-mode=1\n";
  auto video = answerFor(
      "m=video 6002 RTP/AVP 98\na=rtpmap:98 H264/90000\na=fmtp:98 packetization-mode=1\n", h264);
  requireChangedVersion(video);
  REQUIRE(video->answer.mediaDescriptions[0].formats == Formats{"98"});
}

TEST_CASE("One own format answers one re-offer format (NR-3)", "[reoffer_answer][nr3]")
{
  REQUIRE_FALSE(
      answerFor("m=audio 6000 RTP/AVP 96 97\na=rtpmap:96 opus/48000/2\na=rtpmap:97 opus/48000/2\n",
                "m=audio 5000 RTP/AVP 111\na=rtpmap:111 opus/48000/2\n")
          .has_value());
  REQUIRE_FALSE(
      answerFor("m=audio 6000 RTP/AVP 96\na=rtpmap:96 iLBC/8000\n", "m=audio 5000 RTP/AVP 0\n")
          .has_value());
}

TEST_CASE(
    "Format identity without codec entries and by static number (RA-SC-1, RA-SC-10, RA-SC-20)",
    "[reoffer_answer][nr3][rasc1][rasc20]")
{
  SECTION("(a) own rtpmap 18, re-offer bare 18")
  {
    const std::string own = "m=audio 5000 RTP/AVP 18\na=rtpmap:18 G729/8000\n";
    requireUnchanged(answerFor("m=audio 6000 RTP/AVP 18\n", own), own);
  }
  SECTION("(b) own bare 18, re-offer with rtpmap")
  {
    const std::string own = "m=audio 5000 RTP/AVP 18\n";
    requireUnchanged(answerFor("m=audio 6000 RTP/AVP 18\na=rtpmap:18 G729/8000\n", own), own);
  }
  SECTION("(c) m=image t38 has no answer (RA-SC-18)")
  {
    const std::string image = "m=image 5004 udptl t38\n";
    REQUIRE_FALSE(answerFor(image, image).has_value());
  }
  SECTION("(d) data channel token")
  {
    const std::string own = "m=application 5006 UDP/DTLS/SCTP webrtc-datachannel\n";
    requireUnchanged(answerFor(own, own), own);
  }
  SECTION("(e) dynamic 96 with no rtpmap on either side")
  {
    const std::string own = "m=video 5002 RTP/AVP 96\n";
    requireUnchanged(answerFor(own, own), own);
  }
  SECTION("(f) repeated re-offer tokens are used once")
  {
    const std::string own = "m=audio 5000 RTP/AVP 0 8\n";
    requireUnchanged(answerFor("m=audio 6000 RTP/AVP 8 8 0\n", own), own);
  }
  SECTION("(g) repeated own token, narrowed")
  {
    auto result = answerFor("m=audio 6000 RTP/AVP 8\n", "m=audio 5000 RTP/AVP 0 0 8\n");
    requireChangedVersion(result);
    REQUIRE(result->answer.mediaDescriptions[0].formats == Formats{"8"});
  }
  SECTION("(h) two tokens with no payload type do not match")
  {
    REQUIRE_FALSE(answerFor("m=application 6006 UDP/DTLS/SCTP other-token\n",
                            "m=application 5006 UDP/DTLS/SCTP webrtc-datachannel\n")
                      .has_value());
  }
  SECTION("(i) static 18 matches across G729 and G729a")
  {
    const std::string own = "m=audio 5000 RTP/AVP 0 18\na=rtpmap:18 G729/8000\n";
    requireUnchanged(answerFor("m=audio 6000 RTP/AVP 0 18\na=rtpmap:18 G729a/8000\n", own), own);
  }
  SECTION("(j) static 9 matches across G722/8000 and G722/16000")
  {
    const std::string own = "m=audio 5000 RTP/AVP 0 9\n";
    requireUnchanged(answerFor("m=audio 6000 RTP/AVP 0 9\na=rtpmap:9 G722/16000\n", own), own);
  }
  SECTION("(k) dynamic 96 does not decide identity")
  {
    auto result = answerFor("m=audio 6000 RTP/AVP 0 96\na=rtpmap:96 iLBC/8000\n",
                            "m=audio 5000 RTP/AVP 0 96\na=rtpmap:96 opus/48000/2\n");
    requireChangedVersion(result);
    REQUIRE(result->answer.mediaDescriptions[0].formats == Formats{"0"});
    REQUIRE_FALSE(hasRtpMap(result->answer.mediaDescriptions[0], 96));
  }
  SECTION("(l) 35 is not a static number")
  {
    auto result = answerFor("m=audio 6000 RTP/AVP 0 35\na=rtpmap:35 iLBC/8000\n",
                            "m=audio 5000 RTP/AVP 0 35\na=rtpmap:35 opus/48000/2\n");
    requireChangedVersion(result);
    REQUIRE(result->answer.mediaDescriptions[0].formats == Formats{"0"});
    REQUIRE_FALSE(hasRtpMap(result->answer.mediaDescriptions[0], 35));
  }
  SECTION("(m) the peer numbered speex its own way")
  {
    auto result = answerFor(
        "m=audio 6000 RTP/AVP 0 40\na=rtpmap:40 speex/8000\n",
        "m=audio 5000 RTP/AVP 0 40 41\na=rtpmap:40 iLBC/8000\na=rtpmap:41 speex/8000\n");
    requireChangedVersion(result);
    REQUIRE(result->answer.mediaDescriptions[0].formats == Formats{"0", "41"});
    REQUIRE_FALSE(hasRtpMap(result->answer.mediaDescriptions[0], 40));
  }
}

// ---------------------------------------------------------------------------
// RA-T-22 (NR-8, RA-SC-8, RA-SC-6 in S8)
// ---------------------------------------------------------------------------

TEST_CASE("The o= version rises by one only when the answer differs (NR-8)",
          "[reoffer_answer][nr8]")
{
  const std::string own = "m=audio 5000 RTP/AVP 0 8\n";
  const std::string narrowed = "m=audio 6000 RTP/AVP 8\n";

  auto changed = answerFor(narrowed, own);
  requireChangedVersion(changed);
  const auto& origin = *changed->answer.origin;
  const auto ownOrigin = *ownFrom(own).origin;
  REQUIRE(origin.username == ownOrigin.username);
  REQUIRE(origin.sessId == ownOrigin.sessId);
  REQUIRE(origin.netType == ownOrigin.netType);
  REQUIRE(origin.addrType == ownOrigin.addrType);
  REQUIRE(origin.address == ownOrigin.address);

  requireUnchanged(answerFor(own, own), own);

  SessionDescription noOrigin = ownFrom(own);
  noOrigin.origin.reset();
  REQUIRE_FALSE(
      OfferAnswer::answerReofferFromOwnOffer(mustParse(sdpWith(PEER_ORIGIN, own)), noOrigin)
          .has_value());

  SessionDescription badVersion = ownFrom(own);
  badVersion.origin->sessVersion = "1a";
  REQUIRE_FALSE(
      OfferAnswer::answerReofferFromOwnOffer(mustParse(sdpWith(PEER_ORIGIN, narrowed)), badVersion)
          .has_value());
  auto same
      = OfferAnswer::answerReofferFromOwnOffer(mustParse(sdpWith(PEER_ORIGIN, own)), badVersion);
  REQUIRE(same.has_value());
  REQUIRE_FALSE(same->differsFromOwnOffer);
  REQUIRE(same->answer == badVersion);
}

// ---------------------------------------------------------------------------
// RA-T-27 (RA-SC-13), RA-T-29 (RA-SC-15)
// ---------------------------------------------------------------------------

TEST_CASE("Protocol and mid must match on a kept line (RA-SC-13)", "[reoffer_answer][rasc13]")
{
  REQUIRE_FALSE(answerFor("m=audio 6000 RTP/SAVP 0\na=crypto:1 AES_CM_128_HMAC_SHA1_80 "
                          "inline:WVNfX19zZW1jdGwgKCkgewkyMjA7fQp9CnVubGVz\n",
                          "m=audio 5000 RTP/AVP 0\n")
                    .has_value());
  REQUIRE_FALSE(answerFor("m=audio 6000 FOO/BAR 0\n", "m=audio 5000 FOO/BAZ 0\n").has_value());
  REQUIRE_FALSE(answerFor("m=audio 6000 RTP/AVP 0\na=mid:b\n", "m=audio 5000 RTP/AVP 0\na=mid:a\n")
                    .has_value());

  const std::string withMid = "m=audio 5000 RTP/AVP 0\na=mid:a\n";
  auto noReMid = answerFor("m=audio 6000 RTP/AVP 0\n", withMid);
  requireUnchanged(noReMid, withMid);
  REQUIRE(noReMid->answer.mediaDescriptions[0].mid == std::optional<std::string>("a"));

  REQUIRE_FALSE(
      answerFor("m=audio 6000 RTP/AVP 0\na=mid:a\n", "m=audio 5000 RTP/AVP 0\n").has_value());

  auto portZero
      = answerFor("m=audio 6000 RTP/AVP 0\nm=video 0 RTP/SAVP 96\n",
                  "m=audio 5000 RTP/AVP 0\nm=video 5002 RTP/AVP 96\na=rtpmap:96 VP8/90000\n");
  requireChangedVersion(portZero);
  REQUIRE(portZero->answer.mediaDescriptions[1].port == 0);
  REQUIRE(portZero->answer.mediaDescriptions[1].protocol == TransportProtocol::RTP_SAVP);
}

TEST_CASE("Unchanged values are not rewritten (RA-SC-15)", "[reoffer_answer][rasc15]")
{
  const std::string orphan = "m=audio 5000 RTP/AVP 0 8\na=rtpmap:18 G729/8000\n";
  requireUnchanged(answerFor("m=audio 6000 RTP/AVP 0 8\n", orphan), orphan);

  const std::string repeated = "m=audio 5000 RTP/AVP 0 0 8\n";
  requireUnchanged(answerFor("m=audio 6000 RTP/AVP 0 8\n", repeated), repeated);

  const std::string portZero
      = "m=audio 5000 RTP/AVP 0\nm=video 0 RTP/AVP 96\na=rtpmap:96 VP8/90000\na=inactive\n";
  requireUnchanged(answerFor("m=audio 6000 RTP/AVP 0\nm=video 0 RTP/AVP 96\n", portZero), portZero);
}

// ---------------------------------------------------------------------------
// RA-T-31 session cases (b), (f), (i) (RA-SC-17 (ii))
// ---------------------------------------------------------------------------

TEST_CASE("Session-level capability negotiation or key-mgmt has no answer (RA-SC-17)",
          "[reoffer_answer][rasc17]")
{
  const std::string line = "m=audio 5000 RTP/AVP 0\n";
  REQUIRE_FALSE(answerFor(line, "a=tcap:1 RTP/SAVP\n" + line).has_value());
  REQUIRE_FALSE(answerFor(line, "a=pcfg:1 t=1\n" + line).has_value());
  REQUIRE_FALSE(
      answerFor(line, "a=key-mgmt:mikey AQAFgM0XflABAAAAAAAAAAAAAAsAyO\n" + line).has_value());
}

// ---------------------------------------------------------------------------
// RA-T-32 (RA-SC-18, HD-MDS0-1) and RA-T-34 (RA-RK-4 pin, HD-MDS0-2)
// ---------------------------------------------------------------------------

TEST_CASE("An own m=image line that stays live has no answer (RA-SC-18)",
          "[reoffer_answer][rasc18]")
{
  const std::string ownImage = "m=image 5004 udptl t38\na=T38FaxVersion:3\na=T38MaxBitRate:33600\n";
  REQUIRE_FALSE(
      answerFor("m=image 6004 udptl t38\na=T38FaxVersion:0\na=T38MaxBitRate:14400\n", ownImage)
          .has_value());

  const std::string audio = "m=audio 5000 RTP/AVP 0\n";
  REQUIRE_FALSE(answerFor(audio + "m=image 6004 udptl t38\n", audio + "m=image 5004 udptl t38\n")
                    .has_value());

  const std::string bothZero = audio + "m=image 0 udptl t38\n";
  requireUnchanged(answerFor(bothZero, bothZero), bothZero);

  auto rejected = answerFor(audio + "m=image 0 udptl t38\n", audio + ownImage);
  requireChangedVersion(rejected);
  REQUIRE(rejected->answer.mediaDescriptions[1].port == 0);
  REQUIRE(rejected->answer.mediaDescriptions[1].genericAttributes.empty());
}

TEST_CASE("The H.264 level stays ownOffer's (known gap RA-RK-4)", "[reoffer_answer][rark4]")
{
  const std::string own
      = "m=video 5002 RTP/AVP 97\na=rtpmap:97 H264/90000\n"
        "a=fmtp:97 profile-level-id=42e01f;packetization-mode=1\n";
  const std::string re
      = "m=video 6002 RTP/AVP 97\na=rtpmap:97 H264/90000\n"
        "a=fmtp:97 profile-level-id=42e01e;packetization-mode=1\n";
  requireUnchanged(answerFor(re, own), own);
}

// ---------------------------------------------------------------------------
// Task 1.3a rows: RA-T-15 (NR-5), RA-T-18 (NR-7 media), RA-T-19 (groups),
// RA-T-20 (DQ-MD-5), RA-T-21 (kept fields), RA-T-30 (RA-SC-16),
// RA-T-31 line cases (RA-SC-17), RA-T-33 (RA-SC-19)
// ---------------------------------------------------------------------------

namespace {

const std::string KEY1 = "inline:MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkw";
const std::string KEY2 = "inline:QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVphYmNk";
const std::string KEY3 = "inline:ZmVkY2JhOTg3NjU0MzIxMGZlZGNiYTk4NzY1NDMy";
const std::string FINGERPRINT
    = "a=fingerprint:sha-256 "
      "AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:"
      "67:89\n";

} // namespace

TEST_CASE("SDES picks the own key the re-offer kept (NR-5, RA-SC-5)", "[reoffer_answer][nr5]")
{
  const std::string own =
      "m=audio 5000 RTP/SAVP 0\n"
      "a=crypto:1 AES_CM_128_HMAC_SHA1_80 " + KEY1 + "\n"
      "a=crypto:2 AES_CM_128_HMAC_SHA1_32 " + KEY2 + "\n"
      "a=crypto:3 AES_256_CM_HMAC_SHA1_80 " + KEY3 + "\n";
  auto tag2 = answerFor(
      "m=audio 6000 RTP/SAVP 0\na=crypto:2 AES_CM_128_HMAC_SHA1_32 " + KEY3 + "\n", own);
  requireChangedVersion(tag2);
  REQUIRE(tag2->answer.mediaDescriptions[0].crypto.size() == 1);
  REQUIRE(tag2->answer.mediaDescriptions[0].crypto[0]
          == ownFrom(own).mediaDescriptions[0].crypto[1]);

  auto none = answerFor("m=audio 6000 RTP/SAVP 0\n", own);
  requireChangedVersion(none);
  REQUIRE(none->answer.mediaDescriptions[0].crypto.empty());

  REQUIRE_FALSE(
      answerFor("m=audio 6000 RTP/SAVP 0\na=crypto:7 AES_CM_128_HMAC_SHA1_80 " + KEY1 + "\n", own)
          .has_value());

  auto firstMatch = answerFor(
      "m=audio 6000 RTP/SAVP 0\na=crypto:7 AES_CM_128_HMAC_SHA1_80 " + KEY1 + "\n"
      "a=crypto:3 AES_256_CM_HMAC_SHA1_80 " + KEY1 + "\n", own);
  requireChangedVersion(firstMatch);
  REQUIRE(firstMatch->answer.mediaDescriptions[0].crypto.size() == 1);
  REQUIRE(firstMatch->answer.mediaDescriptions[0].crypto[0].tag == 3);
  REQUIRE(firstMatch->answer.mediaDescriptions[0].crypto[0].keyParams == KEY3);

  // RFC 4568 section 5.1.2: the answer carries the tag AND the suite of the
  // accepted offer line, so a tag whose suite differs is not a match.
  REQUIRE_FALSE(
      answerFor("m=audio 6000 RTP/SAVP 0\na=crypto:2 AES_CM_128_HMAC_SHA1_80 " + KEY2 + "\n", own)
          .has_value());
  auto suiteThenMatch = answerFor(
      "m=audio 6000 RTP/SAVP 0\na=crypto:2 AES_CM_128_HMAC_SHA1_80 " + KEY2 + "\n"
      "a=crypto:1 aes_cm_128_hmac_sha1_80 " + KEY2 + "\n", own);
  requireChangedVersion(suiteThenMatch);
  REQUIRE(suiteThenMatch->answer.mediaDescriptions[0].crypto.size() == 1);
  REQUIRE(suiteThenMatch->answer.mediaDescriptions[0].crypto[0]
          == ownFrom(own).mediaDescriptions[0].crypto[0]);

  const std::string plain = "m=audio 5000 RTP/SAVP 0\n";
  requireUnchanged(
      answerFor("m=audio 6000 RTP/SAVP 0\na=crypto:1 AES_CM_128_HMAC_SHA1_80 " + KEY1 + "\n",
                plain),
      plain);
}

TEST_CASE("Media-level answer-only-if-offered fields (NR-7, RA-SC-7)", "[reoffer_answer][nr7]")
{
  SECTION("rtcp-mux")
  {
    const std::string own = "m=audio 5000 RTP/AVP 0\na=rtcp-mux\n";
    auto dropped = answerFor("m=audio 6000 RTP/AVP 0\n", own);
    requireChangedVersion(dropped);
    REQUIRE_FALSE(dropped->answer.mediaDescriptions[0].rtcpMux);
    requireUnchanged(answerFor(own, own), own);
  }
  SECTION("rtcp-mux-only is offer-only")
  {
    auto result = answerFor("m=audio 6000 RTP/AVP 0\na=rtcp-mux\n",
                            "m=audio 5000 RTP/AVP 0\na=rtcp-mux\na=rtcp-mux-only\n");
    requireChangedVersion(result);
    REQUIRE_FALSE(result->answer.mediaDescriptions[0].rtcpMuxOnly);
    REQUIRE(result->answer.mediaDescriptions[0].rtcpMux);
  }
  SECTION("rtcp-rsize")
  {
    auto result = answerFor("m=audio 6000 RTP/AVP 0\n", "m=audio 5000 RTP/AVP 0\na=rtcp-rsize\n");
    requireChangedVersion(result);
    REQUIRE_FALSE(result->answer.mediaDescriptions[0].rtcpRsize);
  }
  SECTION("extmap keeps only the URIs the re-offer has")
  {
    auto result = answerFor(
        "m=audio 6000 RTP/AVP 0\na=extmap:2 urn:b\n",
        "m=audio 5000 RTP/AVP 0\na=extmap:1 urn:a\na=extmap:2/sendonly urn:b\na=extmap:3 urn:c\n");
    requireChangedVersion(result);
    const auto& maps = result->answer.mediaDescriptions[0].extMaps;
    REQUIRE(maps.size() == 1);
    REQUIRE(maps[0].uri == "urn:b");
    REQUIRE(maps[0].id == 2);
    REQUIRE(maps[0].direction == std::optional<Direction>(Direction::SENDONLY));
  }
  SECTION("extmap-allow-mixed (i) both at session level")
  {
    const std::string own = "a=extmap-allow-mixed\nm=audio 5000 RTP/AVP 0\n";
    requireUnchanged(answerFor(own, own), own);
  }
  SECTION("extmap-allow-mixed (ii) own at session level, re only on line 0")
  {
    auto result = answerFor("m=audio 6000 RTP/AVP 0\na=extmap-allow-mixed\n",
                            "a=extmap-allow-mixed\nm=audio 5000 RTP/AVP 0\n");
    requireChangedVersion(result);
    REQUIRE_FALSE(result->answer.extmapAllowMixed);
    REQUIRE_FALSE(result->answer.mediaDescriptions[0].extmapAllowMixed);
  }
  SECTION("extmap-allow-mixed (iii) own on line 0, re at session level")
  {
    const std::string own = "m=audio 5000 RTP/AVP 0\na=extmap-allow-mixed\n";
    requireUnchanged(answerFor("a=extmap-allow-mixed\nm=audio 6000 RTP/AVP 0\n", own), own);
  }
  SECTION("extmap-allow-mixed (iv) own on line 0, re only on line 1")
  {
    auto result
        = answerFor("m=audio 6000 RTP/AVP 0\nm=audio 6002 RTP/AVP 8\na=extmap-allow-mixed\n",
                    "m=audio 5000 RTP/AVP 0\na=extmap-allow-mixed\nm=audio 5002 RTP/AVP 8\n");
    requireChangedVersion(result);
    REQUIRE_FALSE(result->answer.mediaDescriptions[0].extmapAllowMixed);
  }
  SECTION("simulcast and rid")
  {
    const std::string own
        = "m=video 5002 RTP/AVP 96\na=rtpmap:96 VP8/90000\na=rid:h send\na=rid:l "
          "send\na=simulcast:send h;l\n";
    auto removed = answerFor("m=video 6002 RTP/AVP 96\na=rtpmap:96 VP8/90000\n", own);
    requireChangedVersion(removed);
    REQUIRE_FALSE(removed->answer.mediaDescriptions[0].simulcast.has_value());
    REQUIRE(removed->answer.mediaDescriptions[0].rids.empty());
    REQUIRE_FALSE(
        answerFor(
            "m=video 6002 RTP/AVP 96\na=rtpmap:96 VP8/90000\na=rid:h recv\na=simulcast:recv h\n",
            own)
            .has_value());
    REQUIRE_FALSE(answerFor("m=video 6002 RTP/AVP 96\na=rtpmap:96 VP8/90000\na=rid:h recv\n", own)
                      .has_value());
  }
}

TEST_CASE("Groups follow the re-offer (NR-7, RA-SC-9)", "[reoffer_answer][groups]")
{
  const std::string lines
      = "m=audio 5000 RTP/AVP 0\na=mid:0\nm=video 5002 RTP/AVP 96\na=rtpmap:96 "
        "VP8/90000\na=mid:1\n";
  const std::string reLines
      = "m=audio 6000 RTP/AVP 0\na=mid:0\nm=video 6002 RTP/AVP 96\na=rtpmap:96 "
        "VP8/90000\na=mid:1\n";
  const std::string reLine1Zero
      = "m=audio 6000 RTP/AVP 0\na=mid:0\nm=video 0 RTP/AVP 96\na=mid:1\n";

  auto bundle = answerFor(reLines, "a=group:BUNDLE 0 1\n" + lines);
  requireChangedVersion(bundle);
  REQUIRE(bundle->answer.groups.empty());

  auto ls = answerFor("a=group:LS 0 1\n" + reLine1Zero, "a=group:LS 0 1\n" + lines);
  requireChangedVersion(ls);
  REQUIRE(ls->answer.groups.size() == 1);
  REQUIRE(ls->answer.groups[0].mids == std::vector<std::string>{"0"});

  auto fid = answerFor("a=group:LS 0 1\n" + reLines, "a=group:FID 0 1\n" + lines);
  requireChangedVersion(fid);
  REQUIRE(fid->answer.groups.empty());

  const std::string own = "a=group:LS 0 1\n" + lines;
  requireUnchanged(answerFor("a=group:ls 0 1\n" + reLines, own), own);

  const std::string three = "a=group:LS 0 1 2\n" + lines + "m=audio 5004 RTP/AVP 8\na=mid:2\n";
  auto empty = answerFor(
      "a=group:LS 0 1\nm=audio 0 RTP/AVP 0\na=mid:0\nm=video 0 RTP/AVP 96\na=mid:1\nm=audio 6004 "
      "RTP/AVP 8\na=mid:2\n",
      three);
  requireChangedVersion(empty);
  REQUIRE(empty->answer.groups.empty());
}

TEST_CASE("Unknown attributes are kept (DQ-MD-5 = A)", "[reoffer_answer][generic]")
{
  const std::string own = "a=x-session:2\nm=audio 5000 RTP/AVP 0\na=x-vendor:1\n";
  requireUnchanged(answerFor("m=audio 6000 RTP/AVP 0\n", own), own);
}

TEST_CASE("Every kept typed field stays as ownOffer has it (typedFieldTable)",
          "[reoffer_answer][kept]")
{
  const std::string own
      = "i=session info\n"
        "u=http://example.com/s\n"
        "e=ops@example.com\n"
        "p=+1 555 0100\n"
        "b=AS:256\n"
        "r=7d 1h 0 25h\n"
        "z=2882844526 -1h\n"
        "k=prompt\n"
        "a=ice-lite\n"
        "a=ice-ufrag:sess\n"
        "a=ice-pwd:sessionpassword0123456789\n"
        "a=ice-options:trickle\n"
        "a=fingerprint:sha-256 "
        "AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:"
        "67:89\n"
        "m=audio 5000/2 RTP/AVP 0\n"
        "c=IN IP4 192.0.2.9\n"
        "b=TIAS:64000\n"
        "k=prompt\n"
        "a=ssrc:1111 cname:own\n"
        "a=ssrc:2222 cname:own\n"
        "a=ssrc-group:FID 1111 2222\n"
        "a=msid:stream track\n"
        "a=candidate:1 1 udp 2130706431 192.0.2.9 5000 typ host\n"
        "a=ice-ufrag:line\n"
        "a=ice-pwd:linepassword0123456789ab\n"
        "a=ice-options:trickle\n"
        "a=end-of-candidates\n"
        "a=rtcp:5001\n"
        "a=ptime:20\n"
        "a=maxptime:40\n"
        "a=fingerprint:sha-256 "
        "01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:"
        "CD:EF\n"
        "m=video 5002 RTP/AVP 96\n"
        "a=rtpmap:96 VP8/90000\n"
        "a=framerate:30\n"
        "m=application 5006 UDP/DTLS/SCTP webrtc-datachannel\n"
        "a=sctp-port:5000\n"
        "a=max-message-size:262144\n";
  const std::string re
      = "a=ice-ufrag:peer\n"
        "a=ice-pwd:peerpassword0123456789abc\n"
        "m=audio 6000 RTP/AVP 0\n"
        "m=video 6002 RTP/AVP 96\n"
        "a=rtpmap:96 VP8/90000\n"
        "m=application 6006 UDP/DTLS/SCTP webrtc-datachannel\n";
  requireUnchanged(answerFor(re, own), own);
}

TEST_CASE("ICE fields are dropped when the re-offer shows no ICE (RA-SC-16)",
          "[reoffer_answer][rasc16]")
{
  const std::string own
      = "a=ice-lite\na=ice-ufrag:sess\na=ice-pwd:sessionpassword0123456789\na=ice-options:trickle\n"
        "m=audio 5000 RTP/AVP 0\n"
        "a=candidate:1 1 udp 2130706431 192.0.2.9 5000 typ host\na=end-of-candidates\n";
  auto requireIceCleared = [](const std::optional<ReofferAnswer>& result)
  {
    requireChangedVersion(result);
    const auto& session = result->answer;
    REQUIRE_FALSE(session.iceUfrag.has_value());
    REQUIRE_FALSE(session.icePwd.has_value());
    REQUIRE(session.iceOptions.empty());
    REQUIRE_FALSE(session.iceLite);
    REQUIRE(session.mediaDescriptions[0].candidates.empty());
    REQUIRE_FALSE(session.mediaDescriptions[0].endOfCandidates);
  };

  requireIceCleared(answerFor("m=audio 6000 RTP/AVP 0\n", own));
  requireUnchanged(
      answerFor("m=audio 6000 RTP/AVP 0\na=ice-ufrag:peer\na=ice-pwd:peerpassword0123456789abc\n",
                own),
      own);
  requireIceCleared(answerFor(
      "m=audio 6000 RTP/AVP 0\na=candidate:1 1 udp 2130706431 198.51.100.7 6000 typ host\n", own));
  requireIceCleared(answerFor("m=audio 6000 RTP/AVP 0\na=ice-ufrag:peer\n", own));
  requireUnchanged(
      answerFor("a=ice-ufrag:peer\nm=audio 6000 RTP/AVP 0\na=ice-pwd:peerpassword0123456789abc\n",
                own),
      own);

  const std::string withPortZero = own +
      "m=video 0 RTP/AVP 96\na=rtpmap:96 VP8/90000\n"
      "a=ice-ufrag:old\na=ice-pwd:oldpassword0123456789abcd\n"
      "a=candidate:1 1 udp 2130706431 192.0.2.9 5002 typ host\n";
  auto residual = answerFor("m=audio 6000 RTP/AVP 0\nm=video 0 RTP/AVP 96\n", withPortZero);
  requireIceCleared(residual);
  REQUIRE(residual->answer.mediaDescriptions[1] == ownFrom(withPortZero).mediaDescriptions[1]);
}

TEST_CASE("Line-level capability negotiation, key-mgmt and BFCP have no answer (RA-SC-17)",
          "[reoffer_answer][rasc17]")
{
  REQUIRE_FALSE(answerFor("m=application 6008 TCP/BFCP *\na=floorctrl:s-only\n",
                          "m=application 5008 TCP/BFCP *\na=floorctrl:c-only s-only\n")
                    .has_value());
  REQUIRE_FALSE(
      answerFor("m=audio 6000 RTP/AVP 0\n", "m=audio 5000 RTP/AVP 0\na=pcfg:1 t=1\n").has_value());

  auto rejected
      = answerFor("m=audio 6000 RTP/AVP 0\nm=application 0 TCP/BFCP *\n",
                  "m=audio 5000 RTP/AVP 0\nm=application 5008 TCP/BFCP *\na=floorctrl:c-only\n");
  requireChangedVersion(rejected);
  REQUIRE(rejected->answer.mediaDescriptions[1].port == 0);

  REQUIRE_FALSE(
      answerFor("m=audio 6000 RTP/AVP 0\n",
                "m=audio 5000 RTP/AVP 0\na=acap:1 crypto:1 AES_CM_128_HMAC_SHA1_80 " + KEY1 + "\n")
          .has_value());
  REQUIRE_FALSE(
      answerFor("m=application 6008 TCP/BFCP *\n", "m=application 5008 TCP/BFCP *\n").has_value());
  REQUIRE_FALSE(answerFor("m=application 6008 UDP/TLS/BFCP *\na=bfcpver:2\n",
                          "m=application 5008 UDP/TLS/BFCP *\na=bfcpver:1 2\n")
                    .has_value());
  REQUIRE_FALSE(
      answerFor("m=audio 6000 RTP/AVP 0\n",
                "m=audio 5000 RTP/AVP 0\na=key-mgmt:mikey AQAFgM0XflABAAAAAAAAAAAAAAsAyO\n")
          .has_value());
}

TEST_CASE("rtcp-fb of kept formats follows the re-offer (RA-SC-19)", "[reoffer_answer][rasc19]")
{
  auto narrowed = answerFor(
      "m=video 6002 RTP/AVPF 100\na=rtpmap:100 H264/90000\na=rtcp-fb:100 nack pli\na=rtcp-fb:100 "
      "ccm fir\n",
      "m=video 5002 RTP/AVPF 96\na=rtpmap:96 H264/90000\n"
      "a=rtcp-fb:96 nack\na=rtcp-fb:96 nack pli\na=rtcp-fb:96 ccm fir\na=rtcp-fb:96 "
      "goog-remb\na=rtcp-fb:96 transport-cc\n");
  requireChangedVersion(narrowed);
  const auto& feedbacks = narrowed->answer.mediaDescriptions[0].rtcpFeedbacks;
  REQUIRE(feedbacks.size() == 2);
  REQUIRE(feedbacks[0].payloadType == "96");
  REQUIRE(feedbacks[0].type == "nack");
  REQUIRE(feedbacks[0].subtype == std::optional<std::string>("pli"));
  REQUIRE(feedbacks[1].type == "ccm");

  const std::string own
      = "m=video 5002 RTP/AVPF 96 98\na=rtpmap:96 H264/90000\na=rtpmap:98 VP8/90000\na=rtcp-fb:* "
        "nack\n";
  const std::string reCodecs
      = "m=video 6002 RTP/AVPF 100 102\na=rtpmap:100 H264/90000\na=rtpmap:102 VP8/90000\n";
  auto starDropped = answerFor(reCodecs + "a=rtcp-fb:100 nack\n", own);
  requireChangedVersion(starDropped);
  REQUIRE(starDropped->answer.mediaDescriptions[0].rtcpFeedbacks.empty());
  requireUnchanged(answerFor(reCodecs + "a=rtcp-fb:* nack\n", own), own);
  requireUnchanged(answerFor(reCodecs + "a=rtcp-fb:100 nack\na=rtcp-fb:102 nack\n", own), own);
}

// ---------------------------------------------------------------------------
// Task 1.3b rows: RA-T-13, RA-T-14 (NR-4, RA-SC-4), RA-T-16, RA-T-17, RA-T-28
// (NR-6, RA-SC-3, RA-SC-4, RA-SC-14, tls-id)
// ---------------------------------------------------------------------------

TEST_CASE("Direction answers the re-offer (NR-4)", "[reoffer_answer][nr4]")
{
  auto directionOf = [](const std::optional<ReofferAnswer>& result)
  {
    REQUIRE(result.has_value());
    return *result->answer.effectiveDirection(0);
  };
  REQUIRE(directionOf(answerFor("m=audio 6000 RTP/AVP 0\na=sendonly\n",
                                "m=audio 5000 RTP/AVP 0\na=sendonly\n"))
          == Direction::INACTIVE);
  REQUIRE(directionOf(answerFor("m=audio 6000 RTP/AVP 0\na=inactive\n",
                                "m=audio 5000 RTP/AVP 0\na=sendrecv\n"))
          == Direction::INACTIVE);
  REQUIRE(directionOf(answerFor("m=audio 6000 RTP/AVP 0\na=sendrecv\n",
                                "m=audio 5000 RTP/AVP 0\na=sendrecv\n"))
          == Direction::SENDRECV);
  REQUIRE(directionOf(answerFor("m=audio 6000 RTP/AVP 0\na=sendonly\n",
                                "m=audio 5000 RTP/AVP 0\na=sendrecv\n"))
          == Direction::RECVONLY);
}

TEST_CASE("Direction fold-down keeps ownOffer's levels when nothing changes (RA-SC-4)",
          "[reoffer_answer][rasc4]")
{
  const std::string noDirection = "m=audio 5000 RTP/AVP 0\n";
  requireUnchanged(answerFor("m=audio 6000 RTP/AVP 0\na=sendrecv\n", noDirection), noDirection);

  auto split = answerFor("m=audio 6000 RTP/AVP 0\na=sendonly\nm=audio 6002 RTP/AVP 8\n",
                         "a=sendrecv\nm=audio 5000 RTP/AVP 0\nm=audio 5002 RTP/AVP 8\n");
  requireChangedVersion(split);
  REQUIRE_FALSE(split->answer.direction.has_value());
  REQUIRE(split->answer.mediaDescriptions[0].direction
          == std::optional<Direction>(Direction::RECVONLY));
  REQUIRE(split->answer.mediaDescriptions[1].direction
          == std::optional<Direction>(Direction::SENDRECV));

  const std::string holding = "a=sendonly\nm=audio 5000 RTP/AVP 0\n";
  requireUnchanged(answerFor("m=audio 6000 RTP/AVP 0\na=recvonly\n", holding), holding);
}

TEST_CASE("DTLS role keeps the negotiated roles (NR-6, RA-SC-3, RA-SC-14)", "[reoffer_answer][nr6]")
{
  const std::string fingerprint = FINGERPRINT;
  const std::string own = "a=setup:actpass\n" + fingerprint + "m=audio 5000 UDP/TLS/RTP/SAVPF 0\n";
  const std::string reLine = "m=audio 6000 UDP/TLS/RTP/SAVPF 0\n";

  auto passive = answerFor(reLine + "a=setup:active\n", own);
  requireChangedVersion(passive);
  REQUIRE_FALSE(passive->answer.setup.has_value());
  REQUIRE(passive->answer.mediaDescriptions[0].setup
          == std::optional<SetupRole>(SetupRole::PASSIVE));
  REQUIRE(passive->answer.fingerprints == ownFrom(own).fingerprints);

  auto active = answerFor(reLine + "a=setup:passive\n", own);
  REQUIRE(active.has_value());
  REQUIRE(active->answer.mediaDescriptions[0].setup == std::optional<SetupRole>(SetupRole::ACTIVE));

  REQUIRE_FALSE(answerFor(reLine + "a=setup:actpass\n", own).has_value());
  REQUIRE_FALSE(answerFor(reLine + "a=setup:holdconn\n", own).has_value());
  REQUIRE_FALSE(answerFor(reLine, own).has_value());

  const std::string fixedPassive
      = fingerprint + "m=audio 5000 UDP/TLS/RTP/SAVPF 0\na=setup:passive\n";
  requireUnchanged(answerFor(reLine + "a=setup:active\n", fixedPassive), fixedPassive);

  const std::string plainActpass = "a=setup:actpass\nm=audio 5000 RTP/AVP 0\n";
  requireUnchanged(answerFor("m=audio 6000 RTP/AVP 0\n", plainActpass), plainActpass);

  const std::string fixedActive
      = fingerprint + "m=audio 5000 UDP/TLS/RTP/SAVPF 0\na=setup:active\n";
  REQUIRE_FALSE(answerFor(reLine + "a=setup:active\n", fixedActive).has_value());

  const std::string twoLines
      = "a=setup:actpass\n" + fingerprint
        + "m=audio 5000 UDP/TLS/RTP/SAVPF 0\nm=audio 5002 UDP/TLS/RTP/SAVPF 8\n";
  auto sessionSetup = answerFor(
      "a=setup:active\nm=audio 6000 UDP/TLS/RTP/SAVPF 0\nm=audio 6002 UDP/TLS/RTP/SAVPF 8\n",
      twoLines);
  requireChangedVersion(sessionSetup);
  REQUIRE_FALSE(sessionSetup->answer.setup.has_value());
  REQUIRE(sessionSetup->answer.mediaDescriptions[0].setup
          == std::optional<SetupRole>(SetupRole::PASSIVE));
  REQUIRE(sessionSetup->answer.mediaDescriptions[1].setup
          == std::optional<SetupRole>(SetupRole::PASSIVE));
}

TEST_CASE("tls-id is repeated only for a peer that uses it (NR-6)", "[reoffer_answer][tlsid]")
{
  const std::string own = "m=audio 5000 RTP/AVP 0\na=tls-id:abc\n";
  auto removed = answerFor("m=audio 6000 RTP/AVP 0\n", own);
  requireChangedVersion(removed);
  REQUIRE(removed->answer.mediaDescriptions[0].genericAttributes.empty());
  requireUnchanged(answerFor("m=audio 6000 RTP/AVP 0\na=tls-id:xyz\n", own), own);
}

TEST_CASE("Setup fold-down with lines that compute no role (RA-SC-4)", "[reoffer_answer][rasc4]")
{
  const std::string fingerprint = FINGERPRINT;
  const std::string own = "a=setup:actpass\nm=audio 5000 UDP/TLS/RTP/SAVPF 0\n" + fingerprint
                          + "m=audio 5002 RTP/AVP 8\n";
  auto mixed = answerFor(
      "m=audio 6000 UDP/TLS/RTP/SAVPF 0\na=setup:active\nm=audio 6002 RTP/AVP 8\n", own);
  requireChangedVersion(mixed);
  REQUIRE_FALSE(mixed->answer.setup.has_value());
  REQUIRE(mixed->answer.mediaDescriptions[0].setup == std::optional<SetupRole>(SetupRole::PASSIVE));
  REQUIRE(mixed->answer.mediaDescriptions[1].setup == std::optional<SetupRole>(SetupRole::ACTPASS));

  const std::string plain = "a=setup:actpass\nm=audio 5000 RTP/AVP 0\nm=audio 5002 RTP/AVP 8\n";
  requireUnchanged(answerFor("m=audio 6000 RTP/AVP 0\nm=audio 6002 RTP/AVP 8\n", plain), plain);
}

// ---------------------------------------------------------------------------
// Task 1.4: RA-T-25 real-SDP round trips and RA-T-26 totality
// ---------------------------------------------------------------------------

namespace {

struct RealSample
{
  std::string name;
  std::string ownOffer;
  std::vector<std::string> peerAnswers;
};

// The peer's re-offer is its last answer with the o= version raised by one.
std::string reofferFrom(const std::string& peerAnswer)
{
  return replaceAll(peerAnswer, "o=peer 2 3 IN", "o=peer 2 4 IN");
}

const std::string CARRIER_OFFER = R"(v=0
o=- 1 9 IN IP4 192.0.2.1
s=-
c=IN IP4 192.0.2.10
t=0 0
a=sendrecv
m=audio 20000 RTP/AVP 0 8 18 13 101
a=fmtp:18 annexb=no
a=rtpmap:101 telephone-event/8000
a=fmtp:101 0-16
a=ptime:20
a=maxptime:40
)";

const std::string CARRIER_ANSWER_ALL = R"(v=0
o=peer 2 3 IN IP4 198.51.100.20
s=-
c=IN IP4 198.51.100.20
t=0 0
m=audio 30000 RTP/AVP 0 8 18 13 101
a=fmtp:18 annexb=no
a=rtpmap:101 telephone-event/8000
a=fmtp:101 0-16
a=ptime:20
a=sendrecv
)";

const std::string CARRIER_ANSWER_NARROW = R"(v=0
o=peer 2 3 IN IP4 198.51.100.20
s=-
c=IN IP4 198.51.100.20
t=0 0
m=audio 30000 RTP/AVP 0 101
a=rtpmap:101 telephone-event/8000
a=fmtp:101 0-16
a=ptime:20
a=sendrecv
)";

const std::string CARRIER_ANSWER_G729 = R"(v=0
o=peer 2 3 IN IP4 198.51.100.20
s=-
c=IN IP4 198.51.100.20
t=0 0
m=audio 30000 RTP/AVP 18 101
a=fmtp:18 annexb=no
a=rtpmap:101 telephone-event/8000
a=fmtp:101 0-16
a=ptime:20
a=sendrecv
)";

const std::string RTPENGINE_OFFER = R"(v=0
o=- 1 9 IN IP4 203.0.113.5
s=rtpengine
c=IN IP4 203.0.113.5
t=0 0
m=audio 30000 RTP/SAVP 0 8 101
a=rtpmap:0 PCMU/8000
a=rtpmap:8 PCMA/8000
a=rtpmap:101 telephone-event/8000
a=fmtp:101 0-16
a=sendrecv
a=rtcp:30001
a=ssrc:12345 cname:rtpengine0
a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkw
a=crypto:2 AES_CM_128_HMAC_SHA1_32 inline:QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVphYmNk
a=ice-ufrag:rtpe
a=ice-pwd:rtpenginepassword012345
a=candidate:1 1 UDP 2130706431 203.0.113.5 30000 typ host
a=candidate:1 2 UDP 2130706430 203.0.113.5 30001 typ host
)";

const std::string RTPENGINE_ANSWER = R"(v=0
o=peer 2 3 IN IP4 198.51.100.21
s=-
c=IN IP4 198.51.100.21
t=0 0
m=audio 31000 RTP/SAVP 0 101
a=rtpmap:0 PCMU/8000
a=rtpmap:101 telephone-event/8000
a=fmtp:101 0-16
a=sendrecv
a=crypto:2 AES_CM_128_HMAC_SHA1_32 inline:ZmVkY2JhOTg3NjU0MzIxMGZlZGNiYTk4NzY1NDMy
)";

const std::string TEAMS_OFFER = R"(v=0
o=- 1 9 IN IP4 203.0.113.7
s=session
c=IN IP4 203.0.113.7
b=CT:10000000
t=0 0
m=audio 52000 RTP/SAVP 0 8 97 13 101
a=rtcp:52001
a=ice-ufrag:tUfr
a=ice-pwd:teamsPassword0123456789ab
a=candidate:1 1 UDP 2130706431 203.0.113.7 52000 typ host
a=candidate:2 1 tcp-act 2121006078 203.0.113.7 52010 typ srflx raddr 10.0.0.1 rport 52010
a=candidate:3 1 TCP-PASS 2121006079 203.0.113.7 52011 typ srflx raddr 10.0.0.1 rport 52011
a=x-ssrc-range:1000-1000
a=label:main-audio
a=x-source:main-audio
a=rtcp-mux
a=rtcp-rsize
a=rtcp-fb:* x-message app send:dsh recv:dsh
a=crypto:2 AES_CM_128_HMAC_SHA1_80 inline:MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkw|2^31|1:1
a=crypto:3 AES_CM_128_HMAC_SHA1_80 inline:QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVphYmNk|2^31
a=rtpmap:0 PCMU/8000
a=rtpmap:8 PCMA/8000
a=rtpmap:97 RED/8000
a=rtpmap:13 CN/8000
a=rtpmap:101 telephone-event/8000
a=fmtp:101 0-16
a=ptime:20
)";

const std::string TEAMS_ANSWER_NON_ICE = R"(v=0
o=peer 2 3 IN IP4 198.51.100.22
s=-
c=IN IP4 198.51.100.22
t=0 0
m=audio 32000 RTP/SAVP 0 101
a=rtpmap:0 PCMU/8000
a=rtpmap:101 telephone-event/8000
a=fmtp:101 0-16
a=rtcp-mux
a=crypto:2 AES_CM_128_HMAC_SHA1_80 inline:ZmVkY2JhOTg3NjU0MzIxMGZlZGNiYTk4NzY1NDMy
)";

const std::string TEAMS_ANSWER_ICE_LITE = R"(v=0
o=peer 2 3 IN IP4 198.51.100.22
s=-
c=IN IP4 198.51.100.22
t=0 0
a=ice-lite
m=audio 32000 RTP/SAVP 0 101
a=rtpmap:0 PCMU/8000
a=rtpmap:101 telephone-event/8000
a=fmtp:101 0-16
a=rtcp-mux
a=rtcp-rsize
a=rtcp-fb:* x-message app send:dsh recv:dsh
a=ice-ufrag:sbcU
a=ice-pwd:sbcpassword0123456789abcd
a=candidate:1 1 UDP 2130706431 198.51.100.22 32000 typ host
a=crypto:3 AES_CM_128_HMAC_SHA1_80 inline:ZmVkY2JhOTg3NjU0MzIxMGZlZGNiYTk4NzY1NDMy
)";

const std::string T38_REINVITE_OFFER = R"(v=0
o=- 1 9 IN IP4 192.0.2.1
s=-
c=IN IP4 192.0.2.1
t=0 0
m=audio 0 RTP/AVP 0
m=image 20002 udptl t38
a=T38FaxVersion:0
a=T38MaxBitRate:14400
a=T38FaxRateManagement:transferredTCF
a=T38FaxMaxBuffer:262
a=T38FaxMaxDatagram:176
a=T38FaxUdpEC:t38UDPRedundancy
)";

const std::string T38_REINVITE_ANSWER = R"(v=0
o=peer 2 3 IN IP4 198.51.100.23
s=-
c=IN IP4 198.51.100.23
t=0 0
m=audio 0 RTP/AVP 0
m=image 33002 udptl t38
a=T38FaxVersion:0
a=T38MaxBitRate:14400
a=T38FaxRateManagement:transferredTCF
a=T38FaxMaxBuffer:262
a=T38FaxMaxDatagram:176
a=T38FaxUdpEC:t38UDPRedundancy
)";

const std::string VIDEO_REJECTED_OFFER = R"(v=0
o=- 1 9 IN IP4 192.0.2.1
s=-
c=IN IP4 192.0.2.1
t=0 0
m=audio 20000 RTP/AVP 0 101
a=rtpmap:101 telephone-event/8000
a=fmtp:101 0-16
m=video 20002 RTP/AVP 96
a=rtpmap:96 H264/90000
a=fmtp:96 profile-level-id=42e01f;packetization-mode=1
)";

const std::string VIDEO_REJECTED_ANSWER = R"(v=0
o=peer 2 3 IN IP4 198.51.100.24
s=-
c=IN IP4 198.51.100.24
t=0 0
m=audio 34000 RTP/AVP 0 101
a=rtpmap:101 telephone-event/8000
a=fmtp:101 0-16
m=video 0 RTP/AVP 96
)";

const std::string OPUS_OFFER = R"(v=0
o=- 1 9 IN IP4 192.0.2.1
s=-
c=IN IP4 192.0.2.1
t=0 0
m=audio 20000 RTP/AVP 111 100 101
a=rtpmap:111 opus/48000/2
a=fmtp:111 minptime=10;useinbandfec=1
a=rtpmap:100 telephone-event/8000
a=fmtp:100 0-15
a=rtpmap:101 telephone-event/48000
a=fmtp:101 0-15
)";

const std::string OPUS_ANSWER = R"(v=0
o=peer 2 3 IN IP4 198.51.100.25
s=-
c=IN IP4 198.51.100.25
t=0 0
m=audio 35000 RTP/AVP 109 101
a=rtpmap:109 opus/48000/2
a=rtpmap:101 telephone-event/8000
a=fmtp:101 0-15
)";

const std::string T38_SINGLE_SLOT_OFFER = R"(v=0
o=- 1 9 IN IP4 192.0.2.1
s=-
c=IN IP4 192.0.2.1
t=0 0
m=image 20004 udptl t38
a=T38FaxVersion:0
a=T38MaxBitRate:14400
a=T38FaxUdpEC:t38UDPRedundancy
)";

const std::string T38_SINGLE_SLOT_ANSWER = R"(v=0
o=peer 2 3 IN IP4 198.51.100.26
s=-
c=IN IP4 198.51.100.26
t=0 0
m=image 36004 udptl t38
a=T38FaxVersion:0
a=T38MaxBitRate:14400
a=T38FaxUdpEC:t38UDPRedundancy
)";

const std::string CAPNEG_OFFER = R"(v=0
o=- 1 9 IN IP4 192.0.2.1
s=-
c=IN IP4 192.0.2.1
t=0 0
m=audio 20000 RTP/AVP 0
a=tcap:1 RTP/SAVP
a=acap:1 crypto:1 AES_CM_128_HMAC_SHA1_80 inline:MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkw
a=pcfg:1 t=1 a=1
)";

const std::string CAPNEG_ANSWER = R"(v=0
o=peer 2 3 IN IP4 198.51.100.27
s=-
c=IN IP4 198.51.100.27
t=0 0
m=audio 37000 RTP/SAVP 0
a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:ZmVkY2JhOTg3NjU0MzIxMGZlZGNiYTk4NzY1NDMy
a=acfg:1 t=1 a=1
)";

const std::string WEBRTC_OFFER = R"(v=0
o=- 1 9 IN IP4 192.0.2.1
s=-
c=IN IP4 192.0.2.1
t=0 0
a=setup:actpass
a=fingerprint:sha-256 AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89
a=ice-ufrag:wUfr
a=ice-pwd:webrtcpassword0123456789
m=audio 9000 UDP/TLS/RTP/SAVPF 111
a=mid:0
a=rtpmap:111 opus/48000/2
a=rtcp-mux
a=rtcp-rsize
a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level
a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time
a=extmap:3 urn:ietf:params:rtp-hdrext:sdes:mid
a=msid:stream0 audio0
a=ssrc:1111 cname:webrtc0
a=candidate:1 1 udp 2130706431 192.0.2.1 9000 typ host
m=video 9002 UDP/TLS/RTP/SAVPF 96
a=mid:1
a=rtpmap:96 VP8/90000
a=rtcp-mux
a=rtcp-rsize
a=rtcp-fb:96 nack
a=rtcp-fb:96 nack pli
a=rtcp-fb:96 ccm fir
a=rtcp-fb:96 goog-remb
a=rtcp-fb:96 transport-cc
a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time
a=extmap:3 urn:ietf:params:rtp-hdrext:sdes:mid
a=extmap:4 urn:3gpp:video-orientation
a=msid:stream0 video0
a=ssrc:2222 cname:webrtc0
a=candidate:1 1 udp 2130706431 192.0.2.1 9002 typ host
)";

const std::string WEBRTC_ANSWER = R"(v=0
o=peer 2 3 IN IP4 198.51.100.28
s=-
c=IN IP4 198.51.100.28
t=0 0
a=fingerprint:sha-256 01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF
a=ice-ufrag:pUfr
a=ice-pwd:peerwebrtcpassword012345
m=audio 38000 UDP/TLS/RTP/SAVPF 111
a=mid:0
a=setup:active
a=rtpmap:111 opus/48000/2
a=rtcp-mux
a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level
a=candidate:1 1 udp 2130706431 198.51.100.28 38000 typ host
m=video 38002 UDP/TLS/RTP/SAVPF 96
a=mid:1
a=setup:active
a=rtpmap:96 VP8/90000
a=rtcp-mux
a=rtcp-fb:96 nack pli
a=rtcp-fb:96 ccm fir
a=extmap:3 urn:ietf:params:rtp-hdrext:sdes:mid
a=candidate:1 1 udp 2130706431 198.51.100.28 38002 typ host
)";

const std::string WEBRTC_BUNDLE_OFFER
    = replaceAll(WEBRTC_OFFER, "t=0 0\n", "t=0 0\na=group:BUNDLE 0 1\n");
const std::string WEBRTC_BUNDLE_ANSWER
    = replaceAll(WEBRTC_ANSWER, "t=0 0\n", "t=0 0\na=group:BUNDLE 0 1\n");

std::vector<RealSample> realSamples()
{
  return {
      {"(1) carrier trunk",
       CARRIER_OFFER,
       {CARRIER_ANSWER_ALL, CARRIER_ANSWER_NARROW, CARRIER_ANSWER_G729}},
      {"(2) rtpengine SDES", RTPENGINE_OFFER, {RTPENGINE_ANSWER}},
      {"(3) Teams-style SDES", TEAMS_OFFER, {TEAMS_ANSWER_NON_ICE, TEAMS_ANSWER_ICE_LITE}},
      {"(4) T.38 re-INVITE", T38_REINVITE_OFFER, {T38_REINVITE_ANSWER}},
      {"(5) video rejected", VIDEO_REJECTED_OFFER, {VIDEO_REJECTED_ANSWER}},
      {"(6) opus with two telephone-event rates", OPUS_OFFER, {OPUS_ANSWER}},
      {"(7) single-slot T.38", T38_SINGLE_SLOT_OFFER, {T38_SINGLE_SLOT_ANSWER}},
      {"(8) RFC 5939 capneg", CAPNEG_OFFER, {CAPNEG_ANSWER}},
      {"(9) WebRTC non-BUNDLE", WEBRTC_OFFER, {WEBRTC_ANSWER}},
      {"(9b) WebRTC BUNDLE", WEBRTC_BUNDLE_OFFER, {WEBRTC_BUNDLE_ANSWER}},
  };
}

// Answers the re-offer built from `peerAnswer`, after checking the AD-9
// precondition (the re-offer equals the peer's answer apart from its o= line).
std::optional<ReofferAnswer> answerSample(const std::string& ownOffer,
                                          const std::string& peerAnswer)
{
  const std::string reoffer = reofferFrom(peerAnswer);
  REQUIRE(reoffer != peerAnswer);
  REQUIRE(OfferAnswer::equalIgnoringOriginLine(peerAnswer, reoffer));
  return OfferAnswer::answerReofferFromOwnOffer(mustParse(reoffer), mustParse(ownOffer));
}

// Known gap: the serializer writes a=fmtp only for formats that have an
// a=rtpmap (tasks/iora_sdp/backlog/2026-11-25-3), so the round trip loses the
// a=fmtp of a bare static payload type. The expected model drops those lines;
// the fix of 2026-11-25-3 removes this adjustment.
SessionDescription withoutFmtpOfFormatsLackingRtpMap(SessionDescription session)
{
  for (auto& media : session.mediaDescriptions)
  {
    media.fmtps.erase(std::remove_if(media.fmtps.begin(), media.fmtps.end(),
                                     [&](const FmtpAttribute& fmtp)
                                     { return !hasRtpMap(media, fmtp.payloadType); }),
                      media.fmtps.end());
  }
  return session;
}

bool contains(const std::string& text, const std::string& part)
{
  return text.find(part) != std::string::npos;
}

bool hasGenericAttribute(const std::vector<GenericAttribute>& attributes, const std::string& name)
{
  return std::any_of(attributes.begin(), attributes.end(),
                     [&](const GenericAttribute& attribute) { return attribute.name == name; });
}

// Clears the line fields that the re-offer answer rules own, so what is left
// must be exactly ownOffer's line.
MediaDescription withoutRuleOwnedFields(MediaDescription line)
{
  line.formats.clear();
  line.rtpMaps.clear();
  line.fmtps.clear();
  line.rtcpFeedbacks.clear();
  line.crypto.clear();
  line.extMaps.clear();
  line.extmapAllowMixed = false;
  line.rtcpMux = false;
  line.rtcpMuxOnly = false;
  line.rtcpRsize = false;
  line.simulcast.reset();
  line.rids.clear();
  line.candidates.clear();
  line.iceUfrag.reset();
  line.icePwd.reset();
  line.iceOptions.clear();
  line.endOfCandidates = false;
  line.setup.reset();
  line.direction.reset();
  line.genericAttributes.clear();
  return line;
}

// Clears the session fields that the re-offer answer rules own.
SessionDescription withoutRuleOwnedSessionFields(SessionDescription session)
{
  if (session.origin.has_value())
  {
    session.origin->sessVersion.clear();
  }
  session.direction.reset();
  session.setup.reset();
  session.groups.clear();
  session.iceUfrag.reset();
  session.icePwd.reset();
  session.iceOptions.clear();
  session.iceLite = false;
  session.extmapAllowMixed = false;
  session.mediaDescriptions.clear();
  return session;
}

// Common checks of RA-T-25 for an answer that has a value:
// (1) the serialized answer parses back to the answer model;
// (2) every typed field the rules keep equals ownOffer's;
// (3) every generic a= line of ownOffer that the rules keep is in the text.
std::string commonChecks(const ReofferAnswer& result, const std::string& ownOfferText,
                         const std::string& peerAnswer, bool skipCandidateLines)
{
  const SessionDescription ownOffer = mustParse(ownOfferText);
  const SessionDescription reoffer = mustParse(reofferFrom(peerAnswer));
  const SessionDescription& answer = result.answer;

  const std::string text = SdpSerializer::serialize(answer);
  auto reparsed = SdpParser::parse(text);
  REQUIRE(reparsed.hasValue());
  REQUIRE(*reparsed.value == withoutFmtpOfFormatsLackingRtpMap(answer));

  REQUIRE(withoutRuleOwnedSessionFields(answer) == withoutRuleOwnedSessionFields(ownOffer));
  REQUIRE(answer.mediaDescriptions.size() == ownOffer.mediaDescriptions.size());
  for (const auto& attribute : ownOffer.attributes)
  {
    REQUIRE(contains(text, SdpSerializer::serializeAttribute(attribute)));
  }
  for (std::size_t i = 0; i < ownOffer.mediaDescriptions.size(); ++i)
  {
    if (reoffer.mediaDescriptions[i].port == 0)
    {
      continue;
    }
    INFO("media line " << i);
    const auto& own = ownOffer.mediaDescriptions[i];
    REQUIRE(withoutRuleOwnedFields(answer.mediaDescriptions[i]) == withoutRuleOwnedFields(own));
    const bool reofferUsesTlsId
        = hasGenericAttribute(reoffer.mediaDescriptions[i].genericAttributes, "tls-id");
    for (const auto& attribute : own.genericAttributes)
    {
      if ((skipCandidateLines && attribute.name == "candidate")
          || (attribute.name == "tls-id" && !reofferUsesTlsId))
      {
        continue;
      }
      REQUIRE(contains(text, SdpSerializer::serializeAttribute(attribute)));
    }
  }
  return text;
}

std::string readBinary(const std::filesystem::path& path)
{
  std::ifstream in(path, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

} // namespace

TEST_CASE("Real SDP samples round trip through the re-offer answer (RA-T-25)",
          "[reoffer_answer][real_sdp]")
{
  SECTION("(1) carrier trunk, answer that accepts everything")
  {
    auto result = answerSample(CARRIER_OFFER, CARRIER_ANSWER_ALL);
    REQUIRE(result.has_value());
    REQUIRE_FALSE(result->differsFromOwnOffer);
    REQUIRE(result->answer == mustParse(CARRIER_OFFER));
    const auto text = commonChecks(*result, CARRIER_OFFER, CARRIER_ANSWER_ALL, false);
    // Known-gap pin (tasks/iora_sdp/backlog/2026-11-25-3): the answer model keeps
    // a=fmtp:18 annexb=no, but the serializer drops it. The fix flips this check.
    REQUIRE(hasFmtp(result->answer.mediaDescriptions[0], 18));
    REQUIRE_FALSE(contains(text, "a=fmtp:18"));
  }
  SECTION("(1) carrier trunk, answer that keeps 0 101")
  {
    auto result = answerSample(CARRIER_OFFER, CARRIER_ANSWER_NARROW);
    requireChangedVersion(result);
    const auto text = commonChecks(*result, CARRIER_OFFER, CARRIER_ANSWER_NARROW, false);
    const auto& line = result->answer.mediaDescriptions[0];
    REQUIRE(line.formats == Formats{"0", "101"});
    for (int pt : {8, 18, 13})
    {
      REQUIRE_FALSE(hasRtpMap(line, pt));
      REQUIRE_FALSE(hasFmtp(line, pt));
    }
    REQUIRE_FALSE(contains(text, "annexb"));
    REQUIRE(contains(text, "a=fmtp:101 0-16"));
    REQUIRE(contains(text, "a=ptime:20"));
    REQUIRE(contains(text, "a=maxptime:40"));
    REQUIRE(result->answer.connection == mustParse(CARRIER_OFFER).connection);
  }
  SECTION("(1) carrier trunk, answer that keeps 18 101")
  {
    auto result = answerSample(CARRIER_OFFER, CARRIER_ANSWER_G729);
    requireChangedVersion(result);
    const auto text = commonChecks(*result, CARRIER_OFFER, CARRIER_ANSWER_G729, false);
    const auto& line = result->answer.mediaDescriptions[0];
    REQUIRE(line.formats == Formats{"18", "101"});
    REQUIRE(hasFmtp(line, 18));
    // Known-gap pin (tasks/iora_sdp/backlog/2026-11-25-3): the answer differs, so
    // the caller serializes it, and the serializer drops a=fmtp:18 annexb=no.
    // The fix flips this check.
    REQUIRE_FALSE(contains(text, "a=fmtp:18"));
  }
  SECTION("(2) rtpengine SDES")
  {
    auto result = answerSample(RTPENGINE_OFFER, RTPENGINE_ANSWER);
    requireChangedVersion(result);
    const auto text = commonChecks(*result, RTPENGINE_OFFER, RTPENGINE_ANSWER, false);
    const auto& line = result->answer.mediaDescriptions[0];
    REQUIRE(line.crypto.size() == 1);
    REQUIRE(line.crypto[0] == mustParse(RTPENGINE_OFFER).mediaDescriptions[0].crypto[1]);
    REQUIRE_FALSE(contains(text, "a=candidate"));
    REQUIRE_FALSE(contains(text, "a=ice-"));
    REQUIRE(contains(text, "a=rtcp:30001"));
    REQUIRE(contains(text, "a=ssrc:12345"));
  }
  SECTION("(3a) Teams-style SDES, non-ICE SBC answer")
  {
    auto result = answerSample(TEAMS_OFFER, TEAMS_ANSWER_NON_ICE);
    requireChangedVersion(result);
    const auto text = commonChecks(*result, TEAMS_OFFER, TEAMS_ANSWER_NON_ICE, false);
    REQUIRE(result->answer.mediaDescriptions[0].crypto.size() == 1);
    REQUIRE(result->answer.mediaDescriptions[0].crypto[0].tag == 2);
    REQUIRE_FALSE(contains(text, "a=candidate"));
    REQUIRE_FALSE(contains(text, "a=ice-ufrag"));
    REQUIRE_FALSE(contains(text, "a=ice-pwd"));
    REQUIRE_FALSE(contains(text, "x-message"));
    REQUIRE_FALSE(contains(text, "a=rtcp-rsize"));
    REQUIRE(contains(text, "a=x-ssrc-range:1000-1000"));
    REQUIRE(contains(text, "a=x-source:main-audio"));
    REQUIRE(contains(text, "a=label:main-audio"));
  }
  SECTION("(3b) Teams-style SDES, ICE-lite SBC answer")
  {
    auto result = answerSample(TEAMS_OFFER, TEAMS_ANSWER_ICE_LITE);
    requireChangedVersion(result);
    const auto text = commonChecks(*result, TEAMS_OFFER, TEAMS_ANSWER_ICE_LITE, true);
    const auto own = mustParse(TEAMS_OFFER).mediaDescriptions[0];
    const auto& line = result->answer.mediaDescriptions[0];
    REQUIRE(line.crypto.size() == 1);
    REQUIRE(line.crypto[0].tag == 3);
    REQUIRE(line.iceUfrag == own.iceUfrag);
    REQUIRE(line.icePwd == own.icePwd);
    REQUIRE(line.candidates == own.candidates);
    REQUIRE(contains(text, "a=rtcp-fb:* x-message app send:dsh recv:dsh"));
    REQUIRE(contains(text, "a=rtcp-rsize"));
    // Known gap RA-RK-5: the parser stores tcp-act / TCP-PASS as udp.
    // tasks/iora_sdp/backlog/2026-11-25-2 flips these two checks.
    REQUIRE_FALSE(contains(text, "tcp-act"));
    REQUIRE_FALSE(contains(text, "TCP-PASS"));
  }
  SECTION("(4) T.38 re-INVITE and (7) single-slot T.38 have no answer (RA-SC-18)")
  {
    REQUIRE_FALSE(answerSample(T38_REINVITE_OFFER, T38_REINVITE_ANSWER).has_value());
    REQUIRE_FALSE(answerSample(T38_SINGLE_SLOT_OFFER, T38_SINGLE_SLOT_ANSWER).has_value());
  }
  SECTION("(5) video rejected by the peer")
  {
    auto result = answerSample(VIDEO_REJECTED_OFFER, VIDEO_REJECTED_ANSWER);
    requireChangedVersion(result);
    const auto text = commonChecks(*result, VIDEO_REJECTED_OFFER, VIDEO_REJECTED_ANSWER, false);
    const auto& video = result->answer.mediaDescriptions[1];
    MediaDescription expected;
    expected.mediaType = MediaType::VIDEO;
    expected.protocol = TransportProtocol::RTP_AVP;
    expected.port = 0;
    expected.formats = {"96"};
    expected.direction = Direction::INACTIVE;
    REQUIRE(video == expected);
    REQUIRE(result->answer.mediaDescriptions[0]
            == mustParse(VIDEO_REJECTED_OFFER).mediaDescriptions[0]);
    REQUIRE_FALSE(contains(text, "profile-level-id"));
  }
  SECTION("(6) opus with telephone-event at two clock rates")
  {
    auto result = answerSample(OPUS_OFFER, OPUS_ANSWER);
    requireChangedVersion(result);
    commonChecks(*result, OPUS_OFFER, OPUS_ANSWER, false);
    const auto& line = result->answer.mediaDescriptions[0];
    REQUIRE(line.formats == Formats{"111", "100"});
    REQUIRE_FALSE(hasRtpMap(line, 101));
    REQUIRE_FALSE(hasFmtp(line, 101));
  }
  SECTION("(8) RFC 5939 capability negotiation has no answer")
  {
    REQUIRE_FALSE(answerSample(CAPNEG_OFFER, CAPNEG_ANSWER).has_value());
  }
  SECTION("(9) WebRTC-shaped non-BUNDLE DTLS-SRTP")
  {
    auto result = answerSample(WEBRTC_OFFER, WEBRTC_ANSWER);
    requireChangedVersion(result);
    const auto text = commonChecks(*result, WEBRTC_OFFER, WEBRTC_ANSWER, false);
    const auto own = mustParse(WEBRTC_OFFER);
    const auto& answer = result->answer;
    REQUIRE_FALSE(answer.setup.has_value());
    REQUIRE(answer.fingerprints == own.fingerprints);
    REQUIRE(answer.iceUfrag == own.iceUfrag);
    REQUIRE(answer.icePwd == own.icePwd);
    for (std::size_t i = 0; i < 2; ++i)
    {
      const auto& line = answer.mediaDescriptions[i];
      REQUIRE(line.setup == std::optional<SetupRole>(SetupRole::PASSIVE));
      REQUIRE(line.rtcpMux);
      REQUIRE_FALSE(line.rtcpRsize);
      REQUIRE(line.extMaps.size() == 1);
      REQUIRE(line.msid == own.mediaDescriptions[i].msid);
      REQUIRE(line.ssrcs == own.mediaDescriptions[i].ssrcs);
      REQUIRE(line.candidates == own.mediaDescriptions[i].candidates);
    }
    REQUIRE(answer.mediaDescriptions[0].extMaps[0].id == 1);
    REQUIRE(answer.mediaDescriptions[1].extMaps[0].id == 3);
    const auto& feedbacks = answer.mediaDescriptions[1].rtcpFeedbacks;
    REQUIRE(feedbacks.size() == 2);
    REQUIRE(feedbacks[0].payloadType == "96");
    REQUIRE(feedbacks[0].type == "nack");
    REQUIRE(feedbacks[0].subtype == std::optional<std::string>("pli"));
    REQUIRE(feedbacks[1].type == "ccm");
    REQUIRE_FALSE(contains(text, "goog-remb"));
    REQUIRE_FALSE(contains(text, "a=rtcp-rsize"));
  }
  SECTION("(9b) WebRTC with BUNDLE has no answer (NR-9)")
  {
    REQUIRE_FALSE(answerSample(WEBRTC_BUNDLE_OFFER, WEBRTC_BUNDLE_ANSWER).has_value());
  }
}

TEST_CASE("answerReofferFromOwnOffer is total over the corpus and the samples (RA-T-26)",
          "[reoffer_answer][totality]")
{
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(IORA_SDP_FUZZ_CORPUS_DIR))
  {
    if (entry.is_regular_file() && entry.path().extension() == ".sdp")
    {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());

  std::vector<SessionDescription> parsed;
  std::vector<std::string> parsedNames;
  for (const auto& file : files)
  {
    auto result = SdpParser::parse(readBinary(file));
    if (result.hasValue())
    {
      parsed.push_back(*result.value);
      parsedNames.push_back(file.filename().string());
    }
  }
  REQUIRE(std::find(parsedNames.begin(), parsedNames.end(), "minimal.sdp") != parsedNames.end());
  REQUIRE(std::find(parsedNames.begin(), parsedNames.end(), "sip.sdp") != parsedNames.end());

  for (const auto& sample : realSamples())
  {
    INFO(sample.name);
    parsed.push_back(mustParse(sample.ownOffer));
    for (const auto& peerAnswer : sample.peerAnswers)
    {
      parsed.push_back(mustParse(peerAnswer));
      parsed.push_back(mustParse(reofferFrom(peerAnswer)));
    }
  }

  std::size_t pairs = 0;
  for (const auto& reoffer : parsed)
  {
    for (const auto& ownOffer : parsed)
    {
      std::optional<ReofferAnswer> result;
      REQUIRE_NOTHROW(result = OfferAnswer::answerReofferFromOwnOffer(reoffer, ownOffer));
      if (result.has_value() && !result->differsFromOwnOffer)
      {
        REQUIRE(result->answer == ownOffer);
      }
      ++pairs;
    }
  }
  REQUIRE(pairs == parsed.size() * parsed.size());
  INFO("pairs run: " << pairs);
  REQUIRE(pairs > 100);
}

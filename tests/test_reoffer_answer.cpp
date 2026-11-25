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
  const std::string two = "m=audio 5000 RTP/AVP 0\nm=video 5002 RTP/AVP 96\na=rtpmap:96 VP8/90000\n";
  REQUIRE_FALSE(answerFor("m=audio 5000 RTP/AVP 0\n", two).has_value());
  REQUIRE_FALSE(answerFor("m=audio 5000 RTP/AVP 0\nm=audio 5002 RTP/AVP 0\n", two).has_value());

  const std::string unknown = "m=foo 5000 RTP/AVP 0\n";
  requireUnchanged(answerFor(unknown, unknown), unknown);
  REQUIRE_FALSE(answerFor("m=bar 5000 RTP/AVP 0\n", unknown).has_value());
}

TEST_CASE("An own line on port 0 with a live re-offer line has no answer (RA-SC-2)", "[reoffer_answer][rasc2]")
{
  REQUIRE_FALSE(answerFor("m=audio 5000 RTP/AVP 0\n", "m=audio 0 RTP/AVP 0\n").has_value());
}

// ---------------------------------------------------------------------------
// RA-T-3 (NR-2) and RA-T-4 (hoist regression)
// ---------------------------------------------------------------------------

TEST_CASE("A re-offer line on port 0 gets the reject shape (NR-2)", "[reoffer_answer][nr2]")
{
  const std::string audio = "m=audio 5000 RTP/AVP 0\na=mid:a\n";
  const std::string ownVideo =
      "m=video 5002 RTP/SAVP 96\n"
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

TEST_CASE("createAnswer still rejects with the same shape after the hoist", "[reoffer_answer][hoist]")
{
  const std::string offerText = sdpWith(PEER_ORIGIN,
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
  const std::string own =
      "m=audio 5000 RTP/AVP 111 101\n"
      "a=rtpmap:111 opus/48000/2\na=fmtp:111 minptime=10;useinbandfec=1\n"
      "a=rtpmap:101 telephone-event/48000\na=fmtp:101 0-15\n";
  const std::string re =
      "m=audio 6000 RTP/AVP 96 100\n"
      "a=rtpmap:96 opus/48000/2\na=rtpmap:100 telephone-event/48000\n";
  requireUnchanged(answerFor(re, own), own);
}

TEST_CASE("Dropped formats lose their codec lines (NR-3)", "[reoffer_answer][nr3]")
{
  const std::string own =
      "m=audio 5000 RTP/AVP 0 8 18 101\n"
      "a=rtpmap:18 G729/8000\na=fmtp:18 annexb=no\n"
      "a=rtpmap:101 telephone-event/8000\na=fmtp:101 0-15\n"
      "a=rtcp-fb:* nack\n";
  const std::string re =
      "m=audio 6000 RTP/AVP 8 101\na=rtpmap:101 telephone-event/8000\na=rtcp-fb:* nack\n";
  auto result = answerFor(re, own);
  requireChangedVersion(result);
  const auto& line = result->answer.mediaDescriptions[0];
  REQUIRE(line.formats == Formats{"8", "101"});
  REQUIRE_FALSE(hasRtpMap(line, 18));
  REQUIRE_FALSE(hasFmtp(line, 18));
  REQUIRE(line.rtcpFeedbacks.size() == 1);
  REQUIRE(line.rtcpFeedbacks[0].payloadType == "*");
}

TEST_CASE("Several candidates are resolved by number, else no answer (NR-3)", "[reoffer_answer][nr3]")
{
  const std::string own =
      "m=audio 5000 RTP/AVP 96 97\n"
      "a=rtpmap:96 AMR-WB/16000/1\na=fmtp:96 octet-align=0\n"
      "a=rtpmap:97 AMR-WB/16000/1\na=fmtp:97 octet-align=1\n";
  auto kept = answerFor("m=audio 6000 RTP/AVP 97\na=rtpmap:97 AMR-WB/16000/1\na=fmtp:97 octet-align=1\n", own);
  requireChangedVersion(kept);
  REQUIRE(kept->answer.mediaDescriptions[0].formats == Formats{"97"});
  REQUIRE(hasFmtp(kept->answer.mediaDescriptions[0], 97));
  REQUIRE_FALSE(hasFmtp(kept->answer.mediaDescriptions[0], 96));

  REQUIRE_FALSE(answerFor("m=audio 6000 RTP/AVP 100\na=rtpmap:100 AMR-WB/16000/1\n", own).has_value());

  const std::string h264 =
      "m=video 5002 RTP/AVP 97 98\n"
      "a=rtpmap:97 H264/90000\na=fmtp:97 packetization-mode=0\n"
      "a=rtpmap:98 H264/90000\na=fmtp:98 packetization-mode=1\n";
  auto video = answerFor("m=video 6002 RTP/AVP 98\na=rtpmap:98 H264/90000\na=fmtp:98 packetization-mode=1\n", h264);
  requireChangedVersion(video);
  REQUIRE(video->answer.mediaDescriptions[0].formats == Formats{"98"});
}

TEST_CASE("One own format answers one re-offer format (NR-3)", "[reoffer_answer][nr3]")
{
  REQUIRE_FALSE(answerFor("m=audio 6000 RTP/AVP 96 97\na=rtpmap:96 opus/48000/2\na=rtpmap:97 opus/48000/2\n",
                          "m=audio 5000 RTP/AVP 111\na=rtpmap:111 opus/48000/2\n").has_value());
  REQUIRE_FALSE(answerFor("m=audio 6000 RTP/AVP 96\na=rtpmap:96 iLBC/8000\n",
                          "m=audio 5000 RTP/AVP 0\n").has_value());
}

TEST_CASE("Format identity without codec entries and by static number (RA-SC-1, RA-SC-10, RA-SC-20)", "[reoffer_answer][nr3][rasc1][rasc20]")
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
                            "m=application 5006 UDP/DTLS/SCTP webrtc-datachannel\n").has_value());
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
    auto result = answerFor("m=audio 6000 RTP/AVP 0 40\na=rtpmap:40 speex/8000\n",
                            "m=audio 5000 RTP/AVP 0 40 41\na=rtpmap:40 iLBC/8000\na=rtpmap:41 speex/8000\n");
    requireChangedVersion(result);
    REQUIRE(result->answer.mediaDescriptions[0].formats == Formats{"0", "41"});
    REQUIRE_FALSE(hasRtpMap(result->answer.mediaDescriptions[0], 40));
  }
}

// ---------------------------------------------------------------------------
// RA-T-22 (NR-8, RA-SC-8, RA-SC-6 in S8)
// ---------------------------------------------------------------------------

TEST_CASE("The o= version rises by one only when the answer differs (NR-8)", "[reoffer_answer][nr8]")
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
  REQUIRE_FALSE(OfferAnswer::answerReofferFromOwnOffer(mustParse(sdpWith(PEER_ORIGIN, own)), noOrigin).has_value());

  SessionDescription badVersion = ownFrom(own);
  badVersion.origin->sessVersion = "1a";
  REQUIRE_FALSE(OfferAnswer::answerReofferFromOwnOffer(mustParse(sdpWith(PEER_ORIGIN, narrowed)), badVersion).has_value());
  auto same = OfferAnswer::answerReofferFromOwnOffer(mustParse(sdpWith(PEER_ORIGIN, own)), badVersion);
  REQUIRE(same.has_value());
  REQUIRE_FALSE(same->differsFromOwnOffer);
  REQUIRE(same->answer == badVersion);
}

// ---------------------------------------------------------------------------
// RA-T-27 (RA-SC-13), RA-T-29 (RA-SC-15)
// ---------------------------------------------------------------------------

TEST_CASE("Protocol and mid must match on a kept line (RA-SC-13)", "[reoffer_answer][rasc13]")
{
  REQUIRE_FALSE(answerFor("m=audio 6000 RTP/SAVP 0\na=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:WVNfX19zZW1jdGwgKCkgewkyMjA7fQp9CnVubGVz\n",
                          "m=audio 5000 RTP/AVP 0\n").has_value());
  REQUIRE_FALSE(answerFor("m=audio 6000 FOO/BAR 0\n", "m=audio 5000 FOO/BAZ 0\n").has_value());
  REQUIRE_FALSE(answerFor("m=audio 6000 RTP/AVP 0\na=mid:b\n", "m=audio 5000 RTP/AVP 0\na=mid:a\n").has_value());

  const std::string withMid = "m=audio 5000 RTP/AVP 0\na=mid:a\n";
  auto noReMid = answerFor("m=audio 6000 RTP/AVP 0\n", withMid);
  requireUnchanged(noReMid, withMid);
  REQUIRE(noReMid->answer.mediaDescriptions[0].mid == std::optional<std::string>("a"));

  REQUIRE_FALSE(answerFor("m=audio 6000 RTP/AVP 0\na=mid:a\n", "m=audio 5000 RTP/AVP 0\n").has_value());

  auto portZero = answerFor("m=audio 6000 RTP/AVP 0\nm=video 0 RTP/SAVP 96\n",
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

  const std::string portZero = "m=audio 5000 RTP/AVP 0\nm=video 0 RTP/AVP 96\na=rtpmap:96 VP8/90000\na=inactive\n";
  requireUnchanged(answerFor("m=audio 6000 RTP/AVP 0\nm=video 0 RTP/AVP 96\n", portZero), portZero);
}

// ---------------------------------------------------------------------------
// RA-T-31 session cases (b), (f), (i) (RA-SC-17 (ii))
// ---------------------------------------------------------------------------

TEST_CASE("Session-level capability negotiation or key-mgmt has no answer (RA-SC-17)", "[reoffer_answer][rasc17]")
{
  const std::string line = "m=audio 5000 RTP/AVP 0\n";
  REQUIRE_FALSE(answerFor(line, "a=tcap:1 RTP/SAVP\n" + line).has_value());
  REQUIRE_FALSE(answerFor(line, "a=pcfg:1 t=1\n" + line).has_value());
  REQUIRE_FALSE(answerFor(line, "a=key-mgmt:mikey AQAFgM0XflABAAAAAAAAAAAAAAsAyO\n" + line).has_value());
}

// ---------------------------------------------------------------------------
// RA-T-32 (RA-SC-18, HD-MDS0-1) and RA-T-34 (RA-RK-4 pin, HD-MDS0-2)
// ---------------------------------------------------------------------------

TEST_CASE("An own m=image line that stays live has no answer (RA-SC-18)", "[reoffer_answer][rasc18]")
{
  const std::string ownImage =
      "m=image 5004 udptl t38\na=T38FaxVersion:3\na=T38MaxBitRate:33600\n";
  REQUIRE_FALSE(answerFor("m=image 6004 udptl t38\na=T38FaxVersion:0\na=T38MaxBitRate:14400\n", ownImage).has_value());

  const std::string audio = "m=audio 5000 RTP/AVP 0\n";
  REQUIRE_FALSE(answerFor(audio + "m=image 6004 udptl t38\n", audio + "m=image 5004 udptl t38\n").has_value());

  const std::string bothZero = audio + "m=image 0 udptl t38\n";
  requireUnchanged(answerFor(bothZero, bothZero), bothZero);

  auto rejected = answerFor(audio + "m=image 0 udptl t38\n", audio + ownImage);
  requireChangedVersion(rejected);
  REQUIRE(rejected->answer.mediaDescriptions[1].port == 0);
  REQUIRE(rejected->answer.mediaDescriptions[1].genericAttributes.empty());
}

TEST_CASE("The H.264 level stays ownOffer's (known gap RA-RK-4)", "[reoffer_answer][rark4]")
{
  const std::string own =
      "m=video 5002 RTP/AVP 97\na=rtpmap:97 H264/90000\n"
      "a=fmtp:97 profile-level-id=42e01f;packetization-mode=1\n";
  const std::string re =
      "m=video 6002 RTP/AVP 97\na=rtpmap:97 H264/90000\n"
      "a=fmtp:97 profile-level-id=42e01e;packetization-mode=1\n";
  requireUnchanged(answerFor(re, own), own);
}

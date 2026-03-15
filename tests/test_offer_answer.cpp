#include <catch2/catch.hpp>
#include <regex>
#include <set>
#include <string>

#include "iora/sdp/offer_answer.hpp"
#include "iora/sdp/sdp_parser.hpp"
#include "iora/sdp/sdp_serializer.hpp"

using namespace iora::sdp;

// ---------------------------------------------------------------------------
// T8: Options default construction and equality
// ---------------------------------------------------------------------------

TEST_CASE("OfferOptions default construction and equality", "[offer_answer][options]")
{
  OfferOptions a;
  OfferOptions b;
  REQUIRE(a == b);
  REQUIRE(a.audio == true);
  REQUIRE(a.video == true);
  REQUIRE(a.dataChannel == false);
  REQUIRE_FALSE(a.iceUfrag.has_value());
  REQUIRE_FALSE(a.icePwd.has_value());
  REQUIRE_FALSE(a.fingerprint.has_value());
  REQUIRE(a.bundlePolicy == true);
  REQUIRE(a.rtcpMux == true);
  REQUIRE(a.dtlsSetup == SetupRole::ACTPASS);

  b.audio = false;
  REQUIRE(a != b);
}

TEST_CASE("AnswerOptions default construction and equality", "[offer_answer][options]")
{
  AnswerOptions a;
  AnswerOptions b;
  REQUIRE(a == b);
  REQUIRE_FALSE(a.iceUfrag.has_value());
  REQUIRE_FALSE(a.icePwd.has_value());
  REQUIRE_FALSE(a.fingerprint.has_value());
  REQUIRE(a.dtlsSetup == SetupRole::ACTIVE);
  REQUIRE_FALSE(a.supportedCodecs.has_value());
  REQUIRE(a.rejectMediaTypes.empty());

  b.dtlsSetup = SetupRole::PASSIVE;
  REQUIRE(a != b);
}

TEST_CASE("TransceiverOptions default construction and equality", "[offer_answer][options]")
{
  TransceiverOptions a;
  TransceiverOptions b;
  REQUIRE(a == b);
  REQUIRE(a.mid.empty());
  REQUIRE(a.direction == Direction::SENDRECV);
  REQUIRE(a.protocol == TransportProtocol::UDP_TLS_RTP_SAVPF);
  REQUIRE_FALSE(a.iceUfrag.has_value());
  REQUIRE_FALSE(a.icePwd.has_value());
  REQUIRE_FALSE(a.fingerprint.has_value());
  REQUIRE(a.setup == SetupRole::ACTPASS);
  REQUIRE_FALSE(a.msid.has_value());

  b.mid = "1";
  REQUIRE(a != b);
}

TEST_CASE("DataChannelOptions default construction and equality", "[offer_answer][options]")
{
  DataChannelOptions a;
  DataChannelOptions b;
  REQUIRE(a == b);
  REQUIRE(a.mid.empty());
  REQUIRE(a.sctpPort == 5000);
  REQUIRE(a.maxMessageSize == 262144);
  REQUIRE_FALSE(a.iceUfrag.has_value());
  REQUIRE_FALSE(a.icePwd.has_value());
  REQUIRE_FALSE(a.fingerprint.has_value());
  REQUIRE(a.setup == SetupRole::ACTPASS);

  b.sctpPort = 9999;
  REQUIRE(a != b);
}

// ---------------------------------------------------------------------------
// T8: flipDirection
// ---------------------------------------------------------------------------

TEST_CASE("flipDirection", "[offer_answer][direction]")
{
  REQUIRE(OfferAnswer::flipDirection(Direction::SENDRECV) == Direction::SENDRECV);
  REQUIRE(OfferAnswer::flipDirection(Direction::SENDONLY) == Direction::RECVONLY);
  REQUIRE(OfferAnswer::flipDirection(Direction::RECVONLY) == Direction::SENDONLY);
  REQUIRE(OfferAnswer::flipDirection(Direction::INACTIVE) == Direction::INACTIVE);
}

// ---------------------------------------------------------------------------
// T8: setMediaDirection
// ---------------------------------------------------------------------------

TEST_CASE("setMediaDirection", "[offer_answer][direction]")
{
  MediaDescription media;
  media.direction = Direction::SENDRECV;
  OfferAnswer::setMediaDirection(media, Direction::RECVONLY);
  REQUIRE(media.direction == Direction::RECVONLY);
}

// ---------------------------------------------------------------------------
// T8: disableMedia
// ---------------------------------------------------------------------------

TEST_CASE("disableMedia", "[offer_answer][media]")
{
  SECTION("sets port=0 and direction=INACTIVE")
  {
    MediaDescription media;
    media.port = 9;
    media.direction = Direction::SENDRECV;
    OfferAnswer::disableMedia(media);
    REQUIRE(media.port == 0);
    REQUIRE(media.direction == Direction::INACTIVE);
  }

  SECTION("idempotent — calling on already-disabled media is safe")
  {
    MediaDescription media;
    media.port = 0;
    media.direction = Direction::INACTIVE;
    OfferAnswer::disableMedia(media);
    REQUIRE(media.port == 0);
    REQUIRE(media.direction == Direction::INACTIVE);
  }
}

// ---------------------------------------------------------------------------
// T8: isMediaDisabled
// ---------------------------------------------------------------------------

TEST_CASE("isMediaDisabled", "[offer_answer][media]")
{
  MediaDescription media;

  media.port = 0;
  REQUIRE(OfferAnswer::isMediaDisabled(media) == true);

  media.port = 9;
  REQUIRE(OfferAnswer::isMediaDisabled(media) == false);

  media.port = 5004;
  REQUIRE(OfferAnswer::isMediaDisabled(media) == false);
}

// ---------------------------------------------------------------------------
// T8: findMediaByMid
// ---------------------------------------------------------------------------

TEST_CASE("findMediaByMid", "[offer_answer][media]")
{
  SECTION("found case returns non-null pointer to correct section")
  {
    SessionDescription session;
    MediaDescription audio;
    audio.mid = "audio0";
    audio.mediaType = MediaType::AUDIO;
    session.mediaDescriptions.push_back(audio);

    auto* found = OfferAnswer::findMediaByMid(session, "audio0");
    REQUIRE(found != nullptr);
    REQUIRE(found->mediaType == MediaType::AUDIO);
    REQUIRE(found->mid.value() == "audio0");
  }

  SECTION("not found returns nullptr")
  {
    SessionDescription session;
    MediaDescription audio;
    audio.mid = "audio0";
    session.mediaDescriptions.push_back(audio);

    REQUIRE(OfferAnswer::findMediaByMid(session, "video0") == nullptr);
  }

  SECTION("const overload returns const pointer")
  {
    SessionDescription session;
    MediaDescription video;
    video.mid = "1";
    video.mediaType = MediaType::VIDEO;
    session.mediaDescriptions.push_back(video);

    const SessionDescription& constSession = session;
    const MediaDescription* found = OfferAnswer::findMediaByMid(constSession, "1");
    REQUIRE(found != nullptr);
    REQUIRE(found->mediaType == MediaType::VIDEO);
  }

  SECTION("multiple media sections, finds correct one")
  {
    SessionDescription session;
    MediaDescription m0;
    m0.mid = "0";
    m0.mediaType = MediaType::AUDIO;
    MediaDescription m1;
    m1.mid = "1";
    m1.mediaType = MediaType::VIDEO;
    MediaDescription m2;
    m2.mid = "2";
    m2.mediaType = MediaType::APPLICATION;
    session.mediaDescriptions = {m0, m1, m2};

    auto* found = OfferAnswer::findMediaByMid(session, "1");
    REQUIRE(found != nullptr);
    REQUIRE(found->mediaType == MediaType::VIDEO);

    found = OfferAnswer::findMediaByMid(session, "2");
    REQUIRE(found != nullptr);
    REQUIRE(found->mediaType == MediaType::APPLICATION);
  }

  SECTION("empty mediaDescriptions returns nullptr")
  {
    SessionDescription session;
    REQUIRE(OfferAnswer::findMediaByMid(session, "0") == nullptr);
  }

  SECTION("media section with mid == nullopt is skipped safely")
  {
    SessionDescription session;
    MediaDescription noMid;
    noMid.mediaType = MediaType::AUDIO;
    // mid is std::nullopt by default
    MediaDescription withMid;
    withMid.mid = "1";
    withMid.mediaType = MediaType::VIDEO;
    session.mediaDescriptions = {noMid, withMid};

    REQUIRE(OfferAnswer::findMediaByMid(session, "1") != nullptr);
    REQUIRE(OfferAnswer::findMediaByMid(session, "1")->mediaType == MediaType::VIDEO);
    REQUIRE(OfferAnswer::findMediaByMid(session, "0") == nullptr);
  }
}

// ---------------------------------------------------------------------------
// T8: generateIceCredentials
// ---------------------------------------------------------------------------

TEST_CASE("generateIceCredentials", "[offer_answer][ice]")
{
  SECTION("ufrag length >= 4 (implementation uses 8)")
  {
    auto [ufrag, pwd] = OfferAnswer::generateIceCredentials();
    REQUIRE(ufrag.size() >= 4);
    REQUIRE(ufrag.size() == 8);
  }

  SECTION("pwd length >= 22 (implementation uses 24)")
  {
    auto [ufrag, pwd] = OfferAnswer::generateIceCredentials();
    REQUIRE(pwd.size() >= 22);
    REQUIRE(pwd.size() == 24);
  }

  SECTION("characters are from [a-zA-Z0-9+/]")
  {
    std::regex validChars("^[a-zA-Z0-9+/]+$");
    auto [ufrag, pwd] = OfferAnswer::generateIceCredentials();
    REQUIRE(std::regex_match(ufrag, validChars));
    REQUIRE(std::regex_match(pwd, validChars));
  }

  SECTION("multiple calls produce different values")
  {
    std::set<std::string> ufrags;
    std::set<std::string> pwds;
    for (int i = 0; i < 5; ++i)
    {
      auto [u, p] = OfferAnswer::generateIceCredentials();
      ufrags.insert(u);
      pwds.insert(p);
    }
    REQUIRE(ufrags.size() > 1);
    REQUIRE(pwds.size() > 1);
  }
}

// ---------------------------------------------------------------------------
// T8: addIceCandidate
// ---------------------------------------------------------------------------

TEST_CASE("addIceCandidate", "[offer_answer][ice]")
{
  SECTION("candidate appended to vector")
  {
    MediaDescription media;
    IceCandidate cand;
    cand.foundation = "1";
    cand.component = 1;
    cand.address = "192.168.1.1";
    cand.port = 5000;
    OfferAnswer::addIceCandidate(media, cand);
    REQUIRE(media.candidates.size() == 1);
    REQUIRE(media.candidates[0].address == "192.168.1.1");
  }

  SECTION("multiple candidates accumulate")
  {
    MediaDescription media;
    IceCandidate c1;
    c1.foundation = "1";
    c1.address = "192.168.1.1";
    IceCandidate c2;
    c2.foundation = "2";
    c2.address = "10.0.0.1";
    OfferAnswer::addIceCandidate(media, c1);
    OfferAnswer::addIceCandidate(media, c2);
    REQUIRE(media.candidates.size() == 2);
    REQUIRE(media.candidates[0].address == "192.168.1.1");
    REQUIRE(media.candidates[1].address == "10.0.0.1");
  }
}

// ---------------------------------------------------------------------------
// T8: removeIceCandidates
// ---------------------------------------------------------------------------

TEST_CASE("removeIceCandidates", "[offer_answer][ice]")
{
  SECTION("candidates vector cleared and endOfCandidates set to false")
  {
    MediaDescription media;
    IceCandidate cand;
    cand.foundation = "1";
    media.candidates.push_back(cand);
    media.endOfCandidates = true;
    OfferAnswer::removeIceCandidates(media);
    REQUIRE(media.candidates.empty());
    REQUIRE(media.endOfCandidates == false);
  }

  SECTION("on already-empty candidates is a no-op")
  {
    MediaDescription media;
    media.endOfCandidates = false;
    OfferAnswer::removeIceCandidates(media);
    REQUIRE(media.candidates.empty());
    REQUIRE(media.endOfCandidates == false);
  }
}

// ---------------------------------------------------------------------------
// T9: matchCodecs
// ---------------------------------------------------------------------------

TEST_CASE("matchCodecs", "[offer_answer][codecs]")
{
  SECTION("basic intersection — offer has opus+PCMU, answer has PCMU+opus, result is PCMU then opus")
  {
    MediaDescription offer;
    offer.rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});
    offer.rtpMaps.push_back({0, "PCMU", 8000, std::uint8_t(1)});

    MediaDescription answer;
    answer.rtpMaps.push_back({0, "PCMU", 8000, std::uint8_t(1)});
    answer.rtpMaps.push_back({96, "opus", 48000, std::uint8_t(2)});

    auto result = OfferAnswer::matchCodecs(offer, answer);
    REQUIRE(result.size() == 2);
    REQUIRE(result[0].encodingName == "PCMU");
    REQUIRE(result[0].payloadType == 0);
    REQUIRE(result[1].encodingName == "opus");
    REQUIRE(result[1].payloadType == 96);
  }

  SECTION("case-insensitive encoding name matching")
  {
    MediaDescription offer;
    offer.rtpMaps.push_back({111, "OPUS", 48000, std::uint8_t(2)});

    MediaDescription answer;
    answer.rtpMaps.push_back({96, "opus", 48000, std::uint8_t(2)});

    auto result = OfferAnswer::matchCodecs(offer, answer);
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].encodingName == "opus");
    REQUIRE(result[0].payloadType == 96);
  }

  SECTION("clock rate must match — same name different rate produces no match")
  {
    MediaDescription offer;
    offer.rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});

    MediaDescription answer;
    answer.rtpMaps.push_back({96, "opus", 16000, std::uint8_t(2)});

    auto result = OfferAnswer::matchCodecs(offer, answer);
    REQUIRE(result.empty());
  }

  SECTION("no overlap returns empty vector")
  {
    MediaDescription offer;
    offer.rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});

    MediaDescription answer;
    answer.rtpMaps.push_back({96, "VP8", 90000, {}});

    auto result = OfferAnswer::matchCodecs(offer, answer);
    REQUIRE(result.empty());
  }

  SECTION("answerer's payload type numbers used in result")
  {
    MediaDescription offer;
    offer.rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});

    MediaDescription answer;
    answer.rtpMaps.push_back({120, "opus", 48000, std::uint8_t(2)});

    auto result = OfferAnswer::matchCodecs(offer, answer);
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].payloadType == 120);
  }

  SECTION("video codecs work correctly")
  {
    MediaDescription offer;
    offer.rtpMaps.push_back({96, "VP8", 90000, {}});
    offer.rtpMaps.push_back({97, "H264", 90000, {}});

    MediaDescription answer;
    answer.rtpMaps.push_back({100, "H264", 90000, {}});
    answer.rtpMaps.push_back({101, "VP8", 90000, {}});

    auto result = OfferAnswer::matchCodecs(offer, answer);
    REQUIRE(result.size() == 2);
    REQUIRE(result[0].encodingName == "H264");
    REQUIRE(result[0].payloadType == 100);
    REQUIRE(result[1].encodingName == "VP8");
    REQUIRE(result[1].payloadType == 101);
  }

  SECTION("empty rtpMaps in offer returns empty vector")
  {
    MediaDescription offer;
    MediaDescription answer;
    answer.rtpMaps.push_back({96, "opus", 48000, std::uint8_t(2)});

    auto result = OfferAnswer::matchCodecs(offer, answer);
    REQUIRE(result.empty());
  }

  SECTION("empty rtpMaps in answer returns empty vector")
  {
    MediaDescription offer;
    offer.rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});
    MediaDescription answer;

    auto result = OfferAnswer::matchCodecs(offer, answer);
    REQUIRE(result.empty());
  }
}

// ---------------------------------------------------------------------------
// T9: pruneUnusedCodecs
// ---------------------------------------------------------------------------

TEST_CASE("pruneUnusedCodecs", "[offer_answer][codecs]")
{
  SECTION("removes rtpmap, fmtp, rtcp-fb for non-retained PTs")
  {
    MediaDescription media;
    media.rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});
    media.rtpMaps.push_back({0, "PCMU", 8000, std::uint8_t(1)});
    media.fmtps.push_back({111, "minptime=10;useinbandfec=1", {}});
    media.fmtps.push_back({0, "mode=20", {}});
    media.rtcpFeedbacks.push_back({"111", "transport-cc", {}});
    media.rtcpFeedbacks.push_back({"0", "nack", {}});
    media.formats = {"111", "0"};

    OfferAnswer::pruneUnusedCodecs(media, {111});

    REQUIRE(media.rtpMaps.size() == 1);
    REQUIRE(media.rtpMaps[0].payloadType == 111);
    REQUIRE(media.fmtps.size() == 1);
    REQUIRE(media.fmtps[0].payloadType == 111);
    REQUIRE(media.rtcpFeedbacks.size() == 1);
    REQUIRE(media.rtcpFeedbacks[0].payloadType == "111");
    REQUIRE(media.formats.size() == 1);
    REQUIRE(media.formats[0] == "111");
  }

  SECTION("retains wildcard rtcp-fb ('*')")
  {
    MediaDescription media;
    media.rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});
    media.rtpMaps.push_back({0, "PCMU", 8000, std::uint8_t(1)});
    media.rtcpFeedbacks.push_back({"*", "transport-cc", {}});
    media.rtcpFeedbacks.push_back({"0", "nack", {}});
    media.formats = {"111", "0"};

    OfferAnswer::pruneUnusedCodecs(media, {111});

    REQUIRE(media.rtcpFeedbacks.size() == 1);
    REQUIRE(media.rtcpFeedbacks[0].payloadType == "*");
  }

  SECTION("non-numeric formats preserved")
  {
    MediaDescription media;
    media.rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});
    media.formats = {"111", "webrtc-datachannel"};

    OfferAnswer::pruneUnusedCodecs(media, {});

    REQUIRE(media.rtpMaps.empty());
    REQUIRE(media.formats.size() == 1);
    REQUIRE(media.formats[0] == "webrtc-datachannel");
  }

  SECTION("empty retain list removes all codec attributes but preserves non-numeric formats")
  {
    MediaDescription media;
    media.rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});
    media.rtpMaps.push_back({96, "VP8", 90000, {}});
    media.fmtps.push_back({111, "minptime=10", {}});
    media.rtcpFeedbacks.push_back({"111", "transport-cc", {}});
    media.rtcpFeedbacks.push_back({"*", "nack", {}});
    media.formats = {"111", "96", "webrtc-datachannel"};

    OfferAnswer::pruneUnusedCodecs(media, {});

    REQUIRE(media.rtpMaps.empty());
    REQUIRE(media.fmtps.empty());
    REQUIRE(media.rtcpFeedbacks.size() == 1);
    REQUIRE(media.rtcpFeedbacks[0].payloadType == "*");
    REQUIRE(media.formats.size() == 1);
    REQUIRE(media.formats[0] == "webrtc-datachannel");
  }

  SECTION("on media with no codec attributes is a no-op")
  {
    MediaDescription media;
    media.mediaType = MediaType::APPLICATION;
    media.formats = {"webrtc-datachannel"};
    media.sctpPort = 5000;

    OfferAnswer::pruneUnusedCodecs(media, {111});

    REQUIRE(media.formats.size() == 1);
    REQUIRE(media.formats[0] == "webrtc-datachannel");
    REQUIRE(media.rtpMaps.empty());
    REQUIRE(media.fmtps.empty());
    REQUIRE(media.rtcpFeedbacks.empty());
  }
}

// ---------------------------------------------------------------------------
// T9: addTransceiver
// ---------------------------------------------------------------------------

TEST_CASE("addTransceiver", "[offer_answer][factory]")
{
  SECTION("audio with defaults")
  {
    TransceiverOptions opts;
    opts.mid = "0";
    auto media = OfferAnswer::addTransceiver(MediaType::AUDIO, opts);

    REQUIRE(media.mediaType == MediaType::AUDIO);
    REQUIRE(media.port == 9);
    REQUIRE(media.protocol == TransportProtocol::UDP_TLS_RTP_SAVPF);
    REQUIRE(media.direction == Direction::SENDRECV);
    REQUIRE(media.mid.has_value());
    REQUIRE(media.mid.value() == "0");
    REQUIRE(media.rtcpMux == true);
    REQUIRE(media.setup.has_value());
    REQUIRE(media.setup.value() == SetupRole::ACTPASS);
    // Optional fields should be absent when not provided
    REQUIRE_FALSE(media.iceUfrag.has_value());
    REQUIRE_FALSE(media.icePwd.has_value());
    REQUIRE(media.fingerprints.empty());
    REQUIRE(media.msid.empty());
  }

  SECTION("video with custom options")
  {
    TransceiverOptions opts;
    opts.mid = "1";
    opts.direction = Direction::RECVONLY;
    opts.iceUfrag = "testufrag";
    opts.icePwd = "testpwd123456789012345";
    opts.fingerprint = FingerprintAttribute{"sha-256", "AA:BB:CC"};
    opts.setup = SetupRole::ACTIVE;
    opts.msid = MsidAttribute{"stream0", "track0"};

    auto media = OfferAnswer::addTransceiver(MediaType::VIDEO, opts);

    REQUIRE(media.mediaType == MediaType::VIDEO);
    REQUIRE(media.direction == Direction::RECVONLY);
    REQUIRE(media.mid.value() == "1");
    REQUIRE(media.iceUfrag.has_value());
    REQUIRE(media.iceUfrag.value() == "testufrag");
    REQUIRE(media.icePwd.has_value());
    REQUIRE(media.icePwd.value() == "testpwd123456789012345");
    REQUIRE(media.fingerprints.size() == 1);
    REQUIRE(media.fingerprints[0].hashFunction == "sha-256");
    REQUIRE(media.fingerprints[0].fingerprint == "AA:BB:CC");
    REQUIRE(media.setup.value() == SetupRole::ACTIVE);
    REQUIRE(media.msid.size() == 1);
    REQUIRE(media.msid[0].streamId == "stream0");
    REQUIRE(media.msid[0].trackId.value() == "track0");
  }

  SECTION("msid from options pushed into msid vector (single element)")
  {
    TransceiverOptions opts;
    opts.mid = "a";
    opts.msid = MsidAttribute{"mystream", std::nullopt};

    auto media = OfferAnswer::addTransceiver(MediaType::AUDIO, opts);
    REQUIRE(media.msid.size() == 1);
    REQUIRE(media.msid[0].streamId == "mystream");
    REQUIRE_FALSE(media.msid[0].trackId.has_value());
  }

  SECTION("formats vector is empty (application adds codecs)")
  {
    TransceiverOptions opts;
    opts.mid = "0";
    auto media = OfferAnswer::addTransceiver(MediaType::AUDIO, opts);
    REQUIRE(media.formats.empty());
  }
}

// ---------------------------------------------------------------------------
// T9: addDataChannel
// ---------------------------------------------------------------------------

TEST_CASE("addDataChannel", "[offer_answer][factory]")
{
  SECTION("default options")
  {
    DataChannelOptions opts;
    opts.mid = "2";
    auto media = OfferAnswer::addDataChannel(opts);

    REQUIRE(media.mediaType == MediaType::APPLICATION);
    REQUIRE(media.port == 9);
    REQUIRE(media.protocol == TransportProtocol::UDP_DTLS_SCTP);
    REQUIRE(media.formats.size() == 1);
    REQUIRE(media.formats[0] == "webrtc-datachannel");
    REQUIRE(media.direction == Direction::SENDRECV);
    REQUIRE(media.mid.has_value());
    REQUIRE(media.mid.value() == "2");
    REQUIRE(media.sctpPort.has_value());
    REQUIRE(media.sctpPort.value() == 5000);
    REQUIRE(media.maxMessageSize.has_value());
    REQUIRE(media.maxMessageSize.value() == 262144);
    REQUIRE(media.setup.has_value());
    REQUIRE(media.setup.value() == SetupRole::ACTPASS);
    // Optional fields should be absent when not provided
    REQUIRE_FALSE(media.iceUfrag.has_value());
    REQUIRE_FALSE(media.icePwd.has_value());
    REQUIRE(media.fingerprints.empty());
  }

  SECTION("custom options")
  {
    DataChannelOptions opts;
    opts.mid = "dc0";
    opts.sctpPort = 9999;
    opts.maxMessageSize = 65536;
    opts.iceUfrag = "dcufrag";
    opts.icePwd = "dcpwd12345678901234567";
    opts.fingerprint = FingerprintAttribute{"sha-1", "11:22:33"};
    opts.setup = SetupRole::PASSIVE;

    auto media = OfferAnswer::addDataChannel(opts);

    REQUIRE(media.sctpPort.value() == 9999);
    REQUIRE(media.maxMessageSize.value() == 65536);
    REQUIRE(media.iceUfrag.value() == "dcufrag");
    REQUIRE(media.icePwd.value() == "dcpwd12345678901234567");
    REQUIRE(media.fingerprints.size() == 1);
    REQUIRE(media.fingerprints[0].hashFunction == "sha-1");
    REQUIRE(media.setup.value() == SetupRole::PASSIVE);
  }
}

// ---------------------------------------------------------------------------
// T9: createOffer
// ---------------------------------------------------------------------------

TEST_CASE("createOffer", "[offer_answer][offer]")
{
  SECTION("default options — audio+video, no data channel")
  {
    auto offer = OfferAnswer::createOffer();
    REQUIRE(offer.mediaDescriptions.size() == 2);
    REQUIRE(offer.mediaDescriptions[0].mediaType == MediaType::AUDIO);
    REQUIRE(offer.mediaDescriptions[1].mediaType == MediaType::VIDEO);
  }

  SECTION("verify v=0")
  {
    auto offer = OfferAnswer::createOffer();
    REQUIRE(offer.version == 0);
  }

  SECTION("origin with random sessId, sessVersion='1', username='-', address='0.0.0.0'")
  {
    auto offer = OfferAnswer::createOffer();
    REQUIRE_FALSE(offer.origin.sessId.empty());
    REQUIRE(offer.origin.sessVersion == "1");
    REQUIRE(offer.origin.username == "-");
    REQUIRE(offer.origin.netType == NetworkType::IN);
    REQUIRE(offer.origin.addrType == AddressType::IP4);
    REQUIRE(offer.origin.address == "0.0.0.0");
    REQUIRE(offer.sessionName == "-");
  }

  SECTION("timeDescriptions has one entry with startTime=0, stopTime=0")
  {
    auto offer = OfferAnswer::createOffer();
    REQUIRE(offer.timeDescriptions.size() == 1);
    REQUIRE(offer.timeDescriptions[0].startTime == 0);
    REQUIRE(offer.timeDescriptions[0].stopTime == 0);
    REQUIRE(offer.timeDescriptions[0].repeatTimes.empty());
  }

  SECTION("ICE credentials at session level (auto-generated)")
  {
    auto offer = OfferAnswer::createOffer();
    REQUIRE(offer.iceUfrag.has_value());
    REQUIRE(offer.icePwd.has_value());
    REQUIRE(offer.iceUfrag.value().size() >= 4);
    REQUIRE(offer.icePwd.value().size() >= 22);
  }

  SECTION("ICE credentials use provided values when given")
  {
    OfferOptions opts;
    opts.iceUfrag = "myufrag1";
    opts.icePwd = "mypwd123456789012345678";

    auto offer = OfferAnswer::createOffer(opts);
    REQUIRE(offer.iceUfrag.value() == "myufrag1");
    REQUIRE(offer.icePwd.value() == "mypwd123456789012345678");
  }

  SECTION("fingerprint at session level when provided")
  {
    OfferOptions opts;
    opts.fingerprint = FingerprintAttribute{"sha-256", "AA:BB:CC:DD"};

    auto offer = OfferAnswer::createOffer(opts);
    REQUIRE(offer.fingerprints.size() == 1);
    REQUIRE(offer.fingerprints[0].hashFunction == "sha-256");
    REQUIRE(offer.fingerprints[0].fingerprint == "AA:BB:CC:DD");
  }

  SECTION("no fingerprint when not provided")
  {
    auto offer = OfferAnswer::createOffer();
    REQUIRE(offer.fingerprints.empty());
  }

  SECTION("BUNDLE group with correct mids when bundlePolicy=true")
  {
    auto offer = OfferAnswer::createOffer();
    REQUIRE(offer.groups.size() == 1);
    REQUIRE(offer.groups[0].semantics == "BUNDLE");
    REQUIRE(offer.groups[0].mids.size() == 2);
    REQUIRE(offer.groups[0].mids[0] == "0");
    REQUIRE(offer.groups[0].mids[1] == "1");
  }

  SECTION("no BUNDLE group when bundlePolicy=false")
  {
    OfferOptions opts;
    opts.bundlePolicy = false;
    auto offer = OfferAnswer::createOffer(opts);
    REQUIRE(offer.groups.empty());
  }

  SECTION("rtcpMux=true on media sections")
  {
    auto offer = OfferAnswer::createOffer();
    for (const auto& m : offer.mediaDescriptions)
    {
      REQUIRE(m.rtcpMux == true);
    }
  }

  SECTION("rtcpMux=false on media sections")
  {
    OfferOptions opts;
    opts.rtcpMux = false;
    auto offer = OfferAnswer::createOffer(opts);
    REQUIRE(offer.mediaDescriptions[0].rtcpMux == false);
    REQUIRE(offer.mediaDescriptions[1].rtcpMux == false);
  }

  SECTION("audio=false, video=true — only video section")
  {
    OfferOptions opts;
    opts.audio = false;
    opts.video = true;
    auto offer = OfferAnswer::createOffer(opts);
    REQUIRE(offer.mediaDescriptions.size() == 1);
    REQUIRE(offer.mediaDescriptions[0].mediaType == MediaType::VIDEO);
    REQUIRE(offer.mediaDescriptions[0].mid.value() == "0");
  }

  SECTION("audio=true, video=false — only audio section")
  {
    OfferOptions opts;
    opts.audio = true;
    opts.video = false;
    auto offer = OfferAnswer::createOffer(opts);
    REQUIRE(offer.mediaDescriptions.size() == 1);
    REQUIRE(offer.mediaDescriptions[0].mediaType == MediaType::AUDIO);
    REQUIRE(offer.mediaDescriptions[0].mid.value() == "0");
  }

  SECTION("dataChannel=true — application section added")
  {
    OfferOptions opts;
    opts.audio = false;
    opts.video = false;
    opts.dataChannel = true;
    auto offer = OfferAnswer::createOffer(opts);
    REQUIRE(offer.mediaDescriptions.size() == 1);
    REQUIRE(offer.mediaDescriptions[0].mediaType == MediaType::APPLICATION);
    REQUIRE(offer.mediaDescriptions[0].sctpPort.has_value());
    REQUIRE(offer.mediaDescriptions[0].sctpPort.value() == 5000);
    REQUIRE(offer.mediaDescriptions[0].maxMessageSize.has_value());
    REQUIRE(offer.mediaDescriptions[0].maxMessageSize.value() == 262144);
  }

  SECTION("audio+video+dataChannel — three sections with mids '0','1','2'")
  {
    OfferOptions opts;
    opts.dataChannel = true;
    auto offer = OfferAnswer::createOffer(opts);
    REQUIRE(offer.mediaDescriptions.size() == 3);
    REQUIRE(offer.mediaDescriptions[0].mid.value() == "0");
    REQUIRE(offer.mediaDescriptions[0].mediaType == MediaType::AUDIO);
    REQUIRE(offer.mediaDescriptions[1].mid.value() == "1");
    REQUIRE(offer.mediaDescriptions[1].mediaType == MediaType::VIDEO);
    REQUIRE(offer.mediaDescriptions[2].mid.value() == "2");
    REQUIRE(offer.mediaDescriptions[2].mediaType == MediaType::APPLICATION);

    REQUIRE(offer.groups.size() == 1);
    REQUIRE(offer.groups[0].mids.size() == 3);
    REQUIRE(offer.groups[0].mids[2] == "2");
  }

  SECTION("audio=false, video=false, dataChannel=false — no media sections")
  {
    OfferOptions opts;
    opts.audio = false;
    opts.video = false;
    opts.dataChannel = false;
    auto offer = OfferAnswer::createOffer(opts);
    REQUIRE(offer.mediaDescriptions.empty());
    REQUIRE(offer.groups.empty());
  }

  SECTION("setup = options.dtlsSetup")
  {
    OfferOptions opts;
    opts.dtlsSetup = SetupRole::PASSIVE;
    auto offer = OfferAnswer::createOffer(opts);
    REQUIRE(offer.setup.has_value());
    REQUIRE(offer.setup.value() == SetupRole::PASSIVE);
  }

  SECTION("iceOptions = {'trickle'}")
  {
    auto offer = OfferAnswer::createOffer();
    REQUIRE(offer.iceOptions.size() == 1);
    REQUIRE(offer.iceOptions[0] == "trickle");
  }

  SECTION("serializes to valid SDP (parse succeeds with no errors)")
  {
    OfferOptions opts;
    opts.iceUfrag = "testufrag";
    opts.icePwd = "testpwd123456789012345678";
    auto offer = OfferAnswer::createOffer(opts);

    // Add codecs so m= lines have formats
    offer.mediaDescriptions[0].rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});
    offer.mediaDescriptions[0].formats.push_back("111");
    offer.mediaDescriptions[1].rtpMaps.push_back({96, "VP8", 90000, {}});
    offer.mediaDescriptions[1].formats.push_back("96");

    auto sdp = SdpSerializer::serialize(offer);
    auto result = SdpParser::parse(sdp);
    REQUIRE_FALSE(result.error.has_value());
    REQUIRE(result.value.has_value());
  }
}

// ---------------------------------------------------------------------------
// T9: createAnswer
// ---------------------------------------------------------------------------

TEST_CASE("createAnswer", "[offer_answer][answer]")
{
  // Build a basic offer for most tests
  auto makeOffer = []()
  {
    OfferOptions opts;
    opts.iceUfrag = "offerufrag";
    opts.icePwd = "offerpwd12345678901234";
    auto offer = OfferAnswer::createOffer(opts);

    // Add codecs
    offer.mediaDescriptions[0].rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});
    offer.mediaDescriptions[0].rtpMaps.push_back({0, "PCMU", 8000, std::uint8_t(1)});
    offer.mediaDescriptions[0].formats = {"111", "0"};

    offer.mediaDescriptions[1].rtpMaps.push_back({96, "VP8", 90000, {}});
    offer.mediaDescriptions[1].rtpMaps.push_back({97, "H264", 90000, {}});
    offer.mediaDescriptions[1].formats = {"96", "97"};

    return offer;
  };

  SECTION("basic audio+video offer produces correct answer")
  {
    auto offer = makeOffer();
    auto answer = OfferAnswer::createAnswer(offer);

    REQUIRE(answer.version == 0);
    REQUIRE(answer.sessionName == "-");
    REQUIRE_FALSE(answer.origin.sessId.empty());
    REQUIRE(answer.origin.sessVersion == "1");
    REQUIRE(answer.origin.username == "-");
    REQUIRE(answer.origin.address == "0.0.0.0");
    REQUIRE(answer.timeDescriptions.size() == 1);
    REQUIRE(answer.timeDescriptions[0].startTime == 0);
    REQUIRE(answer.timeDescriptions[0].stopTime == 0);
    REQUIRE(answer.iceOptions.size() == 1);
    REQUIRE(answer.iceOptions[0] == "trickle");
    REQUIRE(answer.mediaDescriptions.size() == 2);
    REQUIRE(answer.mediaDescriptions[0].mediaType == MediaType::AUDIO);
    REQUIRE(answer.mediaDescriptions[1].mediaType == MediaType::VIDEO);
    REQUIRE(answer.mediaDescriptions[0].port == 9);
    REQUIRE(answer.mediaDescriptions[1].port == 9);
  }

  SECTION("direction flipped")
  {
    auto offer = makeOffer();
    offer.mediaDescriptions[0].direction = Direction::SENDONLY;
    offer.mediaDescriptions[1].direction = Direction::RECVONLY;

    auto answer = OfferAnswer::createAnswer(offer);

    REQUIRE(answer.mediaDescriptions[0].direction == Direction::RECVONLY);
    REQUIRE(answer.mediaDescriptions[1].direction == Direction::SENDONLY);
  }

  SECTION("direction flipped — sendrecv stays sendrecv, inactive stays inactive")
  {
    auto offer = makeOffer();
    offer.mediaDescriptions[0].direction = Direction::SENDRECV;
    offer.mediaDescriptions[1].direction = Direction::INACTIVE;

    auto answer = OfferAnswer::createAnswer(offer);

    REQUIRE(answer.mediaDescriptions[0].direction == Direction::SENDRECV);
    REQUIRE(answer.mediaDescriptions[1].direction == Direction::INACTIVE);
  }

  SECTION("mid values copied from offer")
  {
    auto offer = makeOffer();
    auto answer = OfferAnswer::createAnswer(offer);

    REQUIRE(answer.mediaDescriptions[0].mid.value() == "0");
    REQUIRE(answer.mediaDescriptions[1].mid.value() == "1");
  }

  SECTION("BUNDLE group copied from offer")
  {
    auto offer = makeOffer();
    REQUIRE(offer.groups.size() == 1);

    auto answer = OfferAnswer::createAnswer(offer);

    REQUIRE(answer.groups.size() == 1);
    REQUIRE(answer.groups[0].semantics == "BUNDLE");
    REQUIRE(answer.groups[0].mids == offer.groups[0].mids);
  }

  SECTION("answerer's ICE credentials used (not offerer's)")
  {
    auto offer = makeOffer();

    AnswerOptions answerOpts;
    answerOpts.iceUfrag = "answerufrag1";
    answerOpts.icePwd = "answerpwd1234567890123";

    auto answer = OfferAnswer::createAnswer(offer, answerOpts);

    REQUIRE(answer.iceUfrag.value() == "answerufrag1");
    REQUIRE(answer.icePwd.value() == "answerpwd1234567890123");
    REQUIRE(answer.iceUfrag.value() != offer.iceUfrag.value());
  }

  SECTION("answerer's ICE credentials auto-generated")
  {
    auto offer = makeOffer();
    auto answer = OfferAnswer::createAnswer(offer);

    REQUIRE(answer.iceUfrag.has_value());
    REQUIRE(answer.icePwd.has_value());
    REQUIRE(answer.iceUfrag.value() != offer.iceUfrag.value());
  }

  SECTION("answerer's fingerprint and setup used")
  {
    auto offer = makeOffer();

    AnswerOptions answerOpts;
    answerOpts.fingerprint = FingerprintAttribute{"sha-256", "DD:EE:FF"};
    answerOpts.dtlsSetup = SetupRole::ACTIVE;

    auto answer = OfferAnswer::createAnswer(offer, answerOpts);

    REQUIRE(answer.fingerprints.size() == 1);
    REQUIRE(answer.fingerprints[0].fingerprint == "DD:EE:FF");
    REQUIRE(answer.setup.has_value());
    REQUIRE(answer.setup.value() == SetupRole::ACTIVE);
  }

  SECTION("supportedCodecs provided — only matching codecs in answer")
  {
    auto offer = makeOffer();

    AnswerOptions answerOpts;
    answerOpts.iceUfrag = "au";
    answerOpts.icePwd = "ap1234567890123456789012";
    // Answerer supports opus (with different PT) and VP8
    answerOpts.supportedCodecs = std::vector<RtpMapAttribute>{
      {120, "opus", 48000, std::uint8_t(2)},
      {100, "VP8", 90000, {}}
    };

    auto answer = OfferAnswer::createAnswer(offer, answerOpts);

    // Audio: only opus matched (answerer PT 120)
    REQUIRE(answer.mediaDescriptions[0].rtpMaps.size() == 1);
    REQUIRE(answer.mediaDescriptions[0].rtpMaps[0].encodingName == "opus");
    REQUIRE(answer.mediaDescriptions[0].rtpMaps[0].payloadType == 120);
    REQUIRE(answer.mediaDescriptions[0].formats.size() == 1);
    REQUIRE(answer.mediaDescriptions[0].formats[0] == "120");

    // Video: only VP8 matched (answerer PT 100)
    REQUIRE(answer.mediaDescriptions[1].rtpMaps.size() == 1);
    REQUIRE(answer.mediaDescriptions[1].rtpMaps[0].encodingName == "VP8");
    REQUIRE(answer.mediaDescriptions[1].rtpMaps[0].payloadType == 100);
    REQUIRE(answer.mediaDescriptions[1].formats.size() == 1);
    REQUIRE(answer.mediaDescriptions[1].formats[0] == "100");
  }

  SECTION("no codec overlap — media section rejected (port=0, direction=INACTIVE)")
  {
    auto offer = makeOffer();

    AnswerOptions answerOpts;
    answerOpts.iceUfrag = "au";
    answerOpts.icePwd = "ap1234567890123456789012";
    // Answerer only supports G722 — no overlap with offer
    answerOpts.supportedCodecs = std::vector<RtpMapAttribute>{
      {9, "G722", 8000, std::uint8_t(1)}
    };

    auto answer = OfferAnswer::createAnswer(offer, answerOpts);

    // Both audio and video rejected (no match)
    REQUIRE(answer.mediaDescriptions[0].port == 0);
    REQUIRE(answer.mediaDescriptions[0].direction == Direction::INACTIVE);
    REQUIRE(answer.mediaDescriptions[1].port == 0);
    REQUIRE(answer.mediaDescriptions[1].direction == Direction::INACTIVE);
  }

  SECTION("supportedCodecs not provided — all offered codecs accepted")
  {
    auto offer = makeOffer();

    AnswerOptions answerOpts;
    answerOpts.iceUfrag = "au";
    answerOpts.icePwd = "ap1234567890123456789012";

    auto answer = OfferAnswer::createAnswer(offer, answerOpts);

    // Audio: all offered codecs accepted
    REQUIRE(answer.mediaDescriptions[0].rtpMaps.size() == 2);
    REQUIRE(answer.mediaDescriptions[0].rtpMaps[0].encodingName == "opus");
    REQUIRE(answer.mediaDescriptions[0].rtpMaps[1].encodingName == "PCMU");

    // Video: all offered codecs accepted
    REQUIRE(answer.mediaDescriptions[1].rtpMaps.size() == 2);
    REQUIRE(answer.mediaDescriptions[1].rtpMaps[0].encodingName == "VP8");
    REQUIRE(answer.mediaDescriptions[1].rtpMaps[1].encodingName == "H264");
  }

  SECTION("rejectMediaTypes — specified types rejected with port=0")
  {
    auto offer = makeOffer();

    AnswerOptions answerOpts;
    answerOpts.iceUfrag = "au";
    answerOpts.icePwd = "ap1234567890123456789012";
    answerOpts.rejectMediaTypes = {MediaType::VIDEO};

    auto answer = OfferAnswer::createAnswer(offer, answerOpts);

    // Audio accepted
    REQUIRE(answer.mediaDescriptions[0].port == 9);
    REQUIRE(answer.mediaDescriptions[0].mediaType == MediaType::AUDIO);

    // Video rejected
    REQUIRE(answer.mediaDescriptions[1].port == 0);
    REQUIRE(answer.mediaDescriptions[1].direction == Direction::INACTIVE);
    REQUIRE(answer.mediaDescriptions[1].mediaType == MediaType::VIDEO);
  }

  SECTION("mixed media — offer has audio+video+data, reject video, accept audio+data")
  {
    OfferOptions offerOpts;
    offerOpts.iceUfrag = "offerufrag";
    offerOpts.icePwd = "offerpwd12345678901234";
    offerOpts.dataChannel = true;
    auto offer = OfferAnswer::createOffer(offerOpts);

    // Add codecs to audio and video
    offer.mediaDescriptions[0].rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});
    offer.mediaDescriptions[0].formats = {"111"};
    offer.mediaDescriptions[1].rtpMaps.push_back({96, "VP8", 90000, {}});
    offer.mediaDescriptions[1].formats = {"96"};

    AnswerOptions answerOpts;
    answerOpts.iceUfrag = "au";
    answerOpts.icePwd = "ap1234567890123456789012";
    answerOpts.rejectMediaTypes = {MediaType::VIDEO};

    auto answer = OfferAnswer::createAnswer(offer, answerOpts);

    REQUIRE(answer.mediaDescriptions.size() == 3);

    // Audio accepted
    REQUIRE(answer.mediaDescriptions[0].port == 9);
    REQUIRE(answer.mediaDescriptions[0].mediaType == MediaType::AUDIO);

    // Video rejected
    REQUIRE(answer.mediaDescriptions[1].port == 0);
    REQUIRE(answer.mediaDescriptions[1].direction == Direction::INACTIVE);

    // Data channel accepted
    REQUIRE(answer.mediaDescriptions[2].port == 9);
    REQUIRE(answer.mediaDescriptions[2].mediaType == MediaType::APPLICATION);
  }

  SECTION("data channel section copied correctly")
  {
    OfferOptions offerOpts;
    offerOpts.audio = false;
    offerOpts.video = false;
    offerOpts.dataChannel = true;
    offerOpts.iceUfrag = "offerufrag";
    offerOpts.icePwd = "offerpwd12345678901234";
    auto offer = OfferAnswer::createOffer(offerOpts);

    AnswerOptions answerOpts;
    answerOpts.iceUfrag = "au";
    answerOpts.icePwd = "ap1234567890123456789012";

    auto answer = OfferAnswer::createAnswer(offer, answerOpts);

    REQUIRE(answer.mediaDescriptions.size() == 1);
    auto& dc = answer.mediaDescriptions[0];
    REQUIRE(dc.mediaType == MediaType::APPLICATION);
    REQUIRE(dc.port == 9);
    REQUIRE(dc.sctpPort.has_value());
    REQUIRE(dc.sctpPort.value() == 5000);
    REQUIRE(dc.maxMessageSize.has_value());
    REQUIRE(dc.maxMessageSize.value() == 262144);
    REQUIRE(dc.formats.size() == 1);
    REQUIRE(dc.formats[0] == "webrtc-datachannel");
    REQUIRE(dc.direction == Direction::SENDRECV);
  }

  SECTION("answer formats vector populated from accepted rtpMaps")
  {
    auto offer = makeOffer();

    AnswerOptions answerOpts;
    answerOpts.iceUfrag = "au";
    answerOpts.icePwd = "ap1234567890123456789012";

    auto answer = OfferAnswer::createAnswer(offer, answerOpts);

    // Audio: formats from rtpMaps
    REQUIRE(answer.mediaDescriptions[0].formats.size() == 2);
    REQUIRE(answer.mediaDescriptions[0].formats[0] == "111");
    REQUIRE(answer.mediaDescriptions[0].formats[1] == "0");

    // Video: formats from rtpMaps
    REQUIRE(answer.mediaDescriptions[1].formats.size() == 2);
    REQUIRE(answer.mediaDescriptions[1].formats[0] == "96");
    REQUIRE(answer.mediaDescriptions[1].formats[1] == "97");
  }

  SECTION("offer with empty mediaDescriptions produces answer with no media sections")
  {
    SessionDescription emptyOffer;
    emptyOffer.version = 0;
    emptyOffer.sessionName = "-";
    emptyOffer.timeDescriptions.push_back({0, 0, {}});
    emptyOffer.iceUfrag = "ou";
    emptyOffer.icePwd = "op1234567890123456789012";

    AnswerOptions answerOpts;
    answerOpts.iceUfrag = "au";
    answerOpts.icePwd = "ap1234567890123456789012";

    auto answer = OfferAnswer::createAnswer(emptyOffer, answerOpts);

    REQUIRE(answer.mediaDescriptions.empty());
    REQUIRE(answer.version == 0);
    REQUIRE(answer.sessionName == "-");
  }

  SECTION("serializes to valid SDP (parse succeeds with no errors)")
  {
    auto offer = makeOffer();

    AnswerOptions answerOpts;
    answerOpts.iceUfrag = "answerufrag1";
    answerOpts.icePwd = "answerpwd1234567890123";

    auto answer = OfferAnswer::createAnswer(offer, answerOpts);

    auto sdp = SdpSerializer::serialize(answer);
    auto result = SdpParser::parse(sdp);
    REQUIRE_FALSE(result.error.has_value());
    REQUIRE(result.value.has_value());
  }

  SECTION("rtcpMux copied from offer")
  {
    auto offer = makeOffer();
    offer.mediaDescriptions[0].rtcpMux = true;
    offer.mediaDescriptions[1].rtcpMux = false;

    AnswerOptions answerOpts;
    answerOpts.iceUfrag = "au";
    answerOpts.icePwd = "ap1234567890123456789012";

    auto answer = OfferAnswer::createAnswer(offer, answerOpts);

    REQUIRE(answer.mediaDescriptions[0].rtcpMux == true);
    REQUIRE(answer.mediaDescriptions[1].rtcpMux == false);
  }

  SECTION("extMaps and fmtps copied when supportedCodecs not provided")
  {
    auto offer = makeOffer();
    offer.mediaDescriptions[0].fmtps.push_back({111, "minptime=10;useinbandfec=1", {}});
    offer.mediaDescriptions[0].rtcpFeedbacks.push_back({"111", "transport-cc", {}});
    offer.mediaDescriptions[0].extMaps.push_back(
      {1, {}, "urn:ietf:params:rtp-hdrext:ssrc-audio-level", {}});

    AnswerOptions answerOpts;
    answerOpts.iceUfrag = "au";
    answerOpts.icePwd = "ap1234567890123456789012";

    auto answer = OfferAnswer::createAnswer(offer, answerOpts);

    REQUIRE(answer.mediaDescriptions[0].fmtps.size() == 1);
    REQUIRE(answer.mediaDescriptions[0].fmtps[0].parameters == "minptime=10;useinbandfec=1");
    REQUIRE(answer.mediaDescriptions[0].rtcpFeedbacks.size() == 1);
    REQUIRE(answer.mediaDescriptions[0].extMaps.size() == 1);
  }

  SECTION("fmtps and rtcpFeedbacks NOT copied when supportedCodecs is provided")
  {
    auto offer = makeOffer();
    offer.mediaDescriptions[0].fmtps.push_back({111, "minptime=10", {}});
    offer.mediaDescriptions[0].rtcpFeedbacks.push_back({"111", "transport-cc", {}});

    AnswerOptions answerOpts;
    answerOpts.iceUfrag = "au";
    answerOpts.icePwd = "ap1234567890123456789012";
    answerOpts.supportedCodecs = std::vector<RtpMapAttribute>{
      {120, "opus", 48000, std::uint8_t(2)}
    };

    auto answer = OfferAnswer::createAnswer(offer, answerOpts);

    // When supportedCodecs provided, only rtpMaps and formats are populated
    REQUIRE(answer.mediaDescriptions[0].rtpMaps.size() == 1);
    REQUIRE(answer.mediaDescriptions[0].fmtps.empty());
    REQUIRE(answer.mediaDescriptions[0].rtcpFeedbacks.empty());
    REQUIRE(answer.mediaDescriptions[0].extMaps.empty());
  }
}

// ---------------------------------------------------------------------------
// T9: Round-trip
// ---------------------------------------------------------------------------

TEST_CASE("Round-trip: createOffer -> serialize -> parse -> createAnswer -> serialize -> parse",
           "[offer_answer][roundtrip]")
{
  OfferOptions offerOpts;
  offerOpts.iceUfrag = "rtufrag1";
  offerOpts.icePwd = "rtpwd12345678901234567890";
  offerOpts.fingerprint = FingerprintAttribute{"sha-256",
    "AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99"};
  offerOpts.dataChannel = true;

  auto offer = OfferAnswer::createOffer(offerOpts);

  // Add codecs
  offer.mediaDescriptions[0].rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});
  offer.mediaDescriptions[0].formats.push_back("111");
  offer.mediaDescriptions[1].rtpMaps.push_back({96, "VP8", 90000, {}});
  offer.mediaDescriptions[1].formats.push_back("96");

  // Serialize and parse the offer
  auto offerSdp = SdpSerializer::serialize(offer);
  auto offerResult = SdpParser::parse(offerSdp);
  REQUIRE_FALSE(offerResult.error.has_value());
  REQUIRE(offerResult.value.has_value());

  // Create answer from parsed offer
  AnswerOptions answerOpts;
  answerOpts.iceUfrag = "rtanswer1";
  answerOpts.icePwd = "rtanswerpwd1234567890123";
  answerOpts.fingerprint = FingerprintAttribute{"sha-256",
    "11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00"};
  answerOpts.dtlsSetup = SetupRole::ACTIVE;

  auto answer = OfferAnswer::createAnswer(offerResult.value.value(), answerOpts);

  // Serialize and parse the answer
  auto answerSdp = SdpSerializer::serialize(answer);
  auto answerResult = SdpParser::parse(answerSdp);
  REQUIRE_FALSE(answerResult.error.has_value());
  REQUIRE(answerResult.value.has_value());

  // Verify answer structure
  auto& parsedAnswer = answerResult.value.value();
  REQUIRE(parsedAnswer.version == 0);
  REQUIRE(parsedAnswer.mediaDescriptions.size() == 3);
  REQUIRE(parsedAnswer.mediaDescriptions[0].mediaType == MediaType::AUDIO);
  REQUIRE(parsedAnswer.mediaDescriptions[1].mediaType == MediaType::VIDEO);
  REQUIRE(parsedAnswer.mediaDescriptions[2].mediaType == MediaType::APPLICATION);
}

// ---------------------------------------------------------------------------
// Edge cases from review
// ---------------------------------------------------------------------------

TEST_CASE("matchCodecs with duplicate codec entries in offer", "[offer_answer][codecs][edge]")
{
  MediaDescription offer;
  offer.rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});
  offer.rtpMaps.push_back({112, "opus", 48000, std::uint8_t(2)});

  MediaDescription answer;
  answer.rtpMaps.push_back({96, "opus", 48000, std::uint8_t(2)});

  auto result = OfferAnswer::matchCodecs(offer, answer);
  // Answer has one opus entry, so result should have exactly one match
  REQUIRE(result.size() == 1);
  REQUIRE(result[0].payloadType == 96);
}

TEST_CASE("pruneUnusedCodecs with retain list containing PTs not present in media",
           "[offer_answer][codecs][edge]")
{
  MediaDescription media;
  media.rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});
  media.formats = {"111"};

  // Retain list includes 111 (present) and 96 (not present) — should work fine
  OfferAnswer::pruneUnusedCodecs(media, {111, 96});

  REQUIRE(media.rtpMaps.size() == 1);
  REQUIRE(media.rtpMaps[0].payloadType == 111);
  REQUIRE(media.formats.size() == 1);
}

TEST_CASE("pruneUnusedCodecs with out-of-range format values", "[offer_answer][codecs][edge]")
{
  MediaDescription media;
  media.rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});
  // "300" is > 127, should be treated like non-numeric (preserved)
  media.formats = {"111", "300"};

  OfferAnswer::pruneUnusedCodecs(media, {111});

  REQUIRE(media.rtpMaps.size() == 1);
  REQUIRE(media.formats.size() == 2);
  REQUIRE(media.formats[0] == "111");
  REQUIRE(media.formats[1] == "300");
}

TEST_CASE("createOffer with partial ICE credentials (only ufrag, no pwd)",
           "[offer_answer][offer][edge]")
{
  OfferOptions opts;
  opts.iceUfrag = "myufrag";
  // icePwd not set — both should be auto-generated (provided ufrag is discarded)
  auto offer = OfferAnswer::createOffer(opts);
  REQUIRE(offer.iceUfrag.has_value());
  REQUIRE(offer.icePwd.has_value());
  // The provided ufrag is discarded because icePwd was not provided
  REQUIRE(offer.iceUfrag.value() != "myufrag");
}

TEST_CASE("createAnswer with partial ICE credentials (only pwd, no ufrag)",
           "[offer_answer][answer][edge]")
{
  auto offer = OfferAnswer::createOffer();
  AnswerOptions answerOpts;
  answerOpts.icePwd = "mypwd123456789012345678";
  // iceUfrag not set — both should be auto-generated
  auto answer = OfferAnswer::createAnswer(offer, answerOpts);
  REQUIRE(answer.iceUfrag.has_value());
  REQUIRE(answer.icePwd.has_value());
  REQUIRE(answer.icePwd.value() != "mypwd123456789012345678");
}

TEST_CASE("createAnswer with offer containing disabled (port=0) media section",
           "[offer_answer][answer][edge]")
{
  OfferOptions offerOpts;
  offerOpts.iceUfrag = "ou";
  offerOpts.icePwd = "op1234567890123456789012";
  auto offer = OfferAnswer::createOffer(offerOpts);

  // Add codecs
  offer.mediaDescriptions[0].rtpMaps.push_back({111, "opus", 48000, std::uint8_t(2)});
  offer.mediaDescriptions[0].formats = {"111"};
  offer.mediaDescriptions[1].rtpMaps.push_back({96, "VP8", 90000, {}});
  offer.mediaDescriptions[1].formats = {"96"};

  // Disable video section in the offer
  offer.mediaDescriptions[1].port = 0;
  offer.mediaDescriptions[1].direction = Direction::INACTIVE;

  AnswerOptions answerOpts;
  answerOpts.iceUfrag = "au";
  answerOpts.icePwd = "ap1234567890123456789012";

  auto answer = OfferAnswer::createAnswer(offer, answerOpts);

  REQUIRE(answer.mediaDescriptions.size() == 2);
  // Audio should be accepted normally
  REQUIRE(answer.mediaDescriptions[0].port == 9);
  // Disabled offer video: direction flipped from INACTIVE->INACTIVE, port set to 9
  // (the implementation does not special-case already-disabled sections)
  REQUIRE(answer.mediaDescriptions[1].direction == Direction::INACTIVE);
}

TEST_CASE("createAnswer rejects APPLICATION when in rejectMediaTypes",
           "[offer_answer][answer][edge]")
{
  OfferOptions offerOpts;
  offerOpts.audio = false;
  offerOpts.video = false;
  offerOpts.dataChannel = true;
  offerOpts.iceUfrag = "ou";
  offerOpts.icePwd = "op1234567890123456789012";
  auto offer = OfferAnswer::createOffer(offerOpts);

  AnswerOptions answerOpts;
  answerOpts.iceUfrag = "au";
  answerOpts.icePwd = "ap1234567890123456789012";
  answerOpts.rejectMediaTypes = {MediaType::APPLICATION};

  auto answer = OfferAnswer::createAnswer(offer, answerOpts);

  REQUIRE(answer.mediaDescriptions.size() == 1);
  REQUIRE(answer.mediaDescriptions[0].port == 0);
  REQUIRE(answer.mediaDescriptions[0].direction == Direction::INACTIVE);
  REQUIRE(answer.mediaDescriptions[0].mediaType == MediaType::APPLICATION);
}

TEST_CASE("createAnswer default setup is ACTIVE", "[offer_answer][answer][edge]")
{
  auto offer = OfferAnswer::createOffer();
  auto answer = OfferAnswer::createAnswer(offer);
  REQUIRE(answer.setup.has_value());
  REQUIRE(answer.setup.value() == SetupRole::ACTIVE);
}

TEST_CASE("createOffer default setup is ACTPASS", "[offer_answer][offer][edge]")
{
  auto offer = OfferAnswer::createOffer();
  REQUIRE(offer.setup.has_value());
  REQUIRE(offer.setup.value() == SetupRole::ACTPASS);
}

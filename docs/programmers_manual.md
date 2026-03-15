# iora_sdp Programmer's Manual

Comprehensive reference for the **iora_sdp** library — a standalone, header-only C++17 SDP
parser/serializer with WebRTC offer/answer utilities.

**Version:** 0.1.0
**License:** MPL-2.0

---

## 1. Introduction

### Overview

iora_sdp is a standalone, header-only C++17 library for parsing, serializing, and manipulating
Session Description Protocol (SDP) messages. It has zero external dependencies — only the C++17
standard library is required. The library targets both plain RTP (SIP) and WebRTC use cases.

Key value propositions:

- **Header-only** — zero link cost, include and use
- **Typed attribute model** — compile-time safety for all known SDP attributes
- **Unknown attribute preservation** — proprietary/future extensions survive parse-serialize round trips via `GenericAttribute`
- **`string_view`-based parser** — zero-copy where possible
- **Lenient parsing by default** — tolerates real-world SDP from browsers and SIP stacks
- **WebRTC-first but not WebRTC-only** — full support for SIP, SRTP, data channels, simulcast
- **No negotiation state machine** — stateless utility functions that applications compose

**Out of scope:** SIP signaling, WebRTC PeerConnection state machine, ICE agent, DTLS handshake,
RTP session creation, codec instantiation, SRTP encryption, SCTP association management, SDP
mangling for SBC B2BUA.

### Architecture

iora_sdp is organized into five layers, where each layer depends only on the layers below it:

```
  ┌─────────────────────────────────────────────────┐
  │  Level 4: Offer/Answer Utilities                │
  │           offer_answer.hpp                      │
  ├─────────────────────────────────────────────────┤
  │  Level 3: Serializer                            │
  │           sdp_serializer.hpp                    │
  ├─────────────────────────────────────────────────┤
  │  Level 2: Parser                                │
  │           sdp_parser.hpp                        │
  ├─────────────────────────────────────────────────┤
  │  Level 1: Data Model                            │
  │           session_description.hpp               │
  │           attributes.hpp                        │
  ├─────────────────────────────────────────────────┤
  │  Level 0: Types and Constants                   │
  │           types.hpp                             │
  └─────────────────────────────────────────────────┘
```

All layers are header-only and reside under `include/iora/sdp/`.

**Companion libraries:**

- **iora_codecs** — maps `rtpmap`/`fmtp` to `CodecInfo` for encoding/decoding
- **iora_rtp** — maps SDP media sections to RTP sessions
- **iora_webrtc** (future) — PeerConnection, ICE agent, DTLS

### Build

**Quick build:**

```bash
cmake -S . -B build
cmake --build build -j$(nproc)
```

**Run tests:**

```bash
cd build && ctest --output-on-failure
# or directly:
./build/tests/sdp_tests
```

**Sanitizer builds:**

```bash
# AddressSanitizer
cmake -S . -B build-asan -DCMAKE_CXX_FLAGS='-fsanitize=address -fno-omit-frame-pointer'
cmake --build build-asan -j$(nproc)

# UndefinedBehaviorSanitizer
cmake -S . -B build-ubsan -DCMAKE_CXX_FLAGS='-fsanitize=undefined'
cmake --build build-ubsan -j$(nproc)

# ThreadSanitizer
cmake -S . -B build-tsan -DCMAKE_CXX_FLAGS='-fsanitize=thread'
cmake --build build-tsan -j$(nproc)
```

**Fuzz build:**

```bash
cmake -S . -B build-fuzz \
  -DIORA_SDP_BUILD_FUZZ=ON \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_CXX_FLAGS='-fsanitize=fuzzer,address'
cmake --build build-fuzz
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `IORA_SDP_BUILD_TESTS` | `ON` | Build test executables |
| `IORA_SDP_BUILD_FUZZ` | `OFF` | Build fuzz testing targets (requires clang with libFuzzer support) |

**CMake integration for consuming projects:**

```cmake
# Method 1: add_subdirectory
add_subdirectory(third_party/iora_sdp)
target_link_libraries(myapp PRIVATE iora_sdp)

# Method 2: FetchContent
include(FetchContent)
FetchContent_Declare(iora_sdp
  GIT_REPOSITORY https://github.com/example/iora_sdp.git
  GIT_TAG main
)
FetchContent_MakeAvailable(iora_sdp)
target_link_libraries(myapp PRIVATE iora_sdp)
```

**Note:** iora_sdp is an `INTERFACE` library — it adds only an include path, no link dependencies.

### Directory Structure

```
iora_sdp/
  include/iora/sdp/
    types.hpp                    — Layer 0: enums, constants, conversion functions
    attributes.hpp               — Layer 1: typed attribute structs
    session_description.hpp      — Layer 1: data model (SessionDescription, MediaDescription)
    sdp_parser.hpp               — Layer 2: SDP text parser
    sdp_serializer.hpp           — Layer 3: SDP text serializer
    offer_answer.hpp             — Layer 4: offer/answer utilities
  tests/
    test_types.cpp               — Layer 0 unit tests
    test_attributes.cpp          — Layer 1 attribute tests
    test_session_description.cpp — Layer 1 data model tests
    test_parser.cpp              — Layer 2 parser tests
    test_serializer.cpp          — Layer 3 serializer tests
    test_offer_answer.cpp        — Layer 4 offer/answer tests
    test_round_trip.cpp          — Round-trip verification tests
    test_webrtc_sdp.cpp          — Real-world WebRTC SDP tests
    fuzz_parser.cpp              — Fuzz testing harness
    fuzz_corpus/                 — Seed corpus for fuzzing
    fuzz_parser.dict             — Fuzzer dictionary
  docs/
    architecture.json            — Architecture specification
    programmers_manual.md        — This document
  CMakeLists.txt
```

---

## 2. Core Types (Layer 0)

**Header:** `include/iora/sdp/types.hpp`

All types are in namespace `iora::sdp`.

### MediaType

```cpp
enum class MediaType
{
  AUDIO,        // audio media
  VIDEO,        // video media
  APPLICATION,  // SCTP data channels (webrtc-datachannel) and other non-audio/video
  TEXT,         // text media
  MESSAGE       // message media
};
```

**Free functions:**

```cpp
std::string_view toString(MediaType v) noexcept;
std::optional<MediaType> mediaTypeFromString(std::string_view s);  // case-insensitive
```

**Example:**

```cpp
using namespace iora::sdp;

auto str = toString(MediaType::AUDIO);           // "audio"
auto mt = mediaTypeFromString("video");          // MediaType::VIDEO
auto bad = mediaTypeFromString("unknown");       // std::nullopt
```

### TransportProtocol

```cpp
enum class TransportProtocol
{
  UDP,               // plain UDP
  RTP_AVP,           // RTP Audio Video Profile (RFC 3551)
  RTP_SAVP,          // Secure RTP AVP
  RTP_SAVPF,         // Secure RTP AVP with Feedback (RFC 4585)
  UDP_TLS_RTP_SAVPF, // WebRTC standard (DTLS-SRTP with feedback)
  DTLS_SCTP,         // legacy data channel transport
  UDP_DTLS_SCTP,     // current standard data channel transport (RFC 8841)
  TCP_DTLS_SCTP      // TCP-based data channel transport
};
```

**Important:** Enum values use underscores; SDP wire format uses slashes:

| Enum Value | SDP Wire Format |
|------------|-----------------|
| `RTP_AVP` | `RTP/AVP` |
| `RTP_SAVP` | `RTP/SAVP` |
| `RTP_SAVPF` | `RTP/SAVPF` |
| `UDP_TLS_RTP_SAVPF` | `UDP/TLS/RTP/SAVPF` |
| `DTLS_SCTP` | `DTLS/SCTP` |
| `UDP_DTLS_SCTP` | `UDP/DTLS/SCTP` |
| `TCP_DTLS_SCTP` | `TCP/DTLS/SCTP` |

**Free functions:**

```cpp
std::string_view toString(TransportProtocol v) noexcept;
std::optional<TransportProtocol> transportProtocolFromString(std::string_view s);
```

**Example:**

```cpp
using namespace iora::sdp;

// WebRTC uses UDP/TLS/RTP/SAVPF
auto webrtcProto = TransportProtocol::UDP_TLS_RTP_SAVPF;
auto str = toString(webrtcProto);  // "UDP/TLS/RTP/SAVPF"

// SIP uses RTP/AVP
auto sipProto = transportProtocolFromString("RTP/AVP");  // TransportProtocol::RTP_AVP
```

### NetworkType

```cpp
enum class NetworkType
{
  IN  // Internet
};
```

**Note:** Only `IN` is defined by RFC 4566. Stored as an enum for type safety.

**Free functions:**

```cpp
std::string_view toString(NetworkType v) noexcept;
std::optional<NetworkType> networkTypeFromString(std::string_view s);
```

### AddressType

```cpp
enum class AddressType
{
  IP4,  // IPv4 address
  IP6   // IPv6 address
};
```

**Free functions:**

```cpp
std::string_view toString(AddressType v) noexcept;
std::optional<AddressType> addressTypeFromString(std::string_view s);
```

### Direction

```cpp
enum class Direction
{
  SENDRECV,  // bidirectional (default per RFC 3264 section 6.1)
  SENDONLY,  // send only
  RECVONLY,  // receive only
  INACTIVE   // no media flow
};
```

**Important:** The default direction is `SENDRECV` if no direction attribute is present in the SDP.

**Free functions:**

```cpp
std::string_view toString(Direction v) noexcept;
std::optional<Direction> directionFromString(std::string_view s);
```

See also: `OfferAnswer::flipDirection()` in Section 7.

### SetupRole

```cpp
enum class SetupRole
{
  ACTIVE,   // initiate DTLS connection
  PASSIVE,  // wait for DTLS connection
  ACTPASS,  // can be either (used in offers)
  HOLDCONN  // hold connection (rarely used)
};
```

**Note:** WebRTC offers typically use `ACTPASS`; answers use `ACTIVE` or `PASSIVE` (RFC 4145).

**Free functions:**

```cpp
std::string_view toString(SetupRole v) noexcept;
std::optional<SetupRole> setupRoleFromString(std::string_view s);
```

### BandwidthType

```cpp
enum class BandwidthType
{
  CT,    // Conference Total
  AS,    // Application Specific
  TIAS,  // Transport Independent Application Specific (RFC 3890)
  RS,    // RTCP bandwidth sender (RFC 3556)
  RR     // RTCP bandwidth receiver (RFC 3556)
};
```

**Note:** `CT` and `AS` values are in kbps; `TIAS` is in bps.

**Free functions:**

```cpp
std::string_view toString(BandwidthType v) noexcept;
std::optional<BandwidthType> bandwidthTypeFromString(std::string_view s);
```

### IceCandidateType

```cpp
enum class IceCandidateType
{
  HOST,   // local address
  SRFLX,  // server reflexive (discovered via STUN)
  PRFLX,  // peer reflexive (discovered during connectivity checks)
  RELAY   // TURN relay address
};
```

**Free functions (RFC 8445):**

```cpp
std::string_view toString(IceCandidateType v) noexcept;
std::optional<IceCandidateType> iceCandidateTypeFromString(std::string_view s);
```

### IceTransportType

```cpp
enum class IceTransportType
{
  UDP,  // standard ICE transport
  TCP   // ICE-TCP per RFC 6544
};
```

**Free functions:**

```cpp
std::string_view toString(IceTransportType v) noexcept;
std::optional<IceTransportType> iceTransportTypeFromString(std::string_view s);
```

### RidDirection

```cpp
enum class RidDirection
{
  SEND,  // restriction applies to send direction
  RECV   // restriction applies to receive direction
};
```

Used in `RidAttribute` for simulcast restriction identifiers (RFC 8851).

**Free functions:**

```cpp
std::string_view toString(RidDirection v) noexcept;
std::optional<RidDirection> ridDirectionFromString(std::string_view s);
```

### AttributeConstants

Compile-time string constants for all known SDP attribute names. All values are
`static constexpr std::string_view` members.

```cpp
struct AttributeConstants
{
  static constexpr std::string_view kRtpmap            = "rtpmap";
  static constexpr std::string_view kFmtp              = "fmtp";
  static constexpr std::string_view kRtcp              = "rtcp";
  static constexpr std::string_view kRtcpMux           = "rtcp-mux";
  static constexpr std::string_view kRtcpMuxOnly       = "rtcp-mux-only";
  static constexpr std::string_view kRtcpRsize         = "rtcp-rsize";
  static constexpr std::string_view kRtcpFb            = "rtcp-fb";
  static constexpr std::string_view kExtmap            = "extmap";
  static constexpr std::string_view kExtmapAllowMixed  = "extmap-allow-mixed";
  static constexpr std::string_view kSsrc              = "ssrc";
  static constexpr std::string_view kSsrcGroup         = "ssrc-group";
  static constexpr std::string_view kMid               = "mid";
  static constexpr std::string_view kMsid              = "msid";
  static constexpr std::string_view kGroup             = "group";
  static constexpr std::string_view kIceUfrag          = "ice-ufrag";
  static constexpr std::string_view kIcePwd            = "ice-pwd";
  static constexpr std::string_view kIceOptions        = "ice-options";
  static constexpr std::string_view kIceLite           = "ice-lite";
  static constexpr std::string_view kCandidate         = "candidate";
  static constexpr std::string_view kEndOfCandidates   = "end-of-candidates";
  static constexpr std::string_view kFingerprint       = "fingerprint";
  static constexpr std::string_view kSetup             = "setup";
  static constexpr std::string_view kCrypto            = "crypto";
  static constexpr std::string_view kSendrecv          = "sendrecv";
  static constexpr std::string_view kSendonly          = "sendonly";
  static constexpr std::string_view kRecvonly          = "recvonly";
  static constexpr std::string_view kInactive          = "inactive";
  static constexpr std::string_view kSctpmap           = "sctpmap";
  static constexpr std::string_view kSctpPort          = "sctp-port";
  static constexpr std::string_view kMaxMessageSize    = "max-message-size";
  static constexpr std::string_view kSimulcast         = "simulcast";
  static constexpr std::string_view kRid               = "rid";
  static constexpr std::string_view kFramerate         = "framerate";
  static constexpr std::string_view kPtime             = "ptime";
  static constexpr std::string_view kMaxptime          = "maxptime";
};
```

The parser uses these for attribute dispatch; the serializer uses them for canonical output.

### detail:: Helper Functions

The `detail::` namespace contains internal helpers not intended for direct use:

```cpp
namespace detail {
  std::string toLower(std::string_view s);                    // lowercase conversion
  bool iequals(std::string_view a, std::string_view b);      // case-insensitive equality
}
```

---

## 3. Typed Attribute Structs (Layer 1)

**Header:** `include/iora/sdp/attributes.hpp`

Each struct represents a parsed SDP attribute with typed fields. All structs provide `operator==` and `operator!=`.

### RtpMapAttribute

Maps to `a=rtpmap:<payloadType> <encodingName>/<clockRate>[/<channels>]` (RFC 4566 section 6, RFC 3551).

```cpp
struct RtpMapAttribute
{
  std::uint8_t payloadType = 0;               // RTP payload type (0-127)
  std::string encodingName;                   // codec name (e.g., "opus", "VP8", "H264")
  std::uint32_t clockRate = 0;                // RTP clock rate in Hz
  std::optional<std::uint8_t> channels;       // channel count (present for audio, omitted for video)
};
```

**SDP example:** `a=rtpmap:111 opus/48000/2`

**Code example:**

```cpp
using namespace iora::sdp;

RtpMapAttribute opus;
opus.payloadType = 111;
opus.encodingName = "opus";
opus.clockRate = 48000;
opus.channels = 2;

RtpMapAttribute vp8;
vp8.payloadType = 96;
vp8.encodingName = "VP8";
vp8.clockRate = 90000;
// channels omitted for video
```

### FmtpAttribute

Maps to `a=fmtp:<payloadType> <parameters>` (RFC 4566 section 6).

```cpp
struct FmtpAttribute
{
  std::uint8_t payloadType = 0;                                  // payload type this fmtp applies to
  std::string parameters;                                        // raw parameter string (preserved exactly)
  std::unordered_map<std::string, std::string> parameterMap;     // parsed key=value pairs
};
```

**SDP examples:**

```
a=fmtp:111 minptime=10;useinbandfec=1
a=fmtp:98 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f
```

**Note:** `parameterMap` is populated by the parser for semicolon-separated `key=value` formats.
For non-key=value formats (e.g., telephone-event `0-16`), use the raw `parameters` string.

**Code example:**

```cpp
using namespace iora::sdp;

// After parsing, read a specific fmtp parameter:
auto result = SdpParser::parse(sdpText);
auto& audio = result.value->mediaDescriptions[0];
for (const auto& fmtp : audio.fmtps)
{
  if (fmtp.payloadType == 111)
  {
    auto it = fmtp.parameterMap.find("useinbandfec");
    if (it != fmtp.parameterMap.end())
    {
      // it->second == "1"
    }
  }
}
```

### RtcpFbAttribute

Maps to `a=rtcp-fb:<payloadType> <type> [<subtype>]` (RFC 4585 section 4.2).

```cpp
struct RtcpFbAttribute
{
  std::string payloadType;                     // PT number OR "*" for wildcard
  std::string type;                            // feedback type (nack, ccm, transport-cc, goog-remb)
  std::optional<std::string> subtype;          // feedback subtype (pli, fir, sli)
};
```

**Note:** `payloadType` is `std::string` (not `uint8_t`) because it can be `"*"` for wildcard.

**Note:** `goog-remb` and `transport-cc` are non-standard types from Google Chrome, widely used in WebRTC.

**SDP examples:**

```
a=rtcp-fb:96 nack
a=rtcp-fb:96 nack pli
a=rtcp-fb:96 ccm fir
a=rtcp-fb:96 transport-cc
a=rtcp-fb:96 goog-remb
a=rtcp-fb:* transport-cc
```

### ExtMapAttribute

Maps to `a=extmap:<id>[/<direction>] <uri> [<extensionAttributes>]` (RFC 5285, RFC 8285).

```cpp
struct ExtMapAttribute
{
  std::uint16_t id = 0;                            // local identifier (1-14 one-byte, 1-255 two-byte)
  std::optional<Direction> direction;              // optional direction restriction
  std::string uri;                                 // extension URI
  std::optional<std::string> extensionAttributes;  // optional extension-specific attributes
};
```

**SDP examples:**

```
a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level
a=extmap:2/sendonly http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time
```

**Common WebRTC extensions:**

| Extension | URI |
|-----------|-----|
| Audio level | `urn:ietf:params:rtp-hdrext:ssrc-audio-level` |
| Abs send time | `http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time` |
| Transport-wide CC | `http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01` |
| SDES mid | `urn:ietf:params:rtp-hdrext:sdes:mid` |
| SDES stream ID | `urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id` |
| SDES repaired ID | `urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id` |
| Timestamp offset | `urn:ietf:params:rtp-hdrext:toffset` |
| Video orientation | `urn:3gpp:video-orientation` |

### SsrcAttribute

Maps to `a=ssrc:<ssrc> <attributeName>[:<attributeValue>]` (RFC 5576 section 4.1).

```cpp
struct SsrcAttribute
{
  std::uint32_t ssrc = 0;                     // synchronization source identifier
  std::string attributeName;                  // attribute name (cname, msid, label, mslabel)
  std::optional<std::string> attributeValue;  // attribute value
};
```

**SDP examples:**

```
a=ssrc:3456789012 cname:user@example.com
a=ssrc:3456789012 msid:stream0 track0
```

### SsrcGroupAttribute

Maps to `a=ssrc-group:<semantics> <ssrc> [<ssrc>...]` (RFC 5576 section 4.2).

```cpp
struct SsrcGroupAttribute
{
  std::string semantics;                     // grouping semantics (FID, SIM)
  std::vector<std::uint32_t> ssrcs;          // list of SSRCs in the group
};
```

**SDP example:** `a=ssrc-group:FID 3456789012 3456789013`

**Note:** FID = flow identification (RTX retransmission), SIM = simulcast (legacy Chrome).

### IceCandidate

Maps to `a=candidate:<foundation> <component> <transport> <priority> <address> <port> typ <type> [raddr <relAddr> rport <relPort>] [tcptype <tcpType>] [<ext> <extValue>]*` (RFC 8445 section 5.1).

```cpp
struct IceCandidate
{
  std::string foundation;                                        // candidate foundation
  std::uint8_t component = 0;                                   // component ID (1=RTP, 2=RTCP)
  IceTransportType transport = IceTransportType::UDP;            // transport protocol
  std::uint32_t priority = 0;                                   // candidate priority (higher=preferred)
  std::string address;                                           // candidate IP address
  std::uint16_t port = 0;                                       // candidate port
  IceCandidateType type = IceCandidateType::HOST;                // candidate type
  std::optional<std::string> relAddr;                            // related address (srflx/prflx/relay)
  std::optional<std::uint16_t> relPort;                          // related port
  std::optional<std::string> tcpType;                            // TCP type per RFC 6544
  std::vector<std::pair<std::string, std::string>> extensions;   // extension attributes
};
```

**SDP examples:**

```
a=candidate:1 1 udp 2113937151 192.168.1.100 5000 typ host
a=candidate:2 1 udp 1845501695 203.0.113.1 6000 typ srflx raddr 192.168.1.100 rport 5000
a=candidate:3 1 tcp 1015021823 192.168.1.100 9 typ host tcptype active
```

**Code example:**

```cpp
using namespace iora::sdp;

IceCandidate host;
host.foundation = "1";
host.component = 1;
host.transport = IceTransportType::UDP;
host.priority = 2113937151;
host.address = "192.168.1.100";
host.port = 5000;
host.type = IceCandidateType::HOST;

IceCandidate srflx;
srflx.foundation = "2";
srflx.component = 1;
srflx.transport = IceTransportType::UDP;
srflx.priority = 1845501695;
srflx.address = "203.0.113.1";
srflx.port = 6000;
srflx.type = IceCandidateType::SRFLX;
srflx.relAddr = "192.168.1.100";
srflx.relPort = 5000;
```

### FingerprintAttribute

Maps to `a=fingerprint:<hashFunction> <fingerprint>` (RFC 4572 section 5).

```cpp
struct FingerprintAttribute
{
  std::string hashFunction;   // hash algorithm (sha-256, sha-384, sha-512)
  std::string fingerprint;    // hex-encoded fingerprint with colons (e.g., "AB:CD:EF:...")
};
```

**SDP example:** `a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99`

**Note:** Multiple fingerprint attributes are allowed for different hash algorithms.

### CryptoAttribute

Maps to `a=crypto:<tag> <suite> <keyParams> [<sessionParams>]` (RFC 4568, RFC 3711).

```cpp
struct CryptoAttribute
{
  std::uint32_t tag = 0;                       // crypto attribute tag
  std::string suite;                           // crypto suite (AES_CM_128_HMAC_SHA1_80, etc.)
  std::string keyParams;                       // key parameters (inline:base64key format)
  std::optional<std::string> sessionParams;    // optional session parameters
};
```

**SDP example:** `a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:base64keyvalue`

**Note:** SDES-SRTP is legacy — WebRTC uses DTLS-SRTP. This attribute is supported for SIP interop.

### RtcpAttribute

Maps to `a=rtcp:<port> [<netType> <addrType> <address>]` (RFC 3605).

```cpp
struct RtcpAttribute
{
  std::uint16_t port = 0;                       // RTCP port number
  std::optional<NetworkType> netType;           // network type (optional)
  std::optional<AddressType> addrType;          // address type (optional)
  std::optional<std::string> address;           // RTCP address (optional)
};
```

**SDP example:** `a=rtcp:5001 IN IP4 192.168.1.1`

**Note:** Used when RTP and RTCP are on separate ports (pre-rtcp-mux).

### MsidAttribute

Maps to `a=msid:<streamId> [<trackId>]` (RFC 8830).

```cpp
struct MsidAttribute
{
  std::string streamId;                         // MediaStream identifier
  std::optional<std::string> trackId;           // MediaStreamTrack identifier
};
```

**SDP example:** `a=msid:stream0 track0`

**Note:** Unified Plan allows multiple `a=msid` lines per media section.

### GroupAttribute

Maps to `a=group:<semantics> <mid> [<mid>...]` (RFC 5888, RFC 8843).

```cpp
struct GroupAttribute
{
  std::string semantics;                    // grouping semantics (BUNDLE, LS, FID)
  std::vector<std::string> mids;            // list of mid values in the group
};
```

**SDP example:** `a=group:BUNDLE 0 1 2`

**Note:** BUNDLE = media multiplexing (RFC 8843), LS = lip sync, FID = flow identification.

### SimulcastStream and SimulcastAttribute

Maps to `a=simulcast:send <streams> [recv <streams>]` (RFC 8853).

```cpp
struct SimulcastStream
{
  std::string rid;          // restriction identifier referencing an a=rid line
  bool paused = false;      // whether stream starts paused ("~" prefix in SDP)
};

struct SimulcastAttribute
{
  std::vector<std::vector<SimulcastStream>> sendStreams;  // send direction streams
  std::vector<std::vector<SimulcastStream>> recvStreams;  // recv direction streams
};
```

**Note:** Outer vector is `;`-separated groups, inner vector is `,`-separated alternatives.

**SDP example:** `a=simulcast:send h;m;l recv h;m;l`

**Code example:**

```cpp
using namespace iora::sdp;

SimulcastAttribute sim;
sim.sendStreams = {
  {{"h", false}},   // high quality
  {{"m", false}},   // medium quality
  {{"l", false}}    // low quality
};
```

### RidAttribute

Maps to `a=rid:<id> <direction> [pt=<pt,...>][;<key>=<value>;...]` (RFC 8851).

```cpp
struct RidAttribute
{
  std::string id;                                                  // restriction identifier
  RidDirection direction = RidDirection::SEND;                     // send or recv
  std::vector<std::uint8_t> payloadTypes;                          // optional payload type filter
  std::unordered_map<std::string, std::string> restrictions;       // key=value restriction parameters
};
```

**SDP example:** `a=rid:h send pt=96;max-width=1920;max-height=1080;max-fps=30`

**Common restriction keys:** `max-width`, `max-height`, `max-fps`, `max-br`, `max-bpp`.

### GenericAttribute

Catch-all struct for unrecognized `a=` lines:

```cpp
struct GenericAttribute
{
  std::string name;                             // attribute name
  std::optional<std::string> value;             // attribute value (absent for flag attributes)
};
```

**Purpose:** Preserves proprietary or future extension attributes through parse-serialize round trips.

**SDP examples:**

```
a=x-google-flag:conference    (name="x-google-flag", value="conference")
```

**Note:** Only truly unrecognized attributes land here. All known attributes are dispatched to typed structs.

---

## 4. Data Model (Layer 1)

**Header:** `include/iora/sdp/session_description.hpp`

### Origin

Maps to `o=<username> <sessId> <sessVersion> <netType> <addrType> <address>`.

```cpp
struct Origin
{
  std::string username = "-";                      // user's login or "-"
  std::string sessId = "0";                        // numeric session identifier (string)
  std::string sessVersion = "0";                   // numeric session version (string)
  NetworkType netType = NetworkType::IN;           // network type
  AddressType addrType = AddressType::IP4;         // address type
  std::string address = "0.0.0.0";                 // originating host address
};
```

**Note:** `sessId` and `sessVersion` are stored as strings because they can be very large `uint64` values that may overflow integer types.

**Code example:**

```cpp
using namespace iora::sdp;

Origin o;
o.username = "-";
o.sessId = "1234567890";
o.sessVersion = "2";
o.netType = NetworkType::IN;
o.addrType = AddressType::IP4;
o.address = "192.168.1.100";
// Serializes to: o=- 1234567890 2 IN IP4 192.168.1.100
```

### ConnectionData

Maps to `c=<netType> <addrType> <address>[/<ttl>][/<numAddresses>]`.

```cpp
struct ConnectionData
{
  NetworkType netType = NetworkType::IN;                  // network type
  AddressType addrType = AddressType::IP4;                // address type
  std::string address;                                    // connection address
  std::optional<std::uint8_t> ttl;                        // multicast TTL (IPv4 only)
  std::optional<std::uint16_t> numberOfAddresses;         // number of multicast addresses
};
```

**Code example:**

```cpp
using namespace iora::sdp;

// Unicast
ConnectionData unicast;
unicast.address = "192.168.1.100";

// Multicast with TTL
ConnectionData multicast;
multicast.address = "224.2.36.42";
multicast.ttl = 127;
```

### BandwidthInfo

Maps to `b=<type>:<bandwidth>`.

```cpp
struct BandwidthInfo
{
  BandwidthType type = BandwidthType::AS;     // bandwidth type
  std::uint32_t bandwidth = 0;                // value in kbps (CT/AS) or bps (TIAS)
};
```

### TimeDescription, RepeatTime, and ZoneAdjustment

```cpp
struct RepeatTime
{
  std::string repeatInterval;                  // e.g., "7d"
  std::string activeDuration;                  // e.g., "1h"
  std::vector<std::string> offsets;            // offsets from start time
};

struct ZoneAdjustment
{
  std::string adjustmentTime;                  // NTP time of adjustment
  std::string offset;                          // offset amount
};

struct TimeDescription
{
  std::uint64_t startTime = 0;                 // NTP start time (0 = permanent)
  std::uint64_t stopTime = 0;                  // NTP stop time (0 = unbounded)
  std::vector<RepeatTime> repeatTimes;         // associated r= lines
};
```

Maps to SDP lines: `t=`, `r=`, `z=`.

**Note:** WebRTC always uses `t=0 0` (permanent session). SIP may use specific times for scheduled conferences.

### MediaDescription

The largest struct, representing a single `m=` section and all its attributes.

```cpp
struct MediaDescription
{
  // Core fields (from m= line)
  MediaType mediaType = MediaType::AUDIO;
  std::uint16_t port = 0;                                   // transport port (0 = rejected)
  std::optional<std::uint16_t> numberOfPorts;                // multicast port count
  TransportProtocol protocol = TransportProtocol::RTP_AVP;
  std::vector<std::string> formats;                          // payload type numbers or format strings

  // Connection and bandwidth
  std::optional<ConnectionData> connection;                  // media-level c= (overrides session)
  std::vector<BandwidthInfo> bandwidths;                     // media-level b= lines
  std::optional<std::string> encryptionKey;                  // k= (deprecated in RFC 8866)

  // Identification and direction
  std::optional<std::string> mid;                            // a=mid (RFC 5888)
  Direction direction = Direction::SENDRECV;

  // Codec attributes
  std::vector<RtpMapAttribute> rtpMaps;                      // a=rtpmap lines
  std::vector<FmtpAttribute> fmtps;                          // a=fmtp lines
  std::vector<RtcpFbAttribute> rtcpFeedbacks;                // a=rtcp-fb lines

  // RTP extensions
  std::vector<ExtMapAttribute> extMaps;                      // a=extmap lines
  std::vector<SsrcAttribute> ssrcs;                          // a=ssrc lines
  std::vector<SsrcGroupAttribute> ssrcGroups;                // a=ssrc-group lines

  // ICE attributes
  std::vector<IceCandidate> candidates;                      // a=candidate lines
  std::optional<std::string> iceUfrag;                       // a=ice-ufrag
  std::optional<std::string> icePwd;                         // a=ice-pwd
  std::vector<std::string> iceOptions;                       // a=ice-options
  bool endOfCandidates = false;                              // a=end-of-candidates

  // DTLS attributes
  std::vector<FingerprintAttribute> fingerprints;            // a=fingerprint
  std::optional<SetupRole> setup;                            // a=setup

  // RTCP attributes
  bool rtcpMux = false;                                      // a=rtcp-mux (RFC 5761)
  bool rtcpMuxOnly = false;                                  // a=rtcp-mux-only (RFC 8858)
  bool rtcpRsize = false;                                    // a=rtcp-rsize (RFC 5506)
  std::optional<RtcpAttribute> rtcp;                         // a=rtcp (RFC 3605)

  // WebRTC attributes
  std::vector<MsidAttribute> msid;                           // a=msid (RFC 8830)

  // SRTP attributes
  std::vector<CryptoAttribute> crypto;                       // a=crypto (RFC 4568)

  // Data channel attributes
  std::optional<std::uint16_t> sctpPort;                     // a=sctp-port (RFC 8841)
  std::optional<std::uint32_t> maxMessageSize;               // a=max-message-size

  // Simulcast attributes
  std::optional<SimulcastAttribute> simulcast;
  std::vector<RidAttribute> rids;                            // a=rid lines

  // Media timing attributes
  std::optional<std::uint32_t> ptime;                        // a=ptime (ms)
  std::optional<std::uint32_t> maxptime;                     // a=maxptime (ms)
  std::optional<double> framerate;                           // a=framerate

  // Extension
  bool extmapAllowMixed = false;                             // a=extmap-allow-mixed (RFC 8285)
  std::vector<GenericAttribute> genericAttributes;           // unrecognized a= lines
};
```

**Code example — WebRTC audio media section:**

```cpp
using namespace iora::sdp;

MediaDescription audio;
audio.mediaType = MediaType::AUDIO;
audio.port = 9;
audio.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
audio.formats = {"111", "0", "8"};
audio.mid = "0";
audio.direction = Direction::SENDRECV;
audio.rtcpMux = true;

RtpMapAttribute opus;
opus.payloadType = 111;
opus.encodingName = "opus";
opus.clockRate = 48000;
opus.channels = 2;
audio.rtpMaps.push_back(opus);
```

### SessionDescription

Top-level container representing a complete SDP message.

```cpp
struct SessionDescription
{
  // Protocol and identity
  std::uint8_t version = 0;                                    // SDP protocol version (always 0)
  Origin origin;                                               // o= line
  std::string sessionName = "-";                               // s= line

  // Optional session info
  std::optional<std::string> sessionInfo;                      // i= line
  std::optional<std::string> uri;                              // u= line
  std::optional<std::string> emailAddress;                     // e= line
  std::optional<std::string> phoneNumber;                      // p= line

  // Connection and bandwidth
  std::optional<ConnectionData> connection;                    // session-level c=
  std::vector<BandwidthInfo> bandwidths;                       // session-level b= lines

  // Timing
  std::vector<TimeDescription> timeDescriptions;               // t= lines
  std::vector<ZoneAdjustment> zoneAdjustments;                 // z= line

  // Deprecated
  std::optional<std::string> encryptionKey;                    // k= line (deprecated in RFC 8866)

  // Session-level attributes
  std::vector<GenericAttribute> attributes;                    // unrecognized session-level a= lines
  std::vector<GroupAttribute> groups;                          // a=group (BUNDLE, LS, FID)
  std::optional<std::string> iceUfrag;                         // session-level ice-ufrag
  std::optional<std::string> icePwd;                           // session-level ice-pwd
  std::vector<std::string> iceOptions;                         // session-level ice-options
  bool iceLite = false;                                        // a=ice-lite
  std::vector<FingerprintAttribute> fingerprints;              // session-level fingerprint
  std::optional<SetupRole> setup;                              // session-level setup
  bool extmapAllowMixed = false;                               // a=extmap-allow-mixed (RFC 8285)

  // Media sections
  std::vector<MediaDescription> mediaDescriptions;             // ordered list of m= blocks
};
```

**Code example — minimal SessionDescription:**

```cpp
using namespace iora::sdp;

SessionDescription session;
session.version = 0;
session.origin.sessId = "1234567890";
session.origin.sessVersion = "1";
session.sessionName = "-";
session.timeDescriptions.push_back({0, 0, {}});
// Add media sections as needed...
```

**Session-level attribute inheritance:** Session-level ICE credentials (`iceUfrag`, `icePwd`),
fingerprints, and setup role apply to all media sections that do not override them at the media level.

---

## 5. Parser API (Layer 2)

**Header:** `include/iora/sdp/sdp_parser.hpp`

### ParseError and ParseWarning

```cpp
struct ParseError
{
  std::size_t line = 0;      // 1-based line number where error occurred
  std::string message;       // human-readable error description
};

struct ParseWarning
{
  std::size_t line = 0;      // 1-based line number
  std::string message;       // human-readable warning description
};
```

Both provide `operator==` and `operator!=`.

**Code example:**

```cpp
using namespace iora::sdp;

auto result = SdpParser::parse(sdpText);
if (result.hasError())
{
  std::cerr << "Parse error at line " << result.error->line
            << ": " << result.error->message << "\n";
}
```

### ParseResult\<T\>

Template struct returned by all parse operations:

```cpp
template<typename T>
struct ParseResult
{
  std::optional<T> value;                   // parsed result (present on success)
  std::optional<ParseError> error;          // fatal error (present on failure)
  std::vector<ParseWarning> warnings;       // non-fatal issues from lenient parsing

  bool hasValue() const noexcept;
  bool hasError() const noexcept;
};
```

**Important:** If `hasError()` is true, `value` will be empty (reset by the parser on error).

**Code example — three-state pattern:**

```cpp
using namespace iora::sdp;

auto result = SdpParser::parse(sdpText);

if (result.hasError())
{
  // Failure: error contains line number and message
  handleError(*result.error);
}
else if (!result.warnings.empty())
{
  // Success with warnings: data model is valid but some lines were skipped
  auto& session = *result.value;
  for (const auto& w : result.warnings)
  {
    logWarning(w.line, w.message);
  }
}
else
{
  // Clean success: no warnings
  auto& session = *result.value;
}
```

### SdpParserOptions

```cpp
struct SdpParserOptions
{
  bool strict = false;                // reject entire SDP on any malformed line
  bool requireVersion = true;         // require v=0 as first line
  bool requireOrigin = true;          // require an o= line
  bool requireSessionName = true;     // require an s= line
  bool requireTiming = false;         // require at least one t= line
  std::size_t maxLineLength = 4096;   // max line length (DoS prevention)
  std::size_t maxMediaSections = 64;  // max m= sections (resource exhaustion prevention)
};
```

**Design rationale for defaults:**

- `strict=false` — real-world SDP from browsers is frequently non-conformant
- `requireTiming=false` — WebRTC SDP from browsers sometimes omits timing
- `maxLineLength=4096` — prevents memory exhaustion from crafted long lines
- `maxMediaSections=64` — prevents resource exhaustion from many m= sections

**Code example — strict mode for validation:**

```cpp
using namespace iora::sdp;

SdpParserOptions strict;
strict.strict = true;
strict.requireTiming = true;
auto result = SdpParser::parse(sdpText, strict);
```

**Code example — lenient mode for production:**

```cpp
using namespace iora::sdp;

// Default options are lenient
auto result = SdpParser::parse(sdpText);
// Equivalent to:
auto result2 = SdpParser::parse(sdpText, SdpParserOptions{});
```

### SdpParser

Non-instantiable struct (constructor deleted). All methods are `static`.

```cpp
struct SdpParser
{
  SdpParser() = delete;

  /// Parse SDP text with default options (lenient mode).
  static ParseResult<SessionDescription> parse(std::string_view sdpText);

  /// Parse SDP text with explicit options.
  static ParseResult<SessionDescription> parse(
      std::string_view sdpText, const SdpParserOptions& options);

  /// Parse a single media section (m= line + subsequent lines).
  static ParseResult<MediaDescription> parseMediaSection(std::string_view text);

  /// Parse a single a= line into name + optional value.
  static GenericAttribute parseAttribute(std::string_view line);
};
```

**Code example — basic parse:**

```cpp
using namespace iora::sdp;

auto result = SdpParser::parse(sdpText);
if (result.hasValue())
{
  auto& session = *result.value;
  for (const auto& media : session.mediaDescriptions)
  {
    std::cout << "Media: " << toString(media.mediaType) << "\n";
    for (const auto& codec : media.rtpMaps)
    {
      std::cout << "  Codec: " << codec.encodingName << "/" << codec.clockRate << "\n";
    }
  }
}
```

**Code example — parseMediaSection for trickle ICE:**

```cpp
using namespace iora::sdp;

std::string mediaSection =
    "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
    "a=candidate:1 1 udp 2113937151 192.168.1.100 5000 typ host\r\n";
auto result = SdpParser::parseMediaSection(mediaSection);
if (result.hasValue())
{
  auto& media = *result.value;
  // media.candidates contains the parsed candidate
}
```

**Code example — parseAttribute for testing:**

```cpp
using namespace iora::sdp;

auto attr = SdpParser::parseAttribute("a=ice-ufrag:abc123");
// attr.name == "ice-ufrag"
// attr.value == "abc123"

auto flag = SdpParser::parseAttribute("a=rtcp-mux");
// flag.name == "rtcp-mux"
// flag.value == std::nullopt
```

### Parser State Machine

The parser operates in two phases:

1. **Session phase:** Lines before the first `m=` are session-level (`v=`, `o=`, `s=`, `i=`, `u=`, `e=`, `p=`, `c=`, `b=`, `t=`, `r=`, `z=`, `k=`, `a=`).
2. **Media phase:** On encountering `m=`, the parser transitions to media-level context. Subsequent lines (`c=`, `b=`, `k=`, `a=`) apply to the current `MediaDescription` until the next `m=` or end of input.

**Attribute dispatch:** Known attributes are dispatched to typed struct fields; unknown attributes are stored as `GenericAttribute`.

**Line ending handling:** RFC mandates CRLF but the parser tolerates bare LF and bare CR for robustness.

**Whitespace handling:** Trimmed in lenient mode, rejected in strict mode.

**Duplicate attribute handling:**

- Unique attributes (`ice-ufrag`, `ice-pwd`, `mid`, `setup`): overwrite-with-warning in lenient mode, error in strict mode.
- Multi-value attributes (`candidate`, `ssrc`, `rtpmap`, `fmtp`, `rtcp-fb`, `extmap`, etc.): always append.

### Lenient vs. Strict Mode

| Condition | Lenient (default) | Strict |
|-----------|-------------------|--------|
| Malformed line | Skip + warning | Fatal error |
| Leading/trailing whitespace | Trimmed | Fatal error |
| Line exceeds `maxLineLength` | Skip + warning | Fatal error |
| Duplicate unique attribute | Overwrite + warning | Fatal error |
| Unexpected line type | Skip + warning | Fatal error |
| Missing `v=` line | Error if `requireVersion` | Error |

**Code example — same input, different modes:**

```cpp
using namespace iora::sdp;

std::string malformed = "v=0\r\no=- 0 0 IN IP4 0.0.0.0\r\ns=-\r\n  bad line  \r\nt=0 0\r\n";

// Lenient: succeeds with a warning
auto lenient = SdpParser::parse(malformed);
assert(lenient.hasValue());
assert(!lenient.warnings.empty());

// Strict: fails on the leading whitespace
SdpParserOptions opts;
opts.strict = true;
auto strict = SdpParser::parse(malformed, opts);
assert(strict.hasError());
```

### Security and Limits

The parser processes untrusted input from network peers (SIP INVITE bodies, WebRTC signaling
messages). Two options provide DoS prevention:

- `maxLineLength` (default 4096) — prevents memory exhaustion from single long lines
- `maxMediaSections` (default 64) — prevents resource exhaustion from many `m=` sections

**Code example — conservative limits for a public-facing server:**

```cpp
using namespace iora::sdp;

SdpParserOptions opts;
opts.maxLineLength = 2048;
opts.maxMediaSections = 16;
auto result = SdpParser::parse(untrustedSdp, opts);
```

---

## 6. Serializer API (Layer 3)

**Header:** `include/iora/sdp/sdp_serializer.hpp`

### SdpSerializerOptions

```cpp
struct SdpSerializerOptions
{
  std::string lineEnding = "\r\n";    // line ending sequence
  bool omitSessionName = false;       // omit s= when sessionName is empty
  bool omitTiming = false;            // omit t= when timeDescriptions is empty
};
```

**Note:** RFC 4566 mandates CRLF line endings, but some applications require bare LF.

**Code example — LF-only line endings for testing:**

```cpp
using namespace iora::sdp;

SdpSerializerOptions opts;
opts.lineEnding = "\n";
auto sdpText = SdpSerializer::serialize(session, opts);
```

### SdpSerializer

Non-instantiable struct (constructor deleted). All methods are `static`.

```cpp
struct SdpSerializer
{
  SdpSerializer() = delete;

  /// Serialize with default options (CRLF line endings, include all fields).
  static std::string serialize(const SessionDescription& session);

  /// Serialize with explicit options.
  static std::string serialize(
      const SessionDescription& session, const SdpSerializerOptions& options);

  /// Serialize a single media section with default options.
  static std::string serializeMediaSection(const MediaDescription& media);

  /// Serialize a single media section with explicit options.
  static std::string serializeMediaSection(
      const MediaDescription& media, const SdpSerializerOptions& options);

  /// Serialize a single attribute to "a=name[:value]" (NO line ending appended).
  static std::string serializeAttribute(const GenericAttribute& attr);
};
```

**Code example — full serialize flow:**

```cpp
using namespace iora::sdp;

auto result = SdpParser::parse(inputSdp);
if (result.hasValue())
{
  auto sdpText = SdpSerializer::serialize(*result.value);
  // sdpText is a conformant SDP string with CRLF line endings
}
```

**Code example — serializing a single media section for trickle ICE:**

```cpp
using namespace iora::sdp;

MediaDescription media;
media.mediaType = MediaType::AUDIO;
media.port = 9;
media.protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
media.formats = {"111"};
// ... add candidates ...
auto section = SdpSerializer::serializeMediaSection(media);
```

### Field Ordering

The serializer emits fields in RFC-correct order regardless of the order the application
populates the data model.

**Session-level field order** (per RFC 4566 section 5):

```
v=, o=, s=, i=, u=, e=, p=, c=, b=, t= (with r=), z=, k=
```

Then session-level `a=` attributes in conventional order:

```
ice-ufrag, ice-pwd, ice-options, ice-lite, fingerprint, setup,
extmap-allow-mixed, group, generic
```

**Media-level field order:**

```
m=, c=, b=, k=
```

Then media-level `a=` attributes in conventional order:

1. `direction`
2. `mid`
3. `msid`
4. `ice-ufrag`, `ice-pwd`, `ice-options`
5. `fingerprint`, `setup`
6. `extmap-allow-mixed`
7. `rtcp-mux`, `rtcp-mux-only`, `rtcp-rsize`
8. `rtcp`
9. `extmap`
10. **Codec grouping:** for each format, emit `rtpmap`, `fmtp`, then `rtcp-fb` for that payload type
11. Wildcard `rtcp-fb` (`payloadType="*"`)
12. `ptime`, `maxptime`, `framerate`
13. `ssrc`, `ssrc-group`
14. `crypto`
15. `sctp-port`, `max-message-size`
16. `simulcast`, `rid`
17. `candidate`, `end-of-candidates`
18. Generic attributes (preserve insertion order)

**Note:** Codec grouping (step 10) emits `rtpmap`+`fmtp`+`rtcp-fb` per payload type, following
Chrome/Firefox convention.

### Default Emission

- When `timeDescriptions` is empty and `omitTiming` is false, the serializer emits `t=0 0`.
- When `sessionName` is set, `s=` line is always emitted (unless `omitSessionName=true` and `sessionName` is empty).
- The `v=` line always emits the stored version (always 0).
- The `o=` line always uses the stored `Origin` values.

---

## 7. Offer/Answer Utilities (Layer 4)

**Header:** `include/iora/sdp/offer_answer.hpp`

### Design Philosophy

These are **stateless utility functions**, not a negotiation state machine. Applications drive
negotiation logic; these functions are composable building blocks.

**Rationale:** SDP negotiation is deeply intertwined with the signaling protocol
(SIP/WebRTC/proprietary). A generic state machine would either be too simple or too complex.

### OfferOptions

```cpp
struct OfferOptions
{
  bool audio = true;                                        // include audio media section
  bool video = true;                                        // include video media section
  bool dataChannel = false;                                 // include SCTP data channel section
  std::optional<std::string> iceUfrag;                      // explicit ICE ufrag (auto-generated if absent)
  std::optional<std::string> icePwd;                        // explicit ICE password (auto-generated if absent)
  std::optional<FingerprintAttribute> fingerprint;          // DTLS fingerprint
  bool bundlePolicy = true;                                 // add BUNDLE group for all media
  bool rtcpMux = true;                                      // add rtcp-mux to all media sections
  SetupRole dtlsSetup = SetupRole::ACTPASS;                 // DTLS setup role
};
```

**Code example — audio-only offer:**

```cpp
using namespace iora::sdp;

OfferOptions opts;
opts.audio = true;
opts.video = false;
opts.dataChannel = false;
auto offer = OfferAnswer::createOffer(opts);
```

### AnswerOptions

```cpp
struct AnswerOptions
{
  std::optional<std::string> iceUfrag;                      // explicit ICE ufrag
  std::optional<std::string> icePwd;                        // explicit ICE password
  std::optional<FingerprintAttribute> fingerprint;          // DTLS fingerprint
  SetupRole dtlsSetup = SetupRole::ACTIVE;                  // DTLS setup role for answer
  std::optional<std::vector<RtpMapAttribute>> supportedCodecs;  // codec intersection (absent = accept all)
  std::vector<MediaType> rejectMediaTypes;                  // media types to reject (port=0)
};
```

**Code example — rejecting video in an answer:**

```cpp
using namespace iora::sdp;

AnswerOptions opts;
opts.rejectMediaTypes = {MediaType::VIDEO};
auto answer = OfferAnswer::createAnswer(offer, opts);
// Video media section will have port=0 and direction=INACTIVE
```

### TransceiverOptions

```cpp
struct TransceiverOptions
{
  std::string mid;                                           // media identification tag
  Direction direction = Direction::SENDRECV;
  TransportProtocol protocol = TransportProtocol::UDP_TLS_RTP_SAVPF;
  std::optional<std::string> iceUfrag;
  std::optional<std::string> icePwd;
  std::optional<FingerprintAttribute> fingerprint;
  SetupRole setup = SetupRole::ACTPASS;
  std::optional<MsidAttribute> msid;
};
```

### DataChannelOptions

```cpp
struct DataChannelOptions
{
  std::string mid;                                           // media identification tag
  std::uint16_t sctpPort = 5000;
  std::uint32_t maxMessageSize = 262144;                     // 256 KB
  std::optional<std::string> iceUfrag;
  std::optional<std::string> icePwd;
  std::optional<FingerprintAttribute> fingerprint;
  SetupRole setup = SetupRole::ACTPASS;
};
```

### OfferAnswer Static Methods

```cpp
struct OfferAnswer
{
  OfferAnswer() = delete;

  /// Flip direction per RFC 3264:
  /// SENDRECV->SENDRECV, SENDONLY->RECVONLY, RECVONLY->SENDONLY, INACTIVE->INACTIVE
  static Direction flipDirection(Direction direction) noexcept;

  /// Set the direction attribute of a media section.
  static void setMediaDirection(MediaDescription& media, Direction direction) noexcept;

  /// Disable media by setting port=0 and direction=INACTIVE.
  static void disableMedia(MediaDescription& media) noexcept;

  /// Check if port == 0.
  static bool isMediaDisabled(const MediaDescription& media) noexcept;

  /// Find media section by mid value. Returns nullptr if not found.
  static MediaDescription* findMediaByMid(
      SessionDescription& session, const std::string& mid) noexcept;

  /// Const overload.
  static const MediaDescription* findMediaByMid(
      const SessionDescription& session, const std::string& mid) noexcept;

  /// Generate random ICE ufrag (8 chars) and pwd (24 chars) per RFC 8445 section 5.3.
  /// Character set: a-zA-Z0-9+/
  static std::pair<std::string, std::string> generateIceCredentials();

  /// Append a trickle ICE candidate to a media section.
  static void addIceCandidate(MediaDescription& media, const IceCandidate& candidate);

  /// Clear all candidates and reset endOfCandidates flag. Used for ICE restart.
  static void removeIceCandidates(MediaDescription& media) noexcept;

  /// Intersect codec lists by encoding name (case-insensitive) and clock rate.
  /// Returns codecs ordered by answerer's preference.
  static std::vector<RtpMapAttribute> matchCodecs(
      const MediaDescription& offer, const MediaDescription& answer);

  /// Remove rtpmap, fmtp, rtcp-fb, and format entries for payload types NOT in retain list.
  /// Wildcard rtcp-fb (payloadType="*") is preserved.
  static void pruneUnusedCodecs(
      MediaDescription& media, const std::vector<std::uint8_t>& retainPayloadTypes);

  /// Create a MediaDescription with WebRTC defaults:
  /// port=9, rtcpMux=true, configurable protocol/direction/mid/setup/ICE/DTLS/msid.
  static MediaDescription addTransceiver(MediaType type, const TransceiverOptions& options);

  /// Create a MediaDescription for SCTP data channels:
  /// mediaType=APPLICATION, protocol=UDP_DTLS_SCTP, formats={"webrtc-datachannel"},
  /// port=9, direction=SENDRECV.
  static MediaDescription addDataChannel(const DataChannelOptions& options);

  /// Scaffold a new SessionDescription for a WebRTC or SIP offer.
  /// Auto-generates: random sessId, version="1", sessionName="-", t=0 0,
  /// ICE credentials (if not provided), trickle ice-options, BUNDLE group.
  static SessionDescription createOffer(const OfferOptions& options = {});

  /// Generate answer from offer: copies groups, creates matching media sections
  /// with flipped directions. Codec intersection if supportedCodecs provided.
  /// Media rejection if mediaType in rejectMediaTypes. Application (data channel)
  /// media sections copied as-is. Auto-generates ICE credentials.
  static SessionDescription createAnswer(
      const SessionDescription& offer, const AnswerOptions& answerOptions = {});
};
```

**Code example — complete offer/answer flow:**

```cpp
using namespace iora::sdp;

// 1. Create offer
OfferOptions offerOpts;
offerOpts.audio = true;
offerOpts.video = true;
offerOpts.dataChannel = true;
auto offer = OfferAnswer::createOffer(offerOpts);

// 2. Add codecs to audio section
auto* audio = OfferAnswer::findMediaByMid(offer, "0");
RtpMapAttribute opus;
opus.payloadType = 111;
opus.encodingName = "opus";
opus.clockRate = 48000;
opus.channels = 2;
audio->rtpMaps.push_back(opus);
audio->formats.push_back("111");

// 3. Serialize offer and send via signaling
auto offerSdp = SdpSerializer::serialize(offer);

// 4. Parse received answer
auto answerResult = SdpParser::parse(receivedAnswerSdp);
if (answerResult.hasValue())
{
  auto& answer = *answerResult.value;

  // 5. Match codecs
  auto matched = OfferAnswer::matchCodecs(
      offer.mediaDescriptions[0], answer.mediaDescriptions[0]);

  // 6. Prune unused codecs
  std::vector<std::uint8_t> retainPts;
  for (const auto& codec : matched)
  {
    retainPts.push_back(codec.payloadType);
  }
  OfferAnswer::pruneUnusedCodecs(offer.mediaDescriptions[0], retainPts);

  // 7. Add ICE candidates
  IceCandidate cand;
  cand.foundation = "1";
  cand.component = 1;
  cand.transport = IceTransportType::UDP;
  cand.priority = 2113937151;
  cand.address = "192.168.1.100";
  cand.port = 5000;
  cand.type = IceCandidateType::HOST;
  OfferAnswer::addIceCandidate(offer.mediaDescriptions[0], cand);
}
```

---

## 8. Usage Examples

### Parsing a WebRTC SDP Offer

```cpp
#include "iora/sdp/sdp_parser.hpp"
#include <iostream>

using namespace iora::sdp;

int main()
{
  std::string sdpText =
      "v=0\r\n"
      "o=- 4611731400430051336 2 IN IP4 127.0.0.1\r\n"
      "s=-\r\n"
      "t=0 0\r\n"
      "a=group:BUNDLE 0 1\r\n"
      "a=extmap-allow-mixed\r\n"
      "a=ice-ufrag:abc1\r\n"
      "a=ice-pwd:longpassword1234567890ab\r\n"
      "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99"
          ":AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n"
      "a=setup:actpass\r\n"
      "m=audio 9 UDP/TLS/RTP/SAVPF 111 0\r\n"
      "c=IN IP4 0.0.0.0\r\n"
      "a=mid:0\r\n"
      "a=sendrecv\r\n"
      "a=rtcp-mux\r\n"
      "a=rtpmap:111 opus/48000/2\r\n"
      "a=fmtp:111 minptime=10;useinbandfec=1\r\n"
      "a=rtpmap:0 PCMU/8000\r\n"
      "a=candidate:1 1 udp 2113937151 192.168.1.100 5000 typ host\r\n"
      "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
      "c=IN IP4 0.0.0.0\r\n"
      "a=mid:1\r\n"
      "a=sendrecv\r\n"
      "a=rtcp-mux\r\n"
      "a=rtpmap:96 VP8/90000\r\n";

  auto result = SdpParser::parse(sdpText);
  if (result.hasError())
  {
    std::cerr << "Error: " << result.error->message << "\n";
    return 1;
  }

  auto& session = *result.value;
  std::cout << "Session: " << session.sessionName << "\n";
  std::cout << "ICE ufrag: " << session.iceUfrag.value_or("(none)") << "\n";
  std::cout << "Media sections: " << session.mediaDescriptions.size() << "\n";

  for (const auto& media : session.mediaDescriptions)
  {
    std::cout << "\n  " << toString(media.mediaType) << " (mid=" << media.mid.value_or("?") << ")\n";
    for (const auto& codec : media.rtpMaps)
    {
      std::cout << "    " << codec.encodingName << "/" << codec.clockRate
                << " (PT " << static_cast<int>(codec.payloadType) << ")\n";
    }
    for (const auto& cand : media.candidates)
    {
      std::cout << "    Candidate: " << cand.address << ":" << cand.port
                << " " << toString(cand.type) << "\n";
    }
  }
}
```

### Constructing and Serializing an SDP Offer

```cpp
#include "iora/sdp/offer_answer.hpp"
#include "iora/sdp/sdp_serializer.hpp"
#include <iostream>

using namespace iora::sdp;

int main()
{
  // Create offer with audio and video
  auto offer = OfferAnswer::createOffer();

  // Add Opus codec to audio section
  auto* audio = OfferAnswer::findMediaByMid(offer, "0");
  RtpMapAttribute opus;
  opus.payloadType = 111;
  opus.encodingName = "opus";
  opus.clockRate = 48000;
  opus.channels = 2;
  audio->rtpMaps.push_back(opus);
  audio->formats.push_back("111");

  FmtpAttribute opusFmtp;
  opusFmtp.payloadType = 111;
  opusFmtp.parameters = "minptime=10;useinbandfec=1";
  opusFmtp.parameterMap = {{"minptime", "10"}, {"useinbandfec", "1"}};
  audio->fmtps.push_back(opusFmtp);

  // Add VP8 codec to video section
  auto* video = OfferAnswer::findMediaByMid(offer, "1");
  RtpMapAttribute vp8;
  vp8.payloadType = 96;
  vp8.encodingName = "VP8";
  vp8.clockRate = 90000;
  video->rtpMaps.push_back(vp8);
  video->formats.push_back("96");

  // Add ICE candidate
  IceCandidate cand;
  cand.foundation = "1";
  cand.component = 1;
  cand.transport = IceTransportType::UDP;
  cand.priority = 2113937151;
  cand.address = "192.168.1.100";
  cand.port = 5000;
  cand.type = IceCandidateType::HOST;
  OfferAnswer::addIceCandidate(*audio, cand);

  // Serialize and send
  auto sdpText = SdpSerializer::serialize(offer);
  std::cout << sdpText;
}
```

### WebRTC Offer/Answer Flow

```cpp
#include "iora/sdp/offer_answer.hpp"
#include "iora/sdp/sdp_parser.hpp"
#include "iora/sdp/sdp_serializer.hpp"

using namespace iora::sdp;

void negotiate(const std::string& receivedOfferSdp)
{
  // 1. Parse the received offer
  auto offerResult = SdpParser::parse(receivedOfferSdp);
  if (!offerResult.hasValue()) return;
  auto& offer = *offerResult.value;

  // 2. Create answer with codec filtering
  AnswerOptions answerOpts;
  // Only support Opus and VP8
  answerOpts.supportedCodecs = std::vector<RtpMapAttribute>{
      {111, "opus", 48000, 2},
      {96, "VP8", 90000, {}}
  };

  auto answer = OfferAnswer::createAnswer(offer, answerOpts);

  // 3. Serialize the answer
  auto answerSdp = SdpSerializer::serialize(answer);
  // Send answerSdp via signaling...

  // 4. After negotiation, find common codecs
  for (std::size_t i = 0; i < offer.mediaDescriptions.size(); ++i)
  {
    auto matched = OfferAnswer::matchCodecs(
        offer.mediaDescriptions[i], answer.mediaDescriptions[i]);

    std::vector<std::uint8_t> retainPts;
    for (const auto& codec : matched)
    {
      retainPts.push_back(codec.payloadType);
    }
    // Prune codecs not in the intersection
    OfferAnswer::pruneUnusedCodecs(answer.mediaDescriptions[i], retainPts);
  }

  // 5. Add trickle ICE candidates as they arrive
  IceCandidate cand;
  cand.foundation = "1";
  cand.component = 1;
  cand.transport = IceTransportType::UDP;
  cand.priority = 2113937151;
  cand.address = "10.0.0.1";
  cand.port = 12345;
  cand.type = IceCandidateType::HOST;
  OfferAnswer::addIceCandidate(answer.mediaDescriptions[0], cand);
}
```

### SIP SDP (Non-WebRTC)

```cpp
#include "iora/sdp/sdp_parser.hpp"
#include <iostream>

using namespace iora::sdp;

void parseSipSdp()
{
  std::string sipSdp =
      "v=0\r\n"
      "o=alice 2890844526 2890844526 IN IP4 192.168.1.100\r\n"
      "s=Phone Call\r\n"
      "c=IN IP4 192.168.1.100\r\n"
      "t=0 0\r\n"
      "m=audio 5004 RTP/AVP 0 8 101\r\n"
      "a=rtpmap:0 PCMU/8000\r\n"
      "a=rtpmap:8 PCMA/8000\r\n"
      "a=rtpmap:101 telephone-event/8000\r\n"
      "a=fmtp:101 0-16\r\n"
      "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:base64keyvalue==\r\n"
      "a=sendrecv\r\n"
      "a=ptime:20\r\n";

  auto result = SdpParser::parse(sipSdp);
  if (result.hasValue())
  {
    auto& session = *result.value;
    auto& audio = session.mediaDescriptions[0];

    // SIP uses RTP/AVP, not UDP/TLS/RTP/SAVPF
    std::cout << "Protocol: " << toString(audio.protocol) << "\n";  // "RTP/AVP"

    // SDES-SRTP crypto instead of DTLS fingerprint
    for (const auto& c : audio.crypto)
    {
      std::cout << "Crypto: " << c.suite << "\n";  // "AES_CM_128_HMAC_SHA1_80"
    }

    // telephone-event fmtp uses raw parameters, not key=value
    for (const auto& fmtp : audio.fmtps)
    {
      if (fmtp.payloadType == 101)
      {
        std::cout << "DTMF range: " << fmtp.parameters << "\n";  // "0-16"
      }
    }
  }
}
```

### Data Channels

```cpp
#include "iora/sdp/offer_answer.hpp"
#include "iora/sdp/sdp_serializer.hpp"

using namespace iora::sdp;

void createDataChannelOffer()
{
  OfferOptions opts;
  opts.audio = false;
  opts.video = false;
  opts.dataChannel = true;

  auto offer = OfferAnswer::createOffer(opts);
  auto& dc = offer.mediaDescriptions[0];

  // Data channel uses SCTP
  // dc.mediaType == MediaType::APPLICATION
  // dc.protocol == TransportProtocol::UDP_DTLS_SCTP
  // dc.formats == {"webrtc-datachannel"}
  // dc.sctpPort == 5000
  // dc.maxMessageSize == 262144

  auto sdp = SdpSerializer::serialize(offer);
}
```

### SDP Manipulation (Parse-Modify-Serialize)

```cpp
#include "iora/sdp/offer_answer.hpp"
#include "iora/sdp/sdp_parser.hpp"
#include "iora/sdp/sdp_serializer.hpp"

using namespace iora::sdp;

std::string mungeSdp(const std::string& inputSdp)
{
  auto result = SdpParser::parse(inputSdp);
  if (!result.hasValue()) return {};

  auto& session = *result.value;

  for (auto& media : session.mediaDescriptions)
  {
    // Change direction to recvonly
    OfferAnswer::setMediaDirection(media, Direction::RECVONLY);

    // Remove unwanted codecs — keep only Opus and VP8
    std::vector<std::uint8_t> keep;
    for (const auto& rm : media.rtpMaps)
    {
      if (rm.encodingName == "opus" || rm.encodingName == "VP8")
      {
        keep.push_back(rm.payloadType);
      }
    }
    if (!keep.empty())
    {
      OfferAnswer::pruneUnusedCodecs(media, keep);
    }

    // Add an ICE candidate
    IceCandidate cand;
    cand.foundation = "1";
    cand.component = 1;
    cand.transport = IceTransportType::UDP;
    cand.priority = 2113937151;
    cand.address = "10.0.0.1";
    cand.port = 9000;
    cand.type = IceCandidateType::HOST;
    OfferAnswer::addIceCandidate(media, cand);
  }

  return SdpSerializer::serialize(session);
}
```

### Simulcast SDP

```cpp
#include "iora/sdp/offer_answer.hpp"
#include "iora/sdp/sdp_serializer.hpp"

using namespace iora::sdp;

void createSimulcastOffer()
{
  auto offer = OfferAnswer::createOffer();
  auto* video = OfferAnswer::findMediaByMid(offer, "1");

  // Add VP8 codec
  RtpMapAttribute vp8;
  vp8.payloadType = 96;
  vp8.encodingName = "VP8";
  vp8.clockRate = 90000;
  video->rtpMaps.push_back(vp8);
  video->formats.push_back("96");

  // Add RTP header extensions for stream identification
  video->extMaps.push_back({4, {}, "urn:ietf:params:rtp-hdrext:sdes:mid"});
  video->extMaps.push_back({5, {}, "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id"});
  video->extMaps.push_back({6, {}, "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id"});

  // Add RID restrictions for three quality layers
  RidAttribute high;
  high.id = "h";
  high.direction = RidDirection::SEND;
  high.payloadTypes = {96};
  high.restrictions = {{"max-width", "1920"}, {"max-height", "1080"}, {"max-fps", "30"}};
  video->rids.push_back(high);

  RidAttribute medium;
  medium.id = "m";
  medium.direction = RidDirection::SEND;
  medium.payloadTypes = {96};
  medium.restrictions = {{"max-width", "640"}, {"max-height", "360"}, {"max-fps", "30"}};
  video->rids.push_back(medium);

  RidAttribute low;
  low.id = "l";
  low.direction = RidDirection::SEND;
  low.payloadTypes = {96};
  low.restrictions = {{"max-width", "320"}, {"max-height", "180"}, {"max-fps", "15"}};
  video->rids.push_back(low);

  // Add simulcast attribute
  SimulcastAttribute sim;
  sim.sendStreams = {{{"h", false}}, {{"m", false}}, {{"l", false}}};
  video->simulcast = sim;

  auto sdp = SdpSerializer::serialize(offer);
}
```

### Trickle ICE

```cpp
#include "iora/sdp/offer_answer.hpp"
#include "iora/sdp/sdp_parser.hpp"
#include "iora/sdp/sdp_serializer.hpp"

using namespace iora::sdp;

void trickleIce()
{
  auto result = SdpParser::parse(offerSdp);
  if (!result.hasValue()) return;
  auto& session = *result.value;

  // Initially no candidates — send offer first, then trickle

  // As candidates are discovered, add them:
  IceCandidate host;
  host.foundation = "1";
  host.component = 1;
  host.transport = IceTransportType::UDP;
  host.priority = 2113937151;
  host.address = "192.168.1.100";
  host.port = 5000;
  host.type = IceCandidateType::HOST;
  OfferAnswer::addIceCandidate(session.mediaDescriptions[0], host);

  IceCandidate srflx;
  srflx.foundation = "2";
  srflx.component = 1;
  srflx.transport = IceTransportType::UDP;
  srflx.priority = 1845501695;
  srflx.address = "203.0.113.1";
  srflx.port = 6000;
  srflx.type = IceCandidateType::SRFLX;
  srflx.relAddr = "192.168.1.100";
  srflx.relPort = 5000;
  OfferAnswer::addIceCandidate(session.mediaDescriptions[0], srflx);

  // Signal end of candidates
  session.mediaDescriptions[0].endOfCandidates = true;

  // For ICE restart, clear and start over
  OfferAnswer::removeIceCandidates(session.mediaDescriptions[0]);
  auto [newUfrag, newPwd] = OfferAnswer::generateIceCredentials();
  session.mediaDescriptions[0].iceUfrag = newUfrag;
  session.mediaDescriptions[0].icePwd = newPwd;
}
```

---

## 9. Round-Trip Patterns

### Round-Trip Guarantee

The library guarantees:

```
parse(serialize(parse(input))) == parse(input)
```

The comparison uses `operator==` on the data model, not string comparison. Serialized text may
differ from the original input in whitespace, attribute ordering, or formatting, but the parsed
data model is equivalent.

**Note:** Unknown attributes are preserved through `GenericAttribute`, ensuring proprietary
extensions survive round-trip.

### Round-Trip Code Pattern

```cpp
using namespace iora::sdp;

auto r1 = SdpParser::parse(input);
REQUIRE(r1.hasValue());

auto serialized = SdpSerializer::serialize(*r1.value);
auto r2 = SdpParser::parse(serialized);
REQUIRE(r2.hasValue());

REQUIRE(*r1.value == *r2.value);  // data model equivalence
```

### What Round-Trips and What Does Not

**Faithfully round-trips:**

- All typed attributes (rtpmap, fmtp, rtcp-fb, extmap, ssrc, ssrc-group, candidate, fingerprint, crypto, msid, group, simulcast, rid)
- All enum values (direction, setup role, media type, transport protocol)
- Unknown/generic attributes (via `GenericAttribute`)
- ICE candidates with all extensions

**May change in serialized form:**

- Attribute ordering within a media section (serializer uses conventional order)
- Line ending normalization (input may use bare LF, output uses CRLF by default)
- Whitespace trimming

**Does NOT round-trip:**

- Media-level `i=` lines (parsed but not stored on `MediaDescription`)
- Duplicate unique attributes (last one wins in lenient mode)
- SDP has no comment syntax, so there is nothing to lose

---

## 10. Error Handling

### Parse Error Patterns

Three outcomes of `SdpParser::parse()`:

1. **Success:** `hasValue() == true`, `hasError() == false`, warnings may be empty
2. **Success with warnings:** `hasValue() == true`, `hasError() == false`, warnings non-empty (lenient mode skipped bad lines)
3. **Failure:** `hasValue() == false`, `hasError() == true`, error contains line number and message

**Code example — robust parse wrapper:**

```cpp
using namespace iora::sdp;

std::optional<SessionDescription> safeParse(
    const std::string& sdpText, std::function<void(const std::string&)> logger)
{
  auto result = SdpParser::parse(sdpText);
  if (result.hasError())
  {
    logger("SDP parse error at line " + std::to_string(result.error->line)
           + ": " + result.error->message);
    return std::nullopt;
  }
  for (const auto& w : result.warnings)
  {
    logger("SDP warning at line " + std::to_string(w.line) + ": " + w.message);
  }
  return std::move(result.value);
}
```

### Common Parse Errors

| Error message | Cause |
|---------------|-------|
| `Empty SDP` | Input is empty |
| `Missing required v= line` | No version line (`requireVersion=true`) |
| `Missing required o= line` | No origin line (`requireOrigin=true`) |
| `Missing required s= line` | No session name (`requireSessionName=true`) |
| `Missing required t= line` | No timing line (`requireTiming=true`) |
| `Exceeded maxMediaSections limit` | Too many `m=` sections |
| `Line exceeds maxLineLength` | Single line too long (strict mode) |
| `Duplicate a=ice-ufrag` | Strict mode with duplicate unique attribute |
| `Malformed line: missing type=value format` | Line doesn't match `x=value` pattern |
| `Leading/trailing whitespace in strict mode` | Whitespace present in strict mode |

### Warning Patterns in Lenient Mode

Common warnings returned in `ParseResult::warnings`:

- `"Malformed line, skipping"` — unparseable line was skipped
- `"Duplicate a=ice-ufrag at session level, overwriting"` — duplicate unique attribute
- `"Line exceeds maxLineLength, skipping"` — long line was skipped
- `"Unexpected line type 'X' in media section, skipping"` — unexpected character after `m=`
- `"SDP version is not 0"` — non-zero version number

**Code example — iterating warnings:**

```cpp
using namespace iora::sdp;

auto result = SdpParser::parse(sdpText);
for (const auto& w : result.warnings)
{
  std::cerr << "[WARN] line " << w.line << ": " << w.message << "\n";
}
```

### Defensive Parsing for Production

Best practices:

1. Always check `hasError()` before accessing `value`
2. Log warnings for debugging but do not fail on them
3. Set conservative `maxLineLength` and `maxMediaSections` for public-facing servers
4. Use lenient mode for production, strict mode for validation/testing
5. Handle the case where optional fields are absent (e.g., no ICE candidates yet)

---

## 11. Build Integration

### CMake Integration

**Method 1: add_subdirectory**

```cmake
add_subdirectory(third_party/iora_sdp)
target_link_libraries(myapp PRIVATE iora_sdp)
```

**Method 2: FetchContent**

```cmake
include(FetchContent)
FetchContent_Declare(iora_sdp
  GIT_REPOSITORY https://github.com/example/iora_sdp.git
  GIT_TAG main
)
FetchContent_MakeAvailable(iora_sdp)
target_link_libraries(myapp PRIVATE iora_sdp)
```

**Method 3: Include path only** (since header-only)

```cmake
target_include_directories(myapp PRIVATE /path/to/iora_sdp/include)
```

**Note:** iora_sdp is an `INTERFACE` library — it adds only an include path, no link dependencies.

**Minimum requirements:** CMake 3.20, C++17 compiler (GCC, Clang, or MSVC).

### Header Include Order

Recommended includes for different use cases:

| Use Case | Include | What it pulls in |
|----------|---------|------------------|
| Parse only | `iora/sdp/sdp_parser.hpp` | Layers 0-2 |
| Parse + serialize | `iora/sdp/sdp_serializer.hpp` | Layers 0-3 |
| Full library | `iora/sdp/offer_answer.hpp` | All layers |
| Types only | `iora/sdp/types.hpp` | Layer 0 |
| Data model only | `iora/sdp/session_description.hpp` | Layers 0-1 |

### Compiler Compatibility

**Tested compilers:**

- GCC (latest stable) — primary development compiler on Linux
- Clang (latest stable) — used for sanitizer builds and fuzz testing
- MSVC (latest Visual Studio) — Windows compatibility

**C++17 features used:** `std::optional`, `std::string_view`, structured bindings, `if constexpr`.

**Note:** ~2000 lines total across all headers. Compile time impact is modest.

---

## 12. Fuzz Testing

### Fuzz Testing Strategy

The SDP parser processes untrusted input from network peers (SIP INVITE bodies, WebRTC signaling
messages). Fuzz testing ensures the parser never crashes, hangs, or exhibits undefined behavior
on any input.

**Target:** `SdpParser::parse()` — the primary entry point for untrusted SDP.

**Guarantee:** The parser must never crash, hang, or trigger undefined behavior regardless of input.

The harness also exercises the round-trip path: if parsing succeeds, the result is serialized and
re-parsed to verify round-trip stability under adversarial input.

### Fuzz Harness

**File:** `tests/fuzz_parser.cpp`

**Compatible with:** libFuzzer (LLVM), AFL persistent mode, standalone stdin.

The core logic:

```cpp
#include "iora/sdp/sdp_parser.hpp"
#include "iora/sdp/sdp_serializer.hpp"

using namespace iora::sdp;

static void fuzzOne(const uint8_t* data, size_t size)
{
  std::string input(reinterpret_cast<const char*>(data), size);
  auto r = SdpParser::parse(input);
  if (r.hasValue())
  {
    auto serialized = SdpSerializer::serialize(*r.value);
    auto r2 = SdpParser::parse(serialized);
    (void)r2;
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  try { fuzzOne(data, size); } catch (...) {}
  return 0;
}
```

The file also includes an AFL persistent mode block (`__AFL_LOOP(10000)`) and a stdin fallback for
manual testing without a fuzzer engine.

### Running the Fuzzer

**Build:**

```bash
cmake -S . -B build-fuzz \
  -DIORA_SDP_BUILD_FUZZ=ON \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_CXX_FLAGS='-fsanitize=fuzzer,address'
cmake --build build-fuzz
```

**Run with libFuzzer:**

```bash
mkdir -p corpus
cp tests/fuzz_corpus/* corpus/
./build-fuzz/fuzz_parser corpus/ -dict=tests/fuzz_parser.dict
```

**Run with stdin (manual testing):**

```bash
# Build without sanitizer flags for stdin mode
cmake -S . -B build-fuzz-stdin -DIORA_SDP_BUILD_FUZZ=ON
cmake --build build-fuzz-stdin
echo -e "v=0\r\no=- 0 0 IN IP4 0.0.0.0\r\ns=-\r\nt=0 0\r\n" | ./build-fuzz-stdin/fuzz_parser
```

**Seed corpus:** `tests/fuzz_corpus/` contains 15 seed files (minimal, WebRTC, SIP, data channel,
maximal, truncated, empty, longline, binary prefix, no media, unknown attrs, bare LF, mixed
endings, null bytes, single char).

**Dictionary:** `tests/fuzz_parser.dict` contains 120+ SDP-specific tokens for dramatically faster
coverage.

### Sanitizer Builds

| Sanitizer | Detects | Build Command |
|-----------|---------|---------------|
| ASan | Heap/stack overflows, use-after-free, double-free, leaks | `-DCMAKE_CXX_FLAGS='-fsanitize=address -fno-omit-frame-pointer'` |
| UBSan | Integer overflow, null deref, shift overflow | `-DCMAKE_CXX_FLAGS='-fsanitize=undefined'` |
| TSan | Data races | `-DCMAKE_CXX_FLAGS='-fsanitize=thread'` |

**Note:** iora_sdp is single-threaded, but TSan validates thread-safe usage with separate
`SessionDescription` instances in different threads.

---

## 13. RFC Reference

### Core SDP RFCs

| RFC | Title | Coverage |
|-----|-------|----------|
| RFC 4566 | SDP: Session Description Protocol | Full parsing/serialization of all fields |
| RFC 8866 | SDP: Session Description Protocol (obsoletes 4566) | Same wire format, deprecates `k=` |
| RFC 3264 | An Offer/Answer Model with SDP | Direction semantics, offer/answer utilities |
| RFC 3551 | RTP Profile for Audio and Video Conferences | rtpmap encoding names, clock rates |

### ICE RFCs

| RFC | Title | Coverage |
|-----|-------|----------|
| RFC 8445 | ICE: A Protocol for NAT Traversal | Candidate parsing, all fields |
| RFC 6544 | TCP Candidates with ICE | `tcptype` attribute, `IceTransportType::TCP` |

### DTLS and SRTP RFCs

| RFC | Title | Coverage |
|-----|-------|----------|
| RFC 4572 | Connection-Oriented Media Transport over TLS | `fingerprint` attribute |
| RFC 4145 | TCP-Based Media Transport | `setup` attribute (active/passive/actpass/holdconn) |
| RFC 4568 | SDP Security Descriptions for Media Streams | `crypto` attribute (SDES-SRTP) |
| RFC 3711 | SRTP Protocol | Referenced by crypto suites (no implementation) |

### RTP Extension RFCs

| RFC | Title | Coverage |
|-----|-------|----------|
| RFC 5761 | Multiplexing RTP and RTCP on Single Port | `rtcp-mux` |
| RFC 8858 | Exclusive rtcp-mux Support | `rtcp-mux-only` |
| RFC 5888 | SDP Grouping Framework | `group` attribute (BUNDLE/LS/FID) |
| RFC 3605 | RTCP Attribute in SDP | `rtcp` port/address |
| RFC 4585 | Extended RTP Profile for RTCP-Based Feedback | `rtcp-fb` |
| RFC 5576 | Source-Specific Media Attributes | `ssrc`, `ssrc-group` |
| RFC 5285 / RFC 8285 | RTP Header Extensions | `extmap`, `extmap-allow-mixed` |
| RFC 5506 | Reduced-Size RTCP | `rtcp-rsize` |
| RFC 3890 | Bandwidth Modifier for TIAS | `TIAS` bandwidth type |
| RFC 3556 | RTCP Bandwidth Modifiers RS/RR | `RS`, `RR` bandwidth types |

### WebRTC RFCs

| RFC | Title | Coverage |
|-----|-------|----------|
| RFC 8829 | JSEP | SDP conventions for WebRTC, BUNDLE, rtcp-mux, DTLS-SRTP |
| RFC 8830 | WebRTC MediaStream Identification | `msid` attribute |
| RFC 8841 | SDP Offer/Answer for SCTP over DTLS | `sctp-port`, `max-message-size`, data channels |
| RFC 8843 | Negotiating Media Multiplexing | BUNDLE grouping |

### Simulcast RFCs

| RFC | Title | Coverage |
|-----|-------|----------|
| RFC 8853 | Using Simulcast in SDP and RTP Sessions | `simulcast` attribute |
| RFC 8851 | RID: RTP Stream Identifier | `rid` attribute, `RidDirection` |

---

## 14. API Quick Reference

### Enums

| Enum | Values | Header |
|------|--------|--------|
| `MediaType` | `AUDIO`, `VIDEO`, `APPLICATION`, `TEXT`, `MESSAGE` | `types.hpp` |
| `TransportProtocol` | `UDP`, `RTP_AVP`, `RTP_SAVP`, `RTP_SAVPF`, `UDP_TLS_RTP_SAVPF`, `DTLS_SCTP`, `UDP_DTLS_SCTP`, `TCP_DTLS_SCTP` | `types.hpp` |
| `NetworkType` | `IN` | `types.hpp` |
| `AddressType` | `IP4`, `IP6` | `types.hpp` |
| `Direction` | `SENDRECV`, `SENDONLY`, `RECVONLY`, `INACTIVE` | `types.hpp` |
| `SetupRole` | `ACTIVE`, `PASSIVE`, `ACTPASS`, `HOLDCONN` | `types.hpp` |
| `BandwidthType` | `CT`, `AS`, `TIAS`, `RS`, `RR` | `types.hpp` |
| `IceCandidateType` | `HOST`, `SRFLX`, `PRFLX`, `RELAY` | `types.hpp` |
| `IceTransportType` | `UDP`, `TCP` | `types.hpp` |
| `RidDirection` | `SEND`, `RECV` | `types.hpp` |

### Structs -- Layer 1 Data Model

| Struct | Header | Fields | Description |
|--------|--------|--------|-------------|
| `Origin` | `session_description.hpp` | 6 | SDP `o=` line |
| `ConnectionData` | `session_description.hpp` | 5 | SDP `c=` line |
| `BandwidthInfo` | `session_description.hpp` | 2 | SDP `b=` line |
| `TimeDescription` | `session_description.hpp` | 3 | SDP `t=` line with repeat times |
| `RepeatTime` | `session_description.hpp` | 3 | SDP `r=` line |
| `ZoneAdjustment` | `session_description.hpp` | 2 | SDP `z=` line entry |
| `MediaDescription` | `session_description.hpp` | 38 | Complete `m=` section |
| `SessionDescription` | `session_description.hpp` | 22 | Complete SDP message |

### Structs -- Layer 1 Attributes

| Struct | Header | SDP Attribute | RFC |
|--------|--------|---------------|-----|
| `RtpMapAttribute` | `attributes.hpp` | `a=rtpmap` | RFC 4566 |
| `FmtpAttribute` | `attributes.hpp` | `a=fmtp` | RFC 4566 |
| `RtcpFbAttribute` | `attributes.hpp` | `a=rtcp-fb` | RFC 4585 |
| `ExtMapAttribute` | `attributes.hpp` | `a=extmap` | RFC 5285/8285 |
| `SsrcAttribute` | `attributes.hpp` | `a=ssrc` | RFC 5576 |
| `SsrcGroupAttribute` | `attributes.hpp` | `a=ssrc-group` | RFC 5576 |
| `IceCandidate` | `attributes.hpp` | `a=candidate` | RFC 8445 |
| `FingerprintAttribute` | `attributes.hpp` | `a=fingerprint` | RFC 4572 |
| `CryptoAttribute` | `attributes.hpp` | `a=crypto` | RFC 4568 |
| `RtcpAttribute` | `attributes.hpp` | `a=rtcp` | RFC 3605 |
| `MsidAttribute` | `attributes.hpp` | `a=msid` | RFC 8830 |
| `GroupAttribute` | `attributes.hpp` | `a=group` | RFC 5888 |
| `SimulcastStream` | `attributes.hpp` | (part of simulcast) | RFC 8853 |
| `SimulcastAttribute` | `attributes.hpp` | `a=simulcast` | RFC 8853 |
| `RidAttribute` | `attributes.hpp` | `a=rid` | RFC 8851 |
| `GenericAttribute` | `attributes.hpp` | (any unrecognized) | — |

### Structs -- Parser/Serializer

| Struct | Header | Purpose |
|--------|--------|---------|
| `ParseError` | `sdp_parser.hpp` | Fatal parse error with line number |
| `ParseWarning` | `sdp_parser.hpp` | Non-fatal parse warning with line number |
| `ParseResult<T>` | `sdp_parser.hpp` | Parse result container (value + error + warnings) |
| `SdpParserOptions` | `sdp_parser.hpp` | Parser configuration (7 fields) |
| `SdpSerializerOptions` | `sdp_serializer.hpp` | Serializer configuration (3 fields) |

### Structs -- Offer/Answer Options

| Struct | Header | Purpose | Key Defaults |
|--------|--------|---------|--------------|
| `OfferOptions` | `offer_answer.hpp` | Configure `createOffer()` | audio=true, video=true, bundlePolicy=true, dtlsSetup=ACTPASS |
| `AnswerOptions` | `offer_answer.hpp` | Configure `createAnswer()` | dtlsSetup=ACTIVE |
| `TransceiverOptions` | `offer_answer.hpp` | Configure `addTransceiver()` | protocol=UDP_TLS_RTP_SAVPF, direction=SENDRECV, setup=ACTPASS |
| `DataChannelOptions` | `offer_answer.hpp` | Configure `addDataChannel()` | sctpPort=5000, maxMessageSize=262144, setup=ACTPASS |

### Static Classes

**`SdpParser`** (`sdp_parser.hpp`):

| Method | Signature |
|--------|-----------|
| `parse` | `ParseResult<SessionDescription> parse(std::string_view sdpText)` |
| `parse` | `ParseResult<SessionDescription> parse(std::string_view sdpText, const SdpParserOptions& options)` |
| `parseMediaSection` | `ParseResult<MediaDescription> parseMediaSection(std::string_view text)` |
| `parseAttribute` | `GenericAttribute parseAttribute(std::string_view line)` |

**`SdpSerializer`** (`sdp_serializer.hpp`):

| Method | Signature |
|--------|-----------|
| `serialize` | `std::string serialize(const SessionDescription& session)` |
| `serialize` | `std::string serialize(const SessionDescription& session, const SdpSerializerOptions& options)` |
| `serializeMediaSection` | `std::string serializeMediaSection(const MediaDescription& media)` |
| `serializeMediaSection` | `std::string serializeMediaSection(const MediaDescription& media, const SdpSerializerOptions& options)` |
| `serializeAttribute` | `std::string serializeAttribute(const GenericAttribute& attr)` |

**`OfferAnswer`** (`offer_answer.hpp`):

| Method | Signature |
|--------|-----------|
| `flipDirection` | `Direction flipDirection(Direction direction) noexcept` |
| `setMediaDirection` | `void setMediaDirection(MediaDescription& media, Direction direction) noexcept` |
| `disableMedia` | `void disableMedia(MediaDescription& media) noexcept` |
| `isMediaDisabled` | `bool isMediaDisabled(const MediaDescription& media) noexcept` |
| `findMediaByMid` | `MediaDescription* findMediaByMid(SessionDescription& session, const std::string& mid) noexcept` |
| `findMediaByMid` | `const MediaDescription* findMediaByMid(const SessionDescription& session, const std::string& mid) noexcept` |
| `generateIceCredentials` | `std::pair<std::string, std::string> generateIceCredentials()` |
| `addIceCandidate` | `void addIceCandidate(MediaDescription& media, const IceCandidate& candidate)` |
| `removeIceCandidates` | `void removeIceCandidates(MediaDescription& media) noexcept` |
| `matchCodecs` | `std::vector<RtpMapAttribute> matchCodecs(const MediaDescription& offer, const MediaDescription& answer)` |
| `pruneUnusedCodecs` | `void pruneUnusedCodecs(MediaDescription& media, const std::vector<std::uint8_t>& retainPayloadTypes)` |
| `addTransceiver` | `MediaDescription addTransceiver(MediaType type, const TransceiverOptions& options)` |
| `addDataChannel` | `MediaDescription addDataChannel(const DataChannelOptions& options)` |
| `createOffer` | `SessionDescription createOffer(const OfferOptions& options = {})` |
| `createAnswer` | `SessionDescription createAnswer(const SessionDescription& offer, const AnswerOptions& answerOptions = {})` |

### Free Functions (Conversion)

All `toString()` overloads return `std::string_view` and are `noexcept`:

| Function | Input Type |
|----------|------------|
| `toString(MediaType)` | `MediaType` |
| `toString(TransportProtocol)` | `TransportProtocol` |
| `toString(NetworkType)` | `NetworkType` |
| `toString(AddressType)` | `AddressType` |
| `toString(Direction)` | `Direction` |
| `toString(SetupRole)` | `SetupRole` |
| `toString(BandwidthType)` | `BandwidthType` |
| `toString(IceCandidateType)` | `IceCandidateType` |
| `toString(IceTransportType)` | `IceTransportType` |
| `toString(RidDirection)` | `RidDirection` |

All `fromString()` functions return `std::optional` (`std::nullopt` on failure) and are **case-insensitive**:

| Function | Return Type |
|----------|-------------|
| `mediaTypeFromString(std::string_view)` | `std::optional<MediaType>` |
| `transportProtocolFromString(std::string_view)` | `std::optional<TransportProtocol>` |
| `networkTypeFromString(std::string_view)` | `std::optional<NetworkType>` |
| `addressTypeFromString(std::string_view)` | `std::optional<AddressType>` |
| `directionFromString(std::string_view)` | `std::optional<Direction>` |
| `setupRoleFromString(std::string_view)` | `std::optional<SetupRole>` |
| `bandwidthTypeFromString(std::string_view)` | `std::optional<BandwidthType>` |
| `iceCandidateTypeFromString(std::string_view)` | `std::optional<IceCandidateType>` |
| `iceTransportTypeFromString(std::string_view)` | `std::optional<IceTransportType>` |
| `ridDirectionFromString(std::string_view)` | `std::optional<RidDirection>` |

### Constants (AttributeConstants)

All 35 `static constexpr std::string_view` members, organized by category:

| Category | Constants |
|----------|-----------|
| Codec | `kRtpmap`, `kFmtp` |
| RTCP | `kRtcp`, `kRtcpMux`, `kRtcpMuxOnly`, `kRtcpRsize`, `kRtcpFb` |
| Extensions | `kExtmap`, `kExtmapAllowMixed` |
| SSRC | `kSsrc`, `kSsrcGroup` |
| Identity | `kMid`, `kMsid` |
| Grouping | `kGroup` |
| ICE | `kIceUfrag`, `kIcePwd`, `kIceOptions`, `kIceLite`, `kCandidate`, `kEndOfCandidates` |
| DTLS/SRTP | `kFingerprint`, `kSetup`, `kCrypto` |
| Direction | `kSendrecv`, `kSendonly`, `kRecvonly`, `kInactive` |
| Data Channel | `kSctpmap`, `kSctpPort`, `kMaxMessageSize` |
| Simulcast | `kSimulcast`, `kRid` |
| Media Timing | `kFramerate`, `kPtime`, `kMaxptime` |

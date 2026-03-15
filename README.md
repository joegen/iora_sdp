# iora_sdp

Standalone, header-only C++17 SDP parser/serializer with WebRTC offer/answer utilities. Built on the [Iora](https://github.com/joegen/iora) framework conventions. Zero external dependencies — pure C++17 standard library.

## Features

- **Full SDP parsing** -- RFC 4566/8866 compliant with lenient and strict modes
- **Typed attribute model** -- 16 strongly-typed attribute structs (rtpmap, fmtp, candidate, fingerprint, etc.)
- **Round-trip fidelity** -- `parse(serialize(parse(input))) == parse(input)` guaranteed
- **WebRTC support** -- BUNDLE, ICE, DTLS, rtcp-mux, simulcast, data channels
- **SIP support** -- RTP/AVP, SDES-SRTP crypto, plain RTP profiles
- **Offer/answer utilities** -- stateless building blocks for SDP negotiation
- **Unknown attribute preservation** -- proprietary extensions survive round trips via GenericAttribute
- **DoS prevention** -- configurable line length and media section limits

## Architecture

```
Level 4  Offer/Answer    OfferAnswer (createOffer, createAnswer, matchCodecs, pruneUnusedCodecs)
Level 3  Serializer      SdpSerializer (serialize, serializeMediaSection, serializeAttribute)
Level 2  Parser          SdpParser (parse, parseMediaSection, parseAttribute)
Level 1  Data Model      SessionDescription, MediaDescription, 16 typed attribute structs
Level 0  Types           10 enum classes, AttributeConstants (35 members), conversion functions
```

## Building

### Prerequisites

- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.20+

### Quick Build

```bash
cmake -S . -B build
cmake --build build -j$(nproc)
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `IORA_SDP_BUILD_TESTS` | ON | Build test executables |
| `IORA_SDP_BUILD_FUZZ` | OFF | Build fuzz testing targets (requires clang + libFuzzer) |

## Testing

```bash
# Run all tests via CTest
cd build && ctest --output-on-failure

# Run directly
./build/tests/sdp_tests
```

### Fuzz Testing

```bash
cmake -S . -B build-fuzz \
  -DIORA_SDP_BUILD_FUZZ=ON \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_CXX_FLAGS='-fsanitize=fuzzer,address'
cmake --build build-fuzz
./build-fuzz/fuzz_parser tests/fuzz_corpus/ -dict=tests/fuzz_parser.dict
```

## Project Structure

```
iora_sdp/
  include/iora/sdp/
    types.hpp                 Layer 0: enums, constants, conversion functions
    attributes.hpp            Layer 1: typed attribute structs
    session_description.hpp   Layer 1: SessionDescription, MediaDescription
    sdp_parser.hpp            Layer 2: SDP text parser
    sdp_serializer.hpp        Layer 3: SDP text serializer
    offer_answer.hpp          Layer 4: offer/answer utilities
  tests/                      Unit tests, WebRTC SDP tests, fuzz harness
  docs/                       Architecture doc and programmer's manual
  CMakeLists.txt
```

## Quick Start

```cpp
#include "iora/sdp/sdp_parser.hpp"
#include "iora/sdp/sdp_serializer.hpp"
#include <iostream>

using namespace iora::sdp;

int main()
{
  std::string sdpText =
      "v=0\r\n"
      "o=- 123 1 IN IP4 127.0.0.1\r\n"
      "s=-\r\n"
      "t=0 0\r\n"
      "a=group:BUNDLE 0\r\n"
      "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n"
      "a=mid:0\r\n"
      "a=sendrecv\r\n"
      "a=rtcp-mux\r\n"
      "a=rtpmap:111 opus/48000/2\r\n"
      "a=fmtp:111 minptime=10;useinbandfec=1\r\n";

  auto result = SdpParser::parse(sdpText);
  if (result.hasValue())
  {
    auto& session = *result.value;
    for (const auto& media : session.mediaDescriptions)
    {
      std::cout << toString(media.mediaType) << " ("
                << toString(media.protocol) << ")\n";
      for (const auto& codec : media.rtpMaps)
      {
        std::cout << "  " << codec.encodingName << "/"
                  << codec.clockRate << "\n";
      }
    }

    // Round-trip: serialize and re-parse
    auto serialized = SdpSerializer::serialize(session);
    auto r2 = SdpParser::parse(serialized);
    assert(*r2.value == session);
  }
}
```

### CMake Integration

```cmake
# Header-only — just add the include path
add_subdirectory(third_party/iora_sdp)
target_link_libraries(myapp PRIVATE iora_sdp)
```

## Documentation

- **[Programmer's Manual](docs/programmers_manual.md)** -- complete API reference with usage examples
- **[Architecture](docs/architecture.json)** -- design document with layer descriptions

## License

This project is licensed under the [Mozilla Public License 2.0](LICENSE).

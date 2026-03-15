// Fuzz testing harness for SdpParser::parse().
// Compatible with libFuzzer, AFL persistent mode, and standalone stdin testing.

#include <cstddef>
#include <cstdint>
#include <string>

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

// ---------------------------------------------------------------------------
// libFuzzer entry point
// ---------------------------------------------------------------------------
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  try
  {
    fuzzOne(data, size);
  }
  catch (...)
  {
  }
  return 0;
}

// ---------------------------------------------------------------------------
// AFL persistent mode
// ---------------------------------------------------------------------------
#ifdef __AFL_FUZZ_TESTCASE_LEN

__AFL_FUZZ_INIT();

int main()
{
  __AFL_INIT();
  unsigned char* buf = __AFL_FUZZ_TESTCASE_BUF;

  while (__AFL_LOOP(10000))
  {
    int len = __AFL_FUZZ_TESTCASE_LEN;
    try
    {
      fuzzOne(buf, static_cast<size_t>(len));
    }
    catch (...)
    {
    }
  }
  return 0;
}

// ---------------------------------------------------------------------------
// Standalone stdin fallback (no fuzzer engine)
// ---------------------------------------------------------------------------
#elif !defined(__LIBFUZZER__) && !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)

#include <iostream>
#include <iterator>

int main()
{
  std::string input(
    (std::istreambuf_iterator<char>(std::cin)),
    std::istreambuf_iterator<char>());
  try
  {
    fuzzOne(reinterpret_cast<const uint8_t*>(input.data()), input.size());
  }
  catch (...)
  {
  }
  return 0;
}

#endif

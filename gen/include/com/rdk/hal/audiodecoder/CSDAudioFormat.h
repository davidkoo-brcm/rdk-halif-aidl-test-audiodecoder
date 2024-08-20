#pragma once

#include <array>
#include <binder/Enums.h>
#include <cstdint>
#include <string>

namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
enum class CSDAudioFormat : int32_t {
  AAC_AUDIO_SPECIFIC_CONFIG = 0,
  ALAC_xxx = 1,
};
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com
namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
[[nodiscard]] static inline std::string toString(CSDAudioFormat val) {
  switch(val) {
  case CSDAudioFormat::AAC_AUDIO_SPECIFIC_CONFIG:
    return "AAC_AUDIO_SPECIFIC_CONFIG";
  case CSDAudioFormat::ALAC_xxx:
    return "ALAC_xxx";
  default:
    return std::to_string(static_cast<int32_t>(val));
  }
}
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com
namespace android {
namespace internal {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"
template <>
constexpr inline std::array<::com::rdk::hal::audiodecoder::CSDAudioFormat, 2> enum_values<::com::rdk::hal::audiodecoder::CSDAudioFormat> = {
  ::com::rdk::hal::audiodecoder::CSDAudioFormat::AAC_AUDIO_SPECIFIC_CONFIG,
  ::com::rdk::hal::audiodecoder::CSDAudioFormat::ALAC_xxx,
};
#pragma clang diagnostic pop
}  // namespace internal
}  // namespace android

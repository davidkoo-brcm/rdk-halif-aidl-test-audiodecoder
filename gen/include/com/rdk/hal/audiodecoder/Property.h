#pragma once

#include <array>
#include <binder/Enums.h>
#include <cstdint>
#include <string>

namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
enum class Property : int32_t {
  RESOURCE_ID = 0,
  LOW_LATENCY_MODE = 1,
  AV_SOURCE = 3,
  SECURE_AUDIO = 4,
  AC4_PRESENTATION_GROUP_INDEX = 200,
  AC4_PREFERRED_LANG1 = 201,
  AC4_PREFERRED_LANG2 = 202,
  AC4_ASSOCIATED_TYPE = 203,
  AC4_AUTO_SELECTION_PRIORITY = 204,
  AC4_MIXER_BALANCE = 205,
  AC4_ASSOCIATED_AUDIO_MIXING_ENABLE = 206,
  METRIC_xxxx = 1000,
};
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com
namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
[[nodiscard]] static inline std::string toString(Property val) {
  switch(val) {
  case Property::RESOURCE_ID:
    return "RESOURCE_ID";
  case Property::LOW_LATENCY_MODE:
    return "LOW_LATENCY_MODE";
  case Property::AV_SOURCE:
    return "AV_SOURCE";
  case Property::SECURE_AUDIO:
    return "SECURE_AUDIO";
  case Property::AC4_PRESENTATION_GROUP_INDEX:
    return "AC4_PRESENTATION_GROUP_INDEX";
  case Property::AC4_PREFERRED_LANG1:
    return "AC4_PREFERRED_LANG1";
  case Property::AC4_PREFERRED_LANG2:
    return "AC4_PREFERRED_LANG2";
  case Property::AC4_ASSOCIATED_TYPE:
    return "AC4_ASSOCIATED_TYPE";
  case Property::AC4_AUTO_SELECTION_PRIORITY:
    return "AC4_AUTO_SELECTION_PRIORITY";
  case Property::AC4_MIXER_BALANCE:
    return "AC4_MIXER_BALANCE";
  case Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE:
    return "AC4_ASSOCIATED_AUDIO_MIXING_ENABLE";
  case Property::METRIC_xxxx:
    return "METRIC_xxxx";
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
constexpr inline std::array<::com::rdk::hal::audiodecoder::Property, 12> enum_values<::com::rdk::hal::audiodecoder::Property> = {
  ::com::rdk::hal::audiodecoder::Property::RESOURCE_ID,
  ::com::rdk::hal::audiodecoder::Property::LOW_LATENCY_MODE,
  ::com::rdk::hal::audiodecoder::Property::AV_SOURCE,
  ::com::rdk::hal::audiodecoder::Property::SECURE_AUDIO,
  ::com::rdk::hal::audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX,
  ::com::rdk::hal::audiodecoder::Property::AC4_PREFERRED_LANG1,
  ::com::rdk::hal::audiodecoder::Property::AC4_PREFERRED_LANG2,
  ::com::rdk::hal::audiodecoder::Property::AC4_ASSOCIATED_TYPE,
  ::com::rdk::hal::audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY,
  ::com::rdk::hal::audiodecoder::Property::AC4_MIXER_BALANCE,
  ::com::rdk::hal::audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE,
  ::com::rdk::hal::audiodecoder::Property::METRIC_xxxx,
};
#pragma clang diagnostic pop
}  // namespace internal
}  // namespace android

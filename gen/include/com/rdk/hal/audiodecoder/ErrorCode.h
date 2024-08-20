#pragma once

#include <array>
#include <binder/Enums.h>
#include <cstdint>
#include <string>

namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
enum class ErrorCode : int32_t {
  xxx = 1,
};
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com
namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
[[nodiscard]] static inline std::string toString(ErrorCode val) {
  switch(val) {
  case ErrorCode::xxx:
    return "xxx";
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
constexpr inline std::array<::com::rdk::hal::audiodecoder::ErrorCode, 1> enum_values<::com::rdk::hal::audiodecoder::ErrorCode> = {
  ::com::rdk::hal::audiodecoder::ErrorCode::xxx,
};
#pragma clang diagnostic pop
}  // namespace internal
}  // namespace android

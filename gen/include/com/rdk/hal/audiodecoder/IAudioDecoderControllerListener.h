#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <com/rdk/hal/audiodecoder/FrameMetadata.h>
#include <cstdint>
#include <optional>
#include <utils/StrongPointer.h>

namespace com::rdk::hal::audiodecoder {
class FrameMetadata;
}  // namespace com::rdk::hal::audiodecoder
namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
class IAudioDecoderControllerListenerDelegator;

class IAudioDecoderControllerListener : public ::android::IInterface {
public:
  typedef IAudioDecoderControllerListenerDelegator DefaultDelegator;
  DECLARE_META_INTERFACE(AudioDecoderControllerListener)
  virtual ::android::binder::Status onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::std::optional<::com::rdk::hal::audiodecoder::FrameMetadata>& metadata) = 0;
};  // class IAudioDecoderControllerListener

class IAudioDecoderControllerListenerDefault : public IAudioDecoderControllerListener {
public:
  ::android::IBinder* onAsBinder() override {
    return nullptr;
  }
  ::android::binder::Status onFrameOutput(int64_t /*nsPresentationTime*/, int64_t /*frameBufferHandle*/, const ::std::optional<::com::rdk::hal::audiodecoder::FrameMetadata>& /*metadata*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
};  // class IAudioDecoderControllerListenerDefault
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com

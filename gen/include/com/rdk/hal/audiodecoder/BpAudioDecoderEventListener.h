#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <com/rdk/hal/audiodecoder/IAudioDecoderEventListener.h>

namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
class BpAudioDecoderEventListener : public ::android::BpInterface<IAudioDecoderEventListener> {
public:
  explicit BpAudioDecoderEventListener(const ::android::sp<::android::IBinder>& _aidl_impl);
  virtual ~BpAudioDecoderEventListener() = default;
  ::android::binder::Status onDecodeError(::com::rdk::hal::audiodecoder::ErrorCode errorCode, int32_t vendorErrorCode) override;
  ::android::binder::Status onStateChanged(::com::rdk::hal::State oldState, ::com::rdk::hal::State newState) override;
};  // class BpAudioDecoderEventListener
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com

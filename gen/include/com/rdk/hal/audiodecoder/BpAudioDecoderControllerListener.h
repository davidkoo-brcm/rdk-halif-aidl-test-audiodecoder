#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <com/rdk/hal/audiodecoder/IAudioDecoderControllerListener.h>

namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
class BpAudioDecoderControllerListener : public ::android::BpInterface<IAudioDecoderControllerListener> {
public:
  explicit BpAudioDecoderControllerListener(const ::android::sp<::android::IBinder>& _aidl_impl);
  virtual ~BpAudioDecoderControllerListener() = default;
  ::android::binder::Status onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::std::optional<::com::rdk::hal::audiodecoder::FrameMetadata>& metadata) override;
};  // class BpAudioDecoderControllerListener
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com

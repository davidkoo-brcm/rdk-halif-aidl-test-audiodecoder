#pragma once

#include <binder/IInterface.h>
#include <com/rdk/hal/audiodecoder/IAudioDecoderControllerListener.h>
//#include <binder/Delegate.h>
#include <com/rdk/hal/audiodecoder/BnAudioDecoderControllerListener.h>


namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
class BnAudioDecoderControllerListener : public ::android::BnInterface<IAudioDecoderControllerListener> {
public:
  static constexpr uint32_t TRANSACTION_onFrameOutput = ::android::IBinder::FIRST_CALL_TRANSACTION + 0;
  explicit BnAudioDecoderControllerListener();
  ::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) override;
};  // class BnAudioDecoderControllerListener

class IAudioDecoderControllerListenerDelegator : public BnAudioDecoderControllerListener {
public:
  explicit IAudioDecoderControllerListenerDelegator(const ::android::sp<IAudioDecoderControllerListener> &impl) : _aidl_delegate(impl) {}

  ::android::sp<IAudioDecoderControllerListener> getImpl() { return _aidl_delegate; }
  ::android::binder::Status onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::std::optional<::com::rdk::hal::audiodecoder::FrameMetadata>& metadata) override {
    return _aidl_delegate->onFrameOutput(nsPresentationTime, frameBufferHandle, metadata);
  }
private:
  ::android::sp<IAudioDecoderControllerListener> _aidl_delegate;
};  // class IAudioDecoderControllerListenerDelegator
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com

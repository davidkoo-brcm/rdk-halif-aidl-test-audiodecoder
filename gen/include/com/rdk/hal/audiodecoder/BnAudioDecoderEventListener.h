#pragma once

#include <binder/IInterface.h>
#include <com/rdk/hal/audiodecoder/IAudioDecoderEventListener.h>
//#include <binder/Delegate.h>
#include <com/rdk/hal/audiodecoder/BnAudioDecoderEventListener.h>


namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
class BnAudioDecoderEventListener : public ::android::BnInterface<IAudioDecoderEventListener> {
public:
  static constexpr uint32_t TRANSACTION_onDecodeError = ::android::IBinder::FIRST_CALL_TRANSACTION + 0;
  static constexpr uint32_t TRANSACTION_onStateChanged = ::android::IBinder::FIRST_CALL_TRANSACTION + 1;
  explicit BnAudioDecoderEventListener();
  ::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) override;
};  // class BnAudioDecoderEventListener

class IAudioDecoderEventListenerDelegator : public BnAudioDecoderEventListener {
public:
  explicit IAudioDecoderEventListenerDelegator(const ::android::sp<IAudioDecoderEventListener> &impl) : _aidl_delegate(impl) {}

  ::android::sp<IAudioDecoderEventListener> getImpl() { return _aidl_delegate; }
  ::android::binder::Status onDecodeError(::com::rdk::hal::audiodecoder::ErrorCode errorCode, int32_t vendorErrorCode) override {
    return _aidl_delegate->onDecodeError(errorCode, vendorErrorCode);
  }
  ::android::binder::Status onStateChanged(::com::rdk::hal::State oldState, ::com::rdk::hal::State newState) override {
    return _aidl_delegate->onStateChanged(oldState, newState);
  }
private:
  ::android::sp<IAudioDecoderEventListener> _aidl_delegate;
};  // class IAudioDecoderEventListenerDelegator
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com

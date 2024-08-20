#pragma once

#include <binder/IInterface.h>
#include <com/rdk/hal/audiodecoder/IAudioDecoderManager.h>
//#include <binder/Delegate.h>
#include <com/rdk/hal/audiodecoder/BnAudioDecoder.h>
#include <com/rdk/hal/audiodecoder/BnAudioDecoderManager.h>


namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
class BnAudioDecoderManager : public ::android::BnInterface<IAudioDecoderManager> {
public:
  static constexpr uint32_t TRANSACTION_getAudioDecoderIds = ::android::IBinder::FIRST_CALL_TRANSACTION + 0;
  static constexpr uint32_t TRANSACTION_getAudioDecoder = ::android::IBinder::FIRST_CALL_TRANSACTION + 1;
  explicit BnAudioDecoderManager();
  ::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) override;
};  // class BnAudioDecoderManager

class IAudioDecoderManagerDelegator : public BnAudioDecoderManager {
public:
  explicit IAudioDecoderManagerDelegator(const ::android::sp<IAudioDecoderManager> &impl) : _aidl_delegate(impl) {}

  ::android::sp<IAudioDecoderManager> getImpl() { return _aidl_delegate; }
  ::android::binder::Status getAudioDecoderIds(::std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id>* _aidl_return) override {
    return _aidl_delegate->getAudioDecoderIds(_aidl_return);
  }
  ::android::binder::Status getAudioDecoder(const ::com::rdk::hal::audiodecoder::IAudioDecoder::Id& decoderResourceId, ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder>* _aidl_return) override {
    auto _status = _aidl_delegate->getAudioDecoder(decoderResourceId, _aidl_return);
    /* (*_aidl_return) {
      *_aidl_return = ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderDelegator>::cast(delegate(*_aidl_return));
    }*/
    return _status;
  }
private:
  ::android::sp<IAudioDecoderManager> _aidl_delegate;
};  // class IAudioDecoderManagerDelegator
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com

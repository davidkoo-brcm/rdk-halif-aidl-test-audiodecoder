#pragma once

#include <binder/IInterface.h>
#include <com/rdk/hal/audiodecoder/IAudioDecoder.h>
//#include <binder/Delegate.h>
#include <com/rdk/hal/audiodecoder/BnAudioDecoder.h>
#include <com/rdk/hal/audiodecoder/BnAudioDecoderController.h>
#include <com/rdk/hal/audiodecoder/BnAudioDecoderControllerListener.h>
#include <com/rdk/hal/audiodecoder/BnAudioDecoderEventListener.h>


namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
class BnAudioDecoder : public ::android::BnInterface<IAudioDecoder> {
public:
  static constexpr uint32_t TRANSACTION_getCapabilities = ::android::IBinder::FIRST_CALL_TRANSACTION + 0;
  static constexpr uint32_t TRANSACTION_getProperty = ::android::IBinder::FIRST_CALL_TRANSACTION + 1;
  static constexpr uint32_t TRANSACTION_setProperty = ::android::IBinder::FIRST_CALL_TRANSACTION + 2;
  static constexpr uint32_t TRANSACTION_getState = ::android::IBinder::FIRST_CALL_TRANSACTION + 3;
  static constexpr uint32_t TRANSACTION_open = ::android::IBinder::FIRST_CALL_TRANSACTION + 4;
  static constexpr uint32_t TRANSACTION_close = ::android::IBinder::FIRST_CALL_TRANSACTION + 5;
  static constexpr uint32_t TRANSACTION_registerEventListener = ::android::IBinder::FIRST_CALL_TRANSACTION + 6;
  static constexpr uint32_t TRANSACTION_unregisterEventListener = ::android::IBinder::FIRST_CALL_TRANSACTION + 7;
  explicit BnAudioDecoder();
  ::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) override;
};  // class BnAudioDecoder

class IAudioDecoderDelegator : public BnAudioDecoder {
public:
  explicit IAudioDecoderDelegator(const ::android::sp<IAudioDecoder> &impl) : _aidl_delegate(impl) {}

  ::android::sp<IAudioDecoder> getImpl() { return _aidl_delegate; }
  ::android::binder::Status getCapabilities(::com::rdk::hal::audiodecoder::Capabilities* _aidl_return) override {
    return _aidl_delegate->getCapabilities(_aidl_return);
  }
  ::android::binder::Status getProperty(::com::rdk::hal::audiodecoder::Property property, ::std::optional<::com::rdk::hal::PropertyValue>* _aidl_return) override {
    return _aidl_delegate->getProperty(property, _aidl_return);
  }
  ::android::binder::Status setProperty(::com::rdk::hal::audiodecoder::Property property, const ::com::rdk::hal::PropertyValue& propertyValue, bool* _aidl_return) override {
    return _aidl_delegate->setProperty(property, propertyValue, _aidl_return);
  }
  ::android::binder::Status getState(::com::rdk::hal::State* _aidl_return) override {
    return _aidl_delegate->getState(_aidl_return);
  }
  ::android::binder::Status open(::com::rdk::hal::audiodecoder::Codec codec, bool secure, const ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderControllerListener>& audioDecoderControllerListener, ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderController>* _aidl_return) override {
    ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderControllerListenerDelegator> _audioDecoderControllerListener;
    /*if (audioDecoderControllerListener) {
      _audioDecoderControllerListener = ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderControllerListenerDelegator>::cast(delegate(audioDecoderControllerListener));
    }*/
    auto _status = _aidl_delegate->open(codec, secure, _audioDecoderControllerListener, _aidl_return);
    /*if (*_aidl_return) {
      *_aidl_return = ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderControllerDelegator>::cast(delegate(*_aidl_return));
    }*/
    return _status;
  }
  ::android::binder::Status close(const ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderController>& audioDecoderController, bool* _aidl_return) override {
    ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderControllerDelegator> _audioDecoderController;
    /*if (audioDecoderController) {
      _audioDecoderController = ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderControllerDelegator>::cast(delegate(audioDecoderController));
    }*/
    return _aidl_delegate->close(_audioDecoderController, _aidl_return);
  }
  ::android::binder::Status registerEventListener(const ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return) override {
    ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderEventListenerDelegator> _audioDecoderEventListener;
    /*if (audioDecoderEventListener) {
      _audioDecoderEventListener = ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderEventListenerDelegator>::cast(delegate(audioDecoderEventListener));
    }*/
    return _aidl_delegate->registerEventListener(_audioDecoderEventListener, _aidl_return);
  }
  ::android::binder::Status unregisterEventListener(const ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return) override {
    ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderEventListenerDelegator> _audioDecoderEventListener;
    /*if (audioDecoderEventListener) {
      _audioDecoderEventListener = ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderEventListenerDelegator>::cast(delegate(audioDecoderEventListener));
    }*/
    return _aidl_delegate->unregisterEventListener(_audioDecoderEventListener, _aidl_return);
  }
private:
  ::android::sp<IAudioDecoder> _aidl_delegate;
};  // class IAudioDecoderDelegator
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com

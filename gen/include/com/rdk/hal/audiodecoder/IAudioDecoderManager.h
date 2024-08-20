#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <com/rdk/hal/audiodecoder/IAudioDecoder.h>
#include <optional>
#include <string>
#include <utils/StrongPointer.h>
#include <vector>

namespace com::rdk::hal::audiodecoder {
class IAudioDecoder;
}  // namespace com::rdk::hal::audiodecoder
namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
class IAudioDecoderManagerDelegator;

class IAudioDecoderManager : public ::android::IInterface {
public:
  typedef IAudioDecoderManagerDelegator DefaultDelegator;
  DECLARE_META_INTERFACE(AudioDecoderManager)
  static const ::std::string& serviceName();
  virtual ::android::binder::Status getAudioDecoderIds(::std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id>* _aidl_return) = 0;
  virtual ::android::binder::Status getAudioDecoder(const ::com::rdk::hal::audiodecoder::IAudioDecoder::Id& decoderResourceId, ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder>* _aidl_return) = 0;
};  // class IAudioDecoderManager

class IAudioDecoderManagerDefault : public IAudioDecoderManager {
public:
  ::android::IBinder* onAsBinder() override {
    return nullptr;
  }
  ::android::binder::Status getAudioDecoderIds(::std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id>* /*_aidl_return*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status getAudioDecoder(const ::com::rdk::hal::audiodecoder::IAudioDecoder::Id& /*decoderResourceId*/, ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder>* /*_aidl_return*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
};  // class IAudioDecoderManagerDefault
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com

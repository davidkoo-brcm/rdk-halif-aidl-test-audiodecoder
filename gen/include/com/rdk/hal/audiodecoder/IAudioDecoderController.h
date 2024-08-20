#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <com/rdk/hal/audiodecoder/CSDAudioFormat.h>
#include <cstdint>
#include <utils/StrongPointer.h>
#include <vector>

namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
class IAudioDecoderControllerDelegator;

class IAudioDecoderController : public ::android::IInterface {
public:
  typedef IAudioDecoderControllerDelegator DefaultDelegator;
  DECLARE_META_INTERFACE(AudioDecoderController)
  virtual ::android::binder::Status start() = 0;
  virtual ::android::binder::Status stop() = 0;
  virtual ::android::binder::Status decodeBuffer(int64_t nsPresentationTime, int64_t bufferHandle, int32_t trimStartNs, int32_t trimEndNs, bool* _aidl_return) = 0;
  virtual ::android::binder::Status flush(bool reset) = 0;
  virtual ::android::binder::Status signalDiscontinuity() = 0;
  virtual ::android::binder::Status signalEOS() = 0;
  virtual ::android::binder::Status parseCodecSpecificData(::com::rdk::hal::audiodecoder::CSDAudioFormat csdAudioFormat, const ::std::vector<uint8_t>& codecData, bool* _aidl_return) = 0;
};  // class IAudioDecoderController

class IAudioDecoderControllerDefault : public IAudioDecoderController {
public:
  ::android::IBinder* onAsBinder() override {
    return nullptr;
  }
  ::android::binder::Status start() override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status stop() override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status decodeBuffer(int64_t /*nsPresentationTime*/, int64_t /*bufferHandle*/, int32_t /*trimStartNs*/, int32_t /*trimEndNs*/, bool* /*_aidl_return*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status flush(bool /*reset*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status signalDiscontinuity() override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status signalEOS() override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status parseCodecSpecificData(::com::rdk::hal::audiodecoder::CSDAudioFormat /*csdAudioFormat*/, const ::std::vector<uint8_t>& /*codecData*/, bool* /*_aidl_return*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
};  // class IAudioDecoderControllerDefault
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com

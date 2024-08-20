#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <com/rdk/hal/State.h>
#include <com/rdk/hal/audiodecoder/ErrorCode.h>
#include <cstdint>
#include <utils/StrongPointer.h>

namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
class IAudioDecoderEventListenerDelegator;

class IAudioDecoderEventListener : public ::android::IInterface {
public:
  typedef IAudioDecoderEventListenerDelegator DefaultDelegator;
  DECLARE_META_INTERFACE(AudioDecoderEventListener)
  virtual ::android::binder::Status onDecodeError(::com::rdk::hal::audiodecoder::ErrorCode errorCode, int32_t vendorErrorCode) = 0;
  virtual ::android::binder::Status onStateChanged(::com::rdk::hal::State oldState, ::com::rdk::hal::State newState) = 0;
};  // class IAudioDecoderEventListener

class IAudioDecoderEventListenerDefault : public IAudioDecoderEventListener {
public:
  ::android::IBinder* onAsBinder() override {
    return nullptr;
  }
  ::android::binder::Status onDecodeError(::com::rdk::hal::audiodecoder::ErrorCode /*errorCode*/, int32_t /*vendorErrorCode*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status onStateChanged(::com::rdk::hal::State /*oldState*/, ::com::rdk::hal::State /*newState*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
};  // class IAudioDecoderEventListenerDefault
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com

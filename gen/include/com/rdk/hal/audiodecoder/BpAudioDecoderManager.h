#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <com/rdk/hal/audiodecoder/IAudioDecoderManager.h>

namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
class BpAudioDecoderManager : public ::android::BpInterface<IAudioDecoderManager> {
public:
  explicit BpAudioDecoderManager(const ::android::sp<::android::IBinder>& _aidl_impl);
  virtual ~BpAudioDecoderManager() = default;
  ::android::binder::Status getAudioDecoderIds(::std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id>* _aidl_return) override;
  ::android::binder::Status getAudioDecoder(const ::com::rdk::hal::audiodecoder::IAudioDecoder::Id& decoderResourceId, ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder>* _aidl_return) override;
};  // class BpAudioDecoderManager
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com

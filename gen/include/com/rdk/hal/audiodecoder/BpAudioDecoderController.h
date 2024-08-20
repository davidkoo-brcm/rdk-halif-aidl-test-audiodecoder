#pragma once

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <com/rdk/hal/audiodecoder/IAudioDecoderController.h>

namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
class BpAudioDecoderController : public ::android::BpInterface<IAudioDecoderController> {
public:
  explicit BpAudioDecoderController(const ::android::sp<::android::IBinder>& _aidl_impl);
  virtual ~BpAudioDecoderController() = default;
  ::android::binder::Status start() override;
  ::android::binder::Status stop() override;
  ::android::binder::Status decodeBuffer(int64_t nsPresentationTime, int64_t bufferHandle, int32_t trimStartNs, int32_t trimEndNs, bool* _aidl_return) override;
  ::android::binder::Status flush(bool reset) override;
  ::android::binder::Status signalDiscontinuity() override;
  ::android::binder::Status signalEOS() override;
  ::android::binder::Status parseCodecSpecificData(::com::rdk::hal::audiodecoder::CSDAudioFormat csdAudioFormat, const ::std::vector<uint8_t>& codecData, bool* _aidl_return) override;
};  // class BpAudioDecoderController
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com

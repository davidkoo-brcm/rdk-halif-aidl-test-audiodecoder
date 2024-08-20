#pragma once

#include <android/binder_to_string.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <binder/Status.h>
#include <com/rdk/hal/PropertyValue.h>
#include <com/rdk/hal/State.h>
#include <com/rdk/hal/audiodecoder/Capabilities.h>
#include <com/rdk/hal/audiodecoder/Codec.h>
#include <com/rdk/hal/audiodecoder/IAudioDecoderController.h>
#include <com/rdk/hal/audiodecoder/IAudioDecoderControllerListener.h>
#include <com/rdk/hal/audiodecoder/IAudioDecoderEventListener.h>
#include <com/rdk/hal/audiodecoder/Property.h>
#include <cstdint>
#include <optional>
#include <tuple>
#include <utils/String16.h>
#include <utils/StrongPointer.h>

namespace com::rdk::hal {
class PropertyValue;
}  // namespace com::rdk::hal
namespace com::rdk::hal::audiodecoder {
class Capabilities;
class IAudioDecoderController;
class IAudioDecoderControllerListener;
class IAudioDecoderEventListener;
}  // namespace com::rdk::hal::audiodecoder
namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
class IAudioDecoderDelegator;

class IAudioDecoder : public ::android::IInterface {
public:
  typedef IAudioDecoderDelegator DefaultDelegator;
  DECLARE_META_INTERFACE(AudioDecoder)
  class Id : public ::android::Parcelable {
  public:
    int32_t value = 0;
    inline bool operator!=(const Id& rhs) const {
      return std::tie(value) != std::tie(rhs.value);
    }
    inline bool operator<(const Id& rhs) const {
      return std::tie(value) < std::tie(rhs.value);
    }
    inline bool operator<=(const Id& rhs) const {
      return std::tie(value) <= std::tie(rhs.value);
    }
    inline bool operator==(const Id& rhs) const {
      return std::tie(value) == std::tie(rhs.value);
    }
    inline bool operator>(const Id& rhs) const {
      return std::tie(value) > std::tie(rhs.value);
    }
    inline bool operator>=(const Id& rhs) const {
      return std::tie(value) >= std::tie(rhs.value);
    }

    enum : int32_t { UNDEFINED = -1 };
    ::android::Parcelable::Stability getStability() const override { return ::android::Parcelable::Stability::STABILITY_VINTF; }
    ::android::status_t readFromParcel(const ::android::Parcel* _aidl_parcel) final;
    ::android::status_t writeToParcel(::android::Parcel* _aidl_parcel) const final;
    static const ::android::String16& getParcelableDescriptor() {
      static const ::android::StaticString16 DESCRIPTOR (u"com.rdk.hal.audiodecoder.IAudioDecoder.Id");
      return DESCRIPTOR;
    }
    inline std::string toString() const {
      std::ostringstream os;
      os << "Id{";
      os << "value: " << ::android::internal::ToString(value);
      os << "}";
      return os.str();
    }
  };  // class Id
  virtual ::android::binder::Status getCapabilities(::com::rdk::hal::audiodecoder::Capabilities* _aidl_return) = 0;
  virtual ::android::binder::Status getProperty(::com::rdk::hal::audiodecoder::Property property, ::std::optional<::com::rdk::hal::PropertyValue>* _aidl_return) = 0;
  virtual ::android::binder::Status setProperty(::com::rdk::hal::audiodecoder::Property property, const ::com::rdk::hal::PropertyValue& propertyValue, bool* _aidl_return) = 0;
  virtual ::android::binder::Status getState(::com::rdk::hal::State* _aidl_return) = 0;
  virtual ::android::binder::Status open(::com::rdk::hal::audiodecoder::Codec codec, bool secure, const ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderControllerListener>& audioDecoderControllerListener, ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderController>* _aidl_return) = 0;
  virtual ::android::binder::Status close(const ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderController>& audioDecoderController, bool* _aidl_return) = 0;
  virtual ::android::binder::Status registerEventListener(const ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return) = 0;
  virtual ::android::binder::Status unregisterEventListener(const ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return) = 0;
};  // class IAudioDecoder

class IAudioDecoderDefault : public IAudioDecoder {
public:
  ::android::IBinder* onAsBinder() override {
    return nullptr;
  }
  ::android::binder::Status getCapabilities(::com::rdk::hal::audiodecoder::Capabilities* /*_aidl_return*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status getProperty(::com::rdk::hal::audiodecoder::Property /*property*/, ::std::optional<::com::rdk::hal::PropertyValue>* /*_aidl_return*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status setProperty(::com::rdk::hal::audiodecoder::Property /*property*/, const ::com::rdk::hal::PropertyValue& /*propertyValue*/, bool* /*_aidl_return*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status getState(::com::rdk::hal::State* /*_aidl_return*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status open(::com::rdk::hal::audiodecoder::Codec /*codec*/, bool /*secure*/, const ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderControllerListener>& /*audioDecoderControllerListener*/, ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderController>* /*_aidl_return*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status close(const ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderController>& /*audioDecoderController*/, bool* /*_aidl_return*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status registerEventListener(const ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderEventListener>& /*audioDecoderEventListener*/, bool* /*_aidl_return*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
  ::android::binder::Status unregisterEventListener(const ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderEventListener>& /*audioDecoderEventListener*/, bool* /*_aidl_return*/) override {
    return ::android::binder::Status::fromStatusT(::android::UNKNOWN_TRANSACTION);
  }
};  // class IAudioDecoderDefault
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com

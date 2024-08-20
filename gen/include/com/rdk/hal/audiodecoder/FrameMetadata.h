#pragma once

#include <android/binder_to_string.h>
#include <binder/Parcel.h>
#include <binder/ParcelableHolder.h>
#include <binder/Status.h>
#include <com/rdk/hal/AVSource.h>
#include <com/rdk/hal/audiodecoder/ChannelType.h>
#include <com/rdk/hal/audiodecoder/Codec.h>
#include <com/rdk/hal/audiodecoder/PCMFormat.h>
#include <cstdint>
#include <tuple>
#include <utils/String16.h>
#include <vector>

namespace com {
namespace rdk {
namespace hal {
namespace audiodecoder {
class FrameMetadata : public ::android::Parcelable {
public:
  ::com::rdk::hal::audiodecoder::Codec sourceCodec = ::com::rdk::hal::audiodecoder::Codec(0);
  bool isDolbyAtmos = false;
  int32_t numChannels = 0;
  ::std::vector<::com::rdk::hal::audiodecoder::ChannelType> channelTypes;
  int32_t sampleRate = 0;
  ::com::rdk::hal::audiodecoder::PCMFormat format = ::com::rdk::hal::audiodecoder::PCMFormat(0);
  bool planarFormat = false;
  int32_t trimStartNs = 0;
  int32_t trimEndNs = 0;
  bool lowLatency = false;
  bool endOfStream = false;
  bool discontinuity = false;
  ::com::rdk::hal::AVSource source = ::com::rdk::hal::AVSource(0);
  ::android::os::ParcelableHolder extension { ::android::Parcelable::Stability::STABILITY_VINTF };
  inline bool operator!=(const FrameMetadata& rhs) const {
    return std::tie(sourceCodec, isDolbyAtmos, numChannels, channelTypes, sampleRate, format, planarFormat, trimStartNs, trimEndNs, lowLatency, endOfStream, discontinuity, source, extension) != std::tie(rhs.sourceCodec, rhs.isDolbyAtmos, rhs.numChannels, rhs.channelTypes, rhs.sampleRate, rhs.format, rhs.planarFormat, rhs.trimStartNs, rhs.trimEndNs, rhs.lowLatency, rhs.endOfStream, rhs.discontinuity, rhs.source, rhs.extension);
  }
  inline bool operator<(const FrameMetadata& rhs) const {
    return std::tie(sourceCodec, isDolbyAtmos, numChannels, channelTypes, sampleRate, format, planarFormat, trimStartNs, trimEndNs, lowLatency, endOfStream, discontinuity, source, extension) < std::tie(rhs.sourceCodec, rhs.isDolbyAtmos, rhs.numChannels, rhs.channelTypes, rhs.sampleRate, rhs.format, rhs.planarFormat, rhs.trimStartNs, rhs.trimEndNs, rhs.lowLatency, rhs.endOfStream, rhs.discontinuity, rhs.source, rhs.extension);
  }
  inline bool operator<=(const FrameMetadata& rhs) const {
    return std::tie(sourceCodec, isDolbyAtmos, numChannels, channelTypes, sampleRate, format, planarFormat, trimStartNs, trimEndNs, lowLatency, endOfStream, discontinuity, source, extension) <= std::tie(rhs.sourceCodec, rhs.isDolbyAtmos, rhs.numChannels, rhs.channelTypes, rhs.sampleRate, rhs.format, rhs.planarFormat, rhs.trimStartNs, rhs.trimEndNs, rhs.lowLatency, rhs.endOfStream, rhs.discontinuity, rhs.source, rhs.extension);
  }
  inline bool operator==(const FrameMetadata& rhs) const {
    return std::tie(sourceCodec, isDolbyAtmos, numChannels, channelTypes, sampleRate, format, planarFormat, trimStartNs, trimEndNs, lowLatency, endOfStream, discontinuity, source, extension) == std::tie(rhs.sourceCodec, rhs.isDolbyAtmos, rhs.numChannels, rhs.channelTypes, rhs.sampleRate, rhs.format, rhs.planarFormat, rhs.trimStartNs, rhs.trimEndNs, rhs.lowLatency, rhs.endOfStream, rhs.discontinuity, rhs.source, rhs.extension);
  }
  inline bool operator>(const FrameMetadata& rhs) const {
    return std::tie(sourceCodec, isDolbyAtmos, numChannels, channelTypes, sampleRate, format, planarFormat, trimStartNs, trimEndNs, lowLatency, endOfStream, discontinuity, source, extension) > std::tie(rhs.sourceCodec, rhs.isDolbyAtmos, rhs.numChannels, rhs.channelTypes, rhs.sampleRate, rhs.format, rhs.planarFormat, rhs.trimStartNs, rhs.trimEndNs, rhs.lowLatency, rhs.endOfStream, rhs.discontinuity, rhs.source, rhs.extension);
  }
  inline bool operator>=(const FrameMetadata& rhs) const {
    return std::tie(sourceCodec, isDolbyAtmos, numChannels, channelTypes, sampleRate, format, planarFormat, trimStartNs, trimEndNs, lowLatency, endOfStream, discontinuity, source, extension) >= std::tie(rhs.sourceCodec, rhs.isDolbyAtmos, rhs.numChannels, rhs.channelTypes, rhs.sampleRate, rhs.format, rhs.planarFormat, rhs.trimStartNs, rhs.trimEndNs, rhs.lowLatency, rhs.endOfStream, rhs.discontinuity, rhs.source, rhs.extension);
  }

  ::android::Parcelable::Stability getStability() const override { return ::android::Parcelable::Stability::STABILITY_VINTF; }
  ::android::status_t readFromParcel(const ::android::Parcel* _aidl_parcel) final;
  ::android::status_t writeToParcel(::android::Parcel* _aidl_parcel) const final;
  static const ::android::String16& getParcelableDescriptor() {
    static const ::android::StaticString16 DESCRIPTOR (u"com.rdk.hal.audiodecoder.FrameMetadata");
    return DESCRIPTOR;
  }
  inline std::string toString() const {
    std::ostringstream os;
    os << "FrameMetadata{";
    os << "sourceCodec: " << ::android::internal::ToString(sourceCodec);
    os << ", isDolbyAtmos: " << ::android::internal::ToString(isDolbyAtmos);
    os << ", numChannels: " << ::android::internal::ToString(numChannels);
    os << ", channelTypes: " << ::android::internal::ToString(channelTypes);
    os << ", sampleRate: " << ::android::internal::ToString(sampleRate);
    os << ", format: " << ::android::internal::ToString(format);
    os << ", planarFormat: " << ::android::internal::ToString(planarFormat);
    os << ", trimStartNs: " << ::android::internal::ToString(trimStartNs);
    os << ", trimEndNs: " << ::android::internal::ToString(trimEndNs);
    os << ", lowLatency: " << ::android::internal::ToString(lowLatency);
    os << ", endOfStream: " << ::android::internal::ToString(endOfStream);
    os << ", discontinuity: " << ::android::internal::ToString(discontinuity);
    os << ", source: " << ::android::internal::ToString(source);
    os << ", extension: " << ::android::internal::ToString(extension);
    os << "}";
    return os.str();
  }
};  // class FrameMetadata
}  // namespace audiodecoder
}  // namespace hal
}  // namespace rdk
}  // namespace com

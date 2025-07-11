/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** @brief
* AudioDecoder.cpp
*
* Implementation of AudioDecoder service
*/

#include "AudioDecoder.h"
#include <iostream>
#include <thread>
#include <chrono>
namespace com
{
    namespace rdk
    {
        namespace hal
        {
            namespace audiodecoder
            {

                bool isAlphabetOnly(const android::String16& str16)
                {
                    std::u16string str = str16.string();  // Convert String16 to std::u16string
                    return !str.empty() && std::all_of(str.begin(), str.end(), [](char16_t c) {
                        return (c >= u'A' && c <= u'Z') || (c >= u'a' && c <= u'z');
                    });
                }

                /* Read only Properties */
                std::vector<::com::rdk::hal::audiodecoder::Codec> g_supported_codecs[NUM_AUDIO_DECODER] = {
                    {com::rdk::hal::audiodecoder::Codec::PCM,
                     com::rdk::hal::audiodecoder::Codec::DOLBY_AC3_PLUS_JOC,
                     com::rdk::hal::audiodecoder::Codec::MP3},
                    {com::rdk::hal::audiodecoder::Codec::PCM,
                     com::rdk::hal::audiodecoder::Codec::DOLBY_AC3_PLUS_JOC,
                     com::rdk::hal::audiodecoder::Codec::MP3}
                };
                IAudioDecoder::Id g_supported_id[NUM_AUDIO_DECODER];

                AudioDecoderManager::AudioDecoderManager()
                {
                    for (int32_t i = 0;i<NUM_AUDIO_DECODER;i++)
                    {
                        g_supported_id[i].value = i;
                    }
                }

                AudioDecoderManager::~AudioDecoderManager()
                {
                }

                android::binder::Status AudioDecoderManager::getAudioDecoderIds(  std::vector<IAudioDecoder::Id>* _aidl_return  )
                {
                    for (int32_t i = 0;i<NUM_AUDIO_DECODER;i++)
                    {
                        _aidl_return->push_back(g_supported_id[i]);
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderManager::getAudioDecoder(  const IAudioDecoder::Id& decoderResourceId, android::sp<IAudioDecoder>* _aidl_return  )
                {
                    android::binder::Status status;
                    int32_t found = 0;
                    for(int32_t i = 0; i < NUM_AUDIO_DECODER; i++)
                    {
                        if(g_supported_id[i].value == decoderResourceId.value)
                        {
                            found = 1;
                            break;
                        }
                    }
                    if(!found)
                    {
                        *_aidl_return = nullptr;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }

                    android::sp<IAudioDecoder> audioDecoder = new AudioDecoder(decoderResourceId);
                    *_aidl_return = audioDecoder;
                    return android::binder::Status::ok();
                }

                AudioDecoder::AudioDecoder(IAudioDecoder::Id id)
                {
                    this->mId = id;
                    this->mEventListener = false;
                    this->mAudioDecoderEventListener = nullptr;
                    this->mLowLatency = 0;
                    this->mAVSource = AVSource::UNKNOWN;
                    this->mSupportsSecure = 1;
                    this->mMetricsFrameDecoded = 0;
                    this->mMetricsDecodeErrors = 0;
                    this->mMetricsFramesDropped = 0;
                    this->mAC4PresentationGroupIndex = -1;
                    this->mAC4PreferredLang1 = android::String16("");
                    this->mAC4PreferredLang2 = android::String16("");
                    this->mAC4AssociatedType = -1;
                    this->mAutoSelectionPriority = -1;
                    this->mAC4MixerBalance = 255;
                    this->mAC4AssociatedAudioMixingEnable = -1;

                    this->setState(State::CLOSED);
                    this->mSupportedCodecs = g_supported_codecs[this->mId.value];
                }

                AudioDecoder::~AudioDecoder()
                {
                }

                android::binder::Status AudioDecoder::getCapabilities(  audiodecoder::Capabilities* _aidl_return  )
                {
                    com::rdk::hal::audiodecoder::Capabilities capabilities;
                    capabilities.supportedCodecs = this->mSupportedCodecs;
                    capabilities.supportsSecure = this->mSupportsSecure;
                    *_aidl_return = capabilities;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::getProperty(audiodecoder::Property property, std::optional<com::rdk::hal::PropertyValue>* _aidl_return)
                {
                    if (!_aidl_return)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
                    }
                    com::rdk::hal::PropertyValue value;
                    switch (property)
                    {
                        case audiodecoder::Property::RESOURCE_ID:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mId.value);
                            break;
                        case audiodecoder::Property::LOW_LATENCY_MODE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mLowLatency);
                            break;
                        case audiodecoder::Property::AV_SOURCE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(static_cast<int32_t>(this->mAVSource));
                            break;
                        case audiodecoder::Property::SECURE_AUDIO:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mSupportsSecure);
                            break;
                        case audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mAC4PresentationGroupIndex);
                            break;
                        case audiodecoder::Property::AC4_PREFERRED_LANG1:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::stringValue>(this->mAC4PreferredLang1);
                            break;
                        case audiodecoder::Property::AC4_PREFERRED_LANG2:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::stringValue>(this->mAC4PreferredLang2);
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_TYPE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mAC4AssociatedType);
                            break;
                        case audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mAutoSelectionPriority);
                            break;
                        case audiodecoder::Property::AC4_MIXER_BALANCE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mAC4MixerBalance);
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mAC4AssociatedAudioMixingEnable);
                            break;
                        case audiodecoder::Property::METRIC_FRAMES_DECODED:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mMetricsFrameDecoded);
                            break;
                        case audiodecoder::Property::METRIC_DECODE_ERRORS:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mMetricsDecodeErrors);
                            break;
                        case audiodecoder::Property::METRIC_FRAMES_DROPPED:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mMetricsFramesDropped);
                            break;
                        default:
                            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    *_aidl_return = value;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::getState(  State* _aidl_return  )
                {
                    *_aidl_return = this->getState();
                    return android::binder::Status::ok();
                }

                State AudioDecoder::getState()
                {
                    std::lock_guard<std::mutex> lock(this->mStateMutex);
                    State state = this->mAudioDecoderState;
                    return state;
                }
                void AudioDecoder::setState(  State newState  )
                {
                    std::lock_guard<std::mutex> lock(this->mStateMutex);
                    this->mAudioDecoderState = newState;
                }

                android::binder::Status AudioDecoder::open(  audiodecoder::Codec codec, bool secure, const android::sp<audiodecoder::IAudioDecoderControllerListener>& audioDecoderControllerListener, android::sp<audiodecoder::IAudioDecoderController>* _aidl_return  )
                {
                    android::binder::Status status;

                    if(std::find(this->mSupportedCodecs.begin(), this->mSupportedCodecs.end(), codec) == this->mSupportedCodecs.end())
                    {
                        *_aidl_return = nullptr;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    if(audioDecoderControllerListener == nullptr)
                    {
                        *_aidl_return = nullptr;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }

                    if(this->getState() != State::CLOSED)
                    {
                        *_aidl_return = nullptr;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }

                    /* To simulate State Transitions */
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (this->mEventListener)
                        auto status = this->mAudioDecoderEventListener->onStateChanged(this->getState(), State::OPENING);

                    this->setState(State::OPENING);

                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    *_aidl_return = new AudioDecoderController(this->mId, this);

                    if (this->mEventListener)
                        this->mAudioDecoderEventListener->onStateChanged(State::OPENING, State::READY);

                    this->setState(State::READY);

                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::close(  const android::sp<audiodecoder::IAudioDecoderController>& audioDecoderController, bool* _aidl_return  )
                {
                    android::binder::Status status;
                    if(this->getState() != State::READY)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }

                    /* To simulate State Transitions */
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (this->mEventListener)
                        auto status = this->mAudioDecoderEventListener->onStateChanged(this->getState(), State::CLOSING);

                    this->setState(State::CLOSING);

                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    * _aidl_return = true;

                    if (this->mEventListener)
                        this->mAudioDecoderEventListener->onStateChanged(State::CLOSING, State::CLOSED);

                    this->setState(State::CLOSED);

                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::registerEventListener(  const android::sp<audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return  )
                {
                    android::binder::Status status;

                    if(this->mEventListener == true)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::ok();
                    }
                    *_aidl_return = true;
                    this->mEventListener = true;
                    this->mAudioDecoderEventListener = audioDecoderEventListener;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::unregisterEventListener(  const android::sp<audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return  )
                {
                    android::binder::Status status;

                    if(this->mEventListener ==false)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::ok();
                    }
                    *_aidl_return = true;
                    this->mEventListener = false;
                    this->mAudioDecoderEventListener = nullptr;
                    return android::binder::Status::ok();
                }

                AudioDecoderController::AudioDecoderController(IAudioDecoder::Id id, AudioDecoder *parentDecoder)
                {
                    this->mId = id;
                    this->mParentDecoder = parentDecoder;
                }

                AudioDecoderController::~AudioDecoderController()
                {
                }

                android::binder::Status AudioDecoderController::start()
                {
                    android::binder::Status status;
                    if(this->mParentDecoder->getState() != State::READY)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }

                    /* To simulate State Transitions */
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (this->mParentDecoder->mEventListener)
                        auto status = this->mParentDecoder->mAudioDecoderEventListener->onStateChanged(this->mParentDecoder->getState(), State::STARTING);

                    this->mParentDecoder->setState(State::STARTING);

                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (this->mParentDecoder->mEventListener)
                        this->mParentDecoder->mAudioDecoderEventListener->onStateChanged(State::STARTING, State::STARTED);

                    this->mParentDecoder->setState(State::STARTED);

                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::stop()
                {
                    android::binder::Status status;
                    if(this->mParentDecoder->getState() != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }

                    /* To simulate State Transitions */
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (this->mParentDecoder->mEventListener)
                        auto status = this->mParentDecoder->mAudioDecoderEventListener->onStateChanged(this->mParentDecoder->getState(), State::STOPPING);

                    this->mParentDecoder->setState(State::STOPPING);

                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (this->mParentDecoder->mEventListener)
                        this->mParentDecoder->mAudioDecoderEventListener->onStateChanged(State::STOPPING, State::READY);

                    this->mParentDecoder->setState(State::READY);

                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::decodeBuffer(  int64_t nsPresentationTime, int64_t bufferHandle, int32_t trimStartNs, int32_t trimEndNs, bool* _aidl_return  )
                {
                    android::binder::Status status;

                    if(bufferHandle < 0)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    if(this->mParentDecoder->getState() != State::STARTED)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    if(nsPresentationTime < 0)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    if(trimStartNs < 0)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    if(trimEndNs < 0)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    if(trimStartNs > trimEndNs)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    *_aidl_return = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::flush(  bool reset  )
                {
                    android::binder::Status status;
                    if(this->mParentDecoder->getState() != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }

                    /* To simulate State Transitions */
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (this->mParentDecoder->mEventListener)
                        auto status = this->mParentDecoder->mAudioDecoderEventListener->onStateChanged(this->mParentDecoder->getState(), State::FLUSHING);

                    this->mParentDecoder->setState(State::FLUSHING);

                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (reset)
                    {
                        if (this->mParentDecoder->mEventListener)
                            this->mParentDecoder->mAudioDecoderEventListener->onStateChanged(State::FLUSHING, State::READY);
                        this->mParentDecoder->setState(State::READY);
                    }
                    else
                    {
                        if (this->mParentDecoder->mEventListener)
                            this->mParentDecoder->mAudioDecoderEventListener->onStateChanged(State::FLUSHING, State::STARTED);
                        this->mParentDecoder->setState(State::STARTED);
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::signalDiscontinuity()
                {
                    android::binder::Status status;
                    if(this->mParentDecoder->getState() != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::signalEOS()
                {
                    android::binder::Status status;
                    if(this->mParentDecoder->getState() != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::parseCodecSpecificData(  rdk::hal::audiodecoder::CSDAudioFormat csdAudioFormat, const std::vector<uint8_t>& codecData, bool* _aidl_return  )
                {
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::setProperty( audiodecoder::Property property, const PropertyValue& propertyValue, bool* _aidl_return )
                {
                    int32_t intvalue;
                    android::String16 strvalue;
                    *_aidl_return = false;
                    switch (property)
                    {
                        case audiodecoder::Property::LOW_LATENCY_MODE:
                            if(this->mParentDecoder->getState() != State::READY)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                            }
                            intvalue  = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < 0 || intvalue > 1)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mLowLatency = intvalue;
                            break;
                        case audiodecoder::Property::AV_SOURCE:
                            if(this->mParentDecoder->getState() != State::READY)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                            }
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();
                            if (intvalue < static_cast<int32_t>(AVSource::AUTO) || intvalue > static_cast<int32_t>(AVSource::COMPOSITE_5))
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAVSource = static_cast<AVSource>(intvalue);
                            break;
                        case audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -1)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAC4PresentationGroupIndex = intvalue;
                            break;
                        case audiodecoder::Property::AC4_PREFERRED_LANG1:
                            strvalue = propertyValue.value->get<PropertyValue::Value::Tag::stringValue>();

                            if((strvalue.size() != 0) && !isAlphabetOnly(strvalue))
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAC4PreferredLang1 = strvalue;
                            break;
                        case audiodecoder::Property::AC4_PREFERRED_LANG2:
                            strvalue = propertyValue.value->get<PropertyValue::Value::Tag::stringValue>();

                            if((strvalue.size() != 0) && !isAlphabetOnly(strvalue))
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAC4PreferredLang2 = strvalue;
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_TYPE:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -1 || intvalue > 2)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAC4AssociatedType = intvalue;
                            break;
                        case audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -1 || intvalue > 1)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAutoSelectionPriority = intvalue;
                            break;
                        case audiodecoder::Property::AC4_MIXER_BALANCE:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -32 || (intvalue > 32 && intvalue != 255))
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAC4MixerBalance = intvalue;
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -1 || intvalue > 1)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAC4AssociatedAudioMixingEnable = intvalue;
                            break;
                        default:
                            *_aidl_return = false;
                            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    *_aidl_return = true;
                    return android::binder::Status::ok();
                }
            }
        }
    }
}

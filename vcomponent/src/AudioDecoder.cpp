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

namespace com
{
    namespace rdk
    {
        namespace hal
        {
            namespace audiodecoder
            {
                State g_audiodecoder_state[NUM_AUDIO_DECODER] = {State::UNKNOWN, State::UNKNOWN};
                ::std::vector<::com::rdk::hal::audiodecoder::Codec> supportedCodecs = {
                    com::rdk::hal::audiodecoder::Codec::PCM,
                    com::rdk::hal::audiodecoder::Codec::DOLBY_AC3_PLUS_JOC,
                    com::rdk::hal::audiodecoder::Codec::MP3
                };

                IAudioDecoder::Id supported_id[NUM_AUDIO_DECODER];
                AudioDecoderManager::AudioDecoderManager()
                {
                    for (int32_t i = 0;i<NUM_AUDIO_DECODER;i++)
                    {
                        supported_id[i].value = i;
                    }
                    printf("AudioDecoderManager service initialized\n");
                }

                AudioDecoderManager::~AudioDecoderManager()
                {
                    printf("AudioDecoderManager service destroyed\n");
                }

                android::sp<audiodecoder::IAudioDecoderEventListener> _audioDecoderEventListener;

                android::binder::Status AudioDecoderManager::getAudioDecoderIds(  std::vector<IAudioDecoder::Id>* _aidl_return  )
                {
                    for (int32_t i = 0;i<NUM_AUDIO_DECODER;i++)
                    {
                        _aidl_return->push_back(supported_id[i]);
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderManager::getAudioDecoder(  const IAudioDecoder::Id& decoderResourceId, android::sp<IAudioDecoder>* _aidl_return  )
                {
                    android::binder::Status status;
                    int32_t found = 0;
                    for(int32_t i = 0; i < NUM_AUDIO_DECODER; i++)
                    {
                        if(supported_id[i].value == decoderResourceId.value)
                        {
                            found = 1;
                            break;
                        }
                    }
                    if(!found)
                    {
                        *_aidl_return = nullptr;
                        android::String8 str8("Illigal argument");
                        status.setException(android::binder::Status::EX_ILLEGAL_ARGUMENT, str8);
                        return status;
                    }

                    android::sp<IAudioDecoder> audioDecoder = new AudioDecoder(decoderResourceId);
                    g_audiodecoder_state[decoderResourceId.value] = State::CLOSED;
                    *_aidl_return = audioDecoder;
                    return android::binder::Status::ok();
                }

                AudioDecoder::AudioDecoder(IAudioDecoder::Id id)
                {
                    this->id = id;
                    this->event_listener = false;
                    printf("AudioDecoder service initialized\n");
                }

                AudioDecoder::~AudioDecoder()
                {
                    printf("AudioDecoder service destroyed\n");
                }

                android::binder::Status AudioDecoder::getCapabilities(  audiodecoder::Capabilities* _aidl_return  )
                {
                    com::rdk::hal::audiodecoder::Capabilities capabilities;
                    capabilities.supportedCodecs = {
                            com::rdk::hal::audiodecoder::Codec::PCM,
                            com::rdk::hal::audiodecoder::Codec::DOLBY_AC3_PLUS_JOC,
                            com::rdk::hal::audiodecoder::Codec::MP3
                    };
                    capabilities.supportsSecure = true;
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
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* your resource ID here */ 42);
                            break;
                        case audiodecoder::Property::LOW_LATENCY_MODE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* your low latency mode value here */ 1);
                            break;
                        case audiodecoder::Property::AV_SOURCE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* your AV source value here */ 25);
                            break;
                        case audiodecoder::Property::SECURE_AUDIO:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* secure audio value */ 1);
                            break;
                        case audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* AC4 presentation group index value */ -1);
                            break;
                        case audiodecoder::Property::AC4_PREFERRED_LANG1:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::stringValue>(android::String16("eng"));
                            break;
                        case audiodecoder::Property::AC4_PREFERRED_LANG2:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::stringValue>(android::String16("poo"));
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_TYPE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* associated type */ 1);
                            break;
                        case audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* auto selection priority */ -1);
                            break;
                        case audiodecoder::Property::AC4_MIXER_BALANCE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* mixer balance */ 255);
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* audio mixing enable */ 1);
                            break;
                        case audiodecoder::Property::METRIC_FRAMES_DECODED:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* metric value */ 100);
                            break;
                        case audiodecoder::Property::METRIC_DECODE_ERRORS:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* metric value */ 200);
                            break;
                        case audiodecoder::Property::METRIC_FRAMES_DROPPED:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* metric value */ 300);
                            break;
                        default:
                            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    *_aidl_return = value;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::getState(  State* _aidl_return  )
                {
                    *_aidl_return = g_audiodecoder_state[this->id.value];
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::open(  audiodecoder::Codec codec, bool secure, const android::sp<audiodecoder::IAudioDecoderControllerListener>& audioDecoderControllerListener, android::sp<audiodecoder::IAudioDecoderController>* _aidl_return  )
                {
                    android::binder::Status status;

                    if(std::find(supportedCodecs.begin(), supportedCodecs.end(), codec) == supportedCodecs.end())
                    {
                        *_aidl_return = nullptr;
                        android::String8 str8("Illigal argument");
                        status.setException(android::binder::Status::EX_ILLEGAL_ARGUMENT, str8);
                        return status;
                    }
                    if(audioDecoderControllerListener == nullptr)
                    {
                        *_aidl_return = nullptr;
                        android::String8 str8("Illigal argument");
                        status.setException(android::binder::Status::EX_ILLEGAL_ARGUMENT, str8);
                        return status;
                    }
                    if(g_audiodecoder_state[this->id.value] != State::CLOSED)
                    {
                        *_aidl_return = nullptr;
                        android::String8 str8("Illigal state");
                        status.setException(android::binder::Status::EX_ILLEGAL_STATE, str8);
                        return status;
                    }
                    *_aidl_return = new AudioDecoderController(this->id);
                    g_audiodecoder_state[this->id.value] = State::READY;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::close(  const android::sp<audiodecoder::IAudioDecoderController>& audioDecoderController, bool* _aidl_return  )
                {
                    android::binder::Status status;
                    if(g_audiodecoder_state[this->id.value] != State::READY)
                    {
                        *_aidl_return = false;
                        android::String8 str8("Illigal state");
                        status.setException(android::binder::Status::EX_ILLEGAL_STATE, str8);
                        return status;
                    }
                    g_audiodecoder_state[this->id.value] = State::CLOSED;
                    * _aidl_return = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::registerEventListener(  const android::sp<audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return  )
                {
                    android::binder::Status status;

                    if(this->event_listener == true)
                    {
                        *_aidl_return = false;
                        android::String8 str8("Illigal state");
                        status.setException(android::binder::Status::EX_ILLEGAL_STATE, str8);
                        return status;
                    }
                    *_aidl_return = true;
                    this->event_listener = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::unregisterEventListener(  const android::sp<audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return  )
                {
                    android::binder::Status status;

                    if(this->event_listener ==false)
                    {
                        *_aidl_return = false;
                        android::String8 str8("Illigal state");
                        status.setException(android::binder::Status::EX_ILLEGAL_STATE, str8);
                        return status;
                    }
                    *_aidl_return = true;
                    this->event_listener = false;
                    return android::binder::Status::ok();
                }

                AudioDecoderController::AudioDecoderController(IAudioDecoder::Id id)
                {
                    this->id = id;
                    printf("AudioDecoderController service initialized");
                }

                AudioDecoderController::~AudioDecoderController()
                {
                    printf("AudioDecoderController service destroyed");
                }

                android::binder::Status AudioDecoderController::start()
                {
                    android::binder::Status status;
                    if(g_audiodecoder_state[this->id.value] != State::READY)
                    {
                        android::String8 str8("Illigal state");
                        status.setException(android::binder::Status::EX_ILLEGAL_STATE, str8);
                        return status;
                    }
                    g_audiodecoder_state[this->id.value] = State::STARTED;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::stop()
                {
                    android::binder::Status status;
                    if(g_audiodecoder_state[this->id.value] != State::STARTED)
                    {
                        android::String8 str8("Illigal state");
                        status.setException(android::binder::Status::EX_ILLEGAL_STATE, str8);
                        return status;
                    }
                    g_audiodecoder_state[this->id.value] = State::READY;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::decodeBuffer(  int64_t nsPresentationTime, int64_t bufferHandle, int32_t trimStartNs, int32_t trimEndNs, bool* _aidl_return  )
                {
                    android::binder::Status status;
                    if(g_audiodecoder_state[this->id.value] != State::STARTED)
                    {
                        *_aidl_return = false;
                        android::String8 str8("Illigal state");
                        status.setException(android::binder::Status::EX_ILLEGAL_STATE, str8);
                        return status;
                    }
                    if(nsPresentationTime < 0)
                    {
                        *_aidl_return = false;
                        android::String8 str8("Illigal state");
                        status.setException(android::binder::Status::EX_ILLEGAL_ARGUMENT, str8);
                        return status;
                    }
                    if(trimStartNs < 0)
                    {
                        *_aidl_return = false;
                        android::String8 str8("Illigal state");
                        status.setException(android::binder::Status::EX_ILLEGAL_ARGUMENT, str8);
                        return status;
                    }
                    if(trimEndNs < 0)
                    {
                        *_aidl_return = false;
                        android::String8 str8("Illigal state");
                        status.setException(android::binder::Status::EX_ILLEGAL_ARGUMENT, str8);
                        return status;
                    }
                    if(trimStartNs > trimEndNs)
                    {
                        *_aidl_return = false;
                        android::String8 str8("Illigal state");
                        status.setException(android::binder::Status::EX_ILLEGAL_ARGUMENT, str8);
                        return status;
                    }
                    *_aidl_return = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::flush(  bool reset  )
                {
                    android::binder::Status status;
                    if(g_audiodecoder_state[this->id.value] != State::STARTED)
                    {
                        android::String8 str8("Illigal state");
                        status.setException(android::binder::Status::EX_ILLEGAL_STATE, str8);
                        return status;
                    }
                    if (reset)
                    {
                        g_audiodecoder_state[this->id.value] = State::READY;
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::signalDiscontinuity()
                {
                    android::binder::Status status;
                    if(g_audiodecoder_state[this->id.value] != State::STARTED)
                    {
                        android::String8 str8("Illigal state");
                        status.setException(android::binder::Status::EX_ILLEGAL_STATE, str8);
                        return status;
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::signalEOS()
                {
                    android::binder::Status status;
                    if(g_audiodecoder_state[this->id.value] != State::STARTED)
                    {
                        android::String8 str8("Illigal state");
                        status.setException(android::binder::Status::EX_ILLEGAL_STATE, str8);
                        return status;
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::parseCodecSpecificData(  rdk::hal::audiodecoder::CSDAudioFormat csdAudioFormat, const std::vector<uint8_t>& codecData, bool* _aidl_return  )
                {
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::setProperty( audiodecoder::Property property, const PropertyValue& propertyValue, bool* _aidl_return )
                {
                    (void) property;
                    (void) propertyValue;
                    (void) _aidl_return;
                    return android::binder::Status::ok();
                }
            }
        }
    }
}




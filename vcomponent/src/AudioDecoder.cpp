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
                State g_audiodecoder_state[NUM_AUDIO_DECODER] = {State::UNKNOWN, State::UNKNOWN};
                ::std::vector<::com::rdk::hal::audiodecoder::Codec> supportedCodecs = {
                        com::rdk::hal::audiodecoder::Codec::PCM,
                        com::rdk::hal::audiodecoder::Codec::DOLBY_AC3_PLUS_JOC,
                        com::rdk::hal::audiodecoder::Codec::MP3
                };
                IAudioDecoder::Id g_supported_id[NUM_AUDIO_DECODER];
                int32_t g_support_secure[NUM_AUDIO_DECODER];
                int32_t g_metrics_frame_decoded[NUM_AUDIO_DECODER];
                int32_t g_metrics_decode_errors[NUM_AUDIO_DECODER];
                int32_t g_metrics_frames_dropped[NUM_AUDIO_DECODER];

                /* Read Write Properties */
                int32_t g_low_latency[NUM_AUDIO_DECODER];
                AVSource g_av_source[NUM_AUDIO_DECODER];
                int32_t g_ac4_presentation_group_index[NUM_AUDIO_DECODER];
                android::String16 g_ac4_preferred_lang1[NUM_AUDIO_DECODER];
                android::String16 g_ac4_preferred_lang2[NUM_AUDIO_DECODER];
                int32_t g_ac4_associated_type[NUM_AUDIO_DECODER];
                int32_t g_auto_selection_priority[NUM_AUDIO_DECODER];
                int32_t g_ac4_mixer_balance[NUM_AUDIO_DECODER];
                int32_t g_ac4_associated_audio_mixing_enable[NUM_AUDIO_DECODER];

                std::mutex state_mutex;
                std::condition_variable state_cv;

                AudioDecoderManager::AudioDecoderManager()
                {
                    for (int32_t i = 0;i<NUM_AUDIO_DECODER;i++)
                    {
                        g_supported_id[i].value = i;
                        g_low_latency[i] = 0;
                        g_av_source[i] = AVSource::UNKNOWN;
                        g_support_secure[i] = 1;
                        g_ac4_presentation_group_index[i] = -1;
                        g_ac4_preferred_lang1[i] = android::String16("");
                        g_ac4_preferred_lang2[i] = android::String16("");
                        g_ac4_associated_type[i] = -1;
                        g_auto_selection_priority[i] = -1;
                        g_ac4_mixer_balance[i] = 255;
                        g_ac4_associated_audio_mixing_enable[i] = -1;
                        g_metrics_frame_decoded[i] = 0;
                        g_metrics_decode_errors[i] = 0;
                        g_metrics_frames_dropped[i] = 0;
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
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->id.value);
                            break;
                        case audiodecoder::Property::LOW_LATENCY_MODE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_low_latency[this->id.value]);
                            break;
                        case audiodecoder::Property::AV_SOURCE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(static_cast<int32_t>(g_av_source[this->id.value]));
                            break;
                        case audiodecoder::Property::SECURE_AUDIO:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_support_secure[this->id.value]);
                            break;
                        case audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_ac4_presentation_group_index[this->id.value]);
                            break;
                        case audiodecoder::Property::AC4_PREFERRED_LANG1:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::stringValue>(g_ac4_preferred_lang1[this->id.value]);
                            break;
                        case audiodecoder::Property::AC4_PREFERRED_LANG2:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::stringValue>(g_ac4_preferred_lang2[this->id.value]);
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_TYPE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_ac4_associated_type[this->id.value]);
                            break;
                        case audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_auto_selection_priority[this->id.value]);
                            break;
                        case audiodecoder::Property::AC4_MIXER_BALANCE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_ac4_mixer_balance[this->id.value]);
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_ac4_associated_audio_mixing_enable[this->id.value]);
                            break;
                        case audiodecoder::Property::METRIC_FRAMES_DECODED:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_metrics_frame_decoded[this->id.value]);
                            break;
                        case audiodecoder::Property::METRIC_DECODE_ERRORS:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_metrics_decode_errors[this->id.value]);
                            break;
                        case audiodecoder::Property::METRIC_FRAMES_DROPPED:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_metrics_frames_dropped[this->id.value]);
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
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    if(audioDecoderControllerListener == nullptr)
                    {
                        *_aidl_return = nullptr;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    if(g_audiodecoder_state[this->id.value] != State::CLOSED)
                    {
                        *_aidl_return = nullptr;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from CLOSED to OPENING...\n";
                            g_audiodecoder_state[this->id.value] = State::OPENING;
                        }
                        //average wait time to simulate transitory state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        std::cout << "[SERVICE]::Calling onStateChanged for OPENING...\n";
                        _audioDecoderEventListener->onStateChanged(State::CLOSED, State::OPENING);
                        state_cv.notify_all();
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from OPENING to READY...\n";
                            g_audiodecoder_state[this->id.value] = State::READY;
                        }
                        //average wait time to simulate target state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(250));
                        std::cout << "[SERVICE]::Calling onStateChanged for READY...\n";
                        _audioDecoderEventListener->onStateChanged(State::OPENING, State::READY);
                        state_cv.notify_all();
                    }
                    *_aidl_return = new AudioDecoderController(this->id);
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::close(  const android::sp<audiodecoder::IAudioDecoderController>& audioDecoderController, bool* _aidl_return  )
                {
                    android::binder::Status status;
                    if(g_audiodecoder_state[this->id.value] != State::READY)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from READY to CLOSING...\n";
                            g_audiodecoder_state[this->id.value] = State::CLOSING;
                        }
                        //average wait time to simulate transitory state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        std::cout << "[SERVICE]::Calling onStateChanged for CLOSING...\n";
                        _audioDecoderEventListener->onStateChanged(State::READY, State::CLOSING);
                        state_cv.notify_all();
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from CLOSING to CLOSED...\n";
                            g_audiodecoder_state[this->id.value] = State::CLOSED;
                        }
                        //average wait time to simulate target state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(250));
                        std::cout << "[SERVICE]::Calling onStateChanged for CLOSED...\n";
                        _audioDecoderEventListener->onStateChanged(State::CLOSING, State::CLOSED);
                        state_cv.notify_all();
                    }
                    * _aidl_return = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::registerEventListener(  const android::sp<audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return  )
                {
                    android::binder::Status status;

                    if(this->event_listener == true)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
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
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
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
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from READY to STARTING...\n";
                            g_audiodecoder_state[this->id.value] = State::STARTED;
                        }
                        //average wait time to simulate transitory state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        std::cout << "[SERVICE]::Calling onStateChanged for STARTING...\n";
                        _audioDecoderEventListener->onStateChanged(State::READY, State::STARTING);
                        state_cv.notify_all();
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from STARTING to STARTED...\n";
                            g_audiodecoder_state[this->id.value] = State::STARTED;
                        }
                        //average wait time to simulate target state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(250));
                        std::cout << "[SERVICE]::Calling onStateChanged for STARTED...\n";
                        _audioDecoderEventListener->onStateChanged(State::STARTING, State::STARTED);
                        state_cv.notify_all();
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::stop()
                {
                    android::binder::Status status;
                    if(g_audiodecoder_state[this->id.value] != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from STARTED to STOPPING...\n";
                            g_audiodecoder_state[this->id.value] = State::STOPPING;
                        }
                        //average wait time to simulate transitory state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        std::cout << "[SERVICE]::Calling onStateChanged for STOPPING...\n";
                        _audioDecoderEventListener->onStateChanged(State::STARTED, State::STOPPING);
                        state_cv.notify_all();
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from STOPPING to READY...\n";
                            g_audiodecoder_state[this->id.value] = State::READY;
                        }
                        //average wait time to simulate target state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(250));
                        std::cout << "[SERVICE]::Calling onStateChanged for READY...\n";
                        _audioDecoderEventListener->onStateChanged(State::STOPPING, State::READY);
                        state_cv.notify_all();
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::decodeBuffer(  int64_t nsPresentationTime, int64_t bufferHandle, int32_t trimStartNs, int32_t trimEndNs, bool* _aidl_return  )
                {
                    android::binder::Status status;
                    if(g_audiodecoder_state[this->id.value] != State::STARTED)
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
                    if(g_audiodecoder_state[this->id.value] != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }

                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from STARTED to FLUSHING...\n";
                            g_audiodecoder_state[this->id.value] = State::FLUSHING;
                        }
                        //average wait time to simulate transitory state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        std::cout << "[SERVICE]::Calling onStateChanged for FLUSHING...\n";
                        _audioDecoderEventListener->onStateChanged(State::STARTED, State::FLUSHING);
                        state_cv.notify_all();
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from FLUSHING to STARTED...\n";
                            g_audiodecoder_state[this->id.value] = State::STARTED;
                        }
                        //average wait time to simulate target state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(250));
                        std::cout << "[SERVICE]::Calling onStateChanged for STARTED...\n";
                        _audioDecoderEventListener->onStateChanged(State::FLUSHING, State::STARTED);
                        state_cv.notify_all();
                    }

                    if (reset)
                    {
                        {
                            {
                                std::lock_guard <std::mutex> lock(state_mutex);
                                std::cout << "[SERVICE]::Changing state from STARTED to READY...\n";
                                g_audiodecoder_state[this->id.value] = State::READY;
                            }
                            //average wait time to simulate target state change
                            std::this_thread::sleep_for(std::chrono::milliseconds(250));
                            std::cout << "[SERVICE]::Calling onStateChanged for READY...\n";
                            _audioDecoderEventListener->onStateChanged(State::STARTED, State::READY);
                            state_cv.notify_all();
                        }
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::signalDiscontinuity()
                {
                    android::binder::Status status;
                    if(g_audiodecoder_state[this->id.value] != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::signalEOS()
                {
                    android::binder::Status status;
                    if(g_audiodecoder_state[this->id.value] != State::STARTED)
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
                    *_aidl_return = true;
                    switch (property)
                    {
                        case audiodecoder::Property::LOW_LATENCY_MODE:
                            if(g_audiodecoder_state[this->id.value] != State::READY)
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
                            g_low_latency[this->id.value] = intvalue;
                            break;
                        case audiodecoder::Property::AV_SOURCE:
                            if(g_audiodecoder_state[this->id.value] != State::READY)
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
                            g_av_source[this->id.value] = static_cast<AVSource>(intvalue);
                            break;
                        case audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -1)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            g_ac4_presentation_group_index[this->id.value] = intvalue;
                            break;
                        case audiodecoder::Property::AC4_PREFERRED_LANG1:
                            strvalue = propertyValue.value->get<PropertyValue::Value::Tag::stringValue>();

                            if((strvalue.size() != 0) && !isAlphabetOnly(strvalue))
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            g_ac4_preferred_lang1[this->id.value] = strvalue;
                            break;
                        case audiodecoder::Property::AC4_PREFERRED_LANG2:
                            strvalue = propertyValue.value->get<PropertyValue::Value::Tag::stringValue>();

                            if((strvalue.size() != 0) && !isAlphabetOnly(strvalue))
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            g_ac4_preferred_lang2[this->id.value] = strvalue;
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_TYPE:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -1 || intvalue > 2)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            g_ac4_associated_type[this->id.value] = intvalue;
                            break;
                        case audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -1 || intvalue > 1)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            g_auto_selection_priority[this->id.value] = intvalue;
                            break;
                        case audiodecoder::Property::AC4_MIXER_BALANCE:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -32 || (intvalue > 32 && intvalue != 255))
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            g_ac4_mixer_balance[this->id.value] = intvalue;
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -1 || intvalue > 1)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            g_ac4_associated_audio_mixing_enable[this->id.value] = intvalue;
                            break;
                        default:
                            *_aidl_return = false;
                            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    return android::binder::Status::ok();
                }
            }
        }
    }
}

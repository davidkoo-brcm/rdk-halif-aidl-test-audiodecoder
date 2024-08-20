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

                AudioDecoderManager::AudioDecoderManager()
                {
                    printf("AudioDecoderManager service initialized\n");
                }

                AudioDecoderManager::~AudioDecoderManager()
                {
                    printf("AudioDecoderManager service destroyed\n");
                }

                android::sp<audiodecoder::IAudioDecoderEventListener> _audioDecoderEventListener;

                android::binder::Status AudioDecoderManager::getAudioDecoderIds(  std::vector<IAudioDecoder::Id>* _aidl_return  )
                {
                    IAudioDecoder::Id id1;
                    id1.value = 0; // Set value
                    IAudioDecoder::Id id2;
                    id2.value = 1; // Set value
                    _aidl_return->push_back(id1);
                    _aidl_return->push_back(id2);
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderManager::getAudioDecoder(  const IAudioDecoder::Id& decoderResourceId, android::sp<IAudioDecoder>* _aidl_return  )
                {
                    android::sp<IAudioDecoder> audioDecoder = new AudioDecoder();
                    *_aidl_return = audioDecoder;
                    return android::binder::Status::ok();
                }

                AudioDecoder::AudioDecoder()
                {
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
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* your AV source value here */ 0);
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
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* mixer balance */ 0);
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* audio mixing enable */ 1);
                            break;
                        case audiodecoder::Property::METRIC_xxxx:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(/* metric value */ 100);
                            break;
                        default:
                            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    *_aidl_return = value;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::setProperty(  audiodecoder::Property property, const PropertyValue& propertyValue, bool* _aidl_return  )
                {
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::getState(  State* _aidl_return  )
                {
                    *_aidl_return = State::CLOSED;
                    return android::binder::Status::ok();
                }



                android::binder::Status AudioDecoder::open(  audiodecoder::Codec codec, bool secure, const android::sp<audiodecoder::IAudioDecoderControllerListener>& audioDecoderControllerListener, android::sp<audiodecoder::IAudioDecoderController>* _aidl_return  )
                {
                    *_aidl_return = new AudioDecoderController();
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::close(  const android::sp<audiodecoder::IAudioDecoderController>& audioDecoderController, bool* _aidl_return  )
                {
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::registerEventListener(  const android::sp<audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return  )
                {
                    *_aidl_return = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::unregisterEventListener(  const android::sp<audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return  )
                {
                    *_aidl_return = true;
                    return android::binder::Status::ok();
                }

                AudioDecoderController::AudioDecoderController()
                {
                    printf("AudioDecoderController service initialized");
                }

                AudioDecoderController::~AudioDecoderController()
                {
                    printf("AudioDecoderController service destroyed");
                }

                android::binder::Status AudioDecoderController::start()
                {
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::stop()
                {
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::decodeBuffer(  int64_t nsPresentationTime, int64_t bufferHandle, int32_t trimStartNs, int32_t trimEndNs, bool* _aidl_return  )
                {
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::flush(  bool reset  )
                {
                    return android::binder::Status::ok();
                }
    
                android::binder::Status AudioDecoderController::signalDiscontinuity()
                {
                    return android::binder::Status::ok();
                }
    
                android::binder::Status AudioDecoderController::signalEOS()
                {
                    return android::binder::Status::ok();
                }
    
                android::binder::Status AudioDecoderController::parseCodecSpecificData(  rdk::hal::audiodecoder::CSDAudioFormat csdAudioFormat, const std::vector<uint8_t>& codecData, bool* _aidl_return  )
                {
                    return android::binder::Status::ok();
                }
            }
        }
    }
}




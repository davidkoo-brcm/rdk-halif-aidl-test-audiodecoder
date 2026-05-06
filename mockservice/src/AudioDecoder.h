/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
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
* AudioDecoder.h
*
* Header file for the AudioDecoder service implementation
*/

#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include <com/rdk/hal/audiodecoder/BnAudioDecoderManager.h>
#include <com/rdk/hal/audiodecoder/BnAudioDecoder.h>
#include <com/rdk/hal/audiodecoder/IAudioDecoder.h>
#include <com/rdk/hal/audiodecoder/BnAudioDecoderController.h>
#include <com/rdk/hal/audiodecoder/InputBufferMetadata.h>
#include <binder/BinderService.h>
#include <optional>
#include <string>
#include <utils/StrongPointer.h>
#include <vector>
#include <mutex>

namespace com {
    namespace rdk {
        namespace hal {
            namespace audiodecoder {

#define NUM_AUDIO_DECODER 2

                class AudioDecoderManager :public android::BinderService<AudioDecoderManager>,
                                           public BnAudioDecoderManager
                {
                public:
                    AudioDecoderManager();
                    virtual ~AudioDecoderManager();

                    static char const* getServiceName() { return IAudioDecoderManager::serviceName().c_str(); }

                    android::binder::Status getAudioDecoderIds( std::vector<IAudioDecoder::Id>* _aidl_return ) override;
                    android::binder::Status getAudioDecoder( const IAudioDecoder::Id& decoderResourceId, android::sp<IAudioDecoder>* _aidl_return ) override;
                };

                class AudioDecoder :public android::BinderService<AudioDecoder>,
                                    public BnAudioDecoder
                {
                public:
                    /* Decoder Read-Only Properties */
                    IAudioDecoder::Id mId;
                    int32_t mSupportsSecure;
                    int32_t mMetricsFrameDecoded;
                    int32_t mMetricsDecodeErrors;
                    int32_t mMetricsFramesDropped;

                    /* Read Write Properties */
                    int32_t mLowLatency;
                    AVSource mAVSource;
                    int32_t mAC4PresentationGroupIndex;
                    android::String16 mAC4PreferredLang1;
                    android::String16 mAC4PreferredLang2;
                    int32_t mAC4AssociatedType;
                    int32_t mAutoSelectionPriority;
                    int32_t mAC4MixerBalance;
                    int32_t mAC4AssociatedAudioMixingEnable;

                    std::vector<::com::rdk::hal::audiodecoder::Codec> mSupportedCodecs;
                    State mAudioDecoderState;
                    std::mutex mStateMutex;

                    /* Supporting variables */
                    bool mEventListener;
                    android::sp<audiodecoder::IAudioDecoderEventListener> mAudioDecoderEventListener;
                public:
                    AudioDecoder(IAudioDecoder::Id id);
                    virtual ~AudioDecoder();

                    void setState(State state);
                    State getState();

                    android::binder::Status getCapabilities( audiodecoder::Capabilities* _aidl_return ) override;
                    android::binder::Status getProperty( audiodecoder::Property property, std::optional<PropertyValue>* _aidl_return ) override;
                    android::binder::Status getState( State* _aidl_return ) override;
                    android::binder::Status open( audiodecoder::Codec codec, bool secure, const android::sp<audiodecoder::IAudioDecoderControllerListener>& audioDecoderControllerListener, android::sp<audiodecoder::IAudioDecoderController>* _aidl_return ) override;
                    android::binder::Status close( const android::sp<audiodecoder::IAudioDecoderController>& audioDecoderController, bool* _aidl_return ) override;
                    android::binder::Status registerEventListener( const android::sp<audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return ) override;
                    android::binder::Status unregisterEventListener( const android::sp<audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return ) override;
                };

                class AudioDecoderController :public android::BinderService<AudioDecoderController>,
                                              public BnAudioDecoderController
                {
                private:
                    IAudioDecoder::Id mId;
                    AudioDecoder *mParentDecoder;
                public:
                    AudioDecoderController(IAudioDecoder::Id id, AudioDecoder *parentDecoder);
                    virtual ~AudioDecoderController();

                    android::binder::Status start() override;
                    android::binder::Status stop() override;
                    android::binder::Status decodeBufferWithMetadata( int64_t bufferHandle, const audiodecoder::InputBufferMetadata& metadata, bool* _aidl_return ) override;
                    android::binder::Status flush( bool reset ) override;
                    android::binder::Status signalDiscontinuity() override;
                    android::binder::Status parseCodecSpecificData( rdk::hal::audiodecoder::CSDAudioFormat csdAudioFormat, const std::vector<uint8_t>& codecData, bool* _aidl_return ) override;
                    android::binder::Status setProperty( audiodecoder::Property property, const PropertyValue& propertyValue, bool* _aidl_return ) override;
                };
            }
        }
    }
}

#endif AUDIODECODER_H

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

/**
 * @addtogroup HPK Hardware Porting Kit
 * @{
 * @par The Hardware Porting Kit
 * HPK is the next evolution of the well-defined Hardware Abstraction Layer
 * (HAL), but augmented with more comprehensive documentation and test suites
 * that OEM or SOC vendors can use to self-certify their ports before taking
 * them to RDKM for validation or to an operator for final integration and
 * deployment. The Hardware Porting Kit effectively enables an OEM and/or SOC
 * vendor to self-certify their own Video Accelerator devices, with minimal RDKM
 * assistance.
 *
 */

/**
 * @addtogroup AUDIO_DECODER Audio Decoder Module
 * @{
 */

/**
 * @addtogroup AUDIO_DECODER_HALTEST Audio Decoder HAL Tests
 * @{
 */

/**
 * @defgroup AUDIO_DECODER_HALTEST_CLASS Audio Decoder HAL Tests helper functions Header File
 * @{
 * @parblock
 *
 * ### Common functions/macros required for the audio module :
 *
 * Common functions/macros required for the audio module .
 *
 * **Pre-Conditions:**  None @n
 * **Dependencies:** None @n
 *
 * @endparblock
 *
 */

/**
 * @file vts_audiodecoder.cpp
 *
 */

#include "vts_audiodecoder.h"
#include "vts_avbuffer.h"

#define MAX_WAIT_TIME       200   /* ms — timeout for error event callbacks */
#define MAX_FRAME_WAIT_TIME 1000  /* ms — timeout for decoded frame output (must survive flush gaps) */

/* Controller listener Class Functions */

/**
 * Convert vector to comma-separated string
 *
 * @param channels Channel information vector
 * @return comma-separated string
 */
string AudioDecoderControllerListener::formatChannelVector(const std::vector<ChannelType>& channels)
{
    std::ostringstream oss;
    for (size_t i = 0; i < channels.size(); ++i) {
        oss << audiodecoder::toString(channels[i]);
        if (i != channels.size() - 1) oss << ", ";
    }
    return oss.str();
}

/**
 * Override Call-back funtion for decoder output
 *
 * @param nsPresentationTime PTS in nanoseconds
 * @param frameBufferHandle private decoded buffer handle
 * @param metadata stream metadata
 * @return Status
 */
Status AudioDecoderControllerListener::onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::optional<audiodecoder::FrameMetadata>& metadata)
{
    UT_LOG_INFO("[Audio DecoderID: %d] onFrameOutput: Received frameoutput [PTS:%lld, handle: %llX]", mDecoderID, nsPresentationTime, frameBufferHandle);

    DecodedAudioFrame_t frame;
    frame.receiveTime = chrono::steady_clock::now();
    frame.framePTS = nsPresentationTime;

    frame.isTunnel = false;
    if (frameBufferHandle == -1)
    {
        frame.isTunnel = true;
    }
    frame.frameBufferHandle = frameBufferHandle;
    frame.isMetaAvailable = false;
    if (metadata.has_value())
    {
        if (metadata->metadata.has_value())
        {
            const auto& pcm = metadata->metadata.value();

            UT_LOG_INFO("[Audio DecoderID: %d] Stream Details [Codec: %s, SampleRate: %d, Channels: %d, channelType: [%s] format: %s planarFormat: %s, isDolbyAtmos: %s]",
                        mDecoderID,
                        audiodecoder::toString(metadata->sourceCodec).c_str(),
                        pcm.sampleRate,
                        pcm.numChannels,
                        formatChannelVector(pcm.channelTypes).c_str(),
                        ::com::rdk::hal::audiodecoder::toString(pcm.format).c_str(),
                        UT_Control_GetMapString(bool_mapTable, pcm.planarFormat),
                        UT_Control_GetMapString(bool_mapTable, metadata->isDolbyAtmos));
        }
        else
        {
            UT_LOG_INFO("[Audio DecoderID: %d] Stream Details [Codec: %s, PCMMetadata: not available, isDolbyAtmos: %s]",
                        mDecoderID,
                        audiodecoder::toString(metadata->sourceCodec).c_str(),
                        UT_Control_GetMapString(bool_mapTable, metadata->isDolbyAtmos));
        }

        UT_LOG_INFO("[Audio DecoderID: %d] Control Parameters [PTS: %lld, discontinuity: %d, EOS: %d, LowLatency: %d, Source: %d, Trim(%lld-%lld)]",
                    mDecoderID, nsPresentationTime, metadata->discontinuity, metadata->endOfStream, metadata->lowLatency,
                    metadata->source, metadata->trimStartNs, metadata->trimEndNs);

        frame.isMetaAvailable = true;

        frame.metadata.sourceCodec = metadata->sourceCodec;
        frame.metadata.isDolbyAtmos = metadata->isDolbyAtmos;
        frame.metadata.trimStartNs = metadata->trimStartNs;
        frame.metadata.trimEndNs = metadata->trimEndNs;
        frame.metadata.pcm_metadata.numChannels = metadata->metadata->numChannels;
        frame.metadata.pcm_metadata.sampleRate = metadata->metadata->sampleRate;
        frame.metadata.pcm_metadata.format = metadata->metadata->format;
        frame.metadata.lowLatency = metadata->lowLatency;
        frame.metadata.endOfStream = metadata->endOfStream;
        frame.metadata.discontinuity = metadata->discontinuity;
        frame.metadata.source = metadata->source;

        frame.frameType = metadata->type;

        // PCM details moved under FrameMetadata::metadata (optional<PCMMetadata>)
        if (metadata->metadata.has_value())
        {
            const auto& pcm = metadata->metadata.value();

            UT_LOG_INFO("[Audio DecoderID: %d] PCM Details [SampleRate: %d, Channels: %d, channelType: [%s] format: %s planarFormat: %s]",
                        mDecoderID,
                        pcm.sampleRate,
                        pcm.numChannels,
                        formatChannelVector(pcm.channelTypes).c_str(),
                        ::com::rdk::hal::audiodecoder::toString(pcm.format).c_str(),
                        UT_Control_GetMapString(bool_mapTable, pcm.planarFormat));

            // Copy PCM fields into your local metadata struct
            frame.metadata.channelTypes = pcm.channelTypes;
            frame.metadata.pcm_metadata.numChannels  = pcm.numChannels;
            frame.metadata.planarFormat = pcm.planarFormat;
            frame.metadata.pcm_metadata.sampleRate   = pcm.sampleRate;
            frame.metadata.pcm_metadata.format = pcm.format;
        }
        else
        {
            frame.metadata.pcm_metadata.numChannels  = 0;
            frame.metadata.pcm_metadata.sampleRate   = 0;
            frame.metadata.planarFormat = false;
            frame.metadata.channelTypes.clear();
        }
        if (metadata->type == com::rdk::hal::audiodecoder::FrameType::SOC_PROPRIETARY)
        {
            UT_LOG_INFO("[Audio DecoderID: %d] FrameType SOC_PROPRIETARY: pass-through output; decoded PCM may not be produced.",
                        mDecoderID);
            frame.socPrivate = metadata->SoCPrivate;
        }
    }
    {
        lock_guard<mutex> lock(mQueueMutex);
        mFrameQueue.push(frame);
    }

    mFrameAvailable.notify_one();

    return ::android::binder::Status::ok();
}

/**
 * Returns the queued frames from onFrameOutput
 *
 * @param frameInfo frame information
 * @param popFrame if true function reads and pop the frame from queue
 *                 otherwise function just reads the frame info
 * @return True: if frame reads successfull, False: if frame is not available
 */
bool AudioDecoderControllerListener::getNextFrame(DecodedAudioFrame_t &frameInfo, int32_t waitTime, bool popFrame)
{
    std::unique_lock<std::mutex> lock(mQueueMutex);

    // Wait until queue is not empty or timeout occurs
    bool gotFrame = mFrameAvailable.wait_for(
        lock,
        std::chrono::milliseconds(waitTime),
        [this]() { return !mFrameQueue.empty(); }
    );

    if (!gotFrame)
    {
        UT_LOG_WARNING("[Audio DecoderID: %d] Decoded frame not received after %d msec", mDecoderID, waitTime);
        return false;
    }

    frameInfo = mFrameQueue.front();
    if (popFrame) {
        mFrameQueue.pop();
    }

    return true;
}

/**
 * Clears the frame queue
 *
 * @param None.
 * @return None
 */
void AudioDecoderControllerListener::resetFrames()
{
    lock_guard<mutex> lock(mQueueMutex);
    while (!mFrameQueue.empty())
    {
        mFrameQueue.pop();
    }
}

/**
 * Clears flags and notifies all mutexes
 *
 * @param None
 * @return None
 */
void AudioDecoderControllerListener::shutdown()
{
    mShutdownFlag = true;
    mFrameAvailable.notify_all();
}

/**
 * Sets the Decoder ID
 *
 * @param id: Decoder ID
 * @return None
 */
void AudioDecoderControllerListener::setDecoderID(int32_t id)
{
    mDecoderID = id;
}

/* Event listener Class Functions */

/**
 * Override Call-back funtion for decoder errors
 *
 * @param vendorErrorCode vendor error code
 * @param errorCode General error code
 * @return Status
 */
Status AudioDecoderEventListener::onDecodeError(audiodecoder::ErrorCode vendorErrorCode, int32_t errorCode)
{
    ErrorDetails_t errorDetails;
    errorDetails.receiveTime = chrono::steady_clock::now();

    UT_LOG_INFO("[Audio DecoderID: %d] Error Callback: Vendor Error Code: %s, errorCode: %d", mDecoderID, audiodecoder::toString(vendorErrorCode).c_str(), errorCode);

    errorDetails.errorCode = errorCode;
    errorDetails.vendorErrorCode = vendorErrorCode;

    {
        lock_guard<mutex> lock(mErrorMutex);
        mErrors.push(errorDetails);
    }
    mErrorAvailable.notify_one();
    return Status::ok();
}

/**
 * Override Call-back funtion for decoder state changes
 *
 * @param oldState Old state
 * @param newState New state
 * @return Status
 */
Status AudioDecoderEventListener::onStateChanged(AudioDecoderState oldState,AudioDecoderState newState)
{
    UT_LOG_INFO("[Audio DecoderID: %d] State Change received: %s -> %s", mDecoderID, audiodecoder::toString(oldState).c_str(), audiodecoder::toString(newState).c_str());
    if(oldState == AudioDecoderState::CLOSED && newState == AudioDecoderState::OPENING)
    {
        mUsedDecoderID = mDecoderID;
    }
    {
        lock_guard<mutex> lock(mMutex);
        mStateChanges.emplace_back(oldState, newState);
        mCurrentState = newState;
    }
    mCV.notify_all(); // Unblock any waiting test code
    return Status::ok();
}

/**
 * Returns the queued eror info from onDecodeError
 *
 * @param errorDetails error information
 * @return True: if frame reads successfull, False: if frame is not available
 */
bool AudioDecoderEventListener::getNextError(ErrorDetails_t &errorDetails)
{
    std::unique_lock<std::mutex> lock(mErrorMutex);

    bool gotError = mErrorAvailable.wait_for(
        lock,
        std::chrono::milliseconds(MAX_WAIT_TIME),
        [this]() { return !mErrors.empty(); }
    );

    if (!gotError) {
        UT_LOG_ERROR("Error event not received after %d msec", MAX_WAIT_TIME);
        return false;
    }

    errorDetails = mErrors.front();
    mErrors.pop();

    return true;
}

/**
 * Wait for the decoder to reach a particular State within a given timeout.
 *
 * @param desiredState The state we want to observe.
 * @param timeout      How long to wait, e.g. chrono::milliseconds(500).
 * @return true if the state was reached; false if we timed out waiting.
 */
bool AudioDecoderEventListener::waitForState(AudioDecoderState desiredState, chrono::milliseconds timeout)
{
    unique_lock<mutex> lock(mMutex);
    // Wait until mCurrentState == desiredState or until timeout
    return mCV.wait_for(lock, timeout, [this, desiredState]() {
        return (mCurrentState == desiredState);
    });
}

/**
 * Clears the state queue
 *
 * @param None.
 * @return None
 */
void AudioDecoderEventListener::resetStateChanges()
{
    lock_guard<mutex> lock(mMutex);
    mStateChanges.clear();
}

/**
 * Checks if the expected states received from the call-back
 *
 * @param expOldState Expected old state
 * @param expNewState Expected new state
 * @return returns if the expected states received or not
 */

bool AudioDecoderEventListener::checkStateTransistion(AudioDecoderState expOldState, AudioDecoderState expNewState)
{
    vector<pair<AudioDecoderState, AudioDecoderState>> stateChange;
    bool foundClosedToOpening = false;

    {
        lock_guard<mutex> lock(mMutex);
        stateChange = mStateChanges;
    }

    for (const auto &[oldState, newState] : stateChange)
    {
        if (oldState == expOldState && newState == expNewState)
        {
            foundClosedToOpening = true;
            break;
        }
    }

    return foundClosedToOpening;
}

/**
 * Clears the error queue
 *
 * @param None.
 * @return None
 */
void AudioDecoderEventListener::resetErrors()
{
    lock_guard<mutex> lock(mErrorMutex);
    while (!mErrors.empty())
    {
        mErrors.pop();
    }
}

void resetDecodeSession(sp<AudioDecoderEventListener> eventListener,
                        sp<AudioDecoderControllerListener> controlListener)
{
    eventListener->resetErrors();
    controlListener->resetFrames();
}

/**
 * Sets the Decoder ID
 *
 * @param id: Decoder ID
 * @return None
 */
void AudioDecoderEventListener::setDecoderID(int32_t id)
{
    mDecoderID = id;
}

int32_t AudioDecoderEventListener::mUsedDecoderID = -1;

void AudioDecoderTest::run(bool validateMetrics)
{
    thread validateThread;

    setupAudioDecoder();

    if (validateMetrics && !mIsSinkAttached)
    {
        validateThread = thread(&AudioDecoderTest::validateDecodedFrame, this);
    }

    startDecoding();

    if (validateThread.joinable())
    {
        validateThread.join();
    }

    teardownDecoder();
}

void AudioDecoderTest::getDecoderMetrics(int32_t &framesDecoded, int32_t &decodeErrors, int32_t &framesDropped)
{
    optional<PropertyValue> propertyValue;

    framesDecoded = decodeErrors = framesDropped = -1;

    AUDIODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, audiodecoder::Property::METRIC_FRAMES_DECODED, propertyValue);
    if (propertyValue.has_value() && propertyValue->value.has_value())
    {
        framesDecoded = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }

    AUDIODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, audiodecoder::Property::METRIC_DECODE_ERRORS, propertyValue);
    if (propertyValue.has_value() && propertyValue->value.has_value())
    {
        decodeErrors = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }

    AUDIODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, audiodecoder::Property::METRIC_FRAMES_DROPPED, propertyValue);
    if (propertyValue.has_value() && propertyValue->value.has_value())
    {
        framesDropped = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }
}

class AVBufferSpaceListener : public BnAVBufferSpaceListener
{
public:

    AVBufferSpaceListener() {};
    virtual ~AVBufferSpaceListener() = default;

    Status onSpaceAvailable() override
    {
        return Status::ok();
    }
};

AudioDecoderTest::AudioDecoderTest(IAudioDecoder::Id decoderId, sp<IAudioDecoder> decoderHandle,
    sp<AudioDecoderControllerListener> controlListener,
    sp<AudioDecoderEventListener> eventListener,
    sp<IAVBuffer> AVBufferManager, IAVBufferHelper* avbufferHelper,
    bool secureDecode, testConfiguration_t config, bool isSinkAttached)
    : mDecoderId(decoderId),
      mDecoderHandle(decoderHandle),
      mControlListener(controlListener),
      mEventListener(eventListener),
      mAVBufferManager(AVBufferManager),
      mAVbufferHelper(avbufferHelper),
      mSecureDecode(secureDecode),
      mTestConfiguration(config),
      mIsSinkAttached(isSinkAttached)
{
    mBufferListener = new AVBufferSpaceListener();
    AVBUFFER_CREATE_AUDIOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, mBufferListener, mAudioPool);
}

AudioDecoderTest::~AudioDecoderTest()
{
    AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, mAudioPool);
}

void AudioDecoderTest::setupAudioDecoder()
{
    mAvSourcePropertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(int32_t(static_cast<int32_t>(AVSource::IP)));

    UT_LOG_DEBUG("[Audio DecoderID: %d] Start of Testing Stream: %s", mDecoderId.value, mTestConfiguration.input.c_str());

    mInputFile.open(mTestConfiguration.input, ios::binary);
    if (!mInputFile)
    {
        UT_LOG_ERROR("[Audio DecoderID: %d] Failed to open file: %s", mDecoderId.value, mTestConfiguration.input.c_str());
        UT_FAIL("Failed to open input file");
        return;
    }

    /* Open the Decoder */
    AUDIODECODER_OPEN_SUCCESS(mDecoderHandle, mTestConfiguration.codec, mSecureDecode, mControlListener, mAudioDecoderController, true);

    /* Set AV Source */
    AUDIODECODER_SETPROPERTY_SUCCESS(mAudioDecoderController, audiodecoder::Property::AV_SOURCE, mAvSourcePropertyValue);

    /* Set audio format */
    AUDIODECODER_SETAUDIOFORMAT_SUCCESS(mAudioDecoderController,
                                        mTestConfiguration.numChannels,
                                        mTestConfiguration.sampleRate);

    /* Start the decoder */
    AUDIODECODER_START_SUCCESS(mDecoderHandle, mAudioDecoderController, true);

    /* Reset shared input frame queue */
    {
        lock_guard<mutex> lock(mInputFrameMutex);
        mInputFrame.clear();
    }
}

void AudioDecoderTest::startDecoding()
{
    int64_t audioBufferHandle;

    /* Decode loop: runs in parallel with validateDecodedFrame */
    const auto& frames = mTestConfiguration.frameDetails;
    for(size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex)
    {
        const auto& [frameSize, framePTS, errorFrame] = frames[frameIndex];
        vector<uint8_t> inputBuffer;

        /* Gapless trim: apply trimStartNs only to the first frame (encoder delay)
         * and trimEndNs only to the last frame (encoder padding).
         * All other frames are passed with zero trim. */
        int32_t trimStartNs = (frameIndex == 0) ? mTestConfiguration.trimStartNs : 0;
        int32_t trimEndNs   = (frameIndex == frames.size() - 1) ? mTestConfiguration.trimEndNs : 0;
        void *mapBuffer = nullptr;
        uint32_t mapSize = 0;
        InputAudioFrame_t inputFrame;

        /* Allocate the audio buffer */
        AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, mAudioPool, frameSize, audioBufferHandle);

        inputBuffer.resize(frameSize);
        mInputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);
        if (mInputFile.eof() || mInputFile.gcount() < frameSize) {
            mInputFile.clear();
            mInputFile.seekg(0, std::ios::beg);
            mInputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);
        }

        if(mSecureDecode)
        {
            if(!mAVbufferHelper->writeSecureHandle(audioBufferHandle, inputBuffer.data(), frameSize))
            {
                UT_LOG_ERROR("[Audio DecoderID: %d] Failed to copy the data to secure buffer", mDecoderId.value);
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, audioBufferHandle);
                continue;
            }
        }
        else
        {
            mapBuffer = mAVbufferHelper->mapHandle(audioBufferHandle, &mapSize);
            memcpy(mapBuffer, inputBuffer.data(), mapSize);
        }

        /* ReStart the decoder */
        if(mTestConfiguration.stopPts == framePTS)
        {
            AUDIODECODER_STOP_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
            AUDIODECODER_START_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
        }

        if(mTestConfiguration.discontinuityPts == framePTS)
        {
            AUDIODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
        }

        if(mTestConfiguration.flushPts == framePTS)
        {
            AUDIODECODER_FLUSH_SUCCESS(mDecoderHandle, mAudioDecoderController, false, true);
            AUDIODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
        }

        if(mTestConfiguration.flushRestPts == framePTS)
        {
            AUDIODECODER_FLUSH_SUCCESS(mDecoderHandle, mAudioDecoderController, true, true);
            AUDIODECODER_START_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
        }

        /* Push input frame to shared queue for latency timing in validate thread */
        inputFrame.sendTime = chrono::steady_clock::now();
        inputFrame.framePTS = framePTS;
        inputFrame.isErrorFrame = errorFrame;
        {
            lock_guard<mutex> lock(mInputFrameMutex);
            mInputFrame.push_back(inputFrame);
        }

        if(mTestConfiguration.eosPts == framePTS)
        {
            UT_LOG_INFO("Signaling EOS at PTS:%lld", framePTS);
            AUDIODECODER_SIGNAL_EOS_SUCCESS(mDecoderHandle, mAudioDecoderController, true);

            /* After EOS, decodeBuffer must be rejected — reuse the already-allocated
             * audioBufferHandle so no extra alloc/free is needed. */
            AUDIODECODER_DECODEBUFFER_FAILURE(mAudioDecoderController, framePTS, audioBufferHandle, trimStartNs, trimEndNs, Status::EX_NONE);

            /* Decoder must be restarted after EOS */
            AUDIODECODER_STOP_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
            AUDIODECODER_START_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
        }

        /* Send the buffer to decoder */
        AUDIODECODER_DECODEBUFFER_SUCCESS(mDecoderId.value, mAudioDecoderController, framePTS, audioBufferHandle, trimStartNs, trimEndNs);

        /* wait time to next frames */
        if(mTestConfiguration.decoderDelay > 0)
            this_thread::sleep_for(chrono::milliseconds(mTestConfiguration.decoderDelay));

        auto endPTS = std::get<1>(mTestConfiguration.frameDetails.back());

        if(framePTS == endPTS)
        {
            UT_LOG_DEBUG("Signalled EOS after the last frame framePTS:%lld, endPTS:%lld", framePTS, endPTS);
            AUDIODECODER_SIGNAL_EOS_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
        }
    }

    UT_LOG_INFO("[Audio DecoderID: %d] End of Testing Stream: %s",
                mDecoderId.value, mTestConfiguration.input.c_str());
}

void AudioDecoderTest::teardownDecoder()
{
    /* Stop the decoder */
    AUDIODECODER_STOP_SUCCESS(mDecoderHandle, mAudioDecoderController, true);

    /* Close the decoder */
    AUDIODECODER_CLOSE_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
}

void AudioDecoderTest::validateDecodedFrame()
{
    DecodedAudioFrame_t decodedFrame;
    int32_t framesDecoded = 0;
    int32_t eosFramesReceived = 0;
    int32_t discontinuityFramesReceived = 0;
    bool isPassThrough = false;

    /* Pre-compute decode error count — static, does not depend on loop results */
    int32_t decodeErrors = static_cast<int32_t>(count_if(
        mTestConfiguration.frameDetails.begin(),
        mTestConfiguration.frameDetails.end(),
        [](const auto& f) { return get<2>(f); }));

    while (mControlListener->getNextFrame(decodedFrame, MAX_FRAME_WAIT_TIME, true))
    {
        /* Block 1: frame data — only when buffer is valid */
        if (decodedFrame.frameBufferHandle != -1)
        {
            framesDecoded++;
            bool isPassThroughFrame = false;

            /* SOC_PROPRIETARY (pass-through): log once, skip decoded-frame validation */
            if (decodedFrame.isMetaAvailable &&
                decodedFrame.frameType == com::rdk::hal::audiodecoder::FrameType::SOC_PROPRIETARY)
            {
                if (!isPassThrough)
                {
                    UT_LOG_WARNING("[Audio DecoderID: %d] SOC_PROPRIETARY(pass-through) output detected. "
                                   "Skipping decoded-frame validation.", mDecoderId.value);
                    isPassThrough = true;
                }

                isPassThroughFrame = true;
            }
            else
            {
                /* Latency measurement — decoded PTS must match a submitted input PTS */
                lock_guard<mutex> lock(mInputFrameMutex);
                auto inputIt = find_if(mInputFrame.begin(), mInputFrame.end(),
                    [&decodedFrame](const InputAudioFrame_t& f) {
                        return f.framePTS == decodedFrame.framePTS;
                    });
                if (inputIt == mInputFrame.end())
                {
                    UT_LOG_ERROR("[Audio DecoderID: %d] Out Frame [PTS: %lld] does not match any submitted Input Frame PTS",
                                 mDecoderId.value, decodedFrame.framePTS);
                    UT_ASSERT_TRUE(false);
                }
                else
                {
                    auto duration = chrono::duration_cast<chrono::microseconds>(
                        decodedFrame.receiveTime - inputIt->sendTime);
                    UT_LOG_INFO("[Audio DecoderID: %d] Decode Time in micro second [PTS: %lld]: %lld",
                                mDecoderId.value, decodedFrame.framePTS, duration.count());
                    UT_ASSERT_TRUE(duration.count() <= mTestConfiguration.decodeTime);
                }
            }

            /* Free decoded output buffer — both SOC_PROPRIETARY and normal path */
            bool isBufferValid;

            AVBUFFER_ISVALID_SUCCESS(mAVBufferManager, decodedFrame.frameBufferHandle, isBufferValid);

            if (isBufferValid)
            {
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, decodedFrame.frameBufferHandle);
            }

            if (isPassThroughFrame)
            {
                continue;
            }
        }

        /* Block 2: metadata — separate from buffer handling */
        if (decodedFrame.isMetaAvailable)
        {
            UT_ASSERT_EQUAL(decodedFrame.metadata.sourceCodec, mTestConfiguration.codec);
            UT_ASSERT_EQUAL(int32_t(static_cast<int32_t>(decodedFrame.metadata.source)),
                            mAvSourcePropertyValue.value->get<PropertyValue::Value::Tag::intValue>());

            if (decodedFrame.metadata.discontinuity)
            {
                discontinuityFramesReceived++;
            }

            if (decodedFrame.metadata.endOfStream)
            {
                eosFramesReceived++;
            }

            /* Trim metadata validation  */
            if (mTestConfiguration.trimStartNs > 0 || mTestConfiguration.trimEndNs > 0)
            {
                int32_t expectedTrimStart = (framesDecoded == 1) ? mTestConfiguration.trimStartNs : 0;
                int32_t expectedTrimEnd   = (framesDecoded == (int32_t)mTestConfiguration.frameDetails.size()) ? mTestConfiguration.trimEndNs : 0;
                UT_ASSERT_EQUAL(expectedTrimStart, decodedFrame.metadata.trimStartNs);
                UT_ASSERT_EQUAL(expectedTrimEnd,   decodedFrame.metadata.trimEndNs);
            }
        }
    }

    /* Post-loop validations */
    int32_t totalErrors = mEventListener->getErrorCount();

    /* expectedFrames: passthrough counts all frames; non-passthrough subtracts error frames */
    int32_t expectedFrames = isPassThrough ? mTestConfiguration.numOfFrames
                                           : mTestConfiguration.numOfFrames - decodeErrors;

    /* Frame count check — common to both passthrough and non-passthrough.
     * Flush discards queued frames mid-stream so use relaxed (<=) when flush is configured. */
    if ((mTestConfiguration.flushPts == -1) && (mTestConfiguration.flushRestPts == -1))
    {
        UT_ASSERT_EQUAL(framesDecoded, expectedFrames);
    }
    else
    {
        UT_ASSERT_TRUE(framesDecoded <= expectedFrames);
    }

    /* SOC_PROPRIETARY pass-through: frame count already validated above, no further checks needed */
    if (isPassThrough)
    {
        return;
    }

    /* Discontinuity/flush validation: at least one but not all frames should carry
     * the discontinuity flag when a discontinuity or flush point is configured */
    if ((mTestConfiguration.discontinuityPts != -1) || (mTestConfiguration.flushPts != -1))
    {
        UT_ASSERT_TRUE((discontinuityFramesReceived > 0 && discontinuityFramesReceived < expectedFrames));
    }

    /* EOS validation: one EOS always expected at stream end; a second when eosPts is configured */
    int32_t expectedEOSFrames = 1;

    if (mTestConfiguration.eosPts != -1)
    {
        expectedEOSFrames++;
    }

    UT_ASSERT_EQUAL(eosFramesReceived, expectedEOSFrames);

    /* Error count validation */
    UT_ASSERT_EQUAL(totalErrors, decodeErrors);

    /* Decoder metrics validation — -1 means vendor has not implemented the metric, skip.
     * When flush is configured use relaxed (<=) checks. */
    int32_t metricsFrameDecoded, metricsDecodeErrors, metricsFramesDropped;

    getDecoderMetrics(metricsFrameDecoded, metricsDecodeErrors, metricsFramesDropped);

    UT_LOG_DEBUG("Final Decoder Metrics: Codec: %d, Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
        mTestConfiguration.codec, metricsFrameDecoded, metricsDecodeErrors, metricsFramesDropped);

    if ((mTestConfiguration.flushPts == -1) && (mTestConfiguration.flushRestPts == -1))
    {
        if (metricsFrameDecoded != -1)
        {
            UT_ASSERT_EQUAL(metricsFrameDecoded, expectedFrames);
        }
        if (metricsDecodeErrors != -1)
        {
            UT_ASSERT_EQUAL(metricsDecodeErrors, decodeErrors);
        }
        if (metricsFramesDropped != -1)
        {
            UT_ASSERT_EQUAL(metricsFramesDropped, 0);
        }
    }
    else
    {
        if (metricsFrameDecoded != -1)
        {
            UT_ASSERT_TRUE(metricsFrameDecoded <= expectedFrames);
        }
        if (metricsDecodeErrors != -1)
        {
            UT_ASSERT_TRUE(metricsDecodeErrors <= decodeErrors);
        }
        if (metricsFramesDropped != -1)
        {
            UT_ASSERT_TRUE(metricsFramesDropped >= 0);
        }
    }
}

/** @} */ // End of AUDIO_DECODER_HALTEST_HEADER
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK

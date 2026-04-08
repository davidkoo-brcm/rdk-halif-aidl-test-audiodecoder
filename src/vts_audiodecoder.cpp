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

#define MAX_WAIT_TIME 200

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
    decodeAndConsumeAudioStreams(validateMetrics);
}

bool AudioDecoderTest::getDecoderMetrics(int32_t &framesDecoded, int32_t &decodeErrors, int32_t &framesDropped)
{
    optional<PropertyValue> propertyValue;

    AUDIODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, audiodecoder::Property::METRIC_FRAMES_DECODED, propertyValue);
    if (!propertyValue.has_value() || !propertyValue->value.has_value())
    {
        return false;
    }
    framesDecoded = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

    AUDIODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, audiodecoder::Property::METRIC_DECODE_ERRORS, propertyValue);
    if (!propertyValue.has_value() || !propertyValue->value.has_value())
    {
        return false;
    }
    decodeErrors = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

    AUDIODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, audiodecoder::Property::METRIC_FRAMES_DROPPED, propertyValue);
    if (!propertyValue.has_value() || !propertyValue->value.has_value())
    {
        return false;
    }
    framesDropped = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

    return true;
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
      mIsSinkAttached(isSinkAttached),
      mDecodeComplete(false)
{
    mBufferListener = new AVBufferSpaceListener();
    AVBUFFER_CREATE_AUDIOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, mBufferListener, mAudioPool);
}

AudioDecoderTest::~AudioDecoderTest()
{
    AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, mAudioPool);
}

void AudioDecoderTest::decodeAndConsumeAudioStreams(bool validateMetrics)
{
    try {
        sp<IAudioDecoderController> audioDecoderController;
        int64_t audioBufferHandle;
        int32_t startFramesDecoded, startFrameErrors, startFramesDropped;
        PropertyValue avSourcePropertyValue;

        avSourcePropertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(int32_t(static_cast<int32_t>(AVSource::IP)));

        UT_LOG_WARNING("[Audio DecoderID: %d] Start of Testing Stream: %s", mDecoderId.value, mTestConfiguration.input.c_str());

        ifstream inputFile(mTestConfiguration.input, ios::binary);
        if (!inputFile)
        {
            UT_LOG_ERROR("[Audio DecoderID: %d] Failed to open file: %s", mDecoderId.value, mTestConfiguration.input.c_str());
            UT_FAIL("Failed to open input file");
            return;
        }

        /* Open the Decoder */
        AUDIODECODER_OPEN_SUCCESS(mDecoderHandle, mTestConfiguration.codec, mSecureDecode, mControlListener, audioDecoderController, true);

        /* Set AV Source */
        AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, audiodecoder::Property::AV_SOURCE, avSourcePropertyValue);

        /* Set audio format */
        AUDIODECODER_SETAUDIOFORMAT_SUCCESS(audioDecoderController,
                                            mTestConfiguration.numChannels,
                                            mTestConfiguration.sampleRate);

        /* Start the decoder */
        AUDIODECODER_START_SUCCESS(mDecoderHandle, audioDecoderController, true);

        /* Get decoder metrics before decoding */
        UT_ASSERT_TRUE(getDecoderMetrics(startFramesDecoded, startFrameErrors, startFramesDropped));

        /* Reset shared input frame queue */
        {
            lock_guard<mutex> lock(mInputFrameMutex);
            mInputFrame.clear();
            mDecodeComplete = false;
        }

        /* Launch validate metrics thread in parallel before the decode loop.
         * Only runs when validateMetrics is true and no sink is attached. */
        std::thread validateThread;
        /* RAII guard: ensures validateThread is always joined on every exit path
         * (normal return, early return, or exception).  Without this, an exception
         * thrown after the thread starts would destroy a joinable std::thread and
         * call std::terminate during stack unwinding.  The guard also sets
         * mDecodeComplete and signals the CV so the thread can unblock if it is
         * still waiting for input frames when an exception fires mid-loop. */
        struct JoinOnExit {
            std::thread& t;
            std::mutex& mtx;
            bool& done;
            std::condition_variable& cv;
            ~JoinOnExit() {
                { std::lock_guard<std::mutex> lk(mtx); done = true; }
                cv.notify_all();
                if (t.joinable()) t.join();
            }
        } validateThreadGuard{validateThread, mInputFrameMutex, mDecodeComplete, mInputFrameCV};

        if (validateMetrics && !mIsSinkAttached)
        {
            validateThread = std::thread(&AudioDecoderTest::validateMetricsThread, this,
                                         avSourcePropertyValue,
                                         startFramesDecoded, startFrameErrors, startFramesDropped);
        }

        /* Decode loop: runs in parallel with validateMetricsThread */
        for(const auto& [frameSize, framePTS, errorFrame]: mTestConfiguration.frameDetails)
        {
            vector<uint8_t> inputBuffer;
            int32_t trimEndNs = mTestConfiguration.trimEndNs;
            int32_t trimStartNs = mTestConfiguration.trimStartNs;
            void *mapBuffer = nullptr;
            uint32_t mapSize = 0;
            InputAudioFrame_t inputFrame;
            bool isValidBuffer = false;

            /* Allocate the audio buffer */
            AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, mAudioPool, frameSize, audioBufferHandle);

            inputBuffer.resize(frameSize);
            inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);
            if (inputFile.eof() || inputFile.gcount() < frameSize) {
                inputFile.clear();
                inputFile.seekg(0, std::ios::beg);
                inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);
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
                AUDIODECODER_STOP_SUCCESS(mDecoderHandle, audioDecoderController, true);
                AUDIODECODER_START_SUCCESS(mDecoderHandle, audioDecoderController, true);
            }

            if(mTestConfiguration.discontinuityPts == framePTS)
            {
                AUDIODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, audioDecoderController, true);
            }

            if(mTestConfiguration.flushPts == framePTS)
            {
                AUDIODECODER_FLUSH_SUCCESS(mDecoderHandle, audioDecoderController, false, true);
            }

            if(mTestConfiguration.flushRestPts == framePTS)
            {
                AUDIODECODER_FLUSH_SUCCESS(mDecoderHandle, audioDecoderController, true, true);
                AUDIODECODER_START_SUCCESS(mDecoderHandle, audioDecoderController, true);
            }

            /* Push input frame to shared queue before decode so validate thread
             * can start waiting for the corresponding decoded output */
            inputFrame.sendTime = chrono::steady_clock::now();
            inputFrame.framePTS = framePTS;
            inputFrame.isErrorFrame = errorFrame;
            {
                lock_guard<mutex> lock(mInputFrameMutex);
                mInputFrame.push_back(inputFrame);
            }
            mInputFrameCV.notify_one();

            /* Send the buffer to decoder */
            AUDIODECODER_DECODEBUFFER_SUCCESS(mDecoderId.value, audioDecoderController, framePTS, audioBufferHandle, trimStartNs, trimEndNs);

            if(mTestConfiguration.eosPts == framePTS)
            {
                UT_LOG_INFO("Signaling EOS at PTS:%lld", framePTS);
                AUDIODECODER_SIGNAL_EOS_SUCCESS(mDecoderHandle, audioDecoderController, true);

                /* After EOS, decodeBuffer must be rejected with EX_ILLEGAL_STATE.
                 * A fresh buffer is allocated for this test since audioBufferHandle
                 * was already submitted to the HAL and is no longer owned by VTS. */
                int64_t eosTestHandle = -1;
                AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, mAudioPool, 1, eosTestHandle);
                AUDIODECODER_DECODEBUFFER_FAILURE(audioDecoderController, framePTS, eosTestHandle, trimStartNs, trimEndNs, Status::EX_ILLEGAL_STATE);
                /* HAL rejected the buffer — VTS must free it */
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, eosTestHandle);

                /* Decoder must be restarted after EOS */
                AUDIODECODER_STOP_SUCCESS(mDecoderHandle, audioDecoderController, true);
                AUDIODECODER_START_SUCCESS(mDecoderHandle, audioDecoderController, true);
            }

            /* Validate that the input buffer has been released by the decoder */
            AVBUFFER_ISVALID_SUCCESS(mAVBufferManager, audioBufferHandle, isValidBuffer);
            if(isValidBuffer)
            {
                UT_LOG_ERROR("output buffer not released by decoder [handle:0x%llx]", audioBufferHandle);
            } else {
                UT_LOG_INFO("output buffer released by decoder [handle:0x%llx]", audioBufferHandle);
            }

            /* wait time to next frames */
            if(mTestConfiguration.decoderDelay > 0)
                this_thread::sleep_for(chrono::milliseconds(mTestConfiguration.decoderDelay));

            auto endPTS = std::get<1>(mTestConfiguration.frameDetails.back());

            if(framePTS == endPTS)
            {
                UT_LOG_DEBUG("Signalled EOS after the last frame framePTS:%lld, endPTS:%lld", framePTS, endPTS);
                AUDIODECODER_SIGNAL_EOS_SUCCESS(mDecoderHandle, audioDecoderController, true);
            }
        }

        /* Signal validate thread that no more input frames will be produced */
        {
            lock_guard<mutex> lock(mInputFrameMutex);
            mDecodeComplete = true;
        }
        mInputFrameCV.notify_all();

        /* validateThreadGuard joins validateThread here on the normal exit path */

        /* Stop the decoder */
        AUDIODECODER_STOP_SUCCESS(mDecoderHandle, audioDecoderController, true);

        /* Close the decoder */
        AUDIODECODER_CLOSE_SUCCESS(mDecoderHandle, audioDecoderController, true);

        UT_LOG_INFO("[Audio DecoderID: %d] End of Testing Stream: %s",
                    mDecoderId.value, mTestConfiguration.input.c_str());
    }
    catch (const std::exception& ex) {
        UT_LOG_ERROR("[Audio DecoderID: %d] Exception in decodeAndConsumeAudioStreams: %s",
                     mDecoderId.value, ex.what());
        UT_ASSERT_TRUE(false);
    }
    catch (...) {
        UT_LOG_ERROR("[Audio DecoderID: %d] Unknown exception in decodeAndConsumeAudioStreams",
                     mDecoderId.value);
        UT_ASSERT_TRUE(false);
    }
}

void AudioDecoderTest::validateMetricsThread(PropertyValue avSourcePropertyValue,
                                              int32_t startFramesDecoded,
                                              int32_t startFrameErrors,
                                              int32_t startFramesDropped)
{
    int32_t inputFrameIndex = 0;
    int32_t errorFrames = 0;
    int32_t framesDecoded = 0;
    int32_t eosCount = 0;
    bool firstDecodedFrameUsed = false;
    DecodedAudioFrame_t firstDecodedFrame;
    int32_t endFramesDecoded, endFrameErrors, endFramesDropped;

    /* Wait until the decode loop has enqueued at least one input frame before
     * polling for decoded output.  The thread is started before any decoding
     * work begins, so without this gate the MAX_WAIT_TIME window could expire
     * before the first frame has even been submitted to the HAL. */
    {
        unique_lock<mutex> lock(mInputFrameMutex);
        mInputFrameCV.wait(lock, [this]{ return !mInputFrame.empty() || mDecodeComplete; });
    }

    /* Peek first decoded frame to detect SOC_PROPRIETARY pass-through */
    bool haveFirst = mControlListener->getNextFrame(firstDecodedFrame, MAX_WAIT_TIME, false);
    if (!haveFirst)
    {
        UT_LOG_WARNING("[Audio DecoderID: %d] No initial decoded frame received within %d ms "
                       "after first buffer was submitted — skipping decoded-frame and metrics validation.",
                       mDecoderId.value, MAX_WAIT_TIME);
        return;
    }

    if (firstDecodedFrame.isMetaAvailable &&
        firstDecodedFrame.frameType == com::rdk::hal::audiodecoder::FrameType::SOC_PROPRIETARY)
    {
        UT_LOG_WARNING("[Audio DecoderID: %d] SOC_PROPRIETARY(pass-through) output detected. "
                       "Skipping decoded-frame and metrics validation.", mDecoderId.value);
        /* Drain remaining input frames, waiting for decode to complete */
        while (true)
        {
            unique_lock<mutex> lock(mInputFrameMutex);
            mInputFrameCV.wait(lock, [this]{ return !mInputFrame.empty() || mDecodeComplete; });
            if (mInputFrame.empty()) break;
            mInputFrame.pop_front();
        }
        // Drain all output frames and free their buffer handles so the pool can be destroyed.
        // The decoder passes input handles through as output (no PCM decode), so VTS must free them.
        // The first frame was only peeked (not popped), so it is still in the queue and handled here.
        DecodedAudioFrame_t drainFrame;
        while (mControlListener->getNextFrame(drainFrame, MAX_WAIT_TIME, true))
        {
            if (!drainFrame.isTunnel && drainFrame.frameBufferHandle != -1)
            {
                bool isBufferValid;
                AVBUFFER_ISVALID_SUCCESS(mAVBufferManager, drainFrame.frameBufferHandle, isBufferValid);
                if (isBufferValid)
                {
                    AVBUFFER_FREE_SUCCESS(mAVBufferManager, drainFrame.frameBufferHandle);
                }
            }
        }
        return;
    }

    /* firstDecodedFrame was peeked above and is still at the head of the queue.
     * The validation loop pops it only once the current input PTS is confirmed to
     * have a corresponding output — trim/missed inputs produce no decoded frame and
     * must not advance the queue head. */

    /* Consume and validate each input frame in order */
    while (true)
    {
        InputAudioFrame_t inputFrame;

        {
            unique_lock<mutex> lock(mInputFrameMutex);
            mInputFrameCV.wait(lock, [this]{ return !mInputFrame.empty() || mDecodeComplete; });
            if (mInputFrame.empty() && mDecodeComplete) break;
            if (mInputFrame.empty()) continue;
            inputFrame = mInputFrame.front();
            mInputFrame.pop_front();
        }

        inputFrameIndex++;

        /* Error frames: wait for the error callback and log timing */
        if (inputFrame.isErrorFrame)
        {
            ErrorDetails_t errorDetails;
            errorFrames++;
            if (!mEventListener->getNextError(errorDetails))
            {
                UT_ASSERT_TRUE(false);
            }
            else
            {
                auto duration = chrono::duration_cast<chrono::microseconds>(
                    errorDetails.receiveTime - inputFrame.sendTime);
                UT_LOG_INFO("[Audio DecoderID: %d] Error Frame Decode Time in micro second [PTS: %lld]: %lld",
                            mDecoderId.value, inputFrame.framePTS, duration.count());
            }
            continue;
        }

        /* Peek at the next decoded output frame — do not pop yet.
         * We defer the pop until PTS alignment is confirmed.  Trim and missed input
         * frames produce no decoded output; popping before that check would consume
         * a frame that belongs to a later input and permanently desync the pairing. */
        DecodedAudioFrame_t decodedFrame;
        if (!firstDecodedFrameUsed)
        {
            /* firstDecodedFrame was already peeked at function entry and is still
             * at the head of the queue; use the cached copy without a second call. */
            decodedFrame = firstDecodedFrame;
        }
        else
        {
            if (!mControlListener->getNextFrame(decodedFrame, MAX_WAIT_TIME, false))
            {
                UT_LOG_ERROR("[Audio DecoderID: %d] onFrameOutput callback not received...", mDecoderId.value);
                UT_ASSERT_TRUE(false);
                break;
            }
        }

        /* Flush scenario: the input falls within the flushed range, but whether it
         * produced decoded output depends on pipeline timing.
         *   - decoded PTS == input PTS: HAL decoded the frame before the flush arrived
         *     → pop and free the decoded buffer.
         *   - decoded PTS > input PTS:  HAL flushed the frame before decoding it
         *     → no decoded frame to consume; leave the queue untouched. */
        if (mTestConfiguration.flushPts > 0 || mTestConfiguration.flushRestPts > 0)
        {
            if (inputFrame.framePTS < mTestConfiguration.flushPts ||
                (mTestConfiguration.flushRestPts > 0 && inputFrame.framePTS < mTestConfiguration.flushRestPts))
            {
                if (inputFrame.framePTS < decodedFrame.framePTS)
                {
                    UT_LOG_INFO("[Audio DecoderID: %d] Frame [PTS: %lld] was flushed before decoding — no output to consume",
                                mDecoderId.value, inputFrame.framePTS);
                    continue;
                }
                mControlListener->getNextFrame(decodedFrame, MAX_WAIT_TIME, true);
                firstDecodedFrameUsed = true;
                UT_LOG_INFO("[Audio DecoderID: %d] Frame [PTS: %lld] is flushed - skipping metadata validation",
                            mDecoderId.value, inputFrame.framePTS);
                if (!decodedFrame.isTunnel && decodedFrame.frameBufferHandle != -1)
                {
                    bool isBufferValid;
                    AVBUFFER_ISVALID_SUCCESS(mAVBufferManager, decodedFrame.frameBufferHandle, isBufferValid);
                    if (isBufferValid)
                    {
                        AVBUFFER_FREE_SUCCESS(mAVBufferManager, decodedFrame.frameBufferHandle);
                    }
                }
                continue;
            }
        }

        /* Validate metadata */
        if (inputFrameIndex == 1)
        {
            UT_ASSERT_TRUE(decodedFrame.isMetaAvailable);
        }

        if (decodedFrame.isMetaAvailable)
        {
            UT_ASSERT_EQUAL(decodedFrame.metadata.sourceCodec, mTestConfiguration.codec);
            UT_ASSERT_EQUAL(int32_t(static_cast<int32_t>(decodedFrame.metadata.source)),
                            avSourcePropertyValue.value->get<PropertyValue::Value::Tag::intValue>());
        }

        if (mTestConfiguration.discontinuityPts == decodedFrame.framePTS)
        {
            UT_ASSERT_TRUE(decodedFrame.isMetaAvailable && decodedFrame.metadata.discontinuity);
        }

        if ((mTestConfiguration.trimEndNs - mTestConfiguration.trimStartNs) > 0)
        {
            UT_ASSERT_TRUE(decodedFrame.isMetaAvailable &&
                           (mTestConfiguration.trimStartNs == decodedFrame.metadata.trimStartNs));
            UT_ASSERT_TRUE(decodedFrame.isMetaAvailable &&
                           (mTestConfiguration.trimEndNs == decodedFrame.metadata.trimEndNs));

            if (inputFrame.framePTS < decodedFrame.framePTS)
            {
                UT_LOG_INFO("[Audio DecoderID: %d] Frame [PTS: %lld] is Trimmed",
                            mDecoderId.value, inputFrame.framePTS);
                continue;
            }
        }

        if (inputFrame.framePTS < decodedFrame.framePTS)
        {
            UT_LOG_ERROR("[Audio DecoderID: %d] Frame [PTS: %lld] is Missed",
                         mDecoderId.value, inputFrame.framePTS);
            UT_ASSERT_TRUE(false);
            continue;  /* decoded frame stays in queue — belongs to a later input */
        }

        /* PTS alignment confirmed — pop the peeked decoded frame */
        mControlListener->getNextFrame(decodedFrame, MAX_WAIT_TIME, true);
        firstDecodedFrameUsed = true;

        /* Count EOS frames in decoded output */
        if (decodedFrame.isMetaAvailable && decodedFrame.metadata.endOfStream)
        {
            eosCount++;
        }

        framesDecoded++;

        if (inputFrame.framePTS != decodedFrame.framePTS)
        {
            UT_LOG_ERROR("[Audio DecoderID: %d] Input Frame [PTS: %lld] and Out Frame [PTS: %lld] are not matching",
                         mDecoderId.value, inputFrame.framePTS, decodedFrame.framePTS);
            UT_ASSERT_TRUE(false);
            continue;
        }

        auto duration = chrono::duration_cast<chrono::microseconds>(
            decodedFrame.receiveTime - inputFrame.sendTime);
        UT_LOG_INFO("[Audio DecoderID: %d] Decode Time in micro second [PTS: %lld]: %lld",
                    mDecoderId.value, decodedFrame.framePTS, duration.count());
        UT_ASSERT_TRUE(duration.count() <= mTestConfiguration.decodeTime);

        /* Free the decoded output buffer handle */
        if (!decodedFrame.isTunnel && decodedFrame.frameBufferHandle != -1)
        {
            bool isBufferValid;
            AVBUFFER_ISVALID_SUCCESS(mAVBufferManager, decodedFrame.frameBufferHandle, isBufferValid);
            if (isBufferValid)
            {
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, decodedFrame.frameBufferHandle);
            }
        }
    }

    /* EOS count validation: two EOS signals are expected when eosPts is configured
     * (once at eosPts and once at the last frame) */
    if (mTestConfiguration.eosPts > 0)
    {
        UT_ASSERT_EQUAL(eosCount, 2);
    }

    /* Get decoder metrics after decoding and validate against counters */
    UT_ASSERT_TRUE(getDecoderMetrics(endFramesDecoded, endFrameErrors, endFramesDropped));

    if (startFramesDecoded >= 0 && endFramesDecoded >= 0)
    {
        UT_ASSERT_TRUE((endFramesDecoded - startFramesDecoded) == framesDecoded);
    }
    if (startFrameErrors >= 0 && endFrameErrors >= 0)
    {
        UT_ASSERT_TRUE((endFrameErrors - startFrameErrors) == errorFrames);
    }
    if (startFramesDropped >= 0 && endFramesDropped >= 0)
    {
        UT_ASSERT_TRUE((endFramesDropped - startFramesDropped) == 0);
    }
}

/** @} */ // End of AUDIO_DECODER_HALTEST_HEADER
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK

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
#define PTS_DELTA_NS        10    /* ns — acceptable tolerance for PTS spacing between decoded frames */

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
 * @param frameAVBufferHandle private decoded buffer handle
 * @param metadata stream metadata
 * @return Status
 */
Status AudioDecoderControllerListener::onFrameOutput(int64_t nsPresentationTime, int64_t frameAVBufferHandle, const ::optional<audiodecoder::FrameMetadata>& metadata)
{
    UT_LOG_INFO("[Audio DecoderID: %d] onFrameOutput: Received frameoutput [PTS:%lld, handle: %llX]", mDecoderID, nsPresentationTime, frameAVBufferHandle);

    DecodedAudioFrame_t frame;
    frame.receiveTime = chrono::steady_clock::now();
    frame.framePTS = nsPresentationTime;

    frame.isTunnel = false;
    if (frameAVBufferHandle == -1)
    {
        frame.isTunnel = true;
    }
    frame.frameBufferHandle = frameAVBufferHandle;
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
            // Copy PCM fields into your local metadata struct
            frame.metadata.channelTypes = pcm.channelTypes;
            frame.metadata.pcm_metadata.numChannels  = pcm.numChannels;
            frame.metadata.planarFormat = pcm.planarFormat;
            frame.metadata.pcm_metadata.sampleRate   = pcm.sampleRate;
            frame.metadata.pcm_metadata.format = pcm.format;
        }
        else
        {
            UT_LOG_INFO("[Audio DecoderID: %d] Stream Details [Codec: %s, PCMMetadata: not available, isDolbyAtmos: %s]",
                        mDecoderID,
                        audiodecoder::toString(metadata->sourceCodec).c_str(),
                        UT_Control_GetMapString(bool_mapTable, metadata->isDolbyAtmos));
            frame.metadata.pcm_metadata.numChannels  = 0;
            frame.metadata.pcm_metadata.sampleRate   = 0;
            frame.metadata.planarFormat = false;
            frame.metadata.channelTypes.clear();
        }

        UT_LOG_INFO("[Audio DecoderID: %d] Control Parameters [PTS: %lld, discontinuity: %d, EOS: %d, LowLatency: %d, Source: %d, Trim(%lld-%lld)]",
                    mDecoderID, nsPresentationTime, metadata->discontinuity, metadata->endOfStream, metadata->lowLatency,
                    metadata->source, metadata->trimStartNs, metadata->trimEndNs);

        frame.isMetaAvailable = true;

        frame.metadata.sourceCodec = metadata->sourceCodec;
        frame.metadata.isDolbyAtmos = metadata->isDolbyAtmos;
        frame.metadata.trimStartNs = metadata->trimStartNs;
        frame.metadata.trimEndNs = metadata->trimEndNs;
        frame.metadata.lowLatency = metadata->lowLatency;
        frame.metadata.endOfStream = metadata->endOfStream;
        frame.metadata.discontinuity = metadata->discontinuity;
        frame.metadata.source = metadata->source;

        frame.frameType = metadata->type;

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
    mDecodeBufferCV.notify_all();
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

/**
 * onDecodeBufferAvailable — fires once when input queue transitions from full to available.
 * Sets mDecodeBufferAvailableFlag and unblocks any thread waiting in waitForDecodeBufferAvailable().
 *
 * @return Status
 */
Status AudioDecoderControllerListener::onDecodeBufferAvailable()
{
    UT_LOG_INFO("[Audio DecoderID: %d] onDecodeBufferAvailable: input queue has space", mDecoderID);
    mDecodeBufferAvailableCount++;
    {
        lock_guard<mutex> lock(mDecodeBufferMutex);
        mDecodeBufferAvailableFlag = true;
    }
    mDecodeBufferCV.notify_one();
    return Status::ok();
}

/**
 * Wait for back-pressure relief. Returns true if onDecodeBufferAvailable() fired within waitTimeMs.
 *
 * @param waitTimeMs maximum time to wait in milliseconds
 * @return true if callback received, false on timeout
 */
bool AudioDecoderControllerListener::waitForDecodeBufferAvailable(int32_t waitTimeMs)
{
    unique_lock<mutex> lock(mDecodeBufferMutex);
    return mDecodeBufferCV.wait_for(
        lock,
        chrono::milliseconds(waitTimeMs),
        [this]() { return mDecodeBufferAvailableFlag.load(); }
    );
}

/**
 * Reset the decode-buffer-available flag before a decode call that may block.
 *
 * @return None
 */
void AudioDecoderControllerListener::resetDecodeBufferAvailable()
{
    lock_guard<mutex> lock(mDecodeBufferMutex);
    mDecodeBufferAvailableFlag = false;
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
    controlListener->resetDecodeBufferAvailable();
    controlListener->resetDecodeBufferAvailableCount();
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
    if (!mAVbufferHelper)
    {
        UT_ASSERT_TRUE(false) << "AVBufferHelper is not initialized";
        return;
    }

    setupAudioDecoder();

    thread validateThread;

    if (validateMetrics && !mIsSinkAttached && !mTestConfiguration.onDecodeBufAvailTest)
        validateThread = thread(&AudioDecoderTest::validateDecodedFrame, this);

    startDecoding();

    if (mTestConfiguration.onDecodeBufAvailTest)
        UT_ASSERT_TRUE(mControlListener->getDecodeBufferAvailableCount() >= 1);

    if (validateMetrics && !mIsSinkAttached && mTestConfiguration.onDecodeBufAvailTest)
        validateThread = thread(&AudioDecoderTest::validateDecodedFrame, this);

    if (validateThread.joinable())
        validateThread.join();

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

bool AudioDecoderTest::prepareInputBuffer(int32_t frameSize, int64_t& audioBufferHandle)
{
    vector<uint8_t> inputBuffer(frameSize);
    void* mapBuffer = nullptr;
    uint32_t mapSize = 0;

    AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, mAudioPool, frameSize, audioBufferHandle);

    mInputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);
    if (mInputFile.eof() || mInputFile.gcount() < frameSize)
    {
        mInputFile.clear();
        mInputFile.seekg(0, std::ios::beg);
        mInputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);
    }

    if (mSecureDecode)
    {
        if (!mAVbufferHelper->writeSecureHandle(audioBufferHandle, inputBuffer.data(), frameSize))
        {
            UT_LOG_ERROR("[Audio DecoderID: %d] Failed to copy the data to secure buffer", mDecoderId.value);
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, audioBufferHandle);
            return false;
        }
    }
    else
    {
        mapBuffer = mAVbufferHelper->mapHandle(audioBufferHandle, &mapSize);
        if (mapBuffer && mapSize >= (uint32_t)frameSize)
        {
            memcpy(mapBuffer, inputBuffer.data(), frameSize);
            mAVbufferHelper->unmapHandle(audioBufferHandle);
        }
        else
        {
            UT_LOG_ERROR("[Audio DecoderID: %d] Failed to map buffer or insufficient size (mapBuffer:%p, mapSize:%u, required:%u)",
                         mDecoderId.value, mapBuffer, mapSize, frameSize);
            if (mapBuffer)
                mAVbufferHelper->unmapHandle(audioBufferHandle);
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, audioBufferHandle);
            return false;
        }
    }
    return true;
}

void AudioDecoderTest::handleControlPoints(int64_t framePTS)
{
    if (mTestConfiguration.stopPts == framePTS)
    {
        AUDIODECODER_STOP_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
        AUDIODECODER_START_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
    }

    if (mTestConfiguration.discontinuityPts == framePTS)
        AUDIODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, mAudioDecoderController, true);

    if (mTestConfiguration.flushPts == framePTS)
    {
        AUDIODECODER_FLUSH_SUCCESS(mDecoderHandle, mAudioDecoderController, false, true);
        AUDIODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
    }

    if (mTestConfiguration.flushRestPts == framePTS)
    {
        AUDIODECODER_FLUSH_SUCCESS(mDecoderHandle, mAudioDecoderController, true, true);
        AUDIODECODER_START_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
    }
}

void AudioDecoderTest::trackInputFrame(int64_t framePTS, bool errorFrame)
{
    InputAudioFrame_t inputFrame;
    inputFrame.sendTime    = chrono::steady_clock::now();
    inputFrame.framePTS    = framePTS;
    inputFrame.isErrorFrame = errorFrame;
    lock_guard<mutex> lock(mInputFrameMutex);
    mInputFrame.push_back(inputFrame);
}

audiodecoder::InputBufferMetadata AudioDecoderTest::buildMetadata(size_t frameIndex, int64_t framePTS,
                                                                   int64_t endPTS, bool isEosPtsFrame)
{
    audiodecoder::InputBufferMetadata metadata;
    metadata.nsPresentationTime = framePTS;
    metadata.trimStartNs   = (frameIndex == 0) ? mTestConfiguration.trimStartNs : 0;
    metadata.trimEndNs     = (frameIndex == mTestConfiguration.frameDetails.size() - 1) ? mTestConfiguration.trimEndNs : 0;
    metadata.discontinuity = false;
    metadata.endOfStream   = isEosPtsFrame || (framePTS == endPTS);

    if (metadata.endOfStream)
        UT_LOG_INFO("[Audio DecoderID: %d] Submitting buffer with endOfStream=true at PTS:%lld",
                    mDecoderId.value, framePTS);
    return metadata;
}

void AudioDecoderTest::submitFrame(int64_t audioBufferHandle, const audiodecoder::InputBufferMetadata& metadata)
{
    ::android::binder::Status status;
    bool decodeResult;

    mControlListener->resetDecodeBufferAvailable();

    UT_LOG_INFO("[Audio DecoderID: %d] Calling decodeBufferWithMetadata(IN:bufferHandle[0x%016" PRIx64 "], IN:PTS[%" PRId64 "], IN:endOfStream[%s], IN:trim[%d/%d])",
        mDecoderId.value, (uint64_t)audioBufferHandle, metadata.nsPresentationTime,
        UT_Control_GetMapString(bool_mapTable, metadata.endOfStream),
        metadata.trimStartNs, metadata.trimEndNs);

    status = mAudioDecoderController->decodeBufferWithMetadata(audioBufferHandle, metadata, &decodeResult);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_EQUAL(status.exceptionCode(), android::binder::Status::EX_NONE);

    if (!decodeResult)
    {
        UT_LOG_INFO("[Audio DecoderID: %d] Queue full at PTS %lld, waiting for onDecodeBufferAvailable",
                    mDecoderId.value, metadata.nsPresentationTime);
        UT_ASSERT_TRUE(mControlListener->waitForDecodeBufferAvailable(MAX_FRAME_WAIT_TIME));
        AUDIODECODER_DECODE_WITH_METADATA_SUCCESS(mDecoderId.value, mAudioDecoderController, audioBufferHandle, metadata);
    }
}

void AudioDecoderTest::handleMidStreamEOS(int64_t audioBufferHandle, int64_t framePTS)
{
    audiodecoder::InputBufferMetadata dummyMeta;
    dummyMeta.nsPresentationTime = framePTS;
    dummyMeta.endOfStream  = false;
    dummyMeta.discontinuity = false;
    dummyMeta.trimStartNs  = 0;
    dummyMeta.trimEndNs    = 0;
    AUDIODECODER_DECODE_WITH_METADATA_FAILURE(mAudioDecoderController, audioBufferHandle, dummyMeta, Status::EX_NONE);

    UT_LOG_INFO("[Audio DecoderID: %d] Restarting decoder after mid-stream EOS at PTS:%lld",
                mDecoderId.value, framePTS);
    AUDIODECODER_STOP_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
    AUDIODECODER_START_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
}

void AudioDecoderTest::startDecoding()
{
    const auto& frames = mTestConfiguration.frameDetails;
    int64_t endPTS = std::get<1>(frames.back());

    for (size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex)
    {
        const auto& [frameSize, framePTS, errorFrame] = frames[frameIndex];
        int64_t audioBufferHandle;

        if (!prepareInputBuffer(frameSize, audioBufferHandle))
            continue;

        handleControlPoints(framePTS);
        trackInputFrame(framePTS, errorFrame);

        bool isEosPtsFrame = (mTestConfiguration.eosPts != -1 && mTestConfiguration.eosPts == framePTS);
        auto metadata = buildMetadata(frameIndex, framePTS, endPTS, isEosPtsFrame);

        submitFrame(audioBufferHandle, metadata);

        if (mTestConfiguration.decoderDelay > 0)
            this_thread::sleep_for(chrono::milliseconds(mTestConfiguration.decoderDelay));

        if (isEosPtsFrame)
            handleMidStreamEOS(audioBufferHandle, framePTS);
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

bool AudioDecoderTest::checkPassThrough(const DecodedAudioFrame_t& frame, int32_t framesDecoded, bool& isPassThrough)
{
    /* Latency measurement on first frame — common to passthrough and non-passthrough */
    if (framesDecoded == 1)
    {
        lock_guard<mutex> lock(mInputFrameMutex);
        auto inputIt = find_if(mInputFrame.begin(), mInputFrame.end(),
            [&frame](const InputAudioFrame_t& f) {
                return f.framePTS == frame.framePTS;
            });
        if (inputIt == mInputFrame.end())
        {
            UT_LOG_ERROR("[Audio DecoderID: %d] Out Frame [PTS: %lld] does not match any submitted Input Frame PTS",
                         mDecoderId.value, frame.framePTS);
            UT_ASSERT_TRUE(false);
        }
        else
        {
            auto duration = chrono::duration_cast<chrono::microseconds>(
                frame.receiveTime - inputIt->sendTime);
            UT_LOG_INFO("[Audio DecoderID: %d] Decode Time in micro second [PTS: %lld]: %lld",
                        mDecoderId.value, frame.framePTS, duration.count());
            UT_ASSERT_TRUE(duration.count() <= mTestConfiguration.decodeTime);
        }
    }

    /* SOC_PROPRIETARY (pass-through): log once, skip decoded-frame validation */
    if (frame.isMetaAvailable &&
        frame.frameType == com::rdk::hal::audiodecoder::FrameType::SOC_PROPRIETARY)
    {
        if (!isPassThrough)
        {
            UT_LOG_WARNING("[Audio DecoderID: %d] SOC_PROPRIETARY(pass-through) output detected. "
                           "Skipping decoded-frame validation.", mDecoderId.value);
            isPassThrough = true;
        }
        return true;
    }
    return false;
}

void AudioDecoderTest::checkPTSSpacing(const DecodedAudioFrame_t& frame, int64_t frameBytes, FrameValidationState& state)
{
    int32_t frameSampleRate = (frame.isMetaAvailable && frame.metadata.pcm_metadata.sampleRate > 0)
                              ? frame.metadata.pcm_metadata.sampleRate : state.decodedSampleRate;
    int32_t frameChannels   = (frame.isMetaAvailable && frame.metadata.pcm_metadata.numChannels > 0)
                              ? frame.metadata.pcm_metadata.numChannels : state.decodedChannels;
    audiodecoder::PCMFormat frameFormat = frame.isMetaAvailable
                              ? frame.metadata.pcm_metadata.format : state.decodedFormat;
    int32_t bytesPerSample  = vts_bytesPerSample(frameFormat) * frameChannels;
    int64_t numSamples      = (bytesPerSample > 0) ? frameBytes / bytesPerSample : 0;
    int64_t frameDurationNs = (frameSampleRate > 0 && numSamples > 0)
                              ? (numSamples * 1000000000LL) / frameSampleRate : 0;

    if (state.prevPTS != -1)
    {
        int64_t delta = frame.framePTS - state.expectedNextPTS;
        UT_LOG_INFO("[Audio DecoderID: %d] PTS spacing [frame: %d, expectedPTS: %lld, actualPTS: %lld, delta: %lld]",
                    mDecoderId.value, state.framesDecoded, (long long)state.expectedNextPTS, (long long)frame.framePTS, (long long)delta);
        UT_ASSERT_TRUE(delta >= -PTS_DELTA_NS && delta <= PTS_DELTA_NS);
    }
    state.prevPTS         = frame.framePTS;
    state.expectedNextPTS = frame.framePTS + frameDurationNs;
}

void AudioDecoderTest::checkFrameMetadata(const DecodedAudioFrame_t& frame, FrameValidationState& state)
{
    UT_ASSERT_EQUAL(frame.metadata.sourceCodec, mTestConfiguration.codec);
    UT_ASSERT_EQUAL(int32_t(static_cast<int32_t>(frame.metadata.source)),
                    mAvSourcePropertyValue.value->get<PropertyValue::Value::Tag::intValue>());

    if (frame.metadata.discontinuity)
    {
        state.discontinuityFramesReceived++;
    }

    if (frame.metadata.endOfStream)
    {
        state.eosFramesReceived++;
    }

/* TODO: Trim metadata validation disabled — contract not fully settled.
 * trimStartNs/trimEndNs are per-frame durations (ns, invariant under resampling), but when one input frame
 * produces multiple output frames the HAL must map trim to the first/last output frame derived from that input.
 * An alternative of absolute PTS-based trim is under discussion, which would remove the remapping burden
 * from the HAL since it already tracks PTS per output frame. Re-enable once the approach is agreed. */
#if 0
    if (mTestConfiguration.trimStartNs > 0 || mTestConfiguration.trimEndNs > 0)
    {
        int64_t expectedTrimStart = (state.framesDecoded == 1) ? mTestConfiguration.trimStartNs : 0LL;
        int64_t expectedTrimEnd   = (state.framesDecoded == (int32_t)mTestConfiguration.frameDetails.size()) ? mTestConfiguration.trimEndNs : 0LL;
        UT_ASSERT_EQUAL(expectedTrimStart, (int64_t)frame.metadata.trimStartNs);
        UT_ASSERT_EQUAL(expectedTrimEnd,   (int64_t)frame.metadata.trimEndNs);
    }
#endif

    if (state.decodedSampleRate == 0 && frame.metadata.pcm_metadata.sampleRate > 0)
    {
        state.decodedSampleRate = frame.metadata.pcm_metadata.sampleRate;
        state.decodedChannels   = frame.metadata.pcm_metadata.numChannels;
        state.decodedFormat     = frame.metadata.pcm_metadata.format;
        UT_LOG_INFO("[Audio DecoderID: %d] PCM metadata: sampleRate=%d channels=%d format=%s",
                    mDecoderId.value, state.decodedSampleRate, state.decodedChannels,
                    ::com::rdk::hal::audiodecoder::toString(state.decodedFormat).c_str());
    }
}

void AudioDecoderTest::validatePostLoopMetrics(const FrameValidationState& state)
{
    UT_LOG_INFO("[Audio DecoderID: %d] duration=%lld decodedSampleRate=%d decodedChannels=%d",
                    mDecoderId.value, (long long)mTestConfiguration.duration, state.decodedSampleRate, state.decodedChannels);

    if (mTestConfiguration.duration > 0 && state.decodedSampleRate > 0 && state.decodedChannels > 0)
    {
        int64_t expectedSamples     = static_cast<int64_t>(std::ceil(static_cast<double>(mTestConfiguration.duration) * state.decodedSampleRate / 1e9));
        int64_t totalDecodedSamples = (int64_t)(state.accumulatedSize / ((int64_t)vts_bytesPerSample(state.decodedFormat) * state.decodedChannels));
        UT_LOG_INFO("[Audio DecoderID: %d] Resampling validation: totalDecodedSamples=%lld expectedSamples=%lld decodeErrors=%d",
                    mDecoderId.value, (long long)totalDecodedSamples, (long long)expectedSamples, state.decodeErrors);
        if (state.decodeErrors > 0)
        {
            UT_ASSERT_TRUE(totalDecodedSamples <= expectedSamples);
        }
        else
        {
            UT_ASSERT_EQUAL(totalDecodedSamples, expectedSamples);
        }
    }

    int32_t totalErrors = mEventListener->getErrorCount();

    /* expectedFrames: passthrough counts all frames; non-passthrough subtracts error frames */
    int32_t expectedFrames = state.isPassThrough ? mTestConfiguration.numOfFrames
                                                 : mTestConfiguration.numOfFrames - state.decodeErrors;

    /* Frame count check — common to both passthrough and non-passthrough.
     * Flush discards queued frames mid-stream so use relaxed (<=) when flush is configured. */
    if ((mTestConfiguration.flushPts == -1) && (mTestConfiguration.flushRestPts == -1))
    {
        UT_ASSERT_EQUAL(state.framesDecoded, expectedFrames);
    }
    else
    {
        UT_ASSERT_TRUE(state.framesDecoded <= expectedFrames);
    }

    /* SOC_PROPRIETARY pass-through: frame count already validated above, no further checks needed */
    if (state.isPassThrough)
    {
        return;
    }

    /* Discontinuity/flush validation: at least one but not all frames should carry
     * the discontinuity flag when a discontinuity or flush point is configured */
    if ((mTestConfiguration.discontinuityPts != -1) || (mTestConfiguration.flushPts != -1))
    {
        UT_ASSERT_TRUE((state.discontinuityFramesReceived > 0 && state.discontinuityFramesReceived < expectedFrames));
    }

    /* EOS validation: one EOS always expected at stream end; a second when eosPts is configured */
    int32_t expectedEOSFrames = 1;

    if (mTestConfiguration.eosPts != -1)
    {
        expectedEOSFrames++;
    }

    UT_ASSERT_EQUAL(state.eosFramesReceived, expectedEOSFrames);

    /* Error count validation */
    UT_ASSERT_EQUAL(totalErrors, state.decodeErrors);

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
            UT_ASSERT_EQUAL(metricsDecodeErrors, state.decodeErrors);
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
            UT_ASSERT_TRUE(metricsDecodeErrors <= state.decodeErrors);
        }
        if (metricsFramesDropped != -1)
        {
            UT_ASSERT_TRUE(metricsFramesDropped >= 0);
        }
    }
}

void AudioDecoderTest::validateDecodedFrame()
{
    DecodedAudioFrame_t decodedFrame;

    FrameValidationState state;
    state.decodeErrors = static_cast<int32_t>(count_if(
        mTestConfiguration.frameDetails.begin(),
        mTestConfiguration.frameDetails.end(),
        [](const auto& f) { return get<2>(f); }));

    int32_t samplesPerFrame = vts_audioDecoder_helper_getSamplesPerFrame(mTestConfiguration.codec);
    state.validatePTS = (samplesPerFrame > 0 && state.decodeErrors == 0 &&
                         mTestConfiguration.flushPts == -1 &&
                         mTestConfiguration.flushRestPts == -1);

    while (mControlListener->getNextFrame(decodedFrame, MAX_FRAME_WAIT_TIME, true))
    {
        /* Block 1: frame data — only when buffer is valid */
        if (decodedFrame.frameBufferHandle != -1)
        {
            state.framesDecoded++;

            int64_t frameBytes = mAVbufferHelper->getAllocationSize(decodedFrame.frameBufferHandle);

            if (mTestConfiguration.duration > 0)
            {
                state.accumulatedSize += frameBytes;
            }

            bool isPassThroughFrame = checkPassThrough(decodedFrame, state.framesDecoded, state.isPassThrough);

            if (!isPassThroughFrame && state.validatePTS)
            {
                checkPTSSpacing(decodedFrame, frameBytes, state);
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
            checkFrameMetadata(decodedFrame, state);
        }
    }

    validatePostLoopMetrics(state);
}

/** @} */ // End of AUDIO_DECODER_HALTEST_HEADER
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK

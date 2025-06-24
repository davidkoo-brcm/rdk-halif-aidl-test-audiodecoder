/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
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
 *
 */

/**
 * @addtogroup AUDIO_DECODER Audio Decoder Module
 * @{
 *
 */

/**
 * @defgroup AUDIO_DECODER_HALTEST Audio Decoder HAL Tests
 * @{
 *
 */

/**
 * @defgroup AUDIO_DECODER_HALTEST_L3 Audio Decoder HAL Tests L3 File
 * @{
 * @parblock
 *
 * ### L3 Test Cases for Audio Decoder HAL :
 *
 * This module includes Level 3 functional tests (success and failure scenarios).
 * This is to ensure that the API meets the operational requirements of the module across all vendors.
 *
 * **Pre-Conditions:** @n
 * **Dependencies:** None@n
 *
 * @endparblock
 *
 */

/**
 * @file vts_audiodecoder_l3_tests.c
 *
 */

#include <fstream>
#include <ut.h>
#include <ut_log.h>
#include <ut_kvp_profile.h>
#include <ut_control_plane.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <binder/Status.h>
#include <sys/syscall.h>
#include <utils/String16.h>
#include <stdio.h>
#include <vector>
#include <optional>
#include <algorithm>
#include <unordered_set>
#include <tuple>
#include <map>
#include <chrono>


#include "com/rdk/hal/avbuffer/IAVBuffer.h"
#include "com/rdk/hal/avbuffer/BnAVBufferSpaceListener.h"

#include "vts_audiodecoder_helper_functions.h"
#include "vts_audiodecoder_parse_hfp.h"
#include "vts_audiodecoder_parse_testconfig.h"
#include "vts_audiodecoder.h"

#include "IAVBufferHelper.h"

using namespace std::chrono;
using namespace std;

/* Test class registration with UT_Core */
UT_ADD_TEST_TO_GROUP(VTS_L3_AudioDecoder, UT_TESTS_L3)

static const char *gModuleName = "IAudioDecoderManager";
static const char *gTestGroup = "L3";

sp<IAVBuffer> vts_avbuffer_getService()
{
    return
        android::interface_cast<IAVBuffer>(
            android::defaultServiceManager()->getService(
                android::String16(IAVBuffer::serviceName().c_str())
            )
        );
}

#define AVBUFFER_CREATE_AUDIOPOOL_SUCCESS(avBufferManager, secureHeap, audioDecoderId, listener, audioPool) \
    do { \
        UT_LOG_INFO("Calling createAudioPool(IN:secureHeap:[%d], IN:audioDecoderId:[%d], IN:listener:[%p], OUT:audioPool)", \
                    secureHeap, audioDecoderId.value, listener.get()); \
        Status status = avBufferManager->createAudioPool(secureHeap, audioDecoderId, listener, &audioPool); \
        UT_LOG_INFO("Result createAudioPool(IN:secureHeap:[%d], IN:audioDecoderId:[%d], IN:listener:[%p], OUT:audioPool.handle:[%d]), status:[isOK:%s, exception:%s]", \
                    secureHeap, audioDecoderId.value, listener.get(), audioPool.handle, \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_NOT_EQUAL(audioPool.handle, Pool::INVALID); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AVBUFFER_DESTROY_POOL_SUCCESS(avBufferManager, pool) \
    do { \
        bool _aidl_return = false; \
        Status status; \
        UT_LOG_INFO("Calling destroyPool(IN:poolHandle:[%d], OUT:_aidl_return:[])", pool); \
        status = avBufferManager->destroyPool(pool, &_aidl_return); \
        UT_LOG_INFO("Result destroyPool with (IN:poolHandle:[%d], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    pool.handle, UT_Control_GetMapString(bool_mapTable, _aidl_return), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE(_aidl_return); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AVBUFFER_ALLOC_SUCCESS(avBufferManager, pool, size, bufferHandle) \
    do { \
        UT_LOG_INFO("Calling alloc(IN:poolHandle:[%d], IN:size:[%d], OUT:bufferHandle)", pool.handle, size); \
        Status status = avBufferManager->alloc(pool, size, &bufferHandle); \
        UT_LOG_INFO("Result alloc(IN:poolHandle:[%d], IN:size:[%d], OUT:bufferHandle:[0x%X]), status:[isOK:%s, exception:%s]", \
                    pool.handle, size, bufferHandle, UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_NOT_EQUAL(bufferHandle, IAVBuffer::INVALID_HANDLE); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AVBUFFER_FREE_SUCCESS(avBufferManager, bufferHandle) \
    do { \
        bool _aidl_return = false; \
        UT_LOG_INFO("Calling free(IN:bufferHandle:[0x%X], OUT:_aidl_return)", bufferHandle); \
        Status status = avBufferManager->free(bufferHandle, &_aidl_return); \
        UT_LOG_INFO("Result free(IN:bufferHandle:[0x%X], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    bufferHandle, UT_Control_GetMapString(bool_mapTable, _aidl_return), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        UT_ASSERT_TRUE(_aidl_return); \
    } while (0)

#define AVBUFFER_GETMETRICS(avBufferManager, secureHeap, heapMetrics) \
    do { \
        UT_LOG_INFO("Calling getHeapMetrics(IN:secureHeap:[%s], OUT:heapMetrics[])", \
                    UT_Control_GetMapString(bool_mapTable, secureHeap)); \
        Status status = avBufferManager->getHeapMetrics(secureHeap, &heapMetrics); \
        UT_LOG_INFO("Result getHeapMetrics(IN:secureHeap:[%s], OUT:heapMetrics[secure:%s, bytesUsed:%d, bytesTotal:%d]), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, secureHeap), \
                    UT_Control_GetMapString(bool_mapTable, heapMetrics.secure), \
                    heapMetrics.bytesUsed, \
                    heapMetrics.bytesTotal, \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

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

class DecoderTest
{
public:
    DecoderTest(IAudioDecoder::Id decoderId, sp<IAudioDecoder> decoderHandle,
                sp<AudioDecoderControllerListener> controlListener,
                sp<AudioDecoderEventListener> eventListener,
                sp<IAVBuffer> AVBufferManager,
                bool secureDecode, testConfiguration_t& config)
    {
        mDecoderId = decoderId;
        mDecoderHandle = decoderHandle;
        mControlListener = controlListener;
        mSecureDecode = secureDecode;
        mEventListener = eventListener;
        mTestConfiguration = config;
        mAVBufferManager = AVBufferManager;
        mDecodingStream = false;
        mAVbufferHelper = getAVBufferHelperInstance();
    }

    void run(bool concurrentDecodeConsume)
    {
        mConcurrentDecodeConsume = concurrentDecodeConsume;
        mDecodingStream = true;
        if (concurrentDecodeConsume)
        {
            mDecoderThread = thread(&DecoderTest::decoderThreadFunc, this);
            this_thread::sleep_for(chrono::milliseconds(100));
            mConsumerThread = thread(&DecoderTest::consumerThreadFunc, this);

            mDecoderThread.join();
            mConsumerThread.join();
        }
        else
        {
            decodeAndConsumeAudioStreams();
        }
    }

    bool getDecoderMetrics(int32_t &framesDecoded, int32_t &decodeErrors, int32_t framesDropped)
    {
        optional<PropertyValue> propertyValue;

        framesDecoded = decodeErrors = framesDropped = -1;

        AUDIODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, Property::METRIC_FRAMES_DECODED, propertyValue);
        if (!propertyValue.has_value() || !propertyValue->value.has_value())
        {
            return false;
        }
        framesDecoded = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

        AUDIODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, Property::METRIC_DECODE_ERRORS, propertyValue);
        if (!propertyValue.has_value() || !propertyValue->value.has_value())
        {
            return false;
        }
        decodeErrors = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

        AUDIODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, Property::METRIC_FRAMES_DROPPED, propertyValue);
        if (!propertyValue.has_value() || !propertyValue->value.has_value())
        {
            return false;
        }
        framesDropped = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

        return true;
    }

private:
    thread mDecoderThread, mConsumerThread;
    IAudioDecoder::Id mDecoderId;
    sp<IAudioDecoder> mDecoderHandle;
    sp<AudioDecoderControllerListener> mControlListener;
    sp<AudioDecoderEventListener> mEventListener;
    sp<IAVBuffer> mAVBufferManager;
    bool mSecureDecode;
    testConfiguration_t mTestConfiguration;
    deque<InputAudioFrame_t> mInputFrame;
    mutex mDecodeMutex;
    atomic<bool> mDecodingStream;
    bool mConcurrentDecodeConsume;
    IAVBufferHelper* mAVbufferHelper;

    void decoderThreadFunc()
    {
        sp<IAudioDecoderController> audioDecoderController;
        int64_t audioBufferHandle;
        Pool audioPool;
        sp<IAVBufferSpaceListener> bufferListener = new AVBufferSpaceListener();
        mDecodingStream = true;

        UT_LOG_INFO("[PID: %d, TID: %d] decoderThreadFunc Start", getpid(), syscall(SYS_gettid));

        /* Create Audio Pool */
        AVBUFFER_CREATE_AUDIOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, bufferListener, audioPool);

        ifstream inputFile(mTestConfiguration.input, ios::binary);
        if (!inputFile)
        {
            UT_LOG_ERROR("Failed to open file: %s", mTestConfiguration.input.c_str());
            return;
        }

        /* Open the Decoder */
        AUDIODECODER_OPEN_SUCCESS(mDecoderHandle, mTestConfiguration.codec, mSecureDecode, mControlListener, audioDecoderController, true);

        /* Start the decoder */
        AUDIODECODER_START_SUCCESS(mDecoderHandle, audioDecoderController, true);

        for(const auto& [frameSize, framePTS, errorFrame]: mTestConfiguration.frameDetails)
        {
            vector<uint8_t> inputBuffer;
            int32_t trimEndNs = mTestConfiguration.trimEndNs;
            int32_t trimStartNs = mTestConfiguration.trimStartNs;
            InputAudioFrame_t inputFrame;
            void *mapBuffer = nullptr;
            uint32_t mapSize = 0;

            /* Allocate the audio buffer */
            AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, audioPool, frameSize, audioBufferHandle);

            inputBuffer.resize(frameSize);
            inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);

            if(mSecureDecode)
            {
                bool status;
                if(!mAVbufferHelper->writeSecureHandle(audioBufferHandle, inputBuffer.data(), frameSize))
                {
                    UT_LOG_ERROR("Failed to copy the data to secure buffer");
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

            if(mTestConfiguration.eosPts == framePTS)
            {
                /* After EOS signal decode should fail */
                AUDIODECODER_DECODEBUFFER_FAILURE(audioDecoderController, framePTS, audioBufferHandle, trimStartNs, trimEndNs, Status::EX_NONE);

                /* decoder should be restarted after eos */
                AUDIODECODER_STOP_SUCCESS(mDecoderHandle, audioDecoderController, true);
                AUDIODECODER_START_SUCCESS(mDecoderHandle, audioDecoderController, true);
            }

            inputFrame.sendTime = chrono::steady_clock::now();
            inputFrame.framePTS = framePTS;
            inputFrame.reference = mTestConfiguration.reference;
            inputFrame.rms_threshold = mTestConfiguration.rms_diff_threashold;
            inputFrame.isErrorFrame = errorFrame;
            {
                lock_guard<mutex> lock(mDecodeMutex);
                mInputFrame.push_back(inputFrame);
            }

            AUDIODECODER_DECODEBUFFER_SUCCESS(audioDecoderController, framePTS, audioBufferHandle, trimStartNs, trimEndNs);

            this_thread::sleep_for(chrono::milliseconds(mTestConfiguration.decoderDelay));

            /* Free the audio buffer */
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, audioBufferHandle);
        }

        /* Stop the decoder */
        AUDIODECODER_STOP_SUCCESS(mDecoderHandle, audioDecoderController, true);

        /* Close the decoder */
        AUDIODECODER_CLOSE_SUCCESS(mDecoderHandle, audioDecoderController, true);

        /* Destroy audio pool */
        AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, audioPool);

        UT_LOG_INFO("[PID: %d, TID: %d] decoderThreadFunc End", getpid(), syscall(SYS_gettid));
        mDecodingStream = false;
    }
    void consumerThreadFunc()
    {
        ifstream reference_file;
        int32_t errorCount = 0;

        UT_LOG_INFO("[PID: %d, TID: %d] consumerThreadFunc Start", getpid(), syscall(SYS_gettid));
        while (mDecodingStream || !mInputFrame.empty())
        {
            DecodedAudioFrame_t decodedFrame;
            InputAudioFrame_t inputFrame;
            chrono::steady_clock::time_point receiveTime;
            int64_t PTS;

            if(mInputFrame.empty())
            {
                this_thread::sleep_for(chrono::milliseconds(10));
                continue;
            }

            inputFrame = move(mInputFrame.front());
            mInputFrame.pop_front();

            receiveTime = inputFrame.sendTime;
            PTS = inputFrame.framePTS;

            if(inputFrame.isErrorFrame)
            {
                ErrorDetails_t errorDetails;

                if(!mEventListener->getNextError(errorDetails))
                {
                    UT_ASSERT_TRUE(false);
                }
                else
                {
                    receiveTime = errorDetails.receiveTime;
                }
            }
            else
            {
                mControlListener->getNextFrame(decodedFrame);

                if(decodedFrame.framePTS != inputFrame.framePTS)
                {
                    UT_LOG_ERROR("Input frame and decoded frame PTS is not matching..");
                    UT_ASSERT_TRUE(false);
                    continue;
                }

                /* Verify the decoder buffer in non-tunnel mode */
                /* TODO: Check for trim start and end */
                if(!decodedFrame.data.empty())
                {
                    vector<uint8_t> referenceBuffer;
                    double rms_diff;
                    int32_t output_buffer_size = decodedFrame.data.size();

                    if(!reference_file.is_open())
                    {
                        reference_file.open(inputFrame.reference);
                    }
                    referenceBuffer.resize(output_buffer_size);
                    reference_file.read(reinterpret_cast<char*>(referenceBuffer.data()), output_buffer_size);

                    rms_diff = vts_computeRMSDiff(referenceBuffer.data(), decodedFrame.data.data(), decodedFrame.data.size(), decodedFrame.metadata.format);
                    UT_ASSERT_TRUE(rms_diff <= inputFrame.rms_threshold);

                    receiveTime = decodedFrame.receiveTime;
                    PTS = decodedFrame.framePTS;
                }
            }

            auto duration = chrono::duration_cast<chrono::microseconds>(receiveTime - inputFrame.sendTime);
            UT_LOG_INFO("Decode Time in micro second [PTS: %lld]: %lld", PTS, duration.count());
        }
        //TODO: check for error frames
        /*if (errorCount > 0)
        {
            //TODO: check for correct validation
            UT_LOG_INFO("No. of Error frames pushed: %d", errorCount);
            UT_LOG_INFO("No. of Error frames Callbacks: %d", mEventListener->getErrors().size());
            UT_ASSERT_EQUAL(mEventListener->getErrors().size(), errorCount);
        }*/
        UT_LOG_INFO("[PID: %d, TID: %d] consumerThreadFunc end", getpid(), syscall(SYS_gettid));
    }
    void decodeAndConsumeAudioStreams()
    {
        sp<IAudioDecoderController> audioDecoderController;
        int64_t audioBufferHandle;
        Pool audioPool;
        sp<IAVBufferSpaceListener> bufferListener = new AVBufferSpaceListener();

        /* Create Audio Pool */
        AVBUFFER_CREATE_AUDIOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, bufferListener, audioPool);

        ifstream inputFile(mTestConfiguration.input, ios::binary);
        if (!inputFile)
        {
            UT_LOG_ERROR("Failed to open file: %s", mTestConfiguration.input.c_str());
            return;
        }

        ifstream referenceFile(mTestConfiguration.reference, ios::binary);
        if (!referenceFile)
        {
            UT_LOG_ERROR("Failed to open reference file: %s", mTestConfiguration.reference.c_str());
            return;
        }

        /* Open the Decoder */
        AUDIODECODER_OPEN_SUCCESS(mDecoderHandle, mTestConfiguration.codec, mSecureDecode, mControlListener, audioDecoderController, true);

        /* Start the decoder */
        AUDIODECODER_START_SUCCESS(mDecoderHandle, audioDecoderController, true);

        for(const auto& [frameSize, framePTS, errorFrame]: mTestConfiguration.frameDetails)
        {
            vector<uint8_t> inputBuffer;
            vector<uint8_t> referenceBuffer;
            size_t output_buffer_size;
            double rms_diff = 0.0;
            int32_t trimEndNs = mTestConfiguration.trimEndNs;
            int32_t trimStartNs = mTestConfiguration.trimStartNs;
            DecodedAudioFrame_t decodedFrame;
            void *mapBuffer = nullptr;
            uint32_t mapSize = 0;

            chrono::steady_clock::time_point startTime, receiveTime;
            int64_t PTS = framePTS;

            /* Allocate the audio buffer */
            AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, audioPool, frameSize, audioBufferHandle);

            inputBuffer.resize(frameSize);
            inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);

            if(mSecureDecode)
            {
                bool status;
                if(!mAVbufferHelper->writeSecureHandle(audioBufferHandle, inputBuffer.data(), frameSize))
                {
                    UT_LOG_ERROR("Failed to copy the data to secure buffer");
                    AVBUFFER_FREE_SUCCESS(mAVBufferManager, audioBufferHandle);
                    continue;
                }
            }
            else
            {
                mapBuffer = mAVbufferHelper->mapHandle(audioBufferHandle, &mapSize);
                memcpy(mapBuffer, inputBuffer.data(), mapSize);
            }

            startTime = receiveTime = chrono::steady_clock::now();
            AUDIODECODER_DECODEBUFFER_SUCCESS(audioDecoderController, framePTS, audioBufferHandle, trimStartNs, trimEndNs);

            if(errorFrame)
            {
                ErrorDetails_t errorDetails;

                if(!mEventListener->getNextError(errorDetails))
                {
                    UT_ASSERT_TRUE(false);
                }
                else
                {
                    receiveTime = errorDetails.receiveTime;
                }
            }
            else
            {
                if(!mControlListener->getNextFrame(decodedFrame))
                {
                    UT_ASSERT_TRUE(false);
                }

                /* Verify the decodec buffer in non-tunnel mode */
                /* TODO: Check for trim start and end */
                if(!decodedFrame.data.empty())
                {
                    output_buffer_size = decodedFrame.data.size();
                    referenceBuffer.resize(output_buffer_size);
                    referenceFile.read(reinterpret_cast<char*>(referenceBuffer.data()), output_buffer_size);

                    //TODO: validate the decoded output
                    rms_diff = vts_computeRMSDiff(referenceBuffer.data(), decodedFrame.data.data(), output_buffer_size, decodedFrame.metadata.format);
                    UT_ASSERT_TRUE(rms_diff <= mTestConfiguration.rms_diff_threashold);
                    receiveTime = decodedFrame.receiveTime;
                    PTS = decodedFrame.framePTS;
                }
            }

            auto duration = chrono::duration_cast<chrono::microseconds>(receiveTime - startTime);
            UT_LOG_INFO("Decode Time in micro second [PTS: %lld]: %lld", PTS, duration.count());

            /* Free the audio buffer */
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, audioBufferHandle);
        }

        /* Stop the decoder */
        AUDIODECODER_STOP_SUCCESS(mDecoderHandle, audioDecoderController, true);

        /* Close the decoder */
        AUDIODECODER_CLOSE_SUCCESS(mDecoderHandle, audioDecoderController, true);

        /* Destroy audio pool */
        AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, audioPool);
    }
};

class VTS_L3_AudioDecoder : public UTCore
{
public:
    VTS_L3_AudioDecoder() : UTCore() {}

    ~VTS_L3_AudioDecoder() override = default;

    vector<tuple<IAudioDecoder::Id, sp<IAudioDecoder>, sp<AudioDecoderControllerListener>, sp<AudioDecoderEventListener>, audiodecoder::Capabilities *>> mAudioDecoderList;
    sp<IAVBuffer> mAVBufferManager;

    void SetUp() override
    {
        vector<IAudioDecoder::Id> decoderIds;

        mAVBufferManager = vts_avbuffer_getService();
        gTestConfiguration = vts_kvp_create_instance(VTS_TEST_CONFIG_PATH);

        /* Get the Audio decoder IDs */
        AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

        android::ProcessState::self()->startThreadPool();

        for(const auto& _decoderId: decoderIds)
        {
            sp<IAudioDecoder> audioDecoder;

            audiodecoder::Capabilities *capabilities = new audiodecoder::Capabilities();

            sp<AudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            sp<AudioDecoderControllerListener> controlListener = new AudioDecoderControllerListener();

            /* Get Audio decoder handle */
            AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

            /* Register events */
            AUDIODECODER_REGISTER_EVENT_SUCCESS(audioDecoder, eventListener);

            /* Get Capabilities of Audio Decoder */
            AUDIODECODER_GETCAPABILITIES(audioDecoder, capabilities);

            mAudioDecoderList.push_back({_decoderId, audioDecoder, controlListener, eventListener, capabilities});

        }
    }

    void runSequentialDecode(testConfiguration_t& testConfig, bool secureDecode, bool concurrentDecodeConsume)
    {
        for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mAudioDecoderList)
        {
            HeapMetrics beforeHeapMetrics, afterHeapMetrics;
            int32_t startFramesDecoded, startFrameErrors, startFramesDropped;
            int32_t endFramesDecoded, endFrameErrors, endFramesDropped;

            //Skip if the plafrom doesn't support secure decode
            if (secureDecode && !capabilities->supportsSecure)
            {
                UT_LOG_INFO("Decoder [%d] doesn't support secure decoding", _decoderId);
                continue;
            }
            auto result = find(capabilities->supportedCodecs.begin(),capabilities->supportedCodecs.end(),testConfig.codec);
            if(result == capabilities->supportedCodecs.end())
            {
                UT_LOG_INFO("Decoder [%d] doesn't support Codec [%s]", _decoderId, audiodecoder::toString(testConfig.codec).c_str());
                continue;
            }
            /* Get heap metrics */
            AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, beforeHeapMetrics);

            DecoderTest decodeTest(_decoderId, decoderHandle, controlListener,
                                    eventListener, mAVBufferManager, secureDecode, testConfig);

            /* Get decoder metrics before decoding */
            UT_ASSERT_TRUE(decodeTest.getDecoderMetrics(startFramesDecoded, startFrameErrors, startFramesDropped));

            decodeTest.run(concurrentDecodeConsume);

            /* Get decoder metrics after decoding */
            UT_ASSERT_TRUE(decodeTest.getDecoderMetrics(endFramesDecoded, endFrameErrors, endFramesDropped));

            /* Get heap metrics */
            AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, afterHeapMetrics);

            /* check if the decoder freed all the buffer pools */
            UT_ASSERT_TRUE(beforeHeapMetrics.secure == afterHeapMetrics.secure);
            UT_ASSERT_TRUE(beforeHeapMetrics.bytesUsed == afterHeapMetrics.bytesUsed);
            UT_ASSERT_TRUE(beforeHeapMetrics.bytesTotal == afterHeapMetrics.bytesTotal);

            /* TODO: check if this required or not */
            //controlListener->shutdown();
        }
    }

    void runConcurrentDecode(testConfiguration_t& testConfig, bool secureDecode, bool concurrentDecodeConsume)
    {
        vector<thread> threads;
        vector<DecoderTest> decoderTestList;
        HeapMetrics beforeHeapMetrics, afterHeapMetrics;

        //TODO: add check of decoder metrics
        /* Get heap metrics */
        AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, beforeHeapMetrics);

        for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mAudioDecoderList)
        {
            //Skip if the plafrom doesn't support secure decode
            if (secureDecode && !capabilities->supportsSecure)
            {
                UT_LOG_INFO("Decoder [%d] doesn't support secure decoding", _decoderId);
                continue;
            }
            auto result = find(capabilities->supportedCodecs.begin(),capabilities->supportedCodecs.end(),testConfig.codec);
            if(result == capabilities->supportedCodecs.end())
            {
                UT_LOG_INFO("Decoder [%d] doesn't support Codec [%s]", _decoderId, audiodecoder::toString(testConfig.codec).c_str());
                continue;
            }

            DecoderTest decoderTest(_decoderId, decoderHandle, controlListener,
                                    eventListener, mAVBufferManager, secureDecode, testConfig);
            threads.emplace_back(&DecoderTest::run, &decoderTest, concurrentDecodeConsume);
        }
        for (auto& t : threads)
        {
            t.join();
        }
        /* Get heap metrics */
        AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, afterHeapMetrics);

        /* check if the decoder freed all the buffer pools */
        UT_ASSERT_TRUE(beforeHeapMetrics.secure == afterHeapMetrics.secure);
        UT_ASSERT_TRUE(beforeHeapMetrics.bytesUsed == afterHeapMetrics.bytesUsed);
        UT_ASSERT_TRUE(beforeHeapMetrics.bytesTotal == afterHeapMetrics.bytesTotal);
    }

    void runMultiProcessTest(char *testName, bool secureMode)
    {
        //TODO: multi process decoding
#if 0
        vector<testConfiguration_t> testConfiguration;
        vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, testConfiguration);

        for(auto& test: testConfiguration)
        {
            for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mAudioDecoderList)
            {
                pid_t pid = fork();

                if(pid == 0)
                {
                    UT_LOG_INFO("[PID: %d, TID: %d] created process", getpid(), syscall(SYS_gettid));

                    DecoderTest decoderTest(_decoderId, decoderHandle, controlListener,
                                            eventListener, mAVBufferManager, secureMode, test);

                    decoderTest.run(true);

                    _exit(0);
                }
                else if (pid < 0)
                {
                    UT_LOG_ERROR("Failed to create decoder process");
                }
            }
            int status;
            while (wait(&status) > 0);
        }
#endif
    }

    void runSingleProcessTest(const char *testName, bool secureMode, bool concurrentMode)
    {
        vector<testConfiguration_t> testConfiguration;

        vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, testConfiguration);

        if(concurrentMode)
        {
            for(auto& test: testConfiguration)
            {
                runConcurrentDecode(test, secureMode, true);
            }
        }
        else
        {
            for(auto& test: testConfiguration)
            {
                runSequentialDecode(test, secureMode, true);
            }
        }
    }


    void TearDown() override
    {
        for (const auto& [_decoderId, decoder, controlListener, eventListener, capabilities] : mAudioDecoderList)
        {
            /* Un-register the events */
            AUDIODECODER_UNREGISTER_EVENT_SUCCESS(decoder, eventListener);
        }
        mAudioDecoderList.clear();
        //TODO: check right place to call joinThreadPool
        //android::IPCThreadState::self()->joinThreadPool();
    }
};

/**
 * @brief Test the ability to decode audio frames
 *        Decoding type: Sequential
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 001@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_NonTunnel_NonSecure)
{
    const char *testName = "Decode";
    bool secureMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder behavior for error conditions in sequential decode
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 002@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_NonTunnel_NonSecure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test trimming functionality during sequential decoding
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 003@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_NonTunnel_NonSecure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in sequential decode
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 004@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_NonTunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of End-of-Stream (EOS) condition in sequential decode
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 005@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_NonTunnel_NonSecure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder flushing capability mid-stream
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 006@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_NonTunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder flush followed by reset operation
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 007@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_NonTunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test the ability to decode audio frames
 *        Decoding type: Sequential
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 008@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_NonTunnel_Secure)
{
    const char *testName = "Decode";
    bool secureMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder behavior for error conditions in sequential decode
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 009@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_NonTunnel_Secure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test trimming functionality during sequential decoding
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 010@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_NonTunnel_Secure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in sequential decode
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 011@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_NonTunnel_Secure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of End-of-Stream (EOS) condition in sequential decode
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 012@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_NonTunnel_Secure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder flushing capability mid-stream
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 013@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_NonTunnel_Secure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder flush followed by reset operation
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 014@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_NonTunnel_Secure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test sequential decode in tunnel mode (Non-Secure)
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 015@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_Tunnel_NonSecure)
{
    const char *testName = "Decode";
    bool secureMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder behavior for error conditions in tunnel mode (Non-Secure)
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 016@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_Tunnel_NonSecure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test trimming functionality in tunnel mode (Non-Secure)
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 017@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_Tunnel_NonSecure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity in tunnel mode (Non-Secure)
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 018@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_Tunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of End-of-Stream (EOS) in tunnel mode (Non-Secure)
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 019@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_Tunnel_NonSecure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder flushing in tunnel mode (Non-Secure)
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 020@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_Tunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test flush followed by reset in tunnel mode (Non-Secure)
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 021@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_Tunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test sequential decode in secure mode (Tunnel)
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 022@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_Tunnel_Secure)
{
    const char *testName = "Decode";
    bool secureMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder error handling in sequential decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 023@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_Tunnel_Secure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test trimming functionality in sequential decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 024@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_Tunnel_Secure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity in sequential decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 025@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_Tunnel_Secure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of EOS in sequential decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 026@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_Tunnel_Secure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder flush in sequential decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 027@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_Tunnel_Secure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test flush and reset in sequential decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 028@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Sequential_Tunnel_Secure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test concurrent decode in non-tunnel mode (Non-Secure)
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 029@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_NonTunnel_NonSecure)
{
    const char *testName = "Decode";
    bool secureMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder error handling in concurrent decode (Non-Tunnel, Non-Secure)
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 030@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_NonTunnel_NonSecure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test trimming functionality in concurrent decode (Non-Tunnel, Non-Secure)
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 031@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_NonTunnel_NonSecure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity in concurrent decode (Non-Tunnel, Non-Secure)
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 032@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_NonTunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of EOS in concurrent decode (Non-Tunnel, Non-Secure)
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 033@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_NonTunnel_NonSecure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder flush in concurrent decode (Non-Tunnel, Non-Secure)
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 034@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_NonTunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test flush and reset in concurrent decode (Non-Tunnel, Non-Secure)
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 035@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_NonTunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test concurrent decode in secure mode (Non-Tunnel)
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 036@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_NonTunnel_Secure)
{
    const char *testName = "Decode_Secure";
    bool secureMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder error handling in concurrent decode (Non-Tunnel, Secure)
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 037@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_NonTunnel_Secure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test trimming functionality in concurrent decode (Non-Tunnel, Secure)
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 038@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_NonTunnel_Secure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity in concurrent decode (Non-Tunnel, Secure)
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 039@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_NonTunnel_Secure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of EOS in concurrent decode (Non-Tunnel, Secure)
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 040@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_NonTunnel_Secure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder flush in concurrent decode (Non-Tunnel, Secure)
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 041@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_NonTunnel_Secure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test flush and reset in concurrent decode (Non-Tunnel, Secure)
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 042@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_NonTunnel_Secure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test concurrent decode in secure mode (Tunnel)
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 043@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_Tunnel_NonSecure)
{
    const char *testName = "Decode";
    bool secureMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder error handling in concurrent decode (Tunnel, Non-Secure)
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 044@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_Tunnel_NonSecure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test trimming functionality in concurrent decode (Tunnel, Non-Secure)
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 045@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_Tunnel_NonSecure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity in concurrent decode (Tunnel, Non-Secure)
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 046@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_Tunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of EOS in concurrent decode (Tunnel, Non-Secure)
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 047@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_Tunnel_NonSecure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder flush in concurrent decode (Tunnel, Non-Secure)
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 048@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_Tunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test flush and reset in concurrent decode (Tunnel, Non-Secure)
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 049@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_Tunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test concurrent decode in secure mode (Tunnel)
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 050@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_Tunnel_Secure)
{
    const char *testName = "Decode";
    bool secureMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder error handling in concurrent decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 051@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_Tunnel_Secure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test trimming functionality in concurrent decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 052@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_Tunnel_Secure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity in concurrent decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 053@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_Tunnel_Secure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test handling of EOS in concurrent decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 054@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_Tunnel_Secure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test decoder flush in concurrent decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 055@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_Tunnel_Secure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/**
 * @brief Test flush and reset in concurrent decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 056@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Concurrent_Tunnel_Secure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, concurrentMode);
}

/** @} */ // End of AUDIO_DECODER_HALTEST L3 File
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER Module
/** @} */ // End of HPK
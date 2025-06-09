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

using namespace std::chrono;
using namespace std;

/* Test class registration with UT_Core */
UT_ADD_TEST_TO_GROUP(VTS_L3_AudioDecoder, UT_TESTS_L3)

static char *gModuleName = "IAudioDecoderManager";
static char *gTestGroup = "L3";

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

    void decoderThreadFunc()
    {
        sp<IAudioDecoderController> audioDecoderController;
        int64_t audioBufferHandle;
        Pool audioPool;
        sp<IAVBufferSpaceListener> bufferListener = new AVBufferSpaceListener();

        /* Create Audio Pool */
        AVBUFFER_CREATE_AUDIOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, bufferListener, audioPool);

        //for(const auto& test: mTestConfiguration)
        {
            int64_t audioBufferHandle;
            sp<IAudioDecoderController> audioDecoderController;

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
                vector<uint8_t> inputBuffer; //TODO: replace with mapped buffer
                int32_t trimEndNs = 0;
                int32_t trimStartNs = 0;
                InputAudioFrame_t inputFrame;

                /* Allocate the audio buffer */
                AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, audioPool, frameSize, audioBufferHandle);

                inputBuffer.resize(frameSize);
                inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);

                //TODO: Fill the buffer and call decoder. Add different machanism for secure decode
                inputFrame.sendTime = chrono::steady_clock::now();
                inputFrame.framePTS = framePTS;
                inputFrame.reference = mTestConfiguration.reference;
                inputFrame.rms_threshold = mTestConfiguration.rms_diff_threashold;
                inputFrame.isErrorFrame = errorFrame;
                {
                    lock_guard<mutex> lock(mDecodeMutex);
                    mInputFrame.push_back(inputFrame);
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

                //AUDIODECODER_DECODEBUFFER_SUCCESS(audioDecoderController, framePTS, audioBufferHandle, trimStartNs, trimEndNs);

                //TODO: add some delay for the next frame to push
                this_thread::sleep_for(chrono::milliseconds(mTestConfiguration.decoderDelay));
                /* Free the audio buffer */
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, audioBufferHandle);
            }

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(mDecoderHandle, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(mDecoderHandle, audioDecoderController, true);
        }

        /* Destroy audio pool */
        AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, audioPool);

        mDecodingStream = false;
    }
    void consumerThreadFunc()
    {
        ifstream reference_file;

        while (mDecodingStream || !mInputFrame.empty())
        {
            DecodedAudioFrame_t decodedFrame;
            InputAudioFrame_t inputFrame;

            if(mInputFrame.empty())
            {
                this_thread::sleep_for(chrono::milliseconds(10));
                continue;
            }

            mControlListener->getNextFrame(decodedFrame);
            inputFrame = move(mInputFrame.front());
            mInputFrame.pop_front();

            if(!reference_file.is_open())
            {
                reference_file.open(inputFrame.reference);
            }

            if(decodedFrame.framePTS != inputFrame.framePTS)
            {
                UT_LOG_ERROR("input frame and decoded frame PTS is not matching..");
                UT_ASSERT_TRUE(false);
                continue;
            }
            if(inputFrame.isErrorFrame)
            {
                continue;
            }

            /* Verify the decodec buffer in non-tunnel mode */
            /* TODO: Check for trim start and end */
            if(!decodedFrame.data.empty())
            {
                vector<uint8_t> referenceBuffer;
                double rms_diff;
                int32_t output_buffer_size = decodedFrame.numSamples * vts_bytesPerSample(decodedFrame.metadata.format);
                referenceBuffer.resize(output_buffer_size);
                reference_file.read(reinterpret_cast<char*>(referenceBuffer.data()), output_buffer_size);

                //TODO: validate the decoded output
                rms_diff = vts_computeRMSDiff(referenceBuffer.data(), decodedFrame.data.data(), decodedFrame.numSamples, decodedFrame.metadata.format);
                UT_ASSERT_TRUE(rms_diff <= inputFrame.rms_threshold);
            }

            auto duration = chrono::duration_cast<chrono::microseconds>(decodedFrame.receiveTime - inputFrame.sendTime);
            UT_LOG_INFO("Decode Time in msec [PTS: %lld]: %lld", decodedFrame.framePTS, duration.count());
        }
    }
    void decodeAndConsumeAudioStreams()
    {
        sp<IAudioDecoderController> audioDecoderController;
        int64_t audioBufferHandle;
        Pool audioPool;
        sp<IAVBufferSpaceListener> bufferListener = new AVBufferSpaceListener();

        /* Create Audio Pool */
        AVBUFFER_CREATE_AUDIOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, bufferListener, audioPool);

        //for(const auto& testConfig: mTestConfiguration)
        {
            int64_t audioBufferHandle;
            sp<IAudioDecoderController> audioDecoderController;

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
                vector<uint8_t> inputBuffer; //TODO: replace with mapped buffer
                vector<uint8_t> referenceBuffer;
                size_t numSamples = 0, output_buffer_size;
                double rms_diff = 0.0;
                int32_t trimEndNs = 0;
                int32_t trimStartNs = 0;
                int32_t startFramesDecoded, startFrameErrors, startFramesDropped;
                int32_t endFramesDecoded, endFrameErrors, endFramesDropped;
                DecodedAudioFrame_t decodedFrame;

                chrono::steady_clock::time_point startTime, endTime;

                /* Allocate the audio buffer */
                AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, audioPool, frameSize, audioBufferHandle);

                inputBuffer.resize(frameSize);
                inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);

                //TODO: Fill the buffer and call decoder. Add different machanism for secure decode
                startTime = chrono::steady_clock::now();
                //AUDIODECODER_DECODEBUFFER_SUCCESS(audioDecoderController, framePTS, audioBufferHandle, trimStartNs, trimEndNs);

                //TODO: wait for the decoded output
                mControlListener->getNextFrame(decodedFrame);

                /* Verify the decodec buffer in non-tunnel mode */
                /* TODO: Check for trim start and end */
                if(!decodedFrame.data.empty())
                {
                    output_buffer_size = numSamples * vts_bytesPerSample(decodedFrame.metadata.format);
                    referenceBuffer.resize(output_buffer_size);
                    //refFile.read(reinterpret_cast<char*>(referenceBuffer.data()), output_buffer_size);

                    //TODO: validate the decoded output
                    rms_diff = vts_computeRMSDiff(referenceBuffer.data(), decodedFrame.data.data(), numSamples, decodedFrame.metadata.format);
                    UT_ASSERT_TRUE(rms_diff <= mTestConfiguration.rms_diff_threashold);
                }

                auto duration = chrono::duration_cast<chrono::microseconds>(endTime - startTime);
                UT_LOG_INFO("Decode Time [PTS: %lld]: %lld", framePTS, duration.count());

                startFramesDecoded++;

                /* If frame is a error frame increase the error count */
                if(errorFrame)
                {
                    startFrameErrors++;
                }

                /* Validate the decoder metrics */
                UT_ASSERT_TRUE(startFramesDecoded == endFramesDecoded);
                UT_ASSERT_TRUE(startFrameErrors == endFrameErrors);
                UT_ASSERT_TRUE(startFramesDropped == endFramesDropped);

                /* Free the audio buffer */
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, audioBufferHandle);
                break;
            }

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(mDecoderHandle, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(mDecoderHandle, audioDecoderController, true);
        }

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

    /*void similateonframeoutput(sp<AudioDecoderControllerListener> controlListener)
    {
        int64_t PTS[] = {0,21333333,42666666,63999999,85333332,106666665,127999998,149333331,170666664,191999997};
        optional<FrameMetadata> metadata;
        metadata->sourceCodec = Codec::AAC_LC;
        metadata->isDolbyAtmos = false;
        metadata->numChannels = 2;
        metadata->channelTypes = {ChannelType::FRONT_LEFT, ChannelType::FRONT_RIGHT};
        metadata->sampleRate = 48000;
        metadata->format = PCMFormat::S16LE;
        metadata->planarFormat = 0;
        metadata->trimStartNs = 0;
        metadata->trimEndNs = 0;
        metadata->lowLatency = 0;
        metadata->endOfStream = 0;
        metadata->discontinuity = 0;
        metadata->source = AVSource::IP;
        UT_LOG_INFO("similateonframeoutput: start simulation, listener: %p\n", controlListener.get());
        for(int32_t i=0;i<10;i++)
        {
            controlListener->onFrameOutput(PTS[i], -1, metadata);
            this_thread::sleep_for(chrono::milliseconds(50));
        }

    }*/
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
            //UT_ASSERT_TRUE(getDecoderMetrics(decoderHandle, startFramesDecoded, startFrameErrors, startFramesDropped));

            decodeTest.run(concurrentDecodeConsume);
            /*{
                this_thread::sleep_for(chrono::milliseconds(40));
                 vector<thread> threads;
                 UT_LOG_INFO("runSequentialDecode_v2: start simulation thread listener:%p\n", controlListener.get());
                 threads.emplace_back(&VTS_L3_AudioDecoder::similateonframeoutput, this,
                                            controlListener);
                for (auto& t : threads)
                {
                    t.join();
                }
            }*/
            //decodeTest.wait();

            /* Get decoder metrics after decoding */
            //UT_ASSERT_TRUE(getDecoderMetrics(decoderHandle, endFramesDecoded, endFrameErrors, endFramesDropped));

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
 * @brief Test the ability to decode audio frames in non-tunnel mode
 *
 * This test verifies that decoder can decode the audio frames in non-tunnel mode.
 *
 * **Test Group ID:** 03@n
 * **Test Case ID:** 001@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_NonTunnel)
{
    char *testName = "Decode_NonTunnel";
    vector<testConfiguration_t> testConfiguration;
    bool secureMode = false;

    vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, testConfiguration);

    for(auto& test: testConfiguration)
    {
        runSequentialDecode(test, secureMode, true);
    }
}

/**
 * @brief Test the ability to decode audio frames in secure mode and in non-tunnel mode
 *
 * This test verifies that decoder can decode the audio frames in secure mode and in non-tunnel mode.
 *
 * **Test Group ID:** 03@n
 * **Test Case ID:** 002@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_NonTunnel_Secure)
{
    char *testName = "Decode_NonTunnel_Secure";
    vector<testConfiguration_t> testConfiguration;
    bool secureMode = true;

    vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, testConfiguration);
    for(auto& test: testConfiguration)
    {
        runSequentialDecode(test, secureMode, true);
    }
}

/**
 * @brief Test the ability to decode audio frames in tunnel mode
 *
 * This test verifies that decoder can decode the audio frames in tunnel mode.
 *
 * **Test Group ID:** 03@n
 * **Test Case ID:** 003@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Tunnel)
{
    char *testName = "Decode_Tunnel";
    vector<testConfiguration_t> testConfiguration;
    bool secureMode = false;

    vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, testConfiguration);

    for(auto& test: testConfiguration)
    {
        runSequentialDecode(test, secureMode, true);
    }
}

/**
 * @brief Test the ability to decode audio frames in secure mode and in tunnel mode
 *
 * This test verifies that decoder can decode the audio frames in secure mode and in tunnel mode.
 *
 * **Test Group ID:** 03@n
 * **Test Case ID:** 004@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Tunnel_Secure)
{
    char *testName = "Decode_Tunnel_Secure";
    vector<testConfiguration_t> testConfiguration;
    bool secureMode = false;

    vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, testConfiguration);

    for(auto& test: testConfiguration)
    {
        runSequentialDecode(test, secureMode, true);
    }
}

/**
 * @brief Test the ability to handle invalid audio frames in secure and non secure mode and in tunnel and non-tunnel mode
 *
 * This test verifies that decoder can handle invalid audio frames.
 *
 * **Test Group ID:** 03@n
 * **Test Case ID:** 005@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Inavlid_Frames)
{
    char *testName = "Decode_Inavlid_Frames";
    vector<testConfiguration_t> testConfiguration;
    bool secureMode = false;

    vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, testConfiguration);

    for (bool secureDecode : {false, true})
    {
        for(auto& test: testConfiguration)
        {
            runSequentialDecode(test, secureMode, secureDecode);
        }
    }
}

/**
 * @brief Test the trim functionality of the decoder
 *
 * This test verifies trim functionality.
 *
 * **Test Group ID:** 03@n
 * **Test Case ID:** 006@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Trim)
{
    char *testName = "Decode_Trim";
    vector<testConfiguration_t> testConfiguration;
    bool secureMode = false;

    vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, testConfiguration);

    for (bool secureDecode : {false, true})
    {
        for(auto& test: testConfiguration)
        {
            runSequentialDecode(test, secureMode, secureDecode);
        }
    }
}

/**
 * @brief Test the ability to decode audio frames cuncurrently in non-tunnel mode
 *
 * This test verifies that decoder can decode the audio frames in non-tunnel mode.
 *
 * **Test Group ID:** 03@n
 * **Test Case ID:** 007@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, Decode_Cuncurrent_NonTunnel)
{
    char *testName = "Decode_Cuncurrent_NonTunnel";
    vector<testConfiguration_t> testConfiguration;
    bool secureMode = false;

    vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, testConfiguration);

    for (bool secureDecode : {false, true})
    {
        for(auto& test: testConfiguration)
        {
            runConcurrentDecode(test, secureMode, true);
        }
    }
}

/** @} */ // End of AUDIO_DECODER_HALTEST L3 File
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER Module
/** @} */ // End of HPK
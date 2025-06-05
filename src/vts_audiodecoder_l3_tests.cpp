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

class VTS_L3_AudioDecoder : public UTCore
{
public:
    VTS_L3_AudioDecoder() : UTCore() {}

    ~VTS_L3_AudioDecoder() override = default;

    public:
        std::vector<std::tuple<IAudioDecoder::Id, sp<IAudioDecoder>, sp<AudioDecoderControllerListener>, sp<AudioDecoderEventListener>, audiodecoder::Capabilities *>> mAudioDecoderList;
        sp<IAVBuffer> mAVBufferManager;

    void SetUp() override
    {
        std::vector<IAudioDecoder::Id> decoderIds;

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

    void checkCompleteStateTransition(sp<IAudioDecoder> decoder, sp<AudioDecoderEventListener> eventListener, State oldState, State intermediateState, State newState)
    {
        ::com::rdk::hal::State currentState;

        UT_ASSERT_TRUE(eventListener->checkStateTransistion(oldState, intermediateState));

        eventListener->waitForState(newState, std::chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS));

        AUDIODECODER_GET_STATE(decoder, currentState);

        UT_ASSERT_EQUAL(currentState, newState);

        UT_ASSERT_TRUE(eventListener->checkStateTransistion(intermediateState, newState));

        eventListener->resetStateChanges();
    }

    bool getDecoderMetrics(sp<IAudioDecoder> decoder, int32_t &framesDecoded, int32_t &decodeErrors, int32_t framesDropped)
    {
        std::optional<PropertyValue> propertyValue;

        framesDecoded = decodeErrors = framesDropped = -1;

        AUDIODECODER_GET_PROPERTY_SUCCESS(decoder, Property::METRIC_FRAMES_DECODED, propertyValue);
        if (!propertyValue.has_value() || !propertyValue->value.has_value())
        {
            return false;
        }
        framesDecoded = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

        AUDIODECODER_GET_PROPERTY_SUCCESS(decoder, Property::METRIC_DECODE_ERRORS, propertyValue);
        if (!propertyValue.has_value() || !propertyValue->value.has_value())
        {
            return false;
        }
        decodeErrors = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

        AUDIODECODER_GET_PROPERTY_SUCCESS(decoder, Property::METRIC_FRAMES_DROPPED, propertyValue);
        if (!propertyValue.has_value() || !propertyValue->value.has_value())
        {
            return false;
        }
        framesDropped = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

        return true;
    }

    void decodeAudioStreams(IAudioDecoder::Id decoderId, sp<IAudioDecoder> decoder, sp<AudioDecoderControllerListener> controlListener,
                            sp<AudioDecoderEventListener> eventListener,
                            bool secureDecode, std::vector<testConfiguration_t> streamDetails)
    {

        sp<IAudioDecoderController> audioDecoderController;
        int64_t audioBufferHandle;
        Pool audioPool;
        sp<IAVBufferSpaceListener> bufferListener = new AVBufferSpaceListener();

        /* Create Audio Pool */
        AVBUFFER_CREATE_AUDIOPOOL_SUCCESS(mAVBufferManager, secureDecode, decoderId, bufferListener, audioPool);

        for(const auto& stream: streamDetails)
        {
            int64_t audioBufferHandle;
            sp<IAudioDecoderController> audioDecoderController;

            std::ifstream inputFile(stream.input, std::ios::binary);
            if (!inputFile)
            {
                UT_LOG_ERROR("Failed to open file: %s", stream.input.c_str());
                break;
            }

            std::ifstream referenceFile(stream.reference, std::ios::binary);
            if (!referenceFile)
            {
                UT_LOG_ERROR("Failed to open reference file: %s", stream.reference.c_str());
                break;
            }

            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(decoder, stream.codec, secureDecode, controlListener, audioDecoderController, true);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(decoder, audioDecoderController, true);

            for(const auto& [frameSize, framePTS, errorFrame]: stream.frameDetails)
            {
                std::vector<uint8_t> inputBuffer; //TODO: replace with mapped buffer
                std::vector<uint8_t> referenceBuffer;
                size_t numSamples = 0, output_buffer_size;
                double rms_diff = 0.0;
                int32_t trimEndNs = 0;
                int32_t trimStartNs = 0;
                int32_t startFramesDecoded, startFrameErrors, startFramesDropped;
                int32_t endFramesDecoded, endFrameErrors, endFramesDropped;
                AudioFrame decodedFrame;

                std::chrono::steady_clock::time_point startTime, endTime;

                /* Allocate the audio buffer */
                AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, audioPool, frameSize, audioBufferHandle);

                inputBuffer.resize(frameSize);
                inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);

#if 0
                /* Get decoder metrics before decoding */
                UT_ASSERT_TRUE(getDecoderMetrics(decoder, startFramesDecoded, startFrameErrors, startFramesDropped));

                //TODO: Fill the buffer and call decoder. Add different machanism for secure decode
                startTime = std::chrono::steady_clock::now();
                //AUDIODECODER_DECODEBUFFER_SUCCESS(audioDecoderController, framePTS, audioBufferHandle, trimStartNs, trimEndNs);

                //TODO: wait for the decoded output
                controlListener->getNextFrame(decodedFrame);

                /* Verify the decodec buffer in non-tunnel mode */
                /* TODO: Check for trim start and end */
                if(!decodedFrame.data.empty())
                {
                    output_buffer_size = numSamples * vts_bytesPerSample(decodedFrame.metadata.format);
                    referenceBuffer.resize(output_buffer_size);
                    //refFile.read(reinterpret_cast<char*>(referenceBuffer.data()), output_buffer_size);

                    //TODO: validate the decoded output
                    rms_diff = vts_computeRMSDiff(referenceBuffer.data(), decodedFrame.data.data(), numSamples, decodedFrame.metadata.format);
                    UT_ASSERT_TRUE(rms_diff <= stream.rms_diff_threashold);
                }

                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
                UT_LOG_INFO("Decode Time [PTS: %lld]: %lld", framePTS, duration.count());

                /* Get decoder metrics after decoding */
                UT_ASSERT_TRUE(getDecoderMetrics(decoder, endFramesDecoded, endFrameErrors, endFramesDropped));

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
#endif
                /* Free the audio buffer */
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, audioBufferHandle);
                break;
            }

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(decoder, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(decoder, audioDecoderController, true);
        }

        /* Destroy audio pool */
        AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, audioPool);
    }
    void runSequentialDecode(std::vector<testConfiguration_t>& streamDetails, bool secureDecode)
    {
        for (const auto& [_decoderId, decoder, controlListener, eventListener, capabilities] : mAudioDecoderList)
        {
            Pool audioPool;
            sp<IAVBufferSpaceListener> listener = new AVBufferSpaceListener();
            HeapMetrics beforeHeapMetrics, afterHeapMetrics;

            //Skip if the plafrom doesn't support secure decode
            if (secureDecode && !capabilities->supportsSecure)
            {
                continue;
            }

            /* Get heap metrics */
            AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, beforeHeapMetrics);

            decodeAudioStreams(_decoderId, decoder, controlListener, eventListener, secureDecode, streamDetails);

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
    void runConcurrentDecode(std::vector<testConfiguration_t>& streamDetails, bool secureDecode)
    {
        std::vector<std::thread> threads;
        HeapMetrics beforeHeapMetrics, afterHeapMetrics;

        /* Get heap metrics */
        AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, beforeHeapMetrics);

        for (const auto& [_decoderId, decoder, controlListener, eventListener, capabilities] : mAudioDecoderList)
        {
            //Skip if the plafrom doesn't support secure decode
            if (secureDecode && !capabilities->supportsSecure)
            {
                continue;
            }

            threads.emplace_back(&VTS_L3_AudioDecoder::decodeAudioStreams, this,
                                            _decoderId, decoder, controlListener,
                                            eventListener, secureDecode, streamDetails);
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
    std::vector<testConfiguration_t> streamDetails;
    bool secureMode = false;

    vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, streamDetails);

    runSequentialDecode(streamDetails, secureMode);
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
    std::vector<testConfiguration_t> streamDetails;
    bool secureMode = true;

    vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, streamDetails);

    runSequentialDecode(streamDetails, secureMode);
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
    std::vector<testConfiguration_t> streamDetails;
    bool secureMode = false;

    vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, streamDetails);

    runSequentialDecode(streamDetails, secureMode);
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
    std::vector<testConfiguration_t> streamDetails;
    bool secureMode = true;

    vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, streamDetails);

    runSequentialDecode(streamDetails, secureMode);
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
    std::vector<testConfiguration_t> streamDetails;

    vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, streamDetails);

    for (bool secureDecode : {false, true})
    {
        runSequentialDecode(streamDetails, secureDecode);
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
    std::vector<testConfiguration_t> streamDetails;

    vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, streamDetails);

    for (bool secureDecode : {false, true})
    {
        runSequentialDecode(streamDetails, secureDecode);
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
    std::vector<testConfiguration_t> streamDetails;
    bool secureMode = false;

    vts_audiodeccoder_parse_testconfig(gTestConfiguration, gModuleName, gTestGroup, testName, streamDetails);

    for (bool secureDecode : {false, true})
    {
        runConcurrentDecode(streamDetails, secureMode);
    }
}

/** @} */ // End of AUDIO_DECODER_HALTEST L3 File
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER Module
/** @} */ // End of HPK
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
        gTestConfiguration = vts_kvp_create_instance("./test_configuration/vts_l3_stream_config.yaml");

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
    std::vector<testStreamConfig_t> streamDetails;

    vts_testconfig_getStream_details(gTestConfiguration, gModuleName, gTestGroup, testName, streamDetails);

    for (const auto& [_decoderId, decoder, controlListener, eventListener, capabilities] : mAudioDecoderList)
    {
        Pool audioPool;
        sp<IAVBufferSpaceListener> listener = new AVBufferSpaceListener();

        /* Create Audio Pool */
        AVBUFFER_CREATE_AUDIOPOOL_SUCCESS(mAVBufferManager, false, _decoderId, listener, audioPool);

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
            AUDIODECODER_OPEN_SUCCESS(decoder, stream.codec, false, controlListener, audioDecoderController, true);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(decoder, audioDecoderController, true);

            for(const auto& [frameSize, framePTS]: stream.frameDetails)
            {
                std::vector<uint8_t> decodedBuffer;
                std::vector<uint8_t> inputBuffer; //TODO: replace with mapped buffer
                std::vector<uint8_t> referenceBuffer;
                size_t numSamples = 0, output_buffer_size;
                PCMFormat format = PCMFormat::S16LE;
                double rms_diff = 0.0;
                int32_t trimEndNs = 0;
                int32_t trimStartNs = 0;
                std::chrono::steady_clock::time_point startTime, endTime;

                /* Allocate the audio buffer */
                AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, audioPool, frameSize, audioBufferHandle);

                inputBuffer.resize(frameSize);
                inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);

                //TODO: Fill the buffer and call decoder and verify the output
                startTime = std::chrono::steady_clock::now();
                //AUDIODECODER_DECODEBUFFER_SUCCESS(audioDecoderController, framePTS, audioBufferHandle, trimEndNs, trimStartNs);

                //TODO: wait for the decoded output
                //controlListener->getNextFrame(decodedBuffer, numSamples, format, endTime);

                //output_buffer_size = numSamples * vts_bytesPerSample(format);
                //referenceBuffer.resize(output_buffer_size);
                //referenceFile.read(reinterpret_cast<char*>(referenceBuffer.data()), output_buffer_size);

                //TODO: validate the decoded output
                //rms_diff = vts_computeRMSDiff(referenceBuffer.data(), decodedBuffer.data(), numSamples, format);
                //UT_ASSERT_TRUE(rms_diff <= stream.rms_diff_threashold);

                //TODO: print decode time
                //auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
                //UT_LOG_INFO("Decode Time [PTS: %lld]: %lld", framePTS, duration.count());
                //UT_LOG_INFO("test: %lld",std::chrono::duration_cast<std::chrono::microseconds>(startTime.time_since_epoch()).count());

                /* Free the audio buffer */
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, audioBufferHandle);
            }

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(decoder, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(decoder, audioDecoderController, true);
        }

        /* Destroy Audio Pool */
        AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, audioPool);

        /* TODO: check if this required or not */
        //controlListener->shutdown();
    }
}

/** @} */ // End of AUDIO_DECODER_HALTEST L3 File
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER Module
/** @} */ // End of HPK
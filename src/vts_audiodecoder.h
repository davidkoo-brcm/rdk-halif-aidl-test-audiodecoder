/**
 *  If not stated otherwise in this file or this component's LICENSE
 *  file the following copyright and licenses apply:
 *
 *  Copyright 2024 RDK Management
 *
 *  Licensed under the Apache License, Version 2.0 (the License);
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an AS IS BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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
 * @defgroup AUDIO_DECODER_HALTEST_HEADER Audio Decoder HAL Tests helper functions Header File
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
 * @file vts_audiodecoder.h
 *
 */

#ifndef __VTS_AUDIODECODER_H__
#define __VTS_AUDIODECODER_H__

#include <thread>
#include <chrono>
#include <deque>
#include <condition_variable>

#include "com/rdk/hal/audiodecoder/IAudioDecoderManager.h"
#include "com/rdk/hal/audiodecoder/BnAudioDecoderControllerListener.h"
#include "com/rdk/hal/audiodecoder/BnAudioDecoderEventListener.h"

#include "com/rdk/hal/avbuffer/IAVBuffer.h"
#include "com/rdk/hal/avbuffer/BnAVBufferSpaceListener.h"

using android::sp;
using android::String16;
using android::defaultServiceManager;
using android::binder::Status;
using namespace std::chrono;

using namespace com::rdk::hal;
using namespace com::rdk::hal::audiodecoder;
using namespace com::rdk::hal::avbuffer;

extern sp<IAudioDecoderManager> gAudioDecoderManager;
extern void *gTestConfiguration;

/* Status::Exception */
const static ut_control_keyStringMapping_t statusException_mapTable[] =
{
    {"EX_NONE",                  (int32_t) Status::EX_NONE},
    {"EX_SECURITY",              (int32_t) Status::EX_SECURITY},
    {"EX_BAD_PARCELABLE",        (int32_t) Status::EX_BAD_PARCELABLE},
    {"EX_ILLEGAL_ARGUMENT",      (int32_t) Status::EX_ILLEGAL_ARGUMENT},
    {"EX_NULL_POINTER",          (int32_t) Status::EX_NULL_POINTER},
    {"EX_ILLEGAL_STATE",         (int32_t) Status::EX_ILLEGAL_STATE},
    {"EX_NETWORK_MAIN_THREAD",   (int32_t) Status::EX_NETWORK_MAIN_THREAD},
    {"EX_UNSUPPORTED_OPERATION", (int32_t) Status::EX_UNSUPPORTED_OPERATION},
    {"EX_SERVICE_SPECIFIC",      (int32_t) Status::EX_SERVICE_SPECIFIC},
    {"EX_PARCELABLE",            (int32_t) Status::EX_PARCELABLE},
    {"EX_HAS_REPLY_HEADER",      (int32_t) Status::EX_HAS_REPLY_HEADER},
    {"EX_TRANSACTION_FAILED",    (int32_t) Status::EX_TRANSACTION_FAILED},
    {NULL, -1}
};

/* bool */
const static ut_control_keyStringMapping_t bool_mapTable[] =
{
    {"false", (int32_t) false},
    {"true",  (int32_t) true},
    {NULL, -1}
};

#define MAX_TRANSITORY_STATE_WAIT_MS  5
#define MAX_FINAL_STATE_WAIT_MS  500

#define AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getAudioDecoderIds(OUT:decoderIds:[])"); \
        status = gAudioDecoderManager->getAudioDecoderIds(&decoderIds); \
        UT_LOG_INFO("Result getAudioDecoderIds(OUT:decoderIds:[size: %d]), status:[isOK:%s, exception:%s]", \
                                decoderIds.size(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_FALSE_FATAL(decoderIds.empty()); \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[])", _decoderId.value); \
        status = gAudioDecoderManager->getAudioDecoder(_decoderId, &audioDecoder); \
        UT_LOG_INFO("Result getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[0x%X]), status:[isOK:%s, exception:%s]", \
                                _decoderId.value, audioDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_TRUE_FATAL(audioDecoder.get() != nullptr); \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_MANAGER_GETAUDIODECODER_FAILURE(_decoderId, audioDecoder) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[])", _decoderId.value); \
        status = gAudioDecoderManager->getAudioDecoder(_decoderId, &audioDecoder); \
        UT_LOG_INFO("Result getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[0x%X]), status:[isOK:%s, exception:%s]", \
                                _decoderId.value, audioDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_TRUE(audiodecoder.get() == nullptr); \
    } while (0)

#define AUDIODECODER_GETCAPABILITIES(audioDecoder, capabilities) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getCapabilities(OUT:capabilities)"); \
        audioDecoder->getCapabilities(capabilities); \
        UT_LOG_INFO("Result getCapabilities(OUT:capabilities), status:[isOK:%s, exception:%s]", \
                                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_GET_PROPERTY_SUCCESS(audioDecoder, property, propertyValue) \
    do { \
        ::android::binder::Status status; \
        ::com::rdk::hal::State state; \
        UT_LOG_INFO("Calling getProperty(IN:property[%s], OUT:propertyValue[]", audiodecoder::toString(property).c_str()); \
        status = (audioDecoder)->getProperty(property, &propertyValue); \
        switch (propertyValue->value.value().getTag()) \
        { \
            case PropertyValue::Value::Tag::intValue: \
                UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[%d]), status:[isOK:%s, exception:%s]", audiodecoder::toString(property).c_str(), \
                propertyValue->value->get<PropertyValue::Value::Tag::intValue>(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
                break; \
            case PropertyValue::Value::Tag::stringValue: \
                UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[%s]), status:[isOK:%s, exception:%s]", audiodecoder::toString(property).c_str(), \
                propertyValue->value->get<PropertyValue::Value::Tag::stringValue>().string(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
                break; \
        } \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_GET_PROPERTY_FAILURE(audioDecoder, property, propertyValue, exception) \
    do { \
        ::android::binder::Status status; \
        ::com::rdk::hal::State state; \
        UT_LOG_INFO("Calling getProperty(IN:property[%s], OUT:propertyValue[]", audiodecoder::toString(property).c_str()); \
        status = (audioDecoder)->getProperty(property, &propertyValue); \
        UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[]), status:[isOK:%s, exception:%s]", audiodecoder::toString(property).c_str(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

#define AUDIODECODER_GET_STATE(audioDecoder, state) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getState(OUT:state[]"); \
        status = (audioDecoder)->getState(&state); \
        UT_LOG_INFO("Result getState(OUT:state[%s]), status:[isOK:%s, exception:%s]", com::rdk::hal::toString(state).c_str(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_WAIT_FOR_STATE(audioDecoder, desiredState, lastState) \
    do { \
        int32_t sleep_time = 0; \
        do \
        { \
            std::this_thread::sleep_for(std::chrono::milliseconds(MAX_TRANSITORY_STATE_WAIT_MS)); \
            sleep_time += MAX_TRANSITORY_STATE_WAIT_MS; \
            /* Check the state of the decoder */ \
            AUDIODECODER_GET_STATE((audioDecoder), lastState); \
        } while (sleep_time < MAX_FINAL_STATE_WAIT_MS && lastState != desiredState); \
    } while (0)

#define AUDIODECODER_OPEN_SUCCESS(audioDecoder, codec, secure, controllerListener, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        \
        /* Open the Decoder */ \
        UT_LOG_INFO("Calling open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%p], OUT:audioDecoderController[])", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener); \
        status = (audioDecoder)->open(codec, secure, controllerListener, &audioDecoderController); \
        UT_LOG_INFO("Result open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%p], OUT:audioDecoderController[0x%X]), status:[isOK:%s, exception:%s]", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener, audioDecoderController.get(), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_NOT_EQUAL_FATAL(audioDecoderController.get(), nullptr); \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
        \
        if (stateCheck) \
        { \
            ::com::rdk::hal::State lastState; \
            AUDIODECODER_WAIT_FOR_STATE(audioDecoder, State::READY, lastState); \
            UT_ASSERT_EQUAL(lastState, State::READY); \
        } \
    } while (0)

#define AUDIODECODER_OPEN_FAILURE(audioDecoder, codec, secure, controllerListener, audioDecoderController, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%p], OUT:audioDecoderController[])", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener); \
        status = audioDecoder->open(codec, secure, controllerListener, &audioDecoderController); \
        UT_LOG_INFO("Result open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%p], OUT:audioDecoderController[0x%X]), status:[isOK:%s, exception:%s]", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener, audioDecoderController.get(), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_EQUAL(audioDecoderController.get(), nullptr); \
    } while (0)

#define AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling close(IN:audioDecoderController[0x%p], OUT:_aidl_return[])", audioDecoderController.get()); \
        status = audioDecoder->close(audioDecoderController, &_aidl_return); \
        UT_LOG_INFO("Result close(IN:audioDecoderController[0x%p], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            audioDecoderController.get(), UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_TRUE_FATAL(_aidl_return); \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) \
        { \
            ::com::rdk::hal::State lastState; \
            AUDIODECODER_WAIT_FOR_STATE(audioDecoder, State::CLOSED, lastState); \
            UT_ASSERT_EQUAL(lastState, State::CLOSED); \
        } \
    } while (0)

#define AUDIODECODER_CLOSE_FAILURE(audioDecoder, audioDecoderController, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling close(IN:audioDecoderController[0x%p], OUT:_aidl_return[])", audioDecoderController.get()); \
        status = audioDecoder->close(audioDecoderController, &_aidl_return); \
        UT_LOG_INFO("Result close(IN:audioDecoderController[0x%p], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            audioDecoderController.get(), UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(_aidl_return); \
    } while (0)

#define AUDIODECODER_REGISTER_EVENT_SUCCESS(audioDecoder, eventListener) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling registerEventListener(IN:eventListener[0x%p], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->registerEventListener(eventListener, &_aidl_return); \
        UT_LOG_INFO("Result registerEventListener(IN:eventListener[0x%p], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            eventListener.get(), UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        UT_ASSERT_TRUE(_aidl_return); \
    } while (0)

#define AUDIODECODER_REGISTER_EVENT_FAILURE(audioDecoder, eventListener, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling registerEventListener(IN:eventListener[0x%p], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->registerEventListener(eventListener, &_aidl_return); \
        UT_LOG_INFO("Result registerEventListener(IN:eventListener[0x%p], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            eventListener.get(), UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        if (exception == Status::EX_NONE) \
            UT_ASSERT_TRUE(status.isOk()); \
        else \
            UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(_aidl_return); \
    } while (0)

#define AUDIODECODER_UNREGISTER_EVENT_SUCCESS(audioDecoder, eventListener) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling unregisterEventListener(IN:eventListener[0x%p], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->unregisterEventListener(eventListener, &_aidl_return); \
        UT_LOG_INFO("Result unregisterEventListener(IN:eventListener[0x%p], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            eventListener.get(), UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE(_aidl_return); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_UNREGISTER_EVENT_FAILURE(audioDecoder, eventListener, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling unregisterEventListener(IN:eventListener[0x%p], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->unregisterEventListener(eventListener, &_aidl_return); \
        UT_LOG_INFO("Result unregisterEventListener(IN:eventListener[0x%p], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            eventListener.get(), UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(_aidl_return); \
    } while (0)

#define AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling start()"); \
        status = audioDecoderController->start(); \
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) \
        { \
            ::com::rdk::hal::State lastState; \
            AUDIODECODER_WAIT_FOR_STATE(audioDecoder, State::STARTED, lastState); \
            UT_ASSERT_EQUAL(lastState, State::STARTED); \
        } \
    } while (0)

#define AUDIODECODER_START_FAILURE(audioDecoderController, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling start()"); \
        status = audioDecoderController->start(); \
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling stop()"); \
        status = audioDecoderController->stop(); \
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) \
        { \
            ::com::rdk::hal::State lastState; \
            AUDIODECODER_WAIT_FOR_STATE(audioDecoder, State::READY, lastState); \
            UT_ASSERT_EQUAL(lastState, State::READY); \
        } \
    } while (0)

#define AUDIODECODER_STOP_FAILURE(audioDecoderController, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling stop()"); \
        status = audioDecoderController->stop(); \
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_DECODEBUFFER_SUCCESS(audioDecoderController, nsPresentationTime, bufferHandle, trimEndNs, trimStartNs) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling decodeBuffer(IN:nsPresentationTime[%ld], IN:bufferHandle[0x%X], trimStartNs[%d], trimEndNs[%d], OUT:_aidl_return[])", \
            nsPresentationTime, bufferHandle, trimStartNs, trimEndNs); \
        status = audioDecoderController->decodeBuffer(nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, &_aidl_return); \
        UT_LOG_INFO("Result decodeBuffer(IN:nsPresentationTime[%ld], IN:bufferHandle[0x%X], trimStartNs[%d], trimEndNs[%d], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_DECODEBUFFER_FAILURE(audioDecoderController, nsPresentationTime, bufferHandle, trimEndNs, trimStartNs, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling decodeBuffer(IN:nsPresentationTime[%ld], IN:bufferHandle[0x%X], trimStartNs[%d], trimEndNs[%d], OUT:_aidl_return[])", \
            nsPresentationTime, bufferHandle, trimStartNs, trimEndNs); \
        status = audioDecoderController->decodeBuffer(nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, &_aidl_return); \
        UT_LOG_INFO("Result decodeBuffer(IN:nsPresentationTime[%ld], IN:bufferHandle[0x%X], trimStartNs[%d], trimEndNs[%d], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

#define AUDIODECODER_FLUSH_SUCCESS(audioDecoder, audioDecoderController, reset, stateCheck) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling flush(IN: reset[%s])", UT_Control_GetMapString(bool_mapTable, reset)); \
        status = audioDecoderController->flush(reset); \
        UT_LOG_INFO("Result flush(IN: reset[%s]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, reset), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) \
        { \
            std::this_thread::sleep_for(std::chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS)); \
            if (reset) \
            { \
                ::com::rdk::hal::State lastState; \
                AUDIODECODER_WAIT_FOR_STATE(audioDecoder, State::READY, lastState); \
                UT_ASSERT_EQUAL(lastState, State::READY); \
            } \
            else \
            { \
                ::com::rdk::hal::State lastState; \
                AUDIODECODER_WAIT_FOR_STATE(audioDecoder, State::STARTED, lastState); \
                UT_ASSERT_EQUAL(lastState, State::STARTED); \
            } \
        } \
    } while (0)

#define AUDIODECODER_FLUSH_FAILURE(audioDecoderController, reset, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling flush(IN: reset[%s])", UT_Control_GetMapString(bool_mapTable, reset)); \
        status = audioDecoderController->flush(reset); \
        UT_LOG_INFO("Result flush(IN: reset[%s]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, reset), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_SIGNAL_DISCONTINUITY_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        ::com::rdk::hal::State state; \
        UT_LOG_INFO("Calling signalDiscontinuity()"); \
        status = audioDecoderController->signalDiscontinuity(); \
        UT_LOG_INFO("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) \
        { \
            ::com::rdk::hal::State lastState; \
            AUDIODECODER_WAIT_FOR_STATE(audioDecoder, State::STARTED, lastState); \
            UT_ASSERT_EQUAL(lastState, State::STARTED); \
        } \
    } while (0)

#define AUDIODECODER_SIGNAL_DISCONTINUITY_FAILURE(audioDecoderController, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling signalDiscontinuity()"); \
        status = audioDecoderController->signalDiscontinuity(); \
        UT_LOG_INFO("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_SIGNAL_EOS_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling signalEOS()"); \
        status = audioDecoderController->signalEOS(); \
        UT_LOG_INFO("Result signalEOS(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) \
        { \
            ::com::rdk::hal::State lastState; \
            AUDIODECODER_WAIT_FOR_STATE(audioDecoder, State::STARTED, lastState); \
            UT_ASSERT_EQUAL(lastState, State::STARTED); \
        } \
    } while (0)

#define AUDIODECODER_SIGNAL_EOS_FAILURE(audioDecoderController, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling signalEOS()"); \
        status = audioDecoderController->signalEOS(); \
        UT_LOG_INFO("Result signalEOS(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_PARSE_CSD_SUCCESS(audioDecoder, audioDecoderController, csdAudioFormat, codecData, stateCheck) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdAudioFormat[%d], IN:codecData[0x%X], OUT:_aidl_return[])", csdAudioFormat, codecData); \
        status = audioDecoderController->parseCodecSpecificData(csdAudioFormat, codecData, &_aidl_return); \
        UT_LOG_INFO("Result parseCodecSpecificData(IN:csdAudioFormat[%d], IN:codecData[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", csdAudioFormat, codecData, \
            UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE(_aidl_return); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) \
        { \
            ::com::rdk::hal::State lastState; \
            AUDIODECODER_WAIT_FOR_STATE(audioDecoder, State::STARTED, lastState); \
            UT_ASSERT_EQUAL(lastState, State::STARTED); \
        } \
    } while (0)

#define AUDIODECODER_PARSE_CSD_FAILURE(audioDecoderController, csdAudioFormat, codecData, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdAudioFormat[%d], IN:codecData[0x%X], OUT:_aidl_return[])", csdAudioFormat, codecData); \
        status = audioDecoderController->parseCodecSpecificData(csdAudioFormat, codecData, &_aidl_return); \
        UT_LOG_INFO("Result parseCodecSpecificData(IN:csdAudioFormat[%d], IN:codecData[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", csdAudioFormat, codecData, \
            UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(_aidl_return); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, property, propertyValue) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling setProperty(IN:property[%s], IN:propertyValue[0x%X], OUT:_aidl_return[])", com::rdk::hal::audiodecoder::toString(property).c_str(), propertyValue.value); \
        status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return); \
        UT_LOG_INFO("Result setProperty(IN:property[%s], IN:propertyValue[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", com::rdk::hal::audiodecoder::toString(property).c_str(), propertyValue.value, \
            UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE(_aidl_return); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_SETPROPERTY_FAILURE(audioDecoderController, property, propertyValue, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling setProperty(IN:property[%s], IN:propertyValue[0x%X], OUT:_aidl_return[])", com::rdk::hal::audiodecoder::toString(property).c_str(), propertyValue.value); \
        status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return); \
        UT_LOG_INFO("Result setProperty(IN:property[%s], IN:propertyValue[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", com::rdk::hal::audiodecoder::toString(property).c_str(), propertyValue.value, \
            UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(_aidl_return); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_SETPROPERTY_LIST_SUCCESS(audioDecoderController, properyList, type) \
    do { \
        for (const auto& [property, value] : properyList) \
        { \
            PropertyValue propertyValue; \
            propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::type>(value); \
            AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, property, propertyValue); \
        } \
    } while (0)

#define AUDIODECODER_SETPROPERTY_STRING_LIST_SUCCESS(audioDecoderController, properyList) \
    do { \
        for (const auto& [property, value] : properyList) \
        { \
            PropertyValue propertyValue; \
            propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::stringValue>(value); \
            AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, property, propertyValue); \
        } \
    } while (0)

#define AUDIODECODER_SETPROPERTY_LIST_FAILURE(audioDecoderController, properyList, type, exception) \
    do { \
        for (const auto& [property, value] : properyList) \
        { \
            PropertyValue propertyValue; \
            propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::type>(value); \
            AUDIODECODER_SETPROPERTY_FAILURE(audioDecoderController, property, propertyValue, exception); \
        } \
    } while (0)

#define AUDIODECODER_SETPROPERTY_STRING_LIST_FAILURE(audioDecoderController, properyList, exception) \
    do { \
        for (const auto& [property, value] : properyList) \
        { \
            PropertyValue propertyValue; \
            propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::stringValue>(value); \
            AUDIODECODER_SETPROPERTY_FAILURE(audioDecoderController, property, propertyValue, exception); \
        } \
    } while (0)

struct AudioMetatData
{
    Codec sourceCodec;
    bool isDolbyAtmos;
    int32_t numChannels;
    int32_t sampleRate;
    PCMFormat format;
    bool planarFormat;
    int32_t trimStartNs;
    int32_t trimEndNs;
    bool lowLatency;
    bool endOfStream;
    bool discontinuity;
    ::std::vector<audiodecoder::ChannelType> channelTypes;
    AVSource source;
};

typedef struct AudioFrame
{
    std::vector<uint8_t> data;
    size_t numSamples;
    bool isMetaAvailable;
    struct AudioMetatData metadata;
    std::chrono::steady_clock::time_point receiveTime;
}AudioFrame_t;

class AudioDecoderControllerListener : public BnAudioDecoderControllerListener
{
    public:
        AudioDecoderControllerListener() : BnAudioDecoderControllerListener(), mShutdownFlag(false){}
        virtual ~AudioDecoderControllerListener() = default;

        Status onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::std::optional<FrameMetadata>& metadata) override
        {
            UT_LOG_INFO("Received frameoutput: %lld", nsPresentationTime);
            //TODO: update these variables
            uint32_t numSamples = 0;
            uint32_t bps = 0;

            //TODO: Update the code with av buffer helper
            //TODO: metadata its not always valid
            AudioFrame_t frame;
            frame.receiveTime = std::chrono::steady_clock::now();
            if(frameBufferHandle != -1)
            {
                frame.numSamples = numSamples;
                frame.data.resize(numSamples * bps);
                //std::memcpy(frame.data.data(), decodedData, frame.data.size());
            }

            frame.isMetaAvailable = false;
            if (metadata.has_value())
            {
                frame.isMetaAvailable = true;
                frame.metadata.sourceCodec = metadata->sourceCodec;
                frame.metadata.isDolbyAtmos = metadata->isDolbyAtmos;
                frame.metadata.numChannels = metadata->numChannels;
                frame.metadata.channelTypes = metadata->channelTypes;
                frame.metadata.sampleRate = metadata->sampleRate;
                frame.metadata.format = metadata->format;
                frame.metadata.planarFormat = metadata->planarFormat;
                frame.metadata.trimStartNs = metadata->trimStartNs;
                frame.metadata.trimEndNs = metadata->trimEndNs;
                frame.metadata.lowLatency = metadata->lowLatency;
                frame.metadata.endOfStream = metadata->endOfStream;
                frame.metadata.discontinuity = metadata->discontinuity;
                frame.metadata.source = metadata->source;
            }

            {
                std::lock_guard<std::mutex> lock(mQueueMutex);
                mFrameQueue.push_back(std::move(frame));
            }

            mFrameAvailable.notify_one();

            return ::android::binder::Status::ok();
        }

        bool getNextFrame(AudioFrame_t &frameInfo)
        {
            std::unique_lock<std::mutex> lock(mQueueMutex);

            /*mFrameAvailable.wait(lock, [this]{
                return !mFrameQueue.empty() || mShutdownFlag;
            });*/
            if (mShutdownFlag && mFrameQueue.empty())
            {
                return false;
            }
            AudioFrame_t frame = std::move(mFrameQueue.front());
            mFrameQueue.pop_front();

            frameInfo = frame;

            return true;
        }
        void shutdown()
        {
            mShutdownFlag = true;
            mFrameAvailable.notify_all();
        }
    private:
        std::deque<AudioFrame_t> mFrameQueue;
        std::mutex mQueueMutex;
        std::condition_variable mFrameAvailable;
        std::atomic<bool> mShutdownFlag;
};

class AudioDecoderEventListener : public BnAudioDecoderEventListener
{
    public:
        AudioDecoderEventListener(): mCurrentState(State::CLOSED) {}
        virtual ~AudioDecoderEventListener() = default;

        Status onDecodeError(ErrorCode vendorErrorCode, int32_t errorCode) override
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mErrors.push_back(errorCode);
            return Status::ok();
        }
        Status onStateChanged(State oldState,State newState) override
        {
            UT_LOG_INFO("[%p]State Change received: %s -> %s", this, com::rdk::hal::toString(oldState).c_str(), com::rdk::hal::toString(newState).c_str());
            {
                std::lock_guard<std::mutex> lock(mMutex);
                mStateChanges.emplace_back(oldState, newState);
                mCurrentState = newState;
            }
            mCV.notify_all(); // Unblock any waiting test code
            return Status::ok();
        }
        /**
         * Wait for the decoder to reach a particular State within a given timeout.
         *
         * @param desiredState The state we want to observe.
         * @param timeout      How long to wait, e.g. std::chrono::milliseconds(500).
         * @return true if the state was reached; false if we timed out waiting.
         */
        bool waitForState(State desiredState, std::chrono::milliseconds timeout)
        {
            std::unique_lock<std::mutex> lock(mMutex);
            // Wait until mCurrentState == desiredState or until timeout
            return mCV.wait_for(lock, timeout, [this, desiredState]() {
                return (mCurrentState == desiredState);
            });
        }
        // Accessors for test verifications if desired
        std::vector<std::pair<State, State>> getStateChanges() const
        {
            std::lock_guard<std::mutex> lock(mMutex);
            return mStateChanges;
        }

        void resetStateChanges()
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mStateChanges.clear();
        }

        bool checkStateTransistion(State expOldState, State expNewState)
        {
            bool foundClosedToOpening = false;
            for (const auto &[oldState, newState] : mStateChanges)
            {
                if (oldState == expOldState && newState == expNewState)
                {
                    foundClosedToOpening = true;
                    break;
                }
            }
            return foundClosedToOpening;
        }

        std::vector<int32_t> getErrors() const
        {
            std::lock_guard<std::mutex> lock(mMutex);
            return mErrors;
        }
    private:
        mutable std::mutex mMutex;
        std::condition_variable mCV;

        State mCurrentState;
        std::vector<std::pair<State, State>> mStateChanges;
        std::vector<int32_t> mErrors;
};

#endif //__VTS_AUDIODECODER_H__

/** @} */ // End of AUDIO_DECODER_HALTEST_HEADER
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK

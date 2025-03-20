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

#include "com/rdk/hal/audiodecoder/IAudioDecoderManager.h"
#include "com/rdk/hal/audiodecoder/BnAudioDecoderControllerListener.h"
#include "com/rdk/hal/audiodecoder/BnAudioDecoderEventListener.h"

using android::sp;
using android::String16;
using android::defaultServiceManager;
using android::binder::Status;

using namespace com::rdk::hal;
using namespace com::rdk::hal::audiodecoder;

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

#define AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getAudioDecoderIds(OUT:decoderIds:[])"); \
        status = gAudioDecoderManager->getAudioDecoderIds(&decoderIds); \
        UT_LOG_INFO("Result getAudioDecoderIds(OUT:decoderIds:[size: %d]), status:[isOK:%s, exception:%s]", \
                                decoderIds.size(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_FALSE(decoderIds.empty()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[])", _decoderId.value); \
        status = gAudioDecoderManager->getAudioDecoder(_decoderId, &audioDecoder); \
        UT_LOG_INFO("Result getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[0x%X]), status:[isOK:%s, exception:%s]", \
                                _decoderId.value, audioDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE(audioDecoder.get() != nullptr); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
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
        audioDecoder->getCapabilities(&capabilities); \
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
        UT_LOG_INFO("Calling getProperty(IN:property[%s], OUT:propertyValue[]", com::rdk::hal::audiodecoder::toString(property).c_str()); \
        status = (audioDecoder)->getProperty(property, &propertyValue); \
        UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[]), status:[isOK:%s, exception:%s]", com::rdk::hal::audiodecoder::toString(property).c_str(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE(vts_audiodecoder_helper_validatePropertyValue(_decoderId.value, propertyValue, property)); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_GET_PROPERTY_FAILURE(audioDecoder, property, propertyValue, exception) \
    do { \
        ::android::binder::Status status; \
        ::com::rdk::hal::State state; \
        UT_LOG_INFO("Calling getProperty(IN:property[%s], OUT:propertyValue[]", com::rdk::hal::audiodecoder::toString(property).c_str()); \
        status = (audioDecoder)->getProperty(property, &propertyValue); \
        UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[]), status:[isOK:%s, exception:%s]", com::rdk::hal::audiodecoder::toString(property).c_str(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

#define AUDIODECODER_GET_STATE(audioDecoder, expected_state) \
    do { \
        ::android::binder::Status status; \
        ::com::rdk::hal::State state; \
        UT_LOG_INFO("Calling getState(OUT:state[]"); \
        status = (audioDecoder)->getState(&state); \
        UT_LOG_INFO("Result getState(OUT:state[%s]), status:[isOK:%s, exception:%s]", com::rdk::hal::toString(state).c_str(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(state, expected_state); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_OPEN_SUCCESS(audioDecoder, codec, secure, controllerListener, audioDecoderController) \
    do { \
        ::android::binder::Status status; \
        \
        /* Open the Decoder */ \
        UT_LOG_INFO("Calling open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:audioDecoderController[])", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener); \
        status = (audioDecoder)->open(codec, secure, controllerListener, &audioDecoderController); \
        UT_LOG_INFO("Result open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:audioDecoderController[0x%X]), status:[isOK:%s, exception:%s]", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener, audioDecoderController.get(), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_NOT_EQUAL(audioDecoderController.get(), nullptr); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        \
        /* Check the state of the decoder */ \
        AUDIODECODER_GET_STATE((audioDecoder), State::READY); \
    } while (0)

#define AUDIODECODER_OPEN_FAILURE(audioDecoder, codec, secure, controllerListener, audioDecoderController, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:audioDecoderController[])", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener); \
        status = audioDecoder->open(codec, secure, controllerListener, &audioDecoderController); \
        UT_LOG_INFO("Result open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:audioDecoderController[0x%X]), status:[isOK:%s, exception:%s]", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener, audioDecoderController.get(), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_EQUAL(audioDecoderController.get(), nullptr); \
    } while (0)

#define AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling close(IN:audioDecoderController[0x%X], OUT:_aidl_return[])", audioDecoderController.get()); \
        status = audioDecoder->close(audioDecoderController, &_aidl_return); \
        UT_LOG_INFO("Result close(IN:audioDecoderController[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            audioDecoderController.get(), UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE(_aidl_return); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        AUDIODECODER_GET_STATE((audioDecoder), State::CLOSED); \
    } while (0)

#define AUDIODECODER_CLOSE_FAILURE(audioDecoder, audioDecoderController, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling close(IN:audioDecoderController[0x%X], OUT:_aidl_return[])", audioDecoderController.get()); \
        status = audioDecoder->close(audioDecoderController, &_aidl_return); \
        UT_LOG_INFO("Result close(IN:audioDecoderController[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
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
        UT_LOG_INFO("Calling registerEventListener(IN:eventListener[0x%X], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->registerEventListener(eventListener, &_aidl_return); \
        UT_LOG_INFO("Result registerEventListener(IN:eventListener[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
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
        UT_LOG_INFO("Calling registerEventListener(IN:eventListener[0x%X], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->registerEventListener(eventListener, &_aidl_return); \
        UT_LOG_INFO("Result registerEventListener(IN:eventListener[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            eventListener.get(), UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(_aidl_return); \
    } while (0)

#define AUDIODECODER_UNREGISTER_EVENT_SUCCESS(audioDecoder, eventListener) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling unregisterEventListener(IN:eventListener[0x%X], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->unregisterEventListener(eventListener, &_aidl_return); \
        UT_LOG_INFO("Result unregisterEventListener(IN:eventListener[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            eventListener.get(), UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE(_aidl_return); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_UNREGISTER_EVENT_FAILURE(audioDecoder, eventListener) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling unregisterEventListener(IN:eventListener[0x%X], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->unregisterEventListener(eventListener, &_aidl_return); \
        UT_LOG_INFO("Result unregisterEventListener(IN:eventListener[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            eventListener.get(), UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_FALSE(_aidl_return); \
    } while (0)

#define AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling start()"); \
        status = audioDecoderController->start(); \
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        AUDIODECODER_GET_STATE((audioDecoder), State::STARTED); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
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

#define AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling stop()"); \
        status = audioDecoderController->stop(); \
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        AUDIODECODER_GET_STATE((audioDecoder), State::READY); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
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

#define AUDIODECODER_FLUSH_SUCCESS(audioDecoder, audioDecoderController, reset) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling flush(IN: reset[%s])", UT_Control_GetMapString(bool_mapTable, reset)); \
        status = audioDecoderController->flush(reset); \
        UT_LOG_INFO("Result flush(IN: reset[%s]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, reset), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (reset) \
            AUDIODECODER_GET_STATE((audioDecoder), State::READY); \
        else \
            AUDIODECODER_GET_STATE((audioDecoder), State::STARTED); \
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

#define AUDIODECODER_SIGNAL_DISCONTINUITY_SUCCESS(audioDecoder, audioDecoderController) \
    do { \
        ::android::binder::Status status; \
        ::com::rdk::hal::State state; \
        UT_LOG_INFO("Calling signalDiscontinuity()"); \
        status = audioDecoderController->signalDiscontinuity(); \
        UT_LOG_INFO("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        AUDIODECODER_GET_STATE((audioDecoder), State::STARTED); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
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

#define AUDIODECODER_SIGNAL_EOS_SUCCESS(audioDecoder, audioDecoderController) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling signalEOS()"); \
        status = audioDecoderController->signalEOS(); \
        UT_LOG_INFO("Result signalEOS(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        AUDIODECODER_GET_STATE((audioDecoder), State::STARTED); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
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

#define AUDIODECODER_PARSE_CSD_SUCCESS(audioDecoder, audioDecoderController, csdAudioFormat, codecData) \
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
        AUDIODECODER_GET_STATE((audioDecoder), State::STARTED); \
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

#endif //__VTS_AUDIODECODER_H__

/** @} */ // End of AUDIO_DECODER_HALTEST_HEADER
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK
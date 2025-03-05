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
 * @defgroup AUDIO_DECODER_HALTEST_L1 Audio Decoder HAL Tests L1 File
 * @{
 * @parblock
 *
 * ### L1 Test Cases for Audio Decoder HAL :
 *
 * This module includes Level 1 functional tests (success and failure scenarios).
 * This is to ensure that the API meets the operational requirements of the module across all vendors.
 *
 * **Pre-Conditions:** @n
 * **Dependencies:** None@n
 *
 * @endparblock
 *
 */

/**
 * @file vts_l1_audiodecoder.c
 *
 */

#include <ut.h>
#include <ut_kvp_profile.h>
#include <ut_log.h>
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

#include "com/rdk/hal/audiodecoder/IAudioDecoderManager.h"
#include "com/rdk/hal/audiodecoder/BnAudioDecoderControllerListener.h"
#include "com/rdk/hal/audiodecoder/BnAudioDecoderEventListener.h"

#include "vts_helper_functions.h"
#include "vts_parse_audiodecoder_hfp.h"

using android::sp;
using android::String16;
using android::defaultServiceManager;
using android::binder::Status;

using namespace com::rdk::hal;
using namespace com::rdk::hal::audiodecoder;

sp<IAudioDecoderManager> service;
//sp<IAudioDecoder> audioDecoder;
IAudioDecoder::Id g_audiodecoderId;

UT_status_t getAudioDecoderService()
{
    service = android::interface_cast<IAudioDecoderManager>(
            defaultServiceManager()->getService(String16(IAudioDecoderManager::serviceName().c_str())));
    if (service == nullptr)
    {
        UT_LOG_ERROR("AudioDecoder Service not available");
        return UT_STATUS_FAILURE;
    }
    return UT_STATUS_OK;
}

/*currently HAL FEATURE PROFILE contain string value for codec instead of enum. so the helper required to convert*/
Codec helper_StringTocodec(const std::string& format)
{
    static const std::unordered_map<std::string, Codec> formatMap = {
    {"PCM", Codec::PCM},
    {"AAC_LC", Codec::AAC_LC},
    {"HE_AAC", Codec::HE_AAC},
    {"HE_AAC2", Codec::HE_AAC2},
    {"AAC_ELD", Codec::AAC_ELD},
    {"DOLBY_AC3", Codec::DOLBY_AC3},
    {"DOLBY_AC3_PLUS", Codec::DOLBY_AC3_PLUS},
    {"DOLBY_AC3_PLUS_JOC", Codec::DOLBY_AC3_PLUS_JOC},
    {"DOLBY_AC4", Codec::DOLBY_AC4},
    {"DOLBY_MAT", Codec::DOLBY_MAT},
    {"DOLBY_MAT2", Codec::DOLBY_MAT2},
    {"DOLBY_TRUEHD", Codec::DOLBY_TRUEHD},
    {"MPEG2", Codec::MPEG2},
    {"MP3", Codec::MP3},
    {"FLAC", Codec::FLAC},
    {"VORBIS", Codec::VORBIS},
    {"DTS", Codec::DTS},
    {"OPUS", Codec::OPUS},
    {"WMA", Codec::WMA},
    {"REALAUDIO", Codec::REALAUDIO},
    {"USAC", Codec::USAC},
    {"X_HE_AAC", Codec::X_HE_AAC}
    };

    auto it = formatMap.find(format);
    if (it != formatMap.end())
    {
        return it->second;
    }
    return static_cast<Codec>(-1);
}

bool helper_isValidState(::com::rdk::hal::State state)
{
    switch(state) {
        case ::com::rdk::hal::State::UNKNOWN:
        case ::com::rdk::hal::State::CLOSED:
        case ::com::rdk::hal::State::OPENING:
        case ::com::rdk::hal::State::READY:
        case ::com::rdk::hal::State::STARTING:
        case ::com::rdk::hal::State::STARTED:
        case ::com::rdk::hal::State::FLUSHING:
        case ::com::rdk::hal::State::STOPPING:
        case ::com::rdk::hal::State::CLOSING:
            return true;
        default:
            return false;
    }
}

class AudioDecoderControllerListener : public BnAudioDecoderControllerListener
{
    public:

        AudioDecoderControllerListener() {};
        virtual ~AudioDecoderControllerListener() = default;

        Status onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::std::optional<FrameMetadata>& metadata) override {
            return ::android::binder::Status::ok();
        }
};

class AudioDecoderEventListener : public BnAudioDecoderEventListener
{
    public:
        AudioDecoderEventListener() {};
        virtual ~AudioDecoderEventListener() = default;

        Status onDecodeError(ErrorCode errorCode, int32_t vendorErrorCode) override {
            return ::android::binder::Status::ok();
        }
        Status onStateChanged(::com::rdk::hal::State oldState, ::com::rdk::hal::State newState) override {
            return ::android::binder::Status::ok();
        }
};

class VTS_L1_AudioDecoder : public UTCore
{
public:
    VTS_L1_AudioDecoder() : UTCore() {}

    ~VTS_L1_AudioDecoder() override = default;

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};


UT_ADD_TEST_TO_GROUP(VTS_L1_AudioDecoder, UT_TESTS_L1)

void audiodecoder_open_success(sp<IAudioDecoder> audioDecoder, Codec codec, bool secure,
                               sp<IAudioDecoderControllerListener> controllerListener,
                               sp<IAudioDecoderController> *audioDecoderController)
{
    ::android::binder::Status status;
    ::com::rdk::hal::State state;

    /* Open the Decoder */
    status = audioDecoder->open(codec, secure, controllerListener, audioDecoderController);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_NOT_EQUAL(audioDecoderController, nullptr);

    /* Check the state of the decoder */
    status = audioDecoder->getState(&state);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_EQUAL(state, ::com::rdk::hal::State::READY);
}

void audiodecoder_open_failure(sp<IAudioDecoder> audioDecoder, Codec codec, bool secure,
                               sp<IAudioDecoderControllerListener> controllerListener,
                               sp<IAudioDecoderController> *audioDecoderController)
{
    ::android::binder::Status status;
    ::com::rdk::hal::State state;

    /* Open the Decoder */
    status = audioDecoder->open(codec, secure, controllerListener, audioDecoderController);
    UT_ASSERT_FALSE(status.isOk());
    UT_ASSERT_EQUAL(audioDecoderController, nullptr);

}

void audiodecoder_close_success(sp<IAudioDecoder> audioDecoder,
                                sp<IAudioDecoderController> audioDecoderController,
                                bool *aidl_return)
{
    ::android::binder::Status status;
    ::com::rdk::hal::State state;

    /* close the Decoder */
    status = audioDecoder->close(audioDecoderController, aidl_return);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_TRUE(*aidl_return);

    /* Check the state of the decoder */
    status = audioDecoder->getState(&state);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_EQUAL(state, ::com::rdk::hal::State::CLOSED);
}

void audiodecoder_close_failure(sp<IAudioDecoder> audioDecoder,
                                sp<IAudioDecoderController> audioDecoderController,
                                bool *aidl_return)
{
    ::android::binder::Status status;
    ::com::rdk::hal::State state;

    /* close the Decoder */
    status = audioDecoder->close(audioDecoderController, aidl_return);
    UT_ASSERT_FALSE(status.isOk());
    UT_ASSERT_FALSE(*aidl_return);
}

void audiodecoder_registerEvent_success(sp<IAudioDecoder> audioDecoder,
                                        sp<IAudioDecoderEventListener> eventListener,
                                        bool *aidl_return)
{
    ::android::binder::Status status;

    /* Register events */
    status = audioDecoder->registerEventListener(eventListener, aidl_return);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_TRUE(*aidl_return);
}

void audiodecoder_registerEvent_failure(sp<IAudioDecoder> audioDecoder,
                                        sp<IAudioDecoderEventListener> eventListener,
                                        bool *aidl_return)
{
    ::android::binder::Status status;

    /* Register events */
    status = audioDecoder->registerEventListener(eventListener, aidl_return);
    UT_ASSERT_FALSE(status.isOk());
    UT_ASSERT_FALSE(*aidl_return);
}

void audiodecoder_unregisterEvent_success(sp<IAudioDecoder> audioDecoder,
                                          sp<IAudioDecoderEventListener> eventListener,
                                          bool *aidl_return)
{
    ::android::binder::Status status;

    /* Register events */
    status = audioDecoder->unregisterEventListener(eventListener, aidl_return);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_TRUE(*aidl_return);
}

void audiodecoder_unregisterEvent_failure(sp<IAudioDecoder> audioDecoder,
                                          sp<IAudioDecoderEventListener> eventListener,
                                          bool *aidl_return)
{
    ::android::binder::Status status;

    /* Register events */
    status = audioDecoder->unregisterEventListener(eventListener, aidl_return);
    UT_ASSERT_FALSE(status.isOk());
    UT_ASSERT_FALSE(*aidl_return);
}

void audiodecoder_start_success(sp<IAudioDecoder> audioDecoder,
                                sp<IAudioDecoderController> audioDecoderController)
{
    ::android::binder::Status status;
    ::com::rdk::hal::State state;

    /* Start the decoder */
    status = audioDecoderController->start();
    UT_ASSERT_TRUE(status.isOk());

    /* Check the state of the decoder */
    status = audioDecoder->getState(&state);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_EQUAL(state, ::com::rdk::hal::State::STARTED);
}

void audiodecoder_start_failure(sp<IAudioDecoder> audioDecoder,
                                sp<IAudioDecoderController> audioDecoderController)
{
    ::android::binder::Status status;
    ::com::rdk::hal::State state;

    /* Start the decoder */
    status = audioDecoderController->start();
    UT_ASSERT_FALSE(status.isOk());
}

void audiodecoder_stop_success(sp<IAudioDecoder> audioDecoder,
                               sp<IAudioDecoderController> audioDecoderController)
{
    ::android::binder::Status status;
    ::com::rdk::hal::State state;

    /* Start the decoder */
    status = audioDecoderController->stop();
    UT_ASSERT_TRUE(status.isOk());

    /* Check the state of the decoder */
    status = audioDecoder->getState(&state);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_EQUAL(state, ::com::rdk::hal::State::READY);
}

void audiodecoder_stop_failure(sp<IAudioDecoder> audioDecoder,
                               sp<IAudioDecoderController> audioDecoderController)
{
    ::android::binder::Status status;
    ::com::rdk::hal::State state;

    /* Start the decoder */
    status = audioDecoderController->stop();
    UT_ASSERT_FALSE(status.isOk());
}

/**
* @brief Tests the retrieval of audio decoder IDs to ensure the function fetches valid and unique IDs.
*
* This test verifies that the service's getAudioDecoderIds function properly retrieves a list of audio decoder IDs,
* ensuring they are unique and correspond to expected values defined in the HAL FEATURE PROFILE. Also verifies
* they are consistent across retrievals.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 001@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Retrieve the initial audio decoder IDs|None|Should retrieve a non-empty list of IDs|Should Pass|
* |02|Retrieve the second set of audio IDs|None|Should retrieve a non-empty list of IDs|Should Pass|
* |03|Ensure the sizes and values of both lists are equal|initialDecoderIds.size(), secondDecoderIDs.size(), initialDecoderIds[i].value, secondDecoderIds[i].value|Sizes and values should match|Should Pass|
* |04|Get the supported decoder IDs from the profile file|Profile file|Profile should contain supported decoder IDs|Should Pass|
* |05|Ensure the sizes and values of profile file and inital set should match|initialDecoderIds.size(), initialDecoderIds[i].value|Sizes and values should match|Should Pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecodermanager_getAudioDecoderIds_positive)
{
    int i = 0;
    std::vector<IAudioDecoder::Id> initialDecoderIds;
    std::vector<IAudioDecoder::Id> secondDecoderIds;
    std::vector<int> decoderId_from_profile;

    ::android::binder::Status status = service->getAudioDecoderIds(&initialDecoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(initialDecoderIds.empty());
    UT_ASSERT_TRUE(helper_areElementsUnique(initialDecoderIds));

    status = service->getAudioDecoderIds(&secondDecoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(secondDecoderIds.empty());

    UT_ASSERT_EQUAL(initialDecoderIds.size(), secondDecoderIds.size());

    for (size_t i = 0; i < initialDecoderIds.size(); ++i)
    {
        UT_ASSERT_EQUAL(initialDecoderIds[i].value, secondDecoderIds[i].value);
    }

    /* checking the number of audio decoder ids from the api getAudioDecoderIds()
       equal to the number of audio decoder ids from HAL FEATURE PROFILE  */
    UT_ASSERT_EQUAL(initialDecoderIds.size(),decoders.size());

    for(const auto& _decoder: decoders)
    {
        decoderId_from_profile.push_back(_decoder.id);
    }

    /* checking whether retrieved audio decoder ids from the api getAudioDecoderIds()
       is listed in the HAL FEATURE PROFILE */
    for(const auto& _decoderId: initialDecoderIds)
    {
        auto result = std::find(decoderId_from_profile.begin(), decoderId_from_profile.end(), _decoderId.value);
        UT_ASSERT_NOT_EQUAL(result, decoderId_from_profile.end());
    }
}

/**
* @brief Test case for verifying the positive scenario of getAudioDecoder functionality
*
* This test case checks the getAudioDecoder method of the IAudioDecoder service for all available decoder
* id and ensures that the returned decoder object is valid and the status is OK. Also verifies
* they are consistent across retrievals
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 002@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Retrieve the initial audio decoder IDs|None|Should retrieve a non-empty list of IDs|Should Pass|
* |02|Loop through each decoder in the decoder list and get deoder instance|id, audiodecoder|Status is OK and audiodecoder is not NULL|Should Pass|
* |03|Invoke getAudioDecoder again with same id; check status|id, audiodecoder|Status is OK and audiodecoder is not NULL|Should Pass|
* |04|Compare RESOURCE_ID from both invocations|resource_id_1, resource_id_2|RESOURCE_IDs are equal|Should Pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecodermanager_getAudioDecoder_positive)
{
    sp<IAudioDecoder> audiodecoder_1;
    sp<IAudioDecoder> audiodecoder_2;
    IAudioDecoder::Id id ;
    std::optional<PropertyValue> resource_id_1;
    std::optional<PropertyValue> resource_id_2;
    Property property = Property::RESOURCE_ID;

    for(const auto& _decoder: decoders)
    {
        id.value = _decoder.id;
        Status status = service->getAudioDecoder(id, &audiodecoder_1);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audiodecoder_1 != nullptr);

        status = audiodecoder_1->getProperty(property, &resource_id_1);
        UT_ASSERT_TRUE(status.isOk());

        status = service->getAudioDecoder(id, &audiodecoder_2);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audiodecoder_2 != nullptr);

        status = audiodecoder_2->getProperty(property, &resource_id_2);
        UT_ASSERT_TRUE(status.isOk());

        int32_t _resource_id_1 = resource_id_1->value->get<PropertyValue::Value::Tag::intValue>();
        int32_t _resource_id_2 = resource_id_2->value->get<PropertyValue::Value::Tag::intValue>();

        UT_ASSERT_EQUAL(_resource_id_1, _resource_id_2);
    }
}

/**
* @brief Test to verify behavior of getAudioDecoder with invalid ID
*
* This test checks if getAudioDecoder returns the appropriate error when provided with an invalid decoder ID
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 003@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Retrieve the initial audio decoder IDs|None|Should retrieve a non-empty list of IDs|Should Pass|
* |02|Get audio decoder with invalid id (min id value - 1)|(min id value - 1), audiodecoder|status.isOk() should be false and audiodecoder should be nullptr|Should Pass|
* |03|Get audio decoder with invalid id (max id value + 1)|(max id value + 1), audiodecoder|status.isOk() should be false and audiodecoder should be nullptr|Should Pass|
* |04|Get audio decoder for UNDEFINED ID|UNDEFINED, audiodecoder|status.isOk() should be false and audiodecoder should be nullptr|Should Pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecodermanager_getAudioDecoder_negative)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    std::vector<int> decoderId_from_profile;
    IAudioDecoder::Id invalidID;
    sp<IAudioDecoder> audiodecoder;

    ::android::binder::Status status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    if(decoderIds.size() > 1)
        std::sort(decoderIds.begin(), decoderIds.end());

    invalidID = decoderIds.front();
    invalidID.value -= 1;
    status = service->getAudioDecoder(invalidID, &audiodecoder);
    UT_ASSERT_FALSE(status.isOk());
    UT_ASSERT_TRUE(audiodecoder == nullptr);

    invalidID = decoderIds.back();
    invalidID.value += 1;
    status = service->getAudioDecoder(invalidID, &audiodecoder);
    UT_ASSERT_FALSE(status.isOk());
    UT_ASSERT_TRUE(audiodecoder == nullptr);

    invalidID.value = IAudioDecoder::Id::UNDEFINED;
    status = service->getAudioDecoder(invalidID, &audiodecoder);
    UT_ASSERT_FALSE(status.isOk());
    UT_ASSERT_TRUE(audiodecoder == nullptr);
}

/**
* @brief Test positive case for getting audio decoder capabilities
*
* This test verifies the successful retrieval of capabilities from the audio decoder interface.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 004@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Invoke getCapabilities() with the Capabilities instance for all supported audiodecoders|Capabilities instance|Status should be ok|Should Pass|
* |02|Validate the capabilities obtained with HFP|Capabilities instance|Successful validation of capabilities|Should Pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecoder_getCapabilities_positive)
{
    Capabilities capabilities;
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;

    ::android::binder::Status status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        audioDecoder->getCapabilities(&capabilities);
        UT_ASSERT_TRUE(status.isOk());
        helper_validateCapabilities(audioDecoder, capabilities);
    }
}

/**
* @brief Test to check the positive scenarios for retrieving various audio decoder properties.
*
* This test iterates over a list of audio decoder properties and verifies that the getProperty
* method of the audioDecoder object correctly retrieves the property values. It ensures that
* the status is OK and the property value matches the expected criteria.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 005@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Initialize audioDecoder object and iterate through the properties list for supported audio decoders|decoder, property list[RESOURCE_ID, SECURE_AUDIO, METRIC_FRAMES_DECODED, METRIC_DECODE_ERRORS, METRIC_FRAMES_DROPPED]|Status should be ok|Should Pass|
* |02|Validate the property obtained with HFP|property value|Successful validation of property|Should Pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecoder_getProperty_positive)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;

    ::android::binder::Status status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        for (auto property :
        {
            Property::RESOURCE_ID,
            Property::SECURE_AUDIO,
            Property::METRIC_FRAMES_DECODED,
            Property::METRIC_DECODE_ERRORS,
            Property::METRIC_FRAMES_DROPPED
        }) {
            std::optional<PropertyValue> propertyValue;
            status = audioDecoder->getProperty(property, &propertyValue);
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(helper_validatePropertyValue(propertyValue, property));
        }
    }
}

/**
* @brief Tests the behavior of the audioDecoder's getProperty function with an invalid property.
*
* This test validates that the getProperty function of the audioDecoder handles invalid properties
* correctly by passing a non-existent property ID.
* This ensures robustness by verifying the function’s response to unexpected or erroneous input.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 006@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Call getProperty function with invalid property value for all supported decoders|decoder, invalid property|Status should be invalid|Should Pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecoder_getProperty_negative)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    std::optional<PropertyValue> propertyValue;
    int32_t firstValue = static_cast<int32_t>(Property::RESOURCE_ID);
    Property Invalidproperty = static_cast<Property>(firstValue - 1);

    ::android::binder::Status status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        status = audioDecoder->getProperty(Invalidproperty, &propertyValue);
        UT_ASSERT_FALSE(status.isOk());
    }
}

/**
* @brief Test the audio decoder's state retrieval function.
*
* This test verifies if the getState function of the audio decoder retrieves the state correctly and ensures the state is valid.@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 007@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Invoke getState to retrieve the state of all available decoders|status.isOk()|status.isOk() should be true|Should Pass|
* |02|Validate the retrieved state|state|State should be valid|Should Pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecoder_getState_positive)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    ::com::rdk::hal::State state;
    ::android::binder::Status status;

    status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        status = audioDecoder->getState(&state);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_EQUAL(state, ::com::rdk::hal::State::CLOSED);
    }
}

/**
* @brief This test verifies the ability to open  audio decoders in various codecs.
*
* This test checks the initial state of the audio decoder, ensures there are available decoders,
* and iteratively opens the decoders for supported codecs and then closes them.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 008@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecoder_open_positive)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    Capabilities capabilities;
    ::com::rdk::hal::State state;
    ::android::binder::Status status;
    bool _aidl_return;

    status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        audioDecoder->getCapabilities(&capabilities);
        UT_ASSERT_TRUE(status.isOk());

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();

            /* Open the decoder */
            audiodecoder_open_success(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, &audioDecoderController);

            /* Close the decoder */
            audiodecoder_close_success(audioDecoder, audioDecoderController, &_aidl_return);
        }
    }
}

/**
* @brief This test case is designed to check the error handling of the audioDecoder's open function
*
* The function tests the behavior of the audioDecoder API when attempting to open an audio decoder with
* 1. Invalid Parameters
* 2. Open decoder again.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 009@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Loop throug supported audio decoders|getAudioDecoderIds|None|Should be successful|
* |02|Get audio decoder and capabilites|audioDecoder, capabilities|status should bo ok|Should be successful|
* |03|Send invalid codec to open|invalidCodec|status.isOk() = false and audioDecoderController = nullptr|Should be successful|
* |04|Try to open as secure decoding if decoder doesn't supprot secure|supportSecure=True|status.isOk() = false and audioDecoderController = nullptr|Should be successful|
* |05|send invalid listener|listener=nullptr|status.isOk() = false and audioDecoderController = nullptr|Should be successful|
* |06|Open the decoder with valid parameters|valid parameters to open|status.isOk() = true and audioDecoderController = valid pointer|Should be successful|
* |07|Open the decoder again with valid parameters|valid parameters to open|status.isOk() = false and audioDecoderController = nullptr|Should be successful|
* |08|Close the decoder|None|status.isOk() = true|Should be successful|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecoder_open_negative)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    Codec invalidCodec;
    Capabilities capabilities;
    ::com::rdk::hal::State state;
    ::android::binder::Status status;
    sp<IAudioDecoderController> audioDecoderController;
    sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();

    bool _aidl_return;

    status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        audioDecoder->getCapabilities(&capabilities);
        UT_ASSERT_TRUE(status.isOk());

        if(capabilities.supportedCodecs.size() > 1)
            std::sort(capabilities.supportedCodecs.begin(), capabilities.supportedCodecs.end());

        invalidCodec = static_cast<Codec>(static_cast<int32_t>(capabilities.supportedCodecs.front()) - 1);

        /* Send invalid audio codec value */
        audiodecoder_open_failure(audioDecoder, invalidCodec, capabilities.supportsSecure, controllerListener, &audioDecoderController);

        /* If decoder doesn't support secure, request for secure decoder */
        if (capabilities.supportsSecure == false)
        {
            audiodecoder_open_failure(audioDecoder, invalidCodec, true, controllerListener, &audioDecoderController);
        }

        /* Send invalid listener */
        audiodecoder_open_failure(audioDecoder, capabilities.supportedCodecs.front(), capabilities.supportsSecure, nullptr, &audioDecoderController);

        /* Open the decoder */
        audiodecoder_open_success(audioDecoder, capabilities.supportedCodecs.front(), capabilities.supportsSecure, controllerListener, &audioDecoderController);

        /* Open the Decoder again */
        audiodecoder_open_failure(audioDecoder, capabilities.supportedCodecs.front(), capabilities.supportsSecure, controllerListener, &audioDecoderController);

        audiodecoder_close_success(audioDecoder, audioDecoderController, &_aidl_return);
    }
}

/**
* @brief Test to verify the successful closing of an audio decoder
*
* This test ensures that the audio decoder closes correctly when it is in the READY state.
* The objective is to test the audio decoder's state transition handling and verify that the `close` API functions as expected.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 010@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecoder_close_positive)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    Capabilities capabilities;
    ::com::rdk::hal::State state;
    ::android::binder::Status status;
    bool _aidl_return;

    status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        audioDecoder->getCapabilities(&capabilities);
        UT_ASSERT_TRUE(status.isOk());

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();

            /* Open the decoder */
            audiodecoder_open_success(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, &audioDecoderController);

            /* Close the decoder */
            audiodecoder_close_success(audioDecoder, audioDecoderController, &_aidl_return);
        }
    }
}

/**
* @brief Test for closing the audio decoder in different states
*
* This test checks the behavior of the audio decoder when trying to close it
* from the READY state and then attempts to close it again from the CLOSED state.
* Also test checks the behavious when decoder is closed again@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 011@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|Close the decoder with invalid controller|audioDecoderController=nullptr|status should be not ok|Should pass|
* |05|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
* |06|Close the audio decoder again|audioDecoderController, _aidl_return|status.isOk() should be false, _aidl_return = false|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecoder_close_negative)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    Capabilities capabilities;
    ::com::rdk::hal::State state;
    ::android::binder::Status status;
    bool _aidl_return;

    status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        audioDecoder->getCapabilities(&capabilities);
        UT_ASSERT_TRUE(status.isOk());

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();

            /* Open the decoder */
            audiodecoder_open_success(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, &audioDecoderController);

            /* Close with invalid controller */
            audiodecoder_close_failure(audioDecoder, nullptr, &_aidl_return);

            /* Close the decoder */
            audiodecoder_close_success(audioDecoder, audioDecoderController, &_aidl_return);

            /* Close the decoder again*/
            audiodecoder_close_failure(audioDecoder, audioDecoderController, &_aidl_return);
        }
    }
}

/**
* @brief Test to register an audio decoder event listener and verify its successful registration
*
* This test checks if an audio decoder event listener can be registered successfully.
* It ensures that the function registering the listener returns true and completes without errors.
* It also cleans up by unregistering the listener.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 012 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|register decoder events|listener|status should be ok|Should pass|
* |03|un-register decoder events|listener|status should be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecoder_registerEventListener_positive)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    ::android::binder::Status status;
    bool _aidl_return;

    status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        /* Register events */
        audiodecoder_registerEvent_success(audioDecoder, eventListener, &_aidl_return);

        /* Un-register the events */
        audiodecoder_unregisterEvent_success(audioDecoder, eventListener, &_aidl_return);

    }
}

/**
* @brief Test the negative case scenario for registering an audio decoder event listener.
*
* This test attempts to register an audio decoder event listener that has already been registered
* in the SetUp method. It ensures that a listener cannot be registered more than once and validates
* the appropriate negative responses from the registration function. This test also checks the behaviour
* if invalid listener is passed
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 013@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|register decoder events with invalid listener|listener=nullptr|status should not be ok|Should pass|
* |03|register decoder events|listener|status should be ok|Should pass|
* |04|register decoder events again|listener|status should not be ok|Should pass|
* |05|un-register decoder events|listener|status should be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecoder_registerEventListener_negative)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    ::android::binder::Status status;
    bool _aidl_return;

    status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

        /* Register events with invalid listener */
        audiodecoder_registerEvent_failure(audioDecoder, nullptr, &_aidl_return);

        /* Register events */
        audiodecoder_registerEvent_success(audioDecoder, eventListener, &_aidl_return);

        /* Register events again */
        audiodecoder_registerEvent_failure(audioDecoder, eventListener, &_aidl_return);

        /* Un-register the events */
        audiodecoder_unregisterEvent_success(audioDecoder, eventListener, &_aidl_return);
    }
}

/**
* @brief Test to register an audio decoder event listener and verify its successful registration and un-registration
*
* This test checks if an audio decoder event listener can be registered successfully.
* It ensures that the function registering the listener returns true and completes without errors.
* It also cleans up by unregistering the listener.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 014 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|register decoder events|listener|status should be ok|Should pass|
* |03|un-register decoder events|listener|status should be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecoder_unregisterEventListener_positive)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    ::android::binder::Status status;
    bool _aidl_return;

    status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        /* Register events */
        audiodecoder_registerEvent_success(audioDecoder, eventListener, &_aidl_return);

        /* Un-register the events */
        audiodecoder_unregisterEvent_success(audioDecoder, eventListener, &_aidl_return);

    }
}

/**
* @brief Test to check the negative scenarios of unregister event listener
*
* This test verifies the behavior of the audio decoder when trying to unregister an event listener
* that was never registered. Also verifies with invalid parameters.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 015 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|un-register decoder events without register|listener|status should be ok|Should pass|
* |03|register decoder events|listener|status should be ok|Should pass|
* |04|un-register decoder events with nullptr|listener=nullptr|status should not be ok|Should pass|
* |05|un-register decoder events|listener|status should be ok|Should pass|
* |06|un-register decoder events again|listener|status should not be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecoder_unregisterEventListener_negative)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    ::android::binder::Status status;
    bool _aidl_return;

    status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        /* Un-register the events */
        audiodecoder_unregisterEvent_failure(audioDecoder, eventListener, &_aidl_return);

        /* Register events */
        audiodecoder_registerEvent_success(audioDecoder, eventListener, &_aidl_return);

        /* Un-register the events with nullptr*/
        audiodecoder_unregisterEvent_failure(audioDecoder, nullptr, &_aidl_return);

        /* Un-register the events */
        audiodecoder_unregisterEvent_success(audioDecoder, eventListener, &_aidl_return);

        /* Un-register the events again */
        audiodecoder_unregisterEvent_failure(audioDecoder, eventListener, &_aidl_return);
    }
}

/**
* @brief Test the positive start functionality of the audio decoder.
*
* This test verifies that the audio decoder can be successfully started and stopped,
* ensuring that the start method works as expected without error.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 016@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|start the decoder|audioDecoderController|status should be ok|Should pass|
* |05|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |06|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |07|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecodercontroller_start_positive)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    audiodecoder::Capabilities capabilities;
    ::com::rdk::hal::State state;
    ::android::binder::Status status;
    bool _aidl_return;

    status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        audioDecoder->getCapabilities(&capabilities);
        UT_ASSERT_TRUE(status.isOk());

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
            sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            /* Open the Decoder */
            audiodecoder_open_success(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, &audioDecoderController);

            /* Start the decoder */
            audiodecoder_start_success(audioDecoder, audioDecoderController);

            /* Stop the decoder */
            audiodecoder_stop_success(audioDecoder, audioDecoderController);

            /* Close the decoder */
            audiodecoder_close_success(audioDecoder, audioDecoderController, &_aidl_return);
        }
    }
}

/**
* @brief Test for negative scenario when starting the audio decoder that is already started
*
* This test checks the behavior of the audio decoder when trying to start it again
* after it has already been started. The expected behavior is to throw an illegal state exception.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 015@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|start the decoder|audioDecoderController|status should be ok|Should pass|
* |05|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |06|start the decoder again|None|status should not be ok|Should pass|
* |07|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |08|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
* |09|start the decoder again|None|status should not be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecodercontroller_start_negative)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    Capabilities capabilities;
    ::com::rdk::hal::State state;
    ::android::binder::Status status;
    bool _aidl_return;

    status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        audioDecoder->getCapabilities(&capabilities);
        UT_ASSERT_TRUE(status.isOk());

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
            sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            /* Open the decoder */
            audiodecoder_open_success(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, &audioDecoderController);

            /* Start the decoder */
            audiodecoder_start_success(audioDecoder, audioDecoderController);

            /* Start the decoder again */
            audiodecoder_start_failure(audioDecoder, audioDecoderController);

            /* Stop the decoder */
            audiodecoder_stop_success(audioDecoder, audioDecoderController);

            /* Close the decoder */
            audiodecoder_close_success(audioDecoder, audioDecoderController, &_aidl_return);

            /* Start the decoder again */
            audiodecoder_start_failure(audioDecoder, audioDecoderController);
        }
    }
}

/** @} */ // End of AUDIO_DECODER_HALTEST L1 File
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER Module
/** @} */ // End of HPK

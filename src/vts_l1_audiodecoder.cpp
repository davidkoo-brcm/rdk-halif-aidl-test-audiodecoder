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
sp<IAudioDecoder> audioDecoder;
::com::rdk::hal::audiodecoder::IAudioDecoder::Id g_audiodecoderId;

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
com::rdk::hal::audiodecoder::Codec helper_StringTocodec(const std::string& format)
{
    static const std::unordered_map<std::string, com::rdk::hal::audiodecoder::Codec> formatMap = {
    {"PCM", com::rdk::hal::audiodecoder::Codec::PCM},
    {"AAC_LC", com::rdk::hal::audiodecoder::Codec::AAC_LC},
    {"HE_AAC", com::rdk::hal::audiodecoder::Codec::HE_AAC},
    {"HE_AAC2", com::rdk::hal::audiodecoder::Codec::HE_AAC2},
    {"AAC_ELD", com::rdk::hal::audiodecoder::Codec::AAC_ELD},
    {"DOLBY_AC3", com::rdk::hal::audiodecoder::Codec::DOLBY_AC3},
    {"DOLBY_AC3_PLUS", com::rdk::hal::audiodecoder::Codec::DOLBY_AC3_PLUS},
    {"DOLBY_AC3_PLUS_JOC", com::rdk::hal::audiodecoder::Codec::DOLBY_AC3_PLUS_JOC},
    {"DOLBY_AC4", com::rdk::hal::audiodecoder::Codec::DOLBY_AC4},
    {"DOLBY_MAT", com::rdk::hal::audiodecoder::Codec::DOLBY_MAT},
    {"DOLBY_MAT2", com::rdk::hal::audiodecoder::Codec::DOLBY_MAT2},
    {"DOLBY_TRUEHD", com::rdk::hal::audiodecoder::Codec::DOLBY_TRUEHD},
    {"MPEG2", com::rdk::hal::audiodecoder::Codec::MPEG2},
    {"MP3", com::rdk::hal::audiodecoder::Codec::MP3},
    {"FLAC", com::rdk::hal::audiodecoder::Codec::FLAC},
    {"VORBIS", com::rdk::hal::audiodecoder::Codec::VORBIS},
    {"DTS", com::rdk::hal::audiodecoder::Codec::DTS},
    {"OPUS", com::rdk::hal::audiodecoder::Codec::OPUS},
    {"WMA", com::rdk::hal::audiodecoder::Codec::WMA},
    {"REALAUDIO", com::rdk::hal::audiodecoder::Codec::REALAUDIO},
    {"USAC", com::rdk::hal::audiodecoder::Codec::USAC},
    {"X_HE_AAC", com::rdk::hal::audiodecoder::Codec::X_HE_AAC}
    };

    auto it = formatMap.find(format);
    if (it != formatMap.end())
    {
        return it->second;
    }
    return static_cast<com::rdk::hal::audiodecoder::Codec>(-1);
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

        Status onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::std::optional<::com::rdk::hal::audiodecoder::FrameMetadata>& metadata) override {
            return ::android::binder::Status::ok();
        }
};

class AudioDecoderEventListener : public BnAudioDecoderEventListener
{
    public:
        AudioDecoderEventListener() {};
        virtual ~AudioDecoderEventListener() = default;

        Status onDecodeError(::com::rdk::hal::audiodecoder::ErrorCode errorCode, int32_t vendorErrorCode) override {
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
UT_ADD_TEST(VTS_L1_AudioDecoder, audiomanager_getAudioDecoderIds_positive)
{
    int i = 0;
    std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> initialDecoderIds;
    std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> secondDecoderIds;
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
UT_ADD_TEST(VTS_L1_AudioDecoder, audiomanager_getAudioDecoder_positive)
{
    ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder> audiodecoder_1;
    ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder> audiodecoder_2;
    ::com::rdk::hal::audiodecoder::IAudioDecoder::Id id ;
    std::optional<::com::rdk::hal::PropertyValue> resource_id_1;
    std::optional<::com::rdk::hal::PropertyValue> resource_id_2;
    ::com::rdk::hal::audiodecoder::Property property = ::com::rdk::hal::audiodecoder::Property::RESOURCE_ID;

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

        int32_t _resource_id_1 = resource_id_1->value->get<::com::rdk::hal::PropertyValue::Value::Tag::intValue>();
        int32_t _resource_id_2 = resource_id_2->value->get<::com::rdk::hal::PropertyValue::Value::Tag::intValue>();

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
UT_ADD_TEST(VTS_L1_AudioDecoder, audiomanager_getAudioDecoder_negative)
{
    std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> decoderIds;
    std::vector<int> decoderId_from_profile;
    ::com::rdk::hal::audiodecoder::IAudioDecoder::Id invalidID;
    ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder> audiodecoder;

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

    invalidID.value = ::com::rdk::hal::audiodecoder::IAudioDecoder::Id::UNDEFINED;
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
    ::com::rdk::hal::audiodecoder::Capabilities capabilities;
    std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> decoderIds;

    ::android::binder::Status status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_FALSE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        audioDecoder->getCapabilities(&capabilities);
        UT_ASSERT_TRUE(status.isOk());
        helper_validateCapabilities(capabilities);
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
    std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> decoderIds;

    ::android::binder::Status status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_FALSE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        for (auto property :
        {
            ::com::rdk::hal::audiodecoder::Property::RESOURCE_ID,
            ::com::rdk::hal::audiodecoder::Property::SECURE_AUDIO,
            ::com::rdk::hal::audiodecoder::Property::METRIC_FRAMES_DECODED,
            ::com::rdk::hal::audiodecoder::Property::METRIC_DECODE_ERRORS,
            ::com::rdk::hal::audiodecoder::Property::METRIC_FRAMES_DROPPED
        }) {
            std::optional<::com::rdk::hal::PropertyValue> propertyValue;
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
    std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> decoderIds;
    std::optional<::com::rdk::hal::PropertyValue> propertyValue;
    int32_t firstValue = static_cast<int32_t>(::com::rdk::hal::audiodecoder::Property::RESOURCE_ID);
    ::com::rdk::hal::audiodecoder::Property Invalidproperty = static_cast<::com::rdk::hal::audiodecoder::Property>(firstValue - 1);

    ::android::binder::Status status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_FALSE(status.isOk());
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
    std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> decoderIds;
    ::com::rdk::hal::State state;
    ::android::binder::Status status;

    status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_FALSE(status.isOk());
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
    std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> decoderIds;
    ::com::rdk::hal::audiodecoder::Capabilities capabilities;
    ::com::rdk::hal::State state;
    ::android::binder::Status status;
    bool _aidl_return;

    status = service->getAudioDecoderIds(&decoderIds);
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(decoderIds.empty());

    for(const auto& _decoderId: decoderIds)
    {
        status = service->getAudioDecoder(_decoderId, &audioDecoder);
        UT_ASSERT_FALSE(status.isOk());
        UT_ASSERT_TRUE(audioDecoder == nullptr);

        audioDecoder->getCapabilities(&capabilities);
        UT_ASSERT_TRUE(status.isOk());

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> listener = new AudioDecoderControllerListener();

            status = audioDecoder->open(_codec, capabilities.supportsSecure, listener, &audioDecoderController);
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_NOT_EQUAL(audioDecoderController, nullptr);

            status = audioDecoder->getState(&state);
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_EQUAL(state, ::com::rdk::hal::State::READY);

            status = audioDecoder->close(audioDecoderController, &_aidl_return);
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(_aidl_return);

            status = audioDecoder->getState(&state);
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_EQUAL(state, ::com::rdk::hal::State::CLOSED);
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
* | 01| Check initial state of audioDecoder| state | state = CLOSED | Should be successful |
* | 02| Confirm decoders are available| decoders.size() | decoders.size() > 0 | Should be successful |
* | 03| Attempt to open audioDecoder with invalid codec| codec = -1, supportsSecure = decoders[0].supportsSecure, listener = new AudioDecoderControllerListener(), audioDecoderController = nullptr | status.isOk() = false | Should Fail |
*/
////////TBD
UT_ADD_TEST(VTS_L1_AudioDecoder, audiodecoder_open_negative)
{

}

/** @} */ // End of AUDIO_DECODER_HALTEST L1 File
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER Module
/** @} */ // End of HPK

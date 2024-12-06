/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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
* @file AudioDecoderClient.cpp
* @page AudioDecoder hal Level 1 Tests
*
* ## Module's Role
* This module includes Level 1 functional tests (success and failure scenarios).
* This is to ensure that the AudioDecoder hal APIs meet the requirements across all vendors.
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
#include "yaml_utils.h"

#include "com/rdk/hal/audiodecoder/IAudioDecoderManager.h"
#include "com/rdk/hal/audiodecoder/BnAudioDecoderControllerListener.h"
#include "com/rdk/hal/audiodecoder/BnAudioDecoderEventListener.h"

using android::sp;
using android::String16;
using android::defaultServiceManager;
using android::binder::Status;

using namespace com::rdk::hal;
using namespace com::rdk::hal::audiodecoder;

sp<IAudioDecoderManager> service;
sp<IAudioDecoder> audioDecoder;
::com::rdk::hal::audiodecoder::IAudioDecoder::Id g_audiodecoderId;

struct decoder
{
    int id;
    ::std::vector<::com::rdk::hal::audiodecoder::Codec> supportedCodecs; 
    bool supportsSecure;
};

std::vector<decoder> decoders;


int getService()
{
    service = android::interface_cast<IAudioDecoderManager>(
            defaultServiceManager()->getService(String16(IAudioDecoderManager::serviceName().c_str())));
    if (service == nullptr)
    {
        printf("Service not available\n");
        return -1;
    }
    return 0;
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

bool helper_isValidState(::com::rdk::hal::State state) {
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

void helper_validateCapabilities(const ::com::rdk::hal::audiodecoder::Capabilities& capabilities) 
{
    bool decoderID = false;
    int _resource_id;
    ::com::rdk::hal::audiodecoder::Property property = ::com::rdk::hal::audiodecoder::Property::RESOURCE_ID;
    std::optional<::com::rdk::hal::PropertyValue> resource_id;

    ::android::binder::Status status = audioDecoder->getProperty(property, &resource_id);
    _resource_id = resource_id->value->get<::com::rdk::hal::PropertyValue::Value::Tag::intValue>();
    /*
     "decoders" is a vector, which contains information of audio decoders available in hal feature profile.
     The following are the information for each audio decoder:
     - The decoder ID.
     - Whether the decoder supports secure audio path (SAP) mode. 
     - A list of supported codecs.
   */
    for(const auto& _decoder: decoders)
    {
        if(_decoder.id == _resource_id)
        {
            decoderID = true;
            /* number of supported codec from Hal Feature Profile should be equal to  number of supported codec from getCapabilities() api */
            EXPECT_EQ(_decoder.supportedCodecs.size(),capabilities.supportedCodecs.size());
            /* iterating over each supported codec from getCapabilities() api */
            for(const auto& supportedcodec_from_api: capabilities.supportedCodecs)
            {
                auto result = std::find(_decoder.supportedCodecs.begin(),_decoder.supportedCodecs.end(),supportedcodec_from_api);
                EXPECT_NE(result, _decoder.supportedCodecs.end()) <<"decoder : "<<static_cast<int>(_decoder.id) <<" contain supportedCodec : "<<static_cast<int>(supportedcodec_from_api) <<" which is not matching with the profile"<<std::endl;
            }
            /* ensure that supportsSecure value from  hal feature profile  is equal to the value from getCapabilities() api */
            EXPECT_EQ(_decoder.supportsSecure, capabilities.supportsSecure);
        }
    }
    ASSERT_TRUE(decoderID) <<"Audio decoder id : "<<_resource_id <<" is not in profile";
}


bool helper_areElementsUnique(const std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id>& decoderIds) {
    std::unordered_set<int> uniqueElements;
    
    for (const auto& _decoderId : decoderIds) 
    {
        if(_decoderId.value < 0)
        {
            std::cout <<"find Invalid decoder ID : " <<_decoderId.value <<std::endl;
            return false;
        } 
        if (uniqueElements.find(_decoderId.value) != uniqueElements.end()) 
        {
            std::cout<<"decoder ID is not unique : " <<_decoderId.value <<std::endl;
            return false;
        }
        uniqueElements.insert(_decoderId.value);
    }
    return true;
}

bool helper_validatePropertyValue( const std::optional<::com::rdk::hal::PropertyValue>& propertyValue, const ::com::rdk::hal::audiodecoder::Property property)
{
    if (!propertyValue.has_value() || !propertyValue->value.has_value())
    {
        return false;
    }

    std::variant<int32_t, std::string> val;
    if (propertyValue->value.has_value())
    {
        const auto& propVal = propertyValue->value.value();
        switch (propVal.getTag())
        {
            case PropertyValue::Value::Tag::intValue:
                val = propVal.get<PropertyValue::Value::Tag::intValue>();
                break;
            case PropertyValue::Value::Tag::stringValue:
            {
                android::String16 string16Val = propVal.get<PropertyValue::Value::Tag::stringValue>();
                android::String8 string8Val(string16Val);  // Convert String16 to String8
                val = std::string(string8Val.string());    // Convert String8 to std::string
            }
                break;
            case PropertyValue::Value::Tag::booleanValue:
                val = propVal.get<PropertyValue::Value::Tag::booleanValue>() ? 1 : 0;  // Store as int32_t (1 for true, 0 for false)
                break;

            default:
                std::cerr << "Unsupported type in PropertyValue::Value" << std::endl;
                break;
        }
    }

    switch (property)
    {
        case ::com::rdk::hal::audiodecoder::Property::RESOURCE_ID:
             if(std::holds_alternative<int>(val) && std::get<int>(val) == g_audiodecoderId.value)
             {
                return true;
             }
             else
             {
                return false;
             }

        case ::com::rdk::hal::audiodecoder::Property::LOW_LATENCY_MODE:
            return std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1);

        case ::com::rdk::hal::audiodecoder::Property::AV_SOURCE:
            return std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) == static_cast<int32_t>(::com::rdk::hal::AVSource::AUTO) ||
                    std::get<int32_t>(val) == static_cast<int32_t>(::com::rdk::hal::AVSource::UNKNOWN) ||
                    (std::get<int32_t>(val) >= static_cast<int32_t>(::com::rdk::hal::AVSource::IP) &&
                     std::get<int32_t>(val) <= static_cast<int32_t>(::com::rdk::hal::AVSource::SYSTEM)) ||
                    (std::get<int32_t>(val) >= static_cast<int32_t>(::com::rdk::hal::AVSource::HDMI_1) &&
                     std::get<int32_t>(val) <= static_cast<int32_t>(::com::rdk::hal::AVSource::HDMI_5)) ||
                    (std::get<int32_t>(val) >= static_cast<int32_t>(::com::rdk::hal::AVSource::COMPOSITE_1) &&
                     std::get<int32_t>(val) <= static_cast<int32_t>(::com::rdk::hal::AVSource::COMPOSITE_5)));

        case ::com::rdk::hal::audiodecoder::Property::SECURE_AUDIO:
            return std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1);

        case ::com::rdk::hal::audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= -1;

        case ::com::rdk::hal::audiodecoder::Property::AC4_PREFERRED_LANG1:
        case ::com::rdk::hal::audiodecoder::Property::AC4_PREFERRED_LANG2:
            return std::holds_alternative<std::string>(val) &&
                   (std::get<std::string>(val).empty() || std::get<std::string>(val).size() == 3);

        case ::com::rdk::hal::audiodecoder::Property::AC4_ASSOCIATED_TYPE:
            return std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) >= -1 && std::get<int32_t>(val) <= 2);

        case ::com::rdk::hal::audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY:
            return std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) >= -1 && std::get<int32_t>(val) <= 1);

        case ::com::rdk::hal::audiodecoder::Property::AC4_MIXER_BALANCE:
            return std::holds_alternative<int32_t>(val) &&
                   ((std::get<int32_t>(val) >= -32 && std::get<int32_t>(val) <= 32) || std::get<int32_t>(val) == 255);

        case ::com::rdk::hal::audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE:
            return std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) >= -1 && std::get<int32_t>(val) <= 1);

        default:
            return true;
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

class L1AudioDecoderOpenTest : public virtual UTCore
{
    protected:
        void SetUp() override
        {
            //prerequisite - checking state
            ::com::rdk::hal::State state;
            ::android::binder::Status status = audioDecoder->getState(&state);
            ASSERT_TRUE(status.isOk());
            ASSERT_EQ(state, ::com::rdk::hal::State::CLOSED);
            ASSERT_TRUE(decoders.size()) <<"No decoders available";

            sp<IAudioDecoderControllerListener> listener = new AudioDecoderControllerListener();
            status = audioDecoder->open(decoders[0].supportedCodecs[0], decoders[0].supportsSecure, listener, &audioDecoderController);
            ASSERT_TRUE(status.isOk()) << "Failed to open audio decoder, status: " << status.toString8().string();
            ASSERT_NE(audioDecoderController, nullptr) << "audioDecoderController is null after open()";
        }
        void TearDown() override
        {
            ::com::rdk::hal::State state;
            Status status = audioDecoder->getState(&state);
            ASSERT_TRUE(status.isOk());
            ASSERT_EQ(state, ::com::rdk::hal::State::READY);
            bool _aidl_return;
            status = audioDecoder->close(audioDecoderController, &_aidl_return);
        }
        android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderController> audioDecoderController;
};

class L1AudioDecoderCloseTest : public L1AudioDecoderOpenTest
{
    protected:
        void SetUp() override
        {
            L1AudioDecoderOpenTest::SetUp();
        }
};

class L1AudioDecoderEventRegisterTest : public UTCore
{
    protected:
        void SetUp() override
        {
            listener = new AudioDecoderEventListener();
            bool _aidl_return;
            android::binder::Status status = audioDecoder->registerEventListener(listener, &_aidl_return);
            ASSERT_TRUE(status.isOk());
            ASSERT_EQ(_aidl_return , true);
        }
        android::sp<com::rdk::hal::audiodecoder::IAudioDecoderEventListener> listener;
};

class L1AudioDecoderStartTest : public L1AudioDecoderOpenTest
{
    protected:
        void SetUp() override
        {
            L1AudioDecoderOpenTest::SetUp();
            Status status = audioDecoderController->start();
            ASSERT_TRUE(status.isOk());
        }
        void TearDown() override
        {
            L1AudioDecoderOpenTest::TearDown();
        }
};

/**
* @brief Tests the retrieval of audio decoder IDs to ensure the function fetches valid and unique IDs.
*
* This test verifies that the service's getAudioDecoderIds function properly retrieves a list of audio decoder IDs,
* ensuring they are unique and correspond to expected values defined in the HAL FEATURE PROFILE.
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
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01     | Create an empty vector for decoder IDs named decoderIds | decoderIds = {} | Should be successful | |
* | 02     | Create an empty vector for decoder IDs from the profile named decoderId_from_profile | decoderId_from_profile = {} | Should be successful | |
* | 03     | Call getAudioDecoderIds function to fetch decoder IDs into decoderIds vector | service->getAudioDecoderIds(&decoderIds) | Status returned should be OK | Should Pass |
* | 04     | Assert the status is OK | status.isOk() | True | Should Pass |
* | 05     | Assert the decoderIds vector is not empty | decoderIds.empty() | False | Should Pass |
* | 06     | Assert that all elements in decoderIds are unique using helper function | areElementsUnique(decoderIds) | True | Should Pass |
* | 07     | Assert the count of decoder IDs matches the  count of decoder IDs in profile | decoderIds.size() == decoders.size() | True | Should Pass |
* | 08     | Populate decoderId_from_profile with IDs from the decoders vector(hal feature profile) | | Should be successful | |
* | 09     | For each decoder ID from the getAudioDecoderIds api , ensure it exists in decoderId_from_profile | decoderId_from_profile, decoderIds | Each decoder ID should be found | Should Pass |
*/

TEST(L1AudioDecoderManagerTest, test_l1_audiodecoder_positive_1_getAudioDecoderIds)
{
    int i=0;
    std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> decoderIds;
    std::vector<int> decoderId_from_profile;

    ::android::binder::Status status = service->getAudioDecoderIds(&decoderIds);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(decoderIds.empty());
    ASSERT_TRUE(helper_areElementsUnique(decoderIds));
    /* checking the number of audio decoder ids from the api getAudioDecoderIds() equal to the number of audio decoder ids from HAL FEATURE PROFILE  */
    ASSERT_EQ(decoderIds.size(),decoders.size());

    for(const auto& _decoder: decoders)
    {
        decoderId_from_profile.push_back(_decoder.id);
    }

    /* checking whether retrieved audio decoder ids from the api getAudioDecoderIds() is listed in the HAL FEATURE PROFILE */
    for(const auto& _decoderId: decoderIds)
    {   
        auto result = std::find(decoderId_from_profile.begin(), decoderId_from_profile.end(), _decoderId.value);
        EXPECT_NE(result, decoderId_from_profile.end());
    }
}

/**
* @brief This test ensures that the audio decoder IDs can be successfully retrieved twice and they are consistent across retrievals.
*
* This test performs two calls to retrieve audio decoder IDs and verifies that both calls return consistent and non-empty results.
* It ensures that the internal state of the audio decoder IDs does not change between calls and that the service is able 
* to return the necessary IDs correctly.
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
* | Variation / Step | Description                             | Test Data                 | Expected Result                             | Notes                        |
* | :----:           | ---------                               | ----------                | --------------                              | -----                        |
* | 01               | Retrieve the initial audio decoder IDs. | None                         | Should retrieve a non-empty list of IDs.   | Should Pass                  |
* | 02               | Check if status is OK for initial IDs.  | status.isOk() | status.isOk() should be true.              | Should be successful         |
* | 03               | Verify initial decoder IDs is not empty.| initialDecoderIDs         | initialDecoderIDs.empty() should be false. | Should be successful         |
* | 04               | Retrieve the second set of audio IDs.   | None                         | Should retrieve a non-empty list of IDs.   | Should Pass                  |
* | 05               | Check if status is OK for second IDs.   | status.isOk() | status.isOk() should be true.              | Should be successful         |
* | 06               | Verify second decoder IDs is not empty. | secondDecoderIds          | secondDecoderIDs.empty() should be false. | Should be successful         |
* | 07               | Ensure the sizes of both lists are equal.| initialDecoderIds.size(), secondDecoderIDs.size() | Sizes should match.                        | Should be successful         |
* | 08               | Verify each ID in initial and second lists are equal. | initialDecoderIds[i].value, secondDecoderIds[i].value | Values should match for all elements.      | Should be successful         |
*/
TEST(L1AudioDecoderManagerTest, test_l1_audiodecoder_positive_2_getAudioDecoderIds)
{
    std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> initialDecoderIds;
    std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> secondDecoderIds;

    ::android::binder::Status status = service->getAudioDecoderIds(&initialDecoderIds);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(initialDecoderIds.empty());


    status = service->getAudioDecoderIds(&secondDecoderIds);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(secondDecoderIds.empty());

    ASSERT_EQ(initialDecoderIds.size(), secondDecoderIds.size());
    for (size_t i = 0; i < initialDecoderIds.size(); ++i)
    {
        ASSERT_EQ(initialDecoderIds[i].value, secondDecoderIds[i].value);
    }
}

/**
* @brief Test case for verifying the positive scenario of getAudioDecoder functionality
*
* This test case checks the getAudioDecoder method of the IAudioDecoder service for all available decoder id in the profile and ensures that the returned decoder object is valid and the status is OK.
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
* | Variation / Step | Description                                         | Test Data                    | Expected Result                   | Notes                      |
* |:----------------:|-----------------------------------------------------|-----------------------------|-----------------------------------|----------------------------|
* | 01               | Initialize the audiodecoder and id variables        | audiodecoder = NULL, id     | audiodecoder and id initialized   | Should be successful       |
* | 02               | Loop through each decoder in the decoder list (from the profile)      | decoders list               | Loop starts                       | Should be successful       |
* | 03               | Set decoder id to the each decoder's id from the profile          | id.value = _decoder.id      | id.value is set                   | Should be successful       |
* | 04               | Call getAudioDecoder with id and audiodecoder       | id, audiodecoder            | Status is OK and audiodecoder is not NULL | Should Pass               |
* | 05               | Verify the status and audiodecoder object is not NULL | status.isOk(), audiodecoder != NULL | status.isOk() is true and audiodecoder valid | Should Pass               |
*/
TEST(L1AudioDecoderManagerTest, test_l1_audiodecoder_positive_1_getAudioDecoder)
{
    ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder> audiodecoder;
    ::com::rdk::hal::audiodecoder::IAudioDecoder::Id id ;

    for(const auto& _decoder: decoders)
    {
        id.value = _decoder.id;
        Status status = service->getAudioDecoder(id, &audiodecoder);
        EXPECT_TRUE(status.isOk());
        EXPECT_TRUE(audiodecoder != nullptr);
    }
}

/**
* @brief Test to verify the retrieval of the same IAudioDecoder interface instance across multiple invocations.
*
* This test checks if the getAudioDecoder method from the service returns the same instance of IAudioDecoder when called multiple times with the same Id.
* The test ensures that the resource ID retrieved from the audio decoder interface remains consistent across multiple invocations.
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
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :--------------: | ----------- | --------- | --------------- | ----- |
* | 01               | Initialize audio decoder pointer and ids. | None | None | Should be successful |
* | 02               | Iterate over each decoder and set id value. | decoders | None | Should be successful |
* | 03               | Invoke getAudioDecoder with id; check status. | id.value = _decoder.id | Status.isOK() should be true, audiodecoder != nullptr | Should Pass |
* | 04               | Get the property RESOURCE_ID from audiodecoder. | property = RESOURCE_ID | Status.isOK() should be true | Should Pass |
* | 05               | Invoke getAudioDecoder again with same id; check status. | id.value = _decoder.id | Status.isOK() should be true, audiodecoder != nullptr | Should Pass |
* | 06               | Get the property RESOURCE_ID from audiodecoder again. | property = RESOURCE_ID | Status.isOK() should be true | Should Pass |
* | 07               | Compare RESOURCE_ID from both invocations. | resource_id_1, resource_id_2 | RESOURCE_IDs are equal | Should Pass |
*/
TEST(L1AudioDecoderManagerTest, test_l1_audiodecoder_positive_2_getAudioDecoder)
{
    ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder> audiodecoder;
    ::com::rdk::hal::audiodecoder::IAudioDecoder::Id id ;
    ::com::rdk::hal::audiodecoder::Property property = ::com::rdk::hal::audiodecoder::Property::RESOURCE_ID;
    std::optional<::com::rdk::hal::PropertyValue> resource_id_1;
    std::optional<::com::rdk::hal::PropertyValue> resource_id_2;

    for(const auto& _decoder: decoders)
    {
        id.value = _decoder.id;
        Status status = service->getAudioDecoder(id, &audiodecoder);
        EXPECT_TRUE(status.isOk());
        EXPECT_TRUE(audiodecoder != nullptr);

        status = audioDecoder->getProperty(property, &resource_id_1);
        EXPECT_TRUE(status.isOk());

        status = service->getAudioDecoder(id, &audiodecoder);
        EXPECT_TRUE(status.isOk());
        EXPECT_TRUE(audiodecoder != nullptr);

        status = audioDecoder->getProperty(property, &resource_id_2);
        EXPECT_TRUE(status.isOk());

       int32_t _resource_id_1 = resource_id_1->value->get<::com::rdk::hal::PropertyValue::Value::Tag::intValue>();
       int32_t _resource_id_2 = resource_id_2->value->get<::com::rdk::hal::PropertyValue::Value::Tag::intValue>();

       /* validating whether multiple invocation gives same audioDecoder interface instance by checking RESOURCE_ID */
       EXPECT_EQ(_resource_id_1, _resource_id_2)<<"GOT DIFFERENT INSTANCE OF AUDIODECODER DURING MULTIPLE INVOCATION. ID for first invocation : "<<_resource_id_1 <<" ID for second invocation : "<<_resource_id_2 <<std::endl;
    }
}

/**
* @brief Test case for validating the positive scenario of getAudioDecoder method in IAudioDecoder.
*
* This test verifies that the getAudioDecoder method successfully retrieves a valid audio decoder given a 
* specific ID (here it's UNDEFINED). This test ensures that the function under test works correctly and 
* returns the expected outputs.
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Initialize variables and prepare method inputs. | id.value = UNDEFINED | Variables initialized successfully | Should be successful |
* | 02| Call getAudioDecoder method with prepared inputs. | id.value = UNDEFINED, audiodecoder = nullptr | Status.isOk() should be true, audiodecoder should not be null | Should Pass |
* | 03| Verify the returned status. | status.isOk() = true | Status should be OK | Should Pass |
* | 04| Verify the audiodecoder object is not null. | audiodecoder != nullptr | audiodecoder should not be null | Should Pass |
*/
TEST(L1AudioDecoderManagerTest, test_l1_audiodecoder_positive_3_getAudioDecoder)
{
    ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder> audiodecoder;
    ::com::rdk::hal::audiodecoder::IAudioDecoder::Id id ;
    id.value = ::com::rdk::hal::audiodecoder::IAudioDecoder::Id::UNDEFINED;

    Status status = service->getAudioDecoder(id, &audiodecoder);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(audiodecoder != nullptr);
}

/**
* @brief Test to verify behavior of getAudioDecoder with invalid ID
*
* This test checks if getAudioDecoder returns the appropriate error when provided with an invalid decoder ID
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Create an IAudioDecoder object pointer | None | Object pointer is created | Should be successful |
* | 02 | Initialize ID with invalid value -2 | id.value = -2 | ID is initialized with -2 | Should be successful |
* | 03 | Call getAudioDecoder with invalid ID | id = -2, audiodecoder = nullptr | status.isOk() should be false and audiodecoder should be nullptr | Should Fail |
* | 04 | Check ASSERT_FALSE for status.isOk() | status.isOk() | status.isOk() == false | Should Pass |
* | 05 | Check EXPECT_TRUE for audiodecoder being nullptr | audiodecoder == nullptr | Expectation passes | Should Pass |
*/
TEST(L1AudioDecoderManagerTest, test_l1_audiodecoder_negative_getAudioDecoder)
{
    ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder> audiodecoder;
    ::com::rdk::hal::audiodecoder::IAudioDecoder::Id id ;
    id.value = -2;
    Status status = service->getAudioDecoder(id, &audiodecoder);
    ASSERT_FALSE(status.isOk());
    EXPECT_TRUE(audiodecoder == nullptr);
}

/**
* @brief Test positive case for getting audio decoder capabilities
*
* This test verifies the successful retrieval of capabilities from the audio decoder interface.
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
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Create an instance of Capabilities object | - | - | Should be successful |
* | 02| Invoke getCapabilities() with the Capabilities instance | Capabilities instance | Status object with successful status | Should Pass |
* | 03| Assert that the status is OK | status.isOk() | Assertion that status.isOk() returns true | Should Pass |
* | 04| Validate the capabilities obtained | Capabilities instance | Successful validation of capabilities | Should be successful |
*/
TEST(L1AudioDecoderTest, test_l1_audiodecoder_positive_1_getCapabilities)
{
    ::com::rdk::hal::audiodecoder::Capabilities capabilities;
    ::android::binder::Status status = audioDecoder->getCapabilities(&capabilities);
    ASSERT_TRUE(status.isOk());
    helper_validateCapabilities(capabilities);
}

/**
* @brief Test to check the positive scenarios for retrieving various audio decoder properties.
*
* This test iterates over a list of audio decoder properties and verifies that the getProperty
* method of the audioDecoder object correctly retrieves the property values. It ensures that
* the status is OK and the property value matches the expected criteria.
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
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Initialize audioDecoder object and iterate through the properties list | None | None | Should be successful |
* | 02 | Call getProperty function for Property::RESOURCE_ID | property = Property::RESOURCE_ID, propertyValue = std::optional | status.isOk() should be true, validatePropertyValue should pass | Should Pass |
* | 03 | Call getProperty function for Property::LOW_LATENCY_MODE | property = Property::LOW_LATENCY_MODE, propertyValue = std::optional | status.isOk() should be true, validatePropertyValue should pass | Should Pass |
* | 04 | Call getProperty function for Property::AV_SOURCE | property = Property::AV_SOURCE, propertyValue = std::optional | status.isOk() should be true, validatePropertyValue should pass | Should Pass |
* | 05 | Call getProperty function for Property::SECURE_AUDIO | property = Property::SECURE_AUDIO, propertyValue = std::optional | status.isOk() should be true, validatePropertyValue should pass | Should Pass |
* | 06 | Call getProperty function for Property::AC4_PRESENTATION_GROUP_INDEX | property = Property::AC4_PRESENTATION_GROUP_INDEX, propertyValue = std::optional | status.isOk() should be true, validatePropertyValue should pass | Should Pass |
* | 07 | Call getProperty function for Property::AC4_PREFERRED_LANG1 | property = Property::AC4_PREFERRED_LANG1, propertyValue = std::optional | status.isOk() should be true, validatePropertyValue should pass | Should Pass |
* | 08 | Call getProperty function for Property::AC4_PREFERRED_LANG2 | property = Property::AC4_PREFERRED_LANG2, propertyValue = std::optional | status.isOk() should be true, validatePropertyValue should pass | Should Pass |
* | 09 | Call getProperty function for Property::AC4_ASSOCIATED_TYPE | property = Property::AC4_ASSOCIATED_TYPE, propertyValue = std::optional | status.isOk() should be true, validatePropertyValue should pass | Should Pass |
* | 10 | Call getProperty function for Property::AC4_AUTO_SELECTION_PRIORITY | property = Property::AC4_AUTO_SELECTION_PRIORITY, propertyValue = std::optional | status.isOk() should be true, validatePropertyValue should pass | Should Pass |
* | 11 | Call getProperty function for Property::AC4_MIXER_BALANCE | property = Property::AC4_MIXER_BALANCE, propertyValue = std::optional | status.isOk() should be true, validatePropertyValue should pass | Should Pass |
* | 12 | Call getProperty function for Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE | property = Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, propertyValue = std::optional | status.isOk() should be true, validatePropertyValue should pass | Should Pass |
*/
TEST(L1AudioDecoderTest, test_l1_audiodecoder_positive_1_getProperty)
{
    for (auto property :
    {
        ::com::rdk::hal::audiodecoder::Property::RESOURCE_ID,
        ::com::rdk::hal::audiodecoder::Property::LOW_LATENCY_MODE,
        ::com::rdk::hal::audiodecoder::Property::AV_SOURCE,
        ::com::rdk::hal::audiodecoder::Property::SECURE_AUDIO,
        ::com::rdk::hal::audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX,
        ::com::rdk::hal::audiodecoder::Property::AC4_PREFERRED_LANG1,
        ::com::rdk::hal::audiodecoder::Property::AC4_PREFERRED_LANG2,
        ::com::rdk::hal::audiodecoder::Property::AC4_ASSOCIATED_TYPE,
        ::com::rdk::hal::audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY,
        ::com::rdk::hal::audiodecoder::Property::AC4_MIXER_BALANCE,
        ::com::rdk::hal::audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE
    }) {
    std::optional<::com::rdk::hal::PropertyValue> propertyValue;
    Status status = audioDecoder->getProperty(property, &propertyValue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: " << static_cast<int32_t>(property);
    EXPECT_TRUE(helper_validatePropertyValue(propertyValue, property))
    << "Property value validation failed for property: " << static_cast<int32_t>(property);
    }
}

/**
* @brief Tests the behavior of the audioDecoder's getProperty function with an invalid property.
*
* This test validates that the getProperty function of the audioDecoder handles invalid properties correctly by passing a non-existent property ID (9999).
* This ensures robustness by verifying the function’s response to unexpected or erroneous input.
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
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Declare an optional variable of type PropertyValue for capturing the property value. | propertyValue (optional<::com::rdk::hal::PropertyValue>) = std::nullopt | Should be successful |  |
* | 02| Call getProperty function with invalid property value. | input: Property = 9999, output: propertyValue = std::nullopt | Status should be invalid, and propertyValue should remain unset | Should Fail |
* | 03| Check if status is not OK and report accordingly using EXPECT_FALSE. | status.isOk() | status.isOk() = false | Should Pass |
*/
TEST(L1AudioDecoderTest, test_l1_audiodecoder_negative_1_getProperty)
{
    std::optional<::com::rdk::hal::PropertyValue> propertyValue;
    Status status = audioDecoder->getProperty(static_cast<::com::rdk::hal::audiodecoder::Property>(9999), &propertyValue);
    EXPECT_FALSE(status.isOk());
}

/**
* @brief Tests the positive scenario for setting property in AudioDecoderController.
*
* This test ensures that the property 'LOW_LATENCY_MODE' can be correctly set to a new value and then reverted back to its initial value.
*
* **Test Group ID:** Basic: 01  @n
* **Test Case ID:** 010  @n
* **Priority:** High  @n
* @n
* **Pre-Conditions:** None  @n
* **Dependencies:** None  @n
* **User Interaction:** None  @n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | -------------- | ---------- | -------------- | ----- |
* | 01 | Store initial value of property 'LOW_LATENCY_MODE'. | property=LOW_LATENCY_MODE, initialvalue=std::optional<PropertyValue> | status.isOk() should be true | Should be successful |
* | 02 | Set new value for property 'LOW_LATENCY_MODE'. | property=LOW_LATENCY_MODE, propertyValue=intValue(1), _aidl_return=bool | status.isOk() should be true and _aidl_return should be true | Should Pass |
* | 03 | Revert property 'LOW_LATENCY_MODE' to initial value. | property=LOW_LATENCY_MODE, initialvalue=std::optional<PropertyValue>, _aidl_return=bool | status.isOk() should be true | Should Pass |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_1_setProperty)
{
    Property property = Property::LOW_LATENCY_MODE;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);
    bool _aidl_return;

    /* storing initial value */
    Status status = audioDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: LOW_LATENCY_MODE";

    status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    /*clean-up setting back initial value*/
    status = audioDecoderController->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : LOW_LATENCY_MODE";
}

/**
* @brief Tests setting the LOW_LATENCY_MODE property of the audio decoder controller.
*
* This test verifies that the audio decoder controller  is capable of setting the LOW_LATENCY_MODE property correctly
* and restoring it to its initial value. This is crucial for validating the property manipulation functionalities 
* of the decoder.
*
* **Test Group ID:** Basic: 01  @n
* **Test Case ID:** 011  @n
* **Priority:** High  @n
*  @n
* **Pre-Conditions:** None  @n
* **Dependencies:** None  @n
* **User Interaction:** None  @n
*
* **Test Procedure:**
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Get the initial value of the LOW_LATENCY_MODE property using audioDecoder->getProperty() | property = Property::LOW_LATENCY_MODE | status.isOk() should be true, initialvalue should be set | Should be successful |
* | 02 | Set the LOW_LATENCY_MODE property to a new value using audioDecoderController->setProperty() | property = Property::LOW_LATENCY_MODE, propertyValue.value = 0 | status.isOk() should be true, _aidl_return should be true | Should Pass |
* | 03 | Revert the LOW_LATENCY_MODE property to its initial value using audioDecoderController->setProperty() | property = Property::LOW_LATENCY_MODE, initialvalue = initialvalue.value() | status.isOk() should be true | Should Pass |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_2_setProperty)
{
    Property property = Property::LOW_LATENCY_MODE;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(0);
    bool _aidl_return;

    /* storing initial value */
    Status status = audioDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: LOW_LATENCY_MODE";

    status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    /*clean-up setting back initial value*/
    status = audioDecoderController->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : LOW_LATENCY_MODE";
}

/**
* @brief This test function tests the ability of the audio decoder controller to set various AV source properties positively.
*
* This test ensures that the `audioDecoderController` can handle different AV source inputs by setting and reverting each property. It verifies the correct behavior of `setProperty` API under diverse inputs.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 012 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Fetch initial property value | property = Property::AV_SOURCE | Status.isOK() should be true | Should be successful |
* | 02 | Set AV source to AUTO | property = AV_SOURCE, propertyValue = static_cast<int32_t>(::com::rdk::hal::AVSource::AUTO), propertyValue = int32_t value | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 03 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
* | 04 | Set AV source to UNKNOWN | property = AV_SOURCE, propertyValue = static_cast<int32_t>(::com::rdk::hal::AVSource::UNKNOWN), propertyValue = int32_t value| Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 05 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
* | 06 | Set AV source to IP | property = AV_SOURCE, propertyValue = static_cast<int32_t>(::com::rdk::hal::AVSource::IP), propertyValue = int32_t value | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 07 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
* | 08 | Set AV source to TUNER | property = AV_SOURCE, propertyValue = static_cast<int32_t>(::com::rdk::hal::AVSource::TUNER), propertyValue = int32_t value | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 09 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
* | 10 | Set AV source to SYSTEM | property = AV_SOURCE,propertyValue = static_cast<int32_t>(::com::rdk::hal::AVSource::SYSTEM), propertyValue = int32_t value | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 11 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
* | 12 | Set AV source to HDMI_1 | property = AV_SOURCE, propertyValue = static_cast<int32_t>(::com::rdk::hal::AVSource::HDMI_1), propertyValue = int32_t value | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 13 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
* | 14 | Set AV source to HDMI_2 | property = AV_SOURCE, propertyValue = static_cast<int32_t>(::com::rdk::hal::AVSource::HDMI_2), propertyValue = int32_t value | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 15 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
* | 16 | Set AV source to HDMI_3 | property = AV_SOURCE, propertyValue = static_cast<int32_t>(::com::rdk::hal::AVSource::HDMI_3), propertyValue = int32_t value | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 17 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
* | 18 | Set AV source to HDMI_4 | property = AV_SOURCE, propertyValue = static_cast<int32_t>(::com::rdk::hal::AVSource::HDMI_4), propertyValue = int32_t value | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 19 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
* | 20 | Set AV source to HDMI_5 | property = AV_SOURCE, propertyValue = static_cast<int32_t>(::com::rdk::hal::AVSource::HDMI_5), propertyValue = int32_t value | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 21 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
* | 22 | Set AV source to COMPOSITE_1 | property = AV_SOURCE,propertyValue = static_cast<int32_t>(::com::rdk::hal::AVSource::COMPOSITE_1), propertyValue = int32_t value | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 23 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
* | 24 | Set AV source to COMPOSITE_2 | property = AV_SOURCE, propertyValue = static_cast<int32_t>(::com::rdk::hal::AVSource::COMPOSITE_2), propertyValue = int32_t value | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 25 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
* | 26 | Set AV source to COMPOSITE_3 | property = AV_SOURCE, propertyValuee = static_cast<int32_t>(::com::rdk::hal::AVSource::COMPOSITE_3), propertyValue = int32_t value | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 27 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
* | 28 | Set AV source to COMPOSITE_4 | property = AV_SOURCE, propertyValue = static_cast<int32_t>(::com::rdk::hal::AVSource::COMPOSITE_4), propertyValue = int32_t value | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 29 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
* | 30 | Set AV source to COMPOSITE_5 | property = AV_SOURCE, propertyValue = static_cast<int32_t>(::com::rdk::hal::AVSource::COMPOSITE_5), propertyValue = int32_t value | Status.isOK() should be true, _aidl_return = true | Should Pass |
* | 31 | Revert to initial property value | property = Property::AV_SOURCE, initialvalue = previously fetched initial value | Status.isOK() should be true | Should successfully revert |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_3_setProperty) {
    std::optional<PropertyValue> initialvalue;    
    Property property = Property::AV_SOURCE;
    for (auto avSource :
    {
        ::com::rdk::hal::AVSource::AUTO,
        ::com::rdk::hal::AVSource::UNKNOWN,
        ::com::rdk::hal::AVSource::IP,
        ::com::rdk::hal::AVSource::TUNER,
        ::com::rdk::hal::AVSource::SYSTEM,
        ::com::rdk::hal::AVSource::HDMI_1,
        ::com::rdk::hal::AVSource::HDMI_2,
        ::com::rdk::hal::AVSource::HDMI_3,
        ::com::rdk::hal::AVSource::HDMI_4,
        ::com::rdk::hal::AVSource::HDMI_5,
        ::com::rdk::hal::AVSource::COMPOSITE_1,
        ::com::rdk::hal::AVSource::COMPOSITE_2,
        ::com::rdk::hal::AVSource::COMPOSITE_3,
        ::com::rdk::hal::AVSource::COMPOSITE_4,
        ::com::rdk::hal::AVSource::COMPOSITE_5
    }) {
        PropertyValue propertyValue;
        propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(int32_t(static_cast<int32_t>(avSource)));
        bool _aidl_return;

        /* storing initial value */
        Status status = audioDecoder->getProperty(property, &initialvalue);
        ASSERT_TRUE(status.isOk()) << "getProperty failed for property: AV_SOURCE";

        status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);

        /*clean-up setting back initial value*/
        status = audioDecoderController->setProperty(property, initialvalue.value(), &_aidl_return);
        ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : AV_SOURCE";
    }
}

/**
* @brief Test to verify the positive scenario of setting a property in L1 Audio Decoder Controller.
*
* This test verifies that a property in the L1 Audio Decoder controller can be successfully set to a new value and then reverted back to its initial value. The property tested is AC4_PRESENTATION_GROUP_INDEX.
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
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Retrieve the initial value of the property AC4_PRESENTATION_GROUP_INDEX | property = Property::AC4_PRESENTATION_GROUP_INDEX | status.isOk() should be true, initialvalue should contain the current property value | Should be successful |
* | 02 | Set the property AC4_PRESENTATION_GROUP_INDEX to a new value 0 | property = Property::AC4_PRESENTATION_GROUP_INDEX, propertyValue = 0 | status.isOk() should be true, _aidl_return should be true | Should Pass |
* | 03 | Revert the property AC4_PRESENTATION_GROUP_INDEX back to its initial value | property = Property::AC4_PRESENTATION_GROUP_INDEX, initialvalue | status.isOk() should be true, _aidl_return should be true | Should Pass |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_4_setProperty)
{
    Property property = Property::AC4_PRESENTATION_GROUP_INDEX;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;  
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(0);
    bool _aidl_return;

    /* storing initial value */
    Status status = audioDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: AC4_PRESENTATION_GROUP_INDEX";

    status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    /*clean-up setting back initial value*/
    status = audioDecoderController->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : AC4_PRESENTATION_GROUP_INDEX";
}

/**
* @brief Test to verify setting a property in L1 audio decoder is successful.
*
* This test aims to validate if the `setProperty` method of the L1 audio decoder works correctly for a positive scenario where the property is successfully set and then reverted to its initial value.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 014@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Initialize variables for property and property value | property = Property::AC4_PRESENTATION_GROUP_INDEX, propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(-1) | Variables should be initialized | Should be successful |
* | 02 | Store the initial property value using `getProperty` method | property, initialvalue | status.isOk() should be true | Should be successful |
* | 03 | Set the new property value using `setProperty` method | property, propertyValue | status.isOk() and _aidl_return should be true | Should pass |
* | 04 | Revert to initial property value using `setProperty` method | property, initialvalue | status.isOk() should be true | Should be successful |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_5_setProperty)
{
    Property property = Property::AC4_PRESENTATION_GROUP_INDEX;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue; 
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(-1);
    bool _aidl_return;
    /* storing initial value */
    Status status = audioDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: AC4_PRESENTATION_GROUP_INDEX";

    status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    /*clean-up setting back initial value*/
    status = audioDecoderController->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : AC4_PRESENTATION_GROUP_INDEX";
}

/**
* @brief Test setting and reverting a property in L1 Audio Decoder Controller.
*
* This test verifies the functionality of setting a specific property (`AC4_PREFERRED_LANG1`) in the L1 Audio Decoder Controller and then reverting it back to its initial value to ensure that the set and revert operations work correctly.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 015 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Get the initial value of property `AC4_PREFERRED_LANG1`. | property = AC4_PREFERRED_LANG1, initialvalue | status.isOk() should be true | Should be successful |
* | 02 | Set the property `AC4_PREFERRED_LANG1` to "eng". | property = AC4_PREFERRED_LANG1, propertyValue = "eng", _aidl_return | status.isOk() should be true | Should Pass |
* | 03 | Revert the property `AC4_PREFERRED_LANG1` to its initial value. | property = AC4_PREFERRED_LANG1, initialvalue, _aidl_return | status.isOk() should be true | Should Pass |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_6_setProperty)
{
    Property property = Property::AC4_PREFERRED_LANG1;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::stringValue>(u"eng");
    bool _aidl_return;

    /* storing initial value */
    Status status = audioDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: AC4_PREFERRED_LANG1";

    status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    /*clean-up setting back initial value*/
    status = audioDecoderController->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : AC4_PREFERRED_LANG1";
}

/**
* @brief Test the ability to set a property value for an audio decoder controller
*
* This test verifies the proper functionality of setting a property value on an audio decoder controller object.
* Specifically, it tests the setter function to ensure an appropriate property can be set and reverted back.@n
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
* | Variation / Step | Description                              | Test Data                                 | Expected Result                                           | Notes                         |
* | :--------------: | ---------------------------------------- | ----------------------------------------- | ---------------------------------------------------------- | ----------------------------- |
* | 01               | Set up initial value by getProperty      | property = AC4_PREFERRED_LANG1, &initialvalue |status.isOk() should be true, initialvalue = valid value expected         | Should be successful          |
* | 02               | Set property to new value                | property = AC4_PREFERRED_LANG1, propertyValue, &_aidl_return | status.isOk() should be true, _aidl_return = true                         | Should Pass                   |
* | 03               | Verify set property was successful       | status.isOk(),  _aidl_return              | status.isOk() should be true, _aidl_return = true            | Should Pass                   |
* | 04               | Revert property back to initial value    | property = AC4_PREFERRED_LANG1, initialvalue.value(), &_aidl_return | status = true                                              | Should be successful          |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_7_setProperty)
{
    Property property = Property::AC4_PREFERRED_LANG1;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::stringValue>(u"");
    bool _aidl_return;

    /* storing initial value */
    Status status = audioDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: AC4_PREFERRED_LANG1";

    status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    /*clean-up setting back initial value*/
    status = audioDecoderController->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : AC4_PREFERRED_LANG1";
}

/**
* @brief Test to verify the setting of property for audio decoder controller.
*
* This test verifies if the audio decoder controller can successfully set a property and ensure it is updated correctly. It first retrieves an initial value for the property, sets a new value, and then checks if the value was successfully applied. Finally, it reverts the property to its initial state to ensure clean-up.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 017@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Retrieve initial value of the property | property = Property::AC4_PREFERRED_LANG2 | status.isOk() should be true, initialvalue has the current property value | Should be successful |
* | 02 | Set the property to a new value | property = Property::AC4_PREFERRED_LANG2, propertyValue.value = u"spa" | status.isOk() should be true, _aidl_return is true | Should Pass |
* | 03 | Revert to initial value |property = Property::AC4_PREFERRED_LANG2, initialvalue.value() | status.isOk() should be true | Should be successful |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_8_setProperty)
{
    Property property = Property::AC4_PREFERRED_LANG2;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::stringValue>(u"spa");
    bool _aidl_return;

    /* storing initial value */
    Status status = audioDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: AC4_PREFERRED_LANG2";

    status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    /*clean-up setting back initial value*/
    status = audioDecoderController->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : AC4_PREFERRED_LANG2";
}

/**
* @brief Test the setting and reverting of a property on the audio decoder cotroller
*
* This test aims to verify the functionality of setting a specific property (`AC4_PREFERRED_LANG2`) on the L1 audio decoder controller and ensure that it can revert to the initial value. It tests both the ability to set the property and to clean up by restoring the initial value.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 018@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :--------------: | ----------- | --------- | --------------  | ----- |
* | 01               | Retrieve initial value of property 'AC4_PREFERRED_LANG2' | property = AC4_PREFERRED_LANG2 | Status is OK | Should be successful |
* | 02               | Set the property 'AC4_PREFERRED_LANG2' to an empty string | property = AC4_PREFERRED_LANG2, propertyValue.value = "" | Status is OK, _aidl_return is true | Should Pass |
* | 03               | Revert the property 'AC4_PREFERRED_LANG2' to its initial value | property = AC4_PREFERRED_LANG2, propertyValue = initialvalue.value() | Status is OK | Should be successful |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_9_setProperty)
{
    Property property = Property::AC4_PREFERRED_LANG2;
    PropertyValue propertyValue;
    std::optional<PropertyValue> initialvalue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::stringValue>(u"");
    bool _aidl_return;

    /* storing initial value */
    Status status = audioDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: AC4_PREFERRED_LANG2";

    status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);

    /*clean-up setting back initial value*/
    status = audioDecoderController->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : AC4_PREFERRED_LANG2";
}

/**
* @brief Tests the functionality of setting a property value in the Audio Decoder Controller for AC4_ASSOCIATED_TYPE.
*
* This test aims to verify that the Audio Decoder Controller correctly sets the property value for different integer inputs and reverts to
* the initial value without error.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 019@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Initialize and retrieve the initial value of the property | property = Property::AC4_ASSOCIATED_TYPE, initialvalue = std::nullopt | status.isOk() == true | Should be successful|
* | 02| Iterate over each input value and set property accordingly | input = -1 | status.isOk() == true, _aidl_return == true | Should Pass |
* | 03| Iterate over each input value and set property accordingly | input = 0 | status.isOk() == true, _aidl_return == true | Should Pass |
* | 04| Iterate over each input value and set property accordingly | input = 1 | status.isOk() == true, _aidl_return == true | Should Pass |
* | 05| Iterate over each input value and set property accordingly | input = 2 | status.isOk() == true, _aidl_return == true | Should Pass |
* | 06| Clean-up and revert to the initial property value | initialvalue | status.isOk() == true | Should be successful |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_10_setProperty)
{
    Property property = Property::AC4_ASSOCIATED_TYPE;
    std::optional<PropertyValue> initialvalue;
    std::vector<int32_t> inputs = { -1, 0, 1, 2 };
    bool _aidl_return;

    /* storing initial value */
    Status status = audioDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: AC4_ASSOCIATED_TYPE";

    for (const auto& input : inputs)
    {
        PropertyValue propertyValue;
        propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(input);

        status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }

    /*clean-up setting back initial value*/
    status = audioDecoderController->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : AC4_ASSOCIATED_TYPE";
}

/**
* @brief Test for setting audio decoder controller property (AC4_AUTO_SELECTION_PRIORITY) to various integer values
*
* This test verifies that the audio decoder controller property can be set to various integer values including -1, 0, and 1, and successfully reverts back to the initial value@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 020@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Store the initial value of the property by calling the getProperty function | property = Property::AC4_AUTO_SELECTION_PRIORITY | status.isOk() should be true | Should be successful |
* | 02| Set the property to -1 using the setProperty function | property = Property::AC4_AUTO_SELECTION_PRIORITY, propertyValue = -1 | status.isOk() should be true, _aidl_return should be true | Should Pass |
* | 03| Set the property to 0 using the setProperty function | property = Property::AC4_AUTO_SELECTION_PRIORITY, propertyValue = 0 | status.isOk() should be true, _aidl_return should be true | Should Pass |
* | 04| Set the property to 1 using the setProperty function | property = Property::AC4_AUTO_SELECTION_PRIORITY, propertyValue = 1 | status.isOk() should be true, _aidl_return should be true | Should Pass |
* | 05| Revert the property to its initial value using the setProperty function | property = Property::AC4_AUTO_SELECTION_PRIORITY, propertyValue = initialvalue | status.isOk() should be true | Should be successful |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_11_setProperty)
{
    Property property = Property::AC4_AUTO_SELECTION_PRIORITY;
    std::optional<PropertyValue> initialvalue;
    std::vector<int32_t> inputs = { -1, 0, 1 };
    bool _aidl_return;

    /* storing initial value */
    Status status = audioDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: AC4_AUTO_SELECTION_PRIORITY";

    for (const auto& input : inputs)
    {
        PropertyValue propertyValue;
        propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(input);

        status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }

    /*clean-up setting back initial value*/
    status = audioDecoderController->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : AC4_AUTO_SELECTION_PRIORITY";    
}

/**
* @brief Tests the positive scenario of setting the property AC4_MIXER_BALANCE on the L1 Audio Decoder.
*
* This test verifies that the `setProperty` function of the L1 Audio Decoder sets the property values correctly and
* reverts to the initial value without errors. It checks for successful status codes and valid return values for various integer inputs.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 021@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Fetch the initial value of the property `AC4_MIXER_BALANCE` | property = Property::AC4_MIXER_BALANCE | status.isOk() and `getProperty` should succeed | Should be successful |
* | 02 | Set `AC4_MIXER_BALANCE` to 255 using `setProperty` | property = Property::AC4_MIXER_BALANCE, value = 255 | status.isOk() and `_aidl_return` should be true | Should Pass |
* | 03 | Set `AC4_MIXER_BALANCE` to -32 using `setProperty` | property = Property::AC4_MIXER_BALANCE, value = -32 | status.isOk() and `_aidl_return` should be true | Should Pass |
* | 04 | Set `AC4_MIXER_BALANCE` to 32 using `setProperty` | property = Property::AC4_MIXER_BALANCE, value = 32 | status.isOk() and `_aidl_return` should be true | Should Pass |
* | 05 | Revert `AC4_MIXER_BALANCE` back to its initial value | property = Property::AC4_MIXER_BALANCE, value = initialvalue | Status should be OK and `setProperty` should succeed | Should be successful |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_12_setProperty)
{
    std::optional<PropertyValue> initialvalue;
    Property property = Property::AC4_MIXER_BALANCE;
    std::vector<int32_t> inputs = { 255, -32, 32 };
    bool _aidl_return;
    /* storing initial value */
    Status status = audioDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: AC4_MIXER_BALANCE";

    for (const auto& input : inputs)
    {
        PropertyValue propertyValue;
        propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(input);

        status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }

    /*clean-up setting back initial value*/
    status = audioDecoderController->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : AC4_MIXER_BALANCE";  
}

/**
* @brief Tests the setProperty function of L1AudioDecoder positively with various inputs.
*
* This test verifies that the setProperty function properly sets the values for the AC4_ASSOCIATED_AUDIO_MIXING_ENABLE property with
*  valid input values, and that it can successfully revert to its initial value.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 022@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
*
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | ------------------------------- | ---------- | -------------- | ----- |
* | 01     | retrieve initial value for property | property = Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, initialvalue = std::optional<PropertyValue> | status.isOk() should return true and initialvalue set | Done by Pre-requisite SetUp function |
| 02     | Attempt to set property to -1 | property = Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, input = -1 | status.isOk() should return true, _aidl_return should be true | Should Pass |
| 03     | Attempt to set property to 0 | property = Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, input = 0 | status.isOk() should return true, _aidl_return should be true | Should Pass |
| 04     | Attempt to set property to 1 | property = Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, input = 1 | status.isOk() should return true, _aidl_return should be true | Should Pass |
| 05     | Revert property to initial value | property = Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, initialvalue = initialvalue.value() | status.isOk() should return true, _aidl_return should be true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_13_setProperty)
{
    std::optional<PropertyValue> initialvalue;
    Property property = Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE;
    std::vector<int32_t> inputs = { -1, 0, 1 };
    bool _aidl_return;

    /* storing initial value */
    Status status = audioDecoder->getProperty(property, &initialvalue);
    ASSERT_TRUE(status.isOk()) << "getProperty failed for property: AC4_ASSOCIATED_AUDIO_MIXING_ENABLE";

    for (const auto& input : inputs) 
    {
        PropertyValue propertyValue;
        propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(input);

        status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }

    /*clean-up setting back initial value*/
    status = audioDecoderController->setProperty(property, initialvalue.value(), &_aidl_return);
    ASSERT_TRUE(status.isOk()) << "Failed to revert value for property : AC4_ASSOCIATED_AUDIO_MIXING_ENABLE";  
}

/**
* @brief Test to handle invalid property setting in L1AudioDecoder
*
* This test aims to verify the behavior of the audio decoder when an invalid property is set. 
* Specifically, it tests the decoder's response to an out-of-bound Property enum value.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 023@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description                             | Test Data                                    | Expected Result | Notes                    |
* | :--------------: | --------------------------------------- | -------------------------------------------- | --------------- | ------------------------ |
* |        01        | Initialize Property to an invalid value | property = static_cast<Property>(9999)       | Should be successful            |      |
* |        02        | Create PropertyValue instance           | propertyValue.value = 12345 | Should be successful            |      |
* |        03        | Call setProperty with invalid Property  | property = 9999, propertyValue = 12345 | status.isOk() should be true            | Should fail              |
* |        04        | Assert status is  OK                    | ASSERT_TRUE(status.isOk())                  | status.isOk() should be true            | Should Pass              |
* |        05        | Assert _aidl_return is False            | ASSERT_FALSE(_aidl_return)                   |_aidl_return should be false            | Should Pass              |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_1_setProperty)
{
    Property property = static_cast<Property>(9999);
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(12345);
    bool _aidl_return;
    android::binder::Status status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

/**
* @brief Test the behavior of setProperty method in Audio Decoder when negative values are passed.
*
* This test aims to verify that the setProperty function correctly handles inappropriate negative inputs and returns failure status.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 024@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Initialize Property with incorrect associated type value | property = Property::AC4_ASSOCIATED_TYPE | Should be successful  |  |
* | 02| Create PropertyValue instance | propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(3)| Should be successful |  |
* | 03 | Call setProperty on audioDecoder |property = Property::AC4_ASSOCIATED_TYPE, propertyValue.value = 3 | status.isOk() = true, _aidl_return = false | Should Fail |
* | 04 | Assert that status is  OK | status.isOk() |status.isOk() = true  | Should pass |
* | 05 | Assert that _aidl_return is false | _aidl_return |_aidl_return = false | Should pass |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_2_setProperty)
{
    Property property = Property::AC4_ASSOCIATED_TYPE;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(3);
    bool _aidl_return;
    android::binder::Status status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

/**
* @brief Test case for negative scenario of setting property in L1AudioDecoder
*
* This test checks whether setting the property AC4_AUTO_SELECTION_PRIORITY with a specific value fails as expected in the AudioDecoderController. The purpose is to validate the error handling and correctness of the setProperty function under negative conditions.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 025@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Initialize the Property variable to AC4_AUTO_SELECTION_PRIORITY | property = Property::AC4_AUTO_SELECTION_PRIORITY | Property should be set to AC4_AUTO_SELECTION_PRIORITY | Should be successful |
* | 02| Initialize the PropertyValue variable with intValue set to 2 | propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(2) | PropertyValue should be set with intValue 2 | Should be successful |
* | 03| Call setProperty on audioDecoder with initialized Property and PropertyValue | property, propertyValue, &_aidl_return | status.isOk() should true| Should fail |
* | 04| Check that status.isOk() returns true | status.isOk() | status.isOk() should be true | Should Pass |
* | 05| Check that _aidl_return is false | _aidl_return | _aidl_return should be false | Should Pass |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_3_setProperty)
{
    Property property = Property::AC4_AUTO_SELECTION_PRIORITY;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(2);
    bool _aidl_return;
    android::binder::Status status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

/**
* @brief This test validates the setProperty function handles invalid property values correctly.
*
* This test checks the behavior of the setProperty function of the L1 Audio Decoder Controller when provided with an
* invalid property value.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 026@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Instantiate the Property object with Property::AC4_MIXER_BALANCE | property = Property::AC4_MIXER_BALANCE | Should be successful | |
* | 02 | Instantiate PropertyValue and set its intValue to 33 | propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(33) | Should be successful | |
* | 03 | Invoke setProperty with the above Property and PropertyValue | property = AC4_MIXER_BALANCE, propertyValue = 33 | status.isOk() should be true, _aidl_return = false | Should Fail |
* | 04 | Assert the status is  OK | status.isOk() | status.isOk() Should be true | should pass |
* | 05 | Assert the return value _aidl_return is false | _aidl_return | _aidl_return Should be false | Should pass |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_4_setProperty)
{
    Property property = Property::AC4_MIXER_BALANCE;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(33);
    bool _aidl_return;
    android::binder::Status status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

/**
* @brief Test for setting an invalid propertyvalue in audio decoder
*
* This test checks the behavior when trying to set an invalid propertyvalue
* on the audio decoder. It ensures that the property setting returns
* an error and that the method properly handles invalid inputs.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 027 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Set the Property object to AC4_ASSOCIATED_AUDIO_MIXING_ENABLE | Property = AC4_ASSOCIATED_AUDIO_MIXING_ENABLE | Property object should be set successfully | Should be successful |
* | 02| Assign the value 2 to propertyValue using PropertyValue::Value::make | PropertyValue.value = 2 | PropertyValue object should be set with value 2 | Should be successful |
* | 03| Call the setProperty function with the valid property and invalid propertyValue | property = AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, propertyValue = 2, _aidl_return = false | setProperty should fail, returning false and status should  OK | Should Fail |
* | 04| Assert that the status is OK | status.isOk() | status.isOk() should be true | Should pass |
* | 05| Assert that _aidl_return is false | _aidl_return | _aidl_return should be false | Should pass |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_5_setProperty)
{
    Property property = Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(2);
    bool _aidl_return;
    android::binder::Status status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

/**
* @brief Test the negative scenario for the setProperty function of L1AudioDecoder
*
* This test checks if the setProperty function of the L1AudioDecoder handles invalid input correctly, identifying and responding to an illegal value for the AC4_PRESENTATION_GROUP_INDEX property field@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 028@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description                  | Test Data                                      | Expected Result        | Notes             |
* | :--------------: | ---------------------------- | ---------------------------------------------- | ---------------------- | ----------------- |
* | 01               | Initialize Property object   | property = Property::AC4_PRESENTATION_GROUP_INDEX | Should be successful   | Should be successful |
* | 02               | Create PropertyValue object and set its value to an invalid value | propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(-2) | Should be successful | Should be successful |
* | 03               | Call setProperty function of audioDecoder object | property = Property::AC4_PRESENTATION_GROUP_INDEX, propertyValue, &_aidl_return = false | status.isOk() should be true, _aidl_return should be false | Should Fail            |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_6_setProperty)
{
    Property property = Property::AC4_PRESENTATION_GROUP_INDEX;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(-2);
    bool _aidl_return;
    android::binder::Status status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

/**
* @brief This test verifies the response of the audio decoder controller when setting an invalid property.
*
* This test checks the behavior of the audio decoder controller when trying to set RESOURCE_ID . It ensures that the controller returns a failure status and does not accept the property change.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 029@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Initialize the property to an unsupported value | property = Property::RESOURCE_ID | Property is initialized to RESOURCE_ID | Should be successful |
* | 02| Set the propertyValue with an invalid integer value | propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1) | propertyValue initialized with intValue = 1 | Should be successful |
* | 03| Invoke the setProperty method on the audio decoder with the invalid property and value | property = RESOURCE_ID, propertyValue = 1, _aidl_return | status.isOk() should be true , _aidl_return should be false | Should Fail |
* | 04| Assert that the status returned by setProperty is  OK | status.isOk() | status.isOk() = true | Assertion should pass | Should Pass |
* | 05| Assert that the _aidl_return is false indicating the property set action was not successful | _aidl_return | _aidl_return = false | Assertion should pass | Should Pass |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_7_setProperty)
{
    Property property = Property::RESOURCE_ID;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);
    bool _aidl_return;
    android::binder::Status status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

/**
* @brief Test the audio decoder's state retrieval function.
*
* This test verifies if the getState function of the audio decoder retrieves the state correctly and ensures the state is valid.@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 030@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Invoke getState to retrieve the decoder state | status.isOk() | status.isOk() should be true | Should Pass |
* | 02 | Validate the retrieved state | state | State should be valid | Should Pass |
*/
TEST(L1AudioDecoderTest, test_l1_audiodecoder_positive_1_getState)
{
    ::com::rdk::hal::State state;
    ::android::binder::Status status = audioDecoder->getState(&state);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(helper_isValidState(state));
}

/**
* @brief This test verifies the ability to open  audio decoders in various codecs.
*
* This test checks the initial state of the audio decoder, ensures there are available decoders, and iteratively opens the decoders for supported codecs and then closes them.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 031@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Get the current state of the audio decoder | state | status.isOk() should be true, state = CLOSED | Should be successful |
* | 02 | Check if there are any decoders available | decoders.size() | decoders.size() > 0 | Should be successful |
* | 03 | Iterate over each decoder in the profile | decoder in decoders | None | Should be successful |
* | 04 | For each decoder, verify if the decoder ID matches the current audio decoder interface ID | _decoder.id = g_audiodecoderId.value | None | Should be successful |
* | 05 | Within the matched decoder, iterate over supported codecs | codec in _decoder.supportedCodecs | None | Should be successful |
* | 06 | Open the audio decoder with the codec and create audioDecoderController with listener | _codec, _decoder.supportsSecure, listener, audioDecoderController | status.isOk() should be true, audioDecoderController != nullptr | Should pass |
* | 07 | Close the audio decoder and verify cleanup succeeded | audioDecoderController, _aidl_return | status.isOk() should be true, _aidl_return = true | Should pass |
*/
TEST(L1AudioDecoderTest, test_l1_audiodecoder_positive_1_open)
{
    //prerequisite - checking state
    ::com::rdk::hal::State state;
    bool _aidl_return;
    ::android::binder::Status status = audioDecoder->getState(&state);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(state, ::com::rdk::hal::State::CLOSED);
    
    ASSERT_TRUE(decoders.size()) <<"No decoders available";
    
    /* iterating over each decoder listed in the profile */
    for(const auto& _decoder: decoders)
    {
        /* checking whether the decoder id same as the one, which is used while getting the current audiodecoder interface*/
        if(_decoder.id == g_audiodecoderId.value)
        {
            /*iterating over supportedCodecs of current audio decoder*/
            for(const auto& _codec: _decoder.supportedCodecs)
            {
                android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderController> audioDecoderController;
                sp<IAudioDecoderControllerListener> listener = new AudioDecoderControllerListener();

                status = audioDecoder->open(_codec, _decoder.supportsSecure, listener, &audioDecoderController);
                ASSERT_TRUE(status.isOk());
                ASSERT_NE(audioDecoderController, nullptr);
                //cleaning up
                status = audioDecoder->close(audioDecoderController, &_aidl_return);
                ASSERT_TRUE(status.isOk());
                ASSERT_TRUE(_aidl_return);
            }

        }
    }
}

/**
* @brief This test case is designed to check the error handling of the audioDecoder's open function when given an invalid codec.
*
* The function tests the behavior of the audioDecoder API when attempting to open an audio decoder with an invalid codec. This scenario is expected to fail. This helps ensure that the audioDecoder correctly identifies and rejects invalid codec values.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 032@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Check initial state of audioDecoder| state | state = CLOSED | Should be successful |
* | 02| Confirm decoders are available| decoders.size() | decoders.size() > 0 | Should be successful |
* | 03| Attempt to open audioDecoder with invalid codec| codec = -1, supportsSecure = decoders[0].supportsSecure, listener = new AudioDecoderControllerListener(), audioDecoderController = nullptr | status.isOk() = false | Should Fail |
*/
TEST(L1AudioDecoderTest, test_l1_audiodecoder_negative_1_open)
{
    // prerequisite - checking state
    ::com::rdk::hal::State state;
    ::android::binder::Status status = audioDecoder->getState(&state);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(state, ::com::rdk::hal::State::CLOSED);
    ASSERT_TRUE(decoders.size()) <<"No decoders available";

    android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderController> audioDecoderController;
    sp<IAudioDecoderControllerListener> listener = new AudioDecoderControllerListener();
    status = audioDecoder->open(static_cast<::com::rdk::hal::audiodecoder::Codec>(-1), decoders[0].supportsSecure, listener, &audioDecoderController);
    ASSERT_FALSE(status.isOk());
}

/**
* @brief Test the negative scenario for opening an audio decoder when it is already open.
*
* This test checks whether the system correctly handles the attempt to open an audio decoder that is already open, 
* which should return an illegal state exception.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 033@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Setting up test environment by ensuring initial state and opening the decoder | N/A | Should be successful | Done by Pre-requisite SetUp function |
* | 02 | Attempting to open an already open audio decoder | supportedCodec = decoders[0].supportedCodecs[0], supportsSecure = decoders[0].supportsSecure, listener instance created, audioDecoderController | Status::EX_ILLEGAL_STATE | Should Fail |
* | 03 | Tearing down the test environment by calling close() api | N/A | Should be successful | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderOpenTest, test_l1_audiodecoder_negative_2_open)
{
    sp<IAudioDecoderControllerListener> listener = new AudioDecoderControllerListener();
    //calling open() when state READY (EX_ILLEGAL_STATE)
    Status status = audioDecoder->open(decoders[0].supportedCodecs[0], decoders[0].supportsSecure, listener, &audioDecoderController);
    EXPECT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

/**
* @brief Test to verify the successful closing of an audio decoder
*
* This test ensures that the audio decoder closes correctly when it is in the READY state. The objective is to test the audio decoder's state transition handling and verify that the `close` API functions as expected.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 034@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description                                                                 | Test Data                                | Expected Result                         | Notes                        |
* | :--------------: | --------------------------------------------------------------------------- | --------------------------------------------- |----------------------------------------- | --------------------------- |
* | 01 | Setting up test environment by ensuring initial state and opening the decoder | N/A | Should be successful | Done by Pre-requisite SetUp function |
* | 02               | Retrieve the current state of the audio decoder.                           | status.isOk()                                         | State should be retrieved successfully  | Should be successful        |
* | 03               | Verify if the state is READY.                                              | state = ::com::rdk::hal::State::READY         | State should be READY                   | Should be successful        |
* | 04               | Close the audio decoder.                                                   | audioDecoderController, _aidl_return=true    | Decoder should close successfully       | Should Pass                 |
*/
TEST_F(L1AudioDecoderCloseTest, test_l1_audiodecoder_positive_close_1)
{
    // prerequisite - checking state
    ::com::rdk::hal::State state;
    ::android::binder::Status status = audioDecoder->getState(&state);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(state, ::com::rdk::hal::State::READY);

    bool _aidl_return;
    status = audioDecoder->close(audioDecoderController, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}

/**
* @brief Test for closing the audio decoder in different states
*
* This test checks the behavior of the audio decoder when trying to close it 
* from the READY state and then attempts to close it again from the CLOSED state@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 035@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Setting up test environment by ensuring initial state and opening the decoder | N/A | Should be successful | Done by Pre-requisite SetUp function |
* | 02 | Verify the state is READY | None | status.isOk() should be true , state should be READY | Should be successful |
* | 03 | Call close() method for the first time  | controller = audioDecoderController, return = should be true | status.isOk() should be true, return value should be true | Should Pass |
* | 04 | Call close() method again from CLOSED state | controller = audioDecoderController, return = should be false | status.isOk() should be true, _aidl_return value should be false | Should Fail |
*/
TEST_F(L1AudioDecoderCloseTest, test_l1_audiodecoder_negative_close_1)
{
    // prerequisite - checking state
    ::com::rdk::hal::State state;
    ::android::binder::Status status = audioDecoder->getState(&state);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(state, ::com::rdk::hal::State::READY);

    bool _aidl_return;
    status = audioDecoder->close(audioDecoderController, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
    //calling close() from CLOSED state
    status = audioDecoder->close(audioDecoderController, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

/**
* @brief Test to register an audio decoder event listener and verify its successful registration
*
* This test checks if an audio decoder event listener can be registered successfully. It ensures that the function registering the listener returns true and completes without errors. It also cleans up by unregistering the listener.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 036 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Create a new AudioDecoderEventListener instance | listener = new AudioDecoderEventListener() | AudioDecoderEventListener instance created | Should be successful |
* | 02 | Register the Event listener to audio decoder | listener, &_aidl_return = true | status.isOk() should be true and _aidl_return = true | Should Pass |
* | 03 | Assert the status of the listener registration | ASSERT_TRUE(status.isOk()) | status.isOk() = true | Should be successful |
* | 04 | Assert the return value of the listener registration | ASSERT_EQ(_aidl_return, true) | _aidl_return = true | Should be successful |
* | 05 | Unregister the Event listener | listener, &_aidl_return = true | status.isOk() should be true and _aidl_return = true | Should Pass |
* | 06 | Assert the status of the listener unregistration | ASSERT_TRUE(status.isOk()) | status.isOk() = true | Should be successful |
* | 07 | Assert the return value of the listener unregistration | ASSERT_EQ(_aidl_return, true) | _aidl_return = true | Should be successful |
*/
TEST(L1AudioDecoderTest, test_l1_audiodecoder_positive_1_registerEventListener )
{
    android::sp<com::rdk::hal::audiodecoder::IAudioDecoderEventListener> listener = new AudioDecoderEventListener();
    bool _aidl_return;
    android::binder::Status status = audioDecoder->registerEventListener(listener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(_aidl_return , true);
    //cleaning up
    status = audioDecoder->unregisterEventListener(listener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(_aidl_return , true);
}

/**
* @brief Test the negative case scenario for registering an audio decoder event listener.
*
* This test attempts to register an audio decoder event listener that has already been registered 
* in the SetUp method. It ensures that a listener cannot be registered more than once and validates 
* the appropriate negative responses from the registration function.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 037@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | -------- | ---------- | -------------- | ----- |
* | 01 | Call SetUp to register the event listener first | listener = new AudioDecoderEventListener() | Should be successful | Done by Pre-requisite SetUp function |
* | 02 | Register the listener again to simulate negative scenario | listener = already registered listener, _aidl_return = false | status.isOk() should be true, _aidl_return should be false | Should Fail |
* | 03 | Unregister the first listener to clean up | listener = registered listener, _aidl_return = true | status.isOk() should be true, _aidl_return should be true | Should be successful |
*/
TEST_F(L1AudioDecoderEventRegisterTest, test_l1_audiodecoder_negative_1_registerEventListener )
{
    bool _aidl_return;
    // Attempt to register the listener again (first registration in L1AudioDecoderEventRegisterTest)
    Status status = audioDecoder->registerEventListener(listener, &_aidl_return);
    EXPECT_TRUE(status.isOk());
    EXPECT_FALSE(_aidl_return);
    //cleaning up
    status = audioDecoder->unregisterEventListener(listener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(_aidl_return , true);
}

/**
* @brief Test to validate the positive scenario of unregistering an audio decoder event listener
*
* This test verifies that the audio decoder's unregisterEventListener function works correctly and unregisters the event listener successfully.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 038@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Setup the test environment by creating an AudioDecoderEventListener instance and registering it with the audio decoder | None | AudioDecoderEventListener instance created and registered successfully | Done by Pre-requisite SetUp function |
* | 02 | Unregister the event listener from the audio decoder to verify the function works correctly | listener, &_aidl_return | status.isOk() should return true, _aidl_return should be true | Should Pass |
* | 03 | Verify the function call results using assertion checks | listener, &_aidl_return | status.isOk() should return true, _aidl_return should be true | Should be successful |
*/
TEST_F(L1AudioDecoderEventRegisterTest, test_l1_audiodecoder_positive_1_unregisterEventListener)
{
    bool _aidl_return;
    Status status = audioDecoder->unregisterEventListener(listener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}

/**
* @brief Test to check the behavior when attempting to unregister event listener, which is not registered.
*
* This test verifies the behavior of the audio decoder when trying to unregister an event listener that was never registered. It checks that the unregisterEventListener function returns the expected status and result.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 039@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**
* | Variation / Step | Description                                      | Test Data                                                                                                       | Expected Result                       | Notes                              |
* | :--------------: | ------------------------------------------------ | --------------------------------------------------------------------------------------------------------------- | ------------------------------------- | ---------------------------------- |
* | 01               | Create a new instance of AudioDecoderEventListener | N/A  | Instance of AudioDecoderEventListener created | Should be successful              |
* | 02               | Attempt to unregister the listener using audioDecoder's unregisterEventListener method | listener = AudioDecoderEventListener instance, _aidl_return = uninitialized                                     | _aidl_return = false, status.isOk() = true | Should fail                        |
* | 03               | Assert the status returned by unregisterEventListener is correct  | status.isOk()  | status.isOk() should be true                                   | Should be successful              |
* | 04               | Assert that _aidl_return is false  | _aidl_return = false    | false    | Should Pass    |
*/
TEST(L1AudioDecoderTest, test_l1_audiodecoder_negatitive_1_unregisterEventListener)
{
    android::sp<com::rdk::hal::audiodecoder::IAudioDecoderEventListener> listener = new AudioDecoderEventListener();
    bool _aidl_return;
    // Attempt to unregister the listener which is not registered
    Status status = audioDecoder->unregisterEventListener(listener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

/**
* @brief Test the positive start functionality of the audio decoder.
*
* This test verifies that the audio decoder can be successfully started and stopped, ensuring that the start method works as expected without error.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 040@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | ------------------ | ---------- |----------------- | ----- |
* | 01 | Check the initial state of the audio decoder | state = CLOSED, decoders.size() > 0 | state = CLOSED, decoders available |Done by Pre-requisite SetUp function |
* | 02 | Open the audio decoder with a supported codec and listener | codec = decoders[0].supportedCodecs[0], supportsSecure = decoders[0].supportsSecure, listener = AudioDecoderControllerListener() | audioDecoderController != nullptr | Done by Pre-requisite SetUp function |
* | 03 | Assert that audioDecoder is not null | audioDecoder != nullptr | Should Pass | Should be successful |
* | 04 | Assert that audioDecoderController is not null | audioDecoderController != nullptr | Should Pass | Should be successful |
* | 05 | Start the audio decoder via the controller | None | status.isOk() = true | Should Pass |
* | 06 | Stop the audio decoder to clean up | None | status.isOk() = true | Should Pass |
* | 07 | Check the state of the audio decoder to ensure it is ready | state = READY | status.isOk() = true, state = READY |Done by Pre-requisite TearDown function |
* | 08 | Close the audio decoder | audioDecoderController, return = _aidl_return | status.isOk() = true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderOpenTest, test_l1_audiodecoder_positive_1_start)
{
    ASSERT_NE(audioDecoder, nullptr) << "audioDecoder is null, cannot proceed with test.";
    ASSERT_NE(audioDecoderController, nullptr) << "audioDecoderController is null, cannot proceed with test.";
    Status status = audioDecoderController->start();
    ASSERT_TRUE(status.isOk());
    //cleaning up
    status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Test for negative scenario when starting the audio decoder that is already started
*
* This test checks the behavior of the audio decoder when trying to start it again after it has already been started. The expected behavior is to throw an illegal state exception.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 041@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Setup pre-requisite, check current state | state | State should be CLOSED and decoders should be available | Done by Pre-requisite SetUp function |
* | 02 | Open audio decoder | decoder = decoders[0].supportedCodecs[0], supportsSecure = decoders[0].supportsSecure, listener | Audio decoder should be opened successfully | Done by Pre-requisite SetUp function |
* | 03 | Verify audioDecoder is not null | audioDecoder = nullptr | Should proceed | Should be successful |
* | 04 | Verify audioDecoderController is not null | audioDecoderController = nullptr | Should proceed | Should be successful |
* | 05 | Start audio decoder | status.isOk() | State should change to STARTED | Should be successful |
* | 06 | Start audio decoder again while in STARTED state | status.exceptionCode() | status should be EX_ILLEGAL_STATE | Should Fail |
* | 07 | Stop audio decoder | status.isOk() | State should change to READY | Should be successful |
* | 08 | Close the audio decoder | audioDecoderController, return = _aidl_return | status.isOk() = true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderOpenTest, test_l1_audiodecoder_negative_1_start)
{
    ASSERT_NE(audioDecoder, nullptr) << "audioDecoder is null, cannot proceed with test.";
    ASSERT_NE(audioDecoderController, nullptr) << "audioDecoderController is null, cannot proceed with test.";
    //changing State to STARTED
    Status status = audioDecoderController->start();
    ASSERT_TRUE(status.isOk());
    //calling start() from STARTED state (EX_ILLEGAL_STATE)
    status = audioDecoderController->start();
    EXPECT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
    //cleaning up
    status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Test the functionality of stopping the audio decoder controller.
*
* This test validates the correct behavior of the stop method on the audio decoder controller, ensuring that the decoder can be stopped from the STARTED state.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 042 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Invoke SetUp() of the pre-requisite class to initialize and start the audio decoder controller. | None | Should be successful | Done by Pre-requisite SetUp function |
* | 02 | Invoke getState() on the audioDecoder to ensure it is in CLOSED state. | status.isOk() |status.isOk() should be true | Done by Pre-requisite SetUp function |
* | 03 | Invoke open() and check the audioDecoderController is not null after open(). | status.isOk() | status.isOk() should be true | Done by Pre-requisite SetUp function |
* | 04 | Start the audio decoder controller using start() method and ensure status is OK. | status.isOk() | status.isOk() should be true | Done by Pre-requisite SetUp function |
* | 05 | Call the stop() method on the audioDecoderController and ensure the status is OK. | status.isOk() | status.isOk() should be true | Should Pass |
* | 06 | Close the audio decoder | audioDecoderController, return = _aidl_return | status.isOk() = true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_1_stop)
{
    Status status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Test the stop function of the audio decoder when called in an invalid state
*
* This test validates whether the audio decoder controller handles illegal state transitions correctly by invoking the stop function twice consecutively.@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 043@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | SetUp prerequisite state | state | status.isOk() should be true and state == ::com::rdk::hal::State::CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Ensure decoders are available | decoders.size() | decoders.size() > 0 | Done by Pre-requisite SetUp function |
* | 03 | Open audio decoder with first available codec | status = audioDecoder->open(decoders[0].supportedCodecs[0], decoders[0].supportsSecure, listener, &audioDecoderController) | status.isOk() should be true and audioDecoderController != nullptr | Done by Pre-requisite SetUp function |
* | 04 | Start audio decoder controller | status.isOk() | status.isOk() should be true | Done by Pre-requisite SetUp function |
* | 05 | Call the stop API to bring it to READY state | status.isOk() | status.isOk() should be true | Should Pass |
* | 06 | Call the stop API again in READY state | status.isOk() | status.exceptionCode() == Status::EX_ILLEGAL_STATE | Should Fail |
* | 07 | Close the audio decoder | audioDecoderController, return = _aidl_return | status.isOk() = true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_1_stop)
{
    // Perform initial stop to bring it to READY state
    Status status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
    //calling stop() from READY state
    status = audioDecoderController->stop();
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

/**
* @brief This test validates if the `decodeBuffer` method handles invalid input arguments correctly.
*
* The objective of this test is to provide an invalid `nsPresentationTime` value to the `decodeBuffer` method and verify that it returns an appropriate error code for invalid arguments.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 044@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | check initial state | state | decoder state = CLOSED, decoder list available | Done by Pre-requisite SetUp function |
* | 02 | Open decoder with a valid call | decoders[0].supportedCodecs[0], decoders[0].supportsSecure, listener, &audioDecoderController | status.isOk() = true, audioDecoderController != nullptr | Done by Pre-requisite SetUp function |
* | 03 | Start the decoder | status.isOk() | status.isOk() = true | Done by Pre-requisite SetUp function |
* | 04 | Call `decodeBuffer` with invalid nsPresentationTime value | nsPresentationTime = -1000000, bufferHandle = 1, trimStartNs = 1000, trimEndNs = 500, &_aidl_return | status.exceptionCode() = Status::EX_ILLEGAL_ARGUMENT | Should Fail |
* | 05 | Stop the decoder | status.isOk() | status.isOk() = true | Should be successful |
* | 06 | Close the decoder | audioDecoderController, &_aidl_return | status.isOk() = true, _aidl_return = true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_1_decodeBuffer)
{
    int64_t nsPresentationTime = -1000000; //invalid nsPresentationTime
    int64_t bufferHandle = 1;
    int32_t trimStartNs = 1000;
    int32_t trimEndNs = 500;
    bool _aidl_return;
    ::android::binder::Status status = audioDecoderController->decodeBuffer(nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, &_aidl_return);
    EXPECT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_ARGUMENT);
    //cleaning up
    status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

/**
* @brief This test case is designed to validate the behavior of the decodeBuffer function in the IAudioDecoderController class when provided with an invalid trimStartNs value.
*
* The test specifically checks if decodeBuffer correctly handles and returns an illegal argument exception when an invalid trimStartNs value is supplied.@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 045@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Initialize and check the state of the audio decoder | state | Decoder state should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Open the audio decoder with initial parameters | status.isOk() | status.isOk() should be true | Done by Pre-requisite SetUp function |
* | 03 | Initialize audioDecoderController with a listener and ensure it is not null | decoders[0].supportedCodecs[0], decoders[0].supportsSecure, listener | decoder should not be null | Done by Pre-requisite SetUp function |
* | 04 | Start the audio decoder controller | status.isOk() | status.isOk() should be true | Done by Pre-requisite SetUp function |
* | 05 | Call the decodeBuffer method with invalid trimStartNs | nsPresentationTime = 1000000, bufferHandle = 1, trimStartNs = -1000, trimEndNs = 500, _aidl_return | Should return EX_ILLEGAL_ARGUMENT exception | Should Pass |
* | 06 | Stop the audio decoder controller | status.isOk() | status.isOk() should be true | Should be successful |
* | 07 | Check the state of the audio decoder after stop | state | Decoder state should be READY | Done by Pre-requisite TearDown function |
* | 08 | Close the audio decoder | audioDecoderController, _aidl_return | Should close successfully | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_2_decodeBuffer)
{
    int64_t nsPresentationTime = 1000000;
    int64_t bufferHandle = 1;
    int32_t trimStartNs = -1000; //invalid trimStartNs
    int32_t trimEndNs = 500;
    bool _aidl_return;
    ::android::binder::Status status = audioDecoderController->decodeBuffer(nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, &_aidl_return);
    EXPECT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_ARGUMENT);
    //cleaning up
    status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Test for negative scenario in decoding buffer with invalid trimEndNs
*
* This test verifies the behavior of the audio decoder when an invalid trimEndNs value,
* which is negative, is provided. This helps ensure that appropriate error handling 
* mechanisms are in place for illegal argument exceptions.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 046@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Check initial state of the audio decoder | state | State should be CLOSED and at least one decoder available | Done by Pre-requisite SetUp function |
* | 02| Open the audio decoder | Codec: decoders[0].supportedCodecs[0], Secure: decoders[0].supportsSecure, Listener: new AudioDecoderControllerListener() | Decoder should open successfully, audioDecoderController should not be null | Done by Pre-requisite SetUp function |
* | 03| Start the audio decoder | Status.isok() | Status.isok() should be true, decoder should start successfully | Done by Pre-requisite SetUp function |
* | 04| Invoke decodeBuffer with negative trimEndNs | nsPresentationTime = 1000000, bufferHandle = 1, trimStartNs = 1000, trimEndNs = -500, _aidl_return = valid pointer | Status exceptionCode should be EX_ILLEGAL_ARGUMENT | Should Fail |
* | 05| Stop the audio decoder | Status.isok() | Should stop without errors, Status.isok() should be true | Should be successful |
* | 06| Close the audio decoder | Status.isok() | Decoder should close successfully, Status.isok() should be true | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_3_decodeBuffer)
{
    int64_t nsPresentationTime = 1000000;
    int64_t bufferHandle = 1;
    int32_t trimStartNs = 1000;
    int32_t trimEndNs = -500; //invalid trimEndNs
    bool _aidl_return;
    ::android::binder::Status status = audioDecoderController->decodeBuffer(nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, &_aidl_return);
    EXPECT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_ARGUMENT);
    //cleaning up
    status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Test the L1AudioDecoder start functionality with a negative scenario for decodeBuffer.
*
* Tests the behavior of audioDecoderController's decodeBuffer function when provided with invalid buffer handle. 
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 047@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01| Check initial state of the audio decoder | state | State should be CLOSED and at least one decoder available | Done by Pre-requisite SetUp function |
* | 02| Open the audio decoder | Codec: decoders[0].supportedCodecs[0], Secure: decoders[0].supportsSecure, Listener: new AudioDecoderControllerListener() | Decoder should open successfully, audioDecoderController should not be null | Done by Pre-requisite SetUp function |
* | 03| Start the audio decoder | Status.isok() | Status.isok() should be true, decoder should start successfully | Done by Pre-requisite SetUp function |
* | 04 | Invoke decodeBuffer with invalid buffer handle -1 | nsPresentationTime = 1000000, bufferHandle = -1, trimStartNs = 1000, trimEndNs = 500, _aidl_return = false | status.exceptionCode() == Status::EX_ILLEGAL_ARGUMENT | Should fail |
* | 05 | Cleanup by stopping audio decoder controller | status.isOk() | status.isOk() == true | Should be successful |
* | 06| Close the audio decoder | status.isOk() | status.isOk() == true, Decoder should close successfully | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_4_decodeBuffer)
{
    int64_t nsPresentationTime = 1000000;
    int64_t bufferHandle = -1; //invalid bufferHandle
    int32_t trimStartNs = 1000;
    int32_t trimEndNs = 500;
    bool _aidl_return;
    ::android::binder::Status status = audioDecoderController->decodeBuffer(nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, &_aidl_return);
    EXPECT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_ARGUMENT);
    //cleaning up
    status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Test for positive scenario of audio decoder's flush function
*
* This test validates the functionality of the `flush` function of the audio decoder controller when reset parameter is set to true. The state of the decoder is expected to change to READY upon flushing.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 048 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Check the initial state of the audio decoder | state | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Open the audio decoder | codec=supportedCodec, supportsSecure=true | Audio decoder should be opened successfully, and audioDecoderController should not be null | Should be successful |
* | 03 | Start the audio decoder controller | status.isOk() | Status should be OK and controller should start successfully | Done by Pre-requisite SetUp function |
* | 04 | Call the flush function with reset=true | reset=true | status.isOk() should be true and state should change to READY | Should Pass |
* | 05 | Check the final state of the audio decoder | state | State should be READY | Done by Pre-requisite TearDown function |
* | 06 | Close the audio decoder | controller=audioDecoderController | status.isOk() should be true and decoder should be closed successfully | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_1_flush)
{
    bool reset = true;
    ::android::binder::Status status = audioDecoderController->flush(reset);
    ASSERT_TRUE(status.isOk());
    //when flush() called with reset=true state is changed to READY.
}

/**
* @brief Test for flushing an open audio decoder with reset flag set to true, in an invalid state
*
* This test attempts to call the `flush` method on an audio decoder controller object with the reset flag set to true, 
* expecting a failure due to an invalid state. This is to ensure that the flush operation handles illegal state conditions correctly.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 049@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Check the initial state of the audio decoder | state | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Verify if there are available decoders | | At least one decoder should be available | Should be successful |
* | 03 | Open the audio decoder | codec=supportedCodec, supportsSecure=true | Audio decoder should be opened successfully, and audioDecoderController should not be null | Should be successful |
* | 04 | Call `flush` with reset flag set to true in an invalid state | reset = true |  status.exceptionCode() == Status::EX_ILLEGAL_STATE | Should Fail |
* | 05 | Close the audio decoder | controller=audioDecoderController | status.isOk() should be true and decoder should be closed successfully | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderOpenTest, test_l1_audiodecoder_negative_1_flush)
{
    bool reset = true;
    ::android::binder::Status status = audioDecoderController->flush(reset);
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

/**
* @brief Test to check the functionality of signalDiscontinuity in the AudioDecoder
*
* This test validates the signalDiscontinuity method on the AudioDecoderController to ensure it handles 
* a signal discontinuity appropriately without any errors.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 050 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:** @n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Check the initial state of the audio decoder | state | State should be CLOSED | Done by Pre-requisite SetUp function |
* | 02 | Open the audio decoder | codec=supportedCodec, supportsSecure=true | Audio decoder should be opened successfully, and audioDecoderController should not be null | Should be successful |
* | 03 | Start the audio decoder controller | status.isOk() | status.isOk() should be true and controller should start successfully | Done by Pre-requisite SetUp function |
* | 04 | Call signalDiscontinuity method on audioDecoderController to signal a discontinuity | status.isOk() | status.isOk() should be true | Should Pass |
* | 05 | Stop the audio decoder by calling the stop method on audioDecoderController | status.isOk() |status.isOk() should be true | Should Pass |
* | 06 | Close the audio decoder | controller=audioDecoderController | status.isOk() should be true and decoder should be closed successfully | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_1_signalDiscontinuity)
{
    Status status = audioDecoderController->signalDiscontinuity();
    EXPECT_TRUE(status.isOk());
    //cleaning up
    status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Test for negative signal discontinuity in the L1 audio decoder
*
* This test verifies the behavior of the audio decoder when a signal
* discontinuity is signaled in an incorrect state. Specifically, it checks 
* the response when the signalDiscontinuity method is called while the 
* decoder is not in a suitable state, expecting an illegal state exception.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 051@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data |Expected Result |Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Check the state of the audio decoder | state | state is CLOSED, decoders available | Done by Pre-requisite Setup function |
* | 02 | Open the audio decoder, and create a controller | codec = decoders[0].supportedCodecs[0], secure = decoders[0].supportsSecure | status.isOk() == true, audioDecoderController != null | Done by Pre-requisite Setup function |
* | 03 | Invoke signalDiscontinuity() | None | status.exceptionCode() == Status::EX_ILLEGAL_STATE | Should Fail |
* | 04 | Close the audio decoder | controller=audioDecoderController | status.isOk() should be true and decoder should be closed successfully | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderOpenTest, test_l1_audiodecoder_negative_1_signalDiscontinuity)
{
    Status status = audioDecoderController->signalDiscontinuity();
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

/**
* @brief This test case verifies that the audio decoder can successfully signal an End-of-Stream (EOS) condition in a positive scenario.
*
* This test ensures that the audio decoder can transition to an EOS state and handle it correctly. Signaling EOS is a key functionality that ensures proper stream termination
* and resource management, which is crucial for maintaining performance and stability.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 052@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :-------------: | ----------- | --------- | ---------------- | ----- |
* | 01 | Check initial state of the decoder | state | State should be CLOSED and decoders should be available | Done by Pre-requisite SetUp function |
* | 02 | Open the audio decoder | codec = decoders[0].supportedCodecs[0], secure = decoders[0].supportsSecure, listener = new AudioDecoderControllerListener() | Decoder should be successfully opened and audioDecoderController should not be null | Done by Pre-requisite SetUp function |
* | 03 | Start the audio decoder | status.isOk() | Decoder should start successfully with status OK | Done by Pre-requisite SetUp function |
* | 04 | Signal End of Stream (EOS) | status.isOk()  | status.isOk() should return true | Should Pass |
* | 05 | Clean up by stopping the decoder | status.isOk() | Decoder should stop successfully with status OK | Should be successful |
* | 06 | Close the audio decoder | controller=audioDecoderController | status.isOk() should be true and decoder should be closed successfully | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_signalEOS)
{
    Status status = audioDecoderController->signalEOS();
    EXPECT_TRUE(status.isOk());
    //cleaning up
    status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Test for negative signalEOS behavior in the Audio Decoder Controller
*
* This test checks the negative scenario when signalEOS() is called. It ensures that the function
* returns the expected exception code indicating an illegal state, thereby validating the function's
* error handling mechanism.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 053@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Call SetUp method to initialize the environment and open the decoder controller | None | Should be successful | Done by Pre-requisite SetUp function |
* | 02 | Invoke signalEOS() to test illegal state handling | status.exceptionCode() | status.exceptionCode() = Status::EX_ILLEGAL_STATE | Should Fail |
* | 03 | Close the audio decoder | controller=audioDecoderController | status.isOk() should be true and decoder should be closed successfully | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderOpenTest, test_l1_audiodecoder_negative_signalEOS)
{
    Status status = audioDecoderController->signalEOS();
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

/**
* @brief Test for verifying negative scenario of parsing codec specific data with empty codec data and MP4_AUDIO_SPECIFIC_CONFIG as csdAudioFormat.
*
* This test verifies the behavior of the parseCodecSpecificData API when provided with an empty codec data vector. This negative test ensures that the API correctly handles invalid or empty input data.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 054@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :------: | ------------- | ----------- | -------------- | ----- |
* | 01 | Check initial state of the decoder | state | State should be CLOSED and decoders should be available | Done by Pre-requisite SetUp function |
* | 02 | Open the audio decoder | codec = decoders[0].supportedCodecs[0], secure = decoders[0].supportsSecure, listener = new AudioDecoderControllerListener() | Decoder should be successfully opened and audioDecoderController should not be null | Done by Pre-requisite SetUp function |
* | 03 | Start the audio decoder | status.isOk() | status.isOk() should be true | Done by Pre-requisite SetUp function |
* | 04 | Call parseCodecSpecificData with empty codec data | csdAudioFormat = CSDAudioFormat::MP4_AUDIO_SPECIFIC_CONFIG, codecData = {}, _aidl_return = false | status.isok() should be true, _aidl_return = false | Should Fail |
* | 05 | Stop the audio decoder | N/A | status = ::android::binder::Status::ok() | Should be successful |
* | 06 | Close the audio decoder | controller=audioDecoderController | status.isOk() should be true and decoder should be closed successfully | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_1_parseCodecSpecificData)
{
    CSDAudioFormat csdAudioFormat = CSDAudioFormat::MP4_AUDIO_SPECIFIC_CONFIG;
    std::vector<uint8_t> codecData = {}; // Empty byte array
    bool _aidl_return;
    Status status = audioDecoderController->parseCodecSpecificData(csdAudioFormat, codecData, &_aidl_return);
    EXPECT_TRUE(status.isOk());
    EXPECT_FALSE(_aidl_return);
    //cleaning up
    status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

/**
* @brief Test the negative scenario for parsing codec-specific data in an audio decoder when the state is READY.
*
* This test checks the behavior of the audio decoder when attempting to parse codec-specific data using an empty byte array. 
* The purpose is to verify that the decoder correctly returns an illegal state exception error. This helps in ensuring that 
* the decoder handles invalid data input correctly.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 056@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :--------------: | ----------- | --------- | --------------- | ----- |
* | 01               | Check initial state and set up decoder for testing | NA | Decoder should be in CLOSED state and successfully open | Done by Pre-requisite SetUp function |
* | 02               | Set the CSDAudioFormat for testing | csdAudioFormat = CSDAudioFormat::MP4_AUDIO_SPECIFIC_CONFIG | Should be successful | Should be successful |
* | 03               | Initialize empty byte array for codec data | codecData = {} | Should be successful | Should be successful |
* | 04               | Call parseCodecSpecificData with empty byte array | csdAudioFormat = CSDAudioFormat::MP4_AUDIO_SPECIFIC_CONFIG, codecData = {}, _aidl_return = false | Status::EX_ILLEGAL_STATE | Should Fail |
* | 05 | Close the audio decoder | controller=audioDecoderController | status.isOk() should be true and decoder should be closed successfully | Done by Pre-requisite TearDown function |
*/
TEST_F(L1AudioDecoderOpenTest, test_l1_audiodecoder_negative_3_parseCodecSpecificData)
{
    CSDAudioFormat csdAudioFormat = CSDAudioFormat::MP4_AUDIO_SPECIFIC_CONFIG;
    std::vector<uint8_t> codecData = {}; // Empty byte array
    bool _aidl_return;
    Status status = audioDecoderController->parseCodecSpecificData(csdAudioFormat, codecData, &_aidl_return);
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

int main(int argc, char **argv) 
{
    std::string yaml_file;
    int opt;
    int result1, result2;
    int number_of_audiodecoders;
    int supportedcodec_count;
    decoder temporary;

     std::string audiodecoderpath = "components.IAudioDecoderManager.IAudioDecoder";

    UT_init(argc, argv);
    if(getService() == -1)
    {
        return -1;
    }

    optind = 1;

    while ((opt = getopt(argc, argv, "p:")) != -1) 
    {
        switch (opt) 
        {
            case 'p':
                yaml_file = optarg;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " -p <yaml_file>" << std::endl;
                return 1;
        }
    }

    if (!yaml_file.empty()) 
    {
        try 
        {
            YAML::Node config = YAML::LoadFile(yaml_file.c_str());
            number_of_audiodecoders = get_list_Count_from_profile(config, audiodecoderpath);
            if(number_of_audiodecoders <= 0)
            {
                std::cout<< " No audiodecoders find in the profile" <<std::endl;
                return -1;
            }
            std::cout <<"audiodecoders : " <<number_of_audiodecoders <<std::endl;

            for(int i=0; i<number_of_audiodecoders; ++i)
            {
                std::string audiodecoderidpath = audiodecoderpath+"."+std::to_string(i) + ".id";
                std::string supportedcodecpath = audiodecoderpath+"."+std::to_string(i) + ".supportedCodecs";
                std::string supportsSecurepath = audiodecoderpath+"."+std::to_string(i) + ".supportsSecure";

                temporary.id = get_integer_value_from_profile(config, audiodecoderidpath);
                temporary.supportsSecure = get_boolean_value_from_profile(config, supportsSecurepath);

                supportedcodec_count = get_list_Count_from_profile(config, supportedcodecpath);

                for(int j=0; j<supportedcodec_count; ++j)
                {
                    std::string codecpath = supportedcodecpath+"."+std::to_string(j);
                    std::string _codec = get_string_value_from_profile(config, codecpath);
                    temporary.supportedCodecs.push_back(helper_StringTocodec(_codec));
                }
                decoders.push_back(temporary);
                temporary.supportedCodecs.clear();
            }

            for(const auto& _decoder: decoders)
            {
                g_audiodecoderId.value = _decoder.id;
                Status status = service->getAudioDecoder(g_audiodecoderId, &audioDecoder);
                ::testing::GTEST_FLAG(filter) = "L1AudioDecoderOpenTest.*:L1AudioDecoderCloseTest.*:L1AudioDecoderEventRegisterTest.*:L1AudioDecoderStartTest.*:L1AudioDecoderTest.*";
                result1 = UT_run_tests();
            }
            ::testing::GTEST_FLAG(filter) = "L1AudioDecoderManagerTest.*";
            result2 = UT_run_tests();
            return (result1 || result2);
        }
        catch (const YAML::BadFile& e) 
        {
            std::cerr << "Error: Could not open YAML file. " << e.what() << std::endl;
            return -1;
        } 
        catch (const YAML::BadConversion& e) 
        {
            std::cerr << "Error: Bad type conversion in YAML. " << e.what() << std::endl;
            return -1;
        }
        catch (const std::runtime_error& e) 
        {
            std::cerr << "Runtime Error: " << e.what() << std::endl;
            return -1;
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return -1;
        }
    }
    else
    {
        std::cout << "USAGE: " << argv[0] << " -p <config_file>" <<std::endl;
        return -1;
    }

}

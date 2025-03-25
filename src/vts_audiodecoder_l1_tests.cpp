
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


using android::sp;
using android::String16;
using android::defaultServiceManager;
using android::binder::Status;

using namespace com::rdk::hal;
using namespace com::rdk::hal::audiodecoder;

static sp<IAudioDecoderManager>                  gAudioDecoderManagerService;
static std::vector<IAudioDecoder::Id>            gAudioDecoderIds;
static std::vector<sp<IAudioDecoder>>            gAudioDecoders;
static std::vector<sp<IAudioDecoderController>>  gAudioDecoderControllers;


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


/**
* Simple implementation of IAudioDecoderEventListener to observe state changes/errors.
*/
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


/**
* Simple implementation of IAudioDecoderControllerListener to observe frame outputs.
*/
class AudioDecoderControllerListener : public BnAudioDecoderControllerListener
{
    public:

        AudioDecoderControllerListener() {};
        virtual ~AudioDecoderControllerListener() = default;

        Status onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::std::optional<FrameMetadata>& metadata) override {
            return ::android::binder::Status::ok();
        }
};

static sp<MyAudioDecoderEventListener>         gDecoderEventListener;
static sp<MyAudioDecoderControllerListener>    gDecoderControllerListener;

static int getService() {
    gAudioDecoderManagerService = interface_cast<IAudioDecoderManager>(
        defaultServiceManager()->getService(String16(IAudioDecoderManager::serviceName().c_str())));
    if (gAudioDecoderManagerService == nullptr) {
        return -1;
    }
    return 0;
}

/**
* AudioDecoderManagerBaseTest
*/
class AudioDecoderManagerBaseTest : public virtual UTCore {
protected:
    static void SetUpTestSuite() {
        UT_ASSERT_EQUAL(getService(), 0);
    }

    static void TearDownTestSuite() {
        gAudioDecoderManagerService.clear();
    }
};

/**
* AudioDecoderBaseTest
*/
class AudioDecoderBaseTest : public AudioDecoderManagerBaseTest {
    protected:
        static void SetUpTestSuite() {
            AudioDecoderManagerBaseTest::SetUpTestSuite();
    
        std::vector<IAudioDecoder::Id> tmpIds;
            android::binder::Status status = gAudioDecoderManagerService->getAudioDecoderIds(&tmpIds);
            UT_ASSERT_TRUE(status.isOk());
            gAudioDecoderIds = tmpIds;
    
            gDecoderEventListener = new MyAudioDecoderEventListener();
    
            for (auto &id : gAudioDecoderIds) {
                sp<IAudioDecoder> decoder;
                android::binder::Status status = gAudioDecoderManagerService->getAudioDecoder(id, &decoder);
                UT_ASSERT_TRUE(status.isOk());
                if (decoder != nullptr) {
                    bool registered = false;
                    decoder->registerEventListener(gDecoderEventListener, &registered);
                    gAudioDecoders.push_back(decoder);
                }
            }
        }
    
        static void TearDownTestSuite() {
            for (auto &decoder : gAudioDecoders) {
                bool unregistered = false;
                decoder->unregisterEventListener(gDecoderEventListener, &unregistered);
            }
            gDecoderEventListener.clear();
        gAudioDecoderIds.clear();
            gAudioDecoders.clear();
            AudioDecoderManagerBaseTest::TearDownTestSuite();
        }
    };

/**
* AudioDecoderControllerBaseTest
*/
class AudioDecoderControllerBaseTest : public AudioDecoderBaseTest {
protected:
    static void SetUpTestSuite() {
        AudioDecoderBaseTest::SetUpTestSuite();

        gDecoderControllerListener = new MyAudioDecoderControllerListener();

        for (auto &decoder : gAudioDecoders) {
            sp<IAudioDecoderController> controller;
            android::binder::Status status = decoder->open(Codec::CODEC_MP3, false, gDecoderControllerListener, &controller);
            UT_ASSERT_TRUE(status.isOk());
            if (controller != nullptr) {
                gAudioDecoderControllers.push_back(controller);
            }
        }
    }

    static void TearDownTestSuite() {
        for (size_t i = 0; i < gAudioDecoders.size(); ++i) {
            if (i < gAudioDecoderControllers.size()) {
                bool closed = false;
                gAudioDecoders[i]->close(gAudioDecoderControllers[i], &closed);
            }
        }
        gAudioDecoderControllers.clear();
        gDecoderControllerListener.clear();
        AudioDecoderBaseTest::TearDownTestSuite();
    }
};


/**
 * @brief Test the retrieval of audio decoder IDs from the AudioDecoderManager service.
 *
 * This test verifies that the AudioDecoderManager service correctly retrieves a list of audio decoder IDs. It checks that the service call is successful, the list is not empty, and the IDs are valid and match expected values.
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
 * | 01 | Initialize audioDecoderIds vector | audioDecoderIds = empty vector | Vector initialized | Should be successful |
 * | 02 | Call getAudioDecoderIds to retrieve IDs | OUT: audioDecoderIds | status.isOk() = true, audioDecoderIds not empty | Should Pass |
 * | 03 | Check each ID is not UNDEFINED | id.value != UNDEFINED | All IDs are valid | Should Pass |
 * | 04 | Verify the count of retrieved IDs matches expected count | expectedCount = UT_KVP_PROFILE_GET_LIST_COUNT(IAudioDecoderManager.IAudioDecoder) | audioDecoderIds.size() == expectedCount | Should Pass |
 * | 05 | Validate each ID matches expected values | expectedIdValue = UT_KVP_PROFILE_GET_UINT32(IAudioDecoderManager.IAudioDecoder.<index>) | ID values match expected values | Should Pass |
 */
UT_ADD_TEST(AudioDecoderManagerBaseTest, test_l1_AudioDecoderManager_positive_getAudioDecoderIds) {
    std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> audioDecoderIds;
    UT_LOG_INFO("Calling getAudioDecoderIds(OUT:audioDecoderIds[])");
    android::binder::Status status = gAudioDecoderManagerService->getAudioDecoderIds(&audioDecoderIds);
    UT_LOG_INFO("Result getAudioDecoderIds(OUT:audioDecoderIds[]), status:[isOK:%s, exception:%s]",
                UT_Control_GetMapString(bool_mapTable, status.isOk()),
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(audioDecoderIds.empty());

    for (const auto& id : audioDecoderIds) {
        UT_ASSERT_NOT_EQUAL(id.value, ::com::rdk::hal::audiodecoder::IAudioDecoder::Id::UNDEFINED);
    }
    
    std::string audiodecoder_count_path = "IAudioDecoderManager.IAudioDecoder"
    size_t expectedCount = UT_KVP_PROFILE_GET_LIST_COUNT(audiodecoder_count_path.c_str());
    UT_ASSERT_EQUAL(audioDecoderIds.size(), expectedCount);

    for (size_t i = 0; i < expectedCount; ++i) {
        std::string audiodecoderidpath = "IAudioDecoderManager.IAudioDecoder."+std::to_string(i) + ".id";
        int32_t expectedIdValue = UT_KVP_PROFILE_GET_UINT32(audiodecoderidpath.c_str());
        UT_ASSERT_TRUE(std::any_of(audioDecoderIds.begin(), audioDecoderIds.end(),
                                [expectedIdValue](const auto& id) { return id.value == expectedIdValue; }));
    }
}

/**
 * @brief Test the positive scenario of retrieving an audio decoder using its ID.
 *
 * This test verifies the functionality of the `getAudioDecoder` method in the `AudioDecoderManagerService` class. It checks whether the method successfully retrieves an audio decoder object for each given decoder ID and ensures that the returned object is valid.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the test environment and retrieve the list of audio decoder IDs. | decoderId = value | Should be successful | |
 * | 02 | Iterate over each decoder ID and call the `getAudioDecoder` method to retrieve the audio decoder object. | decoderId = value, audioDecoder = nullptr | status.isOk() = true, audioDecoder != nullptr | Should Pass |
 * | 03 | Log the result of the `getAudioDecoder` call, including the status and the retrieved audio decoder object. | decoderId = value, audioDecoder = object | Log output should match expected format | Should be successful |
 * | 04 | Assert that the status of the `getAudioDecoder` call is OK and the audio decoder object is not null. | status.isOk() = true, audioDecoder != nullptr | Assertions should pass | Should Pass |
 */

UT_ADD_TEST(AudioDecoderManagerBaseTest, test_l1_AudioDecoderManager_positive_getAudioDecoder) {
    std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> audioDecoderIds;
    UT_LOG_INFO("Calling getAudioDecoderIds(OUT:audioDecoderIds[])");

    android::binder::Status status = gAudioDecoderManagerService->getAudioDecoderIds(&audioDecoderIds);
    UT_LOG_INFO("Result getAudioDecoderIds(OUT:audioDecoderIds[]), status:[isOK:%s, exception:%s]",
                UT_Control_GetMapString(bool_mapTable, status.isOk()),
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

    for (const auto& decoderId : audioDecoderIds) {
        sp<IAudioDecoder> audioDecoder;
        UT_LOG_INFO("Calling getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[])", decoderId.value);
        status = gAudioDecoderManagerService->getAudioDecoder(decoderId, &audioDecoder);
        UT_LOG_INFO("Result getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[0x%X]), status:[isOK:%s, exception:%s]",
                    decoderId.value, audioDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_NOT_EQUAL(audioDecoder, nullptr);
    }
}



/**
 * @brief Test the behavior of getAudioDecoder with an invalid decoder resource ID.
 *
 * This test verifies that the getAudioDecoder function correctly handles an invalid decoder resource ID by ensuring that the output parameter is null and the status is OK. This is important to ensure that the system gracefully handles invalid inputs without crashing or returning unexpected results.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Retrieve the list of valid decoder resource IDs using the YAML configuration. | None | Status is OK | Should be successful |
 * | 02 | Create an invalid decoder resource ID. | invalidId.value = UNDEFINED | None | Should be successful |
 * | 03 | Ensure the invalid ID is not in the list of valid IDs. | invalidId.value = UNDEFINED, validIds = [...] | Assertion is false | Should be successful |
 * | 04 | Call getAudioDecoder with the invalid ID. | invalidId.value = UNDEFINED, audioDecoder = nullptr | Status is OK, audioDecoder is nullptr | Should Pass |
 */
UT_ADD_TEST(AudioDecoderManagerBaseTest, test_l1_AudioDecoderManager_negative_getAudioDecoder) {
    // Test Case 3: Invalid Decoder Resource ID
    UT_LOG_INFO("Starting test: negative_getAudioDecoder");

    // Retrieve the list of valid decoder resource IDs using the YAML configuration
    std::vector<IAudioDecoder::Id> validIds;
    android::binder::Status status = gAudioDecoderManagerService->getAudioDecoderIds(&validIds);
    UT_ASSERT_TRUE(status.isOk());

    // Create an invalid decoder resource ID
    IAudioDecoder::Id invalidId;
    invalidId.value = IAudioDecoder::Id::UNDEFINED;

    // Ensure the invalid ID is not in the list of valid IDs
    UT_ASSERT_FALSE(std::find_if(validIds.begin(), validIds.end(), [&](const IAudioDecoder::Id& id) {
        return id.value == invalidId.value;
    }) != validIds.end());

    // Call getAudioDecoder with the invalid ID
    sp<IAudioDecoder> audioDecoder;
    UT_LOG_INFO("Calling getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[])", invalidId.value);
    status = gAudioDecoderManagerService->getAudioDecoder(invalidId, &audioDecoder);

    // Verify that the output parameter is null
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_EQUAL(audioDecoder, nullptr);

    UT_LOG_INFO("Result getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[0x%X]), status:[isOK:%s, exception:%s]",
                invalidId.value, audioDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()),
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
}



/**
 * @brief Test the positive scenario for getting capabilities of audio decoders.
 *
 * This test verifies that the `getCapabilities` function of each audio decoder in the list returns the expected capabilities, including supported codecs and secure support. It ensures that the function behaves correctly and returns valid data.
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
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the test environment and prepare the list of audio decoders. | gAudioDecoders.size() | Should be successful | Should be successful |
 * | 02 | Iterate over each audio decoder and call the `getCapabilities` function. | capabilities (OUT) | Should Pass | Should Pass |
 * | 03 | Log the result of the `getCapabilities` call for debugging purposes. | capabilities, status | Should be successful | Should be successful |
 * | 04 | Assert that the status of the `getCapabilities` call is OK. | status.isOk() | Should Pass | Should Pass |
 * | 05 | Log the detailed capabilities and status information. | capabilities.supportedCodecs.size(), capabilities.supportsSecure, status.isOk(), status.exceptionCode() | Should be successful | Should be successful |
 * | 06 | Assert that the list of supported codecs is not empty. | capabilities.supportedCodecs.empty() | Should Pass | Should Pass |
 * | 07 | Verify that the number of supported codecs matches the expected count. | capabilities.supportedCodecs.size(), expectedCodecCount | Should Pass | Should Pass |
 * | 08 | For each codec, verify that it is present in the supported codecs list. | codecString, capabilities.supportedCodecs | Should Pass | Should Pass |
 * | 09 | Verify that the secure support matches the expected value. | capabilities.supportsSecure, expectedSupportsSecure | Should Pass | Should Pass |
 */
UT_ADD_TEST(AudioDecoderBaseTest, test_l1_AudioDecoder_positive_getCapabilities) {
    for (size_t i = 0; i < gAudioDecoders.size(); ++i) {
        Capabilities capabilities;
        UT_LOG_INFO("Calling getCapabilities(OUT:capabilities[])");
        android::binder::Status status = gAudioDecoders[i]->getCapabilities(&capabilities);
            
        UT_ASSERT_TRUE(status.isOk());
        UT_LOG_INFO("Result getCapabilities(OUT:capabilities[supportedCodecs.size:%zu, supportsSecure:%s]), status:[isOK:%s, exception:%s]",
                    capabilities.supportedCodecs.size(),
                    capabilities.supportsSecure,
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        
        UT_ASSERT_FALSE(capabilities.supportedCodecs.empty());

        std::string supportedcodec_count_path = "IAudioDecoderManager.IAudioDecoder."+std::to_string(i)+".supportedCodecs"
        size_t expectedCodecCount = UT_KVP_PROFILE_GET_LIST_COUNT(supportedcodec_count_path.c_str());
        UT_ASSERT_EQUAL(capabilities.supportedCodecs.size(), expectedCodecCount);
        
        int codec_iterator = 0;
        for (const auto& codec : capabilities.supportedCodecs) {
            std::string codecString;
            std::string supportedcodecpath = supportedcodec_count_path+"."+std::to_string(codec_iterator)
            UT_KVP_PROFILE_GET_STRING(supportedcodecpath.c_str(), codecString);
            UT_ASSERT_TRUE(std::find_if(capabilities.supportedCodecs.begin(), capabilities.supportedCodecs.end(),
                                     [&codecString](const Codec& c) { return codecString == std::to_string(static_cast<int32_t>(c)); }) != capabilities.supportedCodecs.end());
            codec_iterator++;
        }
        
        std::string supportsecurepath = "IAudioDecoderManager.IAudioDecoder."+std::to_string(i)+".supportsSecure"
        bool expectedSupportsSecure = UT_KVP_PROFILE_GET_BOOL(supportsecurepath.c_str());
        UT_ASSERT_EQUAL(capabilities.supportsSecure, expectedSupportsSecure);
    }
}

/**
 * @brief Test the positive scenarios for the getProperty function of AudioDecoder
 *
 * This test verifies the correct behavior of the getProperty function for various properties of the AudioDecoder. It ensures that the function returns expected values and statuses for each property tested.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the test environment and iterate over each audio decoder in gAudioDecoders. | None | Should be successful | Should be successful |
 * | 02 | Call getProperty with Property::RESOURCE_ID and check the return status and value. | property = RESOURCE_ID, _aidl_return = optional | status.isOk() = true, _aidl_return.has_value() = true, _aidl_return->value->getTag() = intValue | Should Pass |
 * | 03 | Call getProperty with Property::LOW_LATENCY_MODE and check the return status and value. | property = LOW_LATENCY_MODE, _aidl_return = optional | status.isOk() = true, _aidl_return.has_value() = true, _aidl_return->value->getTag() = intValue, lowLatencyMode = 0 or 1 | Should Pass |
 * | 04 | Call getProperty with Property::AV_SOURCE and check the return status and value. | property = AV_SOURCE, _aidl_return = optional | status.isOk() = true, _aidl_return.has_value() = true, _aidl_return->value->getTag() = intValue | Should Pass |
 * | 05 | Call getProperty with Property::SECURE_AUDIO and check the return status and value. | property = SECURE_AUDIO, _aidl_return = optional | status.isOk() = true, _aidl_return.has_value() = true, _aidl_return->value->getTag() = intValue, secureAudio = 0 or 1 | Should Pass |
 * | 06 | Call getProperty with Property::AC4_PRESENTATION_GROUP_INDEX and check the return status and value. | property = AC4_PRESENTATION_GROUP_INDEX, _aidl_return = optional | status.isOk() = true, _aidl_return.has_value() = true, _aidl_return->value->getTag() = intValue, ac4Index >= -1 | Should Pass |
 */
UT_ADD_TEST(AudioDecoderBaseTest, test_l1_AudioDecoder_positive_getProperty) {
    for (auto &decoder : gAudioDecoders) {
        ::std::optional<::com::rdk::hal::PropertyValue> _aidl_return;
        ::android::binder::Status status;

        // Test for Property::RESOURCE_ID
        UT_LOG_INFO("Calling getProperty(IN:property:[RESOURCE_ID], OUT:_aidl_return[])");
        status = decoder->getProperty(::com::rdk::hal::audiodecoder::Property::RESOURCE_ID, &_aidl_return);
        UT_LOG_INFO("Result getProperty(IN:property:[RESOURCE_ID], OUT:_aidl_return[0x%X]), status:[isOK:%s, exception:%s]",
                    _aidl_return.has_value(), UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(_aidl_return.has_value());
        UT_ASSERT_EQUAL(_aidl_return->value->getTag(), ::com::rdk::hal::PropertyValue::Value::Tag::intValue);

        // Test for Property::LOW_LATENCY_MODE
        UT_LOG_INFO("Calling getProperty(IN:property:[LOW_LATENCY_MODE], OUT:_aidl_return[])");
        status = decoder->getProperty(::com::rdk::hal::audiodecoder::Property::LOW_LATENCY_MODE, &_aidl_return);
        UT_LOG_INFO("Result getProperty(IN:property:[LOW_LATENCY_MODE], OUT:_aidl_return[0x%X]), status:[isOK:%s, exception:%s]",
                    _aidl_return.has_value(), UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(_aidl_return.has_value());
        UT_ASSERT_EQUAL(_aidl_return->value->getTag(), ::com::rdk::hal::PropertyValue::Value::Tag::intValue);
        int lowLatencyMode = _aidl_return->value->get<::com::rdk::hal::PropertyValue::Value::Tag::intValue>();
        UT_ASSERT_TRUE(lowLatencyMode == 0 || lowLatencyMode == 1);

        // Test for Property::AV_SOURCE
        UT_LOG_INFO("Calling getProperty(IN:property:[AV_SOURCE], OUT:_aidl_return[])");
        status = decoder->getProperty(::com::rdk::hal::audiodecoder::Property::AV_SOURCE, &_aidl_return);
        UT_LOG_INFO("Result getProperty(IN:property:[AV_SOURCE], OUT:_aidl_return[0x%X]), status:[isOK:%s, exception:%s]",
                    _aidl_return.has_value(), UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(_aidl_return.has_value());
        UT_ASSERT_EQUAL(_aidl_return->value->getTag(), ::com::rdk::hal::PropertyValue::Value::Tag::intValue);

        // Test for Property::SECURE_AUDIO
        UT_LOG_INFO("Calling getProperty(IN:property:[SECURE_AUDIO], OUT:_aidl_return[])");
        status = decoder->getProperty(::com::rdk::hal::audiodecoder::Property::SECURE_AUDIO, &_aidl_return);
        UT_LOG_INFO("Result getProperty(IN:property:[SECURE_AUDIO], OUT:_aidl_return[0x%X]), status:[isOK:%s, exception:%s]",
                    _aidl_return.has_value(), UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(_aidl_return.has_value());
        UT_ASSERT_EQUAL(_aidl_return->value->getTag(), ::com::rdk::hal::PropertyValue::Value::Tag::intValue);
        int secureAudio = _aidl_return->value->get<::com::rdk::hal::PropertyValue::Value::Tag::intValue>();
        UT_ASSERT_TRUE(secureAudio == 0 || secureAudio == 1);

        // Test for Property::AC4_PRESENTATION_GROUP_INDEX
        UT_LOG_INFO("Calling getProperty(IN:property:[AC4_PRESENTATION_GROUP_INDEX], OUT:_aidl_return[])");
        status = decoder->getProperty(::com::rdk::hal::audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX, &_aidl_return);
        UT_LOG_INFO("Result getProperty(IN:property:[AC4_PRESENTATION_GROUP_INDEX], OUT:_aidl_return[0x%X]), status:[isOK:%s, exception:%s]",
                    _aidl_return.has_value(), UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(_aidl_return.has_value());
        UT_ASSERT_EQUAL(_aidl_return->value->getTag(), ::com::rdk::hal::PropertyValue::Value::Tag::intValue);
        int ac4Index = _aidl_return->value->get<::com::rdk::hal::PropertyValue::Value::Tag::intValue>();
        UT_ASSERT_GREATER_EQUAL(ac4Index, -1);
    }
}



/**
 * @brief Test the negative scenario for the getProperty function of AudioDecoder
 *
 * This test verifies the behavior of the getProperty function when an invalid property is passed. 
 * It ensures that the function handles invalid inputs gracefully and returns the expected failure status.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize invalid property value | invalidProperty = 9999 | Should be successful | |
 * | 02 | Log the API call with invalid property | invalidProperty = 9999 | Should be successful | |
 * | 03 | Call getProperty with invalid property | invalidProperty = 9999, aidlReturn = null | Should Fail | |
 * | 04 | Log the result of getProperty call | invalidProperty = 9999, aidlReturn = null, status = failure | Should be successful | |
 * | 05 | Verify that aidlReturn has no value | aidlReturn = null | Should Fail | |
 * | 06 | Verify that status is not OK | status = failure | Should Fail | |
 */
UT_ADD_TEST(AudioDecoderBaseTest, test_l1_AudioDecoder_negative_getProperty) {
    // Setup
    ::com::rdk::hal::audiodecoder::Property invalidProperty = static_cast<::com::rdk::hal::audiodecoder::Property>(9999);
    ::std::optional<::com::rdk::hal::PropertyValue> aidlReturn;

    // Log the API call
    UT_LOG_INFO("Calling getProperty(IN:property:[%d], OUT:_aidl_return[])", static_cast<int32_t>(invalidProperty));

    // Call the API
    android::binder::Status status = gAudioDecoders[0]->getProperty(invalidProperty, &aidlReturn);

    // Log the result
    UT_LOG_INFO("Result getProperty(IN:property:[%d], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]",
                static_cast<int32_t>(invalidProperty),
                aidlReturn.has_value() ? "not null" : "null",
                UT_Control_GetMapString(bool_mapTable, status.isOk()),
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

    // Verification
    UT_ASSERT_FALSE(aidlReturn.has_value());
    UT_ASSERT_FALSE(status.isOk());
}



/**
 * @brief Test the positive scenario of getting the state from AudioDecoder
 *
 * This test verifies the functionality of the getState method in the AudioDecoder class. It checks whether the method correctly retrieves the state of the audio decoder and ensures that the returned state is valid and expected.
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
 * | 01 | Initialize the test environment and prepare the list of audio decoders | None | None | Should be successful |
 * | 02 | Iterate over each audio decoder in the list | None | None | Should be successful |
 * | 03 | Call the getState method on the current audio decoder | decoder = current, state = OUT | status.isOk() = true, state != nullptr | Should Pass |
 * | 04 | Log the result of the getState method call | state = retrieved state, status = call status | None | Should be successful |
 * | 05 | Assert that the status of the getState call is OK | status.isOk() = true | None | Should Pass |
 * | 06 | Assert that the state is not null | state != nullptr | None | Should Pass |
 * | 07 | Assert that the state is one of the expected values | state = one of UNKNOWN, CLOSED, OPENING, READY, STARTING, STARTED, FLUSHING, STOPPING, CLOSING | None | Should Pass |
 */
UT_ADD_TEST(AudioDecoderBaseTest, test_l1_AudioDecoder_positive_getState) {
    for (auto& decoder : gAudioDecoders) {
        ::com::rdk::hal::State state;
        UT_LOG_INFO("Calling getState(OUT:_aidl_return)");

        android::binder::Status status = decoder->getState(&state);

        UT_LOG_INFO("Result getState(OUT:_aidl_return:[%d]), status:[isOK:%s, exception:%s]",
                    static_cast<int32_t>(state),
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_NOT_EQUAL(state, nullptr);
        UT_ASSERT_TRUE(state == ::com::rdk::hal::State::UNKNOWN ||
                    state == ::com::rdk::hal::State::CLOSED ||
                    state == ::com::rdk::hal::State::OPENING ||
                    state == ::com::rdk::hal::State::READY ||
                    state == ::com::rdk::hal::State::STARTING ||
                    state == ::com::rdk::hal::State::STARTED ||
                    state == ::com::rdk::hal::State::FLUSHING ||
                    state == ::com::rdk::hal::State::STOPPING ||
                    state == ::com::rdk::hal::State::CLOSING);
    }
}

/**
 * @brief Test the positive scenario of opening audio decoders.
 *
 * This test verifies the ability to open audio decoders with supported codecs and secure support. It checks if the decoder can be opened successfully and if the controller is not null.
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
 * | 01 | Retrieve the count of supported codecs for each audio decoder | IAudioDecoderManager.IAudioDecoder.<i>.supportedCodecs | Count of supported codecs retrieved | Should be successful |
 * | 02 | Retrieve each supported codec string and add to the list | IAudioDecoderManager.IAudioDecoder.<i>.supportedCodecs.<j>, codec | Codec strings retrieved and added to list | Should be successful |
 * | 03 | Retrieve secure support information for each audio decoder | IAudioDecoderManager.IAudioDecoder.<i>.supportsSecure, supportsSecure | Secure support information retrieved | Should be successful |
 * | 04 | Iterate over each supported codec and map to enum | codecStr, codec | Codec string mapped to enum | Should be successful |
 * | 05 | Call open function with codec, secure support, and listener | codec, supportsSecure, gDecoderControllerListener, controller | Open function called, controller returned | Should Pass |
 * | 06 | Verify the status of the open function call | status.isOk(), controller | Status is OK, controller is not null | Should Pass |
 */
UT_ADD_TEST(AudioDecoderBaseTest, test_l1_AudioDecoder_positive_open) {
    for (size_t i = 0; i < gAudioDecoders.size(); ++i) {
        sp<IAudioDecoderController> controller;
        std::vector<std::string> supportedCodecs;
        bool supportsSecure = false;

        // Retrieve supported codecs and secure support from YAML using macros
        std::string supportedCodecs_count_path = "IAudioDecoderManager.IAudioDecoder."+std::to_string(i)+".supportedCodecs"
        int codecCount = UT_KVP_PROFILE_GET_LIST_COUNT(supportedCodecs_count_path.c_str());
        for (int j = 0; j < codecCount; ++j) {
            std::string codec;
            std::string supportedCodecspath = supportedCodecs_count_path+std::to_string(j)
            UT_KVP_PROFILE_GET_STRING(supportedCodecspath.c_str(), codec);
            supportedCodecs.push_back(codec);
        }
        std::string supportsSecurepath = "IAudioDecoderManager.IAudioDecoder."+std::to_string(i)+".supportsSecure"
        supportsSecure = UT_KVP_PROFILE_GET_BOOL(supportsSecurepath.c_str());

        // Iterate over supported codecs
        for (const auto& codecStr : supportedCodecs) {
            controllerlistener = new MyAudioDecoderControllerListener();

            Codec codec = Codec::PCM; // Default value, replace with actual mapping from string to enum
            if (codecStr == "AAC_LC") codec = Codec::AAC_LC;
            else if (codecStr == "HE_AAC") codec = Codec::HE_AAC;
            else if (codecStr == "HE_AAC2") codec = Codec::HE_AAC2;
            else if (codecStr == "DOLBY_AC3") codec = Codec::DOLBY_AC3;
            else if (codecStr == "DOLBY_AC3_PLUS") codec = Codec::DOLBY_AC3_PLUS;
            else if (codecStr == "DOLBY_AC3_PLUS_JOC") codec = Codec::DOLBY_AC3_PLUS_JOC;
            else if (codecStr == "DOLBY_AC4") codec = Codec::DOLBY_AC4;
            else if (codecStr == "X_HE_AAC") codec = Codec::X_HE_AAC;

            UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:audioDecoderControllerListener:[0x%X], OUT:controller[])", static_cast<int32_t>(codec), supportsSecure, controllerlistener.get());
            android::binder::Status status = gAudioDecoders[i]->open(codec, supportsSecure, controllerlistener, &controller);
            UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:audioDecoderControllerListener:[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", static_cast<int32_t>(codec), supportsSecure, controllerlistener.get(), controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_NOT_EQUAL(controller, nullptr);


            // Calling close API 
            bool isClosed = false;
            UT_LOG_INFO("Calling close(IN:controller[0x%X], OUT:_aidl_return[])", controller.get());
            status = gAudioDecoders[i]->close(controller, &isClosed);
            // Result close
            UT_LOG_INFO("Result close(IN:audioDecoderController[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]",
                        controller.get(), UT_Control_GetMapString(bool_mapTable, isClosed),
                        UT_Control_GetMapString(bool_mapTable, status.isOk()),
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(isClosed);
        }
    }
}



/**
 * @brief Test the negative scenarios for opening an audio decoder.
 *
 * This test evaluates various negative scenarios for the `open` function of the `IAudioDecoder` interface. It checks the behavior when unsupported codecs are used, when secure mode is unsupported, when resources are not in the CLOSED state, and when the controller is nullptr.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Open with Unsupported Codec | codec = REALAUDIO, secure = [value], audioDecoderControllerListener = [address], controller = nullptr | controller == nullptr, status.isOk() == false | Should Fail |
 * | 02 | Open with Secure Mode Unsupported | codec = [supportedCodec], secure = false, audioDecoderControllerListener = [address], controller = nullptr | controller == nullptr, status.isOk() == false | Should Fail |
 * | 03 | Open with Resource Not in CLOSED State | codec = [supportedCodec], secure = [value], audioDecoderControllerListener = [address], controller = nullptr | controller == nullptr, status.exceptionCode() == EX_ILLEGAL_STATE | Should Fail |
 * | 04 | Open with Supported Codec and Secure Mode but IAudioDecoderController as nullptr | codec = [supportedCodec], secure = [value], audioDecoderControllerListener = [address], controller = nullptr | controller == nullptr, status.isOk() == false | Should Fail |
 */
UT_ADD_TEST(AudioDecoderBaseTest, test_l1_AudioDecoder_negative_open) {
    for (size_t i = 0; i < gAudioDecoders.size(); ++i) {
        sp<IAudioDecoder> decoder = gAudioDecoders[i];
        sp<IAudioDecoderController> controller;
        ::android::binder::Status status;

        // Test Case 4: Open with Unsupported Codec
        {
            Codec unsupportedCodec = Codec::REALAUDIO;
            std::string supportsSecurepath = "IAudioDecoderManager.IAudioDecoder."+std::to_string(i)+".supportsSecure"
            bool secure = UT_KVP_PROFILE_GET_BOOL(supportsSecurepath.c_str());
            UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:audioDecoderControllerListener:[0x%X], OUT:controller[nullptr])", static_cast<int>(unsupportedCodec), secure, gDecoderControllerListener.get());
            status = decoder->open(unsupportedCodec, secure, gDecoderControllerListener, &controller);
            UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:audioDecoderControllerListener:[0x%X], OUT:controller[nullptr]), status:[isOK:%s, exception:%s]", static_cast<int>(unsupportedCodec), secure, gDecoderControllerListener.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(controller == nullptr);
            UT_ASSERT_FALSE(status.isOk());
        }

        // Test Case 5: Open with Secure Mode Unsupported
        {
            Codec supportedCodec;
            std::vector<Codec> supportedCodecs;
            std::string supportedCodecs_count_path = "IAudioDecoderManager.IAudioDecoder."+std::to_string(i)+".supportedCodecs"
            std::string supportsSecurepath = "IAudioDecoderManager.IAudioDecoder."+std::to_string(i)+".supportsSecure"

            int codeccount = UT_KVP_PROFILE_GET_LIST_COUNT(supportedCodecs_count_path.c_str());
            for (size_t j = 0; j < codeccount; ++j) {
                std::string supportedCodecspath = supportedCodecs_count_path+std::to_string(j)
                UT_KVP_PROFILE_GET_STRING(supportedCodecspath.c_str(), supportedCodec);
                
                bool secure = !UT_KVP_PROFILE_GET_BOOL(supportsSecurepath.c_str());
                UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:audioDecoderControllerListener:[0x%X], OUT:controller[nullptr])", static_cast<int>(supportedCodec), secure, gDecoderControllerListener.get());
                status = decoder->open(supportedCodec, secure, gDecoderControllerListener, &controller);
                UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:audioDecoderControllerListener:[0x%X], OUT:controller[nullptr]), status:[isOK:%s, exception:%s]", static_cast<int>(supportedCodec), secure, gDecoderControllerListener.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
                UT_ASSERT_TRUE(controller == nullptr);
                UT_ASSERT_FALSE(status.isOk());
            }
        }

        // Test Case 6: Open with Resource Not in CLOSED State
        {
            
            std::vector<std::string> supportedCodecs;
            bool supportsSecure = false;

            sp<IAudioDecoderController> controller1;
            sp<IAudioDecoderController> controller2;
            // Retrieve supported codecs and secure support from YAML using macros
            std::string supportedCodecs_count_path = "IAudioDecoderManager.IAudioDecoder."+std::to_string(0)+".supportedCodecs"
            int codecCount = UT_KVP_PROFILE_GET_LIST_COUNT(supportedCodecs_count_path.c_str());
            for (int j = 0; j < codecCount; ++j) {
                std::string codec;
                std::string supportedCodecspath = supportedCodecs_count_path+std::to_string(j)
                UT_KVP_PROFILE_GET_STRING(supportedCodecspath.c_str(), codec);
                supportedCodecs.push_back(codec);
            }
            std::string supportsSecurepath = "IAudioDecoderManager.IAudioDecoder."+std::to_string(i)+".supportsSecure"
            supportsSecure = UT_KVP_PROFILE_GET_BOOL(supportsSecurepath.c_str());
            
            UT_LOG_INFO("Calling open from CLOSED state")

            UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:audioDecoderControllerListener:[0x%X], OUT:controller[])", static_cast<int32_t>(supportedCodecs[0]), supportsSecure, gDecoderControllerListener.get());
            android::binder::Status status = gAudioDecoders[0]->open(supportedCodecs[0], supportsSecure, gDecoderControllerListener, &controller1);
            UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:audioDecoderControllerListener:[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", static_cast<int32_t>(codec), supportsSecure, gDecoderControllerListener.get(), controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_NOT_EQUAL(controller1, nullptr);

            UT_LOG_INFO("Calling open from READY state")

            UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:audioDecoderControllerListener:[0x%X], OUT:controller[])", static_cast<int32_t>(supportedCodecs[0]), supportsSecure, gDecoderControllerListener.get());
            status = gAudioDecoders[0]->open(supportedCodecs[0], supportsSecure, gDecoderControllerListener, &controller2);
            UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:audioDecoderControllerListener:[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", static_cast<int32_t>(codec), supportsSecure, gDecoderControllerListener.get(), controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

            UT_ASSERT_EQUAL(status.exceptionCode(), binder::Status::EX_ILLEGAL_STATE);
        }


        // Test Case 7: Open with Supported Codec and Secure Mode but IAudioDecoderController as nullptr
        {
            Codec supportedCodec;
            std::vector<Codec> supportedCodecs;
            sp<IAudioDecoderController> nullcontroller = nullptr;

            std::string supportedCodecs_count_path = "IAudioDecoderManager.IAudioDecoder."+std::to_string(i)+".supportedCodecs"
            std::string supportsSecurepath = "IAudioDecoderManager.IAudioDecoder."+std::to_string(i)+".supportsSecure"

            int codec_count = UT_KVP_PROFILE_GET_LIST_COUNT(supportedCodecs_count_path.c_str());
            for (size_t j = 0; j < codec_count; ++j) {
                
                std::string supportedCodecspath = supportedCodecs_count_path+std::to_string(j)
                UT_KVP_PROFILE_GET_STRING(supportedCodecspath.c_str(), supportedCodec);
                bool secure = UT_KVP_PROFILE_GET_BOOL(supportsSecurepath.c_str());

                UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:audioDecoderControllerListener:[0x%X], OUT:controller[nullptr])", static_cast<int>(supportedCodec), secure, gDecoderControllerListener.get());
                status = decoder->open(supportedCodec, secure, gDecoderControllerListener, &nullcontroller);
                UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:audioDecoderControllerListener:[0x%X], OUT:controller[nullptr]), status:[isOK:%s, exception:%s]", static_cast<int>(supportedCodec), secure, gDecoderControllerListener.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
                UT_ASSERT_TRUE(controller == nullptr);
                UT_ASSERT_FALSE(status.isOk());
            }
        }
    }
}



/**
 * @brief Test the positive scenario of closing the audio decoder.
 *
 * This test verifies that the audio decoder can be successfully closed when it is in the READY state. It ensures that the close operation returns a successful status and the decoder is marked as closed.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Check if the audio decoder controllers list is not empty | gAudioDecoderControllers | Should be successful | |
 * | 02 | Ensure each audio decoder is in the READY state by invoking the start method | controller->start() | status.isOk() = true | Should be successful |
 * | 03 | Call the close API on the audio decoder controller | controller->close(controller, &isClosed) | status.isOk() = true, isClosed = true | Should Pass |
 */

UT_ADD_TEST(AudioDecoderBaseTest, test_l1_AudioDecoder_positive_close) {

    bool isClosed = false;
    std::vector<std::string> supportedCodecs;
    bool supportsSecure = false;

    sp<IAudioDecoderController> controller1;

    // Retrieve supported codecs and secure support from YAML using macros
    std::string supportedCodecs_count_path = "IAudioDecoderManager.IAudioDecoder."+std::to_string(0)+".supportedCodecs"
    int codecCount = UT_KVP_PROFILE_GET_LIST_COUNT(supportedCodecs_count_path.c_str());
    for (int j = 0; j < codecCount; ++j) {
        std::string codec;
        std::string supportedCodecspath = supportedCodecs_count_path+std::to_string(j)
        UT_KVP_PROFILE_GET_STRING(supportedCodecspath.c_str(), codec);
        supportedCodecs.push_back(codec);
    }
    std::string supportsSecurepath = "IAudioDecoderManager.IAudioDecoder."+std::to_string(i)+".supportsSecure"
    supportsSecure = UT_KVP_PROFILE_GET_BOOL(supportsSecurepath.c_str());
    
    UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:audioDecoderControllerListener:[0x%X], OUT:controller[])", static_cast<int32_t>(supportedCodecs[0]), supportsSecure, gDecoderControllerListener.get());
    android::binder::Status status = gAudioDecoders[0]->open(supportedCodecs[0], supportsSecure, gDecoderControllerListener, &controller1);
    UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:audioDecoderControllerListener:[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", static_cast<int32_t>(codec), supportsSecure, gDecoderControllerListener.get(), controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_NOT_EQUAL(controller1, nullptr);

    // Calling close API
    UT_LOG_INFO("Calling close(IN:audioDecoderController[0x%X], OUT:_aidl_return[])", controller1.get());
    status = gAudioDecoders[0]->close(controller1, &isClosed);

    // Result close
    UT_LOG_INFO("Result close(IN:audioDecoderController[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]",
                controller1.get(), UT_Control_GetMapString(bool_mapTable, isClosed),
                UT_Control_GetMapString(bool_mapTable, status.isOk()),
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_TRUE(isClosed);

}



/**
 * @brief Test the negative scenarios for closing an audio decoder controller.
 *
 * This test evaluates the behavior of the `close` function when invoked on an audio decoder controller in invalid states. It checks two scenarios: closing when the controller is not in the READY state and closing with an invalid controller instance.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the audio decoder is in a state other than READY. Call the `close` function on the controller. | controller = valid, _aidl_return = false | _aidl_return should be false, status should indicate failure | Should Fail |
 * | 02 | Ensure the audio decoder is in the READY state. Call the `close` function with an invalid controller instance. | controller = nullptr, _aidl_return = false | _aidl_return should be false, status should indicate failure | Should Fail |
 */
UT_ADD_TEST(AudioDecoderBaseTest, test_l1_AudioDecoder_negative_close) {
    sp<IAudioDecoderController> controller1;
    bool isClosed = false;
    ::android::binder::Status status;

    // Calling close API
    UT_LOG_INFO("Calling close without prior open() (IN:audioDecoderController[0x%X], OUT:_aidl_return[])", controller1.get());
    status = gAudioDecoders[0]->close(controller1, &isClosed);

    // Result close
    UT_LOG_INFO("Result close(IN:audioDecoderController[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]",
                controller1.get(), UT_Control_GetMapString(bool_mapTable, isClosed),
                UT_Control_GetMapString(bool_mapTable, status.isOk()),
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(isClosed);
}



/**
 * @brief Test the positive scenario of registering an event listener with audio decoders.
 *
 * This test verifies the functionality of the registerEventListener API by ensuring that an AudioDecoderEventListener can be successfully registered with all available audio decoders. The test checks the status of the registration and asserts that the registration is successful.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 012@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Create a new AudioDecoderEventListener instance | audioDecoderEventListener = new AudioDecoderEventListener() | Instance should be created successfully | Should be successful |
 * | 02 | Iterate over all available audio decoders | decoder = gAudioDecoders[i] | Decoders should be available for iteration | Should be successful |
 * | 03 | Log the call to registerEventListener | Log the input and output parameters | Log should be generated successfully | Should be successful |
 * | 04 | Call the registerEventListener API | audioDecoderEventListener, registered = false | API should return status isOk and registered should be true | Should Pass |
 * | 05 | Verify the results | Assert status.isOk() and registered | Assertions should pass | Should Pass |
 * | 06 | Log the result of registerEventListener | Log the status and exception code | Log should be generated successfully | Should be successful |
 */
UT_ADD_TEST(AudioDecoderBaseTest, test_l1_AudioDecoder_positive_registerEventListener) {
    // Create a new AudioDecoderEventListener instance
    sp<AudioDecoderEventListener> audioDecoderEventListener = new AudioDecoderEventListener();

    // Iterate over all available audio decoders
    for (auto &decoder : gAudioDecoders) {
        bool registered = false;
        // Log the call to registerEventListener
        UT_LOG_INFO("Calling registerEventListener(IN:audioDecoderEventListener[0x%X], OUT:_aidl_return[%s])", audioDecoderEventListener.get(), registered);
        // Call the registerEventListener API
        android::binder::Status status = decoder->registerEventListener(audioDecoderEventListener, &registered);
        // Verify the results
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(registered);
        // Log the result of registerEventListener
        UT_LOG_INFO("Result registerEventListener(IN:audioDecoderEventListener[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", audioDecoderEventListener.get(), UT_Control_GetMapString(bool_mapTable, registered), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
    }
}



/**
 * @brief Test the behavior of registering multiple IAudioDecoderEventListener objects.
 *
 * This test verifies the behavior of the AudioDecoder when attempting to register a second IAudioDecoderEventListener object. 
 * It ensures that only one listener can be registered at a time, and attempts to register a second listener should fail.
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
 * | 01 | Ensure a IAudioDecoderEventListener is already registered | audioDecoderEventListener = gDecoderEventListener, registered = false | status.isOk() = true, registered = true | Should be successful |
 * | 02 | Attempt to register a new IAudioDecoderEventListener object | audioDecoderEventListener = newListener, secondRegistered = false | status.isOk() = true, secondRegistered = false | Should Pass |
 */
UT_ADD_TEST(AudioDecoderBaseTest, test_l1_AudioDecoder_negative_registerEventListener) {
    // Iterate over all available audio decoders
    for (auto &decoder : gAudioDecoders) {
        // Create a new AudioDecoderEventListener instance
        sp<AudioDecoderEventListener> audioDecoderEventListener = new AudioDecoderEventListener();
        bool registered = false;
        // Log the call to registerEventListener
        UT_LOG_INFO("Calling registerEventListener(IN:audioDecoderEventListener[0x%X], OUT:_aidl_return[%s])", audioDecoderEventListener.get(), registered);
        // Call the registerEventListener API
        android::binder::Status status = decoder->registerEventListener(audioDecoderEventListener, &registered);
        // Verify the results
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(registered);
        // Log the result of registerEventListener
        UT_LOG_INFO("Result registerEventListener(IN:audioDecoderEventListener[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", audioDecoderEventListener.get(), UT_Control_GetMapString(bool_mapTable, registered), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));



        UT_LOG_INFO("Calling registerEventListener with EventListener which is allready registered")
        UT_LOG_INFO("Calling registerEventListener(IN:audioDecoderEventListener[0x%X], OUT:_aidl_return[%s])", audioDecoderEventListener.get(), registered);
        // Call the registerEventListener API
        android::binder::Status status = decoder->registerEventListener(audioDecoderEventListener, &registered);
        // Verify the results
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_FALSE(registered);
        // Log the result of registerEventListener
        UT_LOG_INFO("Result registerEventListener(IN:audioDecoderEventListener[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", audioDecoderEventListener.get(), UT_Control_GetMapString(bool_mapTable, registered), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

    }
}



/**
 * @brief Test the positive scenario of unregistering an event listener from audio decoders.
 *
 * This test verifies that the event listener can be successfully unregistered from each audio decoder. 
 * It ensures that the unregister operation is successful and the listener is no longer registered.
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
 * | 01 | Check if the global audio decoders list is not empty | gAudioDecoders | Should be successful |  |
 * | 02 | Check if the global decoder event listener is not null | gDecoderEventListener | Should be successful |  |
 * | 03 | Iterate over each audio decoder and register the event listener | audioDecoderEventListener = gDecoderEventListener, registered = false | status.isOk() = true, registered = true | Should be successful |
 * | 04 | Unregister the event listener from each audio decoder | audioDecoderEventListener = gDecoderEventListener, unregistered = false | status.isOk() = true, unregistered = true | Should Pass |
 */
UT_ADD_TEST(AudioDecoderBaseTest, test_l1_AudioDecoder_positive_unregisterEventListener) {
    UT_ASSERT_FALSE(gAudioDecoders.empty());
    for (auto &decoder : gAudioDecoders) {

        // Create a new AudioDecoderEventListener instance
        sp<AudioDecoderEventListener> audioDecoderEventListener = new AudioDecoderEventListener();
        bool registered = false;
        // Log the call to registerEventListener
        UT_LOG_INFO("Calling registerEventListener(IN:audioDecoderEventListener[0x%X], OUT:_aidl_return[%s])", audioDecoderEventListener.get(), registered);
        // Call the registerEventListener API
        android::binder::Status status = decoder->registerEventListener(audioDecoderEventListener, &registered);
        // Verify the results
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(registered);
        // Log the result of registerEventListener
        UT_LOG_INFO("Result registerEventListener(IN:audioDecoderEventListener[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", audioDecoderEventListener.get(), UT_Control_GetMapString(bool_mapTable, registered), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));


        bool unregistered = false;
        UT_LOG_INFO("Calling unregisterEventListener(IN:audioDecoderEventListener[0x%X], OUT:unregistered)", audioDecoderEventListener.get());
        status = decoder->unregisterEventListener(audioDecoderEventListener, &unregistered);
        UT_LOG_INFO("Result unregisterEventListener(IN:audioDecoderEventListener[0x%X], OUT:unregistered[%s]), status:[isOK:%s, exception:%s]",
                    audioDecoderEventListener.get(), UT_Control_GetMapString(bool_mapTable, unregistered),
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(unregistered);
    }
}



/**
 * @brief Test the behavior of unregisterEventListener with a non-registered listener
 *
 * This test verifies that calling unregisterEventListener with a listener that has not been registered does not succeed. It checks that the function returns false, indicating the listener was not found, and that the status is OK.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Create a valid instance of IAudioDecoderEventListener that has not been registered | nonRegisteredListener = new AudioDecoderEventListener() | Should be successful | |
 * | 02 | Ensure the nonRegisteredListener is not registered by iterating over gAudioDecoders | nonRegisteredListener, isRegistered = false | Should be successful | |
 * | 03 | Call unregisterEventListener with the non-registered listener | nonRegisteredListener, unregistered = false | Should Fail | |
 * | 04 | Verify the return value is false, indicating the listener was not found | unregistered = false | Should be successful | |
 * | 05 | Verify the status is OK | status.isOk() = true | Should be successful | |
 */

UT_ADD_TEST(AudioDecoderBaseTest, test_l1_AudioDecoder_negative_unregisterEventListener) {

    // Ensure the nonRegisteredListener is not registered
    bool isRegistered = false;
    for (const auto& decoder : gAudioDecoders) 
    {
        // Create a valid instance of IAudioDecoderEventListener that has not been registered
        sp<AudioDecoderEventListener> nonRegisteredListener = new AudioDecoderEventListener();

        // Call unregisterEventListener with the non-registered listener
        bool unregistered = false;
        UT_LOG_INFO("Calling unregisterEventListener(IN:audioDecoderEventListener[0x%X], OUT:_aidl_return)", nonRegisteredListener.get());
        android::binder::Status status = decoder->unregisterEventListener(nonRegisteredListener, &unregistered);

        // Verify the return value is false, indicating the listener was not found
        UT_ASSERT_FALSE(unregistered);
        UT_ASSERT_TRUE(status.isOk());

        // Log the result
        UT_LOG_INFO("Result unregisterEventListener(IN:audioDecoderEventListener[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]",
                    nonRegisteredListener.get(), UT_Control_GetMapString(bool_mapTable, unregistered),
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

    }
}



/**
 * @brief Test the setProperty function of AudioDecoderControllerBase for various properties.
 *
 * This test verifies the ability of the AudioDecoderControllerBase to correctly set different properties using the setProperty function. It checks the function's behavior with various property values and ensures that the function returns the expected results without errors.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Set LOW_LATENCY_MODE to 1 in READY state | property: LOW_LATENCY_MODE, propertyValue: 1 | status.isOk() = true, aidl_return = true | Should Pass |
 * | 02 | Set AV_SOURCE to a valid value in READY state | property: AV_SOURCE, propertyValue: 2 | status.isOk() = true, aidl_return = true | Should Pass |
 * | 03 | Set AC4_PRESENTATION_GROUP_INDEX to 0 in any state | property: AC4_PRESENTATION_GROUP_INDEX, propertyValue: 0 | status.isOk() = true, aidl_return = true | Should Pass |
 * | 04 | Set AC4_PREFERRED_LANG1 to a valid language code in any state | property: AC4_PREFERRED_LANG1, propertyValue: "eng" | status.isOk() = true, aidl_return = true | Should Pass |
 * | 05 | Set AC4_ASSOCIATED_TYPE to 1 in any state | property: AC4_ASSOCIATED_TYPE, propertyValue: 1 | status.isOk() = true, aidl_return = true | Should Pass |
 */
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_positive_setProperty) {
    for (auto &controller : gAudioDecoderControllers) {
        bool aidl_return = false;
        ::android::binder::Status status;

        // Test Case 1: Set LOW_LATENCY_MODE to 1 in READY state
        UT_LOG_INFO("Calling setProperty(IN:property:[LOW_LATENCY_MODE], IN:propertyValue:[1])");
        status = controller->setProperty(::com::rdk::hal::audiodecoder::Property::LOW_LATENCY_MODE, 
                                         ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::intValue>(1), 
                                         &aidl_return);
        UT_LOG_INFO("Result setProperty(IN:property:[LOW_LATENCY_MODE], IN:propertyValue:[1]), status:[isOK:%s, exception:%s], aidl_return:[%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), 
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()), 
                    UT_Control_GetMapString(bool_mapTable, aidl_return));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(aidl_return);

        // Test Case 2: Set AV_SOURCE to a valid value in READY state
        UT_LOG_INFO("Calling setProperty(IN:property:[AV_SOURCE], IN:propertyValue:[2])");
        status = controller->setProperty(::com::rdk::hal::audiodecoder::Property::AV_SOURCE, 
                                         ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::intValue>(2), 
                                         &aidl_return);
        UT_LOG_INFO("Result setProperty(IN:property:[AV_SOURCE], IN:propertyValue:[2]), status:[isOK:%s, exception:%s], aidl_return:[%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), 
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()), 
                    UT_Control_GetMapString(bool_mapTable, aidl_return));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(aidl_return);

        // Test Case 3: Set AC4_PRESENTATION_GROUP_INDEX to 0 in any state
        UT_LOG_INFO("Calling setProperty(IN:property:[AC4_PRESENTATION_GROUP_INDEX], IN:propertyValue:[0])");
        status = controller->setProperty(::com::rdk::hal::audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX, 
                                         ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::intValue>(0), 
                                         &aidl_return);
        UT_LOG_INFO("Result setProperty(IN:property:[AC4_PRESENTATION_GROUP_INDEX], IN:propertyValue:[0]), status:[isOK:%s, exception:%s], aidl_return:[%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), 
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()), 
                    UT_Control_GetMapString(bool_mapTable, aidl_return));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(aidl_return);

        // Test Case 4: Set AC4_PREFERRED_LANG1 to a valid language code in any state
        UT_LOG_INFO("Calling setProperty(IN:property:[AC4_PREFERRED_LANG1], IN:propertyValue:[\"eng\"])");
        status = controller->setProperty(::com::rdk::hal::audiodecoder::Property::AC4_PREFERRED_LANG1, 
                                         ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::stringValue>(::android::String16("eng")), 
                                         &aidl_return);
        UT_LOG_INFO("Result setProperty(IN:property:[AC4_PREFERRED_LANG1], IN:propertyValue:[\"eng\"]), status:[isOK:%s, exception:%s], aidl_return:[%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), 
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()), 
                    UT_Control_GetMapString(bool_mapTable, aidl_return));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(aidl_return);

        // Test Case 5: Set AC4_ASSOCIATED_TYPE to 1 in any state
        UT_LOG_INFO("Calling setProperty(IN:property:[AC4_ASSOCIATED_TYPE], IN:propertyValue:[1])");
        status = controller->setProperty(::com::rdk::hal::audiodecoder::Property::AC4_ASSOCIATED_TYPE, 
                                         ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::intValue>(1), 
                                         &aidl_return);
        UT_LOG_INFO("Result setProperty(IN:property:[AC4_ASSOCIATED_TYPE], IN:propertyValue:[1]), status:[isOK:%s, exception:%s], aidl_return:[%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), 
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()), 
                    UT_Control_GetMapString(bool_mapTable, aidl_return));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(aidl_return);
    }
}



/**
 * @brief Test the negative scenarios for setting properties in AudioDecoderController
 *
 * This test evaluates the behavior of the AudioDecoderController when attempting to set various properties to invalid values or in inappropriate states. The test ensures that the system correctly handles these scenarios by returning appropriate error statuses and values.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Attempt to set LOW_LATENCY_MODE to 1 in a non-READY state | property = LOW_LATENCY_MODE, propertyValue = 1 | status.isOk() = false, _aidl_return = false | Should Fail |
 * | 02 | Attempt to set AV_SOURCE to an invalid value in READY state | property = AV_SOURCE, propertyValue = -1 | status.isOk() = false, _aidl_return = false | Should Fail |
 * | 03 | Attempt to set SECURE_AUDIO to 1 | property = SECURE_AUDIO, propertyValue = 1 | status.isOk() = false, _aidl_return = false | Should Fail |
 * | 04 | Attempt to set METRIC_FRAMES_DECODED to 100 | property = METRIC_FRAMES_DECODED, propertyValue = 100 | status.isOk() = false, _aidl_return = false | Should Fail |
 * | 05 | Attempt to set AC4_PREFERRED_LANG1 to an invalid language code | property = AC4_PREFERRED_LANG1, propertyValue = "xyz" | status.isOk() = false, _aidl_return = false | Should Fail |
 */
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_negative_setProperty) 
{
    for (auto &controller : gAudioDecoderControllers) 
    {
        bool _aidl_return = false;
        ::android::binder::Status status;

        // Test Case 1: Set LOW_LATENCY_MODE to 1 in a non-READY state (STARTED)
        {
            status = controller->start();
            ::com::rdk::hal::PropertyValue propertyValue;
            propertyValue.value = ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::intValue>(1);
            UT_LOG_INFO("Calling setProperty(IN:property:[LOW_LATENCY_MODE], IN:propertyValue:[1])");
            status = controller->setProperty(::com::rdk::hal::audiodecoder::Property::LOW_LATENCY_MODE, propertyValue, &_aidl_return);
            UT_LOG_INFO("Result setProperty(IN:property:[LOW_LATENCY_MODE], IN:propertyValue:[1]), status:[isOK:%s, exception:%s], _aidl_return:[%s]",
                        UT_Control_GetMapString(bool_mapTable, status.isOk()),
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()),
                        UT_Control_GetMapString(bool_mapTable, _aidl_return));
            UT_ASSERT_FALSE(status.isOk());
            UT_ASSERT_FALSE(_aidl_return);
            status = controller->stop();
        }

        // Test Case 2: Set AV_SOURCE to an invalid value in READY state
        {
            
            ::com::rdk::hal::PropertyValue propertyValue;
            propertyValue.value = ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::intValue>(-1);
            UT_LOG_INFO("Calling setProperty(IN:property:[AV_SOURCE], IN:propertyValue:[-1])");
            status = controller->setProperty(::com::rdk::hal::audiodecoder::Property::AV_SOURCE, propertyValue, &_aidl_return);
            UT_LOG_INFO("Result setProperty(IN:property:[AV_SOURCE], IN:propertyValue:[-1]), status:[isOK:%s, exception:%s], _aidl_return:[%s]",
                        UT_Control_GetMapString(bool_mapTable, status.isOk()),
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()),
                        UT_Control_GetMapString(bool_mapTable, _aidl_return));
            UT_ASSERT_FALSE(status.isOk());
            UT_ASSERT_FALSE(_aidl_return);
        }

        // Test Case 3: Set SECURE_AUDIO to 1 which is READ-ONLY
        {
            
            ::com::rdk::hal::PropertyValue propertyValue;
            propertyValue.value = ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::intValue>(1);
            UT_LOG_INFO("Calling setProperty(IN:property:[SECURE_AUDIO], IN:propertyValue:[1])");
            status = controller->setProperty(::com::rdk::hal::audiodecoder::Property::SECURE_AUDIO, propertyValue, &_aidl_return);
            UT_LOG_INFO("Result setProperty(IN:property:[SECURE_AUDIO], IN:propertyValue:[1]), status:[isOK:%s, exception:%s], _aidl_return:[%s]",
                        UT_Control_GetMapString(bool_mapTable, status.isOk()),
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()),
                        UT_Control_GetMapString(bool_mapTable, _aidl_return));
            UT_ASSERT_FALSE(status.isOk());
            UT_ASSERT_FALSE(_aidl_return);
        }

        // Test Case 4: Set METRIC_FRAMES_DECODED to 100
        {
            
            ::com::rdk::hal::PropertyValue propertyValue;
            propertyValue.value = ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::intValue>(100);
            UT_LOG_INFO("Calling setProperty(IN:property:[METRIC_FRAMES_DECODED], IN:propertyValue:[100])");
            status = controller->setProperty(::com::rdk::hal::audiodecoder::Property::METRIC_FRAMES_DECODED, propertyValue, &_aidl_return);
            UT_LOG_INFO("Result setProperty(IN:property:[METRIC_FRAMES_DECODED], IN:propertyValue:[100]), status:[isOK:%s, exception:%s], _aidl_return:[%s]",
                        UT_Control_GetMapString(bool_mapTable, status.isOk()),
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()),
                        UT_Control_GetMapString(bool_mapTable, _aidl_return));
            UT_ASSERT_FALSE(status.isOk());
            UT_ASSERT_FALSE(_aidl_return);
        }

        // Test Case 5: Set AC4_PREFERRED_LANG1 to an invalid language code
        {
            
            ::com::rdk::hal::PropertyValue propertyValue;
            propertyValue.value = ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::stringValue>(::android::String16("xyz"));
            UT_LOG_INFO("Calling setProperty(IN:property:[AC4_PREFERRED_LANG1], IN:propertyValue:[xyz])");
            status = controller->setProperty(::com::rdk::hal::audiodecoder::Property::AC4_PREFERRED_LANG1, propertyValue, &_aidl_return);
            UT_LOG_INFO("Result setProperty(IN:property:[AC4_PREFERRED_LANG1], IN:propertyValue:[xyz]), status:[isOK:%s, exception:%s], _aidl_return:[%s]",
                        UT_Control_GetMapString(bool_mapTable, status.isOk()),
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()),
                        UT_Control_GetMapString(bool_mapTable, _aidl_return));
            UT_ASSERT_FALSE(status.isOk());
            UT_ASSERT_FALSE(_aidl_return);
        }
    }
}



/**
 * @brief Test the positive scenario of starting the audio decoder controller.
 *
 * This test verifies the successful transition of the audio decoder from READY to STARTING and then to STARTED state. It ensures that the start API is functioning correctly and the state transitions are as expected.
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
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the audio decoder is in the READY state | decoder = gAudioDecoders[i], currentState = READY | status.isOk() = true, currentState = READY | Should be successful |
 * | 02 | Log before calling the start API | None | None | Should be successful |
 * | 03 | Invoke the start API | decoder = gAudioDecoders[i] | status.isOk() = true | Should Pass |
 * | 04 | Log the result of the start API | None | None | Should be successful |
 * | 05 | Verify the state transition to STARTING | decoder = gAudioDecoders[i], currentState = STARTING | status.isOk() = true, currentState = STARTING | Should Pass |
 * | 06 | Verify the state transition to STARTED | decoder = gAudioDecoders[i], currentState = STARTED | status.isOk() = true, currentState = STARTED | Should Pass |
 */
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_positive_start) {
    for (auto &controller1 : gAudioDecoderControllers) {
        // Log before calling the start API
        UT_LOG_INFO("Calling start()");
        // Invoke the start API
        android::binder::Status status = controller1->start();
        // Log the result of the start API
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        // Verify the state transition to STARTING and then to STARTED
        UT_ASSERT_TRUE(status.isOk());
    }
}



/**
 * @brief Test the negative scenario of starting an audio decoder when it is already started.
 *
 * This test verifies that the audio decoder does not transition to the STARTING or STARTED state when the start API is called on an already started decoder. It checks that the appropriate exception code is returned.
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
 * | 01 | Ensure the audio decoder is in a state other than READY (e.g., STARTED) | decoder = gAudioDecoders[i] | Decoder should be in STARTED state | Should be successful |
 * | 02 | Log before calling the start API | None | Log should be generated | Should be successful |
 * | 03 | Invoke the start API on the already started decoder | decoder->start() | status.isOk() = false | Should Fail |
 * | 04 | Log the result of the start API | status | Log should contain status details | Should be successful |
 * | 05 | Verify the state does not change to STARTING or STARTED | decoder->getState(&isStartingOrStarted) | isStartingOrStarted = false | Should Fail |
 * | 06 | Check the return value is binder::Status::EX_ILLEGAL_STATE | status.exceptionCode() | exceptionCode = binder::Status::EX_ILLEGAL_STATE | Should Fail |
 */
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_negative_start) {
    for (auto &controller1 : gAudioDecoderControllers) {
        // Ensure the audio decoder is in a state other than READY (e.g., STARTED)
        bool isStarted = false;
        android::binder::Status status = controller1->start();
        UT_ASSERT_TRUE(status.isOk())
        // Log before calling the start API
        
        UT_LOG_INFO("Calling start() from STARTED state");
        // Invoke the start API
        status = controller1->start();
        // Log the result of the start API
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        // Check the return value is binder::Status::EX_ILLEGAL_STATE
        UT_ASSERT_EQUAL(status.exceptionCode(), binder::Status::EX_ILLEGAL_STATE);

        //cleanup
        status = controller1->stop();
    }
}



/**
 * @brief Test the positive scenario of stopping the audio decoder controller.
 *
 * This test verifies the functionality of the stop() API in the AudioDecoderController. It ensures that the audio decoder transitions from the STARTED state to the READY state and that no input data buffers remain allocated after stopping the decoder.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the audio decoder is in the STARTED state by invoking the start() API. | decoder = gAudioDecoders[i], startStatus = decoder->start() | startStatus.isOk() = true | Should be successful |
 * | 02 | Call the stop() API to stop the audio decoder. | decoder = gAudioDecoders[i], stopStatus = decoder->stop() | stopStatus.isOk() = true | Should Pass |
 * | 03 | Verify the state of the audio decoder to confirm it is in the READY state. | decoder = gAudioDecoders[i], currentState, stateStatus = decoder->getState(&currentState) | stateStatus.isOk() = true, currentState = READY | Should be successful |
 * | 04 | Ensure no input data buffers remain allocated. | decoder = gAudioDecoders[i], allocatedBuffers, bufferStatus = decoder->getAllocatedBuffers(&allocatedBuffers) | bufferStatus.isOk() = true, allocatedBuffers.empty() = true | Should be successful |
 */
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_positive_stop) {
    for (auto &controller1 : gAudioDecoderControllers) {
        // Ensure the audio decoder is in the STARTED state by invoking the start() API.
        UT_LOG_INFO("Calling start()");
        android::binder::Status startStatus = controller1->start();
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, startStatus.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, startStatus.exceptionCode()));
        UT_ASSERT_TRUE(startStatus.isOk());

        // Call the stop() API.
        UT_LOG_INFO("Calling stop()");
        android::binder::Status stopStatus = controller1->stop();
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, stopStatus.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, stopStatus.exceptionCode()));
        UT_ASSERT_TRUE(stopStatus.isOk());
    }
}



/**
 * @brief Test the negative scenario of stopping an audio decoder when it is not in the STARTED state.
 *
 * This test verifies that the stop() API returns an EX_ILLEGAL_STATE exception when called on an audio decoder that is not in the STARTED state. This ensures that the stop() function behaves correctly under invalid conditions.
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
 * | 01 | Ensure the audio decoder is in a state other than STARTED (e.g., READY) | decoder = gAudioDecoders[i], isReady = false | status.isOk() = true, isReady = true | Should be successful |
 * | 02 | Call the stop() API on the audio decoder | decoder = gAudioDecoders[i] | status.exceptionCode() = EX_ILLEGAL_STATE | Should Fail |
 */

UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_negative_stop) {
    for (auto &controller1 : gAudioDecoderControllers) {
        // Ensure the audio decoder is in a state other than STARTED (e.g., READY)
        // Call the stop() API
        UT_LOG_INFO("Calling stop()");
        status = controller1->stop();
        // Verify the return status of the stop() API is EX_ILLEGAL_STATE
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_EQUAL(status.exceptionCode(), ::android::binder::Status::EX_ILLEGAL_STATE);
    }
}



/**
 * @brief Test the positive scenario of the AudioDecoderController's decodeBuffer function.
 *
 * This test verifies that the decodeBuffer function of the AudioDecoderController works correctly when the controller is in the STARTED state. It ensures that the function returns a successful status and that the decode operation is successful.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 022@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Start the AudioDecoderController to ensure it is in the STARTED state. | controller = gAudioDecoderControllers[i], started = false | status.isOk() = true, started = true | Should be successful |
 * | 02 | Call the decodeBuffer function with default parameters. | nsPresentationTime = 0, bufferHandle = 0, trimStartNs = 0, trimEndNs = 0, decodeSuccess = false | status.isOk() = true, decodeSuccess = true | Should Pass |
 * | 03 | Stop the AudioDecoderController after the test. | controller = gAudioDecoderControllers[i], stopped = false | status.isOk() = true, stopped = true | Should be successful |
 */
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_positive_decodeBuffer) {
    for (auto& controller1 : gAudioDecoderControllers) {
        // Ensure the controller is in the STARTED state
        bool started = false;
        android::binder::Status status = controller1->start();
        UT_ASSERT_TRUE(status.isOk());

        // Prepare parameters for decodeBuffer
        int64_t nsPresentationTime = 1000; 
        int64_t bufferHandle = 1; 
        int32_t trimStartNs = 1; 
        int32_t trimEndNs = 10; 
        bool decodeSuccess = false;

        // Log the call to decodeBuffer
        UT_LOG_INFO("Calling decodeBuffer(IN:nsPresentationTime:[%lld], IN:bufferHandle:[%lld], IN:trimStartNs:[%d], IN:trimEndNs:[%d], OUT:decodeSuccess[])", nsPresentationTime, bufferHandle, trimStartNs, trimEndNs);

        // Call decodeBuffer
        status = controller1->decodeBuffer(nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, &decodeSuccess);

        // Log the result of decodeBuffer
        UT_LOG_INFO("Result decodeBuffer(IN:nsPresentationTime:[%lld], IN:bufferHandle:[%lld], IN:trimStartNs:[%d], IN:trimEndNs:[%d], OUT:decodeSuccess[%s]), status:[isOK:%s, exception:%s]", nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, UT_Control_GetMapString(bool_mapTable, decodeSuccess), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        // Validate the result
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(decodeSuccess);

        // Stop the controller after the test
        bool stopped = false;
        status = controller1->stop();
        UT_ASSERT_TRUE(status.isOk());
    }
}



/**
 * @brief Test the negative scenarios for the decodeBuffer function in AudioDecoderController
 *
 * This test evaluates the behavior of the decodeBuffer function when provided with invalid inputs or states. It ensures that the function handles errors gracefully and returns appropriate failure statuses.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the test environment and iterate over each audio decoder in gAudioDecoders. | None | None | Should be successful |
 * | 02 | Call decodeBuffer with a valid presentation time, invalid buffer handle, and check the result. | nsPresentationTime = 1000000000, bufferHandle = 12345, trimStartNs = 0, trimEndNs = 0, aidl_return = false | status.isOk() = false, aidl_return = false | Should Fail |
 * | 03 | Call decodeBuffer with a valid presentation time, invalid buffer handle (-1), and check the result. | nsPresentationTime = 1000000000, bufferHandle = -1, trimStartNs = 0, trimEndNs = 0, aidl_return = false | status.isOk() = false, aidl_return = false | Should Fail |
 * | 04 | Call decodeBuffer with an invalid presentation time, valid buffer handle, and check the result. | nsPresentationTime = -1000000000, bufferHandle = 12345, trimStartNs = 0, trimEndNs = 0, aidl_return = false | status.isOk() = false, aidl_return = false | Should Fail |
 */
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_negative_decodeBuffer) {
    for (auto& controller1 : gAudioDecoderControllers) {
        ::android::binder::Status status;
        bool aidl_return = false;

        // Test Case 1: Invalid State - Decoder Not Started
        UT_LOG_INFO("Calling decodeBuffer(IN:nsPresentationTime:[1000000000], IN:bufferHandle:[12345], IN:trimStartNs:[0], IN:trimEndNs:[0], OUT:_aidl_return)");
        status = controller1->decodeBuffer(1000000000, 12345, 0, 0, &aidl_return);
        UT_LOG_INFO("Result decodeBuffer(IN:nsPresentationTime:[1000000000], IN:bufferHandle:[12345], IN:trimStartNs:[0], IN:trimEndNs:[0], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, aidl_return), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_FALSE(status.isOk());
        UT_ASSERT_FALSE(aidl_return);

        // Test Case 2: Invalid Buffer Handle

        status = controller1->start();
        UT_ASSERT_TRUE(status.isOk());

        UT_LOG_INFO("Calling decodeBuffer(IN:nsPresentationTime:[1000000000], IN:bufferHandle:[-1], IN:trimStartNs:[0], IN:trimEndNs:[0], OUT:_aidl_return)");
        status = controller1->decodeBuffer(1000000000, -1, 0, 0, &aidl_return);
        UT_LOG_INFO("Result decodeBuffer(IN:nsPresentationTime:[1000000000], IN:bufferHandle:[-1], IN:trimStartNs:[0], IN:trimEndNs:[0], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, aidl_return), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_FALSE(status.isOk());
        UT_ASSERT_FALSE(aidl_return);

        // Test Case 3: Invalid Presentation Time
        UT_LOG_INFO("Calling decodeBuffer(IN:nsPresentationTime:[-1000000000], IN:bufferHandle:[12345], IN:trimStartNs:[0], IN:trimEndNs:[0], OUT:_aidl_return)");
        status = controller1->decodeBuffer(-1000000000, 12345, 0, 0, &aidl_return);
        UT_LOG_INFO("Result decodeBuffer(IN:nsPresentationTime:[-1000000000], IN:bufferHandle:[12345], IN:trimStartNs:[0], IN:trimEndNs:[0], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, aidl_return), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_FALSE(status.isOk());
        UT_ASSERT_FALSE(aidl_return);

        status = controller1->stop();
        UT_ASSERT_TRUE(status.isOk());

    }
}



/**
 * @brief Test the flush functionality of the AudioDecoderController in positive scenarios.
 *
 * This test verifies the flush operation of the AudioDecoderController when the decoder is in the STARTED state. It checks both scenarios: flush with reset and flush without reset, ensuring the operation completes successfully.
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
 * | 01 | Initialize the decoder and ensure it is not null | decoder = gAudioDecoders[i] | decoder != nullptr | Should be successful |
 * | 02 | Start the decoder and ensure it is in the STARTED state | started = false | status.isOk() && started == true | Should be successful |
 * | 03 | Retrieve the reset value from YAML configuration | reset = UT_KVP_PROFILE_GET_BOOL(...) | reset = value from YAML | Should be successful |
 * | 04 | Call flush with reset value retrieved from YAML | reset = value from YAML | status.isOk() == true | Should Pass |
 * | 05 | Toggle reset value and call flush again | reset = !reset | status.isOk() == true | Should Pass |
 */
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_positive_flush) {
    for (auto& controller1 : gAudioDecoderControllers) {

        // Ensure the decoder is in the STARTED state
        android::binder::Status status = controller1->start();
        UT_ASSERT_TRUE(status.isOk());

        // Retrieve the reset value from YAML using the macro
        bool reset = true;

        // Test Case 1: Flush with Reset
        UT_LOG_INFO("Calling flush(IN:reset:[%s])", reset);
        status = controller1->flush(reset);
        UT_LOG_INFO("Result flush(IN:reset:[%s]), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable,reset),
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());

        // Test Case 2: Flush without Reset
        reset = false;
        UT_LOG_INFO("Calling flush(IN:reset:[%s])",reset);
        status = controller1->flush(reset);
        UT_LOG_INFO("Result flush(IN:reset:[%s]), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable,reset),
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());

        status = controller1->stop();
        UT_ASSERT_TRUE(status.isOk());
    }
}



/**
 * @brief Test the negative flush operation of the AudioDecoderController
 *
 * This test verifies that the flush operation on the audio decoder fails when the decoder is not in the STARTED state. 
 * It ensures that the flush operation returns an illegal state exception when attempted in an incorrect state.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Retrieve the audio decoder instance from the global list | index = i | Should be successful | |
 * | 02 | Check if the audio decoder supports secure mode | index = i | Should be successful | |
 * | 03 | Ensure the audio decoder is not in the STARTED state by checking its state | isStarted = false | status.isOk() = true, isStarted = false | Should be successful |
 * | 04 | Call the flush method on the audio decoder with the reset flag | reset = value from step 02 | status.exceptionCode() = EX_ILLEGAL_STATE | Should Fail |
 */
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_negative_flush) {
    for (auto& controller1 : gAudioDecoderControllers) {
        bool reset = true;
        UT_LOG_INFO("Calling flush from READY state (IN:reset:[%s])", reset);
        status = controller1->flush(reset);
        UT_LOG_INFO("Result flush(IN:reset:[%s]), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, reset),
                    UT_Control_GetMapString(statusException_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        UT_ASSERT_EQUAL(status.exceptionCode(), binder::Status::EX_ILLEGAL_STATE);
    }
}



/**
 * @brief Test the positive scenario of the signalDiscontinuity function in AudioDecoderController.
 *
 * This test verifies that the signalDiscontinuity function is called successfully on each audio decoder
 * and ensures that the decoder is in the STARTED state before the call. It checks the return status
 * and logs the results for further analysis.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the decoder is in the STARTED state by calling the start function. | decoder = gAudioDecoders[i], started = false | status.isOk() = true, started = true | Should be successful |
 * | 02 | Log the action before calling signalDiscontinuity. | None | None | Should be successful |
 * | 03 | Call the signalDiscontinuity API on the decoder. | decoder = gAudioDecoders[i] | status.isOk() = true | Should Pass |
 * | 04 | Log the result of signalDiscontinuity call. | None | None | Should be successful |
 * | 05 | Verify the return status of signalDiscontinuity is successful. | status.isOk() = true | Assertion passes | Should Pass |
 * | 06 | (Conceptual) Verify subsequent calls to decodeBuffer treat buffers as PTS discontinuous. | decoder = gAudioDecoders[i], ptsDiscontinuous = false | status.isOk() = true, ptsDiscontinuous = true | Should Pass |
 */
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_positive_signalDiscontinuity) {
    for (auto& controller1 : gAudioDecoderControllers) {
        // Ensure the decoder is in the STARTED state
        android::binder::Status status = controller1->start();
        UT_ASSERT_TRUE(status.isOk());

        // Log before calling the API
        UT_LOG_INFO("Calling signalDiscontinuity()");
        // Call the signalDiscontinuity API
        status = controller1->signalDiscontinuity();

        // Log the result
        UT_LOG_INFO("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        // Verify the return status is successful
        UT_ASSERT_TRUE(status.isOk());

        status = controller1->stop();
        UT_ASSERT_TRUE(status.isOk());
    }
}



/**
* @brief Test the behavior of signalDiscontinuity when the audio decoder is not in the STARTED state.
*
* This test verifies that the signalDiscontinuity function returns an illegal state exception when called on an audio decoder that is not in the STARTED state. This ensures that the function behaves correctly under invalid conditions.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 027@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* | Variation / Step | Description | Test Data | Expected Result | Notes |
* | :----: | --------- | ---------- |-------------- | ----- |
* | 01 | Retrieve the current state of each audio decoder. | decoder = gAudioDecoders[i], currentState = unknown | currentState should not be STARTED | Should be successful |
* | 02 | Log the call to signalDiscontinuity. | None | Log should be successful | Should be successful |
* | 03 | Call signalDiscontinuity on the decoder and verify the return status. | decoder = gAudioDecoders[i] | status.exceptionCode() should be EX_ILLEGAL_STATE | Should Fail |
*/
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_negative_signalDiscontinuity) {
    for (auto& controller1 : gAudioDecoderControllers) {
        // Log the call to signalDiscontinuity
        UT_LOG_INFO("Calling signalDiscontinuity() without prior START()");
        // Call signalDiscontinuity and verify the return status
        status = controller1->signalDiscontinuity();
        UT_LOG_INFO("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_EQUAL(status.exceptionCode(), binder::Status::EX_ILLEGAL_STATE);
    }
}



/**
 * @brief Test the positive scenario of signaling End Of Stream (EOS) in AudioDecoderController.
 *
 * This test verifies that the signalEOS() method in AudioDecoderController correctly handles the end of stream signal
 * and ensures that the audio decoder is in the STARTED state before signaling EOS. It checks the successful status
 * return from signalEOS() and verifies the triggering of the onFrameOutput() callback with the endOfStream flag set.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the audio decoder is in the STARTED state | controller = gAudioDecoderControllers[i], currentState = STARTED | status.isOk() = true, currentState = STARTED | Should be successful |
 * | 02 | Call signalEOS() and verify successful status | controller = gAudioDecoderControllers[i] | status.isOk() = true | Should Pass |
 * | 03 | Verify onFrameOutput() callback is triggered with endOfStream flag | controller = gAudioDecoderControllers[i] | endOfStream = true | Should be successful |
 */
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_positive_signalEOS) {
    UT_ASSERT_FALSE(gAudioDecoderControllers.empty());

    for (auto& controller1 : gAudioDecoderControllers) {

        // Ensure the decoder is in the STARTED state
        android::binder::Status status = controller1->start();
        UT_ASSERT_TRUE(status.isOk());

        // Call signalEOS() and verify that the method returns a successful status
        UT_LOG_INFO("Calling signalEOS()");
        status = controller1->signalEOS();
        UT_LOG_INFO("Result signalEOS(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());


        status = controller1->stop();
        UT_ASSERT_TRUE(status.isOk());
    }
}



/**
 * @brief Test the negative scenario of signaling EOS on an audio decoder that is not started.
 *
 * This test verifies that the signalEOS() method returns an EX_ILLEGAL_STATE error when called on an audio decoder that is not in the STARTED state. This ensures that the method behaves correctly in invalid states.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Check if the audio decoder is started | decoder = gAudioDecoders[i], isStarted = false | status.isOk() = true, isStarted = false | Should be successful |
 * | 02 | Call signalEOS() on the audio decoder | decoder = gAudioDecoders[i] | status.exceptionCode() = binder::Status::EX_ILLEGAL_STATE | Should Fail |
 */
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_negative_signalEOS) {
    UT_ASSERT_FALSE(gAudioDecoderControllers.empty());

    for (auto& controller1 : gAudioDecoderControllers) {
        // Call signalEOS() and verify that the method returns a illegal_state.
        UT_LOG_INFO("Calling signalEOS() without prior START()");
        status = controller1->signalEOS();
        UT_LOG_INFO("Result signalEOS(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_EQUAL(status.exceptionCode(), binder::Status::EX_ILLEGAL_STATE);
    }
}



/**
 * @brief Test the positive scenario of parsing codec specific data in AudioDecoderController.
 *
 * This test verifies the ability of the AudioDecoderController to correctly parse codec specific data when the controller is in the STARTED state. It ensures that the controller can handle the MP4_AUDIO_SPECIFIC_CONFIG format and return a successful result.
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
 * | 01 | Start the AudioDecoderController to ensure it is in the STARTED state. | started = false | status.isOk() = true, started = true | Should be successful |
 * | 02 | Prepare the codec specific data for testing. | csdAudioFormat = MP4_AUDIO_SPECIFIC_CONFIG, codecData = {0x12, 0x10} | N/A | Should be successful |
 * | 03 | Call parseCodecSpecificData with prepared data and log the call. | csdAudioFormat = MP4_AUDIO_SPECIFIC_CONFIG, codecData = {0x12, 0x10}, result = false | status.isOk() = true, result = true | Should Pass |
 * | 04 | Validate the result of parseCodecSpecificData. | N/A | status.isOk() = true, result = true | Should be successful |
 * | 05 | Stop the AudioDecoderController to ensure it is in the STOPPED state. | stopped = false | status.isOk() = true, stopped = true | Should be successful |
 */
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_positive_parseCodecSpecificData) {
    for (auto& controller1 : gAudioDecoderControllers) {
        // Ensure the controller is in the STARTED state
        android::binder::Status status = controller1->start();
        UT_ASSERT_TRUE(status.isOk());

        // Prepare test data
        ::com::rdk::hal::audiodecoder::CSDAudioFormat csdAudioFormat = ::com::rdk::hal::audiodecoder::CSDAudioFormat::MP4_AUDIO_SPECIFIC_CONFIG;
        std::vector<uint8_t> codecData = {0x12, 0x10}; // Example codec data
        bool result = false;

        // Log and call the API
        UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdAudioFormat:[%d], IN:codecData:[%zu])", static_cast<int32_t>(csdAudioFormat), codecData.size());
        status = controller1->parseCodecSpecificData(csdAudioFormat, codecData, &result);

        // Log the result
        UT_LOG_INFO("Result parseCodecSpecificData(IN:csdAudioFormat:[%d], IN:codecData:[%zu]), status:[isOK:%s, exception:%s], result:[%s]",
                    static_cast<int32_t>(csdAudioFormat), codecData.size(),
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()),
                    UT_Control_GetMapString(bool_mapTable, result));

        // Validate the result
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(result);

        // Stop the controller
        status = controller1->stop();
        UT_ASSERT_TRUE(status.isOk());
    }
}



/**
 * @brief Test the negative scenario of parsing codec specific data in AudioDecoderController.
 *
 * This test checks the behavior of the AudioDecoderController when provided with invalid codec specific data. The test ensures that the controller correctly handles the scenario where the codec data is not valid and returns the expected result.
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
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure all audio decoder controllers are in the STARTED state. | None | Controllers should be in STARTED state | Should be successful |
 * | 02 | Call the start method on each controller and verify it starts successfully. | started = false | status.isOk() = true, started = true | Should be successful |
 * | 03 | Prepare codec specific data with invalid values. | csdAudioFormat = MP4_AUDIO_SPECIFIC_CONFIG, codecData = {0xFF, 0xFF} | None | Should be successful |
 * | 04 | Invoke parseCodecSpecificData on each decoder with invalid codec data and verify the return value. | csdAudioFormat = MP4_AUDIO_SPECIFIC_CONFIG, codecData = {0xFF, 0xFF}, _aidl_return = false | _aidl_return = false | Should Fail |
 */
UT_ADD_TEST(AudioDecoderControllerBaseTest, test_l1_AudioDecoderController_negative_parseCodecSpecificData) {
    // Ensure the resource is in State::STARTED state
    for (auto &controller1 : gAudioDecoderControllers) 
    {
        // Ensure the controller is in the STARTED state
        android::binder::Status status = controller1->start();
        UT_ASSERT_TRUE(status.isOk());


        // Test Case 5: invalid Codec Data Array
        ::com::rdk::hal::audiodecoder::CSDAudioFormat csdAudioFormat = ::com::rdk::hal::audiodecoder::CSDAudioFormat::MP4_AUDIO_SPECIFIC_CONFIG;
        std::vector<uint8_t> codecData = {0xFF, 0xFF};
        bool _aidl_return = false;

        UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdAudioFormat:[%d], IN:codecData:[0x%X, 0x%X], OUT:_aidl_return)", static_cast<int32_t>(csdAudioFormat), codecData[0], codecData[1]);

        for (auto &decoder : gAudioDecoders) 
        {
            android::binder::Status status = controller1->parseCodecSpecificData(csdAudioFormat, codecData, &_aidl_return);
            UT_LOG_INFO("Result parseCodecSpecificData(IN:csdAudioFormat:[%d], IN:codecData:[0x%X, 0x%X], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", static_cast<int32_t>(csdAudioFormat), codecData[0], codecData[1], _aidl_return ? "true" : "false", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_FALSE(_aidl_return);
        }
    }
}


int main(int argc, char **argv)
{
    UT_status_t result;
    UT_init(argc, argv);
    result = UT_run_tests();
    return result;
}

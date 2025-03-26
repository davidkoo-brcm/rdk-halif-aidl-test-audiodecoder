
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

#include <gtest/gtest.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <binder/Status.h>
#include <utils/String16.h>
#include <stdio.h>
#include <vector>
#include <optional>
#include <ut.h>
#include <ut_kvp_profile.h>
#include <ut_log.h>
#include <ut_control_plane.h>


#include "com/rdk/hal/audiodecoder/IAudioDecoderManager.h"
#include "com/rdk/hal/audiodecoder/BnAudioDecoderControllerListener.h"
#include <com/rdk/hal/audiodecoder/BnAudioDecoderEventListener.h>

using android::sp;
using android::String16;
using android::defaultServiceManager;
using android::binder::Status;

using namespace com::rdk::hal;
using namespace com::rdk::hal::audiodecoder;

static constexpr int MAX_TRANSITORY_STATE_WAIT_MS = 10;
static constexpr int MAX_FINAL_STATE_WAIT_MS = 500;

static sp<IAudioDecoderManager>                  gAudioDecoderManagerService;
static std::vector<IAudioDecoder::Id>            gAudioDecoderIds;
static std::vector<sp<IAudioDecoder>>            gAudioDecoders;
static std::vector<sp<IAudioDecoderController>>  gAudioDecoderControllers;

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

class MyAudioDecoderEventListener
        : public BnAudioDecoderEventListener
{
public:
    MyAudioDecoderEventListener()
            : mCurrentState(State::CLOSED) {}

    virtual ~MyAudioDecoderEventListener() {}

    // Implementation of IAudioDecoderEventListener
    Status onStateChanged(State oldState,State newState) override
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mStateChanges.emplace_back(oldState, newState);
            mCurrentState = newState;
        }
        mCV.notify_all(); // Unblock any waiting test code
        return Status::ok();
    }

    Status onDecodeError(::com::rdk::hal::audiodecoder::ErrorCode vendorErrorCode, int32_t errorCode) override
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mErrors.push_back(errorCode);
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
    std::vector<std::pair<State, State>>
    getStateChanges() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mStateChanges;
    }

    std::vector<int32_t> getErrors() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mErrors;
    }

public:
    mutable std::mutex mMutex;
    std::condition_variable mCV;

    State mCurrentState;
    std::vector<std::pair<State, State>> mStateChanges;
    std::vector<int32_t> mErrors;
};

/**
* Simple implementation of IAudioDecoderControllerListener to observe frame outputs.
*/
class MyAudioDecoderControllerListener : public BnAudioDecoderControllerListener {
public:
    MyAudioDecoderControllerListener() = default;

    Status onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::std::optional<::com::rdk::hal::audiodecoder::FrameMetadata>& metadata) override {
        return ::android::binder::Status::ok();
    }
};

static sp<MyAudioDecoderEventListener>         gDecoderEventListener;
static sp<MyAudioDecoderControllerListener>    gDecoderControllerListener;

static int getService() {
    gAudioDecoderManagerService = android::interface_cast<IAudioDecoderManager>(
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
        UT_LOG_INFO("Calling getAudioDecoderIds(OUT:tmpIds:[])");
        Status status = gAudioDecoderManagerService->getAudioDecoderIds(&tmpIds);
        UT_LOG_INFO("Result getAudioDecoderIds(OUT:tmpIds:[size: %d]), status:[isOK:%s, exception:%s]", decoderIds.size(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_FALSE(tmpIds.empty());
        gAudioDecoderIds = tmpIds;

        gDecoderEventListener = new MyAudioDecoderEventListener();

        for (auto &id : gAudioDecoderIds) {
            sp<IAudioDecoder> decoder;
            UT_LOG_INFO("Calling getAudioDecoder(IN:id:[%d], OUT:decoder[])", id.value);
            status = gAudioDecoderManagerService->getAudioDecoder(id, &decoder);
            UT_LOG_INFO("Result getAudioDecoder(IN:id:[%d], OUT:decoder[0x%X]), status:[isOK:%s, exception:%s]", id.value, decoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_NOT_EQUAL(decoder, nullptr);
            if (decoder != nullptr) {
                bool registered = false;
                UT_LOG_INFO("Calling registerEventListener(IN:gDecoderEventListener[0x%X], OUT:registered[])", gDecoderEventListener.get());
                status = decoder->registerEventListener(gDecoderEventListener, &registered);
                UT_LOG_INFO("Result registerEventListener(IN:eventListener[0x%X], OUT:registered[%s]), status:[isOK:%s, exception:%s]", gDecoderEventListener.get(), UT_Control_GetMapString(bool_mapTable, registered), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
                gAudioDecoders.push_back(decoder);
                UT_ASSERT_TRUE(registered);
            }
        }
    }

    static void TearDownTestSuite() {
        for (auto &decoder : gAudioDecoders) {
            bool unregistered = false;
            UT_LOG_INFO("Calling unregisterEventListener(IN:gDecoderEventListener[0x%X], OUT:unregistered[])", gDecoderEventListener.get());
            Status status = decoder->unregisterEventListener(gDecoderEventListener, &unregistered);
            UT_LOG_INFO("Result unregisterEventListener(IN:gDecoderEventListener[0x%X], OUT:unregistered[%s]), status:[isOK:%s, exception:%s]", gDecoderEventListener.get(), UT_Control_GetMapString(bool_mapTable, unregistered), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(unregistered);
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

        // Codec from Yaml file
        for (size_t i = 0; i < gAudioDecoders.size(); ++i) {
            sp<IAudioDecoder> decoder = gAudioDecoders[i];
            bool supportsSecurepath = false;
            std::string supported_secure_path_key =  "IAudioDecoderManager.IAudioDecoder." + std::to_string(i) + ".supportsSecure";
            UT_KVP_PROFILE_GET_BOOL(supported_secure_path_key.c_str());

            char codecName[UT_KVP_MAX_ELEMENT_SIZE];
            std::string codecpath_key = "IAudioDecoderManager.IAudioDecoder." + std::to_string(i) + ".supportedCodecs." + std::to_string(0);
            UT_KVP_PROFILE_GET_STRING(codecpath_key.c_str(), codecName);

            sp<IAudioDecoderController> controller;
            UT_LOG_INFO("Calling open(IN:codecName[%d], IN:supportsSecurepath[%s], IN:gDecoderControllerListener[0x%X], OUT:controller[])", codecName, UT_Control_GetMapString(bool_mapTable, supportsSecurepath), gDecoderControllerListener.get());
            Status status = decoder->open(codecName, supportsSecurepath, gDecoderControllerListener, &controller);
            UT_LOG_INFO("Result open(IN:codecName[%d], IN:supportsSecurepath[%s], IN:gDecoderControllerListener[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", codecName, UT_Control_GetMapString(bool_mapTable, supportsSecurepath), gDecoderControllerListener, controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_NOT_EQUAL(controller, nullptr);
            if (controller != nullptr) {
                gAudioDecoderControllers.push_back(controller);
            }
        }
    }

    static void TearDownTestSuite() {
        for (size_t i = 0; i < gAudioDecoders.size(); ++i) {
            if (i < gAudioDecoderControllers.size()) {
                bool closed = false;
                UT_LOG_INFO("Calling close(IN:gAudioDecoderControllers[0x%X], OUT:closed[])", gAudioDecoderControllers[i].get());
                Status status = gAudioDecoders[i]->close(gAudioDecoderControllers[i], &closed);
                UT_LOG_INFO("Result close(IN:gAudioDecoderControllers[0x%X], OUT:closed[%s]), status:[isOK:%s, exception:%s]", gAudioDecoderControllers[i].get(), UT_Control_GetMapString(bool_mapTable, closed), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
                UT_ASSERT_TRUE(status.isOk());
                UT_ASSERT_TRUE(closed);
            }
        }
        gAudioDecoderControllers.clear();
        gDecoderControllerListener.clear();
        AudioDecoderBaseTest::TearDownTestSuite();
    }
};

/**
* @brief Test the ability to read and set properties on audio decoders
*
* This test verifies that properties can be set and retrieved correctly on audio decoders. It ensures that the property LOW_LATENCY_MODE can be set to a specific value and then retrieved to confirm the setting was applied correctly.
*
* **Test Group ID:** 02@n
* **Test Case ID:** 001@n
*
* **Test Procedure:**
* Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
*/
UT_ADD_TEST(AudioDecoderBaseTest, TestGetAndSetProperties) {
UT_LOG_INFO("Starting TestReadAndSetProperties");
for (size_t i = 0; i < gAudioDecoders.size(); ++i) {
sp<IAudioDecoder> decoder = gAudioDecoders[i];

bool supportsSecurepath = false;
std::string supported_secure_path_key =  "IAudioDecoderManager.IAudioDecoder." + std::to_string(i) + ".supportsSecure";
UT_KVP_PROFILE_GET_BOOL(supported_secure_path_key.c_str());

int codecCount = 0;
std::string supported_codec_count_key = "IAudioDecoderManager.IAudioDecoder." + std::to_string(i) + ".supportedCodecs";
codecCount = UT_KVP_PROFILE_GET_LIST_COUNT(supported_codec_count_key.c_str());

for (int j = 0; j < codecCount; ++j){
char codecName[UT_KVP_MAX_ELEMENT_SIZE];
std::string codecpath_key = "IAudioDecoderManager.IAudioDecoder." + std::to_string(i) + ".supportedCodecs." + std::to_string(j);
UT_KVP_PROFILE_GET_STRING(codecpath_key.c_str(), codecName);

sp<IAudioDecoderController> controller;
gDecoderControllerListener = new MyAudioDecoderControllerListener();
UT_LOG_INFO("Calling open(IN:codecName[%d], IN:supportsSecurepath[%s], IN:audioDecoderControllerListener[0x%X])", codecName, UT_Control_GetMapString(bool_mapTable, supportsSecurepath), gDecoderControllerListener.get());
Status status = decoder->open(codecName, supportsSecurepath, gDecoderControllerListener, &controller);
UT_LOG_INFO("Result open(IN:codecName[%d], IN:supportsSecurepath[%s], IN:gDecoderControllerListener[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", codecName, UT_Control_GetMapString(bool_mapTable, supportsSecurepath), gDecoderControllerListener, controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_NOT_EQUAL(controller, nullptr);

Property property = Property::LOW_LATENCY_MODE
PropertyValue propertyValue;
propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);
bool setResult = false;

UT_LOG_INFO("Calling setProperty(IN:property:[LOW_LATENCY_MODE], IN:propertyValue:[1]), OUT:setResult[]");
Status setStatus = controller->setProperty(property, propertyValue, &setResult);
UT_LOG_INFO("Result setProperty(IN:property:[LOW_LATENCY_MODE], IN:propertyValue:[1]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, setStatus.isOk()), UT_Control_GetMapString(statusException_mapTable, setStatus.exceptionCode()));
UT_ASSERT_TRUE(setStatus.isOk());
UT_ASSERT_TRUE(setResult);

// Get properties
std::optional<::com::rdk::hal::PropertyValue> getPropertyValue;

UT_LOG_INFO("Calling getProperty(IN:property:[LOW_LATENCY_MODE], OUT:propertyValue[])");
Status getStatus = decoder->getProperty(Property::LOW_LATENCY_MODE, &getPropertyValue);
UT_LOG_INFO("Result getProperty(IN:property:[LOW_LATENCY_MODE], OUT:propertyValue[%d]), status:[isOK:%s, exception:%s]", getPropertyValue->value->value(), UT_Control_GetMapString(bool_mapTable, getStatus.isOk()), UT_Control_GetMapString(statusException_mapTable, getStatus.exceptionCode()));
UT_ASSERT_TRUE(getStatus.isOk());
UT_ASSERT_TRUE(getPropertyValue.has_value());
UT_ASSERT_EQUAL(getPropertyValue->value->value(), 1);

// Close decoder
bool closed = false;
UT_LOG_INFO("Calling close(IN:controller[0x%X]), OUT:closed[]", controller.get());
status = decoder->close(controller, &closed);
UT_LOG_INFO("Result close(IN:controller[0x%X], OUT:closed[%d]), status:[isOK:%s, exception:%s]", controller.get(), UT_Control_GetMapString(bool_mapTable, closed), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_TRUE(closed);
}
}
}

/**
* @brief Test the ability to read and set properties on multi instances of audio decoder
*
* This test verifies that properties can be set and retrieved correctly on audio decoders. It ensures that the property LOW_LATENCY_MODE can be set to a specific value and then retrieved to confirm the setting was applied correctly.
*
* **Test Group ID:** 02@n
* **Test Case ID:** 002@n
*
* **Test Procedure:**
* Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
*/
UT_ADD_TEST(AudioDecoderControllerBaseTest, TestGetAndSetProperties_multipleInstance) {
UT_LOG_INFO("Starting TestGetAndSetProperties_multipleInstance");

for (size_t i = 0; i < gAudioDecoders.size(); ++i) {
sp<IAudioDecoder> decoder = gAudioDecoders[i];
sp<IAudioDecoderController> controller = gAudioDecoderControllers[i];

Property property = Property::LOW_LATENCY_MODE;
PropertyValue propertyValue;
propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);
bool setResult = false;

UT_LOG_INFO("Calling setProperty(IN:property:[LOW_LATENCY_MODE], IN:propertyValue:[1]), OUT:setResult[]");
Status setStatus = controller->setProperty(property, propertyValue, &setResult);
UT_LOG_INFO("Result setProperty(IN:property:[LOW_LATENCY_MODE], IN:propertyValue:[1]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, setStatus.isOk()), UT_Control_GetMapString(statusException_mapTable, setStatus.exceptionCode()));
UT_ASSERT_TRUE(setStatus.isOk());
UT_ASSERT_TRUE(setResult);

// Get properties
std::optional<::com::rdk::hal::PropertyValue> getPropertyValue;

UT_LOG_INFO("Calling getProperty(IN:property:[LOW_LATENCY_MODE], OUT:propertyValue[])");
Status getStatus = decoder->getProperty(Property::LOW_LATENCY_MODE, &getPropertyValue);
UT_LOG_INFO("Result getProperty(IN:property:[LOW_LATENCY_MODE], OUT:propertyValue[%d]), status:[isOK:%s, exception:%s]", getPropertyValue->value->value(), UT_Control_GetMapString(bool_mapTable, getStatus.isOk()), UT_Control_GetMapString(statusException_mapTable, getStatus.exceptionCode()));
UT_ASSERT_TRUE(getStatus.isOk());
UT_ASSERT_TRUE(getPropertyValue.has_value());
UT_ASSERT_EQUAL(getPropertyValue->value->value(), 1);
}
}

/**
* @brief Test the state transitions of the audio decoder
*
* This test verifies the correct state transitions of the audio decoder, including registration, opening, starting, flushing, stopping, closing, and unregistering. It ensures that each operation returns a successful status and the expected state changes occur.
*
* **Test Group ID:** 02@n
* **Test Case ID:** 003@n
*
* **Test Procedure:**
* Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
*/
UT_ADD_TEST(AudioDecoderBaseTest, TestDecoderStateTransitions) {
for (size_t i = 0; i < gAudioDecoders.size(); ++i) {
sp<IAudioDecoder> decoder = gAudioDecoders[i];
// sp<IAudioDecoderController> controller = gAudioDecoderControllers[i];
State state;

bool supportsSecurepath = false;
std::string supported_secure_path_key =  "IAudioDecoderManager.IAudioDecoder." + std::to_string(i) + ".supportsSecure";
UT_KVP_PROFILE_GET_BOOL(supported_secure_path_key.c_str());

int codecCount = 0;
std::string supported_codec_count_key = "IAudioDecoderManager.IAudioDecoder." + std::to_string(i) + ".supportedCodecs";
codecCount = UT_KVP_PROFILE_GET_LIST_COUNT(supported_codec_count_key.c_str());

for (int j = 0; j < codecCount; ++j) {
char codecName[UT_KVP_MAX_ELEMENT_SIZE];
std::string codecpath_key = "IAudioDecoderManager.IAudioDecoder." + std::to_string(i) + ".supportedCodecs." + std::to_string(j);
UT_KVP_PROFILE_GET_STRING(codecpath_key.c_str(), codecName);

// Open decoder
sp<IAudioDecoderController> controller;
UT_LOG_INFO("Calling open(IN:codecName[%d], IN:supportsSecurepath[%s], IN:gDecoderControllerListener[0x%X], OUT:controller[])", codecName, UT_Control_GetMapString(bool_mapTable, supportsSecurepath), gDecoderControllerListener.get());
Status status = decoder->open(codecName, supportsSecurepath, gDecoderControllerListener, &controller);
UT_LOG_INFO("Result open(IN:codecName[%d], IN:supportsSecurepath[%s], IN:gDecoderControllerListener[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", codecName, UT_Control_GetMapString(bool_mapTable, supportsSecurepath), gDecoderControllerListener, controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_NOT_EQUAL(controller, nullptr);

bool foundClosedToOpening = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
    if (oldState == State::CLOSED && newState == State::OPENING) {
    foundClosedToOpening = true;
    break;
    }
}
UT_ASSERT_TRUE(foundClosedToOpening);

gDecoderEventListener->waitForState(State::READY, std::chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS));
UT_LOG_INFO("Calling getState(OUT:state[]");
status = decoder->getState(&state);
UT_LOG_INFO("Result getState(OUT:state[%s]), status:[isOK:%s, exception:%s]", state, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_EQUAL(state, State::READY);

bool foundOpeningToReady = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
    if (oldState == State::OPENING && newState == State::READY) {
    foundOpeningToReady = true;
    break;
    }
}
UT_ASSERT_TRUE(foundOpeningToReady);

// Start decoder
UT_LOG_INFO("Calling start()");
status = controller->start();
UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());

bool foundReadyToStarting = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
    if (oldState == State::READY && newState == State::STARTING) {
    foundReadyToStarting = true;
    break;
    }
}
UT_ASSERT_TRUE(foundReadyToStarting);

gDecoderEventListener->waitForState(State::STARTED, std::chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS));
UT_LOG_INFO("Calling getState(OUT:state[]");
status = decoder->getState(&state);
UT_LOG_INFO("Result getState(OUT:state[%s]), status:[isOK:%s, exception:%s]", state, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_EQUAL(state, State::STARTED);

bool foundStartingToStarted = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
    if (oldState == State::STARTING && newState == State::STARTED) {
    foundStartingToStarted = true;
    break;
    }
}
UT_ASSERT_TRUE(foundStartingToStarted);

// Flush decoder
UT_LOG_INFO("Calling flush(IN:reset[%d])", false);
status = controller->flush(false);
UT_LOG_INFO("Result flush(), status:[isOK:%s, exception:%s]",UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());

bool foundStartedToFlushing = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
if (oldState == State::STARTED && newState == State::FLUSHING) {
foundStartedToFlushing = true;
break;
}
}
UT_ASSERT_TRUE(foundStartedToFlushing);

// Wait for STARTED state
gDecoderEventListener->waitForState(State::STARTED, std::chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS));
UT_LOG_INFO("Calling getState(OUT:state[]");
status = decoder->getState(&state);
UT_LOG_INFO("Result getState(OUT:state[%s]), status:[isOK:%s, exception:%s]", state, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_EQUAL(state, State::STARTED);

bool foundFlushingToStarted = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
if (oldState == State::FLUSHING && newState == State::STARTED) {
foundFlushingToStarted = true;
break;
}
}
UT_ASSERT_TRUE(foundFlushingToStarted);

// Stop decoder
UT_LOG_INFO("Calling stop()");
status = controller->stop();
UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());

bool foundStartedToStopping = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
if (oldState == State::STARTED && newState == State::STOPPING) {
foundStartedToStopping = true;
break;
}
}
UT_ASSERT_TRUE(foundStartedToStopping);


gDecoderEventListener->waitForState(State::READY, std::chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS));
UT_LOG_INFO("Calling getState(OUT:state[]");
status = decoder->getState(&state);
UT_LOG_INFO("Result getState(OUT:state[%s]), status:[isOK:%s, exception:%s]", state, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_EQUAL(state, State::READY);

bool foundStoppingToReady = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
if (oldState == State::STOPPING && newState == State::READY) {
foundStoppingToReady = true;
break;
}
}
UT_ASSERT_TRUE(foundStoppingToReady);


// Close decoder
bool closed = false;
UT_LOG_INFO("Calling close(IN:controller[0x%X], OUT:closed[])", controller.get());
status = decoder->close(controller, &closed);
UT_LOG_INFO("Result close(IN:controller[0x%X], OUT:closed[%s]), status:[isOK:%s, exception:%s]", controller.get(), UT_Control_GetMapString(bool_mapTable, closed), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_TRUE(closed);

bool foundReadyToClosing = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
if (oldState == State::READY && newState == State::CLOSING) {
foundReadyToClosing = true;
break;
}
}
UT_ASSERT_TRUE(foundReadyToClosing);

gDecoderEventListener->waitForState(State::CLOSED, std::chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS));
UT_LOG_INFO("Calling getState(OUT:state[]");
status = decoder->getState(&state);
UT_LOG_INFO("Result getState(OUT:state[%s]), status:[isOK:%s, exception:%s]", state, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_EQUAL(state, State::CLOSED);

bool foundClosingToClosed = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
    if (oldState == State::CLOSING && newState == State::CLOSED) {
    foundClosingToClosed = true;
    break;
    }
}
UT_ASSERT_TRUE(foundClosingToClosed);

}}
}

/**
* @brief Test the state transitions of multiple instances of audio decoders.
*
* This test verifies the correct state transitions of the audio decoder, including registration, opening, starting, flushing, stopping, closing, and unregistering. It ensures that each operation returns a successful status and the expected state changes occur.
*
* **Test Group ID:** 02@n
* **Test Case ID:** 004@n
*
* **Test Procedure:**
* Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
*/
UT_ADD_TEST(AudioDecoderBaseTest, TestDecoderStateTransitions_mutipleInstance) {
for (size_t i = 0; i < gAudioDecoders.size(); ++i) {
sp<IAudioDecoder> decoder = gAudioDecoders[i];
State state;

bool supportsSecurepath = false;
std::string supported_secure_path_key =  "IAudioDecoderManager.IAudioDecoder." + std::to_string(i) + ".supportsSecure";
UT_KVP_PROFILE_GET_BOOL(supported_secure_path_key.c_str());

char codecName[UT_KVP_MAX_ELEMENT_SIZE];
std::string codecpath_key = "IAudioDecoderManager.IAudioDecoder." + std::to_string(i) + ".supportedCodecs." + std::to_string(0);
UT_KVP_PROFILE_GET_STRING(codecpath_key.c_str(), codecName);

sp<IAudioDecoderController> controller;
UT_LOG_INFO("Calling open(IN:codecName[%d], IN:supportsSecurepath[%s], IN:gDecoderControllerListener[0x%X], OUT:controller[])", codecName, UT_Control_GetMapString(bool_mapTable, supportsSecurepath), gDecoderControllerListener.get());
Status status = decoder->open(codecName, supportsSecurepath, gDecoderControllerListener, &controller);
UT_LOG_INFO("Result open(IN:codecName[%d], IN:supportsSecurepath[%s], IN:gDecoderControllerListener[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", codecName, UT_Control_GetMapString(bool_mapTable, supportsSecurepath), gDecoderControllerListener, controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_NOT_EQUAL(controller, nullptr);
if (controller != nullptr) {
gAudioDecoderControllers.push_back(controller);
}

bool foundClosedToOpening = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
if (oldState == State::CLOSED && newState == State::OPENING) {
foundClosedToOpening = true;
break;
}
}
UT_ASSERT_TRUE(foundClosedToOpening);

bool event_status = gDecoderEventListener->waitForState(State::READY, std::chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS));
UT_ASSERT_TRUE(event_status);
UT_LOG_INFO("Calling getState(OUT:state[]");
status = decoder->getState(&state);
UT_LOG_INFO("Result getState(OUT:state[%s]), status:[isOK:%s, exception:%s]", state, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_EQUAL(state, State::READY);

bool foundOpeningToReady = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
if (oldState == State::OPENING && newState == State::READY) {
foundOpeningToReady = true;
break;
}
}
UT_ASSERT_TRUE(foundOpeningToReady);
}

for (size_t i = 0; i < gAudioDecoders.size(); ++i) {
sp<IAudioDecoder> decoder = gAudioDecoders[i];
sp<IAudioDecoderController> controller = gAudioDecoderControllers[i];
State state;

// Start decoder
UT_LOG_INFO("Calling start()");
Status status = controller->start();
UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());

bool foundReadyToStarting = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
    if (oldState == State::READY && newState == State::STARTING) {
    foundReadyToStarting = true;
    break;
    }
}
UT_ASSERT_TRUE(foundReadyToStarting);

bool event_status = gDecoderEventListener->waitForState(State::STARTED, std::chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS));
UT_ASSERT_TRUE(event_status);
UT_LOG_INFO("Calling getState(OUT:state[]");
status = decoder->getState(&state);
UT_LOG_INFO("Result getState(OUT:state[%s]), status:[isOK:%s, exception:%s]", state, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_EQUAL(state, State::STARTED);

bool foundStartingToStarted = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
if (oldState == State::STARTING && newState == State::STARTED) {
foundStartingToStarted = true;
break;
}
}
UT_ASSERT_TRUE(foundStartingToStarted);

// Flush decoder
UT_LOG_INFO("Calling flush(IN:reset[%d])", false);
status = controller->flush(false);
UT_LOG_INFO("Result flush(), status:[isOK:%s, exception:%s]",UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());

bool foundStartedToFlushing = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
if (oldState == State::STARTED && newState == State::FLUSHING) {
foundStartedToFlushing = true;
break;
}
}
UT_ASSERT_TRUE(foundStartedToFlushing);

// Wait for STARTED state
event_status = gDecoderEventListener->waitForState(State::STARTED, std::chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS));
UT_ASSERT_TRUE(event_status);
UT_LOG_INFO("Calling getState(OUT:state[]");
status = decoder->getState(&state);
UT_LOG_INFO("Result getState(OUT:state[%s]), status:[isOK:%s, exception:%s]", state, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_EQUAL(state, State::STARTED);

bool foundFlushingToStarted = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
if (oldState == State::FLUSHING && newState == State::STARTED) {
foundFlushingToStarted = true;
break;
}
}
UT_ASSERT_TRUE(foundFlushingToStarted);


// Stop decoder
UT_LOG_INFO("Calling stop()");
status = controller->stop();
UT_LOG_INFO("Calling getState(OUT:state[]");
UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());

bool foundStartedToStopping = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
    if (oldState == State::STARTED && newState == State::STOPPING) {
    foundStartedToStopping = true;
    break;
    }
}
UT_ASSERT_TRUE(foundStartedToStopping);


event_status = gDecoderEventListener->waitForState(State::READY, std::chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS));
UT_ASSERT_TRUE(event_status);
UT_LOG_INFO("Calling getState(OUT:state[]");
status = decoder->getState(&state);
UT_LOG_INFO("Result getState(OUT:state[%s]), status:[isOK:%s, exception:%s]", state, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_EQUAL(state, State::READY);

bool foundStoppingToReady = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
if (oldState == State::STOPPING && newState == State::READY) {
foundStoppingToReady = true;
break;
}
}
UT_ASSERT_TRUE(foundStoppingToReady);


// Close decoder
bool closed = false;
UT_LOG_INFO("Calling close(IN:controller[0x%X], OUT:closed[])", controller.get());
status = decoder->close(controller, &closed);
UT_LOG_INFO("Result close(IN:controller[0x%X], OUT:closed[%s]), status:[isOK:%s, exception:%s]", controller.get(), UT_Control_GetMapString(bool_mapTable, closed), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_TRUE(closed);

bool foundReadyToClosing = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
    if (oldState == State::READY && newState == State::CLOSING) {
    foundReadyToClosing = true;
    break;
    }
}
UT_ASSERT_TRUE(foundReadyToClosing);

event_status = gDecoderEventListener->waitForState(State::CLOSED, std::chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS));
UT_ASSERT_TRUE(event_status);
UT_LOG_INFO("Calling getState(OUT:state[]");
status = decoder->getState(&state);
UT_LOG_INFO("Result getState(OUT:state[%s]), status:[isOK:%s, exception:%s]", state, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
UT_ASSERT_TRUE(status.isOk());
UT_ASSERT_EQUAL(state, State::CLOSED);

bool foundClosingToClosed = false;
for (const auto& [oldState, newState] : gDecoderEventListener->mStateChanges) {
    if (oldState == State::READY && newState == State::CLOSING) {
    foundClosingToClosed = true;
    break;
    }
}
UT_ASSERT_TRUE(foundClosingToClosed);

}
}

int main(int argc, char **argv)
{
    UT_status_t result;
    UT_init(argc, argv);
    result = UT_run_tests();
    return result;
}


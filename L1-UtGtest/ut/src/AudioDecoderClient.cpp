
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
 
// #include <gtest/gtest.h>
#include <ut_kvp_profile.h>
#include <ut.h>
#include <ut_log.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <binder/Status.h>
#include <utils/String16.h>
#include <stdio.h>
#include <vector>
#include <optional>

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
sp<IAudioDecoderManager> getService()
{
    service = android::interface_cast<IAudioDecoderManager>(
            defaultServiceManager()->getService(String16("audiodecodermanager")));
    if (service == nullptr)
    {
        printf("Service not available\n");
    }
    return service;
}


void validateDecoderIds(const std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id>& ids) 
{
    ASSERT_FALSE(ids.empty());
}

bool validateAudioDecoder(const ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder>& decoder)
{
    // Validate the returned IAudioDecoder object
    if (decoder == nullptr)
    {
        return false;
    }
    return true;
}

void validateCapabilities(const Capabilities& cap)
{
    EXPECT_FALSE(cap.supportedCodecs.empty());
    for (const auto& codec : cap.supportedCodecs)
    {
        EXPECT_GE(static_cast<int>(codec), 0);
        EXPECT_LE(static_cast<int>(codec), 21);
    }
    EXPECT_TRUE(cap.supportsSecure == true || cap.supportsSecure == false);
}

bool validatePropertyValue( const std::optional<::com::rdk::hal::PropertyValue>& propertyValue, const ::com::rdk::hal::audiodecoder::Property property)
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
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= 0;

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
                   (std::get<int32_t>(val) >= -32 && std::get<int32_t>(val) <= 32);

        case ::com::rdk::hal::audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE:
            return std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1);

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

class L1AudioDecoderTest : public UTCore
{
    protected:
        void SetUp() override
        {
            std::vector<IAudioDecoder::Id> decoderIds;
            android::binder::Status status = service->getAudioDecoderIds(&decoderIds);
            ASSERT_TRUE(status.isOk()) << "Failed to retrieve audio decoder IDs";
            ASSERT_FALSE(decoderIds.empty()) << "No audio decoder IDs found";
            if (!decoderIds.empty())
            {
                status = service->getAudioDecoder(decoderIds[0], &audioDecoder);
                ASSERT_TRUE(status.isOk()) << "Failed to retrieve audio decoder for ID: ";
                ASSERT_NE(audioDecoder, nullptr) << "AudioDecoder is null for ID: ";
            }
        }
        void TearDown() override
        {
            printf("tear down : L1AudioDecoderTest\n");
        }
        android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder> audioDecoder;
};

class L1AudioDecoderOpenTest : public L1AudioDecoderTest
{
    protected:
        void SetUp() override
        {
            L1AudioDecoderTest::SetUp();
            //prerequisite - checking state
            ::com::rdk::hal::State state;
            ::android::binder::Status status = audioDecoder->getState(&state);
            ASSERT_TRUE(status.isOk());
            ASSERT_EQ(state, ::com::rdk::hal::State::CLOSED);

            // prerequisite - checking capabilities
            ::com::rdk::hal::audiodecoder::Capabilities capabilities;
            status = audioDecoder->getCapabilities(&capabilities);
            EXPECT_TRUE(status.isOk());
            validateCapabilities(capabilities);

            sp<IAudioDecoderControllerListener> listener = new AudioDecoderControllerListener();
            status = audioDecoder->open(capabilities.supportedCodecs[0], capabilities.supportsSecure, listener, &audioDecoderController);
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
            L1AudioDecoderTest::TearDown();
        }
        android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderController> audioDecoderController;
};

class L1AudioDecoderCloseTest : public L1AudioDecoderTest
{
    protected:
        void SetUp() override
        {
            L1AudioDecoderTest::SetUp();
            //prerequisite - checking state
            ::com::rdk::hal::State state;
            ::android::binder::Status status = audioDecoder->getState(&state);
            ASSERT_TRUE(status.isOk());
            ASSERT_EQ(state, ::com::rdk::hal::State::CLOSED);

            // prerequisite - checking capabilities
            ::com::rdk::hal::audiodecoder::Capabilities capabilities;
            status = audioDecoder->getCapabilities(&capabilities);
            EXPECT_TRUE(status.isOk());
            validateCapabilities(capabilities);

            sp<IAudioDecoderControllerListener> listener = new AudioDecoderControllerListener();
            status = audioDecoder->open(capabilities.supportedCodecs[0], capabilities.supportsSecure, listener, &audioDecoderController);
            ASSERT_TRUE(status.isOk()) << "Failed to open audio decoder, status: " << status.toString8().string();
            ASSERT_NE(audioDecoderController, nullptr) << "audioDecoderController is null after open()";
        }
        void TearDown() override
        {
            L1AudioDecoderTest::TearDown();
        }
        android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderController> audioDecoderController;
};

class L1AudioDecoderEventRegisterTest : public L1AudioDecoderTest
{
    protected:
        void SetUp() override
        {
            L1AudioDecoderTest::SetUp();
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

class L1AudioDecoderPropertyTest : public L1AudioDecoderTest
{
    protected:
        std::map<::com::rdk::hal::audiodecoder::Property, ::com::rdk::hal::PropertyValue> propertyMap;
        void SetUp() override
        {
            L1AudioDecoderTest::SetUp();
            for (auto property :
            {
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
                if (status.isOk() && propertyValue.has_value())
                {
                    propertyMap[property] = propertyValue.value();
                }
                else
                {
                    printf("Failed to get property\n");
                }
            }
        }
        void TearDown() override
        {
            for (const auto& [property, originalValue] : propertyMap)
            {
// FIXME: setProperty() was not declared in IAudioDecoder.aidl
// Comment out to pass compilation
#if 0
                bool _aidl_return;
                android::binder::Status status = audioDecoder->setProperty(property, originalValue, &_aidl_return);
                ASSERT_TRUE(status.isOk()) << "Failed to revert property " << static_cast<int32_t>(property)
                                           << " to its original value. Status: " << status.toString8() << std::endl;
#endif
            }
            L1AudioDecoderTest::TearDown();
        }
};

TEST(AudioDecoderManagerTest, test_l1_audiodecoder_positive_1_getAudioDecoderIds)
{
  std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> decoderIds;
  ::android::binder::Status status = service->getAudioDecoderIds(&decoderIds);
  ASSERT_TRUE(status.isOk());
  ASSERT_FALSE(decoderIds.empty());
}

TEST(AudioDecoderManagerTest, test_l1_audiodecoder_positive_2_getAudioDecoderIds)
{
  std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> initialDecoderIds;
  ::android::binder::Status status = service->getAudioDecoderIds(&initialDecoderIds);
  ASSERT_TRUE(status.isOk());
  ASSERT_FALSE(initialDecoderIds.empty());

  std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id> secondDecoderIds;
  status = service->getAudioDecoderIds(&secondDecoderIds);
  ASSERT_TRUE(status.isOk());

  ASSERT_FALSE(secondDecoderIds.empty());
  ASSERT_EQ(initialDecoderIds.size(), secondDecoderIds.size());
  for (size_t i = 0; i < initialDecoderIds.size(); ++i)
  {
      ASSERT_EQ(initialDecoderIds[i].value, secondDecoderIds[i].value);
  }
}

TEST(AudioDecoderManagerTest, test_l1_audiodecoder_positive_getAudioDecoder)
{
    ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder> decoder;
    ::com::rdk::hal::audiodecoder::IAudioDecoder::Id id ;
    id.value = 0;
    Status status = service->getAudioDecoder(id, &decoder);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(validateAudioDecoder(decoder));
}

TEST(AudioDecoderManagerTest, test_l1_audiodecoder_negative_getAudioDecoder)
{
    ::android::sp<::com::rdk::hal::audiodecoder::IAudioDecoder> decoder;
    ::com::rdk::hal::audiodecoder::IAudioDecoder::Id id ;
    id.value = -2;
    Status status = service->getAudioDecoder(id, &decoder);
    ASSERT_FALSE(status.isOk());
}

TEST_F(L1AudioDecoderTest, test_l1_audiodecoder_positive_getCapabilities)
{
    ::com::rdk::hal::audiodecoder::Capabilities capabilities;
    ::android::binder::Status status = audioDecoder->getCapabilities(&capabilities);
    EXPECT_TRUE(status.isOk());
    validateCapabilities(capabilities);
}

TEST_F(L1AudioDecoderTest, test_l1_audiodecoder_positive_1_getProperty)
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
    EXPECT_TRUE(validatePropertyValue(propertyValue, property))
    << "Property value validation failed for property: " << static_cast<int32_t>(property);
    }
}

TEST_F(L1AudioDecoderTest, test_l1_audiodecoder_negative_1_getProperty)
{
    std::optional<::com::rdk::hal::PropertyValue> propertyValue;
    Status status = audioDecoder->getProperty(static_cast<::com::rdk::hal::audiodecoder::Property>(9999), &propertyValue);
    EXPECT_FALSE(status.isOk());
}

// FIXME: setProperty() was not declared in IAudioDecoder.aidl
// Comment out to pass compilation
#if 0
TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_positive_1_setProperty)
{
    Property property = Property::LOW_LATENCY_MODE;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}

TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_positive_2_setProperty)
{
    Property property = Property::LOW_LATENCY_MODE;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(0);
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}

TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_positive_3_setProperty) {
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
        android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }
}

TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_positive_4_setProperty)
{
    Property property = Property::AC4_PRESENTATION_GROUP_INDEX;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(0);
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}

TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_positive_5_setProperty)
{
    Property property = Property::AC4_PRESENTATION_GROUP_INDEX;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(-1);
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}

TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_positive_6_setProperty)
{
    Property property = Property::AC4_PREFERRED_LANG1;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::stringValue>(u"eng");
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}

TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_positive_7_setProperty)
{
    Property property = Property::AC4_PREFERRED_LANG1;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::stringValue>(u"");
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}


TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_positive_8_setProperty)
{
    Property property = Property::AC4_PREFERRED_LANG2;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::stringValue>(u"spa");
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}

TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_positive_9_setProperty)
{
    Property property = Property::AC4_PREFERRED_LANG2;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::stringValue>(u"");
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}

TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_positive_10_setProperty)
{
    Property property = Property::AC4_ASSOCIATED_TYPE;
    std::vector<int32_t> inputs = { -1, 0, 1, 2 };
    for (const auto& input : inputs)
    {
        PropertyValue propertyValue;
        propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(input);
        bool _aidl_return;
        android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }
}
TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_positive_11_setProperty)
{
    Property property = Property::AC4_AUTO_SELECTION_PRIORITY;
    std::vector<int32_t> inputs = { -1, 0, 1 };
    for (const auto& input : inputs)
    {
        PropertyValue propertyValue;
        propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(input);
        bool _aidl_return;
        android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }
}
TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_positive_12_setProperty)
{
    Property property = Property::AC4_MIXER_BALANCE;
    std::vector<int32_t> inputs = { 255, -32, 32 };

    for (const auto& input : inputs)
    {
        PropertyValue propertyValue;
        propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(input);
        bool _aidl_return;
        android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }
}
TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_positive_13_setProperty)
{
    Property property = Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE;
    std::vector<int32_t> inputs = { -1, 0, 1 };
    for (const auto& input : inputs) 
    {
        PropertyValue propertyValue;
        propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(input);
        bool _aidl_return;
        android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(_aidl_return);
    }
}
TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_negative_1_setProperty)
{
    Property property = static_cast<Property>(9999);
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(12345);
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_FALSE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_negative_2_setProperty)
{
    Property property = Property::AC4_ASSOCIATED_TYPE;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(3);
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_FALSE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_negative_3_setProperty)
{
    Property property = Property::AC4_AUTO_SELECTION_PRIORITY;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(2);
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_FALSE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_negative_4_setProperty)
{
    Property property = Property::AC4_MIXER_BALANCE;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(33);
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_FALSE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_negative_5_setProperty)
{
    Property property = Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(2);
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_FALSE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_negative_6_setProperty)
{
    Property property = Property::AC4_PRESENTATION_GROUP_INDEX;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(-2);
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_FALSE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

TEST_F(L1AudioDecoderPropertyTest, test_l1_audiodecoder_negative_7_setProperty)
{
    Property property = Property::RESOURCE_ID;
    PropertyValue propertyValue;
    propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);
    bool _aidl_return;
    android::binder::Status status = audioDecoder->setProperty(property, propertyValue, &_aidl_return);
    ASSERT_FALSE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}
#endif

TEST_F(L1AudioDecoderTest, test_l1_audiodecoder_positive_1_getState)
{
    ::com::rdk::hal::State state;
    ::android::binder::Status status = audioDecoder->getState(&state);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(state >= ::com::rdk::hal::State::UNKNOWN && state <= ::com::rdk::hal::State::CLOSING);
}

TEST_F(L1AudioDecoderTest, test_l1_audiodecoder_positive_1_open)
{
    //prerequisite - checking state
    ::com::rdk::hal::State state;
    ::android::binder::Status status = audioDecoder->getState(&state);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(state, ::com::rdk::hal::State::CLOSED);

    // prerequisite - checking capabilities
    ::com::rdk::hal::audiodecoder::Capabilities capabilities;
    status = audioDecoder->getCapabilities(&capabilities);
    EXPECT_TRUE(status.isOk());
    validateCapabilities(capabilities);
    bool _aidl_return;
    android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderController> audioDecoderController;
    sp<IAudioDecoderControllerListener> listener = new AudioDecoderControllerListener();
    status = audioDecoder->open(capabilities.supportedCodecs[0], capabilities.supportsSecure, listener, &audioDecoderController);
    ASSERT_TRUE(status.isOk());
    ASSERT_NE(audioDecoderController, nullptr);
    //cleaning up
    status = audioDecoder->close(audioDecoderController, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}

TEST_F(L1AudioDecoderTest, test_l1_audiodecoder_negative_1_open)
{
    // prerequisite - checking state
    ::com::rdk::hal::State state;
    ::android::binder::Status status = audioDecoder->getState(&state);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(state, ::com::rdk::hal::State::CLOSED);

    // prerequisite - checking capabilities
    ::com::rdk::hal::audiodecoder::Capabilities capabilities;
    status = audioDecoder->getCapabilities(&capabilities);
    EXPECT_TRUE(status.isOk());
    validateCapabilities(capabilities);

    android::sp<::com::rdk::hal::audiodecoder::IAudioDecoderController> audioDecoderController;
    sp<IAudioDecoderControllerListener> listener = new AudioDecoderControllerListener();
    status = audioDecoder->open(static_cast<::com::rdk::hal::audiodecoder::Codec>(-1), capabilities.supportsSecure, listener, &audioDecoderController);
    ASSERT_FALSE(status.isOk());
}

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
    ASSERT_FALSE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

TEST_F(L1AudioDecoderTest, test_l1_audiodecoder_positive_1_registerEventListener )
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

TEST_F(L1AudioDecoderEventRegisterTest, test_l1_audiodecoder_negative_1_registerEventListener )
{
    bool _aidl_return;
    // Attempt to register the listener again (first registration in L1AudioDecoderEventRegisterTest)
    Status status = audioDecoder->registerEventListener(listener, &_aidl_return);
    EXPECT_FALSE(status.isOk());
    EXPECT_FALSE(_aidl_return);
    //cleaning up
    status = audioDecoder->unregisterEventListener(listener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(_aidl_return , true);
}

TEST_F(L1AudioDecoderEventRegisterTest, test_l1_audiodecoder_positive_1_unregisterEventListener)
{
    bool _aidl_return;
    Status status = audioDecoder->unregisterEventListener(listener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(_aidl_return);
}

TEST_F(L1AudioDecoderTest, test_l1_audiodecoder_positive_2_unregisterEventListener)
{
    android::sp<com::rdk::hal::audiodecoder::IAudioDecoderEventListener> listener = new AudioDecoderEventListener();
    bool _aidl_return;
    // Attempt to unregister the listener which is not registered
    Status status = audioDecoder->unregisterEventListener(listener, &_aidl_return);
    ASSERT_TRUE(status.isOk());
    ASSERT_FALSE(_aidl_return);
}

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

TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_1_stop)
{
    Status status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_1_stop)
{
    // Perform initial stop to bring it to READY state
    Status status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
    //calling stop() from READY state
    status = audioDecoderController->stop();
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_1_decodeBuffer)
{
    int64_t nsPresentationTime = 1000000;
    int64_t bufferHandle = -1;
    int32_t trimStartNs = 1000;
    int32_t trimEndNs = 500;
    bool _aidl_return;
    ::android::binder::Status status = audioDecoderController->decodeBuffer(nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, &_aidl_return);
    EXPECT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_ARGUMENT);
    //cleaning up
    status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_1_flush)
{
    bool reset = true;
    ::android::binder::Status status = audioDecoderController->flush(reset);
    ASSERT_TRUE(status.isOk());
    //when flush() called with reset=true state is changed to READY.
}

TEST_F(L1AudioDecoderOpenTest, test_l1_audiodecoder_negative_1_flush)
{
    bool reset = true;
    ::android::binder::Status status = audioDecoderController->flush(reset);
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_1_signalDiscontinuity)
{
    Status status = audioDecoderController->signalDiscontinuity();
    EXPECT_TRUE(status.isOk());
    //cleaning up
    status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

TEST_F(L1AudioDecoderOpenTest, test_l1_audiodecoder_negative_1_signalDiscontinuity)
{
    Status status = audioDecoderController->signalDiscontinuity();
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_positive_signalEOS)
{
    Status status = audioDecoderController->signalEOS();
    EXPECT_TRUE(status.isOk());
    //cleaning up
    status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}

TEST_F(L1AudioDecoderOpenTest, test_l1_audiodecoder_negative_signalEOS)
{
    Status status = audioDecoderController->signalEOS();
    ASSERT_EQ(status.exceptionCode(), Status::EX_ILLEGAL_STATE);
}

// FIXME: CSDAudioFormat::AAC_AUDIO_SPECIFIC_CONFIG is not defined in CSDAudioFormat.aidl
// Comment out to pass compilation
#if 0
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_1_parseCodecSpecificData)
{
    CSDAudioFormat csdAudioFormat = CSDAudioFormat::AAC_AUDIO_SPECIFIC_CONFIG;
    std::vector<uint8_t> codecData = {}; // Empty byte array
    bool _aidl_return;
    Status status = audioDecoderController->parseCodecSpecificData(csdAudioFormat, codecData, &_aidl_return);
    EXPECT_FALSE(status.isOk());
    EXPECT_FALSE(_aidl_return);
    //cleaning up
    status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}
#endif

// FIXME: CSDAudioFormat::ALAC_xxx is not defined in CSDAudioFormat.aidl
// Comment out to pass compilation
#if 0
TEST_F(L1AudioDecoderStartTest, test_l1_audiodecoder_negative_2_parseCodecSpecificData)
{
    CSDAudioFormat csdAudioFormat = CSDAudioFormat::ALAC_xxx;
    std::vector<uint8_t> codecData = {}; // Empty byte array
    bool _aidl_return;
    Status status = audioDecoderController->parseCodecSpecificData(csdAudioFormat, codecData, &_aidl_return);
    EXPECT_FALSE(status.isOk());
    EXPECT_FALSE(_aidl_return);
    //cleaning up
    status = audioDecoderController->stop();
    ASSERT_TRUE(status.isOk());
}
#endif

int main(int argc, char **argv) {
#if 0
    ::testing::InitGoogleTest(&argc, argv);
    getService();
    RUN_ALL_TESTS();
#endif

    UT_init(argc, argv);
    getService();
    UT_run_tests();
    return 0;
}

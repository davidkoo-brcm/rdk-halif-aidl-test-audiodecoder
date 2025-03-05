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
 * @defgroup AUDIO_DECODER_HALTEST_HELPER Audio Decoder HAL Tests helper functions
 * @{
 * @parblock
 *
 * ### Helper functions for VTS tests :
 *
 * Helper functions required for the module across all vendors.
 *
 * **Pre-Conditions:**  None @n
 * **Dependencies:** None @n
 *
 * @endparblock
 *
 */

/**
 * @file vts_helper_functions.cpp
 *
 */

#include "vts_helper_functions.h"
#include "vts_parse_audiodecoder_hfp.h"

void helper_validateCapabilities(sp<IAudioDecoder> audioDecoder, const Capabilities& capabilities)
{
    bool decoderID = false;
    int _resource_id;
    Property property = Property::RESOURCE_ID;
    std::optional<PropertyValue> resource_id;

    ::android::binder::Status status = audioDecoder->getProperty(property, &resource_id);
    _resource_id = resource_id->value->get<PropertyValue::Value::Tag::intValue>();
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
            UT_ASSERT_EQUAL(_decoder.supportedCodecs.size(),capabilities.supportedCodecs.size());
            /* iterating over each supported codec from getCapabilities() api */
            for(const auto& supportedcodec_from_api: capabilities.supportedCodecs)
            {
                auto result = std::find(_decoder.supportedCodecs.begin(),_decoder.supportedCodecs.end(),supportedcodec_from_api);
                UT_ASSERT_NOT_EQUAL(result, _decoder.supportedCodecs.end()) <<"decoder : "<<static_cast<int>(_decoder.id) <<" contain supportedCodec : "<<static_cast<int>(supportedcodec_from_api) <<" which is not matching with the profile"<<std::endl;
            }
            /* ensure that supportsSecure value from  hal feature profile  is equal to the value from getCapabilities() api */
            UT_ASSERT_EQUAL(_decoder.supportsSecure, capabilities.supportsSecure);
        }
    }
    UT_ASSERT_TRUE(decoderID) <<"Audio decoder id : "<<_resource_id <<" is not in profile";
}


bool helper_areElementsUnique(const std::vector<::com::rdk::hal::audiodecoder::IAudioDecoder::Id>& decoderIds)
{
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

/** @} */ // End of AUDIO_DECODER_HALTEST_HELPER
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK
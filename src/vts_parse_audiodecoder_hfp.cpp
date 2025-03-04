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
 * @defgroup AUDIO_DECODER_HALTEST_PARSE_HFP Audio Decoder HAL Tests parse Hardware feature profile
 * @{
 * @parblock
 *
 * ### Parse HFP functions for Audio Decoder :
 *
 * Parse HFP functions required for the module across all vendors.
 *
 * **Pre-Conditions:**  None @n
 * **Dependencies:** None @n
 *
 * @endparblock
 *
 */

/**
 * @file parse_audiodecoder_hfp.cpp
 *
 */

#include "vts_helper_functions.h"
#include "vts_parse_audiodecoder_hfp.h"

std::vector<decoder> decoders;

UT_status_t parseAudioDecoderHFP()
{
    ut_kvp_instance_t *config = ut_kvp_profile_getInstance();
    std::map<std::string , int32_t> componentMap;
    std::string audiodecoderpath = AUDIODECODERMGR_IAUDIODECODER;
    int32_t number_of_audiodecoders;
    int32_t supportedcodec_count;
    decoder temporary;
    if (config == nullptr)
    {
        UT_LOG_ERROR("Failed to parse the HFP");
        return UT_STATUS_FAILURE;
    }

    number_of_audiodecoders = ut_kvp_getListCount(config, audiodecoderpath.c_str());
    if(number_of_audiodecoders <= 0)
    {
        UT_LOG_ERROR("No audiodecoders found in the HFP");
        return UT_STATUS_FAILURE;
    }
    for(int32_t i = 0; i < number_of_audiodecoders; ++i)
    {
        std::string audiodecoderidpath = audiodecoderpath+"."+std::to_string(i) + ".id";
        std::string supportedcodecpath = audiodecoderpath+"."+std::to_string(i) + ".supportedCodecs";
        std::string supportsSecurepath = audiodecoderpath+"."+std::to_string(i) + ".supportsSecure";
        temporary.id = ut_kvp_getUInt32Field(config, audiodecoderidpath.c_str());
        temporary.supportsSecure = ut_kvp_getBoolField(config, supportsSecurepath.c_str());
        supportedcodec_count = ut_kvp_getListCount(config, supportedcodecpath.c_str());
        for(int32_t j = 0; j < supportedcodec_count; ++j)
        {
            char result_kvp[UT_KVP_MAX_ELEMENT_SIZE];
            std::string codecpath = supportedcodecpath+"."+std::to_string(j);
            std::string _codec = (0 == ut_kvp_getStringField(config, codecpath.c_str(), result_kvp, UT_KVP_MAX_ELEMENT_SIZE)) ? std::string(result_kvp): "";
            temporary.supportedCodecs.push_back(helper_StringTocodec(_codec));
        }
        decoders.push_back(temporary);
        temporary.supportedCodecs.clear();
    }
    return UT_STATUS_OK;
}

/** @} */ // End of AUDIO_DECODER_HALTEST_PARSE_HFP
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK
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
 * @defgroup AUDIO_DECODER_HALTEST_PARSE_TESTCONFIG Audio Decoder HAL Tests parse test configuration file
 * @{
 * @parblock
 *
 * ### Parse test configuration functions for Audio Decoder :
 *
 * Parse configuration functions required for the module across all vendors.
 *
 * **Pre-Conditions:**  None @n
 * **Dependencies:** None @n
 *
 * @endparblock
 *
 */

/**
 * @file vts_audiodecoder_parse_testconfig.cpp
 *
 */

#include <filesystem>
#include "vts_audiodecoder_parse_testconfig.h"

void* vts_kvp_create_instance(char *fileName)
{
    ut_kvp_instance_t* instance;
    ut_kvp_status_t result;

    UT_LOG("In %s", __FUNCTION__);

    instance = ut_kvp_createInstance();

    if(!instance)
    {
        UT_LOG_ERROR("Failed to create ut kvp instance");
        return nullptr;
    }

    result = ut_kvp_open(instance, fileName);

    if(result != UT_KVP_STATUS_SUCCESS)
    {
        UT_LOG_ERROR("Failed to open test configuration file");
        ut_kvp_destroyInstance(instance);
        return nullptr;
    }

    UT_LOG("Out  %s", __FUNCTION__);

    return (ut_kvp_instance_t*)instance;
}

void vts_kvp_destroy_instance(void *instance)
{
    ut_kvp_destroyInstance((ut_kvp_instance_t*)instance);
}

UT_status_t vts_testconfig_getStream_details(void* pTestInstance, char *module, char *testGroup, char *testName, std::vector<testStreamConfig_t>& streamDetailsList)
{
    ut_kvp_status_t status;
    ut_kvp_instance_t* kvpTestInstance = (ut_kvp_instance_t*)pTestInstance;
    char streamInputPath[UT_KVP_MAX_ELEMENT_SIZE] = {0};
    char streamRefPath[UT_KVP_MAX_ELEMENT_SIZE] = {0};
    int32_t streamCount = 0;
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path fullPath;
    std::string testList = std::string(module) + "." + std::string(testGroup) + "." + std::string(testName) + "." + std::string("Streams");
    std::string streamsDetails = std::string("AudioStreams");

    status = ut_kvp_getStringField(kvpTestInstance, STREAM_INPUT_PATH, streamInputPath, UT_KVP_MAX_ELEMENT_SIZE);
    status = ut_kvp_getStringField(kvpTestInstance, STREAM_REF_PATH, streamRefPath, UT_KVP_MAX_ELEMENT_SIZE);

    streamCount = ut_kvp_getListCount(kvpTestInstance, testList.c_str());

    for(int32_t i = 0; i < streamCount; i++)
    {
        char resultKVP[UT_KVP_MAX_ELEMENT_SIZE] = {0};
        uint32_t streamId;
        testStreamConfig_t streamDetails;
        std::vector<int32_t> errorFrameNum;

        std::string streamName = testList + "." + std::to_string(i) + "Name";
        ut_kvp_getStringField(kvpTestInstance, streamName.c_str(), resultKVP, UT_KVP_MAX_ELEMENT_SIZE);

        std::string input       = streamsDetails + "." + std::string(resultKVP) + "." +"input";
        std::string reference   = streamsDetails + "." + std::string(resultKVP) + "." +"reference";
        std::string codec       = streamsDetails + "." + std::string(resultKVP) + "." +"codec";
        std::string sampleRate  = streamsDetails + "." + std::string(resultKVP) + "." +"sample_rate";
        std::string channels    = streamsDetails + "." + std::string(resultKVP) + "." +"channels";
        std::string format      = streamsDetails + "." + std::string(resultKVP) + "." +"format";
        std::string numOfFrames = streamsDetails + "." + std::string(resultKVP) + "." +"num_of_frames";
        std::string frameSize   = streamsDetails + "." + std::string(resultKVP) + "." +"frame_size";
        std::string errorFrames = streamsDetails + "." + std::string(resultKVP) + "." +"error_frames";
        std::string framePTS    = testList + "." + std::to_string(i) + "." +"PTS";
        std::string rmsDiff     = testList + "." + std::to_string(i) + "." +"rms_diff_threshold";
        std::string trimStart   = testList + "." + std::to_string(i) + "." +"trim.0";
        std::string trimEnd     = testList + "." + std::to_string(i) + "." +"trim.1";

        ut_kvp_getStringField(kvpTestInstance, input.c_str(), resultKVP, UT_KVP_MAX_ELEMENT_SIZE);
        fullPath = currentPath;
        fullPath.append(streamInputPath);
        fullPath.append(resultKVP);
        streamDetails.input = fullPath.string();

        ut_kvp_getStringField(kvpTestInstance, reference.c_str(), resultKVP, UT_KVP_MAX_ELEMENT_SIZE);
        fullPath = currentPath;
        fullPath.append(streamRefPath);
        fullPath.append(resultKVP);
        streamDetails.reference = fullPath.string();

        ut_kvp_getStringField(kvpTestInstance, codec.c_str(), resultKVP, UT_KVP_MAX_ELEMENT_SIZE);
        streamDetails.codec = vts_audiodecoder_helper_StringTocodec(std::string(resultKVP));

        streamDetails.sampleRate = ut_kvp_getUInt32Field(kvpTestInstance, sampleRate.c_str());
        streamDetails.numChannels = ut_kvp_getUInt32Field(kvpTestInstance, channels.c_str());

        ut_kvp_getStringField(kvpTestInstance, format.c_str(), resultKVP, UT_KVP_MAX_ELEMENT_SIZE);
        streamDetails.format = vts_audiodecoder_helper_StringToFormat(std::string(resultKVP));

        streamDetails.numOfFrames = ut_kvp_getUInt32Field(kvpTestInstance, numOfFrames.c_str());

        if(ut_kvp_fieldPresent(kvpTestInstance, errorFrames.c_str()))
        {
            int32_t count = ut_kvp_getListCount(kvpTestInstance, errorFrames.c_str());
            for (int32_t j = 0; j < count; j++)
            {
                std::string error = errorFrames + "." + std::to_string(j);
                int32_t frameNum = ut_kvp_getUInt32Field(kvpTestInstance, error.c_str());
                errorFrameNum.push_back(frameNum);
            }
        }

        for(int32_t j = 0; j < streamDetails.numOfFrames; j++)
        {
            bool errorFrame = false;

            std::string eachFrame = frameSize + "." + std::to_string(j);
            std::string eachPTS = framePTS + "." + std::to_string(j);
            int32_t size = ut_kvp_getUInt32Field(kvpTestInstance, eachFrame.c_str());
            int64_t PTS = ut_kvp_getUInt64Field(kvpTestInstance, eachPTS.c_str());
            if (std::find(errorFrameNum.begin(), errorFrameNum.end(), j+1) != errorFrameNum.end())
            {
                errorFrame = true;
            }
            streamDetails.frameDetails.push_back({size, PTS, errorFrame});
        }

        streamDetails.rms_diff_threashold = ut_kvp_getDoubleField(kvpTestInstance, rmsDiff.c_str());

        streamDetails.trimStartNs = ut_kvp_getUInt64Field(kvpTestInstance, trimStart.c_str());
        streamDetails.trimEndNs = ut_kvp_getUInt64Field(kvpTestInstance, trimEnd.c_str());

        streamDetailsList.push_back(streamDetails);
    }

    return UT_STATUS_OK;
}

/** @} */ // End of AUDIO_DECODER_HALTEST_PARSE_TESTCONFIG
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK
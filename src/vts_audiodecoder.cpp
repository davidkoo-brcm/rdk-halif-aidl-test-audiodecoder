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
 * @defgroup AUDIO_DECODER_HALTEST_CLASS Audio Decoder HAL Tests helper functions Header File
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
 * @file vts_audiodecoder.cpp
 *
 */

#include <fstream>
#include <ut.h>
#include <ut_log.h>
#include <ut_kvp_profile.h>
#include <ut_control_plane.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <binder/Status.h>
#include <sys/syscall.h>
#include <utils/String16.h>
#include <stdio.h>
#include <vector>
#include <optional>
#include <algorithm>
#include <unordered_set>
#include <tuple>
#include <map>
#include <chrono>


#include "com/rdk/hal/avbuffer/IAVBuffer.h"
#include "com/rdk/hal/avbuffer/BnAVBufferSpaceListener.h"

#include "vts_audiodecoder_helper_functions.h"
#include "vts_audiodecoder_parse_hfp.h"
#include "vts_audiodecoder_parse_testconfig.h"
#include "vts_audiodecoder.h"

#include "IAVBufferHelper.h"

using namespace std::chrono;
using namespace std;

#define MAX_WAIT_TIME 500

/**
 * Convert vector to comma-separated string
 *
 * @param channels Channel information vector
 * @return comma-separated string
 */
string AudioDecoderControllerListener::formatChannelVector(const std::vector<ChannelType>& channels)
{
    std::ostringstream oss;
    for (size_t i = 0; i < channels.size(); ++i) {
        oss << audiodecoder::toString(channels[i]);
        if (i != channels.size() - 1) oss << ", ";
    }
    return oss.str();
}

/**
 * Override Call-back funtion for decoder output
 *
 * @param nsPresentationTime PTS in nanoseconds
 * @param frameBufferHandle private decoded buffer handle
 * @param metadata stream metadata
 * @return Status
 */
Status AudioDecoderControllerListener::onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::optional<FrameMetadata>& metadata)
{
    UT_LOG_INFO("[DecoderID: %d] onFrameOutput: Received frameoutput [PTS:%lld, handle: %llX]", mDecoderID, nsPresentationTime, frameBufferHandle);

    DecodedAudioFrame_t frame;
    frame.receiveTime = chrono::steady_clock::now();
    frame.framePTS = nsPresentationTime;

    frame.isTunnel = false;
    if (frameBufferHandle == -1)
    {
        frame.isTunnel = true;
    }
    frame.isMetaAvailable = false;
    if (metadata.has_value())
    {
        UT_LOG_INFO("[DecoderID: %d] Stream Details [Codec: %s, SampleRate: %d, Channels: %d, channelType: [%s] format: %s planarFormat: %s, isDolbyAtom: %s]",
                    mDecoderID, audiodecoder::toString(metadata->sourceCodec).c_str(), metadata->sampleRate, metadata->numChannels,
                    formatChannelVector(metadata->channelTypes).c_str(), audiodecoder::toString(metadata->format).c_str(),
                    UT_Control_GetMapString(bool_mapTable, metadata->planarFormat), UT_Control_GetMapString(bool_mapTable, metadata->isDolbyAtmos));
        UT_LOG_INFO("[DecoderID: %d] Control Parameters [PTS: %lld, discontinuity: %d, EOS: %d, LowLatency: %d, Source: %d, Trim(%lld-%lld)]",
                    mDecoderID, nsPresentationTime, metadata->discontinuity, metadata->endOfStream, metadata->lowLatency,
                    metadata->source, metadata->trimStartNs, metadata->trimEndNs);

        frame.isMetaAvailable = true;
        frame.metadata.sourceCodec = metadata->sourceCodec;
        frame.metadata.isDolbyAtmos = metadata->isDolbyAtmos;
        frame.metadata.numChannels = metadata->numChannels;
        frame.metadata.channelTypes = metadata->channelTypes;
        frame.metadata.sampleRate = metadata->sampleRate;
        frame.metadata.format = metadata->format;
        frame.metadata.planarFormat = metadata->planarFormat;
        frame.metadata.trimStartNs = metadata->trimStartNs;
        frame.metadata.trimEndNs = metadata->trimEndNs;
        frame.metadata.lowLatency = metadata->lowLatency;
        frame.metadata.endOfStream = metadata->endOfStream;
        frame.metadata.discontinuity = metadata->discontinuity;
        frame.metadata.source = metadata->source;
    }
    {
        lock_guard<mutex> lock(mQueueMutex);
        mFrameQueue.push(frame);
    }

    mFrameAvailable.notify_one();

    return ::android::binder::Status::ok();
}

/**
 * Returns the queued frames from onFrameOutput
 *
 * @param frameInfo frame information
 * @param popFrame if true function reads and pop the frame from queue
 *                 otherwise function just reads the frame info
 * @return True: if frame reads successfull, False: if frame is not available
 */
bool AudioDecoderControllerListener::getNextFrame(DecodedAudioFrame_t &frameInfo, bool popFrame)
{
    DecodedAudioFrame_t frame;
    int32_t count = 0;
    //TODO: change waiting mechanism
    while (mFrameQueue.empty())
    {
        //mFrameAvailable.wait(lock);
        this_thread::sleep_for(chrono::milliseconds(1));
        count++;
        if(count > MAX_WAIT_TIME)
        {
            UT_LOG_ERROR("Decoded frame not received after %d msec", MAX_WAIT_TIME);
            return false;
        }
    }
    {
        unique_lock<std::mutex> lock(mQueueMutex);
        frame = mFrameQueue.front();
        if (popFrame)
            mFrameQueue.pop();
    }

    frameInfo = frame;
    return true;
}

/**
 * Clears flags and notifies all mutexes
 *
 * @param None
 * @return None
 */
void AudioDecoderControllerListener::shutdown()
{
    mShutdownFlag = true;
    mFrameAvailable.notify_all();
}

/**
 * Sets the Decoder ID
 *
 * @param id: Decoder ID
 * @return None
 */
void AudioDecoderControllerListener::setDecoderID(int32_t id)
{
    mDecoderID = id;
}

/**
 * Override Call-back funtion for decoder errors
 *
 * @param vendorErrorCode vendor error code
 * @param errorCode General error code
 * @return Status
 */
Status AudioDecoderEventListener::onDecodeError(ErrorCode vendorErrorCode, int32_t errorCode)
{
    ErrorDetails_t errorDetails;
    errorDetails.receiveTime = chrono::steady_clock::now();

    UT_LOG_INFO("[DecoderID: %d] Error Callback: Vendor Error Code: %s, errorCode: %d", mDecoderID, audiodecoder::toString(vendorErrorCode).c_str(), errorCode);

    errorDetails.errorCode = errorCode;
    errorDetails.vendorErrorCode = vendorErrorCode;

    lock_guard<mutex> lock(mErrorMutex);
    mErrors.push(errorDetails);
    return Status::ok();
}

/**
 * Override Call-back funtion for decoder state changes
 *
 * @param oldState Old state
 * @param newState New state
 * @return Status
 */
Status AudioDecoderEventListener::onStateChanged(State oldState,State newState)
{
    UT_LOG_INFO("[DecoderID: %d] State Change received: %s -> %s", mDecoderID, com::rdk::hal::toString(oldState).c_str(), com::rdk::hal::toString(newState).c_str());
    {
        lock_guard<mutex> lock(mMutex);
        mStateChanges.emplace_back(oldState, newState);
        mCurrentState = newState;
    }
    mCV.notify_all(); // Unblock any waiting test code
    return Status::ok();
}

/**
 * Returns the queued eror info from onDecodeError
 *
 * @param errorDetails error information
 * @return True: if frame reads successfull, False: if frame is not available
 */
bool AudioDecoderEventListener::getNextError(ErrorDetails_t &errorDetails)
{
    int32_t count = 0;
    ErrorDetails_t err;
    while (mErrors.empty())
    {
        //mFrameAvailable.wait(lock);
        this_thread::sleep_for(chrono::milliseconds(1));
        count++;
        if(count > MAX_WAIT_TIME)
        {
            UT_LOG_ERROR("Error event not received after %d msec", MAX_WAIT_TIME);
            return false;
        }
    }
    {
        unique_lock<std::mutex> lock(mErrorMutex);
        err = mErrors.front();
        mErrors.pop();
    }

    errorDetails = err;
    return true;
}

/**
 * Wait for the decoder to reach a particular State within a given timeout.
 *
 * @param desiredState The state we want to observe.
 * @param timeout      How long to wait, e.g. chrono::milliseconds(500).
 * @return true if the state was reached; false if we timed out waiting.
 */
bool AudioDecoderEventListener::waitForState(State desiredState, chrono::milliseconds timeout)
{
    unique_lock<mutex> lock(mMutex);
    // Wait until mCurrentState == desiredState or until timeout
    return mCV.wait_for(lock, timeout, [this, desiredState]() {
        return (mCurrentState == desiredState);
    });
}


/**
 * Gets the state changes received from onStateChanged.
 *
 * @param None.
 * @return vector of old and new states
 */
vector<pair<State, State>> AudioDecoderEventListener::getStateChanges() const
{
    lock_guard<mutex> lock(mMutex);
    return mStateChanges;
}

/**
 * Clears the state queue
 *
 * @param None.
 * @return None
 */
void AudioDecoderEventListener::resetStateChanges()
{
    lock_guard<mutex> lock(mMutex);
    mStateChanges.clear();
}

/**
 * Checks if the expected states received from the call-back
 *
 * @param expOldState Expected old state
 * @param expNewState Expected new state
 * @return returns if the expected states received or not
 */

bool AudioDecoderEventListener::checkStateTransistion(State expOldState, State expNewState)
{
    bool foundClosedToOpening = false;
    for (const auto &[oldState, newState] : mStateChanges)
    {
        if (oldState == expOldState && newState == expNewState)
        {
            foundClosedToOpening = true;
            break;
        }
    }
    return foundClosedToOpening;
}

/**
 * Clears the error queue
 *
 * @param None.
 * @return None
 */
void AudioDecoderEventListener::resetErrors()
{
    lock_guard<mutex> lock(mErrorMutex);
    while (!mErrors.empty())
    {
        mErrors.pop();
    }
}

/**
 * Sets the Decoder ID
 *
 * @param id: Decoder ID
 * @return None
 */
void AudioDecoderEventListener::setDecoderID(int32_t id)
{
    mDecoderID = id;
}

/** @} */ // End of AUDIO_DECODER_HALTEST_HEADER
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK
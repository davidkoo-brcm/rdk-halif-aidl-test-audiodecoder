#!/bin/bash
#* ******************************************************************************
#*  If not stated otherwise in this file or this component's LICENSE
#*  file the following copyright and licenses apply:
#*
#*  Copyright 2025 RDK Management
#*
#*  Licensed under the Apache License, Version 2.0 (the License);
#*  you may not use this file except in compliance with the License.
#*  You may obtain a copy of the License at
#*
#*  http://www.apache.org/licenses/LICENSE-2.0
#*
#*  Unless required by applicable law or agreed to in writing, software
#*  distributed under the License is distributed on an AS IS BASIS,
#*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#*  See the License for the specific language governing permissions and
#*  limitations under the License.
#*
#* ******************************************************************************

DEV_BINDER='/dev/binder'
RUN_DIR=`realpath $(dirname $0)`

# Function to check if a process is running
check_process() {
    local process_name=$1
    pgrep -f "$process_name" > /dev/null
    return $?
}

# Function to prompt user with yes/no question
prompt_yes_no() {
    local prompt=$1
    while true; do
        read -p "$prompt (y/n): " response
        case $response in
            [Yy]* ) return 0;;
            [Nn]* ) return 1;;
            * ) echo "Please answer y or n";;
        esac
    done
}

if [ ! -w $DEV_BINDER ]; then
    echo "No permission to access $DEV_BINDER, consider execute this as root"
    exit 1
fi

export LD_LIBRARY_PATH=$RUN_DIR:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/lib:/lib:/home/root:./

# Check and start servicemanager (must run if not running)
if check_process "servicemanager"; then
    echo "servicemanager is already running"
    servicemanager_pid=$(pgrep -f "servicemanager")
else
    echo "Execute [servicemanager]"
    ${RUN_DIR}/servicemanager &
    servicemanager_pid=$!
    # Wait for service to be ready
	sleep 2
fi

# Check if either EmulateAudioDecoderService or AudioDecoderService is running
if check_process "EmulateAudioDecoderService" || check_process "AudioDecoderService"; then
    echo "Audio decoder service is already running"
    if check_process "EmulateAudioDecoderService"; then
        AudioDecoderService_pid=$(pgrep -f "EmulateAudioDecoderService")
    else
        AudioDecoderService_pid=$(pgrep -f "AudioDecoderService")
    fi
else
    # Prompt user to start EmulateAudioDecoderService
    if prompt_yes_no "Do you want to start EmulateAudioDecoderService?"; then
        echo "Execute [EmulateAudioDecoderService]"
        ${RUN_DIR}/EmulateAudioDecoderService &
        AudioDecoderService_pid=$!
        # Wait for service to be ready
        sleep 2
    else
        # Prompt user to start AudioDecoderService
        if prompt_yes_no "Do you want to start AudioDecoderService instead?"; then
            echo "Execute [AudioDecoderService]"
            AudioDecoderService &
            AudioDecoderService_pid=$!
            # Wait for service to be ready
            sleep 2
        else
            echo "Cannot proceed without an audio decoder service"
            # Clean up servicemanager
            if [ ! -z "$servicemanager_pid" ]; then
                kill -9 $servicemanager_pid
            fi
            exit 1
        fi
    fi
fi
echo "Eexecute [vts_audiodecoder_hal_test]"
${RUN_DIR}/vts_audiodecoder_hal_test $@
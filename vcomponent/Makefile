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

.PHONY: all


AUDIODECODER_AIDL_LIB_NAME := audiodecoderaidl

AUDIODECODER_AIDL_LIB := lib$(AUDIODECODER_AIDL_LIB_NAME).so

EMULATE_SERVICE_BIN := EmulateAudioDecoderService

BUILD_DIR ?= $(CURDIR)/../build

AIDL_GEN_DIR ?= $(BUILD_DIR)/audiodecoder_gen/current

EMULATE_SERVICE_DIR ?= $(CURDIR)/src

AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/AVSource.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/HALError.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/PropertyValue.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/State.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/Capabilities.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/ChannelType.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/Codec.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/CSDAudioFormat.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/ErrorCode.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/FrameMetadata.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/IAudioDecoderController.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/IAudioDecoderControllerListener.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/IAudioDecoder.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/IAudioDecoderEventListener.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/IAudioDecoderManager.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/PCMFormat.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/Property.cpp

EMULATE_SERVICE_BIN_SRC := $(EMULATE_SERVICE_DIR)/AudioDecoderService.cpp
EMULATE_SERVICE_BIN_SRC += $(EMULATE_SERVICE_DIR)/AudioDecoder.cpp

EMULATE_SERVICE_INC_FLAGS := -I$(AIDL_GEN_DIR)/h
EMULATE_SERVICE_INC_FLAGS += -I$(BUILD_DIR)/usr/include

EMULATE_SERVICE_LD_LIBS := -lbinder -lutils

EMULATE_SERVICE_LDFLAGS := -L$(BUILD_DIR)/usr/lib
EMULATE_SERVICE_LDFLAGS += -Wl,-rpath-link=$(BUILD_DIR)/usr/lib

EMULATE_SERVICE_CFLAGS := -Wall -Wextra -std=c++17 -ggdb

AUDIODECODER_AIDL_LIB := $(BUILD_DIR)/usr/lib/$(AUDIODECODER_AIDL_LIB)
EMULATE_SERVICE_BIN := $(BUILD_DIR)/usr/lib/$(EMULATE_SERVICE_BIN)

all: $(EMULATE_SERVICE_BIN) $(AUDIODECODER_AIDL_LIB)

$(AUDIODECODER_AIDL_LIB): $(AUDIODECODER_AIDL_SRC:%.cpp=%.o)
	g++ -shared $(EMULATE_SERVICE_LDFLAGS) $^ -o $@ $(EMULATE_SERVICE_LD_LIBS)

$(EMULATE_SERVICE_BIN): $(AUDIODECODER_AIDL_LIB)
$(EMULATE_SERVICE_BIN): $(EMULATE_SERVICE_BIN_SRC:%.cpp=%.o)
	g++ $(EMULATE_SERVICE_LDFLAGS) \
		$(EMULATE_SERVICE_BIN_SRC:%.cpp=%.o) -o $@ \
		$(EMULATE_SERVICE_LD_LIBS) \
		-lpthread \
		-l$(AUDIODECODER_AIDL_LIB_NAME)

%.o: %.cpp
	g++ $(EMULATE_SERVICE_CFLAGS) $(EMULATE_SERVICE_INC_FLAGS) -fPIC -c $< -o $@

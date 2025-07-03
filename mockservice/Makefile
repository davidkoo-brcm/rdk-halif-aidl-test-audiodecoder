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

AVBUFFER_AIDL_LIB_NAME := avbufferaidl
VIDEODECODER_AIDL_LIB_NAME := videodecoderaidl
AUDIODECODER_AIDL_LIB_NAME := audiodecoderaidl
AVBUFFER_HELPER_LIB_NAME := avbufferhelper

AVBUFFER_AIDL_LIB := lib$(AVBUFFER_AIDL_LIB_NAME).so
VIDEODECODER_AIDL_LIB := lib$(VIDEODECODER_AIDL_LIB_NAME).so
AUDIODECODER_AIDL_LIB := lib$(AUDIODECODER_AIDL_LIB_NAME).so
AVBUFFERHELPER_LIB := lib$(AVBUFFER_HELPER_LIB_NAME).so

MOCK_SERVICE_BIN := MockAudioDecoderService

BUILD_DIR ?= $(CURDIR)/../build
AIDL_GEN_DIR ?= $(BUILD_DIR)/audiodecoder_gen/current
AV_BUFFER_AIDL_GEN_DIR ?= $(BUILD_DIR)/avbuffer_gen/current
VIDEO_DECODER_AIDL_GEN_DIR ?= $(BUILD_DIR)/videodecoder_gen/current

MOCK_SERVICE_DIR ?= $(CURDIR)/src

MOCK_SERVICE_BIN_SRC := $(MOCK_SERVICE_DIR)/AudioDecoderService.cpp
MOCK_SERVICE_BIN_SRC += $(MOCK_SERVICE_DIR)/AudioDecoder.cpp

MOCK_SERVICE_INC_FLAGS := -I$(AIDL_GEN_DIR)/h
MOCK_SERVICE_INC_FLAGS += -I$(AV_BUFFER_AIDL_GEN_DIR)/h
MOCK_SERVICE_INC_FLAGS += -I$(VIDEO_DECODER_AIDL_GEN_DIR)/h
MOCK_SERVICE_INC_FLAGS += -I$(BUILD_DIR)/usr/include

MOCK_SERVICE_LD_LIBS := -lbinder -lutils

MOCK_SERVICE_LDFLAGS := -L$(BUILD_DIR)/usr/lib
MOCK_SERVICE_LDFLAGS += -Wl,-rpath-link=$(BUILD_DIR)/usr/lib

MOCK_SERVICE_CFLAGS := -Wall -Wextra -std=c++17 -ggdb

AVBUFFER_AIDL_LIB := $(BUILD_DIR)/usr/lib/$(AVBUFFER_AIDL_LIB)
AUDIODECODER_AIDL_LIB := $(BUILD_DIR)/usr/lib/$(AUDIODECODER_AIDL_LIB)
VIDEODECODER_AIDL_LIB := $(BUILD_DIR)/usr/lib/$(VIDEODECODER_AIDL_LIB)
MOCK_SERVICE_BIN := $(BUILD_DIR)/usr/lib/$(MOCK_SERVICE_BIN)

all: $(MOCK_SERVICE_BIN)

$(MOCK_SERVICE_BIN): $(MOCK_SERVICE_BIN_SRC:%.cpp=%.o)
	g++ $(MOCK_SERVICE_LDFLAGS) \
		$(MOCK_SERVICE_BIN_SRC:%.cpp=%.o) -o $@ \
		$(MOCK_SERVICE_LD_LIBS) \
		-lpthread \
		-l$(AUDIODECODER_AIDL_LIB_NAME)

%.o: %.cpp
	g++ $(MOCK_SERVICE_CFLAGS) $(MOCK_SERVICE_INC_FLAGS) -fPIC -c $< -o $@

cmake_minimum_required(VERSION 3.8)

project(MockTestAudioDecoderService VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT DEFINED LINUX_BINDER_OUT_DIR)
	message(FATAL_ERROR "LINUX_BINDER_OUT_DIR is not defined")
endif()

set(LINUX_BINDER_LIB_DIR "${LINUX_BINDER_OUT_DIR}/lib/")
message(STATUS "LINUX_BINDER_LIB_DIR set to ${LINUX_BINDER_LIB_DIR}")

set(LINUX_BINDER_INCLUDE_DIR "${LINUX_BINDER_OUT_DIR}/include/")
message(STATUS "LINUX_BINDER_INCLUDE_DIR set to ${LINUX_BINDER_INCLUDE_DIR}")

if (NOT DEFINED AUDIODECODER_AIDL_GEN_DIR)
	message(FATAL_ERROR "AUDIODECODER_AIDL_GEN_DIR is not defined")
endif()

set(AUDIODECODER_GEN_SRC_DIR ${AUDIODECODER_AIDL_GEN_DIR}/cpp/com/rdk/hal)
message(STATUS "AUDIODECODER_GEN_SRC_DIR set to ${AUDIODECODER_GEN_SRC_DIR}")


set(AUDIODECODER_GEN_SRC
	${AUDIODECODER_GEN_SRC_DIR}/AVSource.cpp
	${AUDIODECODER_GEN_SRC_DIR}/HALError.cpp
	${AUDIODECODER_GEN_SRC_DIR}/PropertyValue.cpp
	${AUDIODECODER_GEN_SRC_DIR}/State.cpp
	${AUDIODECODER_GEN_SRC_DIR}/audiodecoder/Capabilities.cpp
	${AUDIODECODER_GEN_SRC_DIR}/audiodecoder/ChannelType.cpp
	${AUDIODECODER_GEN_SRC_DIR}/audiodecoder/Codec.cpp
	${AUDIODECODER_GEN_SRC_DIR}/audiodecoder/CSDAudioFormat.cpp
	${AUDIODECODER_GEN_SRC_DIR}/audiodecoder/ErrorCode.cpp
	${AUDIODECODER_GEN_SRC_DIR}/audiodecoder/FrameMetadata.cpp
	${AUDIODECODER_GEN_SRC_DIR}/audiodecoder/IAudioDecoderController.cpp
	${AUDIODECODER_GEN_SRC_DIR}/audiodecoder/IAudioDecoderControllerListener.cpp
	${AUDIODECODER_GEN_SRC_DIR}/audiodecoder/IAudioDecoder.cpp
	${AUDIODECODER_GEN_SRC_DIR}/audiodecoder/IAudioDecoderEventListener.cpp
	${AUDIODECODER_GEN_SRC_DIR}/audiodecoder/IAudioDecoderManager.cpp
	${AUDIODECODER_GEN_SRC_DIR}/audiodecoder/PCMFormat.cpp
	${AUDIODECODER_GEN_SRC_DIR}/audiodecoder/Property.cpp
)

find_library(BINDER_LIB binder PATHS ${LINUX_BINDER_LIB_DIR})
find_library(UTILS_LIB utils PATHS ${LINUX_BINDER_LIB_DIR})

add_library(MockTestAudioDecoder SHARED
	${AUDIODECODER_GEN_SRC}
	AudioDecoder.cpp
)
target_include_directories(MockTestAudioDecoder PRIVATE
	${LINUX_BINDER_INCLUDE_DIR}
	${AUDIODECODER_AIDL_GEN_DIR}/h
)
target_link_libraries(MockTestAudioDecoder PRIVATE
	${BINDER_LIB}
	${UTILS_LIB}
)

add_executable(MockTestAudioDecoderService
	AudioDecoderService.cpp
)
target_include_directories(MockTestAudioDecoderService PRIVATE
	${LINUX_BINDER_INCLUDE_DIR}
	${AUDIODECODER_AIDL_GEN_DIR}/h
)
target_link_libraries(MockTestAudioDecoderService PRIVATE
	${BINDER_LIB}
	${UTILS_LIB}
	MockTestAudioDecoder
)

install(TARGETS
	MockTestAudioDecoder
	MockTestAudioDecoderService
)

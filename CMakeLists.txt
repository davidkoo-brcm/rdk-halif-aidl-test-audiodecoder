cmake_minimum_required(VERSION 3.8)

project(MockTestAudioDecocer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT DEFINED LINUX_BINDER_OUT_DIR)
	message(FATAL_ERROR "LINUX_BINDER_OUT_DIR is not defined")
endif()

if (NOT DEFINED AUDIODECODER_AIDL_GEN_DIR)
	message(FATAL_ERROR "AUDIODECODER_AIDL_GEN_DIR is not defined")
endif()

if (NOT DEFINED GTEST_DIR)
	message(STATUS "GTEST_DIR is not defined")
endif()

add_subdirectory(service)
add_subdirectory(client)

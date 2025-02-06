# Audio Decoder High Level Test Specification Document

## Table of Contents

- [Acronyms, Terms and Abbreviations](#acronyms-terms-and-abbreviations)
- [Introduction](#introduction)
- [Module Description](#module-description)
- [L2 Test Specification](#l2-test-specification)
- [L3 Test Specification](#l3-test-specification)

## Acronyms, Terms and Abbreviations

- `HAL`    - Hardware Abstraction Layer
- `API`    - Application Programming Interface
- `AIDL`   - Android Interface Definition Language
- `L2`     - Level 2 Testing
- `L3`     - Level 3 Testing
- `NA`     - Not Applicable
- `Y`      - Yes
- `Caller` - Any user of the interface via the `APIs`
- `CB`     - Call-back function (suffix)
- `MS12`   - MultiStream 12
- `PCM`    - Pulse Code Modulation
- `AC3`    - Audio Codec 3
- `EAC3`   - Enhanced `AC3`
- `WMA`    - Windows Media Audio
- `AAC`    - Advanced Audio coding
- `DD`     - DOLBY Digital
- `DDPLUS` - DOLBY Digital Plus
- `RDK`    - Reference Design Kit

## Introduction

This document provides an overview of the testing requirements for the Audio Decoder module. It outlines the scope of testing, objectives, testing levels and approaches, specific test requirements, and expected deliverables.

Audio Decoder Interface Definitions - [Audio Decoder HAL](https://github.com/rdkcentral/rdk-halif-aidl/tree/v0.6/audiodecoder/current/com/rdk/hal/audiodecoder)

## Module Description

The Audio Decoder `HAL` service provides the interfaces for compressed audio to be passed to the vendor layer for decoding.  It may be passed secure buffers if it indicates support for secure audio processing.

The output of an audio decoder can take 2 routes; non-tunnelled it comes back to the `RDK` media pipeline as a `PCM` audio frame buffer with metadata or tunnelled through the vendor layer directly to the mixer

## L2 Test Specification

|#|Test Functionality|Description|
|-|------------------|-----------|
|1|[Set and Get Decoder Properties](#set-and-get-decoder-properties)|Set various properties of decoder and verifies by get function|
|2|[Test2](#test2)|Test2|
|3|[Test3](#test3)|Test3|

### 1. Set and Get Decoder Properties

#### Description

- Read supported audio decoders using `IAudioDecoderManager` class
- Loop through supported audio decoders
- set read-write properties (`Property`) of decoder and verify using get function
- get read only properties of decoder and verify with default/supported values

#### HAL APIs

|Class|Function|
|-----|--------|
|IAudioDecoderManager|getAudioDecoderIds, getAudioDecoder|
|IAudioDecoder|open, getProperty, close|
|IAudioDecoderController|setProperty|

### 2. Test2

#### Description

#### HAL APIs

|Class|Function|
|-----|--------|

### 3. Test3

#### Description

#### HAL APIs

|Class|Function|
|-----|--------|

## L3 Test Specification

|#|Test Functionality|Description|
|-|------------------|-----------|
|1|[Decode Single frame of Audio](#decode-single-frame-of-audio)|Decodes the single audio frame of various formats|
|2|[Test2](#test2-1)|Test2|
|3|[Test3](#test3-1)|Test3|

### 1. Decode Single frame of Audio

#### Description

- Read supported audio decoders using `IAudioDecoderManager` class
- Loop through supported audio decoders
- Loop through supported audio formats
- Provide single audio frame to decoder in non-tunnel module
- Verify the decoded frame

#### HAL APIs

|Class|Function|
|-----|--------|
|IAudioDecoderManager|getAudioDecoderIds, getAudioDecoder|
|IAudioDecoder|open, registerEventListener, close|
|IAudioDecoderController|start, decodeBuffer, stop|
|IAudioDecoderControllerListener|onFrameOutput|
|IAudioDecoderEventListener|onDecodeError|
|IAVBuffer|createAudioPool, alloc, free, destroyPool|

### 2. Test2

#### Description

#### HAL APIs

|Class|Function|
|-----|--------|

### 3. Test3

#### Description

#### HAL APIs

|Class|Function|
|-----|--------|
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
- `EOS`    - End of Stream

## Introduction

This document provides an overview of the testing requirements for the Audio Decoder module. It outlines the scope of testing, objectives, testing levels and approaches, specific test requirements.

Audio Decoder Interface Definitions - [Audio Decoder HAL](https://github.com/rdkcentral/rdk-halif-aidl/tree/v0.6/audiodecoder/current/com/rdk/hal/audiodecoder)

## Module Description

The Audio Decoder `HAL` service provides the interfaces for compressed audio to be passed to the vendor layer for decoding.  It may be passed secure buffers if it indicates support for secure audio processing.

The output of an audio decoder can take two routes:
1. **Non-tunnel mode** - Decoded `PCM` audio frames with metadata are sent back to the `RDK` media pipeline.
2. **Tunnel mode** - Audio is passed directly to the vendor layer mixer.

## Test Streams Specification

The following test streams will be used for validation:

|#|Stream Name|Description |
|-|-----------|------------|
|<a id="stream01"></a>01|tones_string_48k_stereo.ac3|`AC3` format audio with tones (single notes) across various frequencies|
|<a id="stream02"></a>02|tones_string_48k_stereo.wav|`WAV` (`PCM`) format with tones across various frequencies|
|<a id="stream03"></a>03|tones_string_48k_stereo.eac3|`EAC3` format with tones across various frequencies|
|<a id="stream04"></a>04|tones_string_48k_stereo.aac|`AAC` format with tones across various frequencies|
|<a id="stream05"></a>05|tones_string_48k_stereo.ogg|`VORBIS` format with tones across various frequencies|
|<a id="stream06"></a>06|tones_string_48k_stereo.wma|`WMA` format with tones across various frequencies|
|<a id="stream07"></a>07|invalid_48k_stereo.ac3|`AC3` format audio with invalid header/data|
|<a id="stream08"></a>08|invalid_48k_stereo.mp2|`mp2` (`PCM`) format with invalid header/data|
|<a id="stream09"></a>09|invalid_48k_stereo.eac3|`EAC3` format with invalid header/data|
|<a id="stream10"></a>10|invalid_48k_stereo.aac|`AAC` format with invalid header/data|
|<a id="stream11"></a>11|invalid_48k_stereo.ogg|`VORBIS` format with invalid header/data|
|<a id="stream12"></a>12|invalid_48k_stereo.wma|`WMA` format with invalid header/data|
|<a id="stream13"></a>13|encrypt_48k_stereo.aac|encrypted audio stream|

## L2 Test Specification

|Test|Description|HAL APIs|
|----|-----------|--------|
|Set and Get Audio Properties|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>set read-write properties (`Property`) of decoder and verify using get function</li><li>get read only properties of decoder and verify with default/supported values read from `hfp_audiodecoder.yaml`</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getProperty, close </li><li>IAudioDecoderController - setProperty</li></ul>|
|States of Audio Decoder|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Register event listeners using `registerEventListener`</li><li>Ensure state transitions (`open -> start -> stop -> close`) work as expected using `getState`. Validate event listener notifications using `onStateChanged`.</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getState, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, stop, flush</li></ul>|

## L3 Test Specification

|Test|Description|HAL APIs|Test Stream|
|----|-----------|--------|-----------|
|Decode Audio Frames (non-tunnel)|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes to decoder in non-tunnel module</li><li>Verify the decoded frame, `nsPresentationTime`, `FrameMetadata` fields received from `onFrameOutput`</li><li>After decoding all frames `stop` and `close` the decoder</li><li>If decoder supports secure mode, repeat above steps for decoder operating in secure mode</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li><li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01), [02](#stream02), [03](#stream03), [04](#stream04), [05](#stream05), [06](#stream06), [13](#stream13)|
|Invalid Audio Frames (non-tunnel)|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes to decoder in non-tunnel module</li><li>check if the decoder raises proper error</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li><li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[07](#stream07), [08](#stream08), [09](#stream09), [10](#stream10), [11](#stream11), [12](#stream12)|
|Signal Discontinuity|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes to decoder in non-tunnel module</li><li>Signal discontinuity using `signalDiscontinuity`</li><li>Decoder should continue decoding without any issue</li><li>Verify the decoded frame, `nsPresentationTime`, `FrameMetadata` fields received from `onFrameOutput`</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, signalDiscontinuity, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li><li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01), [02](#stream02), [03](#stream03), [04](#stream04), [05](#stream05), [06](#stream06)|
|Signal `EOS`|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes to decoder in non-tunnel module</li><li>Signal `EOS` using `signalEOS`</li><li>Decoder should decode without any issue and after all frames decode `FrameMetadata.endOfStream` must be `true`</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, signalEOS, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li><li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01), [02](#stream02), [03](#stream03), [04](#stream04), [05](#stream05), [06](#stream06)|
|Check flush functionality|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes to decoder in non-tunnel module</li><li>call `flush` and check if the queued frames are flushed out. Check decoder state with rest flag enabled and disabled</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, flush, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li><li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01), [02](#stream02), [03](#stream03), [04](#stream04), [05](#stream05), [06](#stream06)|
|Multi-Instance Decoding|If platform supports multiple decoders, open multiple instances of audio decoders at same time and check if the decoding is happening properly or not|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01), [02](#stream02), [03](#stream03), [04](#stream04), [05](#stream05), [06](#stream06)|
|Latency Measurement|Measure the latency of decoder by measuring the time difference between `decodeBuffer` and `onFrameOutput`  and verify the latency is with in the accepted range|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01), [02](#stream02), [03](#stream03), [04](#stream04), [05](#stream05), [06](#stream06)|

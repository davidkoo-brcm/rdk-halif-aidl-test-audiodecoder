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
- `rms`    - Root mean Square
- `PTS`    - Presentation Time Stamp

## Introduction

This document provides an overview of the testing requirements for the Audio Decoder module. It outlines the scope of testing, objectives, testing levels and approaches, specific test requirements.

- Audio Decoder Interface Definitions - [Audio Decoder HAL](https://github.com/rdkcentral/rdk-halif-aidl/tree/v0.6/audiodecoder/current/com/rdk/hal/audiodecoder)

- Audio Decoder Overview - [Audio Decoder Overview](https://wiki.rdkcentral.com/display/RDKHAL/Audio+Decoder)

## Module Description

The Audio Decoder `HAL` service provides the interfaces for compressed audio to be passed to the vendor layer for decoding.  It may be passed secure buffers if it indicates support for secure audio processing.

The output of an audio decoder can take two routes:

1. **Non-tunnel mode** - Decoded `PCM` audio frames with metadata are sent back to the `RDK` media pipeline.
2. **Tunnel mode** - Audio is passed directly to the vendor layer mixer.

## Test Streams Specification

Test streams are local data files for the test suite to inject directly into the `HAL`.

They are read by the test suite, written into buffers and passed into the audio decoder.

The naming convention for the test streams is as follows:

```bash

<Description>_<Samplerate>_<channel>_<Bitrate kbps>.<extension> (elementary stream file)
e.g.
tone_48k_2_256.ac3

```

The following test streams will be used for validation:

|#|Stream Name|Description |
|-|-----------|------------|
|<a id="stream01"></a>01|tones_48k_2_512.ac3|`AC3` format audio with tones (single notes) across various frequencies|
|<a id="stream02"></a>02|tones_48k_2_1536.wav|`WAV` (`PCM`) format with tones across various frequencies|
|<a id="stream03"></a>03|tones_48k_2_128.eac3|`EAC3` format with tones across various frequencies|
|<a id="stream04"></a>04|tones_48k_2_256.aac|`AAC` format with tones across various frequencies|
|<a id="stream05"></a>05|tones_48k_2_128.ogg|`VORBIS` format with tones across various frequencies|
|<a id="stream06"></a>06|tones_48k_2_128.wma|`WMA` format with tones across various frequencies|
|<a id="stream07"></a>07|tones_44.1k_5.1_1024.aac|`aac` format with tones across various frequencies|
|<a id="stream08"></a>08|tones_32k_5.1_756.aac|`aac` format with tones across various frequencies|
|<a id="stream09"></a>09|tones_44.1k_1_128.aac|`aac` format with tones across various frequencies|
|<a id="stream10"></a>10|tones_48k_5.1_1024.ac3|`AC3` format with tones across various frequencies|
|<a id="stream11"></a>11|tones_32k_5.1_756.ac3|`AC3` format with tones across various frequencies|
|<a id="stream12"></a>12|tones_44.1k_1_128.ac3|`AC3` format with tones across various frequencies|
|<a id="stream13"></a>13|tones_48k_7.1_1024.ac3|`AC3` format with tones across various frequencies|
|<a id="stream14"></a>14|invalid_48k_2_256.ac3|`AC3` format audio with invalid header/data|
|<a id="stream15"></a>15|invalid_48k_2_128.mp2|`mp2` format with invalid header/data|
|<a id="stream16"></a>16|invalid_48k_2_128.eac3|`EAC3` format with invalid header/data|
|<a id="stream17"></a>17|invalid_48k_2_256.aac|`AAC` format with invalid header/data|
|<a id="stream18"></a>18|invalid_48k_2_128.ogg|`VORBIS` format with invalid header/data|
|<a id="stream19"></a>19|invalid_48k_2_256.wma|`WMA` format with invalid header/data|
|<a id="stream20"></a>20|encrypt_48k_2_256.aac|encrypted audio stream|
|<a id="stream21"></a>21|MS12 streams|MS12 streams for tunneling mode|

Test should refer `audiodecoder_L3_testConfig.yml` for the stream information like `PTS`, frame sizes.

## L2 Test Specification

|Test|Description|HAL APIs|
|----|-----------|--------|
|Set and Get Audio Properties|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>set read-write properties (`Property`) of decoder and verify using get function</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getProperty, close </li><li>IAudioDecoderController - setProperty</li></ul>|
|States of Audio Decoder|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Register event listeners using `registerEventListener`</li><li>Ensure state transitions (`open -> start -> flush -> stop -> close`) work as expected using `getState`. Validate event listener notifications using `onStateChanged`.</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getState, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, stop, flush</li></ul>|

## L3 Test Specification

|Test|Description|HAL APIs|Test Stream|
|----|-----------|--------|-----------|
|Decode Audio Frames (non-tunnel)|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes to decoder.</li><li>Verify the decoded frame, `nsPresentationTime`, `FrameMetadata` fields received from `onFrameOutput`</li><li>Verify the decoded frames with reference frames using `rms` diff</li><li>Verify the decoder metrics using `getProperty`</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, getProperty, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li><li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01) to [13](#stream13)|
|Decode Secure Audio Frames (non-tunnel)|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>If audio decoder supports secure mode, `open` in secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes to decoder.</li><li>Verify the decoded frame, `nsPresentationTime`, `FrameMetadata` fields received from `onFrameOutput`</li><li>Verify the decoded frames with reference frames using `rms` diff</li><li>Verify the decoder metrics using `getProperty`</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, getProperty, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li><li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[20](#stream20)|
|Decode Audio Frames (tunnel)|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes to decoder.</li><li>Verify the decoded frame(frame buffer handle should be -1), `nsPresentationTime`, `FrameMetadata` fields received from `onFrameOutput`</li><li>Verify the decoder metrics using `getProperty`</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, getProperty, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li><li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[21](#stream21)|
|Invalid Audio Frames|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes to decoder in non-tunnel module</li><li>check if the decoder raises proper error</li><li>Verify the decoder metrics using `getProperty`</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, getProperty, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li><li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[14](#stream12) to [19](#stream15)|
|Decode Audio Frames with Trim|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes and trim star and end values to decoder.</li><li>Verify the decoded frame, `nsPresentationTime`, `FrameMetadata` fields received from `onFrameOutput`</li><li>Verify the decoded frames trimmed or not</li><li>Verify the decoder metrics using `getProperty`</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, getProperty, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li><li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01) to [13](#stream13)|
|Signal Discontinuity|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes to decoder in non-tunnel module</li><li>Signal discontinuity using `signalDiscontinuity`</li><li>Decoder should continue decoding without any issue</li><li>Verify the decoded frame, `nsPresentationTime`, `FrameMetadata` fields received from `onFrameOutput`</li><li>Verify the decoder metrics using `getProperty`</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, getProperty, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, signalDiscontinuity, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li><li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01) to [13](#stream13)|
|Signal `EOS`|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes to decoder in non-tunnel module</li><li>Signal `EOS` using `signalEOS`</li><li>Decoder should decode without any issue and after all frames decode `FrameMetadata.endOfStream` must be `true`</li><li>Verify the decoder metrics using `getProperty`</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, getProperty, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, signalEOS, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li><li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01) to [13](#stream13)|
|Decode after Signal `EOS`|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes to decoder in non-tunnel module</li><li>Signal `EOS` using `signalEOS`</li><li>Decoder should decode without any issue and after all frames decode `FrameMetadata.endOfStream` must be `true`</li><li>Provide the audio buffers after `EOS` without `flush` or `stop`->`start`, decoder should raise proper error code</li><li>`stop`->`start` decoder and provide the audio frames, decoder should continue decoding</li><li>Signal `EOS` using `signalEOS`</li><li>`flush` decoder and provide the audio frames, decoder should continue decoding</li><li>Verify the decoder metrics using `getProperty`</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, getProperty, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, signalEOS, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li><li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01) to [13](#stream13)|
|Check flush functionality|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes to decoder in non-tunnel module</li><li>call `flush` and check if the queued frames are flushed out. Check decoder state with rest flag enabled and disabled</li><li>Verify the decoder metrics using `getProperty`</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, getProperty, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, flush, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li><li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01) to [13](#stream13)|
|Multi-Instance Decoding|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>If multiple decoders supported `open` all instances</li><li>`start` and provide the audio frames to the decoder in parallel</li><li>verify all the frames decoded and verify the decoder metrics</li><li>Measure the latency of each decoder (time difference between `decodeBuffer` and `onFrameOutput`)</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, getProperty, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01) to [13](#stream13)|
|Latency Measurement|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide audio frames from test streams with known frame sizes to decoder.</li><li>Measure the latency of decoder by measuring the time difference between `decodeBuffer` and `onFrameOutput`</li><li>Verify the latency is with in the accepted range</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, getProperty, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01) to [13](#stream13)|
|Buffer fullness check|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Loop through supported audio formats</li><li>`open` audio decoder in non-secure mode</li><li>register control and event listeners</li><li>`start` the decoder</li><li>Provide back to back frames to the decoder, if the buffers are full `decodeBuffer` should return `false`</li><li>After decoding all frames `stop` and `close` the decoder</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getCapabilities, getProperty, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, decodeBuffer, stop<li>IAudioDecoderControllerListener - onFrameOutput<li>IAudioDecoderEventListener - onDecodeError</li>IAVBuffer - createAudioPool, alloc, free, destroyPool</li></ul>|[01](#stream01) to [13](#stream13)|

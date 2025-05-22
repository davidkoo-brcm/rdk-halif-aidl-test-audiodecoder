# Audio Decoder Test Streams

Test streams are local data files for the test suite to inject directly into the `HAL`.

They are read by the test suite, written into buffers and passed into the audio decoder.

The naming convention for the test streams is as follows:

```bash

<Description>_<Samplerate>_<channel>_<Bitrate kbps>.<extension> (elementary stream file)
e.g.
tone_48k_2_256.ac3

```

The following test streams will be used for validation:

## MP3 (MPEG 1/2 Layer 3)

|#|Name|Description|
|-|----|-----------|
|1|tone_48k_2_256.mp3|tones across various frequencies|
|2|tone_48k_1_128.mp3|tones across various frequencies|
|3|tone_44.1k_1_128.mp3|tones across various frequencies|
|4|tone_44.1k_2_256.mp3|tones across various frequencies|
|5|tone_32k_2_256.mp3|tones across various frequencies|
|6|tone_32k_1_128.mp3|tones across various frequencies|
|7|invalid1_48k_2_256.mp3|corrupted header|
|8|invalid2_48k_2_256.mp3|corrupted data|

## MP2 (MPEG 1/2 Layer 2)

|#|Name|Description|
|-|----|-----------|
|1|tone_48k_2_256.mp2|tones across various frequencies|
|2|tone_48k_1_128.mp2|tones across various frequencies|
|3|tone_44.1k_1_128.mp2|tones across various frequencies|
|4|tone_44.1k_2_256.mp2|tones across various frequencies|
|5|tone_32k_2_256.mp2|tones across various frequencies|
|6|tone_32k_1_128.mp2|tones across various frequencies|
|7|invalid1_48k_2_256.mp2|corrupted header|
|8|invalid2_48k_2_256.mp2|corrupted data|
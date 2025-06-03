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

## AAC-LC

|#|Name|Frames|Description|
|-|----|------|-----------|
|1|tones_8k_2_64.aac|10|250Hz tone|
|2|tones_11.025k_2_64.aac|10|250Hz tone|
|3|tones_16k_2_64.aac|10|250Hz tone|
|4|tones_32k_2_64.aac|10|250Hz tone|
|5|tones_44.1k_2_128.aac|10|250Hz tone|
|6|tones_48k_1_256_10.aac|10|250Hz tone|
|7|tones_48k_2_256.aac|10|250Hz tone|
|8|tones_48k_5.1_384.aac|10|250Hz tone|
|9|tones_48k_7.1_384.aac|10|250Hz tone|
|10|tones_64k_2_256.aac|10|250Hz tone|
|11|tones_88.2k_2_256.aac|10|250Hz tone|
|12|tones_96k_2_384.aac|10|250Hz tone|
|13|tones_96k_5.1_512.aac|10|250Hz tone|
|14|tones_96k_7.1_1024.aac|10|250Hz tone|
|15|tones_48k_2_256_zero.aac|10|Random bits in the frames are reset to zero|
|16|tones_48k_2_256_random_bitflip.aac|10|Random bits in the frames are flipped|
|17|tones_48k_2_256_random.aac|10|Random bits in the frames are replaced with random values|
|18|tones_48k_2_256_sync.aac|10|invalid sync-word|
|19|tones_48k_2_256_header_fields.aac|10|Invalid sampling freq, invalid channel config, invalid layer|

## MP3 (MPEG 1/2 Layer 3)

|#|Name|Frames|Description|
|-|----|------|-----------|
|1|tones_32k_2_64.mp3|10|250Hz tone|
|2|tones_44.1k_2_128.mp3|10|250Hz tone|
|3|tones_48k_1_256_10.mp3|10|250Hz tone|
|4|tones_48k_2_256.mp3|10|250Hz tone|
|5|tones_48k_2_256_zero.mp3|10|Random bits in the frames are reset to zero|
|6|tones_48k_2_256_random_bitflip.mp3|10|Random bits in the frames are flipped|
|7|tones_48k_2_256_random.mp3|10|Random bits in the frames are replaced with random values|
|8|tones_48k_2_256_sync.mp3|10|invalid syncword|
|9|tones_48k_2_256_header_fields.mp3|10|Invalid sampling freq, invalid channel config, invalid layer|

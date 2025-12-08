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
|20|tones_48k_2_256_incompleframes.aac|10|Removed random bytes from each frames|

## MP3 (MPEG 1/2 Layer 3)

|#|Name|Frames|Description|
|-|----|------|-----------|
|1|tones_32k_1_256.mp3|10|250Hz tone|
|2|tones_32k_2_256.mp3|10|250Hz tone|
|3|tones_44.1k_1_128.mp3|10|250Hz tone|
|4|tones_44.1k_2_128.mp3|10|250Hz tone|
|5|tones_48k_1_256.mp3|10|250Hz tone|
|6|tones_48k_2_256.mp3|10|250Hz tone|
|7|tones_48k_2_256_sync|10|invalid syncword|
|8|tones_48k_2_256_zero.mp3|3|frame contains valid sync word with data replaced with zero|
|9|tones_48k_2_256_randombits.mp3|3|frame contains valid sync word with data replaced with ff|

### Error Streams Info

AV VTS L4 includes playback of error (corrupted) streams to evaluate the error recovery capability of the product.

As part of this verification, various audio error streams are generated and used during testing.

**Example:** Audio Error Stream File Name

```bash

    aac_48k_2ch_frame-skip-duplication_256k_a2814_v0_60sec.mp4

```

**Stream File Name Breakdown:**

|Codec|Sample Rate|Channel|Error Type|Bit Rate|Audio Frames|Video Frames|Duration|
|------|----------|------------|---------|----------|-------------|--------|------|
|AAC|48k | 2ch        |frame-skip-duplication|256k|2814|0|60sec|

**Supported Audio Error Types:**

Below is the list of supported audio error types used for AV VTS L4 recovery validation:

#### Frame-Level Structural Corruptions

|**Audio Error Type**|**Description**|**Typical Effects**|
|--------------------|---------------|-------------------|
|frame-skip-duplication|Randomly drops or duplicates complete audio frames.|Audible pops, timing drift, “Invalid frame length” warnings.|
|frame-truncation|Cuts a frame before its natural end, leaving partial data.|Decoder “unexpected EOF” or silence gap.|
|adts-header-corruption|Flips bits in ADTS headers (for AAC) but keeps syncword.|“Invalid sampling rate index”, playback mutes briefly.|

#### Bitstream Parameter Corruptions

|**Audio Error Type**|**Description**|**Typical Effects**|
|--------------------|---------------|-------------------|
|bitrate-field-corruption|Alters header fields that signal bitrate or profile.|Decoder fallback to default profile; momentary mute.|
|channel-mode-corruption|Flips channel config bits (stereo/mono/surround).|Channel imbalance, “Invalid channel configuration”.|
|dynamic-range-corruption|Modifies DRC metadata values.|Louder/quieter playback bursts.|

#### Entropy / Coefficient Corruptions

|**Audio Error Type**|**Description**|**Typical Effects**|
|--------------------|---------------|-------------------|
|entropy-bit-flip|Flips bits in Huffman-coded spectral data.|Crackling or “Invalid scalefactor” warnings.|
|quantization-noise-injection|Adds small random values to quantized coefficients.|Subtle distortion or fuzziness.|

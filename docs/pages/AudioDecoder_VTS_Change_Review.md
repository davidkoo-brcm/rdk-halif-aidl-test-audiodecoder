# AudioDecoder VTS Change Review

## Latest test results

Platform: Broadcom Xione. Profile: `profiles/hfp-unified_xione.yaml`. Run date: 2026-11-12.

| Level | Total | PASS | FAIL | SKIP | Duration (s) |
| --- | ---: | ---: | ---: | ---: | ---: |
| L1 | 31 | 31 | 0 | 0 | 9.269 |
| L2 | 5 | 5 | 0 | 0 | 1.924 |
| L3 | 65 | 14 | 4 | 47 | 388.687 |
| Total | 101 | 50 | 4 | 47 | 399.880 |

The four sequential/concurrent Tunnel/NonTunnel NonSecure Error cases remain FAIL. Configurations expect 4, 9 or 10 error callbacks, while the received count is zero. The raw results and error-count assertions remain unchanged.

## Prepare the decoder before starting it

`prepare()` validates input and opens/configures a READY decoder. AVClock and AudioSink callers prepare the sink/STC before invoking `run()`; the helper itself does not wait for a binding. Standalone calls to run still prepare automatically. Empty input is an explicit assertion failure, and the boolean helper returns false without using a void-returning fatal assertion macro.

[`src/vts_audiodecoder.cpp:487`](../../src/vts_audiodecoder.cpp#L487)

```cpp
bool AudioDecoderTest::prepare()
{
    if (mSetupSucceeded) return true;
    if (!mAVbufferHelper)
    {
        UT_ASSERT_TRUE(false) << "AVBufferHelper is not initialized";
        return false;
    }
    if (mTestConfiguration.frameDetails.empty())
    {
        UT_ASSERT_TRUE(false) << "Audio decoder input configuration contains no frames";
        return false;
    }
    setupAudioDecoder();
    return mSetupSucceeded;
}

void AudioDecoderTest::run(bool validateMetrics)
{
    if (!prepare()) return;

    // In AVClock tests the sink/STC has been prepared before run(). NEXUS
    // copies the STC into audio start settings here, before any input arrives.
    AUDIODECODER_START_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
    mDecoderStarted = true;
```

## Retain attached sessions through sink completion

Attached sinks keep the decoder session after input EOS. The owner joins the producer and sink before destroying the helper. Destruction closes the decoder before destroying the buffer pool.

[`src/vts_audiodecoder.cpp:546`](../../src/vts_audiodecoder.cpp#L546)

```cpp
    // Attached sinks still need the decoder and its output after input EOS.
    // The helper's owner joins both threads before destroying this object.
    if (!mIsSinkAttached)
        teardownDecoder();
}
```

## Clean up prepared and started sessions once

Track setup and start separately. Cleanup ignores an already completed teardown, stops only a decoder that actually started, and closes a prepared READY session without issuing stop.

[`src/vts_audiodecoder.cpp:898`](../../src/vts_audiodecoder.cpp#L898)

```cpp
void AudioDecoderTest::teardownDecoder()
{
    if (!mSetupSucceeded)
        return;
    mSetupSucceeded = false;

    // prepare() may have opened the decoder without run() ever starting it.
    if (mDecoderStarted)
    {
        AUDIODECODER_STOP_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
        mDecoderStarted = false;
    }

    /* Close the decoder */
    AUDIODECODER_CLOSE_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
}
```

## Resolve each configured stream name

Insert the missing path separator before Name so every PrimeAudio input resolves its own stream properties, frame sizes and PTS list.

[`src/vts_audiodecoder_parse_testconfig.cpp:95`](../../src/vts_audiodecoder_parse_testconfig.cpp#L95)

```cpp
        string streamName = streamList + "." + to_string(i) + ".Name";
        ut_kvp_getStringField(kvpTestInstance, streamName.c_str(), resultKVP, UT_KVP_MAX_ELEMENT_SIZE);
```

The Error test continues to compare configured error-frame counts with received callbacks; these changes do not weaken that assertion.

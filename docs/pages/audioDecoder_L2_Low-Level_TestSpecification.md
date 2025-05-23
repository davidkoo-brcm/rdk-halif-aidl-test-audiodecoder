# AUDIO DECODER L2 Low Level Test Specification and Procedure Documentation

## Table of Contents

- [Overview](#overview)
- [Acronyms, Terms and Abbreviations](#acronyms-terms-and-abbreviations)
- [Definitions](#definitions)
- [References](#references)
- [Level 2 Test Procedure](#level-2-test-procedure)

## Overview

This document describes the level 2 testing suite for the AUDIO DECODER module.

### Acronyms, Terms and Abbreviations

- `HAL` \- Hardware Abstraction Layer, may include some common components
- `UT`  \- Unit Test(s)
- `OEM`  \- Original Equipment Manufacture
- `SoC`  \- System on a Chip

### Definitions

- `ut-core` \- Common Testing Framework <https://github.com/rdkcentral/ut-core>, which wraps a open-source framework that can be expanded to the requirements for future framework.

### References

- `High Level Test Specification` - [audioDecoder_High-Level_TestSpecification.md](audioDecoder_High-Level_TestSpecification.md)

## Level 2 Test Procedure

The following functions are expecting to test the module operates correctly.

### Test 1

| Title | Details |
| -- | -- |
| Function Name | `test_l2_AudioDecoderBaseTest_TestGetAndSetProperties` |
| Description | Read supported audio decoders using IAudioDecoderManager class. Loop through supported audio decoders, set read-write properties (Property) of decoder and verify using get function. |
| Test Group | Module : 02 |
| Test Case ID | 001 |
| Priority | High |

**Pre-Conditions :**
None

**Dependencies :**
None

**User Interaction :**
If user chose to run the test in interactive mode, then the test case has to be selected via console.

**Test Procedure :**

| Variation / Steps | Description | Test Data | Expected Result | Notes |
| -- | --------- | ---------- | -------------- | ----- |
| 01 | Retrieve audio decoder IDs using `getAudioDecoderIds` | OUT: tmpIds = [] | Status is OK | Should be successful |
| 02 | Loop through each audio decoder ID and get the decoder using `getAudioDecoder` | IN: id = current ID, OUT: decoder = [] | Status is OK | Should be successful |
| 03 | Register event listener for each decoder using `registerEventListener` | IN: gDecoderEventListener = valid listener, OUT: registered = [] | Status is OK | Should be successful |
| 04 | Retrieve supported secure path property using `UT_KVP_PROFILE_GET_BOOL` | supported_secure_path_key = "IAudioDecoderManager.IAudioDecoder.i.supportsSecure" | Value retrieved successfully | Should be successful |
| 05 | Retrieve supported codec count using `UT_KVP_PROFILE_GET_LIST_COUNT` | supported_codec_count_key = "IAudioDecoderManager.IAudioDecoder.i.supportedCodecs" | Value retrieved successfully | Should be successful |
| 06 | Loop through each codec and retrieve codec name using `UT_KVP_PROFILE_GET_STRING` | codecpath_key = "IAudioDecoderManager.IAudioDecoder.i.supportedCodecs.j" | Value retrieved successfully | Should be successful |
| 07 | Open decoder with codec name using `open` | IN: codecName = current codec, supportsSecurepath = retrieved value, gDecoderControllerListener = valid listener, OUT: controller = [] | Status is OK | Should be successful |
| 08 | Set property LOW_LATENCY_MODE using `setProperty` | IN: property = LOW_LATENCY_MODE, propertyValue = 1, OUT: setResult = [] | Status is OK | Should be successful |
| 09 | Get property LOW_LATENCY_MODE using `getProperty` | IN: property = LOW_LATENCY_MODE, OUT: propertyValue = [] | Status is OK, propertyValue = 1 | Should be successful |
| 10 | Close decoder using `close` | IN: controller = valid controller, OUT: closed = [] | Status is OK | Should be successful |

### Test 2

| Title | Details |
| -- | -- |
| Function Name | `test_l2_AudioDecoderControllerBaseTest_TestGetAndSetProperties_multipleInstance` |
| Description | Read supported audio decoders using IAudioDecoderManager class. If platform supports multiple decoder, open all the instances of the decoders, set read-write properties (Property) of all decoders and verify using get function. |
| Test Group | Module : 02 |
| Test Case ID | 002 |
| Priority | High |

**Pre-Conditions :**
None

**Dependencies :**
None

**User Interaction :**
If user chose to run the test in interactive mode, then the test case has to be selected via console.

**Test Procedure :**

| Variation / Steps | Description | Test Data | Expected Result | Notes |
| -- | --------- | ---------- | -------------- | ----- |
| 01 | Initialize test suite by setting up audio decoder controllers and listeners | None | Controllers initialized successfully | Should be successful |
| 02 | Open each audio decoder instance using `decoder->open` | codecName = codecName, supportsSecurepath = supportsSecurepath, gDecoderControllerListener = gDecoderControllerListener | Status is OK | Should be successful |
| 03 | Set property `LOW_LATENCY_MODE` to 1 using `controller->setProperty` | property = LOW_LATENCY_MODE, propertyValue = 1 | Status is OK, setResult is true | Should be successful |
| 04 | Get property `LOW_LATENCY_MODE` using `decoder->getProperty` | property = LOW_LATENCY_MODE | Status is OK, getPropertyValue is 1 | Should be successful |
| 05 | Close each audio decoder instance using `gAudioDecoders[i]->close` | controller = gAudioDecoderControllers[i] | Status is OK, closed is true | Should be successful |
| 06 | Clear audio decoder controllers and listeners | None | Controllers and listeners cleared successfully | Should be successful |

### Test 3

| Title | Details |
| -- | -- |
| Function Name | `test_l2_AudioDecoderBaseTest_TestDecoderStateTransitions` |
| Description | Read supported audio decoders using IAudioDecoderManager class /n Loop through supported audio decoders /n Register event listeners using registerEventListener /n Ensure state transitions (open -> start -> flush -> stop -> close) work as expected using getState. Validate event listener notifications using onStateChanged. |
| Test Group | Module : 02 |
| Test Case ID | 003 |
| Priority | High |

**Pre-Conditions :**
None

**Dependencies :**
None

**User Interaction :**
If user chose to run the test in interactive mode, then the test case has to be selected via console.

**Test Procedure :**

| Variation / Steps | Description | Test Data | Expected Result | Notes |
| -- | --------- | ---------- | -------------- | ----- |
| 01 | Get audio decoder IDs using getAudioDecoderIds | OUT:tmpIds = empty vector | status.isOk() = true | Should be successful |
| 02 | Get audio decoder using getAudioDecoder | IN:id = valid ID, OUT:decoder = nullptr | status.isOk() = true | Should be successful |
| 03 | Register event listener using registerEventListener | IN:gDecoderEventListener = valid listener, OUT:registered = false | registered = true | Should be successful |
| 04 | Open decoder using open | IN:codecName = valid codec name, IN:supportsSecurepath = false, IN:gDecoderControllerListener = valid listener, OUT:controller = nullptr | status.isOk() = true | Should be successful |
| 05 | Get state using getState after opening | OUT:state = uninitialized | state = State::OPENING | Should be successful |
| 06 | Start decoder using start | No input arguments | status.isOk() = true | Should be successful |
| 07 | Get state using getState after starting | OUT:state = uninitialized | state = State::STARTING | Should be successful |
| 08 | Flush decoder using flush | IN:reset = false | status.isOk() = true | Should be successful |
| 09 | Get state using getState after flushing | OUT:state = uninitialized | state = State::STARTED | Should be successful |
| 10 | Stop decoder using stop | No input arguments | status.isOk() = true | Should be successful |
| 11 | Get state using getState after stopping | OUT:state = uninitialized | state = State::STOPPING | Should be successful |
| 12 | Close decoder using close | IN:controller = valid controller, OUT:closed = false | closed = true | Should be successful |
| 13 | Get state using getState after closing | OUT:state = uninitialized | state = State::CLOSED | Should be successful |

### Test 4

| Title | Details |
| -- | -- |
| Function Name | `test_l2_AudioDecoderBaseTest_TestDecoderStateTransitions_mutipleInstance` |
| Description | Read supported audio decoders using IAudioDecoderManager class. If platform supports multiple decoder, open all the instances of the decoders. Register event listeners using registerEventListener. Ensure state transitions of all decoder instances (open -> start -> flush -> stop -> close) work as expected using getState. Validate event listener notifications using onStateChanged. |
| Test Group | Module : 02 |
| Test Case ID | 004 |
| Priority | High |

**Pre-Conditions :**
None

**Dependencies :**
None

**User Interaction :**
If user chose to run the test in interactive mode, then the test case has to be selected via console.

**Test Procedure :**

| Variation / Steps | Description | Test Data | Expected Result | Notes |
| -- | --------- | ---------- | -------------- | ----- |
| 01 | Get audio decoder IDs using `getAudioDecoderIds` | tmpIds = empty vector | status.isOk() = true | Should be successful |
| 02 | Get audio decoder instance using `getAudioDecoder` | id = valid decoder ID, decoder = nullptr | status.isOk() = true | Should be successful |
| 03 | Register event listener using `registerEventListener` | eventListener = valid listener, registered = false | registered = true | Should be successful |
| 04 | Open decoder using `open` | codecName = valid codec name, supportsSecurepath = false, controller = nullptr | status.isOk() = true | Should be successful |
| 05 | Validate state transition to OPENING using `getState` | state = valid state buffer | state = State::OPENING | Should be successful |
| 06 | Validate state transition to READY using `getState` | state = valid state buffer | state = State::READY | Should be successful |
| 07 | Start decoder using `start` | No input arguments | status.isOk() = true | Should be successful |
| 08 | Validate state transition to STARTING using `getState` | state = valid state buffer | state = State::STARTING | Should be successful |
| 09 | Validate state transition to STARTED using `getState` | state = valid state buffer | state = State::STARTED | Should be successful |
| 10 | Flush decoder using `flush` | reset = false | status.isOk() = true | Should be successful |
| 11 | Validate state transition to FLUSHING using `getState` | state = valid state buffer | state = State::STARTED | Should be successful |
| 12 | Stop decoder using `stop` | No input arguments | status.isOk() = true | Should be successful |
| 13 | Validate state transition to STOPPING using `getState` | state = valid state buffer | state = State::STOPPING | Should be successful |
| 14 | Validate state transition to READY using `getState` | state = valid state buffer | state = State::READY | Should be successful |
| 15 | Close decoder using `close` | controller = valid controller, closed = false | closed = true | Should be successful |
| 16 | Validate state transition to CLOSING using `getState` | state = valid state buffer | state = State::CLOSING | Should be successful |
| 17 | Validate state transition to CLOSED using `getState` | state = valid state buffer | state = State::CLOSED | Should be successful |

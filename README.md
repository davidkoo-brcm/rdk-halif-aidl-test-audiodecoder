# Testing Suite For AudioDecoder HAL

## Build Steps

### 1. Build linux binder

The test suite depends on the Linux binder with `AIDL` support. Follow these steps to build it:

For detail build instruction, please refer to [Linux Binder](https://github.com/rdk-e/linux_binder_idl/tree/main)

Here outline the simplest steps are

```bash
git clone -b develop 'ssh://git@github.com/rdk-e/linux_binder_idl.git'

cd linux_binder_idl

./build-linux-binder-aidl.sh 64
```

### 2. Build this test

Clone and build the test suite:

```bash
git clone git@github.com:rdkcentral/rdk-halif-binder-test-audiodecoder.git
```

```bash
cd rdk-halif-binder-test-audiodecoder
```

Set below environment variable:

- AIDL_BIN: Path to the `AIDL` compiler (e.g., /path/to/linux_binder_idl/out/aidl).

- LINUX_BINDER_OUT_DIR: Path to the Linux binder output directory (e.g., /path/to/linux_binder_idl/local).

- UT_CORE_VERSION: Specific version of the UT-Core framework (e.g., 4.7.0). Defaults to the latest tagged version.

- RDK_HALIF_AIDL_VERSION: Specific version of the AIDL HAL Specification (e.g, 0.7.1). Defaults to the latest HEAD.

Example:

```bash
export UT_CORE_VERSION=4.7.0
export RDK_HALIF_AIDL_VERSION=0.7.1
export AIDL_BIN=/path/linux_binder_idl/out/aidl
export LINUX_BINDER_OUT_DIR=/path//linux_binder_idl/local/
```

By default, UT logs are Enabled. To disable them, Set below environment variable

```bash
export DISABLE_UT_INFO_LOGGING=1
```

Run the build script:

```bash
./build.sh all
```

The build.sh script:

- Clones the `rdk-halif-aidl` repository and UT-Core framework.

- Compiles the test suite.

- Generates binaries in the `build/bin` directory.

Example directory structure :

```bash
build/bin/
├── aidl
├── assets
│   └── streams
│       ├── input
│       └── reference
├── lib
├── libaudiodecoderaidl.so
├── libavbufferaidl.so
├── libbase.so
├── libbinder.so
├── libcutils.so
├── libcutils_sockets.so
├── libhal_logger.so
├── liblog.so
├── libut_control.so
├── libutils.so
├── libvideodecoderaidl.so
├── MockAudioDecoderService
├── profiles
│   ├── hfp-audiodecoder_mock.yaml
│   ├── hfp-audiodecoder_vdevice_instance_1.yaml
│   ├── hfp-audiodecoder_vdevice_instance_2.yaml
│   ├── hfp-audiodecoder_vdevice_instance_3.yaml
│   └── hfp-audiodecoder_vdevice.yaml
├── run.sh
├── servicemanager
├── stream_configuration
│   ├── vts_audio_aac_streams.yaml
│   ├── vts_audio_ac3_streams.yaml
│   ├── vts_audio_heaac2_streams.yaml
│   ├── vts_audio_heaac_streams.yaml
│   ├── vts_audio_mp2_streams.yaml
│   └── vts_audio_mp3_streams.yaml
├── test_configuration
│   ├── vts_l3_Decode_Discontinuity.yaml
│   ├── vts_l3_Decode_EOS.yaml
│   ├── vts_l3_Decode_Error.yaml
│   ├── vts_l3_Decode_FlushReset.yaml
│   ├── vts_l3_Decode_Flush.yaml
│   ├── vts_l3_Decode_Stop_Start.yaml
│   ├── vts_l3_Decode_Trim.yaml
│   ├── vts_l3_Decode.yaml
│   └── vts_l3_test_config.yaml
└── vts_audiodecoder_hal_test

```

## Run test

- Package the Binaries:

```bash
cd build
tar -zcf audiodecoder_vts.tar.gz bin
```

- Transfer to Target Device:

Copy the tarball to the target device (e.g., using scp):

```bash
scp audiodecoder_vts.tar.gz root@target-device:/home/root/

```

- Extract on Target Device:

Log into the target device and extract the tarball:

```bash
ssh root@target-device
cd /home/root
tar -zxf audiodecoder_vts.tar.gz
cd bin
```

To execute `L3` test cases, elementary streams are required. Please copy all necessary test streams into the `bin/assets/streams/input/` folder on the target device.

- Run the Test:
Execute the run.sh script as root:

```bash
sudo ./run.sh -p profiles/hfp-audiodecoder.yaml
```

To run the binary with two processes simultaneously:

Open two separate SSH sessions to the target device. In each session, navigate to the `bin` directory  and execute one of the following commands:

```bash
sudo  ./run.sh -p profiles/hfp-audiodecoder_vdevice_instance_1.yaml
```

```bash
sudo ./run.sh -p profiles/hfp-audiodecoder_vdevice_instance_2.yaml
```

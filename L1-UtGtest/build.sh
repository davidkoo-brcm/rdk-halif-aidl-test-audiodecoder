#!/bin/bash

# Define the directory where the repository will be cloned
declare -r UT_CORE_VER="4.1.0"
declare -r TOP=`dirname $(realpath $0)`
declare -r UT_DIR="${TOP}/ut"
declare -r BUILD_DIR="${UT_DIR}/build"

DEF_LINUX_BINDER="`realpath ${TOP}/../../`/linux_binder_idl"
# AIDL_BIN: path of aidl compiler
AIDL_BIN=${AIDL_BIN:-${DEF_LINUX_BINDER}/out/aidl}
# LINUX_BINDER_OUT_DIR: output installed after building linux_binder_idl
# contains bin/servicemanager, lib/libbinder etc
LINUX_BINDER_OUT_DIR=${LINUX_BINDER_OUT_DIR:-${DEF_LINUX_BINDER}/local}

# Used in build audiodecoder_hal
export HAL_DIR=${UT_DIR}

mkdir -p $UT_DIR
mkdir -p $BUILD_DIR

if [ ! -x $AIDL_BIN ]; then
	echo "AIDL_BIN: path does not exists, consdier building the linux_binder_idl in $DEF_LINUX_BINDER"
	exit 1
fi
if [ ! -d $LINUX_BINDER_OUT_DIR ]; then
	echo "LINUX_BINDER_OUT_DIR: path does not exists, consider building the linux_binder_idl in $DEF_LINUX_BINDER"
	exit 1
fi

# Turning them into absolute path because we may calling change current working directory
# whiling running the script
declare -r AIDL_BIN=`realpath ${AIDL_BIN}`
declare -r LINUX_BINDER_OUT_DIR=`realpath ${LINUX_BINDER_OUT_DIR}`

install_linux_binder()
{
	declare -r INSTALL_PREFIX=${BUILD_DIR}/usr

	echo "Install linux-binder \`service manager, headers and libraries'"
	install -d ${INSTALL_PREFIX}/bin ${INSTALL_PREFIX}/lib ${INSTALL_PREFIX}/include
	install -t ${INSTALL_PREFIX}/bin ${LINUX_BINDER_OUT_DIR}/bin/*
	install -t ${INSTALL_PREFIX}/lib ${LINUX_BINDER_OUT_DIR}/lib/*
	cp -apr ${LINUX_BINDER_OUT_DIR}/include/* ${INSTALL_PREFIX}/include/
}

generate_audiodecoder_hal() {
    declare -r HAL_BUILD_DIR=${BUILD_DIR}/audiodecoder_hal
    declare -r AUDIODECODER_VERSION=current
    declare -r AIDL_GEN_DIR="${TOP}/ut/src/gen/hal/$AUDIODECODER_VERSION"

    # Clone the repository if not already present
    if [ ! -d 'ut/hal' ]; then
        git clone -b develop 'ssh://git@github.com/rdk-e/hal.git' ut/hal
    fi

    # Prepare cmake build directories and configuration for each module
    for m in audiodecoder; do
        cmake \
            -DAIDL_BIN=$AIDL_BIN \
            -DAIDL_GEN_DIR=$AIDL_GEN_DIR \
            -DAIDL_SRC_VERSION=$AUDIODECODER_VERSION \
            -DAIDL_TARGET=$m \
            -B ${HAL_BUILD_DIR}/gen/build/${m}_test \
            ut/hal
    done

    # Navigate to the build directory and invoke Makefile once
    pushd ${HAL_BUILD_DIR}/gen/build/audiodecoder_test

    # Invoke Makefile to build the mock_service target
    make -f ${TOP}/ut/Makefile.mock_service
    popd
}

build_ut_core()
{
	if [ ! -d ${UT_DIR}/ut-core ]; then
		git clone https://github.com/rdkcentral/ut-core.git ${UT_DIR}/ut-core
		if [ $? -ne 0 ]; then
			echo "Failed to clone the repository. Exiting."
			exit 1
		fi
	fi

	pushd ${UT_DIR}/ut-core

	git fetch -q origin $UT_CORE_VER
	git checkout $UT_CORE_VER

	./build.sh TARGET=linux VARIANT=CPP
	if [ $? -ne 0 ]; then
		echo "Build script failed. Exiting."
		exit 1
	fi

	echo "Build completed successfully."
	popd
}

install_test()
{
	declare -r BUILD_INSTALL_PREFIX=${BUILD_DIR}/usr
	declare -r INSTALL_PATH=${BUILD_DIR}/bin

	install -t ${INSTALL_PATH} ${BUILD_DIR}/usr/bin/*
	install -t ${INSTALL_PATH} ${BUILD_DIR}/usr/lib/*
	install -t ${INSTALL_PATH} run.sh
}

install_linux_binder
generate_audiodecoder_hal
build_ut_core
# Building test module using ut-core framework
make -C ${UT_DIR}
install_test

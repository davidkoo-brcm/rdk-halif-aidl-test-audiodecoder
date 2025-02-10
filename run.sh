#!/bin/bash

DEV_BINDER='/dev/binder'
RUN_DIR=`realpath $(dirname $0)`

if [ ! -w $DEV_BINDER ]; then
	echo "No permission to access $DEV_BINDER, consider execute this as root"
	exit 1
fi

export LD_LIBRARY_PATH=$RUN_DIR:$LD_LIBRARY_PATH

echo "Eexecute [servicemanager]"
${RUN_DIR}/servicemanager &
servicemanager_pid=$!
# Need to wait some time for the service to be ready
sleep 2

echo "Eexecute [EmulateAudioDecoderService]"
${RUN_DIR}/EmulateAudioDecoderService &
AudioDecoderService_pid=$!
# Need to wait some time for the service to be ready
sleep 2

echo "Eexecute [hal_test]"
${RUN_DIR}/hal_test -p profiles/hfp-audiodecoder.yaml

# cleanup
kill -9 $servicemanager_pid $AudioDecoderService_pid

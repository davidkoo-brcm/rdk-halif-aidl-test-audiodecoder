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

echo "Eexecute [AudioDecoderService]"
${RUN_DIR}/AudioDecoderService &
AudioDecoderService_pid=$!
# Need to wait some time for the service to be ready
sleep 2

echo "Eexecute [L1_audiodecoder]"
${RUN_DIR}/L1_audiodecoder

# cleanup
kill -9 $servicemanager_pid $AudioDecoderService_pid

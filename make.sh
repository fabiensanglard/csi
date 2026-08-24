#!/bin/sh
set -eu

cmake -S . -B build
cmake --build build --config Release

if [ -f build/Release/csi ]; then
	printf 'Built binary: %s/build/Release/csi\n' "$(pwd)"
else
	printf 'Built binary: %s/build/csi\n' "$(pwd)"
fi

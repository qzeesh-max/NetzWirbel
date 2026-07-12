#!/bin/bash
set -e

echo "Building examples..."
# Ensure the project is built before running orchestrator
emcmake cmake -S . -B build || (
	cd $(dirname $0)

	rm -rf build
	mkdir build

	cd build

	echo " at : $(pwd) "
	emcmake cmake ..
)

make -C build -j4


echo "Starting NetzWirbel Examples Orchestrator..."
echo "You can access the examples in your browser (usually http://localhost:3000)"
npm run examples

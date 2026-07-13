#!/bin/bash
set -e


cd $(dirname $0)


./install_prerequisites.sh

# make sure express is installed
( 
	npm init -y
	npm install express
)

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


(cd examples/OrderMatchBackend/ && cd build-native && cmake .. && make) ||
(cd examples/OrderMatchBackend/ && rm -rf build-native && mkdir build-native && cd build-native && cmake .. && make)

(cd examples/OdysseyBackend/ && cd build-native && cmake .. && make) ||
(cd examples/OdysseyBackend/ && rm -rf build-native && mkdir build-native && cd build-native && cmake .. && make)

echo
echo ========================================================
echo   All components built successfully!
echo ========================================================
echo
echo Quick Start:
echo - To run the frontend orchestrator:   npm run examples
echo - To run OrderMatchBackend:           ./examples/OrderMatchBackend/build-native/OrderMatchBackend examples/OrderMatchBackend/ordermatch.cfg
echo - To run OdysseyBackend:              ./examples/OdysseyBackend/build-native/OdysseyBackend
echo

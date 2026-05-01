#!/bin/bash

# actual build

mkdir -p build_linux                               # create directory
cd build_linux                                     # change into new directory
cmake ..                                           # let cmake configure the Makefile
cmake --build . --target install --config Release -j $(nproc) #  use all cores

# after this, a new folder structure should be created within the root directory, '${ROOT}/install/bin'
# executing the app 'direct_volume_renderer_app.exe' from within the bin folder starts the app with standard parameters

# also make sure to have openmp installed

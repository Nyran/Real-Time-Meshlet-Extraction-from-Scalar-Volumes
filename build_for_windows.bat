echo off

rem create directory
mkdir build_windows
rem change into new directory      
cd build_windows

rem let cmake configure the project
cmake ..
rem build everything and run the install target
cmake --build . --target install --config Release 


rem after this, a new folder structure should be created within the root directory, '${ROOT}/install/bin'
rem executing the app 'direct_volume_renderer_app.exe' from within the bin folder starts the app with standard parameters

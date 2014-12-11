mkdir EclipseUnixDebug
cd EclipseUnixDebug
cmake -G "Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DHDF5_ENABLE_THREADSAFE=ON ../..
cd ..

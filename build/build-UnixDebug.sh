mkdir UnixDebug
cd UnixDebug
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DHDF5_ENABLE_THREADSAFE=ON ../..
cd ..


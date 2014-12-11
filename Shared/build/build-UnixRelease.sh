mkdir UnixRelease
cd UnixRelease
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DHDF5_ENABLE_THREADSAFE=ON ../..
cd ..

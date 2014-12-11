mkdir EclipseUnixRelease
cd EclipseUnixRelease
cmake -G "Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DHDF5_ENABLE_THREADSAFE=ON ../..
cd ..

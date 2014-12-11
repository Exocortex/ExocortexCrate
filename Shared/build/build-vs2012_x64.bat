rmdir /s vs2012_x64
mkdir vs2012_x64
cd vs2012_x64
cmake -G "Visual Studio 11 Win64" -DHDF5_ENABLE_THREADSAFE=ON ..\..
cd ..
pause
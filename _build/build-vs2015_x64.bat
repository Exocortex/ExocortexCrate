rmdir /s vs2015_x64
mkdir vs2015_x64
cd vs2015_x64
cmake -G "Visual Studio 14 Win64" -DHDF5_ENABLE_THREADSAFE=ON ..\..
cd ..
pause

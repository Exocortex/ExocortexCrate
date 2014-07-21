rmdir /s vs2008_x64
mkdir vs2008_x64
cd vs2008_x64
cmake -G "Visual Studio 9 2008 Win64" -DHDF5_ENABLE_THREADSAFE=ON ..\..
cd ..
pause
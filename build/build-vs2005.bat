rmdir /s vs2005
mkdir vs2005
cd vs2005
cmake -G "Visual Studio 8 2005" -DHDF5_ENABLE_THREADSAFE=OFF ..\..
cd ..

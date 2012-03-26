mkdir vs2010_WinXP_x64
cd vs2010_WinXP_x64
cmake -G "Visual Studio 10 Win64" -DHDF5_ENABLE_THREADSAFE=OFF ..\..
cd ..
mkdir vs2010_WinXP
cd vs2010_WinXP
cmake -G "Visual Studio 10" -DHDF5_ENABLE_THREADSAFE=OFF ..\..
cd ..

mkdir vs2010_WinXP_x64
cd vs2010_x64
cmake -G "Visual Studio 10 Win64" -DHDF5_ENABLE_THREADSAFE=OFF ..\..
cd ..

rmdir /s vs2008_WinXP
mkdir vs2008_WinXP
cd vs2008_WinXP
cmake -G "Visual Studio 9 2008" -DHDF5_ENABLE_THREADSAFE=OFF ..\..
cd ..

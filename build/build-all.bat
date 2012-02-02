mkdir vs2008
cd vs2008
cmake -G "Visual Studio 9 2008" ..\..
cd ..

mkdir vs2008_x64
cd vs2008_x64
cmake -G "Visual Studio 9 2008 Win64" ..\..
cd ..

mkdir vs2010
cd vs2010
cmake -G "Visual Studio 10" ..\..
cd ..

mkdir vs2010_x64
cd vs2010_x64
cmake -G "Visual Studio 10 Win64" ..\..
cd ..

mkdir vs2008_WinXP
cd vs2008_WinXP
cmake -G "Visual Studio 9 2008" -DHDF5_ENABLE_THREADSAFE=OFF ..\..
cd ..

mkdir vs2008_WinXP_x64
cd vs2008_WinXP_x64
cmake -G "Visual Studio 9 2008 Win64" -DHDF5_ENABLE_THREADSAFE=OFF ..\..
cd ..

mkdir vs2010_WinXP
cd vs2010_WinXP
cmake -G "Visual Studio 10" -DHDF5_ENABLE_THREADSAFE=OFF ..\..
cd ..

mkdir vs2010_WinXP_x64
cd vs2010_WinXP_x64
cmake -G "Visual Studio 10 Win64" -DHDF5_ENABLE_THREADSAFE=OFF ..\..
cd ..

mkdir nmake
cd nmake
cmake -G "NMake Makefiles" ..\..
cd ..

mkdir unixmake
cd unixmake
cmake -G "Unix Makefiles" ..\..
cd ..

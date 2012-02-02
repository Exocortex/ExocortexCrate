mkdir vs2010
cd vs2010
cmake -G "Visual Studio 10" ..\..
cd ..

mkdir vs2010_x64
cd vs2010_x64
cmake -G "Visual Studio 10 Win64" ..\..
cd ..

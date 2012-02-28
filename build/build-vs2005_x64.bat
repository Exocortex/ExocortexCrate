rmdir /s vs2005_x64
mkdir vs2005_x64
cd vs2005_x64
cmake -G "Visual Studio 8 2005 Win64" -DHDF5_ENABLE_THREADSAFE=OFF -DINCLUDE_ALEMBIC_HOUDINI=ON ..\..
cd ..

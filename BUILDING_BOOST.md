## How to Build Boost for use in Exocortex Crate

# Visual Studio 2010 / MSVC-10.0

Turn this into a batch file:

	set BOOST_VERSION=boost_1_44_0
	set TOOLSET=msvc-10.0
	set ZLIB_ROOT=E:\Libraries\zlib
	set ZLIB_INCLUDE=%ZLIB_ROOT%
	set ZLIB_SOURCE=%ZLIB_ROOT%
	set BOOST_INSTALL32=E:/Boost/%BOOST_VERSION%/x86/
	bjam toolset=%TOOLSET% variant=debug variant=release --without-mpi --without-graph --without-math --without-wave --without-wserialization address-model=32 threading=multi link=static runtime-link=static runtime-link=shared -j%NUMBER_OF_PROCESSORS% install --prefix=%BOOST_INSTALL32%
	set BOOST_INSTALL64=E:/Boost/%BOOST_VERSION%/x64/
	bjam toolset=%TOOLSET% variant=debug variant=release --without-mpi --without-graph --without-math --without-wave --without-wserialization address-model=64 threading=multi link=static runtime-link=static runtime-link=shared -j%NUMBER_OF_PROCESSORS% install --prefix=%BOOST_INSTALL64%

# Visual Studio 2008 / MSVC-9.0

Turn this into a batch file:

	set BOOST_VERSION=boost_1_44_0
	set TOOLSET=msvc-9.0
	set ZLIB_ROOT=E:\Libraries\zlib
	set ZLIB_INCLUDE=%ZLIB_ROOT%
	set ZLIB_SOURCE=%ZLIB_ROOT%
	set BOOST_INSTALL32=E:/Boost/%BOOST_VERSION%/x86/windows
	bjam toolset=%TOOLSET% variant=debug variant=release --without-mpi --without-graph --without-math --without-wave --without-wserialization address-model=32 threading=multi link=static runtime-link=static runtime-link=shared -j%NUMBER_OF_PROCESSORS% install --prefix=%BOOST_INSTALL32%
	set BOOST_INSTALL64=E:/Boost/%BOOST_VERSION%/x64/windows
	bjam toolset=%TOOLSET% variant=debug variant=release --without-mpi --without-graph --without-math --without-wave --without-wserialization address-model=64 threading=multi link=static runtime-link=static runtime-link=shared -j%NUMBER_OF_PROCESSORS% install --prefix=%BOOST_INSTALL64%

# GCC on Linux

	set ZLIB_ROOT=$HOME/Work/Libraries.20140707.01/zlib
	set ZLIB_INCLUDE=$ZLIB_ROOT
	set ZLIB_SOURCE=$ZLIB_ROOT
	# with serialization
	./bjam toolset=gcc variant=release cxx-flags=-fPIC --layout=versioned --without-mpi --without-graph --without-math --without-wave address-model=64 threading=multi link=static runtime-link=static -j4 install --prefix=../boost_1_44_0_linux_x64
	# without serialization
	./bjam toolset=gcc variant=release cxx-flags=-fPIC --layout=versioned --without-mpi --without-graph --without-math --without-wave --without-wserialization address-model=64 threading=multi link=static runtime-link=static -j4 install --prefix=../../../Boost/boost_1_44_0/x64/linux

## Extras

To understand fPIC, please read this:

http://lists.boost.org/boost-users/2009/04/46875.php

http://boost.2283326.n4.nabble.com/fPIC-option-for-boost-td3176976.html
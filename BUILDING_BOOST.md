## How to Build Boost for use in Exocortex Crate

Normally you do not need to compile boost yourself as it is included in the Exocortex Libraries package.  BUt if you need to compile boost for a new version of Visual C++ or GCC that isn't yet supported by the Exocortex Libraries package.  Replace the pattern in the libraries path XXXXXXXX.XX with the appropriate numbering for the external libraries package.

# High Level Build Process

Just download the 1.44 version of Boost and unzip it in a directory.

Then run the bootstrap command line program.  This will built bjam.

Then run the appropriate file below to build boost with the operations Exocortex Crate requires.

Then copy the results  into the appropriate directory in the libraries.


# Visual Studio 2010 / MSVC-10.0

Turn this into a batch file:

	set BOOST_VERSION=boost_1_44_0
	set TOOLSET=msvc-10.0
	set ZLIB_ROOT=C:\Libraries.XXXXXXXX.XX\zlib
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
	set ZLIB_ROOT=C:\Libraries.XXXXXXXX.XX\zlib
	set ZLIB_INCLUDE=%ZLIB_ROOT%
	set ZLIB_SOURCE=%ZLIB_ROOT%
	set BOOST_INSTALL32=E:/Boost/%BOOST_VERSION%/x86/windows
	bjam toolset=%TOOLSET% variant=debug variant=release --without-mpi --without-graph --without-math --without-wave --without-wserialization address-model=32 threading=multi link=static runtime-link=static runtime-link=shared -j%NUMBER_OF_PROCESSORS% install --prefix=%BOOST_INSTALL32%
	set BOOST_INSTALL64=E:/Boost/%BOOST_VERSION%/x64/windows
	bjam toolset=%TOOLSET% variant=debug variant=release --without-mpi --without-graph --without-math --without-wave --without-wserialization address-model=64 threading=multi link=static runtime-link=static runtime-link=shared -j%NUMBER_OF_PROCESSORS% install --prefix=%BOOST_INSTALL64%

# GCC on Linux

Turn this into a shell script:

	set ZLIB_ROOT=$HOME/Libraries.XXXXXXXX.XX/zlib
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

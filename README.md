# Exocortex Crate

The professional grade, battle tested and feature filled Alembic suite.

# Binary Downloads

You can download the latest binaries from the main Exocortex Crate product page:

http://exocortex.com/products/crate

The pre-compiled binaries support the following:

- Maya 2016 Windows & Linux
- Maya 2015 Windows & Linux
- Maya 2014 Windows & Linux
- Maya 2013 Windows & Linux
- Maya 2012 Windows & Linux
- Maya 2011 Windows & Linux
- Maya 2010 Windows & Linux
- 3DS Max 2016 Windows
- 3DS Max 2015 Windows
- 3DS Max 2014 Windows
- 3DS Max 2013 Windows
- 3DS Max 2012 Windows
- 3DS Max 2011 Windows
- 3DS Max 2010 Windows
- Softimage 2015 Windows & Linux
- Softimage 2014 Windows & Linux
- Softimage 2013 Windows & Linux
- Softimage 2012 Windows & Linux
- Arnold 4.x Windows & Linux
- Arnold 3.x Windows & Linux
- Python 2.7 Windows
- Python 2.6 Windows & Linux

# License

This software is open source software.  Please consult LICENSE.txt for details.

# Requirements

We use automatically generated projects from a canonical set of CMakeList files.
Thus to create the project files for your platform you will have to install
CMake.

In order to build on Windows platforms, you will require the correct Microsoft C++
for the plugins you require.  If you want to build all that are available,
you will require both Microsoft Visual C++ 2008, 2010 and 2012.  To compile 3DS Max you
must include the MFC libraries when installing MVC++ or your compile will fail.

On Linux, only the standard Gnu tool chain (gcc, gmake) and
cmake is required.  For maximum compatibility, it is recommended that you compile
on Fedora 9 or Fedora 14 (as appropriate) no matter what other operating system you
are using in production.

# External Libraries

To build the plugins you will require the external libraries.  They are specified
programatically in the CMake file.  They usually follow the form:

	Libraries.20160502.02.7z

This external library set will have to be downloaded and un-7z'ed beside the
ExocortexCrate repository, like this:

    /ExocortexCrate/
    /Libraries.20160502.02/

You can find its path when you run CMake, if it doesn't exist, it will tell you
the URL where you can download it.

# Building

Install Cmake and then go into the build folder and run the appropriate batch file:

    cd _build
    build-vs2008_x64.bat        # for Microsoft Visual Studio 2008
    build-vs2010_x64.bat        # for Microsoft Visual Studio 2010
    build-vs2012_x64.bat        # for Microsoft Visual Studio 2012
    build-UnixDebug.sh          # for Linux, Fedora 9 and Fedora 14
    build-UnixRelease.sh        # for linux, Fedora 9 and Fedora 14

The build process will automatically make a deployment.  That deployment is located here:

    ExocortexCrate/install/

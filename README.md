# Exocortex Crate - The Professional Grade Alembic Suite

# Requirements

In order to build on Windows platforms, you will require the correct Microsoft C++
for the plugins you require.  If you want to build all that are available,
you will require both Microsoft Visual C++ 2008, 2010 and 2012.

On Linux, only the standard Gnu tool chain (gcc, gmake) and
cmake is required.  For maximum compatibility, it is recommended that you compile
on Fedora 9 no matter what other operating system you are using in production.

# External Libraries

To build the plugins you will require the external libraries.  They are specified
programatically in the CMake file.  They usually follow the form:

https://s3-us-west-2.amazonaws.com/exocortex-downloads/Libraries.20141211.01.7z

This external library set will have to be downloaded and un-7z'ed beside the
ExocorteXCrate repository, like this:

/ExocortexCrate/
/Libraries.20141211.01/

You can find its path when you run CMake, if it doens't exist, it will tell you
where to download it.

# Building

Install Cmake and then go into the build folder and run the appropriate batch file:

    cd build
    build_vs2008_x64.bat (for Windows Vista, 7 and 8)
    build_vs2008_WinXP_x64.bat (for Windows XP)
    build_unix.sh (for linux)

The build process will automatically make a deployment.  That deployment is located here:

ExocortexCrate/install/

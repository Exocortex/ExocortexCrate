# Exocortex Crate - The Professional Grade Alembic Suite

Exocortex Crate's developer and technical lead is Helge Mathee (helge.mathee@gmx.net)
and is supported by Exocortex Technologies, Inc. (http://www.exocortex.com)

# Building Exocortex Crate

To build all repos in a single place, clone the git repositories
into directories with the repo's name, and ensure to have following
folder structure:

    exocortex
       |-------- ExocortexAlembicShared
       |-------- ExocortexAlembicArnold (optional)
       |-------- ExocortexAlembicPython (optional)
       |-------- ExocortexAlembicSoftimage (optional)
       |-------- ExocortexAlembicMaya (optional)
       |-------- ExocortexAlembic3DSMax (optional)
       |-------- Libraries.########.##

The Libraries folder contains SDKs and other large binaries not suitable
for inclusion in GIT repositories.  We currently distribute it manually.  The
most recent version can be found here:

http://www.exocortex.com/files/Libraries.20120522.01.7z

In order to build on Windows platforms, Visual Studio 2008 (with x64 tools) is
required as is CMake.  After installing Visual Studio 2008 and CMake, you can then
build the solution.

On Linux, only the standard Gnu tool chain (gcc, gmake) and
cmake is required.  For maximum compatibility, it is recommended that you compile
on Fedora 9 no matter what other operating system you are using in production.

Then, use cmake to build the solution. For that cd into ExocortexAlembicShared/build
and run the corresponding batch file, for example

    cd ExocortexAlembicShared/build
    build_vs2008_x64.bat (for Windows Vista, 7 and 8)
    build_vs2008_WinXP_x64.bat (for Windows XP)
    build_unix.sh (for linux)

This will merge all build files into the build folder below ExocortexAlembicShared,
which allows to debug all the way down to the python libs or the hdf5 source.

The build process will automatically make a deployment.  That deployment is located here:

ExocortexAlembicShared/install/

# GitHub Workflow for Committing Changes

We have developed the following policy document that must be followed
for committing changes to the Exocortex Crate Alembic suite:

https://docs.google.com/document/d/1M57fEgEoGoqizzRCbKCePQVGR4IU2IF-4-lIhhHGFfE/edit

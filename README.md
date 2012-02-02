# Build environment for Exocortex Alembic Suite

To build all repos in a single place, clone the git repositories
into directories with the repo's name, and ensure to have following
folder structure:

    exocortex
       |-------- ExocortexAlembicShared
       |-------- ExocortexAlembicArnold (optional)
       |-------- ExocortexAlembicPython (optional)
       |-------- ExocortexAlembicSoftimage (optional)
       |-------- ExocortexAlembicMaya (optional)
       |-------- ExocortexAlembicHoudini (optional)
       |-------- ExocortexAlembic3DSMax (optional)
       |-------- Libraries
   
Then, use cmake to build the solution. For that cd into ExocortexAlembicShared/build
and run the corresponding batch file, for example
     build_vs2008_x64.bat

This will merge all build files into the build folder below ExocortexAlembicShared,
which allows to debug all the way down to the python libs or the hdf5 source.

  
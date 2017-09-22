cmake_minimum_required (VERSION 2.6)

set( Maya_VERSION "2018" )
IF (UNIX AND NOT APPLE )
  add_definitions( -D_MAYA2018LINUX_ )
ENDIF()
include( ../CMakeLists.txt )

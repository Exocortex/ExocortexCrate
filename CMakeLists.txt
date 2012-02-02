SET( PYTHON_LIBS python26)

file(GLOB Sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB Includes ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

SOURCE_GROUP("Source Files" FILES ${Sources})
SOURCE_GROUP("Header Files" FILES ${Includes})

LINK_LIBRARIES( ${ALEMBIC_CORE_LIBS}
			  ${ALEMBIC_HDF5_LIBS}
			  ${PYTHON_LIBS}
			  ${Boost_LIBRARIES}
			  ${ALEMBIC_ILMBASE_LIBS}
			  ${ZLIB_LIBRARIES})

INCLUDE_DIRECTORIES( ${LIBRARY_ROOT}/python/2.6.7/Include )
if(WIN32)
  INCLUDE_DIRECTORIES( ${LIBRARY_ROOT}/python/2.6.7/PC )
  IF(${ALEMBIC64})
          LINK_DIRECTORIES( ${LIBRARY_ROOT}/python/2.6.7/libs/x86_64 )
  ELSE()
          LINK_DIRECTORIES( ${LIBRARY_ROOT}/python/2.6.7/libs/x86 )
  ENDIF()
  ADD_DEFINITIONS( /DBOOST_ALL_NO_LIB )
  SET( MODSUFFIX ".pyd" )
else()
  MESSAGE("WE NEED TO IMPLEMENT LINUX FOR THE PYTHON EXTENSION!")
  SET( MODSUFFIX ".so" )
endif()


ADD_LIBRARY( _ExocortexAlembicPython SHARED ${Sources} ${Includes} )

set_property(TARGET "_ExocortexAlembicPython" PROPERTY FOLDER "${PythonFolder}")

ADD_DEPENDENCIES( _ExocortexAlembicPython AlembicAbcGeom )

SET_TARGET_PROPERTIES(_ExocortexAlembicPython
  PROPERTIES
    OUTPUT_NAME _ExocortexAlembicPython
    PREFIX "" # There is no prefix even on UNIXes
    SUFFIX "${MODSUFFIX}"
)
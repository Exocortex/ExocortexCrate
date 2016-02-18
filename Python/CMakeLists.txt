
project ( Python${Python_SHORTNAME}ExocortexAlembic${crate_VERSION} ) 


if(WIN32) 
  SET( Python_LIBS ${Python_Win32_LIBS} )
else()
  SET( Python_LIBS ${Python_Linux_LIBS} )
endif()
add_definitions( -Dcrate_Python_Version=${Python_SHORTNAME} )

get_filename_component( BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH )
set( LOCAL_CMAKE_FILE_NAME "${BASE_DIR}/CMakeLists.txt" )

file(GLOB Sources ${BASE_DIR}/*.cpp)
file(GLOB Includes ${BASE_DIR}/*.h)

SOURCE_GROUP("Source Files" FILES ${Sources})
SOURCE_GROUP("Header Files" FILES ${Includes})

setup_precompiled_header( ${BASE_DIR} ${Sources} )


LINK_LIBRARIES( CommonUtils
   ${ALL_ALEMBIC_LIBS}
   ${Python_LIBS}
   ${ALL_ALEMBIC_LIBS}
   )

SET( Python_BASE_DIR ${LIBRARY_ROOT}/python/${Python_VERSION} )
if( NOT EXISTS ${Python_BASE_DIR} )
  message(FATAL_ERROR "Can't find Python_BASE_DIR: ${Python_BASE_DIR}" )
endif()

INCLUDE_DIRECTORIES( ${Python_BASE_DIR}/Include )
if(WIN32)
  INCLUDE_DIRECTORIES( ${Python_BASE_DIR}/PC )
  IF(${ALEMBIC64})
          LINK_DIRECTORIES( ${Python_BASE_DIR}/libs/x86_64 )
  ELSE()
          LINK_DIRECTORIES( ${Python_BASE_DIR}/libs/x86 )
  ENDIF()
  ADD_DEFINITIONS( /DBOOST_ALL_NO_LIB )
  SET( MODSUFFIX ".pyd" )
else()

  #if (Python_SHORTNAME MATCHES "27" )
  #  INCLUDE_DIRECTORIES( ${LIBRARY_ROOT}/python/2.7.2/Fedora )
  #  LINK_DIRECTORIES( ${LIBRARY_ROOT}/python/2.7.2/libsFedora )
  #endif()

  if (Python_SHORTNAME MATCHES "26" )
    INCLUDE_DIRECTORIES( ${LIBRARY_ROOT}/python/2.6.7/Fedora )
    #LINK_DIRECTORIES( ${LIBRARY_ROOT}/python/2.6.7/libsFedora )
  endif()

  #if (Python_SHORTNAME MATCHES "25" )
  #  INCLUDE_DIRECTORIES( ${LIBRARY_ROOT}/python/2.5.2/Fedora )
  #endif()
  SET( MODSUFFIX ".so" )

endif()

SET( ${TargetName} _ExocortexAlembicPython )

ADD_LIBRARY( ${PROJECT_NAME} SHARED ${Sources} ${Includes} ${LOCAL_CMAKE_FILE_NAME} )

set_property(TARGET "${PROJECT_NAME}" PROPERTY FOLDER "${PythonFolder}")

ADD_DEPENDENCIES( ${PROJECT_NAME} AlembicAbcGeom )

SET_TARGET_PROPERTIES( ${PROJECT_NAME}
  PROPERTIES
    OUTPUT_NAME _ExocortexAlembicPython
    PREFIX "" # There is no prefix even on UNIXes
    SUFFIX "${MODSUFFIX}"
)

SET( Install_DIR ${Exocortex_INSTALL_BASE_DIR}/Python${Python_SHORTNAME}/ )

get_target_property( Install_TARGET ${PROJECT_NAME} LOCATION )
copy_file_to_directory( ${Install_TARGET} ${Install_DIR} )

# Copy the sample files

SET( Install_SAMPLE_DIR ${Exocortex_INSTALL_BASE_DIR}/Python${Python_SHORTNAME}/samples )
SET( Python_SAMPLE_DIR ${BASE_DIR}/samples )

copy_directory( ${Python_SAMPLE_DIR} ${Install_SAMPLE_DIR} )





project ( Python${Python_SHORTNAME}ExocortexAlembic${alembic_VERSION} ) 

if(WIN32) 
  SET( Python_LIBS ${Python_Win32_LIBS} )
else()
  SET( Python_LIBS ${Python_Linux_LIBS} )
endif()

get_filename_component( BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH )
set( LOCAL_CMAKE_FILE_NAME "${BASE_DIR}/CMakeLists.txt" )

file(GLOB Sources ${BASE_DIR}/*.cpp)
file(GLOB Includes ${BASE_DIR}/*.h)

SOURCE_GROUP("Source Files" FILES ${Sources})
SOURCE_GROUP("Header Files" FILES ${Includes})

if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
	include_directories( ${Rlm_INCLUDE_DIR} )
	include_directories( ${Exocortex_INCLUDE_DIR} )
endif()
 
if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
	link_directories( ${Rlm_LIBRARYDIR} )
endif()

LINK_LIBRARIES( ${ALEMBIC_CORE_LIBS}
			  ${ALEMBIC_HDF5_LIBS}
			  ${Python_LIBS}
			  ${Boost_LIBRARIES}
			  ${ALEMBIC_ILMBASE_LIBS}
			  ${ZLIB_LIBRARIES} ${Rlm_LIBRARIES} )

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
  INCLUDE_DIRECTORIES( ${LIBRARY_ROOT}/python/2.6.7/Fedora )
  LINK_DIRECTORIES( ${LIBRARY_ROOT}/python/2.6.7/libsFedora )
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

copy_file_to_directory( ${RER_DEPENDENCIES} ${Install_DIR} )
copy_file_to_directory( ${EAH_DEPENDENCIES} ${Install_DIR} )

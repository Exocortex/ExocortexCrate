#cmake_minimum_required (VERSION 2.6) 

project ( Maya${Maya_VERSION}ExocortexAlembic${alembic_VERSION} ) 

INCLUDE(../../ExocortexAlembicShared/ExocortexCMakeShared.txt)

if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
	include_directories( ${Rlm_INCLUDE_DIR} )
	include_directories( ${Exocortex_INCLUDE_DIR} )
endif()

SET( Maya_DIR_ROOT ${LIBRARY_ROOT}/Maya/${Maya_VERSION} )
SET( Maya_DIR_INCLUDE ${Maya_DIR_ROOT}/include )

get_filename_component( BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH )
include_directories( ${BASE_DIR}/include )
include_directories( ${BASE_DIR}/src )

file(GLOB_RECURSE Sources ${BASE_DIR}/*.cpp)
file(GLOB_RECURSE Includes ${BASE_DIR}/*.h)

SOURCE_GROUP("Source Files" FILES ${Sources})   
SOURCE_GROUP("Header Files" FILES ${Includes})


IF( WIN32 )
	IF( ${ALEMBIC64} )
		SET( Maya_DIR_LIBRARY ${Maya_DIR_ROOT}/lib/nt-x86-64 )
	ELSE()
		SET( Maya_DIR_LIBRARY ${Maya_DIR_ROOT}/lib/nt-x86 )
	ENDIF()	
ELSEIF( UNIX )
	SET( Maya_DIR_LIBRARY ${Maya_DIR_ROOT}/lib/linux-x86-64 )

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/MayaAlembic.versionScript")
ENDIF()

SET( Maya_LIBRARIES Foundation OpenMaya OpenMayaAnim OpenMayaFX OpenMayaRender OpenMayaUI )		

MESSAGE( "Maya_DIR_LIBRARY: ${Maya_DIR_LIBRARY}" )
MESSAGE( "Maya_LIBRARIES: ${Maya_LIBRARIES}" )

if( Exocortex_SERVICES )
	add_definitions( "-DEXOCORTEX_SERVICES" )
	add_definitions( -DNOMINMAX )	 # disable min/max macros from <windows.h>.
endif()

include_directories( ${Maya_DIR_INCLUDE} )
link_directories( ${Maya_DIR_LIBRARY} )

if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
	link_directories( ${Rlm_LIBRARYDIR} )
endif()

set( MAYA_EXTENSION ".mll" )
set( MAYA_COMPILE_FLAGS "/D \"NT_PLUGIN\" /D \"REQUIRE_IOSTREAM\" /D \"_BOOL\"" )
set( MAYA_LINK_FLAGS " /export:initializePlugin /export:uninitializePlugin " )

add_library( ${PROJECT_NAME} SHARED ${Sources} ${Includes} )

SET_TARGET_PROPERTIES( ${PROJECT_NAME}
  PROPERTIES
  COMPILE_FLAGS ${MAYA_COMPILE_FLAGS}
  LINK_FLAGS ${MAYA_LINK_FLAGS}
  PREFIX ""
  SUFFIX ${MAYA_EXTENSION} )

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${ALEMBIC_ILMBASE_LIBS} ${ALEMBIC_ZLIB_LIBS} ${ALEMBIC_HDF5_LIBS} ${ALEMBIC_CORE_LIBS} ${BOOST_LIBS} ${Maya_LIBRARIES} ${Rlm_LIBRARIES} )
ADD_DEPENDENCIES(${PROJECT_NAME} AlembicAbcGeom)



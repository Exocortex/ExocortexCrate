#cmake_minimum_required (VERSION 2.6) 

project ( Max${Max_VERSION}ExocortexAlembic${alembic_VERSION} ) 

INCLUDE(../../ExocortexAlembicShared/ExocortexCMakeShared.txt) 
 
if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
	include_directories( ${Rlm_INCLUDE_DIR} )
	include_directories( ${Exocortex_INCLUDE_DIR} )
endif()

if( Exocortex_SERVICES )
	add_definitions( "-DEXOCORTEX_SERVICES" )
endif()

add_definitions( "-DEXOCORTEX_BETA_EXPIRY_DATE=1338422400" )	# 1338422400 - May 31, 2012

SET( 3DSMAX_DIR_ROOT ${LIBRARY_ROOT}/Max/${Max_VERSION} ) 
if( NOT EXISTS ${3DSMAX_DIR_ROOT} )
	SET( 3DSMAX_DIR_ROOT ${LIBRARY_ROOT}/Max )
endif()
if( NOT EXISTS ${3DSMAX_DIR_ROOT} )
	message(FATAL_ERROR "Can't find 3DSMAX_DIR_ROOT: ${3DSMAX_DIR_ROOT}" )
endif()
SET( 3DSMAX_DIR_INCLUDE ${3DSMAX_DIR_ROOT}/include )
if( NOT EXISTS ${3DSMAX_DIR_INCLUDE} )
	message(FATAL_ERROR "Can't find 3DSMAX_DIR_INCLUDE: ${3DSMAX_DIR_INCLUDE}" )
endif()
 
get_filename_component( BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH )
set( LOCAL_CMAKE_FILE_NAME "${BASE_DIR}/CMakeLists.txt" )

include_directories( ${BASE_DIR}/Shared )

file(GLOB_RECURSE Shared ${BASE_DIR}/Shared/*.*)
file(GLOB Sources ${BASE_DIR}/*.cpp)
file(GLOB Includes ${BASE_DIR}/*.h)

file(GLOB_RECURSE Scripts ${BASE_DIR}/MaxScripts/*.*)

SOURCE_GROUP("Source Files" FILES ${Sources})   
SOURCE_GROUP("Header Files" FILES ${Includes})
SOURCE_GROUP("_Maxscripts" FILES ${Scripts})

if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
	include_directories( ${Rlm_INCLUDE_DIR} )
	include_directories( ${Exocortex_INCLUDE_DIR} ) 
endif()
 
if( Exocortex_SERVICES OR Exocortex_RLM_ONLY ) 
	link_directories( ${Rlm_LIBRARYDIR} )
endif()

IF( WIN32 )
	IF( ${ALEMBIC64} )
		SET( 3DSMAX_DIR_LIBRARY ${3DSMAX_DIR_ROOT}/x64/lib )
	ELSE()
		SET( 3DSMAX_DIR_LIBRARY ${3DSMAX_DIR_ROOT}/lib )
	ENDIF()	
ENDIF()

SET( 3DSMAX_LIBRARIES core geom gfx maxutil mesh paramblk2 mnmath poly ParticleFlow particle maxscrpt ) 

set( Sources ${Sources} ${BASE_DIR}/Alembic.rc )

include_directories( ${3DSMAX_DIR_INCLUDE} )
link_directories( ${3DSMAX_DIR_LIBRARY} )

if( Exocortex_RLM_ONLY )
	link_directories( ${Rlm_LIBRARYDIR} )
endif()

add_library( ${PROJECT_NAME} SHARED ${Sources} ${Includes}  ${LOCAL_CMAKE_FILE_NAME} ${Scripts} )

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${ALEMBIC_ILMBASE_LIBS}
	${ALEMBIC_ZLIB_LIBS} ${ALEMBIC_HDF5_LIBS} ${ALEMBIC_CORE_LIBS}
	${BOOST_LIBS} ${3DSMAX_LIBRARIES} ${Rlm_LIBRARIES} ${Exocortex_LIBRARIES} )
ADD_DEPENDENCIES( ${PROJECT_NAME} AlembicAbcGeom ) 

SET_TARGET_PROPERTIES( ${PROJECT_NAME} PROPERTIES SUFFIX ".dlu" ) 

set( PluginDirectory "${CMAKE_CURRENT_SOURCE_DIR}/Plugin" )
copy_directory( "${BASE_DIR}/MaxScripts" ${PluginDirectory} )
get_target_property( OutputFilePath ${PROJECT_NAME} LOCATION )

copy_file_to_directory( ${OutputFilePath} ${PluginDirectory} )
copy_file_to_directory( ${RER_DEPENDENCIES} ${PluginDirectory} )

SET( Install_DIR ${Exocortex_INSTALL_BASE_DIR}/Max${Max_VERSION}/ )

copy_directory( "${BASE_DIR}/MaxScripts" ${Install_DIR} )

get_target_property( Install_TARGET ${PROJECT_NAME} LOCATION )
copy_file_to_directory( ${Install_TARGET} ${Install_DIR} )

copy_file_to_directory( ${RER_DEPENDENCIES} ${Install_DIR} )
copy_file_to_directory( ${EAH_DEPENDENCIES} ${Install_DIR} )

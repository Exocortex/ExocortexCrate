#cmake_minimum_required (VERSION 2.6) 

project ( Max${Max_VERSION}ExocortexAlembic${crate_VERSION} ) 

INCLUDE(../../ExocortexCMakeShared.txt NO_POLICY_SCOPE)
  

add_definitions( -D_SCL_SECURE_NO_WARNINGS ) 
add_definitions( -D_CRT_SECURE_NO_WARNINGS )
add_definitions( -Dcrate_Max_Version=${Max_VERSION} )

if( Max_VERSION MATCHES "2013" OR  Max_VERSION MATCHES "2014" OR Max_VERSION MATCHES "2015" OR Max_VERSION MATCHES "2016" OR Max_VERSION MATCHES "2017")
	add_definitions( -D_UNICODE ) 
	add_definitions( -DUNICODE ) 
endif()

if( Max_VERSION MATCHES "2012" OR Max_VERSION MATCHES "2013" OR Max_VERSION MATCHES "2014")
	set( TP ON )
else()
	set( TP OFF )
endif()

SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zm124" )
 
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

if( TP )
	add_definitions( -DTHINKING_PARTICLES ) 

	SET( TP_DIR_ROOT ${LIBRARY_ROOT}/TP/TP5.0_SDK ) 
	if( NOT EXISTS ${TP_DIR_ROOT} )
		message(FATAL_ERROR "Can't find TP_DIR_ROOT: ${TP_DIR_ROOT}" )
	endif()
	SET( TP_DIR_INCLUDE ${TP_DIR_ROOT}/include )
	if( NOT EXISTS ${TP_DIR_INCLUDE} )
		message(FATAL_ERROR "Can't find TP_DIR_INCLUDE: ${TP_DIR_INCLUDE}" )
	endif()
	
	SET( TPL_DIR_ROOT ${LIBRARY_ROOT}/TP/TPLightlineSDK ) 
	if( NOT EXISTS ${TPL_DIR_ROOT} )
		message(FATAL_ERROR "Can't find TPL_DIR_ROOT: ${TPL_DIR_ROOT}" )
	endif()
	SET( TPL_DIR_INCLUDE ${TPL_DIR_ROOT}/include )
	if( NOT EXISTS ${TPL_DIR_INCLUDE} )
		message(FATAL_ERROR "Can't find TPL_DIR_INCLUDE: ${TPL_DIR_INCLUDE}" )
	endif()
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

setup_precompiled_header( ${BASE_DIR} ${Sources} )


IF( WIN32 )
	IF( ${ALEMBIC64} )
		IF( Max_VERSION MATCHES "2015" OR Max_VERSION MATCHES "2016" OR Max_VERSION MATCHES "2017"  )
			SET( 3DSMAX_DIR_LIBRARY ${3DSMAX_DIR_ROOT}/lib/x64/Release )
		ELSE()
			SET( 3DSMAX_DIR_LIBRARY ${3DSMAX_DIR_ROOT}/x64/lib )
		ENDIF()
		SET( TPL_DIR_LIBRARY ${TPL_DIR_ROOT}/${Max_VERSION}/x64/lib )
	ELSE()
	ENDIF()	
ENDIF()

SET( 3DSMAX_LIBRARIES core geom gfx maxutil mesh paramblk2 mnmath poly ParticleFlow particle maxscrpt edmodel) 

set( Sources ${Sources} ${BASE_DIR}/Alembic.rc )

include_directories( ${3DSMAX_DIR_INCLUDE} )
link_directories( ${3DSMAX_DIR_LIBRARY} )

include_directories( ${TP_DIR_INCLUDE} )
link_directories( ${TP_DIR_LIBRARY} )

include_directories( ${TPL_DIR_INCLUDE} )
link_directories( ${TPL_DIR_LIBRARY} )

add_library( ${PROJECT_NAME} SHARED ${Sources} ${Includes}  ${LOCAL_CMAKE_FILE_NAME} ${Scripts} )

TARGET_LINK_LIBRARIES( ${PROJECT_NAME}
	CommonUtils
	${ALL_ALEMBIC_LIBS}
	${3DSMAX_LIBRARIES}
	)
ADD_DEPENDENCIES( ${PROJECT_NAME} AlembicAbcGeom ) 

SET_TARGET_PROPERTIES( ${PROJECT_NAME} PROPERTIES SUFFIX ".dlu" ) 

set( PluginDirectory "${CMAKE_CURRENT_SOURCE_DIR}/Plugin" )

remove_dir( ${PluginDirectory} )

copy_directory( "${BASE_DIR}/MaxScripts" ${PluginDirectory} )
get_target_property( OutputFilePath ${PROJECT_NAME} LOCATION )

copy_file_to_directory( ${OutputFilePath} ${PluginDirectory} )

SET( Install_DIR ${Exocortex_INSTALL_BASE_DIR}/Max${Max_VERSION}/ )

remove_dir( ${Install_DIR} )

copy_directory( "${BASE_DIR}/MaxScripts" ${Install_DIR} )

get_target_property( Install_TARGET ${PROJECT_NAME} LOCATION )
copy_file_to_directory( ${Install_TARGET} ${Install_DIR} )

cmake_minimum_required (VERSION 2.6) 

project ( ExocortexAlembic3dsmax${alembic_VERSION} ) 

INCLUDE(../ExocortexAlembicShared/ExocortexCMakeShared.txt)

if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
	include_directories( ${Rlm_INCLUDE_DIR} )
	include_directories( ${Exocortex_INCLUDE_DIR} )
endif()

SET( 3DSMAX_DIR_ROOT ${LIBRARY_ROOT}/Max )
if( NOT EXISTS ${3DSMAX_DIR_ROOT} )
	message(FATAL_ERROR "Can't find 3DSMAX_DIR_ROOT: ${3DSMAX_DIR_ROOT}" )
endif()
SET( 3DSMAX_DIR_INCLUDE ${3DSMAX_DIR_ROOT}/include )
if( NOT EXISTS ${3DSMAX_DIR_INCLUDE} )
	message(FATAL_ERROR "Can't find 3DSMAX_DIR_INCLUDE: ${3DSMAX_DIR_INCLUDE}" )
endif()

file(GLOB Sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB Includes ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

SOURCE_GROUP("Source Files" FILES ${Sources})  
SOURCE_GROUP("Header Files" FILES ${Includes})

IF( WIN32 )
	IF( ${ALEMBIC64} )
		SET( 3DSMAX_DIR_LIBRARY ${3DSMAX_DIR_ROOT}/x64/lib )
	ELSE()
		SET( 3DSMAX_DIR_LIBRARY ${3DSMAX_DIR_ROOT}/lib )
	ENDIF()	
ELSEIF( UNIX )
	SET( 3DSMAX_DIR_LIBRARY ${3DSMAX_DIR_ROOT}/x64/lib )
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/3dsmaxAlembic.versionScript")
ENDIF()

SET( 3DSMAX_LIBRARIES core geom gfx maxutil mesh paramblk2 mnmath poly ) 

		
MESSAGE( "3DSMAX_DIR_LIBRARY: ${3DSMAX_DIR_LIBRARY}" )

MESSAGE( "3DSMAX_LIBRARIES: ${3DSMAX_LIBRARIES}" )  


set( Sources ${Sources} Alembic.rc )

include_directories( ${3DSMAX_DIR_INCLUDE} )
link_directories( ${3DSMAX_DIR_LIBRARY} )

#if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
#	link_directories( ${Rlm_LIBRARYDIR} )
#endif()
#if( Exocortex_SERVICES )
#	link_directories( ${TBB_LIBRARYDIR} )
#endif()

add_library( ${PROJECT_NAME} SHARED ${Sources} ${Includes})

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${ALEMBIC_ILMBASE_LIBS}
	${ALEMBIC_ZLIB_LIBS} ${ALEMBIC_HDF5_LIBS} ${ALEMBIC_CORE_LIBS}
	${BOOST_LIBS} ${3DSMAX_LIBRARIES} )
	# ${Exocortex_LIBRARIES} ${Rlm_LIBRARIES}
ADD_DEPENDENCIES(${PROJECT_NAME} AlembicAbcGeom) 

SET_TARGET_PROPERTIES( ${PROJECT_NAME} PROPERTIES SUFFIX ".dlu" )

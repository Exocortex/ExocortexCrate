cmake_minimum_required (VERSION 2.6) 

project ( ExocortexAlembicSoftimage${alembic_VERSION} ) 

INCLUDE(../ExocortexAlembicShared/ExocortexCMakeShared.txt)

if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
	include_directories( ${Rlm_INCLUDE_DIR} )
	include_directories( ${Exocortex_INCLUDE_DIR} )
endif()

SET( Softimage_DIR_ROOT ${LIBRARY_ROOT}/Softimage/2012SP1 )
SET( Softimage_DIR_INCLUDE ${Softimage_DIR_ROOT}/include )

file(GLOB Sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB Includes ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

SOURCE_GROUP("Source Files" FILES ${Sources})
SOURCE_GROUP("Header Files" FILES ${Includes})

IF( WIN32 )
	IF( ${ALEMBIC64} )
		SET( Softimage_DIR_LIBRARY ${Softimage_DIR_ROOT}/lib/nt-x86-64 )
	ELSE()
		SET( Softimage_DIR_LIBRARY ${Softimage_DIR_ROOT}/lib/nt-x86 )
	ENDIF()	
	#SET( Softimage_LIBRARIES "${Softimage_DIR_LIBRARY}/sicoresdk.lib" "${Softimage_DIR_LIBRARY}/sicppsdk.lib" )		
ELSEIF( UNIX )
	SET( Softimage_DIR_LIBRARY ${Softimage_DIR_ROOT}/lib/linux-x86-64 )
	#SET( Softimage_LIBRARIES "${Softimage_DIR_LIBRARY}/libsicoresdk.so" "${Softimage_DIR_LIBRARY}/libsicppsdk.so" )		

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/SoftimageAlembic.versionScript")
ENDIF()

SET( Softimage_LIBRARIES sicoresdk sicppsdk )		

MESSAGE( "Softimage_DIR_LIBRARY: ${Softimage_DIR_LIBRARY}" )

MESSAGE( "Softimage_LIBRARIES: ${Softimage_LIBRARIES}" )

if( Exocortex_SERVICES )
	add_definitions( "-DEXOCORTEX_SERVICES" )
	add_definitions( -DNOMINMAX )	 # disable min/max macros from <windows.h>.
endif()

include_directories( ${Softimage_DIR_INCLUDE} )
link_directories( ${Softimage_DIR_LIBRARY} )

if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
	link_directories( ${Rlm_LIBRARYDIR} )
endif()
if( Exocortex_SERVICES )
	#link_directories( ${Exocortex_LIBRARYDIR} )
	link_directories( ${TBB_LIBRARYDIR} )
endif()

add_library( ${PROJECT_NAME} SHARED ${Sources} ${Includes})

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${ALEMBIC_CORE_LIBS} ${BOOST_LIBS} ${ALEMBIC_ILMBASE_LIBS} ${ALEMBIC_ZLIB_LIBS} ${ALEMBIC_HDF5_LIBS} ${Softimage_LIBRARIES} ${Exocortex_LIBRARIES} ${Rlm_LIBRARIES} )
ADD_DEPENDENCIES(${PROJECT_NAME} AlembicAbcGeom)

deploy_softimage( ${PROJECT_NAME} ExocortexAlembic${alembic_VERSION} ${TBB_DEPENDENCIES} ${RER_DEPENDENCIES} ${EAH_DEPENDENCIES} )


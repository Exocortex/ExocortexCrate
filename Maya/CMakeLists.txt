
project ( Maya${Maya_VERSION}ExocortexAlembic ) 

set ( COMPILE_NAME MayaExocortexAlembic )

INCLUDE(../../ExocortexCMakeShared.txt  NO_POLICY_SCOPE)

add_definitions( -Dcrate_Maya_Version=${Maya_VERSION} )

SET( Maya_DIR_ROOT ${LIBRARY_ROOT}/Maya/${Maya_VERSION} )
SET( Maya_DIR_INCLUDE ${Maya_DIR_ROOT}/include )

get_filename_component( BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH )
set( LOCAL_CMAKE_FILE_NAME "${BASE_DIR}/CMakeLists.txt" )

include_directories( ${BASE_DIR}/include )
include_directories( ${BASE_DIR}/src )

file(GLOB_RECURSE Sources ${BASE_DIR}/*.cpp)
list(REMOVE_ITEM Sources ${BASE_DIR}/stdafx.cpp)
file(GLOB_RECURSE Includes ${BASE_DIR}/*.h)
list(REMOVE_ITEM Includes ${BASE_DIR}/stdafx.h)

file(GLOB_RECURSE Scripts ${BASE_DIR}/MEL/*.*)
file(GLOB_RECURSE UI ${BASE_DIR}/UI/*.*)

SOURCE_GROUP("Source Files" FILES ${Sources})   
SOURCE_GROUP("Header Files" FILES ${Includes})
SOURCE_GROUP("_MEL" FILES ${Scripts})
SOURCE_GROUP("_UI" FILES ${UI}) 

setup_precompiled_header( ${BASE_DIR} Sources )

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

#MESSAGE( "Maya_DIR_LIBRARY: ${Maya_DIR_LIBRARY}" )
#MESSAGE( "Maya_LIBRARIES: ${Maya_LIBRARIES}" )

if( WIN32 )
	add_definitions( -DNOMINMAX )	 # disable min/max macros from <windows.h>.
endif()

include_directories( ${Maya_DIR_INCLUDE} )
link_directories( ${Maya_DIR_LIBRARY} )

add_library( ${PROJECT_NAME} SHARED ${Sources} ${Includes} ${LOCAL_CMAKE_FILE_NAME} ${Scripts} ${UI} )

if (WIN32)
	add_definitions( -DNT_PLUGIN )
	add_definitions( -DREQUIRE_IOSTREAM )
	add_definitions( -D_BOOL )

	set( MAYA_EXTENSION ".mll" )
	set( MAYA_LINK_FLAGS " /export:initializePlugin /export:uninitializePlugin " )
	SET_TARGET_PROPERTIES( ${PROJECT_NAME}
		PROPERTIES #
			OUTPUT_NAME ${COMPILE_NAME}
	  		PREFIX ""  # There is no prefix even on UNIXes
	  		LINK_FLAGS ${MAYA_LINK_FLAGS}
			SUFFIX ${MAYA_EXTENSION} )
        set( GL_LIBRARIES "opengl32.lib")
elseif ( UNIX )
	set( MAYA_EXTENSION ".so" )

	SET_TARGET_PROPERTIES( ${PROJECT_NAME}
		PROPERTIES #
			OUTPUT_NAME ${COMPILE_NAME}
			SUFFIX ${MAYA_EXTENSION} )

  set( GL_LIBRARIES "libGL.so")
endif()

#SET_TARGET_PROPERTIES( ${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${COMPILE_NAME} )

TARGET_LINK_LIBRARIES( ${PROJECT_NAME}
	CommonUtils
   ${Maya_LIBRARIES}
   ${GL_LIBRARIES}
   ${ALL_ALEMBIC_LIBS}
   )
	
ADD_DEPENDENCIES(${PROJECT_NAME} AlembicAbcGeom)

SET( Install_DIR ${Exocortex_INSTALL_BASE_DIR}/Maya${Maya_VERSION}/ )

#remove_dir( ${Install_DIR} )
		
SET( Module_FILENAME ${CMAKE_CURRENT_SOURCE_DIR}/Maya${Maya_VERSION}ExocortexAlembic.mod )
copy_file_to_directory( ${Module_FILENAME} ${Install_DIR} )

copy_directory( "${BASE_DIR}/MEL" ${Install_DIR}/Module/scripts )
copy_directory( "${BASE_DIR}/UI" ${Install_DIR}/Module/ui )

get_target_property( Install_TARGET ${PROJECT_NAME} LOCATION )
MESSAGE( "---- Install_TARGET: ${Install_TARGET}" )
copy_file_to_directory( ${Install_TARGET} ${Install_DIR}/Module/plug-ins )

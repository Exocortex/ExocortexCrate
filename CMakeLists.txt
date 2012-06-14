
project ( Maya${Maya_VERSION}ExocortexAlembic${alembic_VERSION} ) 

INCLUDE(../../ExocortexAlembicShared/ExocortexCMakeShared.txt)

if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
	include_directories( ${Rlm_INCLUDE_DIR} )
	include_directories( ${Exocortex_INCLUDE_DIR} )
endif()

add_definitions( "-DEXOCORTEX_BETA_EXPIRY_DATE=1343692800" )	# 1343692800 - July 31, 2012

SET( Maya_DIR_ROOT ${LIBRARY_ROOT}/Maya/${Maya_VERSION} )
SET( Maya_DIR_INCLUDE ${Maya_DIR_ROOT}/include )

get_filename_component( BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH )
set( LOCAL_CMAKE_FILE_NAME "${BASE_DIR}/CMakeLists.txt" )

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


add_library( ${PROJECT_NAME} SHARED ${Sources} ${Includes} ${LOCAL_CMAKE_FILE_NAME} )

if (WIN32)
	add_definitions( -DNT_PLUGIN )
	add_definitions( -DREQUIRE_IOSTREAM )
	add_definitions( -D_BOOL )

	set( MAYA_EXTENSION ".mll" )
	set( MAYA_LINK_FLAGS " /export:initializePlugin /export:uninitializePlugin " )
	SET_TARGET_PROPERTIES( ${PROJECT_NAME}
		PROPERTIES #
	  		PREFIX ""  # There is no prefix even on UNIXes
	  		LINK_FLAGS ${MAYA_LINK_FLAGS}
			SUFFIX ${MAYA_EXTENSION} )
        set( GL_LIBRARIES "opengl32.lib")
elseif ( UNIX )
	set( MAYA_EXTENSION ".so" )
        set( GL_LIBRARIES "libGL.so")
endif()


TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${ALEMBIC_CORE_LIBS} ${BOOST_LIBS} ${ALEMBIC_ILMBASE_LIBS} ${ALEMBIC_ZLIB_LIBS} ${ALEMBIC_HDF5_LIBS} ${Maya_LIBRARIES} ${Rlm_LIBRARIES} ${GL_LIBRARIES} )
ADD_DEPENDENCIES(${PROJECT_NAME} AlembicAbcGeom)

SET( Install_DIR ${Exocortex_INSTALL_BASE_DIR}/Maya${Maya_VERSION}/ )

remove_dir( ${Install_DIR} )

SET( Module_FILENAME ${CMAKE_CURRENT_SOURCE_DIR}/Maya${Maya_VERSION}ExocortexAlembic.mod )
copy_file_to_directory( ${Module_FILENAME} ${Install_DIR} )

copy_directory( "${BASE_DIR}/MEL" ${Install_DIR}/Module/scripts )
copy_directory( "${BASE_DIR}/UI" ${Install_DIR}/Module/ui )

get_target_property( Install_TARGET ${PROJECT_NAME} LOCATION )
copy_file_to_directory( ${Install_TARGET} ${Install_DIR}/Module/plug-ins )

if( WIN32 )
	copy_file_to_directory( ${RER_DEPENDENCIES} ${Install_DIR}/Module/plug-ins )
	copy_file_to_directory( ${EAH_DEPENDENCIES} ${Install_DIR}/Module/plug-ins )
endif()

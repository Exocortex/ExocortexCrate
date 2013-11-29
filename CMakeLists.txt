cmake_minimum_required (VERSION 2.6) 

project ( Softimage${XSI_VERSION}ExocortexAlembic${crate_VERSION} ) 

INCLUDE(../../ExocortexAlembicShared/ExocortexCMakeShared.txt)

if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
	include_directories( ${Rlm_INCLUDE_DIR} )
	include_directories( ${Exocortex_INCLUDE_DIR} )
endif()


add_definitions( -Dcrate_Softimage_Version=${XSI_VERSION} )
if( XSI_VERSION MATCHES "2012")
	SET( Softimage_DIR_ROOT ${LIBRARY_ROOT}/Softimage/2012SP1 )
	SET( Softimage_Version_String "" )
else()
	SET( Softimage_DIR_ROOT ${LIBRARY_ROOT}/Softimage/2014SP2 )
	SET( Softimage_VERSION_STRING "2014" )
endif()
SET( Softimage_DIR_INCLUDE ${Softimage_DIR_ROOT}/include )

get_filename_component( BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH )
file(GLOB Sources ${BASE_DIR}/*.cpp)
file(GLOB Includes ${BASE_DIR}/*.h)

SOURCE_GROUP("Source Files" FILES ${Sources})
SOURCE_GROUP("Header Files" FILES ${Includes})
 
setup_precompiled_header( ${BASE_DIR} ${Sources} )

IF( WIN32 )
	IF( ${ALEMBIC64} )
		SET( Softimage_DIR_LIBRARY ${Softimage_DIR_ROOT}/lib/nt-x86-64 )
	ELSE()
		SET( Softimage_DIR_LIBRARY ${Softimage_DIR_ROOT}/lib/nt-x86 )
	ENDIF()	
ELSEIF( UNIX )
	SET( Softimage_DIR_LIBRARY ${Softimage_DIR_ROOT}/lib/linux-x86-64 )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/SoftimageAlembic.versionScript")
ENDIF()

if( XSI_VERSION MATCHES "2012")
	SET( Softimage_LIBRARIES sicoresdk sicppsdk )
else()	
	SET( Softimage_LIBRARIES Crosswalk )
endif()	

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

TARGET_LINK_LIBRARIES( ${PROJECT_NAME}
   CommonUtils 
   ${ExocortexSoftimage_LIBRARIES}
   ${Rlm_LIBRARIES}
   ${Softimage_LIBRARIES}
   ${ALL_ALEMBIC_LIBS}
   )

ADD_DEPENDENCIES(${PROJECT_NAME} AlembicAbcGeom)

if( WIN32 )
	SET( Install_DIR ${Exocortex_INSTALL_BASE_DIR}/Softimage${Softimage_VERSION_STRING}/Application/Plugins/bin/nt-x86-64 )
elseif( UNIX )
	SET( Install_DIR ${Exocortex_INSTALL_BASE_DIR}/Softimage${Softimage_VERSION_STRING}/Application/Plugins/bin/linux-x86-64 )
else()
	message(FATAL_ERROR "Should not get here")
endif()

remove_dir( ${Install_DIR} )

get_target_property( Install_TARGET ${PROJECT_NAME} LOCATION )

copy_file_to_directory( ${Install_TARGET} ${Install_DIR} )
if( WIN32 )
	copy_file_to_directory( ${RER_DEPENDENCIES} ${Install_DIR} )
	copy_file_to_directory( ${EAH_DEPENDENCIES} ${Install_DIR} )
endif()

SET( CompoundSource_DIR ${BASE_DIR}/Workgroup/Addons/ExocortexAlembic/Data/Compounds )
SET( CompoundInstall_DIR ${Exocortex_INSTALL_BASE_DIR}/Softimage${Softimage_VERSION_STRING}/Data/Compounds )

remove_dir( ${CompoundInstall_DIR} )
copy_directory( ${CompoundSource_DIR} ${CompoundInstall_DIR} )


# -----------------------------------------------------------------------------------
# also distribute all python files located in:
#     ExocortexAlembicSoftimage/Workgroup/Addons/ExocortexAlembic/Application/Plugins

SET( PythonPluginSource_DIR ${BASE_DIR}/Workgroup/Addons/ExocortexAlembic/Application/Plugins )
SET( PythonPluginInstall_DIR ${Exocortex_INSTALL_BASE_DIR}/Softimage${Softimage_VERSION_STRING}/Application/Plugins )

copy_directory( ${PythonPluginSource_DIR} ${PythonPluginInstall_DIR} )


# -----------------------------------------------------------------------------------
# prepare the new target directory for the ArnoldAlembic dll's / dso's
file(MAKE_DIRECTORY ${Exocortex_INSTALL_BASE_DIR}/Softimage${Softimage_VERSION_STRING}/Application/ArnoldAlembicDSO )

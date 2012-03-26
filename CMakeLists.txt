cmake_minimum_required(VERSION 2.6)

if( HDF5_ENABLE_THREADSAFE )
	project( ExocortexAlembic )
else()
	project( ExocortexAlembic_WinXP )
endif()

INCLUDE(./ExocortexCMakeShared.txt)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set( HDF5Folder "Externals" )
set( ZlibFolder "Externals" )
set( IlmBaseFolder "Externals" )
set( AlembicFolder "Externals" )

SET(DIR_SOFTIMAGEALEBMIC ${CMAKE_CURRENT_SOURCE_DIR})

get_filename_component( LIBRARY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" PATH )
set( LIBRARY_ROOT "${LIBRARY_ROOT}/Libraries" )
if( NOT EXISTS ${LIBRARY_ROOT} )
	message(FATAL_ERROR "Can't find LIBRARY_ROOT: ${LIBRARY_ROOT}" )
endif()
 
# Configuration
SET( alembic_MAJOR_VERSION "1" )
SET( alembic_MINOR_VERSION "0" ) 
SET( alembic_VERSION "${alembic_MAJOR_VERSION}.${alembic_MINOR_VERSION}" )

add_definitions( "-Dalembic_MAJOR_VERSION=${alembic_MAJOR_VERSION}" )
add_definitions( "-Dalembic_MINOR_VERSION=${alembic_MINOR_VERSION}" )

if( WIN32 )
	set( Exocortex_SERVICES ON )
elseif( UNIX )
	set( Exocortex_SERVICES OFF )
else()
	message(FATAL_ERROR "Should not get here")
endif()

set( Exocortex_RLM_ONLY ON )					# use only RLM licensing.

if( Exocortex_RLM_ONLY )
	add_definitions( "-DEXOCORTEX_RLM_ONLY" )
endif()

setup_os_name()
setup_cpu_name()

# RLM (floating license manager)
if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
	set( Rlm_INCLUDE_DIR "${LIBRARY_ROOT}/RLM/include" )
	set( Rlm_LIBRARYDIR "${LIBRARY_ROOT}/RLM/libs/${OS_NAME}/${CPU_NAME}" )
	if( WIN32 )
		set( Rlm_LIBRARIES debug "${Rlm_LIBRARYDIR}/rlmclient_mtd.lib" optimized "${Rlm_LIBRARYDIR}/rlmclient.lib" )
	elseif( UNIX )
		set( Rlm_LIBRARIES "${Rlm_LIBRARYDIR}/rlm.a" )
	else()
		message(FATAL_ERROR "Should not get here")
	endif()

	if( NOT EXISTS ${Rlm_INCLUDE_DIR} )
		message(FATAL_ERROR "Can't find Rlm_INCLUDE_DIR: ${Rlm_INCLUDE_DIR}" )
	endif()
	if( NOT EXISTS ${Rlm_LIBRARYDIR} )
		message(FATAL_ERROR "Can't find Rlm_LIBRARYDIR: ${Rlm_LIBRARYDIR}" )
	endif()
endif()



if( Exocortex_SERVICES )
	set( Exocortex_INCLUDE_DIR "${LIBRARY_ROOT}/Exocortex/include" )
	set( Exocortex_DEBUG_LIBRARYDIR "${LIBRARY_ROOT}/Exocortex/lib/${CPU_NAME}/debug/${OS_NAME}/" )
	set( Exocortex_RELEASED_LIBRARYDIR "${LIBRARY_ROOT}/Exocortex/lib/${CPU_NAME}/release/${OS_NAME}/" )
	
	set( Exocortex_LIBRARIES
		debug "${Exocortex_DEBUG_LIBRARYDIR}/LibCore.lib"  optimized "${Exocortex_RELEASED_LIBRARYDIR}/LibCore.lib"
		debug "${Exocortex_DEBUG_LIBRARYDIR}/LibImath.lib"  optimized "${Exocortex_RELEASED_LIBRARYDIR}/LibImath.lib"
		debug "${Exocortex_DEBUG_LIBRARYDIR}/tbb_debug.lib"  optimized "${Exocortex_RELEASED_LIBRARYDIR}/tbb.lib"
		debug "${Exocortex_DEBUG_LIBRARYDIR}/tbbmalloc_debug.lib"  optimized "${Exocortex_RELEASED_LIBRARYDIR}/tbbmalloc.lib"
		)
		
	set( ExocortexSoftimage_LIBRARIES
		debug "${Exocortex_DEBUG_LIBRARYDIR}/LibSoftimageCore.lib"  optimized "${Exocortex_RELEASED_LIBRARYDIR}/LibSoftimageCore.lib"
		debug "${Exocortex_DEBUG_LIBRARYDIR}/LibCopyProtection.lib"  optimized "${Exocortex_RELEASED_LIBRARYDIR}/LibCopyProtection.lib"
		debug "${Exocortex_DEBUG_LIBRARYDIR}/LibFluid.lib"  optimized "${Exocortex_RELEASED_LIBRARYDIR}/LibFluid.lib"
		debug "${Exocortex_DEBUG_LIBRARYDIR}/LibGeometry.lib"  optimized "${Exocortex_RELEASED_LIBRARYDIR}/LibGeometry.lib"
		debug "${Exocortex_DEBUG_LIBRARYDIR}/LibCore.lib"  optimized "${Exocortex_RELEASED_LIBRARYDIR}/LibCore.lib"
		debug "${Exocortex_DEBUG_LIBRARYDIR}/LibCryptopp.lib"  optimized "${Exocortex_RELEASED_LIBRARYDIR}/LibCryptopp.lib"
		debug "${Exocortex_DEBUG_LIBRARYDIR}/LibImath.lib"  optimized "${Exocortex_RELEASED_LIBRARYDIR}/LibImath.lib"
		debug "${Exocortex_DEBUG_LIBRARYDIR}/tbb_debug.lib"  optimized "${Exocortex_RELEASED_LIBRARYDIR}/tbb.lib"
		debug "${Exocortex_DEBUG_LIBRARYDIR}/tbbmalloc_debug.lib"  optimized "${Exocortex_RELEASED_LIBRARYDIR}/tbbmalloc.lib"
		)
	
	set(RER_DEPENDENCIES "${LIBRARY_ROOT}/rer/rer.exe" )
	set(EAH_DEPENDENCIES "${LIBRARY_ROOT}/eah/eah.exe" )

	if( NOT EXISTS ${Exocortex_INCLUDE_DIR} )
		message(FATAL_ERROR "Can't find Exocortex_INCLUDE_DIR: ${Exocortex_INCLUDE_DIR}" )
	endif()
	if( NOT EXISTS ${Exocortex_DEBUG_LIBRARYDIR} )
		message(FATAL_ERROR "Can't find Exocortex_DEBUG_LIBRARYDIR: ${Exocortex_DEBUG_LIBRARYDIR}" )
	endif()
	if( NOT EXISTS ${Exocortex_RELEASED_LIBRARYDIR} )
		message(FATAL_ERROR "Can't find Exocortex_RELEASED_LIBRARYDIR: ${Exocortex_RELEASED_LIBRARYDIR}" )
	endif()
endif()
if( Exocortex_RLM_ONLY )
	add_definitions( "-DEXOCORTEX_RLM_ONLY" )
	set( Exocortex_INCLUDE_DIR "${LIBRARY_ROOT}/Exocortex/include" )

	if( NOT EXISTS ${Exocortex_INCLUDE_DIR} )
		message(FATAL_ERROR "Can't find Exocortex_INCLUDE_DIR: ${Exocortex_INCLUDE_DIR}" )
	endif()
endif()

SET( ALEMBIC64 0 )
IF( WIN32 )
	IF( CMAKE_SIZEOF_VOID_P MATCHES "8" )
		SET( ALEMBIC64 1 )
	ENDIF( CMAKE_SIZEOF_VOID_P MATCHES "8" )
ENDIF( WIN32 )
IF( UNIX )
	IF( CMAKE_SIZEOF_VOID_P MATCHES "8" )
		SET( ALEMBIC64 1 )
	ENDIF( CMAKE_SIZEOF_VOID_P MATCHES "8" )
ENDIF( UNIX )

IF( UNIX )
	# Issues with fpic (requires position independent code option during linking to these libs).
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_RELEASE} -fPIC"  )
	SET(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} -fPIC"  )
ENDIF()

# Boost.
set( BOOST_INCLUDEDIR "${LIBRARY_ROOT}/boost/include/boost-1_44" )
    
if ( UNIX )
	set ( BOOST_OSDIR "linux" )
	set( OS_NAME "linux" )
elseif( WIN32 )
	set ( BOOST_OSDIR "windows" )
	set( OS_NAME "windows" )
else()
	message(FATAL_ERROR "Should not get here") 
endif()

if( CMAKE_SIZEOF_VOID_P MATCHES "8" )
	set( CPU_NAME "x64" )
	set( BOOST_LIBRARYDIR "${LIBRARY_ROOT}/boost/lib/x64/${BOOST_OSDIR}" )
elseif( CMAKE_SIZEOF_VOID_P MATCHES "4" )
	set( CPU_NAME "x86" )
	set( BOOST_LIBRARYDIR "${LIBRARY_ROOT}/boost/lib/x86/${BOOST_OSDIR}" )
else()
	message(FATAL_ERROR "Should not get here")
endif()

MESSAGE( "BOOST_INCLUDEDIR: ${BOOST_INCLUDEDIR}" )
MESSAGE( "BOOST_LIBRARYDIR: ${BOOST_LIBRARYDIR}" )
if( NOT EXISTS ${BOOST_INCLUDEDIR} )
	message(FATAL_ERROR "Can't find BOOST_INCLUDEDIR: ${BOOST_INCLUDEDIR}" )
endif()

if( NOT EXISTS ${BOOST_LIBRARYDIR} )
	message(FATAL_ERROR "Can't find BOOST_LIBRARYDIR: ${BOOST_LIBRARYDIR}" )
endif()

# Boost
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_DETAILED_FAILURE_MSG ON )
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_NO_SYSTEM_PATHS true )
set(BOOST_COMPONENTS date_time program_options thread iostreams python unit_test_framework )
find_package( Boost 1.44.0 COMPONENTS REQUIRED ${BOOST_COMPONENTS} )

include_directories( ${Boost_INCLUDE_DIRS} )
link_directories( ${Boost_LIBRARY_DIRS} )

IF( WIN32 )
	#We statically link to reduce dependancies (copied from Bullet CMakeLists.txt)
	FOREACH(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
		IF(${flag_var} MATCHES "/MD")
			STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
		ENDIF(${flag_var} MATCHES "/MD")
		IF(${flag_var} MATCHES "/MDd")
			STRING(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
		ENDIF(${flag_var} MATCHES "/MDd")
	ENDFOREACH(flag_var)
endif()

# For multithreaded builds		
# [mkonyk] Fix ~ these are only valid on Windows platforms.
IF( WIN32 )
	SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP" )
	SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP" )
	
	# For Alembic library
	SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DPLATFORM_WINDOWS" )
ENDIF( WIN32 )

# ILM Math
include_directories( "ilmbase-1.0.2/Half" )
include_directories( "ilmbase-1.0.2/Iex" )
include_directories( "ilmbase-1.0.2/IlmThread" )
include_directories( "ilmbase-1.0.2/Imath" )

ADD_SUBDIRECTORY ( ilmbase-1.0.2 )
SET( ALEMBIC_ILMBASE_LIBS ilmbase )

# ZLib
SET( BUILD_SHARED_LIBS OFF )
SET( SKIP_INSTALL_LIBRARIES ON )

# For multithreaded builds		
SET( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd" )
SET( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MTd" )
SET( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT" )
SET( CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MT" )

include_directories( "zlib-1.2.5" )
include_directories("${CMAKE_CURRENT_BINARY_DIR}/zlib-1.2.5")

ADD_SUBDIRECTORY ( zlib-1.2.5 )
SET( ALEMBIC_ZLIB_LIBS zlib )

# HDF
SET( ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zlib-1.2.5" )
SET( ZLIB_LIBRARY zlib )
SET(HDF5_ENABLE_THREADSAFE ON CACHE BOOL "Thread safety")
SET(HDF5_ENABLE_SZIP_SUPPORT OFF CACHE BOOL "Disable sZip")
SET(HDF5_BUILD_HL_LIB ON CACHE BOOL "Build high level lib")
ADD_SUBDIRECTORY ( hdf5-1.8.7 )
include_directories( "hdf5-1.8.7/src" )
include_directories( "hdf5-1.8.7/hl/src" )
IF( "${CMAKE_SYSTEM_NAME}" MATCHES "Windows" )
	include_directories( "hdf5-1.8.7/windows/src" )
ENDIF()
include_directories( "hdf5-1.8.7/c++/src" )
include_directories( "hdf5-1.8.7/hl/c++/src" )
SET( ALEMBIC_HDF5_LIBS hdf5 hdf5_hl )

# reset the output directories
UNSET(CMAKE_RUNTIME_OUTPUT_DIRECTORY CACHE)
UNSET(CMAKE_LIBRARY_OUTPUT_DIRECTORY CACHE)
UNSET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY CACHE)

# Alembic
include_directories( "alembic-1.0.4" )
ADD_SUBDIRECTORY ( alembic-1.0.4 )
SET( ALEMBIC_CORE_LIBS AlembicAbcGeom AlembicAbc AlembicAbcCoreHDF5 AlembicAbcCoreAbstract AlembicUtil )

# Python extension
IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembicPython/CMakeLists.txt")
	ADD_SUBDIRECTORY ( "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembicPython" "${CMAKE_CURRENT_BINARY_DIR}/ExocortexAlembicPython" )
ENDIF()

# Arnold dso
IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembicArnold/CMakeLists.txt")
	ADD_SUBDIRECTORY ( "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembicArnold" "${CMAKE_CURRENT_BINARY_DIR}/ExocortexAlembicArnold" )
ENDIF()

# softimage plugin
IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembicSoftimage/CMakeLists.txt")
	ADD_SUBDIRECTORY ( "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembicSoftimage" "${CMAKE_CURRENT_BINARY_DIR}/ExocortexAlembicSoftimage" )
ENDIF()

# maya plugin
IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembicMaya/CMakeLists.txt")
	ADD_SUBDIRECTORY ( "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembicMaya/2011" "${CMAKE_CURRENT_BINARY_DIR}/ExocortexAlembicMaya/2011" )
	ADD_SUBDIRECTORY ( "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembicMaya/2012" "${CMAKE_CURRENT_BINARY_DIR}/ExocortexAlembicMaya/2012" )
ENDIF()

# houdini plugin
#IF(INCLUDE_ALEMBIC_HOUDINI)
#IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembicHoudini/CMakeLists.txt")
#	ADD_SUBDIRECTORY ( "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembicHoudini" "${CMAKE_CURRENT_BINARY_DIR}/ExocortexAlembicHoudini" )
#ENDIF()
#ENDIF(INCLUDE_ALEMBIC_HOUDINI)

# 3dsmax plugin
IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembic3DSMax/CMakeLists.txt")
	ADD_SUBDIRECTORY ( "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembic3DSMax/2010" "${CMAKE_CURRENT_BINARY_DIR}/ExocortexAlembic3DSMax/2010" )
	ADD_SUBDIRECTORY ( "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembic3DSMax/2011" "${CMAKE_CURRENT_BINARY_DIR}/ExocortexAlembic3DSMax/2011" )
	ADD_SUBDIRECTORY ( "${CMAKE_CURRENT_SOURCE_DIR}/../ExocortexAlembic3DSMax/2012" "${CMAKE_CURRENT_BINARY_DIR}/ExocortexAlembic3DSMax/2012" )
ENDIF()


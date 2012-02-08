cmake_minimum_required (VERSION 2.6) 

project (ExocortexAlembicArnold) 

file(GLOB Sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB Includes ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

SOURCE_GROUP("Source Files" FILES ${Sources})
SOURCE_GROUP("Header Files" FILES ${Includes})

if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
	include_directories( ${Rlm_INCLUDE_DIR} )
	include_directories( ${Exocortex_INCLUDE_DIR} )
endif()

if( Exocortex_SERVICES OR Exocortex_RLM_ONLY )
	link_directories( ${Rlm_LIBRARYDIR} )
endif()

SET( EXOCORTEX_ARNOLD_VERSION "4.0.1.0" CACHE STRING "Arnold Version to link against")

IF( "${CMAKE_SYSTEM_NAME}" MATCHES "Windows" )
  SET( Arnold_BASE_DIR "${LIBRARY_ROOT}/arnold/${EXOCORTEX_ARNOLD_VERSION}/windows" )
ELSE() 
  SET( Arnold_BASE_DIR "${LIBRARY_ROOT}/arnold/${EXOCORTEX_ARNOLD_VERSION}/linux" )
ENDIF()

include_directories( "${Arnold_BASE_DIR}/include" )
link_directories( "${Arnold_BASE_DIR}/lib" )

add_library( ${PROJECT_NAME} SHARED ${Sources} ${Includes})
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${ALEMBIC_ILMBASE_LIBS} ${ALEMBIC_ZLIB_LIBS} ${ALEMBIC_HDF5_LIBS} ${ALEMBIC_CORE_LIBS} ${Boost_LIBRARIES}  ${Rlm_LIBRARIES} "ai" )
ADD_DEPENDENCIES(${PROJECT_NAME} AlembicAbcGeom)

cmake_minimum_required (VERSION 2.6) 

project ( CommonUtils ) 

INCLUDE(../ExocortexCMakeShared.txt)

file(GLOB Sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB Includes ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

SOURCE_GROUP("Source Files" FILES ${Sources})
SOURCE_GROUP("Header Files" FILES ${Includes})

add_library( ${PROJECT_NAME} STATIC ${Sources} ${Includes} )

TARGET_LINK_LIBRARIES( ${PROJECT_NAME}
	${ALEMBIC_CORE_LIBS} ${BOOST_LIBS} ${ALEMBIC_ILMBASE_LIBS} ${ALEMBIC_ZLIB_LIBS}
	${ALEMBIC_HDF5_LIBS} )
	
ADD_DEPENDENCIES(${PROJECT_NAME} AlembicAbcGeom)

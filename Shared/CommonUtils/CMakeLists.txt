cmake_minimum_required (VERSION 2.6) 

project ( CommonUtils ) 

INCLUDE(../ExocortexCMakeShared.txt)

file(GLOB Sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB Includes ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

SOURCE_GROUP("Source Files" FILES ${Sources})
SOURCE_GROUP("Header Files" FILES ${Includes})

add_library( ${PROJECT_NAME} STATIC ${Sources} ${Includes} )

TARGET_LINK_LIBRARIES( ${PROJECT_NAME}
   ${ALL_ALEMBIC_LIBS}
   )
	
ADD_DEPENDENCIES(${PROJECT_NAME} AlembicAbcGeom)


SET( DocSource_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Doc )
SET( DocInstall_DIR ${Exocortex_INSTALL_BASE_DIR}/Doc )

remove_dir( ${DocInstall_DIR} )
copy_directory( ${DocSource_DIR} ${DocInstall_DIR} ) 

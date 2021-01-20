cmake_minimum_required(VERSION 2.8)
set(CMAKE_SUPPRESS_REGENERATION "1")

project(MayaMeshTools)

set(CMAKE_DEBUG_POSTFIX "_d")

include_directories(${PROJECT_SOURCE_DIR})
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(WIN32)
	add_definitions(-DNT_PLUGIN)
endif()
if(UNIX)
  add_definitions(-DREQUIRE_IOSTREAM)
  add_compile_options(-fPIC -m64)
  set(PLUGIN_SUFFIX ".so")
  set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
  set(SUFFIX ".so")
  set(STD_INCLUDES "." ".." "/usr/X11R6/include" "/usr/include" "/usr/include/CC")
  include_directories(${STD_INCLUDES})
  add_definitions(-DLINUX)
endif()

subdirs(
  extern/mzd
  )
  
add_library(MayaMeshTools SHARED
	src/FileSystem.h
	src/PluginMain.cpp
	src/MeshLoader.cpp
	src/MeshLoader.h
)

find_package(Maya)
if (MAYA_FOUND)
  include_directories(${MAYA_INCLUDE_DIR})
  target_link_libraries (MayaMeshTools ${MAYA_LIBRARIES})
endif ()

if(WIN32)
  target_link_libraries(MayaMeshTools opengl32.lib glu32.lib mzd)
else()
  target_link_libraries(MayaMeshTools GL GLU mzd)
endif()
add_dependencies(MayaMeshTools mzd)

include_directories(${CMAKE_SOURCE_DIR}/extern/mzd)
include_directories(${CMAKE_SOURCE_DIR}/extern/happly)

add_definitions(-D_CRT_SECURE_NO_WARNINGS) 

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
#set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

MAYA_PLUGIN(MayaMeshTools)

add_custom_command(TARGET MayaMeshTools PRE_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_directory
					${CMAKE_SOURCE_DIR}/scripts $<TARGET_FILE_DIR:MayaMeshTools>/scripts)
					
					
					





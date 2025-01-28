cmake_minimum_required(VERSION 3.30)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED YES)

PROJECT(az)

#if you don't want the full compiler output, remove the following line
SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)

SET(SHADER_SOURCES shaders-dict.c shaders-dict.h)
SET(SMAA_SOURCES smaa-dict.c smaa-dict.h)
set(GLEW_SOURCES ${PROJECT_SOURCE_DIR}/GL/glew.c)

if(NOT WITH_NR)
set(NR_SOURCES ${PROJECT_SOURCE_DIR}/nr/pixblock.c)
endif()

option(SEHLE_USE_NRIMAGE "Use NRImage to set texture images" OFF)

#if(BUILD_TESTING)
#  add_subdirectory(tests)
#endif()

add_subdirectory(shaders)

ADD_CUSTOM_COMMAND(
   OUTPUT ${SHADER_SOURCES}
   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
   COMMAND ${RC666_BINARY} -data shaders-code -dictionary shaders-dict -dictlookupmethod sehle_get_map -dictfreemethod sehle_free_map -dir shaders -files shaders.txt
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders.txt ${SHADER_FILES} ${RC666_BINARY})

add_subdirectory(sehle)

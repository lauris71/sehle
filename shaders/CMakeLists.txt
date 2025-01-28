cmake_minimum_required(VERSION 3.30)

PROJECT(sehle)

#if you don't want the full compiler output, remove the following line
SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)

#SET(GENERATED_SOURCES shaders-dict.c shaders-dict.h)

#SET(SMAA_SOURCES smaa-dict.c)

set(SHADER_FILES STATIC
    SMAA.txt
    control-fragment.txt
    control-vertex.txt
    decodeGBuffer.txt
    depth-fragment.txt
    depth-vertex.txt
    dns-fragment.txt
    dns-vertex.txt
    encodeGBuffer.txt
    exposure.txt
    findTangentSpace.txt
    foliage-gbuf-fragment.txt
    foliage-gbuf-vertex.txt
    light-fragment.txt
    light-vertex.txt
    lightvolume-fragment.txt
    nodes
    occlusion-fragment.txt
    occlusion-vertex.txt
    overlay-fragment.txt
    overlay-vertex.txt
    particles-fragment.txt
    particles-vertex.txt
    plain-fragment.txt
    plain-vertex.txt
    postprocess-blur4x4-fragment.txt
    postprocess-tonemap-fragment.txt
    postprocess-vertex.txt
    sky-fragment.txt
    sky-vertex.txt
    stars-fragment.txt
    stars-vertex.txt
    terrain-gbuf-fragment.txt
    terrain-gbuf-vertex.txt
    water-fragment.txt
    water-vertex.txt)

#ADD_CUSTOM_COMMAND(
#   OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/shaders-dict.c ${CMAKE_CURRENT_SOURCE_DIR}/shaders-dict.h
#   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#   #COMMAND  ${CMAKE_CURRENT_BINARY_DIR}/../rc666/rc666 -dictionary shaders-dict -skipdictheader -dictlookupmethod getMap -data shaders-code -files shaders.txt
#   COMMAND  ${CMAKE_CURRENT_BINARY_DIR}/../rc666/rc666 -dictionary shaders-dict -dictlookupmethod getMap -data shaders-code -files shaders.txt
#   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders.txt ${SHADER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/../rc666/rc666)


cmake_minimum_required(VERSION 3.30)

PROJECT(hello)

#if you don't want the full compiler output, remove the following line
SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)

include(FindOpenGL)

ADD_EXECUTABLE(hello
    hello.c
)

target_include_directories(hello PUBLIC
    ${CMAKE_SOURCE_DIR}/arikkei
    ${CMAKE_SOURCE_DIR}/az
    ${CMAKE_SOURCE_DIR}/elea
    ${CMAKE_SOURCE_DIR}/sehle
    ${SDL2_INCLUDE_DIRS}
)

target_link_directories(hello PUBLIC
    ${CMAKE_BINARY_DIR}/arikkei/arikkei
    ${CMAKE_BINARY_DIR}/az/az
    ${CMAKE_BINARY_DIR}/elea/elea
    ${CMAKE_BINARY_DIR}/sehle/sehle
    ${SDL2_LIBRARY_DIRS}
)

target_link_libraries(hello
    arikkei
    az
    elea
    nr
    sehle
    z
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

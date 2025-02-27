cmake_minimum_required(VERSION 3.30)

PROJECT(hello)

#if you don't want the full compiler output, remove the following line
SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)

include(FindOpenGL)

if(HAS_NR)
set(NR_LIBS nr)
endif()

set(LIBS
    arikkei
    az
    elea
    ${NR_LIBS}
    sehle
    z
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

ADD_EXECUTABLE(hello
    hello.c
)

target_link_libraries(hello
    ${LIBS}
)

ADD_EXECUTABLE(hello2
    hello2.c
)

target_link_libraries(hello2
    ${LIBS}
)

ADD_EXECUTABLE(hello3
    hello3.c
)

target_link_libraries(hello3
    ${LIBS}
)

ADD_EXECUTABLE(hello4
    hello4.c
)

target_link_libraries(hello4
    ${LIBS}
)

include_directories(
    ${CMAKE_SOURCE_DIR}/arikkei
    ${CMAKE_SOURCE_DIR}/az
    ${CMAKE_SOURCE_DIR}/elea
    ${CMAKE_SOURCE_DIR}/sehle
    ${CMAKE_SOURCE_DIR}/nr
    ${SDL2_INCLUDE_DIRS}
)

link_directories(
    ${CMAKE_BINARY_DIR}/arikkei/arikkei
    ${CMAKE_BINARY_DIR}/az/az
    ${CMAKE_BINARY_DIR}/elea/elea
    ${CMAKE_BINARY_DIR}/sehle/sehle
    ${SDL2_LIBRARY_DIRS}
)


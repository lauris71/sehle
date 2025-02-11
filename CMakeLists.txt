cmake_minimum_required(VERSION 3.30)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED YES)

PROJECT(sehle)

#if you don't want the full compiler output, remove the following line
SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)

find_package(SDL2)
find_package(Doxygen)

if(NOT HAS_ARIKKEI)
add_subdirectory(arikkei)
endif()
if(NOT HAS_AZ)
add_subdirectory(az)
endif()
if(NOT HAS_ELEA)
add_subdirectory(elea)
endif()

add_subdirectory(sehle)

if(SDL2_FOUND)
add_subdirectory(tutorial)
endif()

if(DOXYGEN_FOUND)
    configure_file(${PROJECT_SOURCE_DIR}/etc/Doxyfile.in Doxyfile @ONLY )
    add_custom_target(sehle-docs ALL
        Doxygen::doxygen Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
#    install(DIRECTORY ${CMAKE_BINARY_DIR}/doc/ DESTINATION ${CMAKE_INSTALL_DOCDIR} )
#    install(DIRECTORY doc/ DESTINATION ${CMAKE_INSTALL_DOCDIR} )
endif()

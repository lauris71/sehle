SET(SMAA_SOURCES smaa-dict.c smaa-dict.h)
set(GLEW_SOURCES ${PROJECT_SOURCE_DIR}/GL/glew.c)

if(NOT HAS_NR)
	set(NR_SOURCES ${PROJECT_SOURCE_DIR}/nr/pixblock.c)
endif()

SET(SHADER_SOURCES
	${PROJECT_SOURCE_DIR}/sehle/shaders/shaders-dict.c
	${PROJECT_SOURCE_DIR}/sehle/shaders/shaders-dict.h
	${PROJECT_SOURCE_DIR}/sehle/shaders/shaders-code.c
	${PROJECT_SOURCE_DIR}/sehle/shaders/shaders-code.h
)

find_program(RC666 NAMES rc666
    PATHS
        ${CMAKE_SOURCE_DIR}/build/rc666
        ${CMAKE_SOURCE_DIR}/rc666/build
)

if(RC666)
	add_subdirectory(shaders)
endif()

ADD_LIBRARY(sehle STATIC
	sehle.h

	buffer.c buffer.h
	engine.c engine.h
	geometry.c
	index-buffer.c index-buffer.h
	light-directional.c light-directional.h
	light-point.c light-point.h
	light-spot.c light-spot.h
	light-volume.c light-volume.h
	light.c light.h
	material-control.c material-control.h
	material-depth.c material-depth.h
	material-dns.c material-dns.h
	material-foliage.c material-foliage.h
	material-overlay.c material-overlay.h
	material-reflecting.c material-reflecting.h
	material-terrain.c material-terrain.h
	material-water.c material-water.h
	material.c material.h
	octree.c octree.h
	particle-renderer.c particle-renderer.h
	program-occlusion.c program-occlusion.h
	program.c program.h
	render-context.c render-context.h
	render-state.c render-state.h
	render-target-deferred.c render-target-deferred.h
	render-target-texture.c render-target-texture.h
	render-target.c render-target.h
	renderable-collection.c renderable-collection.h
	renderable-list.c renderable-list.h
	renderable.c renderable.h
	resource-manager.c resource-manager.h
	resource.c resource.h
	scene-renderer.c scene-renderer.h
	shader.c shader.h
	shaders-dict.h
	sky-renderer.c sky-renderer.h
	smaa-dict.h
	smaa-renderer.c smaa-renderer.h
	stars-renderer.c stars-renderer.h
	static-mesh.c static-mesh.h
	terrain.c terrain.h
	texture-2d.c texture-2d.h
	texture-cubemap.c texture-cubemap.h
	texture.c texture.h
	ui-renderer.c ui-renderer.h
	vertex-array.c vertex-array.h
	vertex-buffer.c vertex-buffer.h
	${GLEW_SOURCES}
    ${SMAA_SOURCES}
	${SHADER_SOURCES}
	${NR_SOURCES})

target_include_directories(sehle PUBLIC
	${CMAKE_SOURCE_DIR}/arikkei
	${CMAKE_SOURCE_DIR}/az
	${CMAKE_SOURCE_DIR}/elea
	${CMAKE_SOURCE_DIR}/nr
	${CMAKE_SOURCE_DIR}/sehle
	${PROJECT_SOURCE_DIR}
)

set(SHADER_DICT_C ${PROJECT_SOURCE_DIR}/sehle/shaders-dict.c)
set(SHADER_DICT_H ${PROJECT_SOURCE_DIR}/sehle/shaders-dict.h)

#if(RC666)
#message("Shader dict c: ${SHADER_DICT_C}")
#message("Shader dict h: ${SHADER_DICT_H}")
#add_custom_command(
#   OUTPUT ${SHADER_DICT_C} ${SHADER_DICT_H}
#   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/shaders
#   #COMMAND  rc666 -dictionary shaders-dict -skipdictheader -dictlookupmethod getMap -data shaders-code -files shaders.txt
#   COMMAND  ${RC666} -dictionary shaders-dict -dictlookupmethod getMap -data shaders-code -dir ${PROJECT_SOURCE_DIR}/shaders depth-vertex.txt depth-fragment.txt
#   DEPENDS ${SHADER_SOURCES}
#)
#endif()


set(SHADER_FILES
    SMAA.txt
    control-fragment.txt
    control-vertex.txt
    decodeGBuffer.txt
    depth-fragment.txt
    depth-vertex.txt
    dns-fragment.glsl
    dns-vertex.txt
    encodeGBuffer.txt
    exposure.txt
    findTangentSpace.txt
    foliage-gbuf-fragment.txt
    foliage-gbuf-vertex.txt
    light-fragment.glsl
    light-vertex.glsl
    lightvolume-fragment.txt
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
    water-vertex.txt
)

# Empty target needed to enable custom command
add_custom_target(SHADERS ALL
    DEPENDS ${SHADER_SOURCES}
)

add_custom_command(
    OUTPUT ${SHADER_SOURCES}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/sehle/shaders
    COMMAND ${RC666} -data shaders-code -dictionary shaders-dict -dictlookupmethod sehle_get_map -dictfreemethod sehle_free_map ${SHADER_FILES}
    DEPENDS ${SHADER_FILES}
)

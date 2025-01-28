#define __SEHLE_ENGINE_CPP__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2021
 */

static const int debug = 0;

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include <nr/pixblock.h>
#include <elea/geometry.h>
#include <arikkei/arikkei-dict.h>

#include "GL/glew.h"

#ifdef _WIN32
#define strdup _strdup
#endif

#include "engine.h"
#include <sehle/resource-manager.h>
#include "texture-2d.h"
#include "shader.h"
#include "program.h"
#include <sehle/vertex-array.h>
#include "vertex-buffer.h"
#include "index-buffer.h"
#include "render-target.h"
#include "material-overlay.h"
#include "material-control.h"

SehleVertexArray *sehle_build_geometry(SehleEngine *engine, unsigned int type);

static void engine_class_init(SehleEngineClass *klass);
static void engine_init(SehleEngineClass *klass, SehleEngine *engine);
static void engine_finalize(SehleEngineClass *klass, SehleEngine *engine);

unsigned int
sehle_engine_get_type(void) {
    static unsigned int type = 0;
    if (!type) {
        az_register_type(&type, (const unsigned char *) "SehleEngine", AZ_TYPE_BLOCK, sizeof (AZClass), sizeof (SehleEngine), AZ_CLASS_IS_FINAL | AZ_CLASS_ZERO_MEMORY,
                (void (*) (AZClass *)) engine_class_init,
                (void (*) (const AZImplementation *, void *)) engine_init,
                (void (*) (const AZImplementation *, void *)) engine_finalize);
    }
    return type;
}

static void
engine_class_init(SehleEngineClass *klass) {
}

static void
engine_init(SehleEngineClass *klass, SehleEngine *engine) {
    sehle_render_state_init(&engine->render_state);
}

static void
engine_finalize(SehleEngineClass *klass, SehleEngine *engine) {
    sehle_render_state_finalize(&engine->render_state);
    // Unref common textures
    for (unsigned int i = 0; i < SEHLE_NUM_BASIC_TEXTURES; i++) {
        if (engine->textures[i]) {
            az_object_unref(AZ_OBJECT(engine->textures[i]));
        }
    }
    for (unsigned int i = 0; i < SEHLE_NUM_BASIC_GEOMETRIES; i++) {
        if (engine->varrays[i]) {
            az_object_unref((AZObject *) engine->varrays[i]);
        }
    }
    az_instance_delete(SEHLE_TYPE_RESOURCE_MANAGER, engine->resources);
}

static unsigned int glew_initialized = 0;

SehleEngine *
sehle_engine_new(void) {
    if (!glew_initialized) {
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            /* Problem: glewInit failed, something is seriously wrong. */
            fprintf(stderr, "sehle_engine_new: Error: %s\n", glewGetErrorString(err));
        } else {
            glew_initialized = 1;
        }
        if (debug) fprintf(stdout, "sehle_engine_new: Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
        SEHLE_CHECK_ERRORS(0);
        glEnable(GL_FRAMEBUFFER_SRGB);
        SEHLE_CHECK_ERRORS(0);
    }
    SehleEngine *engine = (SehleEngine *) az_instance_new(SEHLE_TYPE_ENGINE);
    engine->resources = sehle_resource_manager_new();

    return engine;
}

void
sehle_engine_delete(SehleEngine *engine) {
    az_instance_delete(SEHLE_TYPE_ENGINE, engine);
}

void
sehle_engine_initialize_render_state(SehleEngine *engine) {
    sehle_render_state_set_default(&engine->render_state);
    if (!engine->running) return;
    glFrontFace(GL_CCW);
    sehle_set_render_state(&engine->render_state);
}

void
sehle_engine_set_render_state(SehleEngine *engine, const SehleRenderState *new_state) {
    sehle_update_render_state(&engine->render_state, new_state);
}

void
sehle_engine_ensure_started(SehleEngine *engine) {
    if (engine->running) return;
    if (!glew_initialized) {
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            /* Problem: glewInit failed, something is seriously wrong. */
            fprintf(stderr, "sehle_engine_ensure_started: Error: %s\n", glewGetErrorString(err));
        } else {
            engine->running = 1;
        }
        if (debug) fprintf(stdout, "sehle_engine_ensure_started: Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
        glew_initialized = 1;
        glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
        engine->running = 1;
    }
    sehle_engine_initialize_render_state(engine);
    SEHLE_CHECK_ERRORS(0);
}

void
sehle_engine_set_render_flags(SehleEngine *engine, unsigned int flags) {
    sehle_update_render_flags(&engine->render_state, flags);
}

void
sehle_engine_set_render_target(SehleEngine *engine, SehleRenderTarget *tgt) {
    arikkei_return_if_fail(engine != NULL);
    arikkei_return_if_fail(SEHLE_IS_ENGINE(engine));
    arikkei_return_if_fail(!tgt || SEHLE_IS_RENDER_TARGET(tgt));
    sehle_update_render_target(&engine->render_state, tgt);
}

void
sehle_engine_set_program(SehleEngine *engine, SehleProgram *prog) {
    arikkei_return_if_fail(engine != NULL);
    arikkei_return_if_fail(SEHLE_IS_ENGINE(engine));
    arikkei_return_if_fail(!prog || SEHLE_IS_PROGRAM(prog));
    sehle_update_program(&engine->render_state, prog);
}

void
sehle_engine_set_texture(SehleEngine *engine, unsigned int idx, SehleTexture *tex) {
    sehle_update_texture(&engine->render_state, idx, tex);
}

void
sehle_engine_set_viewport(SehleEngine *engine, const NRRectl *viewport) {
    sehle_update_viewport(&engine->render_state, viewport);
}

void
sehle_engine_add_resource(SehleEngine *engine, SehleResource *res, unsigned int replace) {
    arikkei_return_if_fail(engine != NULL);
    arikkei_return_if_fail(SEHLE_IS_ENGINE(engine));
    arikkei_return_if_fail(res != NULL);
    arikkei_return_if_fail(SEHLE_IS_RESOURCE(res));
    if (!res->id) return;
    sehle_resource_manager_add_resource(engine->resources, res, replace);
}

void
sehle_engine_remove_resource(SehleEngine *engine, SehleResource *res) {
    arikkei_return_if_fail(engine != NULL);
    arikkei_return_if_fail(SEHLE_IS_ENGINE(engine));
    arikkei_return_if_fail(res != NULL);
    arikkei_return_if_fail(SEHLE_IS_RESOURCE(res));
    if (!res->id) return;
    sehle_resource_manager_remove_resource(engine->resources, res);
}

SehleResource *
sehle_engine_lookup_resource(SehleEngine *engine, unsigned int type, const unsigned char *id) {
    arikkei_return_val_if_fail(engine != NULL, NULL);
    arikkei_return_val_if_fail(SEHLE_IS_ENGINE(engine), NULL);
    arikkei_return_val_if_fail(az_type_is_a(type, SEHLE_TYPE_RESOURCE), NULL);
    if (!id) return NULL;
    return sehle_resource_manager_lookup(engine->resources, type, id);
}


// For some reason VS2010 rand() always gives the same number...

static unsigned int simple_rand(void) {
    static unsigned int m_z = 0x55555555;
    static unsigned int m_w = 0x77777777;
    m_z = 36969 * (m_z & 65535) + (m_z >> 16);
    m_w = 18000 * (m_w & 65535) + (m_w >> 16);
    return (m_z << 16) +m_w;
}

static int
random_int(int range) {
    int r = simple_rand();
    double R = r / (0xffffffff + 1.0);
    return (int) (range * R);
}

static float
random_float(float min, float max) {
    float range = max - min;
    float val = (float) (simple_rand() / (double) 0xffffffff);
    return min + range * val;
}

SehleTexture2D *
sehle_engine_get_standard_texture(SehleEngine *engine, unsigned int type) {
    if (type == SEHLE_TEXTURE_NONE) return NULL;
    if (!engine->textures[type]) {
        NRPixBlock px;
        if (type == SEHLE_TEXTURE_BLACK) {
            static const uint8_t b[] = {0, 0, 0};
            nr_pixblock_setup_extern(&px, NR_PIXBLOCK_MODE_R8G8B8, 0, 0, 1, 1, (uint8_t *) b, 4, 0, 0);
        } else if (type == SEHLE_TEXTURE_WHITE) {
            static const uint8_t b[] = {255, 255, 255};
            nr_pixblock_setup_extern(&px, NR_PIXBLOCK_MODE_R8G8B8, 0, 0, 1, 1, (uint8_t *) b, 4, 0, 0);
        } else if (type == SEHLE_TEXTURE_BLUE) {
            static const uint8_t b[] = {127, 127, 255};
            nr_pixblock_setup_extern(&px, NR_PIXBLOCK_MODE_R8G8B8, 0, 0, 1, 1, (uint8_t *) b, 4, 0, 0);
        } else if (type == SEHLE_TEXTURE_TRANSPARENT) {
            static const uint8_t b[] = {0, 0, 0, 0};
            nr_pixblock_setup_extern(&px, NR_PIXBLOCK_MODE_R8G8B8, 0, 0, 1, 1, (uint8_t *) b, 4, 0, 0);
        } else if (type == SEHLE_TEXTURE_NOISE_16X16) {
            uint8_t b[1024];
            for (unsigned int r = 0; r < 16; r++) {
                for (unsigned int c = 0; c < 16; c++) {
                    for (int i = 0; i < 4; i++) b[64 * r + 4 * c + i] = (unsigned char) random_int(256);
                }
            }
            nr_pixblock_setup_extern(&px, NR_PIXBLOCK_MODE_R8G8B8A8N, 0, 0, 16, 16, (uint8_t *) b, 64, 0, 0);
        }
        px.empty = 0;
        engine->textures[type] = sehle_texture_2d_new(engine, NULL);
        sehle_texture_2d_set_filter(engine->textures[type], SEHLE_TEXTURE_FILTER_NEAREST, SEHLE_TEXTURE_FILTER_NEAREST);
        sehle_texture_2d_set_pixels_from_pixblock(engine->textures[type], &px);
        nr_pixblock_release(&px);
    }
    az_object_ref(AZ_OBJECT(engine->textures[type]));
    return engine->textures[type];
}

static void
engine_build_geometry(SehleEngine *engine, unsigned int type) {
    if (!engine->varrays[type]) {
        engine->varrays[type] = sehle_build_geometry(engine, type);
    }
}

SehleVertexArray *
sehle_engine_get_standard_geometry(SehleEngine *engine, unsigned int type) {
    if (!engine->varrays[type]) engine_build_geometry(engine, type);
    az_object_ref((AZObject *) engine->varrays[type]);
    return engine->varrays[type];
}

SehleVertexBuffer *
sehle_engine_get_vertex_buffer(SehleEngine *engine, const char *id, unsigned int usage) {
    if (id) {
        SehleVertexBuffer *vbuf = (SehleVertexBuffer *) sehle_engine_lookup_resource(engine, SEHLE_TYPE_VERTEX_BUFFER, (const unsigned char *) id);
        if (vbuf && (vbuf->buffer.usage == usage)) {
            az_object_ref(AZ_OBJECT(vbuf));
            return vbuf;
        }
    }
    return sehle_vertex_buffer_new(engine, id, usage);
}

SehleIndexBuffer *
sehle_engine_get_index_buffer(SehleEngine *engine, const char *id, unsigned int usage) {
    if (id) {
        SehleIndexBuffer *ibuf = (SehleIndexBuffer *) sehle_engine_lookup_resource(engine, SEHLE_TYPE_INDEX_BUFFER, (const unsigned char *) id);
        if (ibuf && (ibuf->buffer.usage == usage)) {
            az_object_ref(AZ_OBJECT(ibuf));
            return ibuf;
        }
    }
    return sehle_index_buffer_new(engine, id, usage);
}

SehleShader *
sehle_engine_get_shader(SehleEngine *engine, const char *id, unsigned int shader_type) {
    if (id) {
        SehleShader *res = (SehleShader *) sehle_engine_lookup_resource(engine, SEHLE_TYPE_SHADER, (const unsigned char *) id);
        if (res && (res->shader_type == shader_type)) {
            az_object_ref(AZ_OBJECT(res));
            return (SehleShader *) res;
        }
    }
    return sehle_shader_new(engine, shader_type, (const unsigned char *) id);
}

SehleProgram *
sehle_engine_get_program(SehleEngine *engine, const char *id, unsigned int n_vertex_shaders, unsigned int n_fragment_shaders, unsigned int n_uniforms) {
    if (id) {
        SehleProgram *prog = (SehleProgram *) sehle_engine_lookup_resource(engine, SEHLE_TYPE_PROGRAM, (const unsigned char *) id);
        if (prog && (prog->n_vertex_shaders == n_vertex_shaders) && (prog->n_fragment_shaders == n_fragment_shaders)) {
            az_object_ref(AZ_OBJECT(prog));
            return prog;
        }
    }
    return sehle_program_new(engine, (const unsigned char *) id, n_vertex_shaders, n_fragment_shaders, n_uniforms);
}

#ifdef SEHLE_PERFORMANCE_MONITOR

void
sehle_engine_clear_counter(SehleEngine *engine) {
    memset(&engine->counter, 0, sizeof (SehlePerformanceCounter));
}
#endif

void
sehle_check_error(unsigned int silent, const char *file, unsigned int line) {
    int err_val = glGetError();
    if (err_val != GL_NO_ERROR) {
        if (!silent) {
            fprintf(stderr, "GL Error: %s (%d) at %s:%d\n", sehle_get_error_string(err_val), err_val, file, line);
        }
    }
}

const char *
sehle_get_error_string(unsigned int gl_error) {
    switch (gl_error) {
        case GL_INVALID_ENUM:
            return "INVALID_ENUM";
        case GL_INVALID_VALUE:
            return "INVALID_VALUE";
        case GL_INVALID_OPERATION:
            return "INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
    }
    return "UNKNOWN";
}

const char *
sehle_describe_framebuffer_status(unsigned int status) {
    switch (status) {
        case GL_FRAMEBUFFER_COMPLETE:
            return "Complete";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
            return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            return "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT";
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
            return "GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT";
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_EXT:
            return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_EXT";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT:
            return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT";
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            return "GL_FRAMEBUFFER_UNSUPPORTED_EXT";
        default:
            break;
    }
    return "UNKNOWN ERROR";
}

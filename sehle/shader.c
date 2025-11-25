#define __SEHLE_SHADER_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2018
//

#ifdef DEBUG_OPENGL
static const int debug = 0;
#else
static const int debug = 0;
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <arikkei/arikkei-iolib.h>
#include <arikkei/arikkei-strlib.h>
#include <arikkei/arikkei-utils.h>

#include <GL/glew.h>

#include <az/extend.h>

#include "shaders-dict.h"

#include "engine.h"
#include "shader.h"

static void shader_class_init (SehleShaderClass *klass);
static void shader_finalize (SehleShaderClass *klass, SehleShader *shader);

static void shader_build (SehleResource *res);

static void sehle_shader_clear (SehleShader *shader);

static const uint8_t *default_paths[] = {(const uint8_t *) "."};
static const uint8_t **paths = default_paths;
unsigned int n_paths = 1;

unsigned int
sehle_shader_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_type (&type, (const unsigned char *) "SehleShader", SEHLE_TYPE_RESOURCE, sizeof (SehleShaderClass), sizeof (SehleShader), 0,
			(void (*) (AZClass *)) shader_class_init,
			NULL,
			(void (*) (const AZImplementation *, void *)) shader_finalize);
	}
	return type;
}

static void
shader_class_init (SehleShaderClass *klass)
{
	klass->resource_class.build = shader_build;
}

static void
shader_finalize (SehleShaderClass *klass, SehleShader *shader)
{
	if (shader->resource.gl_handle) {
		glDeleteShader (shader->resource.gl_handle);
	}
	if (shader->code) {
		free (shader->code);
	}
}

static void
shader_build (SehleResource *res)
{
	SehleShader *shader = (SehleShader *) res;
	static const int gl_types[] = { GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER };
	if (shader->resource.gl_handle) return;
	if (!shader->code || sehle_resource_is_invalid (res)) return;
	if (debug) fprintf (stderr, "sehle_shader_get_gl_handle: Shader %s...", shader->resource.id);
	shader->resource.gl_handle = glCreateShader (gl_types[shader->shader_type]);
	if (!shader->resource.gl_handle) {
		sehle_resource_set_sate (res, SEHLE_RESOURCE_STATE_INVALID);
		return;
	}
	glShaderSource (shader->resource.gl_handle, 1, (const char **) &shader->code, NULL);
	glCompileShader (shader->resource.gl_handle);
	int result;
	glGetShaderiv (shader->resource.gl_handle, GL_COMPILE_STATUS, &result);
	// Debug
	int len = 0;
	glGetShaderiv (shader->resource.gl_handle, GL_INFO_LOG_LENGTH, &len);
	if (len > 0) {
		char *log = (char *) malloc (len);
		int written;
		glGetShaderInfoLog (shader->resource.gl_handle, len, &written, log);
		for (int i = 0; i < written; i++) {
			if (isalnum (log[i])) {
				fprintf (stderr, "Shader::getHandle: %s\n", shader->resource.id);
				fprintf (stderr, "%s\n", log);
				break;
			}
		}
		free (log);
	}
	if (result != GL_TRUE) {
		glDeleteShader (shader->resource.gl_handle);
		shader->resource.gl_handle = 0;
		sehle_resource_set_sate (res, SEHLE_RESOURCE_STATE_INVALID);
	} else {
		if (debug) fprintf (stderr, " finished\n");
	}
	sehle_resource_set_sate (res, SEHLE_RESOURCE_STATE_READY);
}

static void
sehle_shader_clear (SehleShader *shader)
{
	if (shader->resource.gl_handle) {
		glDeleteShader (shader->resource.gl_handle);
		shader->resource.gl_handle = 0;
	}
	if (shader->code) {
		free (shader->code);
		shader->code = NULL;
	}
}

SehleShader *
sehle_shader_new (SehleEngine *engine, unsigned int shader_type, const unsigned char *id)
{
	SehleShader *shader = (SehleShader *) az_object_new (SEHLE_TYPE_SHADER);
	sehle_resource_setup (&shader->resource, engine, id);
	shader->shader_type = shader_type;
	return shader;
}

void
sehle_shader_build (SehleShader *shader, const uint8_t *sources[], unsigned int nsources, const int64_t lengths[])
{
	if (debug) fprintf (stderr, "sehle_shader_build: Building %s...", shader->resource.id);
	sehle_shader_clear (shader);
	shader->code = arikkei_strdup_join (sources, nsources, lengths, NULL, 0);

	if (debug) fprintf (stderr, " finished\n");
	sehle_resource_set_sate (&shader->resource, SEHLE_RESOURCE_STATE_MODIFIED);
}

void
sehle_shader_build_from_file (SehleShader *shader, const unsigned char *filename)
{
	sehle_shader_build_from_files (shader, 1, &filename);
}

void
sehle_shader_build_from_files (SehleShader *shader, unsigned int nfiles, const uint8_t *filenames[])
{
	arikkei_return_if_fail (nfiles < 32);
	if (debug) fprintf (stderr, "sehle_shader_build_from_files: Building %s\n", shader->resource.id);
	const uint8_t *sources[32];
	int64_t lengths[32];
	unsigned int ismap[32];
	for (unsigned int i = 0; i < nfiles; i++) {
		unsigned int len;
		sources[i] = sehle_shader_map (filenames[i], &len, &ismap[i]);
		lengths[i] = len;
	}
	sehle_shader_build (shader, sources, nfiles, lengths);
	for (unsigned int i = 0; i < nfiles; i++) {
		if (ismap[i]) arikkei_munmap (sources[i], lengths[i]);
	}
}

void
sehle_shader_build_from_header_files (SehleShader *shader, const unsigned char *hdata, int hsize, const unsigned char *filenames[], unsigned int nfiles)
{
	arikkei_return_if_fail (nfiles < 31);
	if (debug) fprintf (stderr, "sehle_shader_build_from_header_files: Building %s\n", shader->resource.id);
	const uint8_t *sources[32];
	int64_t lengths[32];
	unsigned int ismap[32];
	sources[0] = hdata;
	lengths[0] = hsize;
	for (unsigned int i = 0; i < nfiles; i++) {
		unsigned int len;
		sources[i + 1] = sehle_shader_map (filenames[i], &len, &ismap[i + 1]);
		lengths[i + 1] = len;
	}
	sehle_shader_build (shader, sources, nfiles + 1, lengths);
	for (unsigned int i = 0; i < nfiles; i++) {
		if (ismap[i + 1]) arikkei_munmap (sources[i + 1], lengths[i + 1]);
	}
}

void
sehle_shader_build_from_data (SehleShader *shader, const unsigned char *cdata, unsigned int csize)
{
	int64_t csize64 = csize;
	sehle_shader_build (shader, &cdata, 1, &csize64);
}

SehleShader *
sehle_shader_fetch_from_file (SehleEngine *engine, const char *filename, unsigned int type)
{
	SehleShader *shader = sehle_engine_get_shader (engine, filename, type);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		sehle_shader_build_from_file (shader, (const unsigned char *) filename);
	}
	return shader;
}

const uint8_t *
sehle_shader_map (const uint8_t *key, unsigned int *size, unsigned int *is_map)
{
	const unsigned char *cdata = NULL;
	for (unsigned int i = 0; i < n_paths; i++) {
		const uint8_t *srcs[] = {paths[i], key};
		const int64_t lens[] = {-1, -1};
		uint8_t *path = arikkei_strdup_join(srcs, 2, lens, (const uint8_t *) "/", -1);
		uint64_t msize;
		cdata = arikkei_mmap(path, &msize);
		if (cdata) {
			if (size) *size = (unsigned int) msize;
			if(is_map) *is_map = 1;
			return cdata;
		}
	}
	cdata = sehle_get_map (key, size);
	if (!cdata) {
		fprintf (stderr, "sehle_shader_map: Cannot map file %s\n", key);
	}
	if (is_map) *is_map = 0;
	return cdata;
}



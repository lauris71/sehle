#define __SEHLE_PROGRAM_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2011
//

#ifdef DEBUG_OPENGL
static const int debug = 1;
#else
static const int debug = 0;
#endif

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "GL/glew.h"

#include <az/extend.h>

#include "engine.h"
#include <sehle/material.h>
#include "shader.h"
#include <sehle/vertex-buffer.h>

#include "program.h"

const char *sehle_attribute_names[] = {
	"vertex", "normal", "texcoord", "texcoord1", "color", "properties", "bones", "weights"
};

const char *sehle_uniforms[] = {
	"o2v", "projection", "o2v_projection", "clip_plane", "as2s"
};

struct _SehleProgramVariable {
	const unsigned char *name;
	int location;
};

static void program_class_init (SehleProgramClass *klass);
static void program_init (SehleProgramClass *klass, SehleProgram *prog);
static void program_finalize (SehleProgramClass *klass, SehleProgram *prog);

static void program_shutdown (AZObject *obj);
static void program_build (SehleResource *res);

static void detach_all (SehleProgram *prog);
static void link (SehleProgram *prog);

static unsigned int program_type = 0;
static SehleProgramClass *program_class = NULL;
static SehleResourceClass *parent_class = NULL;

unsigned int
sehle_program_get_type (void)
{
	if (!program_type) {
		program_class = (SehleProgramClass *) az_register_type (&program_type, (const unsigned char *) "SehleProgram", SEHLE_TYPE_RESOURCE, sizeof (SehleProgramClass), sizeof (SehleProgram), 0,
			(void (*) (AZClass *)) program_class_init,
			(void (*) (const AZImplementation *, void *)) program_init,
			(void (*) (const AZImplementation *, void *)) program_finalize);
		parent_class = (SehleResourceClass *) az_class_parent((AZClass *) program_class);
	}
	return program_type;
}

static void
program_class_init (SehleProgramClass *klass)
{
	klass->resource_class.active_object_class.object_class.shutdown = program_shutdown;
	klass->resource_class.build = program_build;
}

static void
program_init (SehleProgramClass *klass, SehleProgram *prog)
{
	az_object_list_setup (&prog->shaders, SEHLE_TYPE_SHADER, 0);
}

static void
program_finalize (SehleProgramClass *klass, SehleProgram *prog)
{
	az_object_list_release (&prog->shaders);
}

static void
program_shutdown (AZObject *obj)
{
	SehleProgram *prog = (SehleProgram *) obj;
	detach_all (prog);
	az_object_list_clear (&prog->shaders);
	if (prog->uniforms) {
		free (prog->uniforms);
		prog->uniforms = NULL;
	}
	parent_class->active_object_class.object_class.shutdown (obj);
}

static void
program_build (SehleResource *res)
{
	SehleProgram *prog = (SehleProgram *) res;
	if (prog->resource.gl_handle) detach_all (prog);
	link (prog);
}

static void
link (SehleProgram *prog)
{
	unsigned int i;
	if (sehle_resource_is_invalid (&prog->resource)) return;
	if (prog->resource.gl_handle) return;
	if (debug > 1) fprintf (stderr, "Program::link: Program %s...\n", prog->resource.id);
	prog->resource.gl_handle = glCreateProgram ();
	if (!prog->resource.gl_handle) {
		sehle_resource_set_sate (&prog->resource, SEHLE_RESOURCE_STATE_INVALID);
		return;
	}
	/* fixme: Manage attachment programs from material side */
	glBindFragDataLocation (prog->resource.gl_handle, 0, "color_fragment");
	glBindFragDataLocation (prog->resource.gl_handle, 0, "normal_fragment");
	glBindFragDataLocation (prog->resource.gl_handle, 1, "albedo_fragment");
	glBindFragDataLocation (prog->resource.gl_handle, 2, "specular_shininess_fragment");

	for (i = 0; i < SEHLE_NUM_ATTRIBUTE_NAMES; i++) {
		glBindAttribLocation (prog->resource.gl_handle, i, sehle_attribute_names[i]);
	}

	//glBindFragDataLocation (prog->gl_handle, 3, "occlusion_fragment");
	for (unsigned int i = 0; i < prog->shaders.length; i++) {
		glAttachShader (prog->resource.gl_handle, sehle_resource_get_handle ((SehleResource *) prog->shaders.objects[i]));
	}
	glLinkProgram (prog->resource.gl_handle);
	int result = 0;
	glGetProgramiv (prog->resource.gl_handle, GL_LINK_STATUS, &result);
	int len = 0;
	glGetProgramiv (prog->resource.gl_handle, GL_INFO_LOG_LENGTH, &len);
	// Debug
	if (len > 1) {
		char *log = (char *) malloc (len);
		int written;
		glGetProgramInfoLog (prog->resource.gl_handle, len, &written, log);
		for (int i = 0; i < written; i++) {
			if (isalnum (log[i])) {
				fprintf (stderr, "Program::link: %s\n", prog->resource.id);
				fprintf (stderr, "%s\n", log);
				break;
			}
		}
		free (log);
	}
	if (result != GL_TRUE) {
		detach_all (prog);
		sehle_resource_set_sate (&prog->resource, SEHLE_RESOURCE_STATE_INVALID);
		return;
	}
	/* Initialize variable locations */
	for (unsigned int i = 0; i < prog->n_uniforms; i++) {
		prog->uniforms[i].location = glGetUniformLocation (prog->resource.gl_handle, (const char *) prog->uniforms[i].name);;
	}
	if (debug > 1) fprintf (stderr, "Program::link: finished\n");
	sehle_resource_set_sate (&prog->resource, SEHLE_RESOURCE_STATE_READY);
}

SehleProgram *
sehle_program_new (SehleEngine *engine, const unsigned char *id, unsigned int n_vertex_shaders, unsigned int n_fragment_shaders, unsigned int n_uniforms)
{
	unsigned int i;
	SehleProgram *prog = (SehleProgram *) az_object_new (SEHLE_TYPE_PROGRAM);
	sehle_resource_setup (SEHLE_RESOURCE(prog), engine, id);
	prog->n_vertex_shaders = n_vertex_shaders;
	prog->n_fragment_shaders = n_fragment_shaders;
	prog->n_uniforms = n_uniforms;
	prog->uniforms = (SehleProgramVariable *) malloc (n_uniforms * sizeof (SehleProgramVariable));
	for (i = 0; i < n_uniforms; i++) {
		prog->uniforms[i].name = NULL;
		prog->uniforms[i].location = -1;
	}
	return prog;
}

void
sehle_program_add_shader (SehleProgram *prog, SehleShader *shader)
{
	az_object_list_append_object (&prog->shaders, &shader->resource.active_object.object);
	sehle_resource_set_sate (&prog->resource, SEHLE_RESOURCE_STATE_MODIFIED);
}

void
sehle_program_set_uniform_names (SehleProgram *prog, unsigned int first, unsigned int n_uniforms, const unsigned char *names[])
{
	unsigned int i;
	arikkei_return_if_fail (first + n_uniforms <= prog->n_uniforms);
	detach_all (prog);
	for (i = 0; i < n_uniforms; i++) {
		prog->uniforms[first + i].name = names[i];
		prog->uniforms[first + i].location = -1;
	}
	sehle_resource_set_sate (&prog->resource, SEHLE_RESOURCE_STATE_MODIFIED);
}

int
sehle_program_get_uniform_location (SehleProgram *prog, unsigned int uniform)
{
	arikkei_return_val_if_fail (!sehle_resource_is_invalid (&prog->resource), -1);
	arikkei_return_val_if_fail ((uniform >= 0) && (uniform < prog->n_uniforms), -1);
	if (sehle_resource_is_modified (&prog->resource)) link (prog);
	return prog->uniforms[uniform].location;
}

unsigned int
sehle_program_setUniform1i (SehleProgram *prog, unsigned int uniform, int value)
{
	int loc = sehle_program_get_uniform_location (prog, uniform);
	if (loc >= 0) glUniform1i (loc, value);
	return loc >= 0;
}

unsigned int
sehle_program_setUniform1iv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, const int values[])
{
	int loc = sehle_program_get_uniform_location (prog, uniform);
	if (loc >= 0) glUniform1iv (loc, nvalues, values);
	return loc >= 0;
}

unsigned int
sehle_program_setUniform1f (SehleProgram *prog, unsigned int uniform, float value)
{
	int loc = sehle_program_get_uniform_location (prog, uniform);
	if (loc >= 0) glUniform1f (loc, value);
	return loc >= 0;
}

unsigned int
sehle_program_setUniform1fv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, const float values[])
{
	int loc = sehle_program_get_uniform_location (prog, uniform);
	if (loc >= 0) glUniform1fv (loc, nvalues, values);
	return loc >= 0;
}

unsigned int
sehle_program_setUniform2fv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, const float values[])
{
	int loc = sehle_program_get_uniform_location (prog, uniform);
	if (loc >= 0) glUniform2fv (loc, nvalues, values);
	return loc >= 0;
}

unsigned int
sehle_program_setUniform3fv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, const float values[])
{
	int loc = sehle_program_get_uniform_location (prog, uniform);
	if (loc >= 0) glUniform3fv (loc, nvalues, values);
	return loc >= 0;
}

unsigned int
sehle_program_setUniform4fv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, const float values[])
{
	int loc = sehle_program_get_uniform_location (prog, uniform);
	if (loc >= 0) glUniform4fv (loc, nvalues, values);
	return loc >= 0;
}

unsigned int
sehle_program_setUniformMatrix3fv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, const float values[])
{
	int loc = sehle_program_get_uniform_location (prog, uniform);
	if (loc >= 0) glUniformMatrix3fv (loc, nvalues, 0, values);
	return loc >= 0;
}

unsigned int
sehle_program_setUniformMatrix4fv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, const float values[])
{
	int loc = sehle_program_get_uniform_location (prog, uniform);
	if (loc >= 0) glUniformMatrix4fv (loc, nvalues, 0, values);
	return loc >= 0;
}

unsigned int
sehle_program_setUniformMatrix4x3fv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, unsigned int transpose, const float values[])
{
	int loc = sehle_program_get_uniform_location (prog, uniform);
	if (loc >= 0) glUniformMatrix4x3fv (loc, nvalues, transpose, values);
	SEHLE_CHECK_ERRORS (0);
	return loc >= 0;
}

static void
detach_all (SehleProgram *prog)
{
	if (prog->resource.gl_handle) {
		for (unsigned int i = 0; i < prog->shaders.length; i++) {
			glDetachShader (prog->resource.gl_handle, ((SehleResource *) prog->shaders.objects[i])->gl_handle);
			SEHLE_CHECK_ERRORS (0);
		}
		glDeleteProgram (prog->resource.gl_handle);
		prog->resource.gl_handle = 0;
		SEHLE_CHECK_ERRORS (0);
	}
}


#ifndef __SEHLE_PROGRAM_H__
#define __SEHLE_PROGRAM_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2015
//

typedef struct _SehleProgram SehleProgram;
typedef struct _SehleProgramClass SehleProgramClass;
typedef struct _SehleProgramVariable SehleProgramVariable;

#define SEHLE_TYPE_PROGRAM (sehle_program_get_type ())
#define SEHLE_PROGRAM(p) (AZ_CHECK_INSTANCE_CAST ((p), SEHLE_TYPE_PROGRAM, SehleProgram))
#define SEHLE_IS_PROGRAM(p) (AZ_CHECK_INSTANCE_TYPE ((p), SEHLE_TYPE_PROGRAM))

extern const char *sehle_attribute_names[];

#include <az/classes/object-list.h>

#include <sehle/resource.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleProgram {
	SehleResource resource;
	AZObjectList shaders;
	unsigned int n_vertex_shaders;
	unsigned int n_fragment_shaders;
	unsigned int n_uniforms;
	SehleProgramVariable *uniforms;
};

struct _SehleProgramClass {
	SehleResourceClass resource_class;
};

unsigned int sehle_program_get_type (void);

SehleProgram *sehle_program_new (SehleEngine *engine, const unsigned char *id, unsigned int n_vertex_shaders, unsigned int n_fragment_shaders, unsigned int n_uniforms);

/* Add shader and grab reference */
void sehle_program_add_shader (SehleProgram *prog, SehleShader *shader);

/* Names should be statically allocated or managed by client */
void sehle_program_set_uniform_names (SehleProgram *prog, unsigned int first, unsigned int n_uniforms, const unsigned char *names[]);
int sehle_program_get_uniform_location (SehleProgram *prog, unsigned int uniform);

unsigned int sehle_program_setUniform1i (SehleProgram *prog, unsigned int uniform, int value);
unsigned int sehle_program_setUniform1iv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, const int values[]);
unsigned int sehle_program_setUniform1f (SehleProgram *prog, unsigned int uniform, float value);
unsigned int sehle_program_setUniform1fv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, const float values[]);
unsigned int sehle_program_setUniform2fv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, const float values[]);
unsigned int sehle_program_setUniform3fv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, const float values[]);
unsigned int sehle_program_setUniform4fv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, const float values[]);
unsigned int sehle_program_setUniformMatrix3fv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, const float values[]);
unsigned int sehle_program_setUniformMatrix4fv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, const float values[]);
unsigned int sehle_program_setUniformMatrix4x3fv (SehleProgram *prog, unsigned int uniform, unsigned int nvalues, unsigned int transpose, const float values[]);

#ifdef __cplusplus
};
#endif

#endif


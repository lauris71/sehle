#define __SEHLE_MATERIAL_CONTROL_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2010
//

#include <stdlib.h>
#include <stdio.h>

#include <az/class.h>

#include "GL/glew.h"

#include "engine.h"
#include <sehle/index-buffer.h>
#include "shader.h"
#include "texture.h"
#include <sehle/render-context.h>
#include "program.h"
#include "texture-2d.h"
#include "vertex-buffer.h"

#include "material-control.h"

static SehleShader *
control_get_shader (SehleEngine *engine, unsigned int shader_type, unsigned int flags, unsigned int n_lights)
{
	char c[256];
	sprintf (c, "Sehle::ControlProgram::%s_C%uT%u_L%u",
		(shader_type == SEHLE_SHADER_VERTEX) ? "Vertex" : "Fragment",
		(flags & SEHLE_PROGRAM_CONTROL_HAS_COLORS) != 0,
		(flags & SEHLE_PROGRAM_CONTROL_HAS_TEXTURE) != 0,
		n_lights);
	SehleShader *shader = sehle_engine_get_shader (engine, c, shader_type);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		const char *sources[1];
		sprintf (c, "#version 140\n#define %s\n#define NUM_LIGHTS %u\n#define HAS_COLORS %u\n#define HAS_TEXTURE %u\n",
			(shader_type == SEHLE_SHADER_VERTEX) ? "VS" : "FS",
			n_lights,
			(flags & SEHLE_PROGRAM_CONTROL_HAS_COLORS) != 0,
			(flags & SEHLE_PROGRAM_CONTROL_HAS_TEXTURE) != 0);
		sources[0] = (shader_type == SEHLE_SHADER_VERTEX) ? "control-vertex.txt" : "control-fragment.txt";
		sehle_shader_build_from_header_files (shader, (const unsigned char *) c, -1, (const unsigned char **) sources, 1);
	}
	return shader;
}

static const char *uniforms[] = {
	"diffuse", "colorSampler", "opacity", "ambient", "light_direction", "light_color"
};

SehleProgram *
sehle_program_control_get_reference (SehleEngine *engine, unsigned int flags, unsigned int n_lights)
{
	char c[256];
	sprintf (c, "Sehle::ControlProgram_C%uT%u_L%u",
		(flags & SEHLE_PROGRAM_CONTROL_HAS_COLORS) != 0,
		(flags & SEHLE_PROGRAM_CONTROL_HAS_TEXTURE) != 0,
		n_lights);
	SehleProgram *prog = sehle_engine_get_program (engine, c, 1, 1, SEHLE_PROGRAM_CONTROL_NUM_UNIFORMS);
	if (!sehle_resource_is_initialized (&prog->resource)) {
		sehle_program_add_shader (prog, control_get_shader (engine, SEHLE_SHADER_VERTEX, flags, n_lights));
		sehle_program_add_shader (prog, control_get_shader (engine, SEHLE_SHADER_FRAGMENT, flags, n_lights));
		sehle_program_set_uniform_names (prog, 0, SEHLE_NUM_UNIFORMS, (const unsigned char **) sehle_uniforms);
		sehle_program_set_uniform_names (prog, SEHLE_NUM_UNIFORMS, SEHLE_PROGRAM_CONTROL_NUM_UNIFORMS - SEHLE_NUM_UNIFORMS, (const unsigned char **) uniforms);
	}
	return prog;
}

static void material_control_class_init (SehleMaterialControlClass *klass);
static void material_control_init (SehleMaterialControlClass *klass, SehleMaterialControl *mctrl);

static void material_control_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type);

static void material_control_update_program_flags (SehleMaterialControl *mctrl);
static void material_control_update_programs (SehleMaterialControl *mctrl, SehleEngine *engine);

static unsigned int material_control_type = 0;
SehleMaterialControlClass *sehle_material_control_class;

unsigned int
sehle_material_control_get_type (void)
{
	if (!material_control_type) {
		az_register_type (&material_control_type, (const unsigned char *) "SehleMaterialControl", AZ_TYPE_BLOCK, sizeof (SehleMaterialControlClass), sizeof (SehleMaterialControl), AZ_CLASS_ZERO_MEMORY,
			(void (*) (AZClass *)) material_control_class_init,
			(void (*) (const AZImplementation *, void *)) material_control_init,
			NULL);
		sehle_material_control_class = (SehleMaterialControlClass *) az_type_get_class (material_control_type);
	}
	return material_control_type;
}

static void
material_control_class_init (SehleMaterialControlClass *klass)
{
	az_class_set_num_interfaces ((AZClass *) klass, 1);
	az_class_declare_interface ((AZClass *) klass, 0, SEHLE_TYPE_MATERIAL,
		ARIKKEI_OFFSET(SehleMaterialControlClass, material_impl),
		ARIKKEI_OFFSET(SehleMaterialControl, material_inst));
	klass->material_impl.bind = material_control_bind;
}

static void
material_control_init (SehleMaterialControlClass *klass, SehleMaterialControl *mctrl)
{
	sehle_material_setup (&mctrl->material_inst, 1, 1);
	mctrl->material_inst.render_stages = SEHLE_STAGE_FORWARD;
	mctrl->material_inst.render_types = SEHLE_RENDER_FORWARD;
	mctrl->color = EleaColor4fWhite;
	mctrl->opacity = 1;
}

static void
material_control_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
	SehleMaterialControl *mctrl = SEHLE_MATERIAL_CONTROL_FROM_MATERIAL_INSTANCE (inst);
	if (!mctrl->material_inst.program_initialized) {
		material_control_update_programs (mctrl, ctx->engine);
	}
	SehleProgram *prog = mctrl->material_inst.programs[0];
	sehle_render_context_set_program (ctx, prog);
	sehle_program_setUniform4fv (prog, SEHLE_PROGRAM_CONTROL_DIFFUSE, 1, mctrl->color.c);
	sehle_program_setUniform1f (prog, SEHLE_PROGRAM_CONTROL_OPACITY, mctrl->opacity);
	if (mctrl->program_flags & SEHLE_PROGRAM_CONTROL_HAS_TEXTURE) {
		sehle_material_instance_bind_texture (inst, ctx, 0, 0, SEHLE_PROGRAM_CONTROL_COLOR_SAMPLER);
	}
	if (mctrl->n_lights) {
		sehle_program_setUniform4fv (prog, SEHLE_PROGRAM_CONTROL_AMBIENT, 1, mctrl->ambient.c);
		sehle_program_setUniform3fv (prog, SEHLE_PROGRAM_CONTROL_LIGHT_DIRECTION, 1, mctrl->light_directions[0].c);
		sehle_program_setUniform4fv (prog, SEHLE_PROGRAM_CONTROL_LIGHT_COLOR, 1, mctrl->light_colors[0].c);
	}
}

static void
material_control_update_program_flags (SehleMaterialControl *mctrl)
{
	unsigned int new_flags = 0;
	if (mctrl->has_colors) new_flags |= SEHLE_PROGRAM_CONTROL_HAS_COLORS;
	if (mctrl->material_inst.textures[0]) new_flags |= SEHLE_PROGRAM_CONTROL_HAS_TEXTURE;
	if (new_flags != mctrl->program_flags) {
		mctrl->program_flags = new_flags;
		mctrl->material_inst.program_initialized = 0;
	}
}

static void
material_control_update_programs (SehleMaterialControl *mctrl, SehleEngine *engine)
{
	if (mctrl->material_inst.programs[0]) az_object_unref (AZ_OBJECT (mctrl->material_inst.programs[0]));
	mctrl->material_inst.programs[0] = sehle_program_control_get_reference (engine, mctrl->program_flags, mctrl->n_lights);
	mctrl->material_inst.program_initialized = 1;
}

SehleMaterialControl *
sehle_material_control_new (SehleEngine *engine)
{
	SehleMaterialControl *mctrl = (SehleMaterialControl *) malloc (sizeof (SehleMaterialControl));
	az_instance_init (mctrl, SEHLE_TYPE_MATERIAL_CONTROL);
	sehle_material_setup (&mctrl->material_inst, 1, 1);
	return mctrl;
}

void
sehle_material_control_delete (SehleMaterialControl *mctrl)
{
	sehle_material_release (&mctrl->material_inst);
	az_instance_finalize (mctrl, SEHLE_TYPE_MATERIAL_CONTROL);
	free (mctrl);
}

void
sehle_material_control_set_texture (SehleMaterialControl *mff, SehleTexture2D *tex2d)
{
	sehle_material_set_texture (&mff->material_inst, 0, (SehleTexture *) tex2d);
	material_control_update_program_flags (mff);
}

void
sehle_material_control_set_has_colors (SehleMaterialControl* mctrl, unsigned int has_colors)
{
	mctrl->has_colors = has_colors;
	material_control_update_program_flags (mctrl);
}

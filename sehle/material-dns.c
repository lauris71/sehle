#define __SEHLE_MATERIAL_DNS_C__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2021
 */

#include <stdlib.h>
#include <stdio.h>

#include <az/class.h>

#include "GL/glew.h"

#include "engine.h"
#include <sehle/index-buffer.h>
#include <sehle/render-context.h>
#include "light.h"
#include "program.h"
#include "shader.h"
#include "texture-2d.h"
#include "vertex-buffer.h"
#include "light-point.h"
#include "light-spot.h"
#include <sehle/material-depth.h>

#include "material-dns.h"

static SehleShader *
dns_get_shader (SehleEngine *engine, unsigned int shader_type, unsigned int flags, unsigned int output)
{
	char c[256];
	sprintf (c, "Sehle::DNSProgram::%s_C%dD%dN%dS%dA%dB%d_O%d",
		(shader_type == SEHLE_SHADER_VERTEX) ? "Vertex" : "Fragment",
		(flags & SEHLE_PROGRAM_DNS_HAS_COLORS) != 0,
		(flags & SEHLE_PROGRAM_DNS_HAS_DIFFUSE) != 0,
		(flags & SEHLE_PROGRAM_DNS_HAS_NORMAL) != 0,
		(flags & SEHLE_PROGRAM_DNS_HAS_SPECULAR) != 0,
		(flags & SEHLE_PROGRAM_DNS_HAS_AMBIENT) != 0,
		(flags & SEHLE_PROGRAM_DNS_HAS_BONES) != 0,
		output);
	SehleShader *shader = sehle_engine_get_shader (engine, c, shader_type);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		static const char *outputs[] = { "GBUFFER", "BLEND", "DENSITY" };
		const char *sources[1];
		sprintf (c, "#version 140\n"
			"#define %s\n"
			"#define HAS_COLORS %d\n"
			"#define HAS_DIFFUSE_TEXTURE %d\n"
			"#define HAS_NORMAL_TEXTURE %d\n"
			"#define HAS_SPECULAR_TEXTURE %d\n"
			"#define HAS_AMBIENT_TEXTURE %d\n"
			"#define HAS_SKIN %d\n"
			"#define %s 1\n",
			(shader_type == SEHLE_SHADER_VERTEX) ? "VS" : "FS",
			(flags & SEHLE_PROGRAM_DNS_HAS_COLORS) != 0,
			(flags & SEHLE_PROGRAM_DNS_HAS_DIFFUSE) != 0,
			(flags & SEHLE_PROGRAM_DNS_HAS_NORMAL) != 0,
			(flags & SEHLE_PROGRAM_DNS_HAS_SPECULAR) != 0,
			(flags & SEHLE_PROGRAM_DNS_HAS_AMBIENT) != 0,
			(flags & SEHLE_PROGRAM_DNS_HAS_BONES) != 0,
			outputs[output]);
		sources[0] = (shader_type == SEHLE_SHADER_VERTEX) ? "dns-vertex.txt" : "dns-fragment.txt";
		sehle_shader_build_from_header_files (shader, (const unsigned char *) c, -1, (const unsigned char **) sources, 1);
	}
	return shader;
}

static const char *uniforms[] = {
	"colorSampler", "normalSampler", "specularSampler", "ambientSampler",
	"diffuse", "specular", "shininess", "ambient",
	"light_ambient", "light_diffuse", "light_direct",
	"light_pos", "light_dir", "point_attenuation", "spot_attenuation",
	"as2s"
};

SehleProgram *
sehle_program_dns_get_reference (SehleEngine *engine, unsigned int flags, unsigned int output)
{
	char c[256];
	sprintf (c, "Sehle::DNSProgram_C%dD%dN%dS%dA%dB%d_O%d",
		(flags & SEHLE_PROGRAM_DNS_HAS_COLORS) != 0,
		(flags & SEHLE_PROGRAM_DNS_HAS_DIFFUSE) != 0,
		(flags & SEHLE_PROGRAM_DNS_HAS_NORMAL) != 0,
		(flags & SEHLE_PROGRAM_DNS_HAS_SPECULAR) != 0,
		(flags & SEHLE_PROGRAM_DNS_HAS_AMBIENT) != 0,
		(flags & SEHLE_PROGRAM_DNS_HAS_BONES) != 0,
		output);
	SehleProgram *prog = sehle_engine_get_program (engine, c, 1, 3, SEHLE_PROGRAM_DNS_NUM_UNIFORMS);
	if (!sehle_resource_is_initialized (&prog->resource)) {
		sehle_program_add_shader (prog, dns_get_shader (engine, SEHLE_SHADER_VERTEX, flags, output));
		sehle_program_add_shader (prog, dns_get_shader (engine, SEHLE_SHADER_FRAGMENT, flags, output));
		SehleShader *shader = sehle_shader_fetch_from_file (engine, "findTangentSpace.txt", SEHLE_SHADER_FRAGMENT);
		sehle_program_add_shader (prog, shader);
		shader = sehle_shader_fetch_from_file (engine, "encodeGBuffer.txt", SEHLE_SHADER_FRAGMENT);
		sehle_program_add_shader (prog, shader);
		sehle_program_set_uniform_names (prog, 0, SEHLE_NUM_UNIFORMS, (const unsigned char **) sehle_uniforms);
		sehle_program_set_uniform_names (prog, SEHLE_NUM_UNIFORMS, SEHLE_PROGRAM_DNS_NUM_UNIFORMS - SEHLE_NUM_UNIFORMS, (const unsigned char**) uniforms);
	}
	return prog;
}

enum {
	DNS_PROGRAM_DEPTH,
	DNS_PROGRAM_DENSITY,
	DNS_PROGRAM_TRANSPARENCY,
	DNS_PROGRAM_GBUFFER,
	DNS_NUM_PROGRAMS
};

static void material_dns_class_init (SehleMaterialDNSClass *klass);
static void material_dns_init (SehleMaterialDNSClass *klass, SehleMaterialDNS *dns);
/* Material implementation */
static void material_dns_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type);

static void material_dns_update_program_flags (SehlematerialDNS *dns);
static void material_dns_update_programs (SehleMaterialInstance *inst, SehleEngine *engine, unsigned int flags);

unsigned int sehle_material_dns_type = 0;
SehleMaterialDNSClass *sehle_material_dns_class;

unsigned int
sehle_material_dns_get_type (void)
{
	if (!sehle_material_dns_type) {
		az_register_type (&sehle_material_dns_type, (const unsigned char *) "SehleMaterialDNS", AZ_TYPE_BLOCK, sizeof (SehleMaterialDNSClass), sizeof (SehleMaterialDNS), AZ_CLASS_ZERO_MEMORY,
			(void (*) (AZClass *)) material_dns_class_init,
			(void (*) (const AZImplementation *, void *)) material_dns_init,
			NULL);
		sehle_material_dns_class = (SehleMaterialDNSClass *) az_type_get_class (sehle_material_dns_type);
	}
	return sehle_material_dns_type;
}

static void
material_dns_class_init (SehleMaterialDNSClass *klass)
{
	az_class_set_num_interfaces ((AZClass *) klass, 1);
	az_class_declare_interface ((AZClass *) klass, 0, SEHLE_TYPE_MATERIAL,
		ARIKKEI_OFFSET(SehleMaterialDNSClass, material_impl),
		ARIKKEI_OFFSET(SehleMaterialDNS, material_inst));
	klass->material_impl.bind = material_dns_bind;
}

static void
material_dns_init (SehleMaterialDNSClass *klass, SehleMaterialDNS *dns)
{
	sehle_material_setup (&dns->material_inst, DNS_NUM_PROGRAMS, SEHLE_MATERIAL_DNS_NUM_MAPS);
	dns->material_inst.render_stages = SEHLE_STAGE_SOLID | SEHLE_STAGE_TRANSPARENT;
	dns->material_inst.render_types = SEHLE_RENDER_DEPTH | SEHLE_RENDER_DENSITY | SEHLE_RENDER_TRANSPARENT | SEHLE_RENDER_GBUFFER;
	dns->diffuse = EleaColor4fWhite;
	dns->specular = EleaColor4fTransparent;
	dns->shininess = 1;
	dns->opacity = 1;
	dns->program_flags = 0;
}

static void
material_dns_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
	SehleMaterialDNS *dns = SEHLE_MATERIAL_DNS_FROM_MATERIAL_INSTANCE (inst);
	if (!dns->material_inst.program_initialized) {
		material_dns_update_programs (&dns->material_inst, ctx->engine, dns->program_flags);
	}

	EleaPlane3f clip_view;
	elea_mat3x4f_transform_plane (&clip_view, &ctx->w2v, &ctx->clip_plane);

	unsigned int pidx = 0;
	if (render_type == SEHLE_RENDER_DEPTH) {
		pidx = DNS_PROGRAM_DEPTH;
	} else if (render_type == SEHLE_RENDER_DENSITY) {
		pidx = DNS_PROGRAM_DENSITY;
	} else if (render_type == SEHLE_RENDER_TRANSPARENT) {
		pidx = DNS_PROGRAM_TRANSPARENCY;
	} else if (render_type == SEHLE_RENDER_GBUFFER) {
		pidx = DNS_PROGRAM_GBUFFER;
	}
	SehleProgram *prog = dns->material_inst.programs[pidx];
	sehle_render_context_set_program (ctx, prog);

	sehle_program_setUniform4fv (prog, SEHLE_UNIFORM_CLIP_PLANE, 1, clip_view.c);

	if (render_type == SEHLE_RENDER_DEPTH) {
		sehle_material_instance_bind_texture (&dns->material_inst, ctx, pidx, SEHLE_MATERIAL_DNS_MAP_DIFFUSE, SEHLE_PROGRAM_DEPTH_COLOR_SAMPLER);
		sehle_program_setUniform4fv (prog, SEHLE_PROGRAM_DEPTH_DIFFUSE, 1, dns->diffuse.c);
		sehle_program_setUniform1f (prog, SEHLE_PROGRAM_DEPTH_OPACITY, dns->opacity);
		return;
	}

	sehle_program_setUniform4fv (prog, SEHLE_PROGRAM_DNS_DIFFUSE, 1, dns->diffuse.c);
	if (dns->program_flags & SEHLE_PROGRAM_DNS_HAS_DIFFUSE) {
		sehle_material_instance_bind_texture (inst, ctx, pidx, SEHLE_MATERIAL_DNS_MAP_DIFFUSE, SEHLE_PROGRAM_DNS_COLOR_SAMPLER);
	}

	if (render_type == SEHLE_RENDER_TRANSPARENT) {
		if (dns->program_flags & SEHLE_PROGRAM_DNS_HAS_NORMAL) {
			sehle_material_instance_bind_texture (inst, ctx, pidx, SEHLE_MATERIAL_DNS_MAP_NORMAL, SEHLE_PROGRAM_DNS_NORMAL_SAMPLER);
		}
		if (dns->program_flags & SEHLE_PROGRAM_DNS_HAS_SPECULAR) {
			sehle_material_instance_bind_texture (inst, ctx, pidx, SEHLE_MATERIAL_DNS_MAP_SPECULAR, SEHLE_PROGRAM_DNS_SPECULAR_SAMPLER);
		} else {
			sehle_program_setUniform4fv (prog, SEHLE_PROGRAM_DNS_SPECULAR, 1, dns->specular.c);
		}
		sehle_program_setUniform1f (prog, SEHLE_PROGRAM_DNS_SHININESS, dns->shininess);
		if (dns->program_flags & SEHLE_PROGRAM_DNS_HAS_AMBIENT) {
			sehle_material_instance_bind_texture (inst, ctx, pidx, SEHLE_MATERIAL_DNS_MAP_AMBIENT, SEHLE_PROGRAM_DNS_AMBIENT_SAMPLER);
		} else {
			sehle_program_setUniform1f (prog, SEHLE_PROGRAM_DNS_AMBIENT, 1.0f);
		}
		/* Light colors */
		EleaVec3f light_colors[4];
		memset (light_colors, 0, sizeof light_colors);
		for (unsigned int i = 0; i < ctx->numlights; i++) {
			memcpy (light_colors[i].c, ctx->lights[i]->ambient.c, sizeof light_colors[i]);
		}
		sehle_program_setUniform3fv (prog, SEHLE_PROGRAM_DNS_LIGHT_AMBIENT, 4, light_colors[0].c);
		for (unsigned int i = 0; i < ctx->numlights; i++) {
			memcpy (light_colors[i].c, ctx->lights[i]->diffuse.c, sizeof light_colors[i]);
		}
		sehle_program_setUniform3fv (prog, SEHLE_PROGRAM_DNS_LIGHT_DIFFUSE, 4, light_colors[0].c);
		for (unsigned int i = 0; i < ctx->numlights; i++) {
			memcpy (light_colors[i].c, ctx->lights[i]->direct.c, sizeof light_colors[i]);
		}
		sehle_program_setUniform3fv (prog, SEHLE_PROGRAM_DNS_LIGHT_DIRECT, 4, light_colors[0].c);
		/* Light position */
		EleaVec4f light_pos[4];
		memset (light_pos, 0, sizeof (light_pos));
		for (unsigned int i = 0; i < ctx->numlights; i++) {
			EleaVec3f t;
			elea_mat3x4f_get_translation (&t, &ctx->lights[i]->l2w);
			elea_mat3x4f_transform_point ((EleaVec3f *) &light_pos[i], &ctx->w2v, &t);
			light_pos[i].w = 0;
		}
		sehle_program_setUniform4fv (prog, SEHLE_PROGRAM_DNS_LIGHT_POS, 4, light_pos[0].c);
		EleaVec3f light_dir[4];
		memset (light_dir, 0, sizeof light_dir);
		for (unsigned int i = 0; i < ctx->numlights; i++) {
			elea_mat3x4f_get_col_vec (&light_dir[i], &ctx->lights[i]->l2w, 2);
			light_dir[i] = elea_vec3f_inv (light_dir[i]);
		}
		sehle_program_setUniform3fv (prog, SEHLE_PROGRAM_DNS_LIGHT_DIR, 4, light_dir[0].c);
		/* Attenuations */
		EleaVec4f point_attenuation[4];
		memset (point_attenuation, 0, sizeof (point_attenuation));
		for (unsigned int i = 0; i < ctx->numlights; i++) {
			memcpy (point_attenuation[i].c, ctx->lights[i]->point_attenuation, sizeof (point_attenuation[i]));
		}
		sehle_program_setUniform4fv (prog, SEHLE_PROGRAM_DNS_POINT_ATTENUATION, 4, point_attenuation[0].c);
		EleaVec3f spot_attenuation[4];
		memset (spot_attenuation, 0, sizeof (spot_attenuation));
		for (unsigned int i = 0; i < ctx->numlights; i++) {
			memcpy (spot_attenuation[i].c, ctx->lights[i]->spot_attenuation, sizeof spot_attenuation[i]);
		}
		sehle_program_setUniform3fv (prog, SEHLE_PROGRAM_DNS_SPOT_ATTENUATION, 4, spot_attenuation[0].c);
	}
	if (render_type == SEHLE_RENDER_GBUFFER) {
		if (dns->program_flags & SEHLE_PROGRAM_DNS_HAS_NORMAL) {
			sehle_material_instance_bind_texture (inst, ctx, pidx, SEHLE_MATERIAL_DNS_MAP_NORMAL, SEHLE_PROGRAM_DNS_NORMAL_SAMPLER);
		}
		if (dns->program_flags & SEHLE_PROGRAM_DNS_HAS_SPECULAR) {
			sehle_material_instance_bind_texture (inst, ctx, pidx, SEHLE_MATERIAL_DNS_MAP_SPECULAR, SEHLE_PROGRAM_DNS_SPECULAR_SAMPLER);
		} else {
			sehle_program_setUniform4fv (prog, SEHLE_PROGRAM_DNS_SPECULAR, 1, dns->specular.c);
		}
		sehle_program_setUniform1f (prog, SEHLE_PROGRAM_DNS_SHININESS, dns->shininess);
		if (dns->program_flags & SEHLE_PROGRAM_DNS_HAS_AMBIENT) {
			sehle_material_instance_bind_texture (inst, ctx, pidx, SEHLE_MATERIAL_DNS_MAP_AMBIENT, SEHLE_PROGRAM_DNS_AMBIENT_SAMPLER);
		} else {
			sehle_program_setUniform1f (prog, SEHLE_PROGRAM_DNS_AMBIENT, 1.0f);
		}
	}
}

static void
material_dns_update_program_flags (SehlematerialDNS *dns)
{
	unsigned int new_flags = 0;
	if (dns->has_colors) new_flags |= SEHLE_PROGRAM_DNS_HAS_COLORS;
	if (dns->skinned) {
		new_flags |= SEHLE_PROGRAM_DNS_HAS_BONES;
	}
	if (dns->material_inst.textures[SEHLE_MATERIAL_DNS_MAP_DIFFUSE] != NULL) new_flags |= SEHLE_PROGRAM_DNS_HAS_DIFFUSE;
	if (dns->material_inst.textures[SEHLE_MATERIAL_DNS_MAP_NORMAL] != NULL) new_flags |= SEHLE_PROGRAM_DNS_HAS_NORMAL;
	if (dns->material_inst.textures[SEHLE_MATERIAL_DNS_MAP_SPECULAR] != NULL) new_flags |= SEHLE_PROGRAM_DNS_HAS_SPECULAR;
	if (dns->material_inst.textures[SEHLE_MATERIAL_DNS_MAP_AMBIENT] != NULL) new_flags |= SEHLE_PROGRAM_DNS_HAS_AMBIENT;
	if (new_flags != dns->program_flags) {
		dns->program_flags = new_flags;
		dns->material_inst.program_initialized = 0;
	}
}

static void
material_dns_update_programs (SehleMaterialInstance *inst, SehleEngine *engine, unsigned int flags)
{
	if (inst->programs[DNS_PROGRAM_DEPTH]) az_object_unref (AZ_OBJECT (inst->programs[DNS_PROGRAM_DEPTH]));
	unsigned int depth_flags = 0;
	if (flags & SEHLE_PROGRAM_DNS_HAS_DIFFUSE) depth_flags |= SEHLE_PROGRAM_DEPTH_HAS_TEXTURE;
	if (flags & SEHLE_PROGRAM_DNS_HAS_BONES) depth_flags |= SEHLE_PROGRAM_DEPTH_HAS_BONES;
	inst->programs[DNS_PROGRAM_DEPTH] = sehle_depth_program_get_reference (engine, depth_flags, 1);

	if (inst->programs[DNS_PROGRAM_GBUFFER]) az_object_unref (AZ_OBJECT (inst->programs[DNS_PROGRAM_GBUFFER]));
	inst->programs[DNS_PROGRAM_GBUFFER] = sehle_program_dns_get_reference (engine, flags, SEHLE_PROGRAM_DNS_OUTPUT_GBUFFER);
	if (inst->programs[DNS_PROGRAM_TRANSPARENCY]) az_object_unref (AZ_OBJECT (inst->programs[DNS_PROGRAM_TRANSPARENCY]));
	inst->programs[DNS_PROGRAM_TRANSPARENCY] = sehle_program_dns_get_reference (engine, flags, SEHLE_PROGRAM_DNS_OUTPUT_BLEND);
	if (inst->programs[DNS_PROGRAM_DENSITY]) az_object_unref (AZ_OBJECT (inst->programs[DNS_PROGRAM_DENSITY]));
	inst->programs[DNS_PROGRAM_DENSITY] = sehle_program_dns_get_reference (engine, flags & (SEHLE_PROGRAM_DNS_HAS_COLORS | SEHLE_PROGRAM_DNS_HAS_DIFFUSE), SEHLE_PROGRAM_DNS_OUTPUT_DENSITY);
	inst->program_initialized = 1;
}

SehleMaterialDNS *
sehle_material_dns_new (SehleEngine *engine)
{
	SehleMaterialDNS *dns = (SehleMaterialDNS *) malloc (sizeof (SehleMaterialDNS));
	az_instance_init (dns, SEHLE_TYPE_MATERIAL_DNS);
	return dns;
}

void
sehle_material_dns_delete (SehleMaterialDNS *dns)
{
	az_instance_finalize (dns, SEHLE_TYPE_MATERIAL_DNS);
	free (dns);
}

void
sehle_material_dns_set_has_colors (SehleMaterialDNS *mdns, unsigned int has_colors)
{
	mdns->has_colors = has_colors;
	material_dns_update_program_flags (mdns);
}

void
sehle_material_dns_set_skinned (SehleMaterialDNS *mdns, unsigned int skinned)
{
	mdns->skinned = skinned;
	material_dns_update_program_flags (mdns);
}

void
sehle_material_dns_set_texture (SehleMaterialDNS *mdns, unsigned int map_type, SehleTexture2D *tex2d)
{
	sehle_material_set_texture (&mdns->material_inst, map_type, (SehleTexture *) tex2d);
	material_dns_update_program_flags (mdns);
}

void
sehle_material_dns_set_transparent (SehleMaterialDNS *mdns, unsigned int transparent, unsigned int sort)
{
	if (transparent) {
		sehle_render_flags_set (&mdns->material_inst.state_flags, SEHLE_BLEND);
		mdns->material_inst.render_stages = SEHLE_STAGE_TRANSPARENT;
		//mdns->material_inst.render_types = SEHLE_RENDER_DEPTH | SEHLE_RENDER_DENSITY | SEHLE_RENDER_TRANSPARENT;
		mdns->material_inst.render_types = SEHLE_RENDER_DENSITY | SEHLE_RENDER_TRANSPARENT;
	} else {
		sehle_render_flags_clear (&mdns->material_inst.state_flags, SEHLE_BLEND);
		mdns->material_inst.render_stages = SEHLE_STAGE_SOLID;
		mdns->material_inst.render_types = SEHLE_RENDER_DEPTH | SEHLE_RENDER_GBUFFER;
	}
	if (sort) {
		mdns->material_inst.properties |= SEHLE_MATERIAL_SORT;
	} else {
		mdns->material_inst.properties &= ~SEHLE_MATERIAL_SORT;
	}
}

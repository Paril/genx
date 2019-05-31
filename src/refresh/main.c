/*
Copyright (C) 2003-2006 Andrey Nazarov

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
 * gl_main.c
 *
 */

#include "gl.h"

glRefdef_t glr;
glStatic_t gl_static;
glConfig_t gl_config;
statCounters_t  c;

entity_t gl_world;

refcfg_t r_config;

int registration_sequence;

// regular variables
cvar_t *gl_partscale;
cvar_t *gl_partstyle;
cvar_t *gl_dotshading;
cvar_t *gl_shadows;
cvar_t *gl_modulate;
cvar_t *gl_modulate_world;
cvar_t *gl_coloredlightmaps;
cvar_t *gl_brightness;
cvar_t *gl_dynamic;
cvar_t *gl_dlight_falloff;
cvar_t *gl_modulate_entities;
cvar_t *gl_doublelight_entities;
cvar_t *gl_fontshadow;

// development variables
cvar_t *gl_znear;
cvar_t *gl_drawworld;
cvar_t *gl_drawentities;
cvar_t *gl_drawsky;
cvar_t *gl_showtris;
cvar_t *gl_showorigins;
cvar_t *gl_showtearing;
#ifdef _DEBUG
	cvar_t *gl_showstats;
	cvar_t *gl_showscrap;
	cvar_t *gl_nobind;
	cvar_t *gl_test;
#endif
cvar_t *gl_cull_nodes;
cvar_t *gl_cull_models;
cvar_t *gl_clear;
cvar_t *gl_finish;
cvar_t *gl_hash_faces;
cvar_t *gl_novis;
cvar_t *gl_lockpvs;
cvar_t *gl_lightmap;
cvar_t *gl_fullbright;
cvar_t *gl_polyblend;
cvar_t *gl_showerrors;
cvar_t *gl_drawbuffer;

// ==============================================================================

static void GL_SetupFrustum(void)
{
	vec_t angle, sf, cf;
	vec3_t forward, left, up;
	cplane_t *p;
	int i;
	// right/left
	angle = DEG2RAD(glr.fd.fov_x / 2);
	sf = sin(angle);
	cf = cos(angle);
	VectorScale(glr.viewaxis[0], sf, forward);
	VectorScale(glr.viewaxis[1], cf, left);
	VectorAdd(forward, left, glr.frustumPlanes[0].normal);
	VectorSubtract(forward, left, glr.frustumPlanes[1].normal);
	// top/bottom
	angle = DEG2RAD(glr.fd.fov_y / 2);
	sf = sin(angle);
	cf = cos(angle);
	VectorScale(glr.viewaxis[0], sf, forward);
	VectorScale(glr.viewaxis[2], cf, up);
	VectorAdd(forward, up, glr.frustumPlanes[2].normal);
	VectorSubtract(forward, up, glr.frustumPlanes[3].normal);

	for (i = 0, p = glr.frustumPlanes; i < 4; i++, p++)
	{
		p->dist = DotProduct(glr.fd.vieworg, p->normal);
		p->type = PLANE_NON_AXIAL;
		SetPlaneSignbits(p);
	}
}

glCullResult_t GL_CullBox(vec3_t bounds[2])
{
	int i, bits;
	glCullResult_t cull;

	if (!gl_cull_models->integer)
		return CULL_IN;

	cull = CULL_IN;

	for (i = 0; i < 4; i++)
	{
		bits = BoxOnPlaneSide(bounds[0], bounds[1], &glr.frustumPlanes[i]);

		if (bits == BOX_BEHIND)
			return CULL_OUT;

		if (bits != BOX_INFRONT)
			cull = CULL_CLIP;
	}

	return cull;
}

glCullResult_t GL_CullSphere(const vec3_t origin, float radius)
{
	float dist;
	cplane_t *p;
	int i;
	glCullResult_t cull;

	if (!gl_cull_models->integer)
		return CULL_IN;

	cull = CULL_IN;

	for (i = 0, p = glr.frustumPlanes; i < 4; i++, p++)
	{
		dist = PlaneDiff(origin, p);

		if (dist < -radius)
			return CULL_OUT;

		if (dist <= radius)
			cull = CULL_CLIP;
	}

	return cull;
}

glCullResult_t GL_CullLocalBox(const vec3_t origin, vec3_t bounds[2])
{
	vec3_t points[8];
	cplane_t *p;
	int i, j;
	vec_t dot;
	bool infront;
	glCullResult_t cull;

	if (!gl_cull_models->integer)
		return CULL_IN;

	for (i = 0; i < 8; i++)
	{
		VectorCopy(origin, points[i]);
		VectorMA(points[i], bounds[(i >> 0) & 1][0], glr.entaxis[0], points[i]);
		VectorMA(points[i], bounds[(i >> 1) & 1][1], glr.entaxis[1], points[i]);
		VectorMA(points[i], bounds[(i >> 2) & 1][2], glr.entaxis[2], points[i]);
	}

	cull = CULL_IN;

	for (i = 0, p = glr.frustumPlanes; i < 4; i++, p++)
	{
		infront = false;

		for (j = 0; j < 8; j++)
		{
			dot = DotProduct(points[j], p->normal);

			if (dot >= p->dist)
			{
				infront = true;

				if (cull == CULL_CLIP)
					break;
			}
			else
			{
				cull = CULL_CLIP;

				if (infront)
					break;
			}
		}

		if (!infront)
			return CULL_OUT;
	}

	return cull;
}

// shared between lightmap and scrap allocators
bool GL_AllocBlock(int width, int height, int *inuse,
	int w, int h, int *s, int *t)
{
	int i, j, k, x, y, max_inuse, min_inuse;
	x = 0;
	y = height;
	min_inuse = height;

	for (i = 0; i < width - w; i++)
	{
		max_inuse = 0;

		for (j = 0; j < w; j++)
		{
			k = inuse[i + j];

			if (k >= min_inuse)
				break;

			if (max_inuse < k)
				max_inuse = k;
		}

		if (j == w)
		{
			x = i;
			y = min_inuse = max_inuse;
		}
	}

	if (y + h > height)
		return false;

	for (i = 0; i < w; i++)
		inuse[x + i] = y + h;

	*s = x;
	*t = y;
	return true;
}

// P = A * B
void GL_MultMatrix(GLfloat *p, const GLfloat *a, const GLfloat *b)
{
	int i, j;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			p[i * 4 + j] =
				a[0 * 4 + j] * b[i * 4 + 0] +
				a[1 * 4 + j] * b[i * 4 + 1] +
				a[2 * 4 + j] * b[i * 4 + 2] +
				a[3 * 4 + j] * b[i * 4 + 3];
		}
	}
}

void GL_RotateForEntity(vec3_t origin)
{
	GLfloat matrix[16];
	matrix[0] = glr.entaxis[0][0];
	matrix[4] = glr.entaxis[1][0];
	matrix[8] = glr.entaxis[2][0];
	matrix[12] = origin[0];
	matrix[1] = glr.entaxis[0][1];
	matrix[5] = glr.entaxis[1][1];
	matrix[9] = glr.entaxis[2][1];
	matrix[13] = origin[1];
	matrix[2] = glr.entaxis[0][2];
	matrix[6] = glr.entaxis[1][2];
	matrix[10] = glr.entaxis[2][2];
	matrix[14] = origin[2];
	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 0;
	matrix[15] = 1;
	GL_MultMatrix(glr.entmatrix, glr.viewmatrix, matrix);
	GL_ForceMatrix(glr.entmatrix);
}

static glCullResult_t cull_sprite(vec3_t origin, model_t *model)
{
	glCullResult_t cull = GL_CullSphere(origin, model->radius);

	if (cull == CULL_OUT)
	{
		c.spheresCulled++;
		return cull;
	}

	return cull;
}


static void GL_DrawSpriteModel(entity_t *ent, model_t *model)
{
	vec3_t origin;
	float backlerp = ent->backlerp;
	float frontlerp = 1.0f - backlerp;

	// interpolate origin, if necessarry
	if (ent->flags & RF_FRAMELERP)
	{
		LerpVector2(ent->oldorigin, ent->origin,
			backlerp, frontlerp, origin);
	}
	else
		VectorCopy(ent->origin, origin);

	if (cull_sprite(origin, model) == CULL_OUT)
		return;

	static const vec_t tcoords[8] = { 0, 1, 0, 0, 1, 1, 1, 0 };
	entity_t *e = glr.ent;
	mspriteframe_t *frame = &model->spriteframes[e->frame % model->numframes];
	image_t *image = frame->image;
	float alpha = (e->flags & RF_TRANSLUCENT) ? e->alpha : 1;
	int bits = GLS_CULL_DISABLE;
	vec3_t up, down, left, right;
	vec3_t points[4];

	if (alpha == 1)
	{
		if (image->flags & IF_TRANSPARENT)
		{
			if (image->flags & IF_PALETTED)
				bits |= GLS_ALPHATEST_ENABLE;
			else
				bits |= GLS_BLEND_BLEND;
		}
	}
	else
		bits |= GLS_BLEND_BLEND;

	GL_LoadMatrix(glr.viewmatrix);
	GL_BindTexture(0, image->texnum);
	GL_StateBits(bits);
	GL_ArrayBits(GLA_VERTEX | GLA_TC);
	vec3_t color;

	if (ent->flags & RF_SPECTRE)
		color[0] = color[1] = color[2] = 0;
	else if (ent->flags & RF_FULLBRIGHT)
		VectorSet(color, 1, 1, 1);
	else
		GL_LightPoint(origin, color);

	if (ent->flags & RF_FROZEN)
	{
		color[0] /= 1.5f;
		color[1] /= 1.5f;
	}

	GL_Color(color[0], color[1], color[2], alpha);

	if (ent->flags & RF_NO_BILLBOARD)
	{
		VectorScale(glr.entaxis[1], frame->origin_x, left);
		VectorScale(glr.entaxis[1], frame->origin_x - frame->width, right);
		VectorScale(glr.entaxis[2], -frame->origin_y, down);
		VectorScale(glr.entaxis[2], frame->height - frame->origin_y, up);
	}
	else
	{
		VectorScale(glr.viewaxis[1], frame->origin_x, left);
		VectorScale(glr.viewaxis[1], frame->origin_x - frame->width, right);
		VectorScale(glr.viewaxis[2], -frame->origin_y, down);
		VectorScale(glr.viewaxis[2], frame->height - frame->origin_y, up);
	}

	VectorAdd3(e->origin, down, left, points[0]);
	VectorAdd3(e->origin, up, left, points[1]);
	VectorAdd3(e->origin, down, right, points[2]);
	VectorAdd3(e->origin, up, right, points[3]);
	GL_TexCoordPointer(2, 0, tcoords);
	GL_VertexPointer(3, 0, &points[0][0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void GL_DrawNullModel(void)
{
	static const uint32_t colors[6] =
	{
		U32_RED, U32_RED,
		U32_GREEN, U32_GREEN,
		U32_BLUE, U32_BLUE
	};
	entity_t *e = glr.ent;
	vec3_t points[6];
	VectorCopy(e->origin, points[0]);
	VectorCopy(e->origin, points[2]);
	VectorCopy(e->origin, points[4]);
	VectorMA(e->origin, 16, glr.entaxis[0], points[1]);
	VectorMA(e->origin, 16, glr.entaxis[1], points[3]);
	VectorMA(e->origin, 16, glr.entaxis[2], points[5]);
	GL_LoadMatrix(glr.viewmatrix);
	GL_BindTexture(0, TEXNUM_WHITE);
	GL_StateBits(GLS_DEFAULT);
	GL_ArrayBits(GLA_VERTEX | GLA_COLOR);
	GL_ColorBytePointer(4, 0, (GLubyte *)colors);
	GL_VertexPointer(3, 0, &points[0][0]);
	glDrawArrays(GL_LINES, 0, 6);
}

#define ANGLE_ONE_TURN ((360.0f / 8.0f) / 2)

static void GL_DrawDirSpriteModel(entity_t *ent, model_t *model)
{
	vec3_t origin;
	float backlerp = ent->backlerp;
	float frontlerp = 1.0f - backlerp;

	// interpolate origin, if necessarry
	if (ent->flags & RF_FRAMELERP)
	{
		LerpVector2(ent->oldorigin, ent->origin,
			backlerp, frontlerp, origin);
	}
	else
		VectorCopy(ent->origin, origin);

	if (cull_sprite(origin, model) == CULL_OUT)
		return;

	vec3_t player_fwd, diff_angle;
	VectorSubtract(origin, glr.fd.vieworg, player_fwd);
	player_fwd[2] = 0;
	VectorNormalize(player_fwd);
	vectoangles2(player_fwd, diff_angle);
	float yaw_diff = diff_angle[1] - ent->angles[1];
	yaw_diff += 180 + ANGLE_ONE_TURN;

	while (yaw_diff > 360)
		yaw_diff = yaw_diff - 360;

	int rot;

	if (model->numdirs == 1)
		rot = 0;
	else
	{
		int ndm = model->numdirs - 1;
		rot = min(ndm, (int)((yaw_diff / 360) * model->numdirs));

		if (rot < 0)
			rot = ndm + rot;
	}

	int frame_num;

	if (ent->flags & RF_ANIM_BOUNCE)
	{
		frame_num = ent->frame % ((model->numframes * 2) - 2);

		if (frame_num >= model->numframes)
			frame_num = model->numframes + (model->numframes - frame_num - 2);

		frame_num = abs(frame_num);
	}
	else
		frame_num = ent->frame;

	mspritedirframe_t *frame = &model->spritedirframes[frame_num % model->numframes].directions[rot];
	static const vec_t tcoords[8] = { 0, 0, 1, 0, 1, 1, 0, 1 };
	image_t *image = frame->image;
	float alpha = (ent->flags & RF_TRANSLUCENT) ? ent->alpha : 1;
	int bits = GLS_CULL_DISABLE | GLS_ALPHATEST_ENABLE;
	vec3_t up, down, left, right;
	vec3_t points[4];

	if (ent->flags & RF_SPECTRE)
		alpha = 0.25f;

	if (alpha == 1)
	{
		if (image->flags & IF_TRANSPARENT)
		{
			if (image->flags & IF_PALETTED)
				bits |= GLS_ALPHATEST_ENABLE;
			else
				bits |= GLS_BLEND_BLEND;
		}
	}
	else
		bits |= GLS_BLEND_BLEND | GLS_DEPTHMASK_FALSE;

	GL_LoadMatrix(glr.viewmatrix);
	GL_BindTexture(0, image->texnum);
	GL_StateBits(bits);
	GL_ArrayBits(GLA_VERTEX | GLA_TC);
	vec3_t color;

	if (ent->flags & RF_SPECTRE)
		color[0] = color[1] = color[2] = 0;
	else if (ent->flags & RF_FULLBRIGHT)
		VectorSet(color, 1, 1, 1);
	else
		GL_LightPoint(origin, color);

	if (ent->flags & RF_FROZEN)
	{
		color[0] /= 1.5f;
		color[1] /= 1.5f;
	}

	GL_Color(color[0], color[1], color[2], alpha);

	if (ent->flags & RF_NO_BILLBOARD)
	{
		VectorScale(glr.entaxis[1], frame->origin_x, left);
		VectorScale(glr.entaxis[1], frame->origin_x - frame->width, right);
		VectorScale(glr.entaxis[2], -frame->origin_y, down);
		VectorScale(glr.entaxis[2], frame->height - frame->origin_y, up);
	}
	else
	{
		if (frame->invert_x)
			VectorScale(glr.viewaxis_nopitch[1], -frame->origin_x, left);
		else
			VectorScale(glr.viewaxis_nopitch[1], frame->origin_x, left);

		VectorScale(glr.viewaxis_nopitch[2], frame->origin_y, up);

		if (frame->invert_x)
			VectorScale(glr.viewaxis_nopitch[1], -(frame->origin_x - frame->width), right);
		else
			VectorScale(glr.viewaxis_nopitch[1], frame->origin_x - frame->width, right);

		VectorScale(glr.viewaxis_nopitch[2], frame->origin_y - frame->height, down);
	}

	VectorScale(left, ent->scale, left);
	VectorScale(right, ent->scale, right);
	VectorScale(up, ent->scale, up);
	VectorScale(down, ent->scale, down);
	VectorAdd3(origin, up, left, points[0]);
	VectorAdd3(origin, up, right, points[1]);
	VectorAdd3(origin, down, right, points[2]);
	VectorAdd3(origin, down, left, points[3]);
	GL_TexCoordPointer(2, 0, tcoords);
	GL_VertexPointer(3, 0, &points[0][0]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

static void GL_DrawEntities(int mask)
{
	entity_t *ent, *last;
	model_t *model;

	if (!gl_drawentities->integer)
		return;

	last = glr.fd.entities + glr.fd.num_entities;

	for (ent = glr.fd.entities; ent != last; ent++)
	{
		if (ent->flags & RF_BEAM)
		{
			// beams are drawn elsewhere in single batch
			glr.num_beams++;
			continue;
		}

		if ((ent->flags & RF_TRANSLUCENT) != mask)
			continue;

		glr.ent = ent;

		// convert angles to axis
		if (VectorEmpty(ent->angles))
		{
			glr.entrotated = false;
			VectorSet(glr.entaxis[0], 1, 0, 0);
			VectorSet(glr.entaxis[1], 0, 1, 0);
			VectorSet(glr.entaxis[2], 0, 0, 1);
		}
		else
		{
			glr.entrotated = true;
			AnglesToAxis(ent->angles, glr.entaxis);
		}

		// inline BSP model
		if (MODEL_HANDLE_TYPE(ent->model) == MODELHANDLE_BSP)
		{
			bsp_t *bsp = gl_static.world.cache;

			if (glr.fd.rdflags & RDF_NOWORLDMODEL)
			{
				Com_Error(ERR_DROP, "%s: inline model without world",
					__func__);
			}

			if (MODEL_HANDLE_ID(ent->model) < 1 || MODEL_HANDLE_ID(ent->model) >= bsp->nummodels)
			{
				Com_Error(ERR_DROP, "%s: inline model %d out of range",
					__func__, MODEL_HANDLE_ID(ent->model));
			}

			GL_DrawBspModel(&bsp->models[MODEL_HANDLE_ID(ent->model)]);
			continue;
		}

		model = MOD_ForHandle(ent->model, ent->game);

		if (!model)
		{
			GL_DrawNullModel();
			continue;
		}

		switch (model->type)
		{
			case MOD_ALIAS:
				GL_DrawAliasModel(model);
				break;

			case MOD_SPRITE:
				GL_DrawSpriteModel(ent, model);
				break;

			case MOD_DIR_SPRITE:
				GL_DrawDirSpriteModel(ent, model);
				break;

			case MOD_EMPTY:
			case MOD_WEAPONSCRIPT:
				break;

			default:
				Com_Error(ERR_FATAL, "%s: bad model type", __func__);
		}

		if (gl_showorigins->integer)
			GL_DrawNullModel();
	}
}

static void GL_DrawTearing(void)
{
	static int i;
	// alternate colors to make tearing obvious
	i++;

	if (i & 1)
		glClearColor(1, 1, 1, 1);
	else
		glClearColor(1, 0, 0, 0);

	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0, 0, 0, 1);
}

static const char *GL_ErrorString(GLenum err)
{
	const char *str;

	switch (err)
	{
#define E(x) case GL_##x: str = "GL_"#x; break;
			E(NO_ERROR)
			E(INVALID_ENUM)
			E(INVALID_VALUE)
			E(INVALID_OPERATION)
			E(OUT_OF_MEMORY)

		default:
			str = "UNKNOWN ERROR";
#undef E
	}

	return str;
}

void GL_ClearError(void)
{
	GLenum err;

	while ((err = glGetError()) != GL_NO_ERROR)
		;
}

bool GL_ShowErrors(const char *func)
{
	GLenum err = glGetError();

	if (err == GL_NO_ERROR)
		return false;

	do
	{
		if (gl_showerrors->integer)
			Com_EPrintf("%s: %s\n", func, GL_ErrorString(err));
	}
	while ((err = glGetError()) != GL_NO_ERROR);

	return true;
}

void R_RenderFrame(refdef_t *fd)
{
	GL_Flush2D();

	if (!gl_static.world.cache && !(fd->rdflags & RDF_NOWORLDMODEL))
		Com_Error(ERR_FATAL, "%s: NULL worldmodel", __func__);

	glr.drawframe++;
	glr.fd = *fd;
	glr.num_beams = 0;

	if (gl_dynamic->integer != 1)
		glr.fd.num_dlights = 0;

	if (lm.dirty)
	{
		GL_RebuildLighting();
		lm.dirty = false;
	}

	GL_Setup3D(true);

	if (gl_cull_nodes->integer)
		GL_SetupFrustum();

	if (!(glr.fd.rdflags & RDF_NOWORLDMODEL) && gl_drawworld->integer)
		GL_DrawWorld();

	GL_DrawEntities(0);
	GL_DrawBeams();
	GL_DrawParticles();
	GL_DrawEntities(RF_TRANSLUCENT);

	if (!(glr.fd.rdflags & RDF_NOWORLDMODEL))
		GL_DrawAlphaFaces();

	// go back into 2D mode
	GL_Setup2D();

	if (gl_polyblend->integer && glr.fd.blend[3] != 0)
		GL_Blend();

#ifdef _DEBUG

	if (gl_lightmap->integer > 1)
		Draw_Lightmaps();

#endif
	GL_ShowErrors(__func__);
}

void R_BeginFrame(void)
{
	memset(&c, 0, sizeof(c));

	if (gl_finish->integer)
		glFinish();

	GL_Setup2D();

	if (gl_clear->integer)
		glClear(GL_COLOR_BUFFER_BIT);

	GL_ShowErrors(__func__);
}

void R_EndFrame(void)
{
#ifdef _DEBUG

	if (gl_showstats->integer)
	{
		GL_Flush2D();
		Draw_Stats();
	}

	if (gl_showscrap->integer)
		Draw_Scrap();

#endif
	GL_Flush2D();

	if (gl_showtearing->integer)
		GL_DrawTearing();

	GL_ShowErrors(__func__);
	VID_EndFrame();
}

// ==============================================================================

static void GL_Strings_f(void)
{
	GLint integer = 0;
	Com_Printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
	Com_Printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
	Com_Printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
	Com_Printf("GL_SHADING_LANGUAGE_VERSION: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	if (Cmd_Argc() > 1)
	{
		Com_Printf("GL_EXTENSIONS: ");
		glGetIntegerv(GL_NUM_EXTENSIONS, &integer);

		for (int i = 0; i < integer; i++)
			Com_Printf("%s ", glGetStringi(GL_EXTENSIONS, i));

		Com_Printf("\n");
	}

	Com_Printf("GL_MAX_TEXTURE_SIZE: %d\n", gl_config.max_texture_size);
	Com_Printf("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: %d\n", gl_config.max_texture_units);

	if (gl_config.max_anisotropy)
		Com_Printf("GL_MAX_TEXTURE_MAX_ANISOTROPY: %.f\n", gl_config.max_anisotropy);

	Com_Printf("GL_PFD: color(%d-bit) Z(%d-bit) stencil(%d-bit)\n",
		gl_config.colorbits, gl_config.depthbits, gl_config.stencilbits);
}

static void GL_RenderModel_f(void)
{
	qhandle_t model = R_RegisterModel(Cmd_Argv(1));
	model_t *mod = MOD_ForHandle(model, GAME_Q2);
	GL_OutputAliasModelAs2D(mod);
}

static size_t GL_ViewCluster_m(char *buffer, size_t size)
{
	return Q_scnprintf(buffer, size, "%d", glr.viewcluster1);
}

static void gl_lightmap_changed(cvar_t *self)
{
	lm.scale = Cvar_ClampValue(gl_coloredlightmaps, 0, 1);
	lm.comp = lm.scale ? GL_RGB : GL_RED;
	lm.add = 255 * Cvar_ClampValue(gl_brightness, -1, 1);
	lm.modulate = Cvar_ClampValue(gl_modulate, 0, 1e6f);
	lm.modulate *= Cvar_ClampValue(gl_modulate_world, 0, 1e6f);

	if (self == gl_brightness || self == gl_modulate || self == gl_modulate_world)
		return;

	lm.dirty = true; // rebuild all lightmaps next frame
}

static void gl_modulate_entities_changed(cvar_t *self)
{
	gl_static.entity_modulate = Cvar_ClampValue(gl_modulate, 0, 1e6f);
	gl_static.entity_modulate *= Cvar_ClampValue(gl_modulate_entities, 0, 1e6f);
}

static void gl_modulate_changed(cvar_t *self)
{
	gl_lightmap_changed(self);
	gl_modulate_entities_changed(self);
}

// ugly hack to reset sky
static void gl_drawsky_changed(cvar_t *self)
{
	if (gl_static.world.cache)
		CL_SetSky();
}

static void gl_novis_changed(cvar_t *self)
{
	glr.viewcluster1 = glr.viewcluster2 = -2;
}

static void gl_drawbuffer_changed(cvar_t *self)
{
	if (!Q_stricmp(self->string, "GL_FRONT"))
		gl_static.drawbuffer = GL_FRONT;
	else if (!Q_stricmp(self->string, "GL_BACK"))
		gl_static.drawbuffer = GL_BACK;
	else
	{
		Cvar_Reset(self);
		gl_static.drawbuffer = GL_BACK;
	}

	glDrawBuffer(gl_static.drawbuffer);
}

static void GL_Register(void)
{
	// regular variables
	gl_partscale = Cvar_Get("gl_partscale", "2", 0);
	gl_partstyle = Cvar_Get("gl_partstyle", "0", 0);
	gl_dotshading = Cvar_Get("gl_dotshading", "1", 0);
	gl_shadows = Cvar_Get("gl_shadows", "0", CVAR_ARCHIVE);
	gl_modulate = Cvar_Get("gl_modulate", "1", CVAR_ARCHIVE);
	gl_modulate->changed = gl_modulate_changed;
	gl_modulate_world = Cvar_Get("gl_modulate_world", "1", 0);
	gl_modulate_world->changed = gl_lightmap_changed;
	gl_coloredlightmaps = Cvar_Get("gl_coloredlightmaps", "1", 0);
	gl_coloredlightmaps->changed = gl_lightmap_changed;
	gl_brightness = Cvar_Get("gl_brightness", "0", 0);
	gl_brightness->changed = gl_lightmap_changed;
	gl_dynamic = Cvar_Get("gl_dynamic", "1", 0);
	gl_dynamic->changed = gl_lightmap_changed;
	gl_dlight_falloff = Cvar_Get("gl_dlight_falloff", "1", 0);
	gl_modulate_entities = Cvar_Get("gl_modulate_entities", "1", CVAR_ARCHIVE);
	gl_modulate_entities->changed = gl_modulate_entities_changed;
	gl_doublelight_entities = Cvar_Get("gl_doublelight_entities", "0", 0);
	gl_fontshadow = Cvar_Get("gl_fontshadow", "0", 0);
	// development variables
	gl_znear = Cvar_Get("gl_znear", "2", CVAR_CHEAT);
	gl_drawworld = Cvar_Get("gl_drawworld", "1", CVAR_CHEAT);
	gl_drawentities = Cvar_Get("gl_drawentities", "1", CVAR_CHEAT);
	gl_drawsky = Cvar_Get("gl_drawsky", "1", 0);
	gl_drawsky->changed = gl_drawsky_changed;
	gl_showtris = Cvar_Get("gl_showtris", "0", CVAR_CHEAT);
	gl_showorigins = Cvar_Get("gl_showorigins", "0", CVAR_CHEAT);
	gl_showtearing = Cvar_Get("gl_showtearing", "0", CVAR_CHEAT);
#ifdef _DEBUG
	gl_showstats = Cvar_Get("gl_showstats", "0", 0);
	gl_showscrap = Cvar_Get("gl_showscrap", "0", 0);
	gl_nobind = Cvar_Get("gl_nobind", "0", CVAR_CHEAT);
	gl_test = Cvar_Get("gl_test", "0", 0);
#endif
	gl_cull_nodes = Cvar_Get("gl_cull_nodes", "1", 0);
	gl_cull_models = Cvar_Get("gl_cull_models", "1", 0);
	gl_hash_faces = Cvar_Get("gl_hash_faces", "1", 0);
	gl_clear = Cvar_Get("gl_clear", "0", 0);
	gl_finish = Cvar_Get("gl_finish", "0", 0);
	gl_novis = Cvar_Get("gl_novis", "0", 0);
	gl_novis->changed = gl_novis_changed;
	gl_lockpvs = Cvar_Get("gl_lockpvs", "0", CVAR_CHEAT);
	gl_lightmap = Cvar_Get("gl_lightmap", "0", CVAR_CHEAT);
	gl_fullbright = Cvar_Get("r_fullbright", "0", CVAR_CHEAT);
	gl_fullbright->changed = gl_lightmap_changed;
	gl_polyblend = Cvar_Get("gl_polyblend", "1", 0);
	gl_showerrors = Cvar_Get("gl_showerrors", "1", 0);
	gl_drawbuffer = Cvar_Get("gl_drawbuffer", "GL_BACK", 0);
	gl_drawbuffer->changed = gl_drawbuffer_changed;
	gl_drawbuffer_changed(gl_drawbuffer);
	gl_lightmap_changed(NULL);
	gl_modulate_entities_changed(NULL);
	Cmd_AddCommand("strings", GL_Strings_f);
	Cmd_AddCommand("rendermodel", GL_RenderModel_f);
	Cmd_AddMacro("gl_viewcluster", GL_ViewCluster_m);
}

static void GL_Unregister(void)
{
	Cmd_RemoveCommand("strings");
}

static void GL_SetupConfig(void)
{
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_config.max_texture_size);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &gl_config.max_texture_units);

	if (GLAD_GL_ARB_texture_filter_anisotropic || GLAD_GL_EXT_texture_filter_anisotropic)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &gl_config.max_anisotropy);
}

static void GL_InitTables(void)
{
	vec_t lat, lng;
	const vec_t *v;
	int i;

	for (i = 0; i < NUMVERTEXNORMALS; i++)
	{
		v = bytedirs[i];
		lat = acos(v[2]);
		lng = atan2(v[1], v[0]);
		gl_static.latlngtab[i][0] = (int)(lat * (float)(255 / (2 * M_PI))) & 255;
		gl_static.latlngtab[i][1] = (int)(lng * (float)(255 / (2 * M_PI))) & 255;
	}

	for (i = 0; i < 256; i++)
		gl_static.sintab[i] = sin(i * (2 * M_PI / 255));
}

static void GL_PostInit(void)
{
	registration_sequence = 1;
	GL_ClearState();
	GL_InitImages();
	MOD_Init();
}

// ==============================================================================

/*
===============
R_Init
===============
*/
bool R_Init(bool total)
{
	Com_DPrintf("GL_Init( %i )\n", total);

	if (!total)
	{
		GL_PostInit();
		return true;
	}

	Com_Printf("------- R_Init -------\n");
	Com_DPrintf("ref_gl " VERSION ", " __DATE__ "\n");

	// initialize OS-specific parts of OpenGL
	// create the window and set up the context
	if (!VID_Init())
		return false;

	// initialize our GL bindings
	if (!gladLoadGL())
		goto fail;

	// get various limits from OpenGL
	GL_SetupConfig();
	// register our variables
	GL_Register();
	GL_InitState();
	GL_InitTables();
	GL_PostInit();
	Com_Printf("----------------------\n");
	return true;
fail:
	memset(&gl_static, 0, sizeof(gl_static));
	memset(&gl_config, 0, sizeof(gl_config));
	VID_Shutdown();
	return false;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown(bool total)
{
	Com_DPrintf("GL_Shutdown( %i )\n", total);
	GL_FreeWorld();
	GL_ShutdownImages();
	MOD_Shutdown();

	if (!total)
		return;

	GL_ShutdownState();
	// shut down OS specific OpenGL stuff like contexts, etc.
	VID_Shutdown();
	GL_Unregister();
	memset(&gl_static, 0, sizeof(gl_static));
	memset(&gl_config, 0, sizeof(gl_config));
}

/*
===============
R_BeginRegistration
===============
*/
void R_BeginRegistration(const char *name)
{
	char fullname[MAX_QPATH];
	gl_static.registering = true;
	registration_sequence++;
	memset(&glr, 0, sizeof(glr));
	glr.viewcluster1 = glr.viewcluster2 = -2;
	Q_concat(fullname, sizeof(fullname), "maps/", name, ".bsp", NULL);
	GL_LoadWorld(fullname);
}

/*
===============
R_EndRegistration
===============
*/
void R_EndRegistration(void)
{
	IMG_FreeUnused();
	MOD_FreeUnused();
	Scrap_Upload();
	gl_static.registering = false;
}

/*
===============
R_ModeChanged
===============
*/
void R_ModeChanged(int width, int height, int flags)
{
	r_config.width = width;
	r_config.height = height;
	r_config.flags = flags;
}

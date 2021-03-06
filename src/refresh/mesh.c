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

#include "gl.h"

typedef void (*tessfunc_t)(const maliasmesh_t *);

static int      oldframenum;
static int      newframenum;
static float    frontlerp;
static float    backlerp;
static vec3_t   origin;
static vec3_t   oldscale;
static vec3_t   newscale;
static vec3_t   translate;
static vec_t    shellscale;
static tessfunc_t tessfunc;
static vec4_t   color;

static const vec_t  *shadelight;
static vec3_t       shadedir;

static GLfloat  shadowmatrix[16];

extern cvar_t	 *scr_gunfov;

static void setup_dotshading(void)
{
	float cp, cy, sp, sy;
	vec_t yaw;
	shadelight = NULL;

	if (!gl_dotshading->integer)
		return;

	if (glr.ent->flags & RF_SHELL_MASK)
		return;

	shadelight = color;
	// matches the anormtab.h precalculations
	yaw = -DEG2RAD(glr.ent->angles[YAW]);
	cy = cos(yaw);
	sy = sin(yaw);
	cp = cos(-M_PI / 4);
	sp = sin(-M_PI / 4);
	shadedir[0] = cp * cy;
	shadedir[1] = cp * sy;
	shadedir[2] = -sp;
}

static inline vec_t shadedot(const vec_t *normal)
{
	vec_t d = DotProduct(normal, shadedir);

	// matches the anormtab.h precalculations
	if (d < 0)
		d *= 0.3f;

	return d + 1;
}

static inline vec_t *get_static_normal(vec_t *normal, const maliasvert_t *vert)
{
	VectorCopy(vert->norm, normal);
	return normal;
}

static void tess_static_shell(const maliasmesh_t *mesh)
{
	maliasvert_t *src_vert = &mesh->verts[newframenum * mesh->numverts];
	vec_t *dst_vert = tess.vertices;
	int count = mesh->numverts;
	vec3_t normal;

	while (count--)
	{
		get_static_normal(normal, src_vert);
		dst_vert[0] = normal[0] * shellscale +
			src_vert->pos[0] * newscale[0] + translate[0];
		dst_vert[1] = normal[1] * shellscale +
			src_vert->pos[1] * newscale[1] + translate[1];
		dst_vert[2] = normal[2] * shellscale +
			src_vert->pos[2] * newscale[2] + translate[2];
		dst_vert += 4;
		src_vert++;
	}
}

static void tess_static_shade(const maliasmesh_t *mesh)
{
	maliasvert_t *src_vert = &mesh->verts[newframenum * mesh->numverts];
	vec_t *dst_vert = tess.vertices;
	int count = mesh->numverts;
	vec3_t normal;
	vec_t d;

	while (count--)
	{
		d = shadedot(get_static_normal(normal, src_vert));
		dst_vert[0] = src_vert->pos[0] * newscale[0] + translate[0];
		dst_vert[1] = src_vert->pos[1] * newscale[1] + translate[1];
		dst_vert[2] = src_vert->pos[2] * newscale[2] + translate[2];
		dst_vert[4] = shadelight[0] * d;
		dst_vert[5] = shadelight[1] * d;
		dst_vert[6] = shadelight[2] * d;
		dst_vert[7] = shadelight[3];
		dst_vert += VERTEX_SIZE;
		src_vert++;
	}
}

static void tess_static_plain(const maliasmesh_t *mesh)
{
	maliasvert_t *src_vert = &mesh->verts[newframenum * mesh->numverts];
	vec_t *dst_vert = tess.vertices;
	int count = mesh->numverts;

	while (count--)
	{
		dst_vert[0] = src_vert->pos[0] * newscale[0] + translate[0];
		dst_vert[1] = src_vert->pos[1] * newscale[1] + translate[1];
		dst_vert[2] = src_vert->pos[2] * newscale[2] + translate[2];
		dst_vert += 4;
		src_vert++;
	}
}

static inline vec_t *get_lerped_normal(vec_t *normal,
	const maliasvert_t *oldvert,
	const maliasvert_t *newvert)
{
	vec3_t oldnorm, newnorm, tmp;
	vec_t len;
	get_static_normal(oldnorm, oldvert);
	get_static_normal(newnorm, newvert);
	LerpVector2(oldnorm, newnorm, backlerp, frontlerp, tmp);
	// normalize result
	len = 1 / VectorLength(tmp);
	VectorScale(tmp, len, normal);
	return normal;
}

static void tess_lerped_shell(const maliasmesh_t *mesh)
{
	maliasvert_t *src_oldvert = &mesh->verts[oldframenum * mesh->numverts];
	maliasvert_t *src_newvert = &mesh->verts[newframenum * mesh->numverts];
	vec_t *dst_vert = tess.vertices;
	int count = mesh->numverts;
	vec3_t normal;

	while (count--)
	{
		get_lerped_normal(normal, src_oldvert, src_newvert);
		dst_vert[0] = normal[0] * shellscale +
			src_oldvert->pos[0] * oldscale[0] +
			src_newvert->pos[0] * newscale[0] + translate[0];
		dst_vert[1] = normal[1] * shellscale +
			src_oldvert->pos[1] * oldscale[1] +
			src_newvert->pos[1] * newscale[1] + translate[1];
		dst_vert[2] = normal[2] * shellscale +
			src_oldvert->pos[2] * oldscale[2] +
			src_newvert->pos[2] * newscale[2] + translate[2];
		dst_vert += 4;
		src_oldvert++;
		src_newvert++;
	}
}

static void tess_lerped_shade(const maliasmesh_t *mesh)
{
	maliasvert_t *src_oldvert = &mesh->verts[oldframenum * mesh->numverts];
	maliasvert_t *src_newvert = &mesh->verts[newframenum * mesh->numverts];
	vec_t *dst_vert = tess.vertices;
	int count = mesh->numverts;
	vec3_t normal;
	vec_t d;

	while (count--)
	{
		d = shadedot(get_lerped_normal(normal, src_oldvert, src_newvert));
		dst_vert[0] =
			src_oldvert->pos[0] * oldscale[0] +
			src_newvert->pos[0] * newscale[0] + translate[0];
		dst_vert[1] =
			src_oldvert->pos[1] * oldscale[1] +
			src_newvert->pos[1] * newscale[1] + translate[1];
		dst_vert[2] =
			src_oldvert->pos[2] * oldscale[2] +
			src_newvert->pos[2] * newscale[2] + translate[2];
		dst_vert[4] = shadelight[0] * d;
		dst_vert[5] = shadelight[1] * d;
		dst_vert[6] = shadelight[2] * d;
		dst_vert[7] = shadelight[3];
		dst_vert += VERTEX_SIZE;
		src_oldvert++;
		src_newvert++;
	}
}

static void tess_lerped_plain(const maliasmesh_t *mesh)
{
	maliasvert_t *src_oldvert = &mesh->verts[oldframenum * mesh->numverts];
	maliasvert_t *src_newvert = &mesh->verts[newframenum * mesh->numverts];
	vec_t *dst_vert = tess.vertices;
	int count = mesh->numverts;

	while (count--)
	{
		dst_vert[0] =
			src_oldvert->pos[0] * oldscale[0] +
			src_newvert->pos[0] * newscale[0] + translate[0];
		dst_vert[1] =
			src_oldvert->pos[1] * oldscale[1] +
			src_newvert->pos[1] * newscale[1] + translate[1];
		dst_vert[2] =
			src_oldvert->pos[2] * oldscale[2] +
			src_newvert->pos[2] * newscale[2] + translate[2];
		dst_vert += 4;
		src_oldvert++;
		src_newvert++;
	}
}

static glCullResult_t cull_static_model(model_t *model)
{
	maliasframe_t *newframe = &model->frames[newframenum];
	vec3_t bounds[2];
	glCullResult_t cull;

	if (glr.entrotated)
	{
		cull = GL_CullSphere(origin, newframe->radius);

		if (cull == CULL_OUT)
		{
			c.spheresCulled++;
			return cull;
		}

		if (cull == CULL_CLIP)
		{
			cull = GL_CullLocalBox(origin, newframe->bounds);

			if (cull == CULL_OUT)
			{
				c.rotatedBoxesCulled++;
				return cull;
			}
		}
	}
	else
	{
		VectorAdd(newframe->bounds[0], origin, bounds[0]);
		VectorAdd(newframe->bounds[1], origin, bounds[1]);
		cull = GL_CullBox(bounds);

		if (cull == CULL_OUT)
		{
			c.boxesCulled++;
			return cull;
		}
	}

	VectorCopy(newframe->scale, newscale);
	VectorCopy(newframe->translate, translate);
	return cull;
}

static glCullResult_t cull_lerped_model(model_t *model)
{
	maliasframe_t *newframe = &model->frames[newframenum];
	maliasframe_t *oldframe = &model->frames[oldframenum];
	vec3_t bounds[2];
	vec_t radius;
	glCullResult_t cull;

	if (glr.entrotated)
	{
		radius = newframe->radius > oldframe->radius ?
			newframe->radius : oldframe->radius;
		cull = GL_CullSphere(origin, radius);

		if (cull == CULL_OUT)
		{
			c.spheresCulled++;
			return cull;
		}

		UnionBounds(newframe->bounds, oldframe->bounds, bounds);

		if (cull == CULL_CLIP)
		{
			cull = GL_CullLocalBox(origin, bounds);

			if (cull == CULL_OUT)
			{
				c.rotatedBoxesCulled++;
				return cull;
			}
		}
	}
	else
	{
		UnionBounds(newframe->bounds, oldframe->bounds, bounds);
		VectorAdd(bounds[0], origin, bounds[0]);
		VectorAdd(bounds[1], origin, bounds[1]);
		cull = GL_CullBox(bounds);

		if (cull == CULL_OUT)
		{
			c.boxesCulled++;
			return cull;
		}
	}

	VectorScale(oldframe->scale, backlerp, oldscale);
	VectorScale(newframe->scale, frontlerp, newscale);
	LerpVector2(oldframe->translate, newframe->translate,
		backlerp, frontlerp, translate);
	return cull;
}

static void setup_color(void)
{
	int flags = glr.ent->flags;
	float f, m;
	int i;
	memset(&glr.lightpoint, 0, sizeof(glr.lightpoint));

	if (flags & RF_SHELL_MASK)
	{
		VectorClear(color);

		if (flags & RF_SHELL_HALF_DAM)
		{
			color[0] = 0.56f;
			color[1] = 0.59f;
			color[2] = 0.45f;
		}

		if (flags & RF_SHELL_DOUBLE)
		{
			color[0] = 0.9f;
			color[1] = 0.7f;
		}

		if (flags & RF_SHELL_RED)
			color[0] = 1;

		if (flags & RF_SHELL_GREEN)
			color[1] = 1;

		if (flags & RF_SHELL_BLUE)
			color[2] = 1;
	}
	else if (flags & RF_FULLBRIGHT)
		VectorSet(color, 1, 1, 1);
	else if ((flags & RF_IR_VISIBLE) && (glr.fd.rdflags & RDF_IRGOGGLES))
		VectorSet(color, 1, 0, 0);
	else
	{
		GL_LightPoint(origin, color);

		if (flags & RF_MINLIGHT)
		{
			for (i = 0; i < 3; i++)
			{
				if (color[i] > 0.1f)
					break;
			}

			if (i == 3)
				VectorSet(color, 0.1f, 0.1f, 0.1f);
		}

		if (flags & RF_GLOW)
		{
			f = 0.1f * sin(glr.fd.time * 7);

			for (i = 0; i < 3; i++)
			{
				m = color[i] * 0.8f;
				color[i] += f;

				if (color[i] < m)
					color[i] = m;
			}
		}

		// Generations
		if (flags & RF_FROZEN)
		{
			color[0] /= 3;
			color[1] /= 3;
			color[2] *= 3;
		}

		for (i = 0; i < 3; i++)
			clamp(color[i], 0, 1);
	}

	if (flags & RF_TRANSLUCENT)
		color[3] = glr.ent->alpha;
	else
		color[3] = 1;
}

static void setup_shadow(void)
{
	GLfloat matrix[16], tmp[16];
	cplane_t *plane;
	vec3_t dir;
	shadowmatrix[15] = 0;

	if (!gl_shadows->integer)
		return;

	if (glr.ent->flags & (RF_WEAPONMODEL | RF_NOSHADOW))
		return;

	if (!glr.lightpoint.surf)
		return;

	// position fake light source straight over the model
	if (glr.lightpoint.surf->drawflags & DSURF_PLANEBACK)
		VectorSet(dir, 0, 0, -1);
	else
		VectorSet(dir, 0, 0, 1);

	// project shadow on ground plane
	plane = &glr.lightpoint.plane;
	matrix[0] = plane->normal[1] * dir[1] + plane->normal[2] * dir[2];
	matrix[4] = -plane->normal[1] * dir[0];
	matrix[8] = -plane->normal[2] * dir[0];
	matrix[12] = plane->dist * dir[0];
	matrix[1] = -plane->normal[0] * dir[1];
	matrix[5] = plane->normal[0] * dir[0] + plane->normal[2] * dir[2];
	matrix[9] = -plane->normal[2] * dir[1];
	matrix[13] = plane->dist * dir[1];
	matrix[2] = -plane->normal[0] * dir[2];
	matrix[6] = -plane->normal[1] * dir[2];
	matrix[10] = plane->normal[0] * dir[0] + plane->normal[1] * dir[1];
	matrix[14] = plane->dist * dir[2];
	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 0;
	matrix[15] = DotProduct(plane->normal, dir);
	GL_MultMatrix(tmp, glr.viewmatrix, matrix);
	// rotate for entity
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
	GL_MultMatrix(shadowmatrix, tmp, matrix);
}

static void draw_shadow(maliasmesh_t *mesh)
{
	if (shadowmatrix[15] < 0.5f)
		return;

	// load shadow projection matrix
	GL_LoadMatrix(shadowmatrix);

	// eliminate z-fighting by utilizing stencil buffer, if available
	if (gl_config.stencilbits)
	{
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_EQUAL, 0, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	}

	GL_StateBits(GLS_BLEND_BLEND);
	GL_BindTexture(0, TEXNUM_WHITE);
	GL_ArrayBits(GLA_VERTEX);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1.0f, -2.0f);
	GL_Color(0, 0, 0, color[3] * 0.5f);
	glDrawElements(GL_TRIANGLES, mesh->numindices, GL_INDEX_ENUM,
		mesh->indices);
	glDisable(GL_POLYGON_OFFSET_FILL);

	// once we have drawn something to stencil buffer, continue to clear it for
	// the lifetime of OpenGL context. leaving stencil buffer "dirty" and
	// clearing just depth is slower (verified for Nvidia and ATI drivers).
	if (gl_config.stencilbits)
	{
		glDisable(GL_STENCIL_TEST);
		gl_static.stencil_buffer_bit |= GL_STENCIL_BUFFER_BIT;
	}
}

static image_t *image_for_mesh(maliasmesh_t *mesh)
{
	entity_t *ent = glr.ent;

	if (ent->flags & RF_SHELL_MASK)
		return NULL;

	if (ent->skin)
		return IMG_ForHandle(ent->skin, GAME_Q2);

	if (!mesh->numskins)
		return NULL;

	if (ent->skinnum < 0 || ent->skinnum >= mesh->numskins)
	{
		Com_DPrintf("%s: no such skin: %d\n", "GL_DrawAliasModel", ent->skinnum);
		return mesh->skins[0];
	}

	if (mesh->skins[ent->skinnum]->texnum == TEXNUM_DEFAULT)
		return mesh->skins[0];

	// Generations
	if (ent->flags & RF_SKIN_ANIM)
		ent->skinnum = (int)(glr.fd.time * 5) % mesh->numskins;

	return mesh->skins[ent->skinnum];
}

static int texnum_for_mesh(maliasmesh_t *mesh)
{
	entity_t *ent = glr.ent;

	if (ent->flags & RF_SHELL_MASK)
		return TEXNUM_WHITE;

	if (ent->skin)
		return IMG_ForHandle(ent->skin, GAME_Q2)->texnum;

	if (!mesh->numskins)
		return TEXNUM_DEFAULT;

	if (ent->skinnum < 0 || ent->skinnum >= mesh->numskins)
	{
		Com_DPrintf("%s: no such skin: %d\n", "GL_DrawAliasModel", ent->skinnum);
		return mesh->skins[0]->texnum;
	}

	if (mesh->skins[ent->skinnum]->texnum == TEXNUM_DEFAULT)
		return mesh->skins[0]->texnum;

	// Generations
	if (ent->flags & RF_SKIN_ANIM)
		ent->skinnum = (int)(glr.fd.time * 5) % mesh->numskins;

	return mesh->skins[ent->skinnum]->texnum;
}

static void draw_alias_mesh(maliasmesh_t *mesh)
{
	glStateBits_t state = GLS_INTENSITY_ENABLE;
	// fall back to entity matrix
	GL_LoadMatrix(glr.entmatrix);

	if (glr.ent->flags & RF_TRANSLUCENT)
		state |= GLS_BLEND_BLEND;

	if ((glr.ent->flags & (RF_TRANSLUCENT | RF_WEAPONMODEL)) == RF_TRANSLUCENT)
		state |= GLS_DEPTHMASK_FALSE;

	GL_StateBits(state);
	GL_BindTexture(0, texnum_for_mesh(mesh));
	(*tessfunc)(mesh);
	c.trisDrawn += mesh->numtris;

	if (shadelight)
	{
		GL_ArrayBits(GLA_VERTEX | GLA_TC | GLA_COLOR);
		GL_VertexPointer(3, VERTEX_SIZE, tess.vertices);
		GL_ColorFloatPointer(4, VERTEX_SIZE, tess.vertices + 4);
	}
	else
	{
		GL_ArrayBits(GLA_VERTEX | GLA_TC);
		GL_VertexPointer(3, 4, tess.vertices);
		GL_Color(color[0], color[1], color[2], color[3]);
	}

	GL_TexCoordPointer(2, 0, (GLfloat *)mesh->tcoords);
	glDrawElements(GL_TRIANGLES, mesh->numindices, GL_INDEX_ENUM,
		mesh->indices);

	if (gl_showtris->integer)
		GL_DrawOutlines(mesh->numindices, mesh->indices);

	draw_shadow(mesh);
}

// Generations
static void draw_alias_mesh_2d(maliasmesh_t *mesh)
{
	(*tessfunc)(mesh);
	GL_BindTexture(0, texnum_for_mesh(mesh));
	image_t *img = image_for_mesh(mesh);

	if (img && !(img->flags & IF_OLDSCHOOL))
		GL_SetFilterAndRepeat(img->type, img->flags | (IF_OLDSCHOOL | IF_CRISPY));

	GL_TexCoordPointer(2, 0, (GLfloat *)mesh->tcoords);
	glDrawElements(GL_TRIANGLES, mesh->numindices, GL_INDEX_ENUM,
		mesh->indices);

	if (img && !(img->flags & IF_OLDSCHOOL))
		GL_SetFilterAndRepeat(img->type, img->flags);
}

static void GL3_MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
	// calculation of left, right, bottom, top is from R_MYgluPerspective() of old gl backend
	// which seems to be slightly different from the real gluPerspective()
	// and thus also from HMM_Perspective()
	GLdouble left, right, bottom, top;
	float A, B, C, D;
	top = zNear * tan(fovy * M_PI / 360.0);
	bottom = -top;
	left = bottom * aspect;
	right = top * aspect;
	// TODO:  stereo stuff
	// left += - gl1_stereo_convergence->value * (2 * gl_state.camera_separation) / zNear;
	// right += - gl1_stereo_convergence->value * (2 * gl_state.camera_separation) / zNear;
	// the following emulates glFrustum(left, right, bottom, top, zNear, zFar)
	// see https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glFrustum.xml
	A = (right + left) / (right - left);
	B = (top + bottom) / (top - bottom);
	C = -(zFar + zNear) / (zFar - zNear);
	D = -(2.0 * zFar * zNear) / (zFar - zNear);
	gl_static.backend.proj_matrix((const float [])
	{
		(2.0 * zNear) / (right - left), 0, 0, 0, // first *column*
		0, (2.0 * zNear) / (top - bottom), 0, 0,
		A, B, C, -1.0,
		0, 0, D, 0
	});
}

static void calc_mins_maxs(const maliasmesh_t *mesh, vec3_t mins, vec3_t maxs)
{
	maliasvert_t *src_vert = &mesh->verts[newframenum * mesh->numverts];
	vec_t *dst_vert = tess.vertices;
	int count = mesh->numverts;

	while (count--)
	{
		vec3_t v =
		{
			src_vert->pos[0] *newscale[0] + translate[0],
			src_vert->pos[1] *newscale[1] + translate[1],
			src_vert->pos[2] *newscale[2] + translate[2]
		};
		AddPointToBounds(v, mins, maxs);
		src_vert++;
	}
}

static void GL_DrawAliasSpriteModel(entity_t *ent, model_t *model, vec3_t origin)
{
	int i;
	vec3_t mins, maxs;
	ClearBounds(mins, maxs);

	for (i = 0; i < model->nummeshes; i++)
	{
		maliasmesh_t *mesh = &model->meshes[i];
		calc_mins_maxs(mesh, mins, maxs);
	}

	image_t *image = R_FRAMEBUFFERTEXTURE;
	float w = ((maxs[0] - mins[0]) + (maxs[1] - mins[1])), h = (maxs[2] - mins[2]);
	float tw = (w / image->width) / 2;
	float yofs = 0.5 + ((mins[2] + translate[2]) / image->height);
	w /= CRISPY_TEXTURE_SCALE;
	const vec_t tcoords[8] =
	{
		0.5 - tw, 0.5 + (-mins[2] / image->height),
		0.5 - tw, 0.5 - (maxs[2] / image->height),
		0.5 + tw, 0.5 + (-mins[2] / image->height),
		0.5 + tw, 0.5 - (maxs[2] / image->height)
	};
	entity_t *e = glr.ent;
	float alpha = 1;
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
	vec3_t color = { 1, 1, 1 };

	if (ent->flags & RF_SPECTRE)
		color[0] = color[1] = color[2] = 0;

	GL_Color(color[0], color[1], color[2], alpha);
	VectorScale(glr.viewaxis_nopitch[1], (w / 2), left);
	VectorScale(glr.viewaxis_nopitch[1], -(w / 2), right);
	VectorScale(glr.viewaxis_nopitch[2], mins[2] / CRISPY_TEXTURE_SCALE, down);
	VectorScale(glr.viewaxis_nopitch[2], (h + mins[2]) / CRISPY_TEXTURE_SCALE, up);
	VectorAdd3(e->origin, down, left, points[0]);
	VectorAdd3(e->origin, up, left, points[1]);
	VectorAdd3(e->origin, down, right, points[2]);
	VectorAdd3(e->origin, up, right, points[3]);
	GL_TexCoordPointer(2, 0, tcoords);
	GL_VertexPointer(3, 0, &points[0][0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static float yaw_for_screen_position(vec3_t origin, vec3_t ent_angles)
{
	vec3_t diff, fwd;
	VectorSubtract(glr.fd.vieworg, origin, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	vectoangles2(diff, fwd);
	return fwd[1] - ent_angles[1];
}

static void GL_RotateFor2DEntity(float yaw)
{
	yaw = round(yaw / 45) * 45;
	vec3_t angles = { yaw + 90, 0, 90 };
	vec3_t axis[3];
	AnglesToAxis(angles, axis);
	GLfloat matrix[16];
	matrix[0] = axis[0][0];
	matrix[4] = axis[1][0];
	matrix[8] = axis[2][0];
	matrix[12] = R_FRAMEBUFFERTEXTURE->width / 2;
	matrix[1] = -axis[0][1];
	matrix[5] = -axis[1][1];
	matrix[9] = -axis[2][1];
	matrix[13] = R_FRAMEBUFFERTEXTURE->height / 2;
	matrix[2] = -axis[0][2];
	matrix[6] = -axis[1][2];
	matrix[10] = -axis[2][2];
	matrix[14] = 0;
	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 0;
	matrix[15] = 1;
	GL_ForceMatrix(matrix);
}

static void setup_alias_mesh_2d(void)
{
	VectorScale(newscale, CRISPY_TEXTURE_SCALE, newscale);
	VectorScale(translate, CRISPY_TEXTURE_SCALE, translate);
	GL_SetupOrtho(0, 0, R_FRAMEBUFFERTEXTURE->width, R_FRAMEBUFFERTEXTURE->height, -256, 256);
	GL_BeginRenderToTexture(R_FRAMEBUFFERTEXTURE->frame_buffer);
	GL_BindTexture(0, TEXNUM_WHITE);
	GL_StateBits(GLS_DEFAULT);

	if (shadelight)
	{
		GL_ArrayBits(GLA_VERTEX | GLA_TC | GLA_COLOR);
		GL_VertexPointer(3, VERTEX_SIZE, tess.vertices);
		GL_ColorFloatPointer(4, VERTEX_SIZE, tess.vertices + 4);
	}
	else
	{
		GL_ArrayBits(GLA_VERTEX | GLA_TC);
		GL_VertexPointer(3, 4, tess.vertices);
		GL_Color(color[0], color[1], color[2], color[3]);
	}

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glFrontFace(GL_CCW);
}

static void end_alias_mesh_2d(void)
{
	glFrontFace(GL_CW);
	glClearColor(0, 0, 0, 1);
	GL_EndRenderToTexture();
	GL_Setup3D(false);
}

void GL_DrawAliasModel(model_t *model)
{
	int i;
	entity_t *ent = glr.ent;
	glCullResult_t cull;
	bool renderAs2D = (CL_GetClientGame() == GAME_DOOM || CL_GetClientGame() == GAME_DUKE);
	newframenum = ent->frame;

	if (newframenum < 0 || newframenum >= model->numframes)
	{
		Com_DPrintf("%s: no such frame %d\n", __func__, newframenum);
		newframenum = 0;
	}

	oldframenum = ent->oldframe;

	if (oldframenum < 0 || oldframenum >= model->numframes)
	{
		Com_DPrintf("%s: no such oldframe %d\n", __func__, oldframenum);
		oldframenum = 0;
	}

	backlerp = ent->backlerp;
	frontlerp = 1.0f - backlerp;

	// interpolate origin, if necessarry
	if (ent->flags & RF_FRAMELERP)
		LerpVector2(ent->oldorigin, ent->origin,
			backlerp, frontlerp, origin);
	else
		VectorCopy(ent->origin, origin);

	// Generations
	if ((ent->flags & RF_NOLERP) || model->nolerp || renderAs2D)
	{
		backlerp = 0;
		frontlerp = 1;
	}

	// optimized case
	if (backlerp == 0)
		oldframenum = newframenum;

	// cull the model, setup scale and translate vectors
	if (newframenum == oldframenum)
		cull = cull_static_model(model);
	else
		cull = cull_lerped_model(model);

	if (cull == CULL_OUT)
		return;

	// setup parameters common for all meshes
	setup_color();
	setup_dotshading();

	if (!renderAs2D)
		setup_shadow();

	// select proper tessfunc
	if (ent->flags & RF_SHELL_MASK)
	{
		shellscale = (ent->flags & RF_WEAPONMODEL) ?
			WEAPONSHELL_SCALE : POWERSUIT_SCALE;
		tessfunc = newframenum == oldframenum ?
			tess_static_shell : tess_lerped_shell;
	}
	else if (shadelight)
	{
		tessfunc = newframenum == oldframenum ?
			tess_static_shade : tess_lerped_shade;
	}
	else
	{
		tessfunc = newframenum == oldframenum ?
			tess_static_plain : tess_lerped_plain;
	}

	if (renderAs2D)
		setup_alias_mesh_2d();

	if (renderAs2D)
		GL_RotateFor2DEntity(yaw_for_screen_position(origin, ent->angles));
	else
		GL_RotateForEntity(origin);

	if (ent->flags & RF_WEAPONMODEL)
	{
		// render weapon with a different FOV (r_gunfov) so it's not distorted at high view FOV
		float screenaspect = (float)r_config.width / r_config.height;
		GL3_MYgluPerspective(scr_gunfov->value, screenaspect, gl_znear->value, gl_static.world.size);
	}

	if ((ent->flags & (RF_WEAPONMODEL | RF_LEFTHAND)) ==
		(RF_WEAPONMODEL | RF_LEFTHAND))
	{
		GL_Reflect();
		glFrontFace(GL_CCW);
	}

	if (ent->flags & RF_DEPTHHACK)
		GL_DepthRange(0, 0.25f);

	// draw all the meshes
	for (i = 0; i < model->nummeshes; i++)
	{
		if (renderAs2D)
			draw_alias_mesh_2d(&model->meshes[i]);
		else
			draw_alias_mesh(&model->meshes[i]);
	}

	if (ent->flags & RF_DEPTHHACK)
		GL_DepthRange(0, 1);

	if ((ent->flags & (RF_WEAPONMODEL | RF_LEFTHAND)) ==
		(RF_WEAPONMODEL | RF_LEFTHAND))
		glFrontFace(GL_CW);

	if (ent->flags & RF_WEAPONMODEL)
		GL_Frustum();

	if (renderAs2D)
	{
		end_alias_mesh_2d();
		GL_DrawAliasSpriteModel(ent, model, origin);
	}
}

#define TARGA_HEADER_SIZE  18

static int GL_WriteTGA(const char *filename, byte *pixels)
{
	qhandle_t handle;
	FS_FOpenFile(filename, &handle, FS_MODE_WRITE);
	byte header[TARGA_HEADER_SIZE], *p;
	int i, j;
	memset(&header, 0, sizeof(header));
	header[2] = 2;        // uncompressed type
	header[12] = R_FRAMEBUFFERTEXTURE->width & 255;
	header[13] = R_FRAMEBUFFERTEXTURE->width >> 8;
	header[14] = R_FRAMEBUFFERTEXTURE->height & 255;
	header[15] = R_FRAMEBUFFERTEXTURE->height >> 8;
	header[16] = 32;     // pixel size
	FS_Write(&header, sizeof(header), handle);
	int row_stride = R_FRAMEBUFFERTEXTURE->width * 4;

	// swap RGB to BGR
	for (i = 0; i < R_FRAMEBUFFERTEXTURE->height; i++)
	{
		p = pixels + i * row_stride;

		for (j = 0; j < R_FRAMEBUFFERTEXTURE->width; j++)
		{
			byte tmp;
			tmp = p[2];
			p[2] = p[0];
			p[0] = tmp;
			p += 4;
		}
	}

	for (i = R_FRAMEBUFFERTEXTURE->height - 1; i >= 0; i--)
		FS_Write(pixels + i * row_stride, R_FRAMEBUFFERTEXTURE->width * 4, handle);

	FS_FCloseFile(handle);
	return 0;
}

void GL_OutputAliasModelAs2D(model_t *model)
{
	backlerp = 0;
	frontlerp = 1;
	shadelight = NULL;
	tessfunc = tess_static_plain;
	Vector4Set(color, 1, 1, 1, 1);
	setup_alias_mesh_2d();
	int i, x;
	char buffer[MAX_QPATH];
	Q_snprintf(buffer, sizeof(buffer), "%s/sprites/", fs_gamedir);
	FS_CreatePath(buffer);
	byte *pixels = Z_Malloc(R_FRAMEBUFFERTEXTURE->width * R_FRAMEBUFFERTEXTURE->height * 4);

	for (i = 0; i < model->numframes; i++)
	{
		int angle = 0;

		for (; angle < 360; angle += 45)
		{
			VectorCopy(model->frames[i].scale, newscale);
			VectorCopy(model->frames[i].translate, translate);
			oldframenum = newframenum = i;
			GL_RotateFor2DEntity(angle);

			for (x = 0; x < model->nummeshes; x++)
				draw_alias_mesh_2d(&model->meshes[x]);

			GL_BindTexture(0, TEXNUM_FRAMEBUFFER);
			Q_snprintf(buffer, sizeof(buffer), "sprites/out_%i_%i.tga", i, angle);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
			GL_WriteTGA(buffer, pixels);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
	}

	Z_Free(pixels);
	end_alias_mesh_2d();
}
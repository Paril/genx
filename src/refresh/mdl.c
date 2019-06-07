/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2008 Andrey Nazarov

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
#include "models.h"

#include "format/mdl.h"

static void MOD_MDLLoadFrame(dmdlheader_t *header, maliasmesh_t *dst_mesh, maliasframe_t *dst_frame, dmdlvertex_t *src_verts, float *mins, float *maxs, GLuint *indice_remap, int frame_id)
{
	LittleVector(header->scale, dst_frame->scale);
	LittleVector(header->translate, dst_frame->translate);
	ClearBounds(mins, maxs);

	for (int i = 0; i < dst_mesh->numverts; i++)
	{
		dmdlvertex_t *src_vert;

		if (i >= header->num_verts)
		{
			int original_index = -1;

			for (int k = 0; k < MDL_MAX_TRIANGLES * 3; ++k)
			{
				if (indice_remap[k] == (uint32_t)i)
				{
					original_index = k;
					break;
				}
			}

			if (original_index == -1)
				Com_Error(ERR_FATAL, "Bad MDL");

			src_vert = &src_verts[original_index];
		}
		else
			src_vert = &src_verts[i];

		maliasvert_t *dst_vert = &dst_mesh->verts[frame_id * dst_mesh->numverts + i];
		dst_vert->pos[0] = src_vert->v[0];
		dst_vert->pos[1] = src_vert->v[1];
		dst_vert->pos[2] = src_vert->v[2];

		for (int k = 0; k < 3; k++)
		{
			short val = dst_vert->pos[k];

			if (val < mins[k])
				mins[k] = val;

			if (val > maxs[k])
				maxs[k] = val;
		}
	}

	VectorVectorScale(mins, dst_frame->scale, mins);
	VectorVectorScale(maxs, dst_frame->scale, maxs);
	dst_frame->radius = RadiusFromBounds(mins, maxs);
	VectorAdd(mins, dst_frame->translate, dst_frame->bounds[0]);
	VectorAdd(maxs, dst_frame->translate, dst_frame->bounds[1]);
}

int MOD_LoadMDL(model_t *model, const void *rawdata, size_t length)
{
	dmdlheader_t    header;
	dmdlframe_t *src_frame;
	dmdlvertex_t *src_verts;
	dmdltriangle_t *src_tri;
	dmdltexcoord_t *src_tc;
	dmdlskin_t *src_skin;
	maliasframe_t *dst_frame;
	maliasmesh_t *dst_mesh;
	maliastc_t *dst_tc;
	GL_INDEX_TYPE *dst_idx;
	int             i, j;
	char            skinname[MAX_QPATH];
	vec_t           scale_s, scale_t;
	vec3_t          mins, maxs;
	byte *ptr, *pic;
	image_t *skins[MDL_MAX_SKINS];
	GL_INDEX_TYPE	indice_remap[MDL_MAX_TRIANGLES * 3];

	if (length < sizeof(header))
		return Q_ERR_FILE_TOO_SMALL;

	// byte swap the header
	header = *(dmdlheader_t *)rawdata;

	for (i = 0; i < sizeof(header) / 4; i++)
		((uint32_t *)& header)[i] = LittleLong(((uint32_t *)& header)[i]);

	// skins
	ptr = (byte *)rawdata + sizeof(dmdlheader_t);
	pic = (byte *)IMG_AllocPixels(header.skinwidth * header.skinheight * 4);

	for (i = 0; i < header.num_skins; ++i)
	{
		src_skin = (dmdlskin_t *)ptr;

		if (src_skin->group != 0)
			return Q_ERR_INVALID_FORMAT;

		Q_snprintf(skinname, MAX_QPATH, "%s_%i", model->name, i);
		skins[i] = IMG_Find(skinname, IT_SKIN, IF_DELAYED | IF_OLDSCHOOL);
		ptr += sizeof(dmdlskin_t);
		skins[i]->upload_width = skins[i]->width = header.skinwidth;
		skins[i]->upload_height = skins[i]->height = header.skinheight;
		skins[i]->flags |= IMG_Unpack8((uint32_t *)pic, ptr, header.skinwidth, header.skinheight, d_palettes[GAME_Q1]);
		IMG_Load(skins[i], pic);
		ptr += header.skinwidth * header.skinheight;
	}

	IMG_FreePixels(pic);
	src_tc = (dmdltexcoord_t *)ptr;
	ptr += sizeof(dmdltexcoord_t) * header.num_verts;
	src_tri = (dmdltriangle_t *)ptr;
	ptr += sizeof(dmdltriangle_t) * header.num_tris;
	int num_real_verts = header.num_verts;

	// calculate real number of indices
	for (i = 0; i < MDL_MAX_TRIANGLES * 3; ++i)
		indice_remap[i] = (uint32_t)-1;

	for (i = 0; i < header.num_tris; i++)
	{
		// Calculate the backface skinvert positioning
		for (j = 0; j < 3; ++j)
		{
			if (src_tri[i].facesfront == 0 && src_tc[src_tri[i].vertex[j]].onseam != 0)
			{
				if (indice_remap[src_tri[i].vertex[j]] == (uint32_t)-1)
					indice_remap[src_tri[i].vertex[j]] = num_real_verts++;
			}
		}
	}

	// count real number of frames
	dmdlframe_t *src_frames = (dmdlframe_t *)ptr;
	ptr = (byte *)src_frames;
	int real_num_frames = 0;

	for (j = 0; j < header.num_frames; ++j)
	{
		src_frame = (dmdlframe_t *)ptr;
		ptr += sizeof(dmdlframe_t);

		if (src_frame->type == 0)
		{
			// verts directly follow simple frames
			ptr += sizeof(dmdlsimpleframe_t);
			++real_num_frames;
			ptr += sizeof(dmdlvertex_t) * header.num_verts;
		}
		else
		{
			// groups are complicated :(
			dmdlgroupframe_t *group = (dmdlgroupframe_t *)ptr;
			real_num_frames += group->num_frames;
			ptr += sizeof(dmdlgroupframe_t);
			// skip timings
			ptr += sizeof(float) * group->num_frames;
			// skip simple frames
			ptr += sizeof(dmdlsimpleframe_t) * group->num_frames;
			// add frames
			ptr += sizeof(dmdlvertex_t) * header.num_verts * group->num_frames;
		}
	}

	header.num_frames = real_num_frames;
	Z_TagChunkCreate(TAG_MODEL, &model->memory, sizeof(maliasmesh_t) +
		(header.num_frames * sizeof(maliasframe_t)) +
					 (num_real_verts * header.num_frames * sizeof(maliasvert_t)) +
					 (num_real_verts * sizeof(maliastc_t)) +
					 (header.num_tris * 3 * sizeof(GL_INDEX_TYPE)));
	model->type = MOD_ALIAS;
	model->nummeshes = 1;
	model->numframes = header.num_frames;
	model->meshes = Z_ChunkAlloc(&model->memory, sizeof(maliasmesh_t));
	model->frames = Z_ChunkAlloc(&model->memory, header.num_frames * sizeof(maliasframe_t));
	model->nolerp = true;
	dst_mesh = model->meshes;
	dst_mesh->numtris = header.num_tris;
	dst_mesh->numindices = header.num_tris * 3;
	dst_mesh->numverts = num_real_verts;
	dst_mesh->numskins = header.num_skins;
	dst_mesh->verts = Z_ChunkAlloc(&model->memory, num_real_verts * header.num_frames * sizeof(maliasvert_t));
	dst_mesh->tcoords = Z_ChunkAlloc(&model->memory, num_real_verts * sizeof(maliastc_t));
	dst_mesh->indices = Z_ChunkAlloc(&model->memory, header.num_tris * 3 * sizeof(GL_INDEX_TYPE));

	for (i = 0; i < header.num_skins; ++i)
		dst_mesh->skins[i] = skins[i];

	dst_tc = dst_mesh->tcoords;
	scale_s = 1.0f / header.skinwidth;
	scale_t = 1.0f / header.skinheight;

	for (i = 0; i < dst_mesh->numverts; i++)
	{
		if (i >= header.num_verts)
		{
			int original_index = -1;

			for (j = 0; j < MDL_MAX_TRIANGLES * 3; ++j)
			{
				if (indice_remap[j] == (uint32_t)i)
				{
					original_index = j;
					break;
				}
			}

			dst_tc[i].st[0] = (src_tc[original_index].s + 0.5f) * scale_s;
			dst_tc[i].st[1] = (src_tc[original_index].t + 0.5f) * scale_t;
			dst_tc[i].st[0] += 0.5f;
		}
		else
		{
			dst_tc[i].st[0] = (src_tc[i].s + 0.5f) * scale_s;
			dst_tc[i].st[1] = (src_tc[i].t + 0.5f) * scale_t;
		}
	}

	// load all triangle indices
	dst_idx = dst_mesh->indices;

	for (i = 0; i < header.num_tris; i++)
	{
		dst_idx[0] = src_tri[i].vertex[0];
		dst_idx[1] = src_tri[i].vertex[1];
		dst_idx[2] = src_tri[i].vertex[2];

		// Calculate the backface skinvert positioning
		for (j = 0; j < 3; ++j)
		{
			if (src_tri[i].facesfront == 0 && src_tc[dst_idx[j]].onseam != 0)
				dst_idx[j] = indice_remap[dst_idx[j]];
		}

		dst_idx += 3;
	}

	// load all frames
	dst_frame = model->frames;
	ptr = (byte *)src_frames;

	for (j = 0; j < header.num_frames;)
	{
		src_frame = (dmdlframe_t *)ptr;
		ptr += sizeof(dmdlframe_t);

		if (src_frame->type == 0)
		{
			// verts directly follow simple frames
			ptr += sizeof(dmdlsimpleframe_t);
			src_verts = (dmdlvertex_t *)ptr;
			MOD_MDLLoadFrame(&header, dst_mesh, dst_frame, src_verts, mins, maxs, indice_remap, j);
			dst_frame++;
			ptr += sizeof(dmdlvertex_t) * header.num_verts;
			j++;
		}
		else
		{
			// groups are complicated :(
			dmdlgroupframe_t *group = (dmdlgroupframe_t *)ptr;
			ptr += sizeof(dmdlgroupframe_t);
			// skip timings
			ptr += sizeof(float) * group->num_frames;

			// add frames
			for (i = 0; i < group->num_frames; ++i)
			{
				ptr += sizeof(dmdlsimpleframe_t);
				src_verts = (dmdlvertex_t *)ptr;
				MOD_MDLLoadFrame(&header, dst_mesh, dst_frame, src_verts, mins, maxs, indice_remap, j);
				dst_frame++;
				ptr += sizeof(dmdlvertex_t) * header.num_verts;
				j++;
			}
		}
	}

	MOD_CalculateNormals(model);

	return Q_ERR_SUCCESS;
}
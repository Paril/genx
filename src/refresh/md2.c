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

#include "format/md2.h"

static int MOD_ValidateMD2(dmd2header_t *header, size_t length)
{
	size_t end;

	// check ident and version
	if (header->ident != MD2_IDENT)
		return Q_ERR_UNKNOWN_FORMAT;

	if (header->version != MD2_VERSION)
		return Q_ERR_UNKNOWN_FORMAT;

	// check triangles
	if (header->num_tris < 1)
		return Q_ERR_TOO_FEW;

	if (header->num_tris > MD2_MAX_TRIANGLES)
		return Q_ERR_TOO_MANY;

	end = header->ofs_tris + sizeof(dmd2triangle_t) * header->num_tris;

	if (header->ofs_tris < sizeof(header) || end < header->ofs_tris || end > length)
		return Q_ERR_BAD_EXTENT;

	// check st
	if (header->num_st < 3)
		return Q_ERR_TOO_FEW;

	if (header->num_st > MAX_ALIAS_VERTS)
		return Q_ERR_TOO_MANY;

	end = header->ofs_st + sizeof(dmd2stvert_t) * header->num_st;

	if (header->ofs_st < sizeof(header) || end < header->ofs_st || end > length)
		return Q_ERR_BAD_EXTENT;

	// check xyz and frames
	if (header->num_xyz < 3)
		return Q_ERR_TOO_FEW;

	if (header->num_xyz > MAX_ALIAS_VERTS)
		return Q_ERR_TOO_MANY;

	if (header->num_frames < 1)
		return Q_ERR_TOO_FEW;

	if (header->num_frames > MD2_MAX_FRAMES)
		return Q_ERR_TOO_MANY;

	end = sizeof(dmd2frame_t) + (header->num_xyz - 1) * sizeof(dmd2trivertx_t);

	if (header->framesize < end || header->framesize > MD2_MAX_FRAMESIZE)
		return Q_ERR_BAD_EXTENT;

	end = header->ofs_frames + (size_t)header->framesize * header->num_frames;

	if (header->ofs_frames < sizeof(header) || end < header->ofs_frames || end > length)
		return Q_ERR_BAD_EXTENT;

	// check skins
	if (header->num_skins)
	{
		if (header->num_skins > MAX_ALIAS_SKINS)
			return Q_ERR_TOO_MANY;

		end = header->ofs_skins + (size_t)MD2_MAX_SKINNAME * header->num_skins;

		if (header->ofs_skins < sizeof(header) || end < header->ofs_skins || end > length)
			return Q_ERR_BAD_EXTENT;
	}

	if (header->skinwidth < 1)
		return Q_ERR_INVALID_FORMAT;

	if (header->skinheight < 1)
		return Q_ERR_INVALID_FORMAT;

	return Q_ERR_SUCCESS;
}

int MOD_LoadMD2(model_t *model, const void *rawdata, size_t length)
{
	dmd2header_t    header;
	dmd2frame_t *src_frame;
	dmd2trivertx_t *src_vert;
	dmd2triangle_t *src_tri;
	dmd2stvert_t *src_tc;
	char *src_skin;
	maliasframe_t *dst_frame;
	maliasvert_t *dst_vert;
	maliasmesh_t *dst_mesh;
	maliastc_t *dst_tc;
	int             i, j, k, ret;
	uint16_t        remap[TESS_MAX_INDICES];
	uint16_t        vertIndices[TESS_MAX_INDICES];
	uint16_t        tcIndices[TESS_MAX_INDICES];
	uint16_t        finalIndices[TESS_MAX_INDICES];
	int             numverts, numindices;
	char            skinname[MAX_QPATH];
	vec_t           scale_s, scale_t;
	vec3_t          mins, maxs;

	if (length < sizeof(header))
		return Q_ERR_FILE_TOO_SMALL;

	// byte swap the header
	header = *(dmd2header_t *)rawdata;

	for (i = 0; i < sizeof(header) / 4; i++)
		((uint32_t *)& header)[i] = LittleLong(((uint32_t *)& header)[i]);

	// validate the header
	ret = MOD_ValidateMD2(&header, length);

	if (ret)
	{
		if (ret == Q_ERR_TOO_FEW)
		{
			// empty models draw nothing
			model->type = MOD_EMPTY;
			return Q_ERR_SUCCESS;
		}

		return ret;
	}

	// load all triangle indices
	numindices = 0;
	src_tri = (dmd2triangle_t *)((byte *)rawdata + header.ofs_tris);

	for (i = 0; i < header.num_tris; i++)
	{
		for (j = 0; j < 3; j++)
		{
			uint16_t idx_xyz = LittleShort(src_tri->index_xyz[j]);
			uint16_t idx_st = LittleShort(src_tri->index_st[j]);

			// some broken models have 0xFFFF indices
			if (idx_xyz >= header.num_xyz || idx_st >= header.num_st)
				break;

			vertIndices[numindices + j] = idx_xyz;
			tcIndices[numindices + j] = idx_st;
		}

		if (j == 3)
		{
			// only count good triangles
			numindices += 3;
		}

		src_tri++;
	}

	if (numindices < 3)
		return Q_ERR_TOO_FEW;

	for (i = 0; i < numindices; i++)
		remap[i] = 0xFFFF;

	// remap all triangle indices
	numverts = 0;
	src_tc = (dmd2stvert_t *)((byte *)rawdata + header.ofs_st);

	for (i = 0; i < numindices; i++)
	{
		if (remap[i] != 0xFFFF)
		{
			continue; // already remapped
		}

		for (j = i + 1; j < numindices; j++)
		{
			if (vertIndices[i] == vertIndices[j] &&
				(src_tc[tcIndices[i]].s == src_tc[tcIndices[j]].s &&
				src_tc[tcIndices[i]].t == src_tc[tcIndices[j]].t))
			{
				// duplicate vertex
				remap[j] = i;
				finalIndices[j] = numverts;
			}
		}

		// new vertex
		remap[i] = i;
		finalIndices[i] = numverts++;
	}

	if (numverts > TESS_MAX_VERTICES)
		return Q_ERR_TOO_MANY;

	Z_TagChunkCreate(TAG_MODEL, &model->memory, sizeof(maliasmesh_t) +
		(header.num_frames * sizeof(maliasframe_t)) +
					 (numverts * header.num_frames * sizeof(maliasvert_t)) +
					 (numverts * sizeof(maliastc_t)) +
					 (numindices * sizeof(GL_INDEX_TYPE)));
	model->type = MOD_ALIAS;
	model->nummeshes = 1;
	model->numframes = header.num_frames;
	model->meshes = Z_ChunkAlloc(&model->memory, sizeof(maliasmesh_t));
	model->frames = Z_ChunkAlloc(&model->memory, header.num_frames * sizeof(maliasframe_t));
	dst_mesh = model->meshes;
	dst_mesh->numtris = numindices / 3;
	dst_mesh->numindices = numindices;
	dst_mesh->numverts = numverts;
	dst_mesh->numskins = header.num_skins;
	dst_mesh->verts = Z_ChunkAlloc(&model->memory, numverts * header.num_frames * sizeof(maliasvert_t));
	dst_mesh->tcoords = Z_ChunkAlloc(&model->memory, numverts * sizeof(maliastc_t));
	dst_mesh->indices = Z_ChunkAlloc(&model->memory, numindices * sizeof(GL_INDEX_TYPE));

	if (dst_mesh->numtris != header.num_tris)
		Com_DPrintf("%s has %d bad triangles\n", model->name, header.num_tris - dst_mesh->numtris);

	// store final triangle indices
	for (i = 0; i < numindices; i++)
		dst_mesh->indices[i] = finalIndices[i];

	// load all skins
	src_skin = (char *)rawdata + header.ofs_skins;

	for (i = 0; i < header.num_skins; i++)
	{
		if (!Q_memccpy(skinname, src_skin, 0, sizeof(skinname)))
		{
			ret = Q_ERR_STRING_TRUNCATED;
			goto fail;
		}

		FS_NormalizePath(skinname, skinname);
		dst_mesh->skins[i] = IMG_Find(skinname, IT_SKIN, model->nolerp ? IF_OLDSCHOOL : IF_NONE);
		src_skin += MD2_MAX_SKINNAME;
	}

	// load all tcoords
	src_tc = (dmd2stvert_t *)((byte *)rawdata + header.ofs_st);
	dst_tc = dst_mesh->tcoords;
	scale_s = 1.0f / header.skinwidth;
	scale_t = 1.0f / header.skinheight;

	for (i = 0; i < numindices; i++)
	{
		if (remap[i] != i)
			continue;

		dst_tc[finalIndices[i]].st[0] =
			(int16_t)LittleShort(src_tc[tcIndices[i]].s) * scale_s;
		dst_tc[finalIndices[i]].st[1] =
			(int16_t)LittleShort(src_tc[tcIndices[i]].t) * scale_t;
	}

	// load all frames
	src_frame = (dmd2frame_t *)((byte *)rawdata + header.ofs_frames);
	dst_frame = model->frames;

	for (j = 0; j < header.num_frames; j++)
	{
		LittleVector(src_frame->scale, dst_frame->scale);
		LittleVector(src_frame->translate, dst_frame->translate);
		// load frame vertices
		ClearBounds(mins, maxs);

		for (i = 0; i < numindices; i++)
		{
			if (remap[i] != i)
				continue;

			src_vert = &src_frame->verts[vertIndices[i]];
			dst_vert = &dst_mesh->verts[j * numverts + finalIndices[i]];
			dst_vert->pos[0] = src_vert->v[0];
			dst_vert->pos[1] = src_vert->v[1];
			dst_vert->pos[2] = src_vert->v[2];

			for (k = 0; k < 3; k++)
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
		src_frame = (dmd2frame_t *)((byte *)src_frame + header.framesize);
		dst_frame++;
	}

	MOD_CalculateNormals(model);

	return Q_ERR_SUCCESS;
fail:
	Z_ChunkFree(&model->memory);
	return ret;
}
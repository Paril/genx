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
#include "format/sp2.h"

// Paril
#include "format/mdl.h"
#include "format/spr.h"

typedef int (*mod_load_t)(model_t *, const void *, size_t);

#define MOD_Malloc(size)    Z_TagMalloc(size, TAG_MODEL)

#if MAX_ALIAS_VERTS > TESS_MAX_VERTICES
	#error TESS_MAX_VERTICES
#endif

#if MD2_MAX_TRIANGLES > TESS_MAX_INDICES / 3
	#error TESS_MAX_INDICES
#endif

// during registration it is possible to have more models than could actually
// be referenced during gameplay, because we don't want to free anything until
// we are sure we won't need it.
#define MAX_RMODELS     (MAX_PRECACHE * 2)

static model_t      r_models[MAX_RMODELS];
static int          r_numModels;

#include "common/modelscript.h"

static modelscript_t	r_modelscripts[MAX_MODEL_SCRIPTS];
static int				r_numModelScripts;

static void MSCR_Untouch()
{
	for (int i = 0; i < r_numModelScripts; ++i)
	{
		for (int g = 1; g < GAME_TOTAL; ++g)
		{
			if (r_modelscripts[i].entries[g].path[0])
			{
				r_modelscripts[i].entries[g].loaded = false;
				r_modelscripts[i].entries[g].model = 0;
			}
		}
	}
}

static void MSCR_Touch()
{
	for (int i = 0; i < r_numModelScripts; ++i)
	{
		for (int g = 1; g < GAME_TOTAL; ++g)
		{
			if (r_modelscripts[i].entries[g].path[0])
			{
				r_modelscripts[i].entries[g].loaded = true;
				r_modelscripts[i].entries[g].model = R_RegisterModel(r_modelscripts[i].entries[g].path);
			}
		}
	}
}

static modelscript_t *MSCR_Alloc()
{
	modelscript_t *script;
	int i;

	for (i = 0, script = r_modelscripts; i < r_numModelScripts; ++i, script++)
	{
		if (!script->name[0])
			break;
	}

	if (i == r_numModelScripts)
	{
		if (r_numModelScripts == MAX_MODEL_SCRIPTS)
			return NULL;

		r_numModelScripts++;
	}

	return script;
}

static modelscript_t *MSCR_Find(const char *name)
{
	modelscript_t *script;
	int i;

	for (i = 0, script = r_modelscripts; i < r_numModelScripts; i++, script++)
	{
		if (!script->name[0])
			continue;

		if (!FS_pathcmp(script->name, name))
			return script;
	}

	return NULL;
}

static modelscript_t *MSCR_Register(const char *name)
{
	char path[MAX_QPATH];
	char *rawdata;
	modelscript_t *script;
	int ret = Q_ERR_INVALID_FORMAT;
	Q_snprintf(path, MAX_QPATH, "modelscripts/%s.msc", name);
	uint32_t filelen = FS_LoadFile(path, (void **)&rawdata);

	if (!rawdata)
	{
		// don't spam about missing models
		if (filelen == Q_ERR_NOENT)
			return 0;

		ret = filelen;
		goto fail1;
	}

	script = MSCR_Alloc();
	memset(script, 0, sizeof(*script));
	Q_snprintf(script->name, MAX_QPATH, "%s", name);

	while (true)
	{
		char *tok = COM_Parse((const char **)&rawdata);

		if (!*tok)
			break;

		if (*tok != '{')
		{
			ret = Q_ERR_INVALID_FORMAT;
			goto fail2;
		}

		modelentry_t entry;
		gametype_t game = GAME_NONE;
		memset(&entry, 0, sizeof(entry));
		entry.scale = 1;
		entry.frame = -1;

		while (*(tok = COM_Parse((const char **)&rawdata)) != '}')
		{
			char *value = COM_Parse((const char **)&rawdata);

			if (!Q_stricmp(tok, "game"))
			{
				if (!Q_stricmp(value, "q2"))
					game = GAME_Q2;
				else if (!Q_stricmp(value, "q1"))
					game = GAME_Q1;
				else if (!Q_stricmp(value, "doom"))
					game = GAME_DOOM;
				else if (!Q_stricmp(value, "duke"))
					game = GAME_DUKE;
			}
			else if (!Q_stricmp(tok, "path"))
				Q_snprintf(entry.path, MAX_QPATH, "%s", value);
			else if (!Q_stricmp(tok, "skin"))
				entry.skin = atoi(value) + 1;
			else if (!Q_stricmp(tok, "effects"))
				entry.add_effects = (uint32_t)atoi(value);
			else if (!Q_stricmp(tok, "renderfx"))
				entry.add_renderfx = (int)atoi(value);
			else if (!Q_stricmp(tok, "frame"))
				entry.frame = (int)atoi(value);
			else if (!Q_stricmp(tok, "translate"))
				sscanf(value, "%f %f %f", &entry.translate[0], &entry.translate[1], &entry.translate[2]);
			else if (!Q_stricmp(tok, "scale"))
				entry.scale = atof(value);
		}

		if (!entry.path[0])
			goto fail2;

		entry.model = R_RegisterModel(entry.path);
		entry.loaded = true;

		if (game == GAME_NONE)
		{
			for (game = GAME_Q2; game < GAME_TOTAL; ++game)
				memcpy(&script->entries[game], &entry, sizeof(entry));
		}
		else
			memcpy(&script->entries[game], &entry, sizeof(entry));
	}

	FS_FreeFile(rawdata);
	return script;
fail2:
	FS_FreeFile(rawdata);
fail1:
	Com_EPrintf("Couldn't load %s: %s\n", path, Q_ErrorString(ret));
	return 0;
}

modelentry_t *MSCR_EntryForHandle(qhandle_t handle, gametype_t game)
{
	modelscript_t *script = &r_modelscripts[MODEL_HANDLE_ID(handle) - 1];

	if (!script->name[0])
		return NULL;

	if (game == GAME_NONE)
		game = GAME_Q2;

	return &script->entries[game];
}

static model_t *MOD_Alloc(void)
{
	model_t *model;
	int i;

	for (i = 0, model = r_models; i < r_numModels; i++, model++)
	{
		if (!model->type)
			break;
	}

	if (i == r_numModels)
	{
		if (r_numModels == MAX_RMODELS)
			return NULL;

		r_numModels++;
	}

	return model;
}

static model_t *MOD_Find(const char *name)
{
	model_t *model;
	int i;

	for (i = 0, model = r_models; i < r_numModels; i++, model++)
	{
		if (!model->type)
			continue;

		if (!FS_pathcmp(model->name, name))
			return model;
	}

	return NULL;
}

static void MOD_List_f(void)
{
	static const char types[4] = "FASE";
	int     i, count;
	model_t *model;
	size_t  bytes;
	Com_Printf("------------------\n");
	bytes = count = 0;

	for (i = 0, model = r_models; i < r_numModels; i++, model++)
	{
		if (!model->type)
			continue;

		Com_Printf("%c alloc %8"PRIz" used %8"PRIz" : %s\n", types[model->type],
			model->memory.allocated, model->memory.used, model->name);
		bytes += model->memory.allocated;
		count++;
	}

	Com_Printf("Total models: %d (out of %d slots)\n", count, r_numModels);
	Com_Printf("Total resident: %"PRIz"\n", bytes);
}

void MOD_FreeUnused(void)
{
	model_t *model;
	int i;

	for (i = 0, model = r_models; i < r_numModels; i++, model++)
	{
		if (!model->type)
			continue;

		if (model->registration_sequence != registration_sequence)
		{
			// don't need this model
			Z_ChunkFree(&model->memory);
			memset(model, 0, sizeof(*model));
		}
	}

	MSCR_Untouch();
	MSCR_Touch();
}

void MOD_FreeAll(void)
{
	model_t *model;
	int i;

	for (i = 0, model = r_models; i < r_numModels; i++, model++)
	{
		if (!model->type)
			continue;

		Z_ChunkFree(&model->memory);
		memset(model, 0, sizeof(*model));
	}

	r_numModels = 0;
	modelscript_t *script;

	for (i = 0, script = r_modelscripts; i < r_numModelScripts; ++i, script++)
	{
		if (!script->name[0])
			continue;

		memset(script, 0, sizeof(*script));
	}

	r_numModelScripts = 0;
}

void MOD_AddIDToName(const char *filename, int id, char *buffer, size_t buffer_len)
{
	const char *ext = strchr(filename, '.');

	if (!ext)
		Com_Error(ERR_DROP, "Bad file extension");

	size_t len = ext - filename;
	size_t ext_len = strlen(ext);
	static char num_buffer[6];
	Q_snprintf(num_buffer, sizeof(num_buffer), "_%i", id);

	if (strlen(num_buffer) + ext_len + len >= buffer_len)
		Com_Error(ERR_DROP, "Buffer overflow");

	size_t i = 0;

	for (size_t z = 0; z < len; ++i, ++z)
		buffer[i] = filename[i];

	for (size_t z = 0; z < strlen(num_buffer); ++i, ++z)
		buffer[i] = num_buffer[z];

	//for (size_t z = 0; z < ext_len; ++i, ++z)
	//	buffer[i] = ext[z];
	buffer[i++] = '.';
	buffer[i++] = 'p';
	buffer[i++] = 'c';
	buffer[i++] = 'x';
	buffer[i++] = 0;
}

static int MOD_LoadSP2(model_t *model, const void *rawdata, size_t length)
{
	dsp2header_t header;
	dsp2frame_t *src_frame;
	mspriteframe_t *dst_frame;
	char buffer[SP2_MAX_FRAMENAME];
	int i;

	if (length < sizeof(header))
		return Q_ERR_FILE_TOO_SMALL;

	// byte swap the header
	header = *(dsp2header_t *)rawdata;

	for (i = 0; i < sizeof(header) / 4; i++)
		((uint32_t *)&header)[i] = LittleLong(((uint32_t *)&header)[i]);

	if (header.ident != SP2_IDENT)
		return Q_ERR_UNKNOWN_FORMAT;

	if (header.version != SP2_VERSION)
		return Q_ERR_UNKNOWN_FORMAT;

	if (header.numframes < 1)
	{
		// empty models draw nothing
		model->type = MOD_EMPTY;
		return Q_ERR_SUCCESS;
	}

	if (header.numframes > SP2_MAX_FRAMES)
		return Q_ERR_TOO_MANY;

	if (sizeof(dsp2header_t) + sizeof(dsp2frame_t) * header.numframes > length)
		return Q_ERR_BAD_EXTENT;

	Z_TagChunkCreate(TAG_MODEL, &model->memory, sizeof(mspriteframe_t) * header.numframes);
	model->type = MOD_SPRITE;
	model->spriteframes = Z_ChunkAlloc(&model->memory, model->memory.allocated);
	model->numframes = header.numframes;
	src_frame = (dsp2frame_t *)((byte *)rawdata + sizeof(dsp2header_t));
	dst_frame = model->spriteframes;

	for (i = 0; i < header.numframes; i++)
	{
		dst_frame->width = (int32_t)LittleLong(src_frame->width);
		dst_frame->height = (int32_t)LittleLong(src_frame->height);
		dst_frame->origin_x = (int32_t)LittleLong(src_frame->origin_x);
		dst_frame->origin_y = (int32_t)LittleLong(src_frame->origin_y);

		if (!Q_memccpy(buffer, src_frame->name, 0, sizeof(buffer)))
		{
			Com_WPrintf("%s has bad frame name\n", model->name);
			dst_frame->image = R_NOTEXTURE;
		}
		else
		{
			FS_NormalizePath(buffer, buffer);
			dst_frame->image = IMG_Find(buffer, IT_SPRITE, IF_NONE);
		}

		src_frame++;
		dst_frame++;
	}

	// calculate clip bounds
	vec3_t mins, maxs;
	ClearBounds(mins, maxs);

	for (i = 0; i < model->numframes; ++i)
	{
		// 0,0 is top-left
		int x1 = -model->spriteframes[i].origin_x;
		int y1 = -model->spriteframes[i].origin_x;
		int x2 = -model->spriteframes[i].origin_x + model->spriteframes[i].width;
		int y2 = -model->spriteframes[i].origin_x + model->spriteframes[i].height;
		int highest = max(abs(y2), max(abs(x2), max(abs(x1), abs(y1))));
		vec3_t low = { -highest, -highest, -highest };
		vec3_t high = { highest, highest, highest };
		AddPointToBounds(low, mins, maxs);
		AddPointToBounds(high, mins, maxs);
	}

	model->radius = RadiusFromBounds(mins, maxs);
	return Q_ERR_SUCCESS;
}

// Paril
static int MOD_LoadSPR(model_t *model, const void *rawdata, size_t length)
{
	dsprheader_t header;
	dsprframe_t *src_frame;
	mspriteframe_t *dst_frame;
	unsigned w, h, x, y;
	char buffer[SP2_MAX_FRAMENAME];
	dsprpicture_t *pic;
	image_t *image;
	byte *temppic;
	int ret = Q_ERR_INVALID_FORMAT;

	if (length < sizeof(header))
		return Q_ERR_FILE_TOO_SMALL;

	// byte swap the header
	header = *(dsprheader_t *)rawdata;

	for (int i = 0; i < sizeof(header) / 4; i++)
		((uint32_t *)&header)[i] = LittleLong(((uint32_t *)&header)[i]);

	if (header.ident != SPR_IDENT)
		return Q_ERR_UNKNOWN_FORMAT;

	if (header.version != SPR_VERSION)
		return Q_ERR_UNKNOWN_FORMAT;

	if (header.numframes < 1)
	{
		// empty models draw nothing
		model->type = MOD_EMPTY;
		return Q_ERR_SUCCESS;
	}

	Z_TagChunkCreate(TAG_MODEL, &model->memory, sizeof(mspriteframe_t) * header.numframes);
	model->type = MOD_SPRITE;
	model->spriteframes = Z_ChunkAlloc(&model->memory, model->memory.allocated);
	model->numframes = header.numframes;
	byte *frame_ptr = (byte *)rawdata + sizeof(dsprheader_t);
	dst_frame = model->spriteframes;

	for (int i = 0; i < header.numframes; i++)
	{
		src_frame = (dsprframe_t *)frame_ptr;

		if (src_frame->group)
		{
			ret = Q_ERR_INVALID_FORMAT;
			goto fail;
		}

		frame_ptr += sizeof(dsprframe_t);
		pic = (dsprpicture_t *)frame_ptr;
		w = LittleLong(pic->width);
		h = LittleLong(pic->height);

		if (w < 1 || h < 1 || w > MAX_TEXTURE_SIZE || h > MAX_TEXTURE_SIZE)
		{
			Com_WPrintf("%s has bad frame dimensions\n", model->name);
			w = 1;
			h = 1;
		}

		dst_frame->width = w;
		dst_frame->height = h;
		// FIXME: are these signed?
		x = LittleLong(pic->offset_x);
		y = LittleLong(pic->offset_y);
		dst_frame->origin_x = -(int)x;
		dst_frame->origin_y = y;
		MOD_AddIDToName(model->name, i, buffer, MAX_QPATH);
		image = IMG_Find(buffer, IT_SPRITE, IF_DELAYED | IF_OLDSCHOOL);
		frame_ptr += sizeof(dsprpicture_t);
		temppic = (byte *)IMG_AllocPixels(dst_frame->width * dst_frame->height * 4);
		image->upload_width = image->width = dst_frame->width;
		image->upload_height = image->height = dst_frame->height;
		image->flags |= IMG_Unpack8((uint32_t *)temppic, frame_ptr, dst_frame->width, dst_frame->height, d_palettes[GAME_Q1]);
		IMG_Load(image, temppic);
		dst_frame->image = image;
		IMG_FreePixels(temppic);
		frame_ptr += dst_frame->width * dst_frame->height;
		dst_frame++;
	}

	// calculate clip bounds
	vec3_t mins, maxs;
	ClearBounds(mins, maxs);

	for (int i = 0; i < model->numframes; ++i)
	{
		// 0,0 is top-left
		int x1 = -model->spriteframes[i].origin_x;
		int y1 = -model->spriteframes[i].origin_x;
		int x2 = -model->spriteframes[i].origin_x + model->spriteframes[i].width;
		int y2 = -model->spriteframes[i].origin_x + model->spriteframes[i].height;
		int highest = max(abs(y2), max(abs(x2), max(abs(x1), abs(y1))));
		vec3_t low = { -highest, -highest, -highest };
		vec3_t high = { highest, highest, highest };
		AddPointToBounds(low, mins, maxs);
		AddPointToBounds(high, mins, maxs);
	}

	model->radius = RadiusFromBounds(mins, maxs);
	return Q_ERR_SUCCESS;
fail:
	Z_ChunkFree(&model->memory);
	return ret;
}

#define D2S_IDENT       (('P'<<24)+('S'<<16)+('2'<<8)+'D')
#define DNS_IDENT       (('P'<<24)+('S'<<16)+('N'<<8)+'D')

typedef struct
{
	short x, y;
} d2s_temp_t;

// Paril
static int MOD_LoadD2S(model_t *model, const void *rawdata, size_t length)
{
	byte *ptr = (byte *)rawdata + 4;
	byte num_pics = *ptr++;
	char buffer[MAX_QPATH];
	image_t		**images = (image_t **)Z_Malloc(sizeof(image_t *) * num_pics);
	d2s_temp_t	*offsets = (d2s_temp_t *)Z_Malloc(sizeof(d2s_temp_t) * num_pics);

	for (int i = 0; i < num_pics; ++i)
	{
		// read x/y offsets
		offsets[i] = *(d2s_temp_t *)ptr;
		ptr += sizeof(d2s_temp_t);
		Q_strlcpy(buffer, model->name, strlen(model->name) - 3);
		Q_snprintf(buffer, sizeof(buffer), "%s_%i.tga", buffer, i);
		images[i] = IMG_Find(buffer, IT_SPRITE, IF_OLDSCHOOL);
	}

	byte num_frames = *ptr++;
	byte num_dirs = *ptr++;
	Z_TagChunkCreate(TAG_MODEL, &model->memory, (sizeof(mspritedirframes_t) + (sizeof(mspritedirframe_t) * num_dirs)) * num_frames);
	mspritedirframes_t *frames = model->spritedirframes = Z_ChunkAlloc(&model->memory, sizeof(mspritedirframes_t) * num_frames);

	for (int i = 0; i < num_frames; ++i)
	{
		mspritedirframes_t *frame = &frames[i];
		mspritedirframe_t *dirs = Z_ChunkAlloc(&model->memory, sizeof(mspritedirframe_t) * num_dirs);
		frame->directions = dirs;

		for (int d = 0; d < num_dirs; ++d)
		{
			byte pic_index = *ptr++;
			byte invert = *ptr++;
			image_t *image = images[pic_index];
			d2s_temp_t *d2s = &offsets[pic_index];
			mspritedirframe_t *dir = &dirs[d];
			dir->image = image;
			dir->width = image->width;
			dir->height = image->height;
			dir->origin_x = d2s->x;
			dir->origin_y = d2s->y;
			dir->invert_x = !!invert;
		}
	}

	model->numdirs = num_dirs;
	model->type = MOD_DIR_SPRITE;
	model->numframes = num_frames;
	// calculate clip bounds
	vec3_t mins, maxs;
	ClearBounds(mins, maxs);

	for (int i = 0; i < num_pics; ++i)
	{
		// 0,0 is top-left
		int x1 = -offsets[i].x;
		int y1 = -offsets[i].y;
		int x2 = -offsets[i].x + images[i]->width;
		int y2 = -offsets[i].y + images[i]->height;
		int highest = max(abs(y2), max(abs(x2), max(abs(x1), abs(y1))));
		vec3_t low = { -highest, -highest, -highest };
		vec3_t high = { highest, highest, highest };
		AddPointToBounds(low, mins, maxs);
		AddPointToBounds(high, mins, maxs);
	}

	model->radius = RadiusFromBounds(mins, maxs);
	Z_Free(offsets);
	Z_Free(images);
	return Q_ERR_SUCCESS;
}

void MOD_CalculateNormals(model_t *model)
{
	for (int i = 0; i < model->nummeshes; i++)
	{
		maliasmesh_t *mesh = &model->meshes[i];

		for (int t = 0; t < mesh->numtris; t++)
		{
			GLuint *tri = mesh->indices + (t * 3);

			for (int j = 0; j < model->numframes; j++)
			{
				maliasvert_t *v0 = &mesh->verts[tri[0] + (j * mesh->numverts)];
				maliasvert_t *v1 = &mesh->verts[tri[1] + (j * mesh->numverts)];
				maliasvert_t *v2 = &mesh->verts[tri[2] + (j * mesh->numverts)];

				vec3_t ba, ca;

				VectorSubtract(v1->pos, v0->pos, ba);
				VectorSubtract(v2->pos, v0->pos, ca);

				vec3_t n;

				CrossProduct(ba, ca, n);
				VectorNormalize(n);

				// for all verts that match vert's position, add the normal
				for (int v = 0; v < mesh->numverts; v++)
				{
					maliasvert_t *mv = &mesh->verts[v + (j * mesh->numverts)];

					if (mv == v0 || mv == v1 || mv == v2 ||
						VectorCompare(mv->pos, v0->pos) || VectorCompare(mv->pos, v1->pos) || VectorCompare(mv->pos, v2->pos))
						VectorAdd(mv->norm, n, mv->norm);
				}
			}
		}

		for (int v = 0; v < mesh->numverts * model->numframes; v++)
		{
			maliasvert_t *vert = &mesh->verts[v];
			VectorNormalize(vert->norm);
		}
	}
}

void MOD_Reference(model_t *model)
{
	int i, j;

	// register any images used by the models
	switch (model->type)
	{
		case MOD_ALIAS:
			for (i = 0; i < model->nummeshes; i++)
			{
				maliasmesh_t *mesh = &model->meshes[i];

				for (j = 0; j < mesh->numskins; j++)
					mesh->skins[j]->registration_sequence = registration_sequence;
			}

			break;

		case MOD_SPRITE:
			for (i = 0; i < model->numframes; i++)
				model->spriteframes[i].image->registration_sequence = registration_sequence;

			break;

		case MOD_EMPTY:
			break;

		// Generations
		case MOD_DIR_SPRITE:
			for (i = 0; i < model->numframes; i++)
			{
				for (j = 0; j < model->numdirs; ++j)
					model->spritedirframes[i].directions[j].image->registration_sequence = registration_sequence;
			}

			break;

		case MOD_WEAPONSCRIPT:
			for (i = 0; i < model->weaponscript->num_sprites; i++)
				for (gametype_t g = GAME_Q2; g < GAME_TOTAL; ++g)
					if (model->weaponscript->sprites[i].model)
						MOD_Reference(MOD_ForHandle(model->weaponscript->sprites[i].model, g));

			break;

		default:
			Com_Error(ERR_FATAL, "%s: bad model type", __func__);
	}

	model->registration_sequence = registration_sequence;
}

int MOD_LoadQ1M(model_t *model, const void *rawdata, size_t length);

qhandle_t R_RegisterModel(const char *name)
{
	char normalized[MAX_QPATH];
	union
	{
		qhandle_t handle;
		modelhandle_t model;
	} index;
	size_t namelen;
	int filelen;
	model_t *model;
	byte *rawdata;
	mod_load_t load;
	int ret;

	// empty names are legal, silently ignore them
	if (!*name)
		return 0;

	if (*name == '*')
	{
		// inline bsp model
		index.model.type = MODELHANDLE_BSP;
		index.model.id = atoi(name + 1);
		return index.handle;
	}
	else if (*name == '%')     // model script
	{
		index.model.type = MODELHANDLE_GAMED;
		modelscript_t *script = MSCR_Find(name + 1);

		if (script)
		{
			index.model.id = (script - r_modelscripts) + 1;
			return index.handle;
		}

		// gotta load it
		script = MSCR_Register(name + 1);

		if (script)
		{
			index.model.id = (script - r_modelscripts) + 1;
			return index.handle;
		}

		Com_EPrintf("Couldn't load modelscript %s\n", name + 1);
		return 0;
	}

	// normalize the path
	namelen = FS_NormalizePathBuffer(normalized, name, MAX_QPATH);

	// this should never happen
	if (namelen >= MAX_QPATH)
		Com_Error(ERR_DROP, "%s: oversize name", __func__);

	// normalized to empty name?
	if (namelen == 0)
	{
		Com_DPrintf("%s: empty name\n", __func__);
		return 0;
	}

	// see if it's already loaded
	model = MOD_Find(normalized);

	if (model)
	{
		MOD_Reference(model);
		goto done;
	}

	filelen = FS_LoadFile(normalized, (void **)&rawdata);

	if (!rawdata)
	{
		// don't spam about missing models
		if (filelen == Q_ERR_NOENT)
			return 0;

		ret = filelen;
		goto fail1;
	}

	if (filelen < 4)
	{
		ret = Q_ERR_FILE_TOO_SMALL;
		goto fail2;
	}

	// Generations
	const char *extension = strchr(name, '.') + 1;

	model = MOD_Alloc();

	if (!model)
	{
		ret = Q_ERR_OUT_OF_SLOTS;
		goto fail2;
	}

	if (!Q_stricmp(extension, "q1m"))
		load = MOD_LoadQ1M;
	else if (!Q_stricmp(extension, "mdl"))
	{
		model->nolerp = true;
		load = MOD_LoadMD2;
	}
	else if (!Q_stricmp(extension, "spr"))
		load = MOD_LoadSPR;
	else if (!Q_stricmp(extension, "md2"))
		load = MOD_LoadMD2;
	// Paril
	else if (!Q_stricmp(extension, "sp2"))
		load = MOD_LoadSP2;
	else if (!Q_stricmp(extension, "d2s") || !Q_stricmp(extension, "dns"))
		load = MOD_LoadD2S;
	else if (!Q_stricmp(extension, "wsc"))
		load = MOD_LoadWSC;
	else
	{
		ret = Q_ERR_UNKNOWN_FORMAT;
		goto fail2;
	}

	memcpy(model->name, normalized, namelen + 1);
	model->registration_sequence = registration_sequence;
	ret = load(model, rawdata, filelen);
	FS_FreeFile(rawdata);

	if (ret)
	{
		memset(model, 0, sizeof(*model));
		goto fail1;
	}

done:
	index.model.type = MODELHANDLE_RAW;
	index.model.id = (model - r_models) + 1;
	return index.handle;
fail2:
	FS_FreeFile(rawdata);
fail1:
	Com_EPrintf("Couldn't load %s: %s\n", normalized, Q_ErrorString(ret));
	return 0;
}

model_t *MOD_ForHandle(qhandle_t index, gametype_t game)
{
	model_t *model;

	if (!MODEL_HANDLE_TYPE(index))
		return NULL;

	switch (MODEL_HANDLE_TYPE(index))
	{
		case MODELHANDLE_BSP:
		default:
			Com_Error(ERR_DROP, "%s: should never happen", __func__);

		case MODELHANDLE_RAW:
			if (MODEL_HANDLE_ID(index) < 0 || MODEL_HANDLE_ID(index) > r_numModels)
				Com_Error(ERR_DROP, "%s: %d out of range", __func__, MODEL_HANDLE_ID(index));

			model = &r_models[MODEL_HANDLE_ID(index) - 1];

			if (!model->type)
				return NULL;

			break;

		case MODELHANDLE_GAMED:
			if (MODEL_HANDLE_ID(index) < 0 || MODEL_HANDLE_ID(index) > r_numModelScripts)
				Com_Error(ERR_DROP, "%s: %d out of range", __func__, MODEL_HANDLE_ID(index));

			modelentry_t *entry = MSCR_EntryForHandle(index, game);

			if (!entry->loaded)
				return NULL;

			return MOD_ForHandle(entry->model, game);
	}

	return model;
}

void MOD_Init(void)
{
	if (r_numModels)
		Com_Error(ERR_FATAL, "%s: %d models not freed", __func__, r_numModels);

	if (r_numModelScripts)
		Com_Error(ERR_FATAL, "%s: %d model scripts not freed", __func__, r_numModelScripts);

	Cmd_AddCommand("modellist", MOD_List_f);
}

void MOD_Shutdown(void)
{
	MOD_FreeAll();
	Cmd_RemoveCommand("modellist");
}

void MOD_AddIDToName(const char *filename, int id, char *buffer, size_t buffer_len);

// Paril
// FIXME: I think I'd like to replace this with a more standard format. OBJ maybe?
int MOD_LoadQ1M(model_t *model, const void *rawdata, size_t length)
{
	byte	*ptr = (byte *)rawdata + 4;
	int		texture_frames = *ptr++;
	int		num_verts = *ptr++;
	int		num_faces = *ptr++;
	int		texture_width = *ptr++;
	int		texture_height = *ptr++;
	int		num_indices = num_faces * 6;
	char	skinname[MAX_QPATH];
	Z_TagChunkCreate(TAG_MODEL, &model->memory, sizeof(maliasmesh_t) + sizeof(maliasframe_t) +
		(num_verts * sizeof(maliasvert_t)) +
		(num_verts * sizeof(maliastc_t)) +
		num_indices * sizeof(GL_INDEX_TYPE));
	model->type = MOD_ALIAS;
	model->nummeshes = 1;
	model->numframes = 1;
	model->meshes = Z_ChunkAlloc(&model->memory, sizeof(maliasmesh_t));
	model->frames = Z_ChunkAlloc(&model->memory, model->numframes * sizeof(maliasframe_t));
	maliasmesh_t *dst_mesh = model->meshes;
	byte *pic = (byte *)IMG_AllocPixels(texture_width * texture_height * 4);

	for (int i = 0; i < texture_frames; ++i)
	{
		MOD_AddIDToName(model->name, i, skinname, MAX_QPATH);
		dst_mesh->skins[i] = IMG_Find(skinname, IT_SKIN, IF_DELAYED | IF_OLDSCHOOL);
		dst_mesh->skins[i]->upload_width = dst_mesh->skins[i]->width = texture_width;
		dst_mesh->skins[i]->upload_height = dst_mesh->skins[i]->height = texture_height;
		dst_mesh->skins[i]->flags |= IMG_Unpack8((uint32_t *)pic, ptr, texture_width, texture_height, d_palettes[GAME_Q1]);
		IMG_Load(dst_mesh->skins[i], pic);
		ptr += texture_width * texture_height;
	}

	IMG_FreePixels(pic);
	dst_mesh->numskins = texture_frames;
	dst_mesh->numverts = num_verts;
	dst_mesh->verts = Z_ChunkAlloc(&model->memory, num_verts * sizeof(maliasvert_t));
	dst_mesh->tcoords = Z_ChunkAlloc(&model->memory, num_verts * sizeof(maliastc_t));
	maliasframe_t *frame = model->frames;
	ClearBounds(frame->bounds[0], frame->bounds[1]);
	float inv_w = 1.0f / texture_width;
	float inv_h = 1.0f / texture_height;

	for (int i = 0; i < num_verts; ++i)
	{
		vec3_t tmp;

		for (int x = 0; x < 3; ++x)
			dst_mesh->verts[i].pos[x] = (char) * ptr++;

		tmp[0] = dst_mesh->verts[i].pos[0];
		tmp[1] = dst_mesh->verts[i].pos[1];
		tmp[2] = dst_mesh->verts[i].pos[2];
		AddPointToBounds(tmp, frame->bounds[0], frame->bounds[1]);

		for (int x = 0; x < 2; ++x)
			dst_mesh->tcoords[i].st[x] = *ptr++ * ((x == 0) ? inv_w : inv_h);
	}

	dst_mesh->numtris = num_indices / 3;
	dst_mesh->numindices = num_indices;
	dst_mesh->indices = Z_ChunkAlloc(&model->memory, num_indices * sizeof(GL_INDEX_TYPE));

	for (int i = 0; i < num_indices; ++i)
		dst_mesh->indices[i] = *ptr++;

	frame->radius = RadiusFromBounds(frame->bounds[0], frame->bounds[1]);
	VectorSet(frame->scale, 1, 1, 1);
	VectorSet(frame->translate, 0, 0, 0);

	MOD_CalculateNormals(model);

	return Q_ERR_SUCCESS;
}

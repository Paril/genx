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

#include "format/md2.h"
#if USE_MD3
#include "format/md3.h"
#endif
#include "format/sp2.h"

// Paril
#include "format/mdl.h"
#include "format/spr.h"
#include "format/mdx.h"

typedef int (*mod_load_t) (model_t *, const void *, size_t);

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

	if (i == r_numModelScripts) {
		if (r_numModelScripts == MAX_MODEL_SCRIPTS) {
			return NULL;
		}
		r_numModelScripts++;
	}

	return script;
}

static modelscript_t *MSCR_Find(const char *name)
{
	modelscript_t *script;
	int i;

	for (i = 0, script = r_modelscripts; i < r_numModelScripts; i++, script++) {
		if (!script->name[0]) {
			continue;
		}
		if (!FS_pathcmp(script->name, name)) {
			return script;
		}
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

	if (!rawdata) {
		// don't spam about missing models
		if (filelen == Q_ERR_NOENT) {
			return 0;
		}

		ret = filelen;
		goto fail1;
	}

	script = MSCR_Alloc();
	memset(script, 0, sizeof(*script));

	Q_snprintf(script->name, MAX_QPATH, "%s", name);

	while (true)
	{
		char *tok = COM_Parse((const char**)&rawdata);

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

		while (*(tok = COM_Parse((const char**)&rawdata)) != '}')
		{
			char *value = COM_Parse((const char**)&rawdata);

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

    for (i = 0, model = r_models; i < r_numModels; i++, model++) {
        if (!model->type) {
            break;
        }
    }

    if (i == r_numModels) {
        if (r_numModels == MAX_RMODELS) {
            return NULL;
        }
        r_numModels++;
    }

    return model;
}

static model_t *MOD_Find(const char *name)
{
    model_t *model;
    int i;

    for (i = 0, model = r_models; i < r_numModels; i++, model++) {
        if (!model->type) {
            continue;
        }
        if (!FS_pathcmp(model->name, name)) {
            return model;
        }
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

    for (i = 0, model = r_models; i < r_numModels; i++, model++) {
        if (!model->type) {
            continue;
        }
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

    for (i = 0, model = r_models; i < r_numModels; i++, model++) {
        if (!model->type) {
            continue;
        }
        if (model->registration_sequence != registration_sequence) {
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

    for (i = 0, model = r_models; i < r_numModels; i++, model++) {
        if (!model->type) {
            continue;
        }

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
    for (i = 0; i < sizeof(header) / 4; i++) {
        ((uint32_t *)&header)[i] = LittleLong(((uint32_t *)&header)[i]);
    }

    if (header.ident != SP2_IDENT)
        return Q_ERR_UNKNOWN_FORMAT;
    if (header.version != SP2_VERSION)
        return Q_ERR_UNKNOWN_FORMAT;
    if (header.numframes < 1) {
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
    for (i = 0; i < header.numframes; i++) {
        dst_frame->width = (int32_t)LittleLong(src_frame->width);
        dst_frame->height = (int32_t)LittleLong(src_frame->height);

        dst_frame->origin_x = (int32_t)LittleLong(src_frame->origin_x);
        dst_frame->origin_y = (int32_t)LittleLong(src_frame->origin_y);

        if (!Q_memccpy(buffer, src_frame->name, 0, sizeof(buffer))) {
            Com_WPrintf("%s has bad frame name\n", model->name);
            dst_frame->image = R_NOTEXTURE;
        } else {
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
	for (int i = 0; i < sizeof(header) / 4; i++) {
		((uint32_t *)&header)[i] = LittleLong(((uint32_t *)&header)[i]);
	}

	if (header.ident != SPR_IDENT)
		return Q_ERR_UNKNOWN_FORMAT;
	if (header.version != SPR_VERSION)
		return Q_ERR_UNKNOWN_FORMAT;
	if (header.numframes < 1) {
		// empty models draw nothing
		model->type = MOD_EMPTY;
		return Q_ERR_SUCCESS;
	}

	Z_TagChunkCreate(TAG_MODEL, &model->memory, sizeof(mspriteframe_t) * header.numframes);
	model->type = MOD_SPRITE;

	model->spriteframes = Z_ChunkAlloc(&model->memory, model->memory.allocated);
	model->numframes = header.numframes;

	byte *frame_ptr = (byte*)rawdata + sizeof(dsprheader_t);

	dst_frame = model->spriteframes;
	for (int i = 0; i < header.numframes; i++)
	{
		src_frame = (dsprframe_t *)frame_ptr;
		
		if (src_frame->group) {
			ret = Q_ERR_INVALID_FORMAT;
			goto fail;
		}

		frame_ptr += sizeof(dsprframe_t);

		pic = (dsprpicture_t*)frame_ptr;

		w = LittleLong(pic->width);
		h = LittleLong(pic->height);

		if (w < 1 || h < 1 || w > MAX_TEXTURE_SIZE || h > MAX_TEXTURE_SIZE) {
			Com_WPrintf("%s has bad frame dimensions\n", model->name);
			w = 1;
			h = 1;
		}

		dst_frame->width = w;
		dst_frame->height = h;

		// FIXME: are these signed?
		x = LittleLong(pic->offset_x);
		y = LittleLong(pic->offset_y);

		/*if (x > 8192 || y > 8192) {
			Com_WPrintf("%s has bad frame origin\n", model->name);
			x = y = 0;
		}*/

		dst_frame->origin_x = -(int)x;
		dst_frame->origin_y = y;

		MOD_AddIDToName(model->name, i, buffer, MAX_QPATH);
		image = IMG_Find(buffer, IT_SPRITE, IF_DELAYED | IF_OLDSCHOOL);

		frame_ptr += sizeof(dsprpicture_t);

		temppic = (byte*)IMG_AllocPixels(dst_frame->width * dst_frame->height * 4);

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

typedef struct {
	short x, y;
} d2s_temp_t;

// Paril
static int MOD_LoadD2S(model_t *model, const void *rawdata, size_t length)
{
	byte *ptr = (byte*)rawdata + 4;

	byte num_pics = *ptr++;
	char buffer[MAX_QPATH];

	image_t		**images = (image_t**)Z_Malloc(sizeof(image_t*) * num_pics);
	d2s_temp_t	*offsets = (d2s_temp_t*)Z_Malloc(sizeof(d2s_temp_t) * num_pics);

	for (int i = 0; i < num_pics; ++i)
	{
		// read x/y offsets
		offsets[i] = *(d2s_temp_t*)ptr;
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
    if (header->num_skins) {
        if (header->num_skins > MAX_ALIAS_SKINS)
            return Q_ERR_TOO_MANY;

        end = header->ofs_skins + (size_t)MD2_MAX_SKINNAME * header->num_skins;
        if (header->ofs_skins < sizeof(header) || end < header->ofs_skins || end > length)
            return Q_ERR_BAD_EXTENT;
    }

    if (header->skinwidth < 1/* || header->skinwidth > MD2_MAX_SKINWIDTH*/)
        return Q_ERR_INVALID_FORMAT;
    if (header->skinheight < 1/* || header->skinheight > MD2_MAX_SKINHEIGHT*/)
        return Q_ERR_INVALID_FORMAT;

    return Q_ERR_SUCCESS;
}

static int MOD_LoadMD2(model_t *model, const void *rawdata, size_t length)
{
    dmd2header_t    header;
    dmd2frame_t     *src_frame;
    dmd2trivertx_t  *src_vert;
    dmd2triangle_t  *src_tri;
    dmd2stvert_t    *src_tc;
    char            *src_skin;
    maliasframe_t   *dst_frame;
    maliasvert_t    *dst_vert;
    maliasmesh_t    *dst_mesh;
    maliastc_t      *dst_tc;
    int             i, j, k, val, ret;
    uint16_t        remap[TESS_MAX_INDICES];
    uint16_t        vertIndices[TESS_MAX_INDICES];
    uint16_t        tcIndices[TESS_MAX_INDICES];
    uint16_t        finalIndices[TESS_MAX_INDICES];
    int             numverts, numindices;
    char            skinname[MAX_QPATH];
    vec_t           scale_s, scale_t;
    vec3_t          mins, maxs;

    if (length < sizeof(header)) {
        return Q_ERR_FILE_TOO_SMALL;
    }

    // byte swap the header
    header = *(dmd2header_t *)rawdata;
    for (i = 0; i < sizeof(header) / 4; i++) {
        ((uint32_t *)&header)[i] = LittleLong(((uint32_t *)&header)[i]);
    }

    // validate the header
    ret = MOD_ValidateMD2(&header, length);
    if (ret) {
        if (ret == Q_ERR_TOO_FEW) {
            // empty models draw nothing
            model->type = MOD_EMPTY;
            return Q_ERR_SUCCESS;
        }
        return ret;
    }

    // load all triangle indices
    numindices = 0;
    src_tri = (dmd2triangle_t *)((byte *)rawdata + header.ofs_tris);
    for (i = 0; i < header.num_tris; i++) {
        for (j = 0; j < 3; j++) {
            uint16_t idx_xyz = LittleShort(src_tri->index_xyz[j]);
            uint16_t idx_st = LittleShort(src_tri->index_st[j]);

            // some broken models have 0xFFFF indices
            if (idx_xyz >= header.num_xyz || idx_st >= header.num_st) {
                break;
            }

            vertIndices[numindices + j] = idx_xyz;
            tcIndices[numindices + j] = idx_st;
        }
        if (j == 3) {
            // only count good triangles
            numindices += 3;
        }
        src_tri++;
    }

    if (numindices < 3) {
        return Q_ERR_TOO_FEW;
    }

    for (i = 0; i < numindices; i++) {
        remap[i] = 0xFFFF;
    }

    // remap all triangle indices
    numverts = 0;
    src_tc = (dmd2stvert_t *)((byte *)rawdata + header.ofs_st);
    for (i = 0; i < numindices; i++) {
        if (remap[i] != 0xFFFF) {
            continue; // already remapped
        }

        for (j = i + 1; j < numindices; j++) {
            if (vertIndices[i] == vertIndices[j] &&
                (src_tc[tcIndices[i]].s == src_tc[tcIndices[j]].s &&
                 src_tc[tcIndices[i]].t == src_tc[tcIndices[j]].t)) {
                // duplicate vertex
                remap[j] = i;
                finalIndices[j] = numverts;
            }
        }

        // new vertex
        remap[i] = i;
        finalIndices[i] = numverts++;
    }

    if (numverts > TESS_MAX_VERTICES) {
        return Q_ERR_TOO_MANY;
    }

	Z_TagChunkCreate(TAG_MODEL, &model->memory, sizeof(maliasmesh_t) +
					 (header.num_frames * sizeof(maliasframe_t)) +
					 (numverts * header.num_frames * sizeof(maliasvert_t)) +
					 (numverts * sizeof(maliastc_t)) +
					 (numindices * sizeof(QGL_INDEX_TYPE)));
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
    dst_mesh->indices = Z_ChunkAlloc(&model->memory, numindices * sizeof(QGL_INDEX_TYPE));

    if (dst_mesh->numtris != header.num_tris) {
        Com_DPrintf("%s has %d bad triangles\n", model->name, header.num_tris - dst_mesh->numtris);
    }

    // store final triangle indices
    for (i = 0; i < numindices; i++) {
        dst_mesh->indices[i] = finalIndices[i];
    }

    // load all skins
    src_skin = (char *)rawdata + header.ofs_skins;
    for (i = 0; i < header.num_skins; i++) {
        if (!Q_memccpy(skinname, src_skin, 0, sizeof(skinname))) {
            ret = Q_ERR_STRING_TRUNCATED;
            goto fail;
        }
        FS_NormalizePath(skinname, skinname);
        dst_mesh->skins[i] = IMG_Find(skinname, IT_SKIN, IF_NONE);
        src_skin += MD2_MAX_SKINNAME;
    }

    // load all tcoords
    src_tc = (dmd2stvert_t *)((byte *)rawdata + header.ofs_st);
    dst_tc = dst_mesh->tcoords;
    scale_s = 1.0f / header.skinwidth;
    scale_t = 1.0f / header.skinheight;
    for (i = 0; i < numindices; i++) {
        if (remap[i] != i) {
            continue;
        }
        dst_tc[finalIndices[i]].st[0] =
            (int16_t)LittleShort(src_tc[tcIndices[i]].s) * scale_s;
        dst_tc[finalIndices[i]].st[1] =
            (int16_t)LittleShort(src_tc[tcIndices[i]].t) * scale_t;
    }

    // load all frames
    src_frame = (dmd2frame_t *)((byte *)rawdata + header.ofs_frames);
    dst_frame = model->frames;
    for (j = 0; j < header.num_frames; j++) {
        LittleVector(src_frame->scale, dst_frame->scale);
        LittleVector(src_frame->translate, dst_frame->translate);

        // load frame vertices
        ClearBounds(mins, maxs);
        for (i = 0; i < numindices; i++) {
            if (remap[i] != i) {
                continue;
            }
            src_vert = &src_frame->verts[vertIndices[i]];
            dst_vert = &dst_mesh->verts[j * numverts + finalIndices[i]];

            dst_vert->pos[0] = src_vert->v[0];
            dst_vert->pos[1] = src_vert->v[1];
            dst_vert->pos[2] = src_vert->v[2];

            val = src_vert->lightnormalindex;
            if (val >= NUMVERTEXNORMALS) {
                dst_vert->norm[0] = 0;
                dst_vert->norm[1] = 0;
            } else {
                dst_vert->norm[0] = gl_static.latlngtab[val][0];
                dst_vert->norm[1] = gl_static.latlngtab[val][1];
            }

            for (k = 0; k < 3; k++) {
                val = dst_vert->pos[k];
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

    return Q_ERR_SUCCESS;

fail:
    Z_ChunkFree(&model->memory);
    return ret;
}

#if MDX
static int MOD_ValidateMDX(dmdxheader_t *header, size_t length)
{
    size_t end;

    // check ident and version
    if (header->ident != MDX_IDENT)
        return Q_ERR_UNKNOWN_FORMAT;
    if (header->version != MDX_VERSION)
        return Q_ERR_UNKNOWN_FORMAT;

    // check triangles
    /*if (header->num_tris < 1)
        return Q_ERR_TOO_FEW;
    if (header->num_tris > MDX_MAX_TRIANGLES)
        return Q_ERR_TOO_MANY;

    end = header->ofs_tris + sizeof(dmdxtriangle_t) * header->num_tris;
    if (header->ofs_tris < sizeof(header) || end < header->ofs_tris || end > length)
        return Q_ERR_BAD_EXTENT;

    // check st
    if (header->num_st < 3)
        return Q_ERR_TOO_FEW;
    if (header->num_st > MAX_ALIAS_VERTS)
        return Q_ERR_TOO_MANY;

    end = header->ofs_st + sizeof(dmd2stvert_t) * header->num_st;
    if (header->ofs_st < sizeof(header) || end < header->ofs_st || end > length)
        return Q_ERR_BAD_EXTENT;*/

    // check xyz and frames
    if (header->num_xyz < 3)
        return Q_ERR_TOO_FEW;
    if (header->num_xyz > MAX_ALIAS_VERTS)
        return Q_ERR_TOO_MANY;
    if (header->num_frames < 1)
        return Q_ERR_TOO_FEW;
    if (header->num_frames > MDX_MAX_FRAMES)
        return Q_ERR_TOO_MANY;

    end = sizeof(dmdxframe_t) + (header->num_xyz - 1) * sizeof(dmdxtrivertx_t);
    if (header->framesize < end || header->framesize > MDX_MAX_FRAMESIZE)
        return Q_ERR_BAD_EXTENT;

    end = header->ofs_frames + (size_t)header->framesize * header->num_frames;
    if (header->ofs_frames < sizeof(header) || end < header->ofs_frames || end > length)
        return Q_ERR_BAD_EXTENT;

    // check skins
    if (header->num_skins) {
        if (header->num_skins > MAX_ALIAS_SKINS)
            return Q_ERR_TOO_MANY;

        end = header->ofs_skins + (size_t)MDX_MAX_SKINNAME * header->num_skins;
        if (header->ofs_skins < sizeof(header) || end < header->ofs_skins || end > length)
            return Q_ERR_BAD_EXTENT;
    }

    if (header->skinwidth < 1)
        return Q_ERR_INVALID_FORMAT;
    if (header->skinheight < 1)
        return Q_ERR_INVALID_FORMAT;

    return Q_ERR_SUCCESS;
}

static int MOD_LoadMDX(model_t *model, const void *rawdata, size_t length)
{
    dmdxheader_t    header;
    dmdxframe_t     *src_frame;
    maliasframe_t   *dst_frame;
    int             i, j, k, ret;

    if (length < sizeof(header)) {
        return Q_ERR_FILE_TOO_SMALL;
    }

    // byte swap the header
    header = *(dmdxheader_t *)rawdata;
    for (i = 0; i < sizeof(header) / 4; i++) {
        ((uint32_t *)&header)[i] = LittleLong(((uint32_t *)&header)[i]);
    }

    // validate the header
    ret = MOD_ValidateMDX(&header, length);
    if (ret) {
        if (ret == Q_ERR_TOO_FEW) {
            // empty models draw nothing
            model->type = MOD_EMPTY;
            return Q_ERR_SUCCESS;
        }
        return ret;
    }

	// load all meshes
	dmdxglcmds_t *src_cmds = (dmdxglcmds_t *) ((byte *)rawdata + header.ofs_glcmds);

	// count all of the triangles in the list
	uint16_t num_glcmds[MDX_MAX_SUBOBJECTS] = { 0 };

	for (j = 0; j < header.num_glcmds; j++) {
		if (src_cmds->num_verts == 0) {
			break;
		}

		int numVertices = abs(src_cmds->num_verts);
		num_glcmds[src_cmds->sub_object] += numVertices;

		src_cmds = (dmdxglcmds_t *) ((byte *)(src_cmds + 1) + sizeof(dmdxglcmd_t) * numVertices);
	}

	// count all the vertices in the list
	uint16_t num_vertices[MDX_MAX_SUBOBJECTS] = { 0 };

	uint32_t *src_vertex = (uint32_t  *)((byte *)rawdata + header.ofs_vertex_info);
	for (j = 0; j < header.num_xyz; j++) {
		num_vertices[(*src_vertex) - 1]++;
		src_vertex++;
	}

	// allocate memory
    Hunk_Begin(&model->hunk, 0x600000);
    model->type = MOD_ALIAS;
    model->nummeshes = header.num_sub_objects;
    model->numframes = header.num_frames;
    model->meshes = MOD_Malloc(sizeof(maliasmesh_t) * header.num_sub_objects);
    model->frames = MOD_Malloc(header.num_frames * sizeof(maliasframe_t));

    // load initial frame data
    dst_frame = model->frames;
    for (j = 0; j < header.num_frames; j++, dst_frame++) {
		src_frame = (dmd2frame_t *)((byte *)rawdata + header.ofs_frames + j * header.framesize);
        LittleVector(src_frame->scale, dst_frame->scale);
        LittleVector(src_frame->translate, dst_frame->translate);
		ClearBounds(dst_frame->bounds[0], dst_frame->bounds[1]);
	}

	// make meshes
	for (i = 0; i < header.num_sub_objects; i++) {
		maliasmesh_t *out_mesh = &model->meshes[i];

		// fetch texcoord/vertex mappings
		dmdxglcmd_t cmds[MDX_MAX_VERTS];
		int32_t fanstrips[MDX_MAX_VERTS];
		int cmdCount = 0, stripCount = 0;

		dmdxglcmds_t *src_cmds = (dmdxglcmds_t *) ((byte *)rawdata + header.ofs_glcmds);

		for (j = 0; j < header.num_glcmds; j++) {
			if (src_cmds->num_verts == 0) {
				break;
			}

			size_t skip = sizeof(dmdxglcmd_t) * abs(src_cmds->num_verts);

			if (src_cmds->sub_object == i) {
				memcpy(cmds + cmdCount, src_cmds + 1, skip);
				cmdCount += abs(src_cmds->num_verts);
				fanstrips[stripCount++] = src_cmds->num_verts;
			}

			src_cmds = (dmdxglcmds_t *) ((byte *)(src_cmds + 1) + sizeof(dmdxglcmd_t) * abs(src_cmds->num_verts));
		}

		// map vertices and texcoords
		out_mesh->numskins = header.num_skins;
		out_mesh->numverts = cmdCount;
		out_mesh->verts = MOD_Malloc(cmdCount * header.num_frames * sizeof(maliasvert_t));
		out_mesh->tcoords = MOD_Malloc(cmdCount * sizeof(maliastc_t));

		for (j = 0; j < cmdCount; j++) {
			for (k = 0; k < 2; k++) {
				out_mesh->tcoords[j].st[k] = LittleFloat(cmds[j].st.st[k]);
			}
		}

		int l = 0, m = 0;
		maliasvert_t *dst_vert = out_mesh->verts;

		// load vertices
		for (j = 0; j < header.num_frames; j++) {
			dmdxframe_t *frame = (dmdxframe_t *) ((byte *)rawdata + header.ofs_frames + header.framesize * j);
			maliasframe_t *out_frame = &model->frames[j];

			for (k = 0; k < cmdCount; k++, l++, dst_vert++) {
				dmdxtrivertx_t *src_vert = &frame->verts[cmds[k].vertexIndex];

				byte val = src_vert->lightnormalindex;
				if (val >= NUMVERTEXNORMALS) {
					dst_vert->norm[0] = 0;
					dst_vert->norm[1] = 0;
				} else {
					dst_vert->norm[0] = gl_static.latlngtab[val][0];
					dst_vert->norm[1] = gl_static.latlngtab[val][1];
				}

				dst_vert->pos[0] = src_vert->v[0];
				dst_vert->pos[1] = src_vert->v[1];
				dst_vert->pos[2] = src_vert->v[2];

				for (m = 0; m < 3; m++) {
					val = dst_vert->pos[m];
					if (val < out_frame->bounds[0][m])
						out_frame->bounds[0][m] = val;
					if (val > out_frame->bounds[1][m])
						out_frame->bounds[1][m] = val;
				}
			}
		}

		// convert cmds from fans/strips to triangles
		int num_tris = 0;
		
		for (k = 0; k < stripCount; k++) {
			num_tris += abs(fanstrips[k]) - 2;
		}

		out_mesh->numtris = num_tris;
		out_mesh->numindices = num_tris * 3;
		out_mesh->indices = MOD_Malloc(num_tris * 3 * sizeof(QGL_INDEX_TYPE));
		
		int curIndex = 0, fan_start = 0;
		QGL_INDEX_TYPE *index = out_mesh->indices;

		for (k = 0; k < stripCount; k++) {
			for (j = 0; j < abs(fanstrips[k]); j++) {
				if (j < 3) {
					if (j == 0) {
						fan_start = curIndex;
					}

					*index++ = curIndex++;
					continue;
				}

				// 012345

				if (fanstrips[k] < 0) {
					// 012 023 034 045
					// fan
					*index++ = fan_start;
					*index++ = curIndex - 1;
					*index++ = curIndex++;
				} else {
					// 012 123 234 345
					// strip
					if (j & 1) {
						*index++ = curIndex;
						*index++ = curIndex - 1;
						*index++ = curIndex - 2;
					} else {
						*index++ = curIndex - 2;
						*index++ = curIndex - 1;
						*index++ = curIndex;
					}
					curIndex++;
				}
			}
		}
	}

	for (j = 0; j < header.num_frames; j++) {
		maliasframe_t *dst_frame = &model->frames[j];

        VectorVectorScale(dst_frame->bounds[0], dst_frame->scale, dst_frame->bounds[0]);
        VectorVectorScale(dst_frame->bounds[1], dst_frame->scale, dst_frame->bounds[1]);

        dst_frame->radius = RadiusFromBounds(dst_frame->bounds[0], dst_frame->bounds[1]);

        VectorAdd(dst_frame->bounds[0], dst_frame->translate, dst_frame->bounds[0]);
        VectorAdd(dst_frame->bounds[1], dst_frame->translate, dst_frame->bounds[1]);
	}

    // load all skins
    char skinname[MAX_QPATH];
    char *src_skin = (char *)rawdata + header.ofs_skins;
    for (i = 0; i < header.num_skins; i++) {
        if (!Q_memccpy(skinname, src_skin, 0, sizeof(skinname))) {
            ret = Q_ERR_STRING_TRUNCATED;
            goto fail;
        }
        FS_NormalizePath(skinname, skinname);
        image_t *img = IMG_Find(skinname, IT_SKIN, IF_NONE);
		for (j = 0; j < header.num_sub_objects; j++) {
			model->meshes[j].skins[i] = img;
		}
        src_skin += MD2_MAX_SKINNAME;
    }

    Hunk_End(&model->hunk);
    return Q_ERR_SUCCESS;

fail:
    Hunk_Free(&model->hunk);
    return ret;
}
#endif

#if USE_MD3
static int MOD_LoadMD3Mesh(model_t *model, maliasmesh_t *mesh,
                           const byte *rawdata, size_t length, size_t *offset_p)
{
    dmd3mesh_t      header;
    size_t          end;
    dmd3vertex_t    *src_vert;
    dmd3coord_t     *src_tc;
    dmd3skin_t      *src_skin;
    uint32_t        *src_idx;
    maliasvert_t    *dst_vert;
    maliastc_t      *dst_tc;
    QGL_INDEX_TYPE  *dst_idx;
    uint32_t        index;
    char            skinname[MAX_QPATH];
    int             i;

    if (length < sizeof(header))
        return Q_ERR_BAD_EXTENT;

    // byte swap the header
    header = *(dmd3mesh_t *)rawdata;
    for (i = 0; i < sizeof(header) / 4; i++)
        ((uint32_t *)&header)[i] = LittleLong(((uint32_t *)&header)[i]);

    if (header.meshsize < sizeof(header) || header.meshsize > length)
        return Q_ERR_BAD_EXTENT;
    if (header.num_verts < 3)
        return Q_ERR_TOO_FEW;
    if (header.num_verts > TESS_MAX_VERTICES)
        return Q_ERR_TOO_MANY;
    if (header.num_tris < 1)
        return Q_ERR_TOO_FEW;
    if (header.num_tris > TESS_MAX_INDICES / 3)
        return Q_ERR_TOO_MANY;
    if (header.num_skins > MAX_ALIAS_SKINS)
        return Q_ERR_TOO_MANY;
    end = header.ofs_skins + header.num_skins * sizeof(dmd3skin_t);
    if (end < header.ofs_skins || end > length)
        return Q_ERR_BAD_EXTENT;
    end = header.ofs_verts + header.num_verts * model->numframes * sizeof(dmd3vertex_t);
    if (end < header.ofs_verts || end > length)
        return Q_ERR_BAD_EXTENT;
    end = header.ofs_tcs + header.num_verts * sizeof(dmd3coord_t);
    if (end < header.ofs_tcs || end > length)
        return Q_ERR_BAD_EXTENT;
    end = header.ofs_indexes + header.num_tris * 3 * sizeof(uint32_t);
    if (end < header.ofs_indexes || end > length)
        return Q_ERR_BAD_EXTENT;

    mesh->numtris = header.num_tris;
    mesh->numindices = header.num_tris * 3;
    mesh->numverts = header.num_verts;
    mesh->numskins = header.num_skins;
    mesh->verts = MOD_Malloc(sizeof(maliasvert_t) * header.num_verts * model->numframes);
    mesh->tcoords = MOD_Malloc(sizeof(maliastc_t) * header.num_verts);
    mesh->indices = MOD_Malloc(sizeof(QGL_INDEX_TYPE) * header.num_tris * 3);

    // load all skins
    src_skin = (dmd3skin_t *)(rawdata + header.ofs_skins);
    for (i = 0; i < header.num_skins; i++) {
        if (!Q_memccpy(skinname, src_skin->name, 0, sizeof(skinname)))
            return Q_ERR_STRING_TRUNCATED;
        FS_NormalizePath(skinname, skinname);
        mesh->skins[i] = IMG_Find(skinname, IT_SKIN, IF_NONE);
    }

    // load all vertices
    src_vert = (dmd3vertex_t *)(rawdata + header.ofs_verts);
    dst_vert = mesh->verts;
    for (i = 0; i < header.num_verts * model->numframes; i++) {
        dst_vert->pos[0] = (int16_t)LittleShort(src_vert->point[0]);
        dst_vert->pos[1] = (int16_t)LittleShort(src_vert->point[1]);
        dst_vert->pos[2] = (int16_t)LittleShort(src_vert->point[2]);

        dst_vert->norm[0] = src_vert->norm[0];
        dst_vert->norm[1] = src_vert->norm[1];

        src_vert++; dst_vert++;
    }

    // load all texture coords
    src_tc = (dmd3coord_t *)(rawdata + header.ofs_tcs);
    dst_tc = mesh->tcoords;
    for (i = 0; i < header.num_verts; i++) {
        dst_tc->st[0] = LittleFloat(src_tc->st[0]);
        dst_tc->st[1] = LittleFloat(src_tc->st[1]);
        src_tc++; dst_tc++;
    }

    // load all triangle indices
    src_idx = (uint32_t *)(rawdata + header.ofs_indexes);
    dst_idx = mesh->indices;
    for (i = 0; i < header.num_tris * 3; i++) {
        index = LittleLong(*src_idx++);
        if (index >= header.num_verts)
            return Q_ERR_BAD_INDEX;
        *dst_idx++ = index;
    }

    *offset_p = header.meshsize;
    return Q_ERR_SUCCESS;
}

static int MOD_LoadMD3(model_t *model, const void *rawdata, size_t length)
{
    dmd3header_t    header;
    size_t          end, offset, remaining;
    dmd3frame_t     *src_frame;
    maliasframe_t   *dst_frame;
    const byte      *src_mesh;
    int             i, ret;

    if (length < sizeof(header))
        return Q_ERR_FILE_TOO_SMALL;

    // byte swap the header
    header = *(dmd3header_t *)rawdata;
    for (i = 0; i < sizeof(header) / 4; i++)
        ((uint32_t *)&header)[i] = LittleLong(((uint32_t *)&header)[i]);

    if (header.ident != MD3_IDENT)
        return Q_ERR_UNKNOWN_FORMAT;
    if (header.version != MD3_VERSION)
        return Q_ERR_UNKNOWN_FORMAT;
    if (header.num_frames < 1)
        return Q_ERR_TOO_FEW;
    if (header.num_frames > MD3_MAX_FRAMES)
        return Q_ERR_TOO_MANY;
    end = header.ofs_frames + sizeof(dmd3frame_t) * header.num_frames;
    if (end < header.ofs_frames || end > length)
        return Q_ERR_BAD_EXTENT;
    if (header.num_meshes < 1)
        return Q_ERR_TOO_FEW;
    if (header.num_meshes > MD3_MAX_MESHES)
        return Q_ERR_TOO_MANY;
    if (header.ofs_meshes > length)
        return Q_ERR_BAD_EXTENT;

    Hunk_Begin(&model->hunk, 0x400000);
    model->type = MOD_ALIAS;
    model->numframes = header.num_frames;
    model->nummeshes = header.num_meshes;
    model->meshes = MOD_Malloc(sizeof(maliasmesh_t) * header.num_meshes);
    model->frames = MOD_Malloc(sizeof(maliasframe_t) * header.num_frames);

    // load all frames
    src_frame = (dmd3frame_t *)((byte *)rawdata + header.ofs_frames);
    dst_frame = model->frames;
    for (i = 0; i < header.num_frames; i++) {
        LittleVector(src_frame->translate, dst_frame->translate);
        VectorSet(dst_frame->scale, MD3_XYZ_SCALE, MD3_XYZ_SCALE, MD3_XYZ_SCALE);

        LittleVector(src_frame->mins, dst_frame->bounds[0]);
        LittleVector(src_frame->maxs, dst_frame->bounds[1]);
        dst_frame->radius = LittleFloat(src_frame->radius);

        src_frame++; dst_frame++;
    }

    // load all meshes
    src_mesh = (const byte *)rawdata + header.ofs_meshes;
    remaining = length - header.ofs_meshes;
    for (i = 0; i < header.num_meshes; i++) {
        ret = MOD_LoadMD3Mesh(model, &model->meshes[i], src_mesh, remaining, &offset);
        if (ret)
            goto fail;
        src_mesh += offset;
        remaining -= offset;
    }

    Hunk_End(&model->hunk);
    return Q_ERR_SUCCESS;

fail:
    Hunk_Free(&model->hunk);
    return ret;
}
#endif

void MOD_Reference(model_t *model)
{
    int i, j;

    // register any images used by the models
    switch (model->type) {
    case MOD_ALIAS:
        for (i = 0; i < model->nummeshes; i++) {
            maliasmesh_t *mesh = &model->meshes[i];
            for (j = 0; j < mesh->numskins; j++) {
                mesh->skins[j]->registration_sequence = registration_sequence;
            }
        }
        break;
	case MOD_SPRITE:
		for (i = 0; i < model->numframes; i++) {
			model->spriteframes[i].image->registration_sequence = registration_sequence;
		}
		break;
    case MOD_EMPTY:
        break;
	// Generations
	case MOD_DIR_SPRITE:
		for (i = 0; i < model->numframes; i++) {
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
int MOD_LoadMDL(model_t *model, const void *rawdata, size_t length);
static int MOD_LoadWSC(model_t *model, const void *rawdata, size_t length);

qhandle_t R_RegisterModel(const char *name)
{
    char normalized[MAX_QPATH];
	union {
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

    if (*name == '*') {
        // inline bsp model
		index.model.type = MODELHANDLE_BSP;
		index.model.id = atoi(name + 1);
        return index.handle;
    } else if (*name == '%') { // model script
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
    if (namelen == 0) {
        Com_DPrintf("%s: empty name\n", __func__);
		return 0;
    }

    // see if it's already loaded
    model = MOD_Find(normalized);
    if (model) {
        MOD_Reference(model);
        goto done;
    }

    filelen = FS_LoadFile(normalized, (void **)&rawdata);
    if (!rawdata) {
        // don't spam about missing models
        if (filelen == Q_ERR_NOENT) {
			return 0;
        }

        ret = filelen;
        goto fail1;
    }

    if (filelen < 4) {
        ret = Q_ERR_FILE_TOO_SMALL;
        goto fail2;
    }

	// Generations
	const char *extension = strchr(name, '.') + 1;

	if (!Q_stricmp(extension, "q1m"))
		load = MOD_LoadQ1M;
	else if (!Q_stricmp(extension, "mdl"))
		load = MOD_LoadMDL;
	else if (!Q_stricmp(extension, "spr"))
		load = MOD_LoadSPR;
	else if (!Q_stricmp(extension, "md2"))
		load = MOD_LoadMD2;
#if USE_MD3
	else if (!Q_stricmp(extension, "md3"))
		load = MOD_LoadMD3;
#endif
	// Paril
	else if (!Q_stricmp(extension, "sp2"))
		load = MOD_LoadSP2;
	else if (!Q_stricmp(extension, "d2s") || !Q_stricmp(extension, "dns"))
		load = MOD_LoadD2S;
	//else if (!Q_stricmp(extension, "mdx"))
	//	load = MOD_LoadMDX;
	else if (!Q_stricmp(extension, "wsc"))
		load = MOD_LoadWSC;
	else
	{
		ret = Q_ERR_UNKNOWN_FORMAT;
		goto fail2;
	}

    model = MOD_Alloc();
    if (!model) {
        ret = Q_ERR_OUT_OF_SLOTS;
        goto fail2;
    }

    memcpy(model->name, normalized, namelen + 1);
    model->registration_sequence = registration_sequence;

    ret = load(model, rawdata, filelen);

    FS_FreeFile(rawdata);

    if (ret) {
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

    if (!MODEL_HANDLE_TYPE(index)) {
        return NULL;
    }

	switch (MODEL_HANDLE_TYPE(index))
	{
	case MODELHANDLE_BSP:
	default:
		Com_Error(ERR_DROP, "%s: should never happen", __func__);
	case MODELHANDLE_RAW:
		if (MODEL_HANDLE_ID(index) < 0 || MODEL_HANDLE_ID(index) > r_numModels) {
			Com_Error(ERR_DROP, "%s: %d out of range", __func__, MODEL_HANDLE_ID(index));
		}

		model = &r_models[MODEL_HANDLE_ID(index) - 1];
		if (!model->type) {
			return NULL;
		}
		break;
	case MODELHANDLE_GAMED:
		if (MODEL_HANDLE_ID(index) < 0 || MODEL_HANDLE_ID(index) > r_numModelScripts) {
			Com_Error(ERR_DROP, "%s: %d out of range", __func__, MODEL_HANDLE_ID(index));
		}

		modelentry_t *entry = MSCR_EntryForHandle(index, game);

		if (!entry->loaded)
			return NULL;

		return MOD_ForHandle(entry->model, game);
	}

    return model;
}

void MOD_Init(void)
{
    if (r_numModels) {
        Com_Error(ERR_FATAL, "%s: %d models not freed", __func__, r_numModels);
    }

	if (r_numModelScripts) {
		Com_Error(ERR_FATAL, "%s: %d model scripts not freed", __func__, r_numModelScripts);
	}

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
	byte	*ptr = (byte*)rawdata + 4;

	int		texture_width = *ptr++;
	int		texture_height = *ptr++;
	int		texture_frames = *ptr++;
	int		num_verts = *ptr++;
	int		num_faces = *ptr++;
	int		num_indices = num_faces * 6;

	char	skinname[MAX_QPATH];

	Z_TagChunkCreate(TAG_MODEL, &model->memory, sizeof(maliasmesh_t) + sizeof(maliasframe_t) +
					(num_verts * sizeof(maliasvert_t)) +
					(num_verts * sizeof(maliastc_t)) +
					 num_indices * sizeof(QGL_INDEX_TYPE));
	model->type = MOD_ALIAS;
	model->nummeshes = 1;
	model->numframes = 1;

    model->meshes = Z_ChunkAlloc(&model->memory, sizeof(maliasmesh_t));
    model->frames = Z_ChunkAlloc(&model->memory, model->numframes * sizeof(maliasframe_t));

	maliasmesh_t *dst_mesh = model->meshes;

	byte *pic = (byte*)IMG_AllocPixels(texture_width * texture_height * 4);

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
			dst_mesh->verts[i].pos[x] = (char)*ptr++;

		tmp[0] = dst_mesh->verts[i].pos[0];
		tmp[1] = dst_mesh->verts[i].pos[1];
		tmp[2] = dst_mesh->verts[i].pos[2];

		AddPointToBounds(tmp, frame->bounds[0], frame->bounds[1]);

		dst_mesh->verts[i].norm[0] = 0;
		dst_mesh->verts[i].norm[1] = 0;

		for (int x = 0; x < 2; ++x)
			dst_mesh->tcoords[i].st[x] = *ptr++ * ((x == 0) ? inv_w : inv_h);
	}

	dst_mesh->numtris = num_indices / 3;
	dst_mesh->numindices = num_indices;
	dst_mesh->indices = Z_ChunkAlloc(&model->memory, num_indices * sizeof(QGL_INDEX_TYPE));

	for (int i = 0; i < num_indices; ++i)
		dst_mesh->indices[i] = *ptr++;

	frame->radius = RadiusFromBounds(frame->bounds[0], frame->bounds[1]);
	VectorSet(frame->scale, 1, 1, 1);
	VectorSet(frame->translate, 0, 0, 0);

	return Q_ERR_SUCCESS;
}

void MOD_MDLLoadFrame(dmdlheader_t *header, maliasmesh_t *dst_mesh, maliasframe_t *dst_frame, dmdlvertex_t *src_verts, float *mins, float *maxs, GLuint *indice_remap, int frame_id)
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

		int val = src_vert->normalIndex;
		if (val >= NUMVERTEXNORMALS) {
			dst_vert->norm[0] = 0;
			dst_vert->norm[1] = 0;
		}
		else {
			dst_vert->norm[0] = gl_static.latlngtab[val][0];
			dst_vert->norm[1] = gl_static.latlngtab[val][1];
		}

		for (int k = 0; k < 3; k++) {
			val = dst_vert->pos[k];
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
	dmdlframe_t     *src_frame;
	dmdlvertex_t    *src_verts;
	dmdltriangle_t  *src_tri;
	dmdltexcoord_t  *src_tc;
	dmdlskin_t      *src_skin;
	maliasframe_t   *dst_frame;
	maliasmesh_t    *dst_mesh;
	maliastc_t      *dst_tc;
	QGL_INDEX_TYPE	*dst_idx;
	int             i, j;
	char            skinname[MAX_QPATH];
	vec_t           scale_s, scale_t;
	vec3_t          mins, maxs;
	byte			*ptr, *pic;
	image_t			*skins[MD2_MAX_SKINS];
	QGL_INDEX_TYPE	indice_remap[MDL_MAX_TRIANGLES * 3];

	if (length < sizeof(header)) {
		return Q_ERR_FILE_TOO_SMALL;
	}

	// byte swap the header
	header = *(dmdlheader_t *)rawdata;
	for (i = 0; i < sizeof(header) / 4; i++) {
		((uint32_t *)&header)[i] = LittleLong(((uint32_t *)&header)[i]);
	}

	// skins
	ptr = (byte*)rawdata + sizeof(dmdlheader_t);

	pic = (byte*)IMG_AllocPixels(header.skinwidth * header.skinheight * 4);

	for (i = 0; i < header.num_skins; ++i)
	{
		src_skin = (dmdlskin_t*)ptr;

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

	src_tc = (dmdltexcoord_t*)ptr;

	ptr += sizeof(dmdltexcoord_t) * header.num_verts;

	src_tri = (dmdltriangle_t*)ptr;

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
	dmdlframe_t *src_frames = (dmdlframe_t*)ptr;
	ptr = (byte*)src_frames;

	int real_num_frames = 0;

	for (j = 0; j < header.num_frames; ++j)
	{
		src_frame = (dmdlframe_t*)ptr;

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
			dmdlgroupframe_t *group = (dmdlgroupframe_t*)ptr;

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
		(header.num_tris * 3 * sizeof(QGL_INDEX_TYPE)));

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
	dst_mesh->indices = Z_ChunkAlloc(&model->memory, header.num_tris * 3 * sizeof(QGL_INDEX_TYPE));

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
	ptr = (byte*)src_frames;

	for (j = 0; j < header.num_frames; )
	{
		src_frame = (dmdlframe_t*)ptr;

		ptr += sizeof(dmdlframe_t);

		if (src_frame->type == 0)
		{
			// verts directly follow simple frames
			ptr += sizeof(dmdlsimpleframe_t);

			src_verts = (dmdlvertex_t*)ptr;

			MOD_MDLLoadFrame(&header, dst_mesh, dst_frame, src_verts, mins,maxs, indice_remap, j);
			dst_frame++;

			ptr += sizeof(dmdlvertex_t) * header.num_verts;
			j++;
		}
		else
		{
			// groups are complicated :(
			dmdlgroupframe_t *group = (dmdlgroupframe_t*)ptr;

			ptr += sizeof(dmdlgroupframe_t);

			// skip timings
			ptr += sizeof(float) * group->num_frames;

			// add frames
			for (i = 0; i < group->num_frames; ++i)
			{
				ptr += sizeof(dmdlsimpleframe_t);

				src_verts = (dmdlvertex_t*)ptr;

				MOD_MDLLoadFrame(&header, dst_mesh, dst_frame, src_verts, mins, maxs, indice_remap, j);
				dst_frame++;

				ptr += sizeof(dmdlvertex_t) * header.num_verts;

				j++;
			}
		}
	}

	return Q_ERR_SUCCESS;
}

void GL_StretchPic(
	float x, float y, float w, float h,
	float s1, float t1, float s2, float t2,
	uint32_t color, image_t *image);

#define GL_WeaponPic(frame, drawFrame, x, y) \
			GL_StretchPic(	(x) * heightAspectInverted, (y) * heightAspectInverted, frame->width * heightAspectInverted, frame->height * heightAspectInverted, \
							(drawFrame->flip & flip_x) ? frame->image->sh : frame->image->sl, (drawFrame->flip & flip_y) ? frame->image->th : frame->image->tl, \
							(drawFrame->flip & flip_x) ? frame->image->sl : frame->image->sh, (drawFrame->flip & flip_y) ? frame->image->tl : frame->image->th, \
							draw.colors[0].u32, frame->image)
		

static void PointFromSizeAnchor(vec2_t out, const vec2_t pos, const vec2_t size, mweaponscript_draw_anchor_t anchor)
{
	if (anchor == anchor_bottom_left ||
		anchor == anchor_center_left ||
		anchor == anchor_top_left)
		out[0] = pos[0];
	else if (anchor == anchor_bottom_right ||
		anchor == anchor_center_right ||
		anchor == anchor_top_right)
		out[0] = pos[0] + size[0];
	else
		out[0] = pos[0] + size[0] / 2;

	if (anchor == anchor_top_right ||
		anchor == anchor_top_center ||
		anchor == anchor_top_left)
		out[1] = pos[1];
	else if (anchor == anchor_bottom_right ||
		anchor == anchor_bottom_center ||
		anchor == anchor_bottom_left)
		out[1] = pos[1] + size[1];
	else
		out[1] = pos[1] + size[1] / 2;
}

static void R_SetWeaponSpriteScale(float scale, float width, float height)
{
	GL_Flush2D();

	GL_Ortho(0, width, height, 0, -1, 1);

	draw.scale = scale;
}

extern vrect_t     scr_vrect;

void R_DrawWeaponSprite(qhandle_t handle, gunindex_e index, float velocity, int time, float frametime, int oldframe, int frame, float frac, int hud_width, int hud_height, bool firing, float yofs, float scale, color_t color, bool invul, float height_diff)
{
	model_t *model = MOD_ForHandle(handle, CL_GetClientGame());

	if (!model)
		return;

	if (model->type != MOD_WEAPONSCRIPT)
		Com_Error(ERR_FATAL, "What");

	mweaponscript_t *script = model->weaponscript;

#define WEAPONTOP (script->view[1] / 2.0)
	
	// bob the weapon based on movement speed
	static float bob[MAX_PLAYER_GUNS] = { 0 };
	float wanted_bob;

	if (!firing)
		wanted_bob = min(6.4f, velocity * 0.02f);
	else
		wanted_bob = 0;

	if (wanted_bob > bob[index])
		bob[index] = min(wanted_bob, bob[index] + frametime * 15);
	else if (wanted_bob < bob[index])
		bob[index] = max(wanted_bob, bob[index] - frametime * 15);
	
	const float bob_x_scale = (CL_GetClientGame() == GAME_DUKE ? 1 : 2);
	const float bob_y_scale = (CL_GetClientGame() == GAME_DUKE ? 1.5 : 2.5);

    float angle = ((128.0f / 40000) * time);
    float bob_x = (bob[index] * sin(angle)) * bob_x_scale;
    angle *= 2;
    float bob_y = (bob[index] * fabs(cos(angle / 2.0))) * bob_y_scale;

	if (frame >= script->num_anim_frames)
		return;

	float oldScale = draw.scale;
	R_SetWeaponSpriteScale(1.0, r_config.width, r_config.height);

	float heightAspect = script->view[1] / (r_config.height - height_diff);
	float realWidth = r_config.width * heightAspect;
	float sidePadding = (realWidth - script->view[0]) / 2.0f;

	float heightAspectInverted = 1.0 / heightAspect;

	const vec2_t realScreenSize = { realWidth, script->view[1] };
	const vec2_t realScreenPosition = { -sidePadding, 0 };

	mweaponscript_frame_t *of = &script->frames[script->anim_frames[oldframe]];
	mweaponscript_frame_t *f = &script->frames[script->anim_frames[frame]];

	R_SetColor(color.u32);

	GL_Flush2D();

	static const vec2_t virtualPosition = { 0, 0 };
	const vec_t *virtualSize = &script->view[0];
	int i;

	yofs = WEAPONTOP * yofs;

	for (i = script->num_sprites - 1; i >= 0; i--)
	{
		if (!f->draws[i].draw)
			continue;

		const model_t *sprite = MOD_ForHandle(script->sprites[i].model, CL_GetClientGame());

		const mweaponscript_frame_draw_t *drawFrame = &f->draws[i];
		const mspritedirframe_t *spriteFrame = &sprite->spritedirframes[drawFrame->sprite_frame].directions[0];
		const vec2_t objectSize = { spriteFrame->image->width, spriteFrame->image->height };
		
		vec2_t scr, pos;
		PointFromSizeAnchor(scr, drawFrame->of_fullscreen ? realScreenPosition : virtualPosition, drawFrame->of_fullscreen ? realScreenSize : virtualSize, drawFrame->at);
		PointFromSizeAnchor(pos, virtualPosition, objectSize, drawFrame->my);

		float curX = sidePadding + (scr[0] - pos[0] + drawFrame->offset[0]);
		float curY = scr[1] - pos[1] + drawFrame->offset[1];

		vec2_t translate = { f->translate[0], f->translate[1] };

		if (f->lerp)
		{
			if ((f->lerp & lerp_draws) && of->draws[i].draw)
			{
				const mweaponscript_frame_draw_t *prevDrawFrame = &of->draws[i];
				const mspritedirframe_t *prevSpriteFrame = &sprite->spritedirframes[prevDrawFrame->sprite_frame].directions[0];
				const vec2_t prevObjectSize = { prevSpriteFrame->image->width, prevSpriteFrame->image->height };
		
				vec2_t prevScr, prevPos;
				PointFromSizeAnchor(prevScr, prevDrawFrame->of_fullscreen ? realScreenPosition : virtualPosition, prevDrawFrame->of_fullscreen ? realScreenSize : virtualSize, prevDrawFrame->at);
				PointFromSizeAnchor(prevPos, virtualPosition, prevObjectSize, prevDrawFrame->my);

				const float prevX = sidePadding + (prevScr[0] - prevPos[0] + prevDrawFrame->offset[0]);
				const float prevY = prevScr[1] - prevPos[1] + prevDrawFrame->offset[1];
				
				curX = LerpFloat(prevX, curX, frac);
				curY = LerpFloat(prevY, curY, frac);
			}

			if (f->lerp & lerp_translates)
			{
				translate[0] = LerpFloat(of->translate[0], translate[0], frac);
				translate[1] = LerpFloat(of->translate[1], translate[1], frac);
			}
		}

		GL_WeaponPic(spriteFrame, drawFrame, curX + translate[0] + bob_x, curY + translate[1] + yofs + bob_y);
	}

	if (invul)
		tess.flags |= 4;

	GL_Flush2D();

	R_SetColor(colorTable[COLOR_WHITE]);

	R_SetScale(oldScale);
}

static mweaponscript_draw_anchor_t ParseAnchor(const char *token)
{
	if (Q_stricmp(token, "top left") == 0)
		return anchor_top_left;
	else if (Q_stricmp(token, "top center") == 0)
		return anchor_top_center;
	else if (Q_stricmp(token, "top right") == 0)
		return anchor_top_right;
	else if (Q_stricmp(token, "center right") == 0)
		return anchor_center_right;
	else if (Q_stricmp(token, "bottom right") == 0)
		return anchor_bottom_right;
	else if (Q_stricmp(token, "bottom center") == 0)
		return anchor_bottom_center;
	else if (Q_stricmp(token, "bottom left") == 0)
		return anchor_bottom_left;
	else if (Q_stricmp(token, "center left") == 0)
		return anchor_center_left;

	return anchor_center;
}

static int MOD_LoadWSC(model_t *model, const void *rawdata, size_t length)
{
	model->type = MOD_WEAPONSCRIPT;

	const char *file = (const char *)rawdata;
	char *token;

	if ((token = COM_Parse(&file))[0] != '{')
		return Q_ERR_INVALID_FORMAT;

	const char *num_searcher = file;

	int num_frames = 0, num_sprites = 0, num_anim_frames = 0;

	while (true)
	{
		token = COM_Parse(&num_searcher);

		if (!token || !num_searcher)
			break;

		if (Q_stricmp(token, "{") == 0)
			num_frames++;
		else if (Q_stricmp(token, "load_sprite") == 0)
			num_sprites++;
		else if (Q_stricmp(token, "animations") == 0)
		{
			token = COM_Parse(&num_searcher);
			const char *anim_searcher = token;

			while (true)
			{
				token = COM_Parse(&anim_searcher);

				if (!token || !anim_searcher)
					break;

				num_anim_frames++;
			}
		}
	}

	Z_TagChunkCreate(TAG_MODEL, &model->memory, sizeof(mweaponscript_t) +
					(sizeof(mweaponscript_sprite_t) * num_sprites) +
					((sizeof(mweaponscript_frame_t) + (sizeof(mweaponscript_frame_draw_t) * num_sprites)) * num_frames) +
					(sizeof(int) * num_anim_frames));
	mweaponscript_t *script_ptr = model->weaponscript = Z_ChunkAlloc(&model->memory, sizeof(mweaponscript_t));

	script_ptr->num_frames = num_frames;
	script_ptr->num_sprites = num_sprites;
	script_ptr->num_anim_frames = num_anim_frames;

	script_ptr->sprites = Z_ChunkAlloc(&model->memory, sizeof(mweaponscript_sprite_t) * script_ptr->num_sprites);
	script_ptr->frames = Z_ChunkAlloc(&model->memory, sizeof(mweaponscript_frame_t) * script_ptr->num_frames);
	script_ptr->anim_frames = Z_ChunkAlloc(&model->memory, sizeof(int) * script_ptr->num_anim_frames);

	for (int i = 0; i < script_ptr->num_frames; ++i)
		script_ptr->frames[i].draws = Z_ChunkAlloc(&model->memory, sizeof(mweaponscript_frame_draw_t) * script_ptr->num_sprites);

	int cur_sprite = 0;
	int cur_frame = 0;
	int cur_anim = 0;

	while ((token = COM_Parse(&file))[0] != '}')
	{
		if (!Q_stricmp(token, "view"))
		{
			token = COM_Parse(&file);
			sscanf(token, "%f %f", &script_ptr->view[0], &script_ptr->view[1]);
		}
		else if (!Q_stricmp(token, "load_sprite"))
		{
			token = COM_Parse(&file);
			script_ptr->sprites[cur_sprite++].model = R_RegisterModel(token);
		}
		else if (!Q_stricmp(token, "animations"))
		{
			token = COM_Parse(&file);
			const char *anim_searcher = token;

			while (true)
			{
				token = COM_Parse(&anim_searcher);

				if (!token || !anim_searcher)
					break;

				script_ptr->anim_frames[cur_anim++] = atoi(token);
			}
		}
		else if (!Q_stricmp(token, "{"))
		{
			int frame_id = cur_frame++;

			mweaponscript_frame_t *frame = &script_ptr->frames[frame_id];

			while ((token = COM_Parse(&file))[0] != '}')
			{
				if (!Q_strncasecmp(token, "draw_sprite", 11))
				{
					int sprite_id = atoi(COM_Parse(&file));

					if (!Q_strncasecmp(token, "draw_sprite_flip_", 17))
					{
						const char *whom = token + 17;

						while (*whom)
						{
							if (*whom == 'x')
								frame->draws[sprite_id].flip |= flip_x;
							if (*whom == 'y')
								frame->draws[sprite_id].flip |= flip_y;
							whom++;
						}
					}

					int sprite_frame_id = atoi(COM_Parse(&file));

					frame->draws[sprite_id].draw = true;
					frame->draws[sprite_id].sprite_frame = sprite_frame_id;
					frame->draws[sprite_id].at = ParseAnchor(COM_Parse(&file));
					frame->draws[sprite_id].my = ParseAnchor(COM_Parse(&file));

					token = COM_Parse(&file);
					sscanf(token, "%f %f", &frame->draws[sprite_id].offset[0], &frame->draws[sprite_id].offset[1]);
					
					token = COM_Parse(&file);
					frame->draws[sprite_id].of_fullscreen = (Q_stricmp(token, "full") == 0);
				}
				else if (!Q_stricmp(token, "lerp"))
				{
					token = COM_Parse(&file);
					const char *lerp_searcher = token;

					while (true)
					{
						token = COM_Parse(&lerp_searcher);

						if (!token || !lerp_searcher)
							break;
						
						if (!Q_stricmp(token, "draws"))
							frame->lerp |= lerp_draws;
						else if (!Q_stricmp(token, "translates"))
							frame->lerp |= lerp_translates;
					}
				}
				else if (!Q_stricmp(token, "translate"))
				{
					token = COM_Parse(&file);
					sscanf(token, "%f %f", &frame->translate[0], &frame->translate[1]);
				}
			}
		}
	}

	return Q_ERR_SUCCESS;
}


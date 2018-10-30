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

// Paril
#include "format/mdl.h"
#include "format/spr.h"

#include "format/md2.h"
#if USE_MD3
#include "format/md3.h"
#endif
#include "format/sp2.h"

typedef int (*mod_load_t) (model_t *, const void *, size_t);

#define MOD_Malloc(size)    Hunk_Alloc(&model->hunk, size)

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
				r_modelscripts[i].entries[g].model = (modelhandle_t) { 0 };
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

modelentry_t *MSCR_EntryForHandle(modelhandle_t handle, gametype_t game)
{
	modelscript_t *script = &r_modelscripts[handle.model.id - 1];

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
        Com_Printf("%c %8"PRIz" : %s\n", types[model->type],
                   model->hunk.mapped, model->name);
        bytes += model->hunk.mapped;
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
        if (model->registration_sequence == registration_sequence) {
            // make sure it is paged in
            Com_PageInMemory(model->hunk.base, model->hunk.cursize);
        } else {
            // don't need this model
            Hunk_Free(&model->hunk);
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

        Hunk_Free(&model->hunk);
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

    Hunk_Begin(&model->hunk, 0x10000);
    model->type = MOD_SPRITE;

    model->spriteframes = MOD_Malloc(sizeof(mspriteframe_t) * header.numframes);
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

	Hunk_End(&model->hunk);

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

    Hunk_Begin(&model->hunk, 0x10000);
	model->type = MOD_SPRITE;

	model->spriteframes = MOD_Malloc(sizeof(mspriteframe_t) * header.numframes);
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
	
    Hunk_End(&model->hunk);
	return Q_ERR_SUCCESS;
fail:
    Hunk_Free(&model->hunk);
    return ret;
}

#define D2S_IDENT       (('P'<<24)+('S'<<16)+('2'<<8)+'D')
#define DNS_IDENT       (('P'<<24)+('S'<<16)+('N'<<8)+'D')

typedef struct {
	short x, y;
	short width, height;
} d2s_temp_t;

// Paril
static int MOD_LoadD2S(model_t *model, const void *rawdata, size_t length)
{
	uint32_t ident = *(uint32_t*)rawdata;
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

		MOD_AddIDToName(model->name, i, buffer, MAX_QPATH);
		byte *temppic = (byte*)IMG_AllocPixels(offsets[i].width * offsets[i].height * 4);
		image_t *image = IMG_Find(buffer, IT_SPRITE, IF_DELAYED | IF_OLDSCHOOL);

		image->upload_width = image->width = offsets[i].width;
		image->upload_height = image->height = offsets[i].height;
		image->flags |= IMG_Unpack8((uint32_t *)temppic, ptr, offsets[i].width, offsets[i].height, d_palettes[ident == D2S_IDENT ? GAME_DOOM : GAME_DUKE]);

		IMG_Load(image, temppic);

		IMG_FreePixels(temppic);
		images[i] = image;

		ptr += offsets[i].width * offsets[i].height;
	}

	byte num_frames = *ptr++;
	byte num_dirs = *ptr++;
	
    Hunk_Begin(&model->hunk, 0x10000);

	mspritedirframes_t *frames = model->spritedirframes = MOD_Malloc(sizeof(mspritedirframes_t) * num_frames);

	for (int i = 0; i < num_frames; ++i)
	{
		mspritedirframes_t *frame = &frames[i];
		mspritedirframe_t *dirs = MOD_Malloc(sizeof(mspritedirframe_t) * num_dirs);

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
		int x2 = -offsets[i].x + offsets[i].width;
		int y2 = -offsets[i].y + offsets[i].height;

		int highest = max(abs(y2), max(abs(x2), max(abs(x1), abs(y1))));

		vec3_t low = { -highest, -highest, -highest };
		vec3_t high = { highest, highest, highest };

		AddPointToBounds(low, mins, maxs);
		AddPointToBounds(high, mins, maxs);
	}

	model->radius = RadiusFromBounds(mins, maxs);

	Z_Free(offsets);
	Z_Free(images);

    Hunk_End(&model->hunk);

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

    if (header->skinwidth < 1 || header->skinwidth > MD2_MAX_SKINWIDTH)
        return Q_ERR_INVALID_FORMAT;
    if (header->skinheight < 1 || header->skinheight > MD2_MAX_SKINHEIGHT)
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

    Hunk_Begin(&model->hunk, 0x400000);
    model->type = MOD_ALIAS;
    model->nummeshes = 1;
    model->numframes = header.num_frames;
    model->meshes = MOD_Malloc(sizeof(maliasmesh_t));
    model->frames = MOD_Malloc(header.num_frames * sizeof(maliasframe_t));

    dst_mesh = model->meshes;
    dst_mesh->numtris = numindices / 3;
    dst_mesh->numindices = numindices;
    dst_mesh->numverts = numverts;
    dst_mesh->numskins = header.num_skins;
    dst_mesh->verts = MOD_Malloc(numverts * header.num_frames * sizeof(maliasvert_t));
    dst_mesh->tcoords = MOD_Malloc(numverts * sizeof(maliastc_t));
    dst_mesh->indices = MOD_Malloc(numindices * sizeof(QGL_INDEX_TYPE));

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

    Hunk_End(&model->hunk);
    return Q_ERR_SUCCESS;

fail:
    Hunk_Free(&model->hunk);
    return ret;
}

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
				if (model->weaponscript->sprites[i].model.handle)
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

modelhandle_t R_RegisterModel(const char *name)
{
    char normalized[MAX_QPATH];
	modelhandle_t index;
    size_t namelen;
    int filelen;
    model_t *model;
    byte *rawdata;
    mod_load_t load;
    int ret;

    // empty names are legal, silently ignore them
    if (!*name)
		return (modelhandle_t) { 0 };

    if (*name == '*') {
        // inline bsp model
		index.model.type = MODELHANDLE_BSP;
		index.model.id = atoi(name + 1);
        return index;
    } else if (*name == '%') { // model script
		index.model.type = MODELHANDLE_GAMED;

		modelscript_t *script = MSCR_Find(name + 1);

		if (script)
		{
			index.model.id = (script - r_modelscripts) + 1;
			return index;
		}

		// gotta load it
		script = MSCR_Register(name + 1);

		if (script)
		{
			index.model.id = (script - r_modelscripts) + 1;
			return index;
		}

		Com_EPrintf("Couldn't load modelscript %s\n", name + 1);
		return (modelhandle_t) { 0 };
	}

    // normalize the path
    namelen = FS_NormalizePathBuffer(normalized, name, MAX_QPATH);

    // this should never happen
    if (namelen >= MAX_QPATH)
        Com_Error(ERR_DROP, "%s: oversize name", __func__);

    // normalized to empty name?
    if (namelen == 0) {
        Com_DPrintf("%s: empty name\n", __func__);
		return (modelhandle_t) { 0 };
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
			return (modelhandle_t) { 0 };
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
	else if (!Q_stricmp(extension, "d2s") || !Q_stricmp(extension, "dns"))
		load = MOD_LoadD2S;
	else if (!Q_stricmp(extension, "md2"))
		load = MOD_LoadMD2;
#if USE_MD3
	else if (!Q_stricmp(extension, "sp2"))
		load = MOD_LoadSP2;
#endif
	else if (!Q_stricmp(extension, "sp2"))
		load = MOD_LoadSP2;
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
    return index;

fail2:
    FS_FreeFile(rawdata);
fail1:
    Com_EPrintf("Couldn't load %s: %s\n", normalized, Q_ErrorString(ret));
	return (modelhandle_t) { 0 };
}

model_t *MOD_ForHandle(modelhandle_t index, gametype_t game)
{
    model_t *model;

    if (!index.model.type) {
        return NULL;
    }

	switch (index.model.type)
	{
	case MODELHANDLE_BSP:
	default:
		Com_Error(ERR_DROP, "%s: should never happen", __func__);
	case MODELHANDLE_RAW:
		if (index.model.id < 0 || index.model.id > r_numModels) {
			Com_Error(ERR_DROP, "%s: %d out of range", __func__, index.model.id);
		}

		model = &r_models[index.model.id - 1];
		if (!model->type) {
			return NULL;
		}
		break;
	case MODELHANDLE_GAMED:
		if (index.model.id < 0 || index.model.id > r_numModelScripts) {
			Com_Error(ERR_DROP, "%s: %d out of range", __func__, index.model.id);
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
int MOD_LoadQ1M(model_t *model, const void *rawdata, size_t length)
{
	byte		*ptr = (byte*)rawdata + 4;

	int			texture_width = *ptr++;
	int			texture_height = *ptr++;
	int			texture_frames = *ptr++;

	char		skinname[MAX_QPATH];

	/*

	model->size = sizeof(maliasmesh_t) +
		sizeof(maliasframe_t) +
		(num_verts * sizeof(maliasvert_t)) +
		(num_verts * sizeof(maliastc_t)) +
		(num_indices * sizeof(QGL_INDEX_TYPE));
	model->buffer = MOD_Malloc(model->size);
*/

	Hunk_Begin(&model->hunk, 0x10000);
	model->type = MOD_ALIAS;
	model->nummeshes = 1;
	model->numframes = 1;

    model->meshes = MOD_Malloc(sizeof(maliasmesh_t));
    model->frames = MOD_Malloc(model->numframes * sizeof(maliasframe_t));

	maliasmesh_t *dst_mesh = model->meshes;

	for (int i = 0; i < texture_frames; ++i)
	{
		MOD_AddIDToName(model->name, i, skinname, MAX_QPATH);
		dst_mesh->skins[i] = IMG_Find(skinname, IT_SKIN, IF_DELAYED | IF_OLDSCHOOL);

		byte *pic = (byte*)IMG_AllocPixels(texture_width * texture_height * 4);

		dst_mesh->skins[i]->upload_width = dst_mesh->skins[i]->width = texture_width;
		dst_mesh->skins[i]->upload_height = dst_mesh->skins[i]->height = texture_height;
		dst_mesh->skins[i]->flags |= IMG_Unpack8((uint32_t *)pic, ptr, texture_width, texture_height, d_palettes[GAME_Q1]);

		IMG_Load(dst_mesh->skins[i], pic);

		IMG_FreePixels(pic);

		ptr += texture_width * texture_height;
	}

	dst_mesh->numskins = texture_frames;

	int num_verts = *ptr++;

	dst_mesh->numverts = num_verts;
	dst_mesh->verts = MOD_Malloc(num_verts * sizeof(maliasvert_t));
	dst_mesh->tcoords = MOD_Malloc(num_verts * sizeof(maliastc_t));

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

	int num_faces = *ptr++;
	int num_indices = num_faces * 6;

	dst_mesh->numtris = num_indices / 3;
	dst_mesh->numindices = num_indices;
	dst_mesh->indices = MOD_Malloc(num_indices * sizeof(QGL_INDEX_TYPE));

	for (int i = 0; i < num_indices; ++i)
		dst_mesh->indices[i] = *ptr++;

	frame->radius = RadiusFromBounds(frame->bounds[0], frame->bounds[1]);
	VectorSet(frame->scale, 1, 1, 1);
	VectorSet(frame->translate, 0, 0, 0);

	Hunk_End(&model->hunk);
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

	for (i = 0; i < header.num_skins; ++i)
	{
		src_skin = (dmdlskin_t*)ptr;

		if (src_skin->group != 0)
			return Q_ERR_INVALID_FORMAT;

		Q_snprintf(skinname, MAX_QPATH, "%s_%i", model->name, i);
		skins[i] = IMG_Find(skinname, IT_SKIN, IF_DELAYED | IF_OLDSCHOOL);

		ptr += sizeof(dmdlskin_t);

		pic = (byte*)IMG_AllocPixels(header.skinwidth * header.skinheight * 4);

		skins[i]->upload_width = skins[i]->width = header.skinwidth;
		skins[i]->upload_height = skins[i]->height = header.skinheight;
		skins[i]->flags |= IMG_Unpack8((uint32_t *)pic, ptr, header.skinwidth, header.skinheight, d_palettes[GAME_Q1]);

		IMG_Load(skins[i], pic);

		IMG_FreePixels(pic);

		ptr += header.skinwidth * header.skinheight;
	}

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

	/*model->size = sizeof(maliasmesh_t) +
		(header.num_frames * sizeof(maliasframe_t)) +
		(num_real_verts * header.num_frames * sizeof(maliasvert_t)) +
		(num_real_verts * sizeof(maliastc_t)) +
		(header.num_tris * 3 * sizeof(QGL_INDEX_TYPE));
	model->buffer = MOD_Malloc(model->size);*/

	Hunk_Begin(&model->hunk, 0x100000);

	model->type = MOD_ALIAS;
	model->nummeshes = 1;
	model->numframes = header.num_frames;
	model->meshes = MOD_Malloc(sizeof(maliasmesh_t));
	model->frames = MOD_Malloc(header.num_frames * sizeof(maliasframe_t));
	model->nolerp = true;

	dst_mesh = model->meshes;
	dst_mesh->numtris = header.num_tris;
	dst_mesh->numindices = header.num_tris * 3;
	dst_mesh->numverts = num_real_verts;
	dst_mesh->numskins = header.num_skins;

	dst_mesh->verts = MOD_Malloc(num_real_verts * header.num_frames * sizeof(maliasvert_t));
	dst_mesh->tcoords = MOD_Malloc(num_real_verts * sizeof(maliastc_t));
	dst_mesh->indices = MOD_Malloc(header.num_tris * 3 * sizeof(QGL_INDEX_TYPE));

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

	Hunk_End(&model->hunk);

	return Q_ERR_SUCCESS;
}

void GL_StretchPic(
	float x, float y, float w, float h,
	float s1, float t1, float s2, float t2,
	uint32_t color, image_t *image);

void R_DrawWeaponSprite(modelhandle_t handle, gunindex_e index, float velocity, int time, float frametime, int oldframe, int frame, float frac, int hud_width, int hud_height, bool firing, float yofs, float scale, color_t color, bool invul, float height_diff)
{
	model_t *model = MOD_ForHandle(handle, CL_GetClientGame());

	if (!model)
		return;

	if (model->type != MOD_WEAPONSCRIPT)
		Com_Error(ERR_FATAL, "What");

	mweaponscript_t *script = model->weaponscript;

	float scr_width = hud_width;
	float scr_height = hud_height;

	float w_diff = (float)scr_width / script->view[0];
	float h_diff = (float)scr_height / script->view[1];
	float view_scale;

	float fit_w, fit_h;

	if (w_diff < h_diff)
		view_scale = w_diff;
	else
		view_scale = h_diff;

	fit_w = script->view[0] * view_scale;
	fit_h = script->view[1] * view_scale;

	float view_x = (scr_width / 2) - (fit_w / 2);
	float view_y = (scr_height - fit_h) + 25.5f * view_scale;

#define WEAPONTOP		64

	// bob the weapon based on movement speed
	static float bob[MAX_PLAYER_GUNS] = { 0 };
	float wanted_bob;
	float sx = 0, sy = 0;

	if (!firing)
		wanted_bob = min(6.4f, velocity * 0.02f);
	else
		wanted_bob = 0;

	if (wanted_bob > bob[index])
		bob[index] = min(wanted_bob, bob[index] + frametime * 15);
	else if (wanted_bob < bob[index])
		bob[index] = max(wanted_bob, bob[index] - frametime * 15);

	float angle = (0.0028f * time);
	sx = ((bob[index] * sin(angle)) * (CL_GetClientGame() == GAME_DUKE ? 5 : 15)) * scale;
	angle *= 2;
	sy = (WEAPONTOP + bob[index] * view_scale + (bob[index] * cos(angle)) * (CL_GetClientGame() == GAME_DUKE ? 4 : 6)) * scale;

	if (yofs)
		sy += (yofs * 80 / scale * view_scale);

	if (frame >= script->num_frames)
		return;

	mweaponscript_frame_t *of = &script->frames[oldframe];
	mweaponscript_frame_t *f = &script->frames[frame];

	R_SetColor(color.u32);

	if (invul)
		GL_Flush2D();

	vec3_t base_translate, base_old_translate;

	for (int x = 0; x < 2; ++x)
		base_translate[x] = script->translate[x] + f->translate[x];

	if (f->lerp)
	{
		for (int x = 0; x < 2; ++x)
			base_old_translate[x] = script->translate[x] + of->translate[x];
	}

	for (int i = 0; i < script->num_sprites; ++i)
	{
		if (!f->draws[i].draw)
			continue;

		int drawframe = f->draws[i].sprite_frame;
		model_t *sprite = MOD_ForHandle(script->sprites[i].model, CL_GetClientGame());
		mspritedirframe_t *spriteframe = &sprite->spritedirframes[drawframe % sprite->numframes].directions[0];

		if (invul)
			tess.flags |= 4;

		vec3_t translate, old_translate;

		/*if (!of->draws[i].draw)
		{*/
			for (int x = 0; x < 2; ++x)
				translate[x] = base_translate[x] + f->draws[i].translate[x];

			if (f->lerp)
			{
				for (int x = 0; x < 2; ++x)
					old_translate[x] = base_old_translate[x] + of->draws[i].translate[x];

				LerpVector(old_translate, translate, frac, translate);
			}
		/*}
		else
		{
			for (int x = 0; x < 2; ++x)
				translate[x] = LerpFloat(of->draws[i].translate[x], f->draws[i].translate[x], frac);
		}*/
		
		GL_StretchPic(sx + (translate[0] + -spriteframe->origin_x) * view_scale + view_x, sy + (translate[1] + -spriteframe->origin_y) * view_scale + view_y - (height_diff * scale), spriteframe->width * view_scale, spriteframe->height * view_scale, spriteframe->image->sl, spriteframe->image->tl, spriteframe->image->sh, spriteframe->image->th, draw.colors[0].u32, spriteframe->image);
	}

	if (invul)
		tess.flags |= 4;

	GL_Flush2D();

	R_SetColor(colorTable[COLOR_WHITE]);
}

static int MOD_LoadWSC(model_t *model, const void *rawdata, size_t length)
{
	model->type = MOD_WEAPONSCRIPT;

	const char *file = (const char *)rawdata;
	char *token;

	if ((token = COM_Parse(&file))[0] != '{')
		return Q_ERR_INVALID_FORMAT;

	// this holds the temporary script that we set up while
	// we're building it. swap it for real script
	// once we allocate it.
	static mweaponscript_t temp_script;
	memset(&temp_script, 0, sizeof(temp_script));
	mweaponscript_t *script_ptr = &temp_script;

	int cur_sprite = 0;

	while ((token = COM_Parse(&file))[0] != '}')
	{
		if (!Q_stricmp(token, "view"))
		{
			token = COM_Parse(&file);
			sscanf(token, "%f %f", &script_ptr->view[0], &script_ptr->view[1]);
		}
		else if (!Q_stricmp(token, "num_frames"))
		{
			token = COM_Parse(&file);
			script_ptr->num_frames = atoi(token);
		}
		else if (!Q_stricmp(token, "num_sprites"))
		{
			token = COM_Parse(&file);
			script_ptr->num_sprites = atoi(token);
		}
		else if (!Q_stricmp(token, "load_sprite"))
		{
			token = COM_Parse(&file);
			script_ptr->sprites[cur_sprite++].model = R_RegisterModel(token);
		}
		else if (!Q_stricmpn(token, "frame_", 6))
		{
			int frame_id = atoi(token + 6);

			mweaponscript_frame_t *frame = &script_ptr->frames[frame_id];

			if ((token = COM_Parse(&file))[0] != '{')
				return Q_ERR_INVALID_FORMAT;

			while ((token = COM_Parse(&file))[0] != '}')
			{
				if (!Q_stricmp(token, "translate"))
				{
					token = COM_Parse(&file);
					sscanf(token, "%f %f", &frame->translate[0], &frame->translate[1]);
				}
				else if (!Q_stricmp(token, "draw_sprite"))
				{
					token = COM_Parse(&file);
					int sprite_id = atoi(token);

					token = COM_Parse(&file);
					int sprite_frame_id = atoi(token);

					frame->draws[sprite_id].draw = true;
					frame->draws[sprite_id].sprite_frame = sprite_frame_id;
				}
				else if (!Q_stricmp(token, "draw_sprite_ofs"))
				{
					token = COM_Parse(&file);
					int sprite_id = atoi(token);

					token = COM_Parse(&file);
					int sprite_frame_id = atoi(token);

					frame->draws[sprite_id].draw = true;
					frame->draws[sprite_id].sprite_frame = sprite_frame_id;

					token = COM_Parse(&file);
					sscanf(token, "%f %f", &frame->draws[sprite_id].translate[0], &frame->draws[sprite_id].translate[1]);
				}
				else if (!Q_stricmp(token, "lerp"))
				{
					token = COM_Parse(&file);

					if (token[0] == '1' || Q_stricmp(token, "true") == 0)
						frame->lerp = true;
				}
			}
		}
		else if (!Q_stricmp(token, "translate"))
		{
			token = COM_Parse(&file);
			sscanf(token, "%f %f", &script_ptr->translate[0], &script_ptr->translate[1]);
		}

		if (script_ptr->num_frames && script_ptr->num_sprites && !model->weaponscript)
		{
			Hunk_Begin(&model->hunk, 0x10000);
			//model->size = sizeof(mweaponscript_t) + (sizeof(mweaponscript_sprite_t) * script_ptr->num_sprites) + (sizeof(mweaponscript_frame_t) * script_ptr->num_frames) + (sizeof(mweaponscript_frame_draw_t) * (script_ptr->num_frames * script_ptr->num_sprites));

			model->weaponscript = MOD_Malloc(sizeof(mweaponscript_t));
			memcpy(model->weaponscript, &temp_script, sizeof(mweaponscript_t));

			script_ptr = model->weaponscript;

			script_ptr->sprites = MOD_Malloc(sizeof(mweaponscript_sprite_t) * script_ptr->num_sprites);
			script_ptr->frames = MOD_Malloc(sizeof(mweaponscript_frame_t) * script_ptr->num_frames);

			for (int i = 0; i < script_ptr->num_frames; ++i)
				script_ptr->frames[i].draws = MOD_Malloc(sizeof(mweaponscript_frame_draw_t) * script_ptr->num_sprites);

			Hunk_End(&model->hunk);
		}
	}

	return Q_ERR_SUCCESS;
}


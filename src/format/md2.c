#include "shared/shared.h"
#include "format/md2.h"
#include "common/error.h"
#include <malloc.h>

static int MD2_Validate(dmd2header_t *header, size_t length)
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

	if (header->num_st > MD2_MAX_VERTS)
		return Q_ERR_TOO_MANY;

	end = header->ofs_st + sizeof(dmd2stvert_t) * header->num_st;

	if (header->ofs_st < sizeof(header) || end < header->ofs_st || end > length)
		return Q_ERR_BAD_EXTENT;

	// check xyz and frames
	if (header->num_xyz < 3)
		return Q_ERR_TOO_FEW;

	if (header->num_xyz > MD2_MAX_VERTS)
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
		if (header->num_skins > MD2_MAX_SKINS)
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

static int MD2_EnsurePosition(qhandle_t f, int64_t pos)
{
	if (FS_Tell(f) == pos)
		return Q_ERR_SUCCESS;

	return FS_Seek(f, pos);
}

// Initiates load of MD2, fills structure or returns error.
// Header is used in subsequent functions for loading pieces.
int MD2_Load(qhandle_t f, dmd2_t *md2, dmd2loadflags_t load_flags, memtag_t tag)
{
	int ret;

	if (!f)
		return Q_ERR_INVAL;

	const int64_t md2_size = FS_Length(f);

	if (md2_size < sizeof(md2->header))
		return md2_size < 0 ? md2_size : Q_ERR_INVALID_FORMAT;

	// make sure we're at the header position
	if ((ret = MD2_EnsurePosition(f, 0)) != Q_ERR_SUCCESS)
		return ret;

	FS_Read(&md2->header, sizeof(md2->header), f);

	ret = MD2_Validate(&md2->header, md2_size);

	if (ret != Q_ERR_SUCCESS)
		return ret;

	// calculate required memory
	size_t max_memory = 0;

	if (load_flags & MD2_LOAD_SKINS)
		max_memory += (MD2_MAX_SKINNAME + 1) * md2->header.num_skins;
	if (load_flags & MD2_LOAD_ST)
		max_memory += sizeof(dmd2stvert_t) * md2->header.num_st;
	if (load_flags & MD2_LOAD_TRIS)
		max_memory += sizeof(dmd2triangle_t) * md2->header.num_tris;
	if (load_flags & (MD2_LOAD_FRAMES | MD2_LOAD_FRAMEDATA))
	{
		if (!(load_flags & MD2_LOAD_FRAMEDATA))
			md2->framesize = sizeof(dmd2frame_t);
		else
			md2->framesize = md2->header.framesize;

		max_memory += md2->framesize * md2->header.num_frames;
	}

	Z_TagChunkCreate(tag, &md2->memory, max_memory);

	// load content
	if (load_flags & MD2_LOAD_SKINS)
	{
		if ((ret = MD2_EnsurePosition(f, md2->header.ofs_skins)) != Q_ERR_SUCCESS)
			return ret;

		md2->skins = Z_ChunkAlloc(&md2->memory, (MD2_MAX_SKINNAME + 1) * md2->header.num_skins);

		for (int i = 0; i < md2->header.num_skins; i++)
		{
			if (FS_Read((char*)MD2_GetSkin(md2, i), MD2_MAX_SKINNAME, f) != MD2_MAX_SKINNAME)
				return Q_ERR_INVALID_FORMAT;
		}
	}

	if (load_flags & MD2_LOAD_ST)
	{
		if ((ret = MD2_EnsurePosition(f, md2->header.ofs_st)) != Q_ERR_SUCCESS)
			return ret;

		size_t st_size = sizeof(dmd2stvert_t) * md2->header.num_st;

		md2->texcoords = Z_ChunkAlloc(&md2->memory, st_size);

		if (FS_Read(md2->texcoords, st_size, f) != st_size)
			return Q_ERR_INVALID_FORMAT;
	}

	if (load_flags & MD2_LOAD_TRIS)
	{
		if ((ret = MD2_EnsurePosition(f, md2->header.ofs_tris)) != Q_ERR_SUCCESS)
			return ret;

		size_t tris_size = sizeof(dmd2triangle_t) * md2->header.num_tris;

		md2->triangles = Z_ChunkAlloc(&md2->memory, tris_size);

		if (FS_Read(md2->triangles, tris_size, f) != tris_size)
			return Q_ERR_INVALID_FORMAT;
	}

	if (load_flags & (MD2_LOAD_FRAMES | MD2_LOAD_FRAMEDATA))
	{
		if ((ret = MD2_EnsurePosition(f, md2->header.ofs_frames)) != Q_ERR_SUCCESS)
			return ret;

		size_t frames_size = md2->framesize * md2->header.num_frames;

		md2->frames = Z_ChunkAlloc(&md2->memory, frames_size);

		// load the whole thing if we request the data
		if (load_flags & MD2_LOAD_FRAMEDATA)
		{
			if (FS_Read(md2->frames, frames_size, f) != frames_size)
				return Q_ERR_INVALID_FORMAT;
		}
		// step discreetly with a buffer
		else
		{
			for (int i = 0; i < md2->header.num_frames; i++)
			{
				if ((ret = MD2_EnsurePosition(f, md2->header.ofs_frames + (md2->header.framesize * i))) != Q_ERR_SUCCESS)
					return ret;

				if (FS_Read((dmd2frame_t *)MD2_GetFrame(md2, i), sizeof(dmd2frame_t), f) != sizeof(dmd2frame_t))
					return Q_ERR_INVALID_FORMAT;
			}
		}
	}

	return Q_ERR_SUCCESS;
}

// convenience
const char *MD2_GetSkin(const dmd2_t *md2, size_t index)
{
	if (index >= md2->header.num_skins)
		return NULL;

	return md2->skins + (index * (MD2_MAX_SKINNAME + 1));
}

const dmd2frame_t *MD2_GetFrame(const dmd2_t *md2, size_t index)
{
	if (index >= md2->header.num_frames)
		return NULL;

	return (const dmd2frame_t *)(((byte *)md2->frames) + (md2->framesize * index));
}

void MD2_Free(dmd2_t *md2)
{
	Z_ChunkFree(&md2->memory);
}

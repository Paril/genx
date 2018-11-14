/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#ifndef FORMAT_MDX_H
#define FORMAT_MDX_H

#include "md2.h"

/*
========================================================================

.MDX triangle model file format

========================================================================
*/

#define MDX_IDENT       (('X'<<24)+('P'<<16)+('D'<<8)+'I')
#define MDX_VERSION     4

#define MDX_MAX_TRIANGLES   MD2_MAX_TRIANGLES
#define MDX_MAX_VERTS       MD2_MAX_VERTS
#define MDX_MAX_FRAMES      1024
#define MDX_MAX_SKINS       MD2_MAX_SKINS
#define MDX_MAX_SKINNAME    MD2_MAX_SKINNAME
#define MDX_MAX_SUBOBJECTS	32 // just guessing

typedef dmd2triangle_t dmdxtriangle_t;
typedef dmd2frame_t dmdxframe_t;
typedef dmd2trivertx_t dmdxtrivertx_t;

#define MDX_MAX_FRAMESIZE	MD2_MAX_FRAMESIZE

typedef struct {
   maliastc_t st;
   int vertexIndex;
} dmdxglcmd_t;

typedef struct {
   int num_verts;
   int sub_object;
} dmdxglcmds_t;

typedef struct dmdxheader_s {
    uint32_t        ident;
    uint32_t        version;

    uint32_t        skinwidth;
    uint32_t        skinheight;
    uint32_t        framesize;      // byte size of each frame

    uint32_t        num_skins;
    uint32_t        num_xyz;
    uint32_t        num_tris;
    uint32_t        num_glcmds;
    uint32_t        num_frames;
	uint32_t		num_sfx_defines;
	uint32_t		num_sfx_entries;
	uint32_t		num_sub_objects;

    uint32_t        ofs_skins;      // each skin is a MAX_SKINNAME string
    uint32_t        ofs_tris;       // offset for dtriangles
    uint32_t        ofs_frames;     // offset for first frame
    uint32_t        ofs_glcmds;
    uint32_t        ofs_vertex_info;
    uint32_t        ofs_sfx_defines;
    uint32_t        ofs_sfx_entries;
    uint32_t        ofs_bbox_frames;
    uint32_t        ofs_unknown;
    uint32_t        ofs_end;        // end of file
} dmdxheader_t;

#endif // FORMAT_MDX_H

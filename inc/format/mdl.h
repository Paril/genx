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

#ifndef FORMAT_MDL_H
#define FORMAT_MDL_H

/*
========================================================================

.MDL triangle model file format

========================================================================
*/

#define MDL_IDENT       (('O'<<24)+('P'<<16)+('D'<<8)+'I')
#define MDL_VERSION     6

#define MDL_MAX_TRIANGLES   2048
#define MDL_MAX_VERTS       1024
#define MDL_MAX_FRAMES      256

/* Compressed vertex */
typedef struct
{
	unsigned char v[3];
	unsigned char normalIndex;
} dmdlvertex_t;

/* Simple frame */
typedef struct
{
	dmdlvertex_t bboxmin; /* bouding box min */
	dmdlvertex_t bboxmax; /* bouding box max */
	char name[16];
} dmdlsimpleframe_t;

typedef struct
{
	int			num_frames;
	dmdlvertex_t bboxmin; /* bouding box min */
	dmdlvertex_t bboxmax; /* bouding box max */
} dmdlgroupframe_t;

/* Model frame */
typedef struct
{
	int type;                        /* 0 = simple, !0 = group */
} dmdlframe_t;

/* Triangle info */
typedef struct
{
	int facesfront;  /* 0 = backface, 1 = frontface */
	int vertex[3];   /* vertex indices */
} dmdltriangle_t;

/* Texture coords */
typedef struct
{
	int onseam;
	int s;
	int t;
} dmdltexcoord_t;

/* Skin */
typedef struct
{
	int group;      /* 0 = single, 1 = group */
} dmdlskin_t;

/* MDL header */
typedef struct
{
	int ident;            /* magic number: "IDPO" */
	int version;          /* version: 6 */

	vec3_t scale;         /* scale factor */
	vec3_t translate;     /* translation vector */
	float boundingradius;
	vec3_t eyeposition;   /* eyes' position */

	int num_skins;        /* number of textures */
	int skinwidth;        /* texture width */
	int skinheight;       /* texture height */

	int num_verts;        /* number of vertices */
	int num_tris;         /* number of triangles */
	int num_frames;       /* number of frames */

	int synctype;         /* 0 = synchron, 1 = random */
	int flags;            /* state flag */
	float size;
} dmdlheader_t;

// Q1M
#define Q1M_IDENT       (('M'<<24)+('B'<<16)+('1'<<8)+'Q')

#endif
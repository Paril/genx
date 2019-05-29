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

#ifndef FORMAT_SPR_H
#define FORMAT_SPR_H

/*
========================================================================

.SPR sprite file format

========================================================================
*/

#define SPR_IDENT       (('P'<<24)+('S'<<16)+('D'<<8)+'I')
#define SPR_VERSION     1

typedef struct
{
	uint32_t offset_x;
	uint32_t offset_y;
	uint32_t width;
	uint32_t height;
} dsprpicture_t;

typedef struct
{
	uint32_t group;
} dsprframe_t;

typedef struct
{
	uint32_t    ident;
	uint32_t    version;
	uint32_t    type;
	float		radius;
	uint32_t	maxwidth;
	uint32_t	maxheight;
	uint32_t	numframes;
	float		beamlength;
	uint32_t	synctype;
} dsprheader_t;

#endif // FORMAT_SPR_H

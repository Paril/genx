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

#ifndef VIDEO_H
#define VIDEO_H

extern cvar_t       *vid_ref;
extern cvar_t       *vid_geometry;
extern cvar_t       *vid_mode;

//
// vid_*.c
//
void VID_PumpEvents(void);
void VID_SetMode(void);

bool    VID_Init(void);
void    VID_Shutdown(void);
void	VID_FatalShutdown(void);

void    VID_UpdateGamma(const byte *table);

void    VID_BeginFrame(void);
void    VID_EndFrame(void);

char    *VID_GetClipboardData(void);
void    VID_SetClipboardData(const char *data);

//
// cl_ref.c
//
bool VID_GetGeometry(vrect_t *rc);
void VID_SetGeometry(vrect_t *rc);
void VID_ToggleFullscreen(void);

// Generations
// Video palettes
extern uint32_t d_palettes[GAME_TOTAL][256];

#endif // VIDEO_H

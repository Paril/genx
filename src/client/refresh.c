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

// Main windowed and fullscreen graphics interface module. This module
// is used for both the software and OpenGL rendering versions of the
// Quake refresh engine.


#include "client.h"

// Console variables that we need to access from this module
cvar_t      *vid_ref;           // Name of Refresh DLL loaded
cvar_t      *vid_geometry;
cvar_t      *vid_mode;

static bool	mode_changed;

/*
==========================================================================

HELPER FUNCTIONS

==========================================================================
*/

// 640x480
// 640x480+0
// 640x480+0+0
// 640x480-100-100
bool VID_GetGeometry(vrect_t *rc)
{
	unsigned long w, h;
	long x, y;
	char *s;
	// fill in default parameters
	rc->x = 0;
	rc->y = 0;
	rc->width = 640;
	rc->height = 480;

	if (!vid_geometry || vid_mode->integer != VM_WINDOWED)
		return false;

	s = vid_geometry->string;

	if (!*s)
		return false;

	w = strtoul(s, &s, 10);

	if (*s != 'x' && *s != 'X')
	{
		Com_DPrintf("Geometry string is malformed\n");
		return false;
	}

	h = strtoul(s + 1, &s, 10);
	x = y = 0;

	if (*s == '+' || *s == '-')
	{
		x = strtol(s, &s, 10);

		if (*s == '+' || *s == '-')
			y = strtol(s, &s, 10);
	}

	// sanity check
	if (w < 64 || w > 8192 || h < 64 || h > 8192)
	{
		Com_DPrintf("Geometry %lux%lu doesn't look sane\n", w, h);
		return false;
	}

	rc->x = x;
	rc->y = y;
	rc->width = w;
	rc->height = h;
	return true;
}

void VID_SetGeometry(vrect_t *rc)
{
	char buffer[MAX_QPATH];

	if (!vid_geometry)
		return;

	Q_snprintf(buffer, sizeof(buffer), "%dx%d%+d%+d",
		rc->width, rc->height, rc->x, rc->y);
	Cvar_SetByVar(vid_geometry, buffer, FROM_CODE);
}

void VID_ToggleFullscreen(void)
{
	if (!vid_mode)
		return;

	if (vid_mode->integer != VM_FULLSCREEN)
		Cbuf_AddText(&cmd_buffer, "set vid_mode 1\n");
	else
		Cbuf_AddText(&cmd_buffer, "set vid_mode 0\n");
}

/*
==========================================================================

LOADING / SHUTDOWN

==========================================================================
*/

/*
============
CL_RunResfresh
============
*/
void CL_RunRefresh(void)
{
	if (!cls.ref_initialized)
		return;

	VID_PumpEvents();

	if (mode_changed)
	{
		VID_SetMode();
		mode_changed = false;
	}

	if (cvar_modified & CVAR_REFRESH)
	{
		CL_RestartRefresh(true);
		cvar_modified &= ~CVAR_REFRESH;
	}
	else if (cvar_modified & CVAR_FILES)
	{
		CL_RestartRefresh(false);
		cvar_modified &= ~CVAR_FILES;
	}
}

static void vid_mode_changed(cvar_t *self)
{
	mode_changed = true;
}

/*
============
CL_InitRefresh
============
*/
void CL_InitRefresh(void)
{
	if (cls.ref_initialized)
		return;

	Com_SetLastError(NULL);
	// Create the video variables so we know how to start the graphics drivers
	vid_ref = Cvar_Get("vid_ref", VID_REF, CVAR_ROM);
	vid_mode = Cvar_Get("vid_mode", "0", CVAR_ARCHIVE);
	vid_geometry = Cvar_Get("vid_geometry", VID_GEOMETRY, CVAR_ARCHIVE);
	Com_SetLastError(NULL);

	if (!R_Init(true))
		Com_Error(ERR_FATAL, "Couldn't initialize refresh: %s", Com_GetLastError());

	cls.ref_initialized = true;
	vid_geometry->changed = vid_mode_changed;
	vid_mode->changed = vid_mode_changed;
	mode_changed = false;
	// Initialize the rest of graphics subsystems
	V_Init();
	SCR_Init();
	UI_Init();
	SCR_RegisterMedia();
	Con_RegisterMedia();
	cvar_modified &= ~(CVAR_FILES | CVAR_REFRESH);
}

/*
============
CL_ShutdownRefresh
============
*/
void CL_ShutdownRefresh(void)
{
	if (!cls.ref_initialized)
		return;

	// Shutdown the rest of graphics subsystems
	V_Shutdown();
	SCR_Shutdown();
	UI_Shutdown();
	vid_geometry->changed = NULL;
	vid_mode->changed = NULL;
	R_Shutdown(true);
	cls.ref_initialized = false;
	// no longer active
	cls.active = ACT_MINIMIZED;
	Z_LeakTest(TAG_RENDERER);
}

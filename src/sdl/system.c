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

#include "shared/shared.h"
#include "common/common.h"
#include "system/system.h"
#include <SDL2/SDL.h>

/*
========================================================================

DLL LOADING

========================================================================
*/

void Sys_FreeLibrary(void *handle)
{
	if (handle)
		SDL_UnloadObject(handle);
}

void *Sys_LoadLibrary(const char *path, const char *sym, void **handle)
{
	void *module;
	void *entry;
	*handle = NULL;
	module = SDL_LoadObject(path);

	if (!module)
	{
		Com_SetLastError(va("%s: LoadLibrary failed with error %lu",
				path, SDL_GetError()));
		return NULL;
	}

	if (sym)
	{
		entry = Sys_GetProcAddress(module, sym);

		if (!entry)
		{
			Sys_FreeLibrary(module);
			return NULL;
		}
	}
	else
		entry = NULL;

	*handle = module;
	return entry;
}

void *Sys_GetProcAddress(void *handle, const char *sym)
{
	void *entry;
	entry = SDL_LoadFunction(handle, sym);

	if (!entry)
		Com_SetLastError(va("GetProcAddress(%s) failed with error %lu",
				sym, SDL_GetError()));

	return entry;
}

void Sys_DebugBreak(void)
{
	SDL_TriggerBreakpoint();
}

unsigned Sys_Milliseconds(void)
{
	return SDL_GetTicks();
}

void Sys_Sleep(unsigned msec)
{
	SDL_Delay(msec);
}
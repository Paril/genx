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
#include "common/zone.h"
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



/*
===============================================================================

ASYNC WORK QUEUE

===============================================================================
*/

static bool work_initialized;
static bool work_terminate;
static SDL_mutex *work_lock;
static SDL_cond *work_cond;
static SDL_Thread *work_thread;
static asyncwork_t *pend_head;
static asyncwork_t *done_head;

static void append_work(asyncwork_t **head, asyncwork_t *work)
{
	asyncwork_t *c, **p;

	for (p = head, c = *head; c; p = &c->next, c = c->next);

	work->next = NULL;
	*p = work;
}

static void complete_work(void)
{
	asyncwork_t *work, *next;

	if (!work_initialized)
		return;

	if (SDL_TryLockMutex(work_lock))
		return;

	if (q_unlikely(done_head))
	{
		for (work = done_head; work; work = next)
		{
			next = work->next;

			if (work->done_cb)
				work->done_cb(work->cb_arg);

			Z_Free(work);
		}

		done_head = NULL;
	}

	SDL_UnlockMutex(work_lock);
}

static int thread_func(void *arg)
{
	SDL_LockMutex(work_lock);

	while (1)
	{
		while (!pend_head && !work_terminate)
			SDL_CondWait(work_cond, work_lock);

		asyncwork_t *work = pend_head;

		if (!work)
			break;

		pend_head = work->next;
		SDL_UnlockMutex(work_lock);
		work->work_cb(work->cb_arg);
		SDL_LockMutex(work_lock);
		append_work(&done_head, work);
	}

	SDL_UnlockMutex(work_lock);
	return 0;
}

static void shutdown_work(void)
{
	if (!work_initialized)
		return;

	SDL_LockMutex(work_lock);
	work_terminate = true;
	SDL_CondSignal(work_cond);
	SDL_UnlockMutex(work_lock);
	SDL_WaitThread(work_thread, NULL);
	complete_work();
	SDL_DestroyMutex(work_lock);
	SDL_DestroyCond(work_cond);
	work_initialized = false;
}

void Sys_QueueAsyncWork(asyncwork_t *work)
{
	if (!work_initialized)
	{
		work_lock = SDL_CreateMutex();

		if (!work_lock)
			Sys_Error("Couldn't create async work mutex: %s", SDL_GetError());

		work_cond = SDL_CreateCond();

		if (!work_cond)
			Sys_Error("Couldn't create async work cond: %s", SDL_GetError());

		work_thread = SDL_CreateThread(thread_func, NULL, NULL);

		if (!work_thread)
			Sys_Error("Couldn't create async work thread: %s", SDL_GetError());

		work_initialized = true;
	}

	SDL_LockMutex(work_lock);
	append_work(&pend_head, Z_CopyStruct(work));
	SDL_CondSignal(work_cond);
	SDL_UnlockMutex(work_lock);
}

void Sys_CompleteAsyncWork(void)
{
	complete_work();
}

void Sys_ShutdownAsyncWork(void)
{
	shutdown_work();
}
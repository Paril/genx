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

#ifndef FILES_H
#define FILES_H

#include "common/cmd.h"
#include "common/error.h"
#include "common/zone.h"

typedef struct file_info_s
{
	int64_t size;
	time_t  ctime;
	time_t  mtime;
	char    name[1];
} file_info_t;

#define FS_Malloc(size)         Z_TagMalloc(size, TAG_FILESYSTEM)
#define FS_Mallocz(size)        Z_TagMallocz(size, TAG_FILESYSTEM)
#define FS_CopyString(string)   Z_TagCopyString(string, TAG_FILESYSTEM)

// just regular malloc for now
#define FS_AllocTempMem(size)   FS_Malloc(size)
#define FS_FreeTempMem(buf)     Z_Free(buf)

// just regular caseless string comparsion
#define FS_pathcmp      Q_strcasecmp
#define FS_pathcmpn     Q_strncasecmp

#define FS_HashPath(s, size)            Com_HashStringLen(s, SIZE_MAX, size)
#define FS_HashPathLen(s, len, size)    Com_HashStringLen(s, len, size)

void    FS_Init(void);
void    FS_Shutdown(void);
void    FS_Restart(bool total);

#if USE_CLIENT
	int FS_RenameFile(const char *from, const char *to);
#endif

int FS_CreatePath(char *path);

qhandle_t FS_EasyOpenFile(char *buf, size_t size, unsigned mode,
	const char *dir, const char *name, const char *ext);

int FS_WriteFile(const char *path, const void *data, size_t len);

bool FS_EasyWriteFile(char *buf, size_t size, unsigned mode,
	const char *dir, const char *name, const char *ext,
	const void *data, size_t len);

int FS_ReadLine(qhandle_t f, char *buffer, size_t size);

void    FS_Flush(qhandle_t f);

bool FS_WildCmp(const char *filter, const char *string);
bool FS_ExtCmp(const char *extension, const char *string);

#define FS_ReallocList(list, count) \
	Z_Realloc(list, ALIGN(count, MIN_LISTED_FILES) * sizeof(void *))

void    **FS_ListFiles(const char *path, const char *filter, unsigned flags, int *count_p);
void    **FS_CopyList(void **list, int count);
file_info_t *FS_CopyInfo(const char *name, int64_t size, time_t ctime, time_t mtime);
void    FS_FreeList(void **list);

size_t FS_NormalizePath(char *out, const char *in);
size_t FS_NormalizePathBuffer(char *out, const char *in, size_t size);

#define PATH_INVALID        0
#define PATH_VALID          1
#define PATH_MIXED_CASE     2

int FS_ValidatePath(const char *s);
void FS_CleanupPath(char *s);

void FS_SanitizeFilenameVariable(cvar_t *var);

#ifdef _WIN32
	char *FS_ReplaceSeparators(char *s, int separator);
#endif

void FS_File_g(const char *path, const char *ext, unsigned flags, genctx_t *ctx);

FILE *Q_fopen(const char *path, const char *mode);

extern cvar_t   *fs_game;

extern char     fs_gamedir[];

#endif // FILES_H

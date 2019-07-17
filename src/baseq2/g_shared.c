#include "g_local.h"

// PRINTS
void Com_LPrintf(print_type_t type, const char *fmt, ...)
{
	va_list     argptr;
	char        text[MAX_STRING_CHARS];

	if (type == PRINT_DEVELOPER)
		return;

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);
	gi.dprintf(type, "%s", text);
}

void Com_Error(error_type_t type, const char *fmt, ...)
{
	va_list     argptr;
	char        text[MAX_STRING_CHARS];
	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);
	gi.error(type, "%s", text);
}

// MSG
void    MSG_WriteChar(int c)
{
	gi.MSG_WriteChar(c);
}

void    MSG_WriteByte(int c)
{
	gi.MSG_WriteByte(c);
}

void    MSG_WriteShort(int c)
{
	gi.MSG_WriteShort(c);
}

void    MSG_WriteUShort(int c)
{
	gi.MSG_WriteUShort(c);
}

void    MSG_WriteLong(int c)
{
	gi.MSG_WriteLong(c);
}

void    MSG_WriteString(const char *s)
{
	gi.MSG_WriteString(s);
}

void    MSG_WritePos(const vec3_t pos)
{
	gi.MSG_WritePos(pos);
}

void    MSG_WriteAngle(float f)
{
	gi.MSG_WriteAngle(f);
}

void    MSG_WriteDir(const vec3_t vector)
{
	gi.MSG_WriteDir(vector);
}

// CMD
int     Cmd_Argc(void)
{
	return gi.Cmd_Argc();
}

char *Cmd_Argv(int arg)
{
	return gi.Cmd_Argv(arg);
}

char *Cmd_Args(void)
{
	return gi.Cmd_Args();
}

// CVAR
cvar_t *Cvar_Get(const char *var_name, const char *value, int flags)
{
	return gi.Cvar_Get(var_name, value, flags);
}

cvar_t *Cvar_Set(const char *var_name, const char *value)
{
	return gi.Cvar_Set(var_name, value);
}

cvar_t *Cvar_UserSet(const char *var_name, const char *value)
{
	return gi.Cvar_UserSet(var_name, value);
}

// FILES
int64_t FS_FOpenFile(const char *filename, qhandle_t *f, unsigned mode)
{
	return gi.FS_FOpenFile(filename, f, mode);
}

int64_t	FS_Tell(qhandle_t f)
{
	return gi.FS_Tell(f);
}

int		FS_Seek(qhandle_t f, int64_t offset)
{
	return gi.FS_Seek(f, offset);
}

int64_t FS_Length(qhandle_t f)
{
	return gi.FS_Length(f);
}

int		FS_Read(void *buffer, size_t len, qhandle_t f)
{
	return gi.FS_Read(buffer, len, f);
}

int		FS_Write(const void *buffer, size_t len, qhandle_t f)
{
	return gi.FS_Write(buffer, len, f);
}

int		FS_FPrintf(qhandle_t f, const char *format, ...) q_printf(2, 3)
{
	va_list     argptr;
	va_start(argptr, format);
	int ret = gi.FS_VPrintf(f, format, argptr);
	va_end(argptr);
	return ret;
}

int		FS_VPrintf(qhandle_t f, const char *format, va_list args)
{
	return gi.FS_VPrintf(f, format, args);
}

int		FS_LoadFileEx(const char *path, void **buffer, unsigned flags, memtag_t tag)
{
	return gi.FS_LoadFileEx(path, buffer, flags, tag);
}

int     FS_FCloseFile(qhandle_t f)
{
	return gi.FS_FCloseFile(f);
}

int FS_LoadFile(const char *path, void **buf)
{
	return FS_LoadFileEx(path, buf, 0, TAG_GAME);
}

bool FS_FileExistsEx(const char *path, unsigned flags)
{
	return FS_LoadFileEx(path, NULL, flags, TAG_FREE) != Q_ERR_NOENT;
}

// MEMORY
void    Z_Free(void *ptr)
{
	gi.Z_Free(ptr);
}

void *Z_Realloc(void *ptr, size_t size)
{
	return gi.Z_Realloc(ptr, size);
}

void *Z_TagMalloc(size_t size, memtag_t tag) q_malloc
{
	return gi.Z_TagMalloc(size, tag);
}

void *Z_TagMallocz(size_t size, memtag_t tag) q_malloc
{
	return gi.Z_TagMallocz(size, tag);
}

void    Z_FreeTags(memtag_t tag)
{
	gi.Z_FreeTags(tag);
}
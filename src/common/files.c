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

#define _GNU_SOURCE
#include "shared/shared.h"
#include "shared/list.h"
#include "common/common.h"
#include "common/cvar.h"
#include "common/error.h"
#include "common/files.h"
#include "common/prompt.h"
#include "system/system.h"
#include "client/client.h"
#include "format/pak.h"

#include <fcntl.h>

#if USE_ZLIB
	#include <zlib.h>
#endif

/*
=============================================================================

QUAKE FILESYSTEM

- transparently merged from several sources
- relative to the single virtual root
- case insensitive at pakfiles level,
  but may be case sensitive at host OS level
- uses / as path separators internally

=============================================================================
*/

#define MAX_FILE_HANDLES    32

#if USE_ZLIB
	#define ZIP_MAXFILES    0x8000  // 32k files
	#define ZIP_BUFSIZE     0x10000 // inflate in blocks of 64k

	#define ZIP_BUFREADCOMMENT      1024
	#define ZIP_SIZELOCALHEADER     30
	#define ZIP_SIZECENTRALHEADER   20
	#define ZIP_SIZECENTRALDIRITEM  46

	#define ZIP_LOCALHEADERMAGIC    0x04034b50
	#define ZIP_CENTRALHEADERMAGIC  0x02014b50
	#define ZIP_ENDHEADERMAGIC      0x06054b50
#endif

#ifdef _DEBUG
#define FS_DPrintf(...) \
	if (fs_debug && fs_debug->integer) \
		Com_LPrintf(PRINT_DEVELOPER, __VA_ARGS__)
#else
#define FS_DPrintf(...)
#endif

#define FS_ERR_READ(fp) \
	(ferror(fp) ? Q_ERR_FAILURE : Q_ERR_UNEXPECTED_EOF)

#define PATH_NOT_CHECKED    -1

// in memory
//

typedef enum
{
	FS_FREE,
	FS_REAL,
	FS_PAK,
#if USE_ZLIB
	FS_ZIP,
	FS_GZ,
#endif
	FS_BAD
} filetype_t;

#if USE_ZLIB
typedef struct
{
	z_stream    stream;
	unsigned    rest_in;
	byte        buffer[ZIP_BUFSIZE];
} zipstream_t;
#endif

typedef struct packfile_s
{
	char        *name;
	size_t      namelen;
	size_t      filepos;
	size_t      filelen;
#if USE_ZLIB
	unsigned    complen;
	byte        compmtd;    // compression method, 0 (stored) or Z_DEFLATED
	bool        coherent;   // true if local file header has been checked
#endif
	struct packfile_s *hash_next;
} packfile_t;

typedef struct
{
	filetype_t  type;       // FS_PAK or FS_ZIP
	unsigned    refcount;   // for tracking pack users
	FILE        *fp;
	unsigned    num_files;
	unsigned    hash_size;
	packfile_t  *files;
	packfile_t  **file_hash;
	char        *names;
	char        *filename;
} pack_t;

typedef struct searchpath_s
{
	struct searchpath_s *next;
	unsigned    mode;
	pack_t      *pack;        // only one of filename / pack will be used
	char        filename[1];
} searchpath_t;

typedef struct
{
	filetype_t  type;
	unsigned    mode;
	FILE        *fp;
#if USE_ZLIB
	void        *zfp;       // gzFile for FS_GZ or zipstream_t for FS_ZIP
#endif
	packfile_t  *entry;     // pack entry this handle is tied to
	pack_t      *pack;      // points to the pack entry is from
	bool        unique;     // if true, then pack must be freed on close
	int         error;      // stream error indicator from read/write operation
	size_t      rest_out;   // remaining unread length for FS_PAK/FS_ZIP
	int64_t     length;     // total cached file length
} file_t;

// these point to user home directory
char                fs_gamedir[MAX_OSPATH];
//static char       fs_basedir[MAX_OSPATH];

static searchpath_t *fs_searchpaths;
static searchpath_t *fs_base_searchpaths;

static file_t       fs_files[MAX_FILE_HANDLES];

#ifdef _DEBUG
	static cvar_t       *fs_debug;
#endif

cvar_t              *fs_game;

#if USE_ZLIB
	// local stream used for all file loads
	static zipstream_t  fs_zipstream;

	static void open_zip_file(file_t *file);
	static void close_zip_file(file_t *file);
	static int tell_zip_file(file_t *file);
	static int seek_zip_file(file_t *file, int64_t offset);
	static int read_zip_file(file_t *file, void *buf, size_t len);
#endif

// for tracking users of pack_t instance
// allows FS to be restarted while reading something from pack
static pack_t *pack_get(pack_t *pack);
static void pack_put(pack_t *pack);

/*

All of Quake's data access is through a hierchal file system,
but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding all game directories.
The base directory is only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that
all generated files (savegames, screenshots, config files) will be saved to.

*/

#ifdef _WIN32
char *FS_ReplaceSeparators(char *s, int separator)
{
	char *p;
	p = s;

	while (*p)
	{
		if (*p == '/' || *p == '\\')
			*p = separator;

		p++;
	}

	return s;
}
#endif

static inline bool validate_char(int c)
{
	if (!Q_isprint(c))
		return false;

#ifdef _WIN32

	if (strchr("<>:\"|?*", c))
		return false;

#endif
	return true;
}

/*
================
FS_ValidatePath

Checks for bad (OS specific) and mixed case characters in path.
================
*/
int FS_ValidatePath(const char *s)
{
	int res = PATH_VALID;

	for (; *s; s++)
	{
		if (!validate_char(*s))
			return PATH_INVALID;

		if (Q_isupper(*s))
			res = PATH_MIXED_CASE;
	}

	return res;
}

void FS_CleanupPath(char *s)
{
	for (; *s; s++)
	{
		if (!validate_char(*s))
			*s = '_';
	}
}

/*
================
FS_SanitizeFilenameVariable

Checks that console variable is a valid single filename (not a path), otherwise
resets it to default value, printing a warning.
================
*/
void FS_SanitizeFilenameVariable(cvar_t *var)
{
	if (!FS_ValidatePath(var->string))
	{
		Com_Printf("'%s' contains invalid characters for a filename.\n", var->name);
		goto reset;
	}

	if (strchr(var->string, '/') || strchr(var->string, '\\'))
	{
		Com_Printf("'%s' should be a single filename, not a path.\n", var->name);
		goto reset;
	}

	return;
reset:
	Com_Printf("...falling back to %s\n", var->default_string);
	Cvar_Reset(var);
}

/*
================
FS_NormalizePath

Simplifies the path, converting backslashes to slashes and removing ./ and ../
components, as well as duplicated slashes. Any leading slashes are also skipped.

May operate in place if in == out.

    ///foo       -> foo
    foo\bar      -> foo/bar
    foo/..       -> <empty>
    foo/../bar   -> bar
    foo/./bar    -> foo/bar
    foo//bar     -> foo/bar
    ./foo        -> foo
================
*/
size_t FS_NormalizePath(char *out, const char *in)
{
	char *start = out;
	uint32_t pre = '/';

	while (1)
	{
		int c = *in++;

		if (c == '/' || c == '\\' || c == 0)
		{
			if ((pre & 0xffffff) == (('/' << 16) | ('.' << 8) | '.'))
			{
				out -= 4;

				if (out < start)
				{
					// can't go past root
					out = start;

					if (c == 0)
						break;
				}
				else
				{
					while (out > start && *out != '/')
						out--;

					if (c == 0)
						break;

					if (out > start)
						// save the slash
						out++;
				}

				pre = '/';
				continue;
			}

			if ((pre & 0xffff) == (('/' << 8) | '.'))
			{
				// eat the dot
				out--;

				if (c == 0)
				{
					if (out > start)
						// eat the slash
						out--;

					break;
				}

				pre = '/';
				continue;
			}

			if ((pre & 0xff) == '/')
			{
				if (c == 0)
					break;

				continue;
			}

			if (c == 0)
				break;

			c = '/';
		}

		pre = (pre << 8) | c;
		*out++ = c;
	}

	*out = 0;
	return out - start;
}

/*
================
FS_NormalizePathBuffer

Buffer safe version of FS_NormalizePath. Return value >= size signifies
overflow, empty string is stored in output buffer in this case.
================
*/
size_t FS_NormalizePathBuffer(char *out, const char *in, size_t size)
{
	size_t len = strlen(in);

	if (len >= size)
	{
		if (size)
			*out = 0;

		return len;
	}

	return FS_NormalizePath(out, in);
}

// =============================================================================

static file_t *alloc_handle(qhandle_t *f)
{
	file_t *file;
	size_t i;

	for (i = 0, file = fs_files; i < MAX_FILE_HANDLES; i++, file++)
	{
		if (file->type != FS_FREE)
			continue;

		*f = (qhandle_t)(i + 1);
		return file;
	}

	return NULL;
}

static file_t *file_for_handle(qhandle_t f)
{
	file_t *file;

	if (!f || (size_t)f > MAX_FILE_HANDLES)
		return NULL;

	file = &fs_files[(size_t)f - 1];

	if (file->type == FS_FREE)
		return NULL;

	if (file->type < FS_FREE || file->type >= FS_BAD)
		Com_Error(ERR_FATAL, "%s: bad file type", __func__);

	return file;
}

/*
================
FS_Length
================
*/
int64_t FS_Length(qhandle_t f)
{
	file_t *file = file_for_handle(f);

	if (!file)
		return Q_ERR_BADF;

	if ((file->mode & FS_MODE_MASK) == FS_MODE_READ)
		return file->length;

	return Q_ERR_NOSYS;
}

/*
============
FS_Tell
============
*/
int64_t FS_Tell(qhandle_t f)
{
	file_t *file = file_for_handle(f);
	int64_t ret;

	if (!file)
		return Q_ERR_BADF;

	switch (file->type)
	{
		case FS_REAL:
			ret = os_ftell(file->fp);

			if (ret == -1)
				return Q_ERRNO;

			return ret;

		case FS_PAK:
			return file->length - file->rest_out;
#if USE_ZLIB

		case FS_ZIP:
			return tell_zip_file(file);

		case FS_GZ:
			ret = gztell(file->zfp);

			if (ret == -1)
				return Q_ERR_LIBRARY_ERROR;

			return ret;
#endif

		default:
			return Q_ERR_NOSYS;
	}
}

static int seek_pak_file(file_t *file, int64_t offset)
{
	packfile_t *entry = file->entry;

	if (offset > entry->filelen)
		offset = entry->filelen;

	if (os_fseek(file->fp, entry->filepos + offset, SEEK_SET) == -1)
		return Q_ERRNO;

	file->rest_out = entry->filelen - offset;
	return Q_ERR_SUCCESS;
}

/*
============
FS_Seek

Seeks to an absolute position within the file.
============
*/
int FS_Seek(qhandle_t f, int64_t offset)
{
	file_t *file = file_for_handle(f);

	if (!file)
		return Q_ERR_BADF;

	if (offset < 0)
		offset = 0;

	switch (file->type)
	{
		case FS_REAL:
			if (os_fseek(file->fp, offset, SEEK_SET) == -1)
				return Q_ERRNO;

			return Q_ERR_SUCCESS;

		case FS_PAK:
			return seek_pak_file(file, offset);
#if USE_ZLIB

		case FS_GZ:
			if (gzseek(file->zfp, offset, SEEK_SET) == -1)
				return Q_ERR_LIBRARY_ERROR;

			return Q_ERR_SUCCESS;

		case FS_ZIP:
			return seek_zip_file(file, offset);
#endif

		default:
			return Q_ERR_NOSYS;
	}
}

/*
============
FS_CreatePath

Creates any directories needed to store the given filename.
Expects a fully qualified, normalized system path (i.e. with / separators).
============
*/
int FS_CreatePath(char *path)
{
	char *ofs;
	int ret;
	ofs = path;
#ifdef _WIN32

	// check for UNC path and skip "//computer/share/" part
	if (*path == '/' && path[1] == '/')
	{
		char *p;
		p = strchr(path + 2, '/');

		if (p)
		{
			p = strchr(p + 1, '/');

			if (p)
				ofs = p + 1;
		}
	}

#endif

	// skip leading slash(es)
	for (; *ofs == '/'; ofs++)
		;

	for (; *ofs; ofs++)
	{
		if (*ofs == '/')
		{
			// create the directory
			*ofs = 0;
			ret = os_mkdir(path);
			*ofs = '/';

			if (ret == -1)
			{
				int err = Q_ERRNO;

				if (err != Q_ERR_EXIST)
					return err;
			}
		}
	}

	return Q_ERR_SUCCESS;
}

/*
==============
FS_FCloseFile
==============
*/
int FS_FCloseFile(qhandle_t f)
{
	file_t *file = file_for_handle(f);
	int ret;

	if (!file)
		return Q_ERR_BADF;

	ret = file->error;

	switch (file->type)
	{
		case FS_REAL:
			if (fclose(file->fp))
				ret = Q_ERRNO;

			break;

		case FS_PAK:
			if (file->unique)
			{
				fclose(file->fp);
				pack_put(file->pack);
			}

			break;
#if USE_ZLIB

		case FS_GZ:
			if (gzclose(file->zfp))
				ret = Q_ERR_LIBRARY_ERROR;

			break;

		case FS_ZIP:
			if (file->unique)
			{
				close_zip_file(file);
				pack_put(file->pack);
			}

			break;
#endif

		default:
			ret = Q_ERR_NOSYS;
			break;
	}

	memset(file, 0, sizeof(*file));
	return ret;
}

static int get_path_info(const char *path, file_info_t *info)
{
	Q_STATBUF st;

	if (os_stat(path, &st) == -1)
		return Q_ERRNO;

	if (Q_ISDIR(st.st_mode))
		return Q_ERR_ISDIR;

	if (!Q_ISREG(st.st_mode))
		return Q_ERR_FILE_NOT_REGULAR;

	if (info)
	{
		info->size = st.st_size;
		info->ctime = st.st_ctime;
		info->mtime = st.st_mtime;
	}

	return Q_ERR_SUCCESS;
}

static int get_fp_info(FILE *fp, file_info_t *info)
{
	Q_STATBUF st;
	int fd;
	fd = os_fileno(fp);

	if (fd == -1)
		return Q_ERRNO;

	if (os_fstat(fd, &st) == -1)
		return Q_ERRNO;

	if (Q_ISDIR(st.st_mode))
		return Q_ERR_ISDIR;

	if (!Q_ISREG(st.st_mode))
		return Q_ERR_FILE_NOT_REGULAR;

	if (info)
	{
		info->size = st.st_size;
		info->ctime = st.st_ctime;
		info->mtime = st.st_mtime;
	}

	return Q_ERR_SUCCESS;
}

FILE *Q_fopen(const char *path, const char *mode)
{
#ifdef _WIN32

	if (mode[0] == 'w' && mode[1] == 'x')
	{
		int flags = _O_WRONLY | _O_CREAT | _O_EXCL | _S_IREAD | _S_IWRITE;
		int fd;
		FILE *fp;

		if (mode[2] == 'b')
			flags |= _O_BINARY;

		fd = _open(path, flags);

		if (fd == -1)
			return NULL;

		fp = _fdopen(fd, (flags & _O_BINARY) ? "wb" : "w");

		if (fp == NULL)
			_close(fd);

		return fp;
	}

#endif
	return fopen(path, mode);
}

static int64_t open_file_write_real(file_t *file, const char *fullpath, const char *mode_str)
{
	FILE *fp;
	int64_t pos = 0;
	int ret;
	fp = Q_fopen(fullpath, mode_str);

	if (!fp)
		return Q_ERRNO;

#ifndef _WIN32
	// check if this is a regular file
	ret = get_fp_info(fp, NULL);

	if (ret)
		goto fail;

#endif

	switch (file->mode & FS_BUF_MASK)
	{
		case FS_BUF_NONE:
			// make it unbuffered
			setvbuf(fp, NULL, _IONBF, BUFSIZ);
			break;

		case FS_BUF_LINE:
			// make it line buffered
			setvbuf(fp, NULL, _IOLBF, BUFSIZ);
			break;

		case FS_BUF_FULL:
			// make it fully buffered
			setvbuf(fp, NULL, _IOFBF, BUFSIZ);
			break;

		default:
			// use default mode (normally fully buffered)
			break;
	}

	switch (file->mode & FS_MODE_MASK)
	{
		case FS_MODE_RDWR:

			// seek to the end of file for appending
			if (os_fseek(fp, 0, SEEK_END) == -1)
			{
				ret = Q_ERRNO;
				goto fail;
			}

		// fall through
		case FS_MODE_APPEND:
			// get current position
			pos = os_ftell(fp);

			if (pos == -1)
			{
				ret = Q_ERRNO;
				goto fail;
			}
	}

	file->type = FS_REAL;
	file->fp = fp;
	file->unique = true;
	file->error = Q_ERR_SUCCESS;
	return pos;
fail:
	fclose(fp);
	return ret;
}

static int64_t open_file_write_gzip(file_t *file, const char *fullpath, const char *mode_str)
{
#if USE_ZLIB
	void *zfp = gzopen(fullpath, mode_str);

	if (!zfp)
		return Q_ERR_LIBRARY_ERROR;

	file->type = FS_GZ;
	file->zfp = zfp;
	file->unique = true;
	file->error = Q_ERR_SUCCESS;
	return 0;
#else
	return Q_ERR_NOSYS;
#endif
}

static int64_t open_file_write(file_t *file, const char *name)
{
	char normalized[MAX_OSPATH], fullpath[MAX_OSPATH];
	char mode_str[8];
	size_t len;
	int64_t pos;
	int ret;
	// normalize the path
	len = FS_NormalizePathBuffer(normalized, name, sizeof(normalized));

	if (len >= sizeof(normalized))
		return Q_ERR_NAMETOOLONG;

	// reject empty paths
	if (len == 0)
		return Q_ERR_NAMETOOSHORT;

	// check for bad characters
	if (!FS_ValidatePath(normalized))
	{
		ret = Q_ERR_INVALID_PATH;
		goto fail;
	}

	// expand the path
	if ((file->mode & FS_PATH_MASK) == FS_PATH_BASE)
	{
		if (sys_homedir->string[0])
		{
			len = Q_concat(fullpath, sizeof(fullpath),
					sys_homedir->string, "/" BASEGAME "/", normalized, NULL);
		}
		else
		{
			len = Q_concat(fullpath, sizeof(fullpath),
					sys_basedir->string, "/" BASEGAME "/", normalized, NULL);
		}
	}
	else
	{
		len = Q_concat(fullpath, sizeof(fullpath),
				fs_gamedir, "/", normalized, NULL);
	}

	if (len >= sizeof(fullpath))
	{
		ret = Q_ERR_NAMETOOLONG;
		goto fail;
	}

	ret = FS_CreatePath(fullpath);

	if (ret)
		goto fail;

	switch (file->mode & FS_MODE_MASK)
	{
		case FS_MODE_APPEND:
			strcpy(mode_str, "a");
			break;

		case FS_MODE_WRITE:
			strcpy(mode_str, "w");

			if (file->mode & FS_FLAG_EXCL)
				strcat(mode_str, "x");

			break;

		case FS_MODE_RDWR:
			// similar to FS_MODE_APPEND, but does not create
			// the file if it does not exist
			strcpy(mode_str, "r+");
			break;

		default:
			Com_Error(ERR_FATAL, "%s: bad mode", __func__);
	}

	// open in binary mode by default
	if (!(file->mode & FS_FLAG_TEXT))
		strcat(mode_str, "b");

	if (file->mode & FS_FLAG_GZIP)
		pos = open_file_write_gzip(file, fullpath, mode_str);
	else
		pos = open_file_write_real(file, fullpath, mode_str);

	if (pos < 0)
	{
		ret = pos;
		goto fail;
	}

	FS_DPrintf("%s: %s: %"PRId64" bytes\n", __func__, fullpath, pos);
	return pos;
fail:
	FS_DPrintf("%s: %s: %s\n", __func__, normalized, Q_ErrorString(ret));
	return ret;
}

#if USE_ZLIB

static int check_header_coherency(FILE *fp, packfile_t *entry)
{
	unsigned ofs, flags, comp_mtd, comp_len, file_len, name_size, xtra_size;
	byte header[ZIP_SIZELOCALHEADER];

	if (os_fseek(fp, entry->filepos, SEEK_SET) == -1)
		return Q_ERRNO;

	if (fread(header, 1, sizeof(header), fp) != sizeof(header))
		return FS_ERR_READ(fp);

	// check the magic
	if (LittleLongMem(&header[0]) != ZIP_LOCALHEADERMAGIC)
		return Q_ERR_NOT_COHERENT;

	flags = LittleShortMem(&header[6]);
	comp_mtd = LittleShortMem(&header[8]);
	comp_len = LittleLongMem(&header[18]);
	file_len = LittleLongMem(&header[22]);
	name_size = LittleShortMem(&header[26]);
	xtra_size = LittleShortMem(&header[28]);

	if (comp_mtd != entry->compmtd)
		return Q_ERR_NOT_COHERENT;

	// bit 3 tells that file lengths were not known
	// at the time local header was written, so don't check them
	if ((flags & 8) == 0)
	{
		if (comp_len != entry->complen)
			return Q_ERR_NOT_COHERENT;

		if (file_len != entry->filelen)
			return Q_ERR_NOT_COHERENT;
	}

	ofs = ZIP_SIZELOCALHEADER + name_size + xtra_size;

	if (entry->filepos + entry->complen > INT_MAX - ofs)
		return Q_ERR_NOT_COHERENT;

	entry->filepos += ofs;
	entry->coherent = true;
	return Q_ERR_SUCCESS;
}

static voidpf FS_zalloc(voidpf opaque, uInt items, uInt size)
{
	return FS_Malloc(items * size);
}

static void FS_zfree(voidpf opaque, voidpf address)
{
	Z_Free(address);
}

static void open_zip_file(file_t *file)
{
	zipstream_t *s;
	z_streamp z;

	if (file->unique)
	{
		s = FS_Malloc(sizeof(*s));
		memset(&s->stream, 0, sizeof(s->stream));
	}
	else
		s = &fs_zipstream;

	z = &s->stream;

	if (z->state)
	{
		// already initialized, just reset
		inflateReset(z);
	}
	else
	{
		z->zalloc = FS_zalloc;
		z->zfree = FS_zfree;

		if (inflateInit2(z, -MAX_WBITS) != Z_OK)
			Com_Error(ERR_FATAL, "%s: inflateInit2() failed", __func__);
	}

	z->avail_in = z->avail_out = 0;
	z->total_in = z->total_out = 0;
	z->next_in = z->next_out = NULL;
	s->rest_in = file->entry->complen;
	file->zfp = s;
}

// only called for unique handles
static void close_zip_file(file_t *file)
{
	zipstream_t *s = file->zfp;
	inflateEnd(&s->stream);
	Z_Free(s);
	fclose(file->fp);
}

static int tell_zip_file(file_t *file)
{
	zipstream_t *s = file->zfp;
	return s->stream.total_out;
}

static int seek_zip_file(file_t *file, int64_t offset)
{
	int64_t left_to_read;
	zipstream_t *s = file->zfp;

	if (offset > s->stream.total_out)
		left_to_read = offset - s->stream.total_out;
	else
	{
		left_to_read = offset;
		open_zip_file(file);
	}

	static byte buffer[4096];

	while (left_to_read)
	{
		size_t num_to_read = min(sizeof(buffer), left_to_read);

		int ret = read_zip_file(file, buffer, num_to_read);

		if (ret < 0)
			return ret;
		else if (!ret)
			return Q_ERR_BADF;

		left_to_read -= ret;
	}

	return Q_ERR_SUCCESS;
}

static int read_zip_file(file_t *file, void *buf, size_t len)
{
	zipstream_t *s = file->zfp;
	z_streamp z = &s->stream;
	size_t block, result;
	int ret;

	if (len > file->rest_out)
		len = file->rest_out;

	if (!len)
		return 0;

	z->next_out = buf;
	z->avail_out = (uInt)len;

	do
	{
		if (!z->avail_in)
		{
			if (!s->rest_in)
				break;

			// fill in the temp buffer
			block = ZIP_BUFSIZE;

			if (block > s->rest_in)
				block = s->rest_in;

			result = fread(s->buffer, 1, block, file->fp);

			if (result != block)
			{
				file->error = FS_ERR_READ(file->fp);

				if (!result)
					break;
			}

			s->rest_in -= result;
			z->next_in = s->buffer;
			z->avail_in = result;
		}

		ret = inflate(z, Z_SYNC_FLUSH);

		if (ret == Z_STREAM_END)
			break;

		if (ret != Z_OK)
		{
			file->error = Q_ERR_INFLATE_FAILED;
			break;
		}

		if (file->error)
			break;
	}
	while (z->avail_out);

	len -= z->avail_out;
	file->rest_out -= len;

	if (file->error && len == 0)
		return file->error;

	return len;
}

#endif

// open a new file on the pakfile
static int64_t open_from_pak(file_t *file, pack_t *pack, packfile_t *entry, bool unique)
{
	FILE *fp;
	int ret;

	if (unique)
	{
		fp = fopen(pack->filename, "rb");

		if (!fp)
		{
			ret = Q_ERRNO;
			goto fail1;
		}
	}
	else
	{
		fp = pack->fp;
		clearerr(fp);
	}

#if USE_ZLIB

	if (pack->type == FS_ZIP && !entry->coherent)
	{
		ret = check_header_coherency(fp, entry);

		if (ret)
			goto fail2;
	}

#endif

	if (os_fseek(fp, entry->filepos, SEEK_SET) == -1)
	{
		ret = Q_ERRNO;
		goto fail2;
	}

	file->type = pack->type;
	file->fp = fp;
	file->entry = entry;
	file->pack = pack;
	file->unique = unique;
	file->error = Q_ERR_SUCCESS;
	file->rest_out = entry->filelen;
	file->length = entry->filelen;
#if USE_ZLIB

	if (pack->type == FS_ZIP)
	{
		if (file->mode & FS_FLAG_DEFLATE)
		{
			// server wants raw deflated data for downloads
			file->type = FS_PAK;
			file->rest_out = entry->complen;
			file->length = entry->complen;
		}
		else if (entry->compmtd)
			open_zip_file(file);
		else
		{
			// stored, just pretend it's a packfile
			file->type = FS_PAK;
		}
	}

#endif

	if (unique)
	{
		// reference source pak
		pack_get(pack);
	}

	FS_DPrintf("%s: %s/%s: %"PRId64" bytes\n",
		__func__, pack->filename, entry->name, file->length);
	return file->length;
fail2:

	if (unique)
		fclose(fp);

fail1:
	FS_DPrintf("%s: %s/%s: %s\n", __func__, pack->filename, entry->name, Q_ErrorString(ret));
	return ret;
}

#if USE_ZLIB
static int check_for_gzip(file_t *file, const char *fullpath)
{
	uint32_t magic, length;
	void *zfp;

	// should have at least 10 bytes of header and 8 bytes of trailer
	if (file->length < 18)
		return 0;

	// read magic
	if (fread(&magic, 1, 4, file->fp) != 4)
		return FS_ERR_READ(file->fp);

	// check for gzip header
	if ((LittleLong(magic) & 0xe0ffffff) != 0x00088b1f)
	{
		// rewind back to beginning
		if (os_fseek(file->fp, 0, SEEK_SET) == -1)
			return Q_ERRNO;

		return 0;
	}

	// seek to the trailer
	if (os_fseek(file->fp, file->length - 4, SEEK_SET) == -1)
		return Q_ERRNO;

	// read uncompressed length
	if (fread(&length, 1, 4, file->fp) != 4)
		return FS_ERR_READ(file->fp);

	zfp = gzopen(fullpath, "rb");

	if (!zfp)
		return Q_ERR_LIBRARY_ERROR;

	file->type = FS_GZ;
	file->fp = NULL;
	file->zfp = zfp;
	file->length = LittleLong(length);
	return 1;
}
#endif

static int64_t open_from_disk(file_t *file, const char *fullpath)
{
	FILE *fp;
	file_info_t info;
	int ret;
	fp = fopen(fullpath, "rb");

	if (!fp)
	{
		ret = Q_ERRNO;
		goto fail;
	}

	ret = get_fp_info(fp, &info);

	if (ret)
	{
		fclose(fp);
		goto fail;
	}

	file->type = FS_REAL;
	file->fp = fp;
	file->unique = true;
	file->error = Q_ERR_SUCCESS;
	file->length = info.size;
#if USE_ZLIB

	if (file->mode & FS_FLAG_GZIP)
	{
		ret = check_for_gzip(file, fullpath);

		if (ret)
		{
			fclose(fp);

			if (ret < 0)
			{
				memset(file, 0, sizeof(*file));
				goto fail;
			}
		}
	}

#endif
	FS_DPrintf("%s: %s: %"PRId64" bytes\n", __func__, fullpath, file->length);
	return file->length;
fail:
	FS_DPrintf("%s: %s: %s\n", __func__, fullpath, Q_ErrorString(ret));
	return ret;
}

// Finds the file in the search path.
// Fills file_t and returns file length.
// Used for streaming data out of either a pak file or a seperate file.
static int64_t open_file_read(file_t *file, const char *normalized, size_t namelen, bool unique)
{
	char            fullpath[MAX_OSPATH];
	searchpath_t    *search;
	pack_t          *pak;
	unsigned        hash;
	packfile_t      *entry;
	int64_t         ret;
	int             valid;
	size_t          len;
	hash = FS_HashPath(normalized, 0);
	valid = PATH_NOT_CHECKED;

	// search through the path, one element at a time
	for (search = fs_searchpaths; search; search = search->next)
	{
		if (file->mode & FS_PATH_MASK)
		{
			if ((file->mode & search->mode & FS_PATH_MASK) == 0)
				continue;
		}

		// is the element a pak file?
		if (search->pack)
		{
			if ((file->mode & FS_TYPE_MASK) == FS_TYPE_REAL)
				continue;

			// don't bother searching in paks if length exceedes MAX_QPATH
			if (namelen >= MAX_QPATH)
				continue;

			pak = search->pack;
#if USE_ZLIB

			if ((file->mode & FS_FLAG_DEFLATE) && pak->type != FS_ZIP)
				continue;

#endif
			// look through all the pak file elements
			entry = pak->file_hash[hash & (pak->hash_size - 1)];

			for (; entry; entry = entry->hash_next)
			{
				if (entry->namelen != namelen)
					continue;

#if USE_ZLIB

				if ((file->mode & FS_FLAG_DEFLATE) && entry->compmtd != Z_DEFLATED)
					continue;

#endif

				if (!FS_pathcmp(entry->name, normalized))
				{
					// found it!
					return open_from_pak(file, pak, entry, unique);
				}
			}
		}
		else
		{
			if ((file->mode & FS_TYPE_MASK) == FS_TYPE_PAK)
				continue;

#if USE_ZLIB

			if (file->mode & FS_FLAG_DEFLATE)
				continue;

#endif

			// don't error out immediately if the path is found to be invalid,
			// just stop looking for it in directory tree but continue to search
			// for it in packs, to give broken maps or mods a chance to work
			if (valid == PATH_NOT_CHECKED)
				valid = FS_ValidatePath(normalized);

			if (valid == PATH_INVALID)
				continue;

			// check a file in the directory tree
			len = Q_concat(fullpath, sizeof(fullpath),
					search->filename, "/", normalized, NULL);

			if (len >= sizeof(fullpath))
			{
				ret = Q_ERR_NAMETOOLONG;
				goto fail;
			}

			ret = open_from_disk(file, fullpath);

			if (ret != Q_ERR_NOENT)
				return ret;

#ifndef _WIN32

			if (valid == PATH_MIXED_CASE)
			{
				// convert to lower case and retry
				Q_strlwr(fullpath + strlen(search->filename) + 1);
				ret = open_from_disk(file, fullpath);

				if (ret != Q_ERR_NOENT)
					return ret;
			}

#endif
		}
	}

	// return error if path was checked and found to be invalid
	ret = valid ? Q_ERR_NOENT : Q_ERR_INVALID_PATH;
fail:
	FS_DPrintf("%s: %s: %s\n", __func__, normalized, Q_ErrorString(ret));
	return ret;
}

// Normalizes quake path
static int64_t expand_open_file_read(file_t *file, const char *name, bool unique)
{
	char        normalized[MAX_OSPATH];
	int64_t     ret;
	size_t      namelen;
	// normalize path
	namelen = FS_NormalizePathBuffer(normalized, name, MAX_OSPATH);

	if (namelen >= MAX_OSPATH)
		return Q_ERR_NAMETOOLONG;

	// reject empty paths
	if (namelen == 0)
		return Q_ERR_NAMETOOSHORT;

	ret = open_file_read(file, normalized, namelen, unique);
	return ret;
}

static int read_pak_file(file_t *file, void *buf, size_t len)
{
	size_t result;

	if (len > file->rest_out)
		len = file->rest_out;

	if (!len)
		return 0;

	result = fread(buf, 1, len, file->fp);

	if (result != len)
	{
		file->error = FS_ERR_READ(file->fp);

		if (!result)
			return file->error;
	}

	file->rest_out -= result;
	return result;
}

static int read_phys_file(file_t *file, void *buf, size_t len)
{
	size_t result;
	result = fread(buf, 1, len, file->fp);

	if (result != len && ferror(file->fp))
	{
		file->error = Q_ERR_FAILURE;

		if (!result)
			return file->error;
	}

	return result;
}

/*
=================
FS_Read
=================
*/
int FS_Read(void *buf, size_t len, qhandle_t f)
{
	file_t *file = file_for_handle(f);
#if USE_ZLIB
	int ret;
#endif

	if (!file)
		return Q_ERR_BADF;

	if ((file->mode & FS_MODE_MASK) != FS_MODE_READ)
		return Q_ERR_INVAL;

	// can't continue after error
	if (file->error)
		return file->error;

	if (len > INT_MAX)
		return Q_ERR_INVAL;

	if (len == 0)
		return 0;

	switch (file->type)
	{
		case FS_REAL:
			return read_phys_file(file, buf, len);

		case FS_PAK:
			return read_pak_file(file, buf, len);
#if USE_ZLIB

		case FS_GZ:
			ret = gzread(file->zfp, buf, len);

			if (ret < 0)
				return Q_ERR_LIBRARY_ERROR;

			return ret;

		case FS_ZIP:
			return read_zip_file(file, buf, len);
#endif

		default:
			return Q_ERR_NOSYS;
	}
}

int FS_ReadLine(qhandle_t f, char *buffer, size_t size)
{
	file_t *file = file_for_handle(f);
	char *s;
	size_t len;

	if (!file)
		return Q_ERR_BADF;

	if ((file->mode & FS_MODE_MASK) != FS_MODE_READ)
		return Q_ERR_INVAL;

	if (file->type != FS_REAL)
		return Q_ERR_NOSYS;

	do
	{
		s = fgets(buffer, size, file->fp);

		if (!s)
			return ferror(file->fp) ? Q_ERR_FAILURE : 0;

		len = strlen(s);
	}
	while (len < 2);

	s[len - 1] = 0;
	return len - 1;
}

void FS_Flush(qhandle_t f)
{
	file_t *file = file_for_handle(f);

	if (!file)
		return;

	switch (file->type)
	{
		case FS_REAL:
			fflush(file->fp);
			break;
#if USE_ZLIB

		case FS_GZ:
			gzflush(file->zfp, Z_SYNC_FLUSH);
			break;
#endif

		default:
			break;
	}
}

/*
=================
FS_Write
=================
*/
int FS_Write(const void *buf, size_t len, qhandle_t f)
{
	file_t  *file = file_for_handle(f);

	if (!file)
		return Q_ERR_BADF;

	if ((file->mode & FS_MODE_MASK) == FS_MODE_READ)
		return Q_ERR_INVAL;

	// can't continue after error
	if (file->error)
		return file->error;

	if (len > INT_MAX)
		return Q_ERR_INVAL;

	if (len == 0)
		return 0;

	switch (file->type)
	{
		case FS_REAL:
			if (fwrite(buf, 1, len, file->fp) != len)
			{
				file->error = Q_ERR_FAILURE;
				return file->error;
			}

			break;
#if USE_ZLIB

		case FS_GZ:
			if (gzwrite(file->zfp, buf, len) != len)
			{
				file->error = Q_ERR_LIBRARY_ERROR;
				return file->error;
			}

			break;
#endif

		default:
			Com_Error(ERR_FATAL, "%s: bad file type", __func__);
	}

	return len;
}

/*
============
FS_FOpenFile
============
*/
int64_t FS_FOpenFile(const char *name, qhandle_t *f, unsigned mode)
{
	file_t *file;
	qhandle_t handle;
	int64_t ret;

	if (!name || !f)
		Com_Error(ERR_FATAL, "%s: NULL", __func__);

	*f = 0;

	if (!fs_searchpaths)
	{
		return Q_ERR_AGAIN; // not yet initialized
	}

	// allocate new file handle
	file = alloc_handle(&handle);

	if (!file)
		return Q_ERR_MFILE;

	file->mode = mode;

	if ((mode & FS_MODE_MASK) == FS_MODE_READ)
		ret = expand_open_file_read(file, name, true);
	else
		ret = open_file_write(file, name);

	if (ret >= 0)
		*f = handle;

	return ret;
}

// reading from outside of source directory is allowed, extension is optional
static qhandle_t easy_open_read(char *buf, size_t size, unsigned mode,
	const char *dir, const char *name, const char *ext)
{
	size_t len;
	int64_t ret;
	qhandle_t f;

	if (*name == '/')
	{
		// full path is given, ignore directory and extension
		len = Q_strlcpy(buf, name + 1, size);
	}
	else
	{
		// first try without extension
		len = Q_concat(buf, size, dir, name, NULL);

		if (len >= size)
		{
			ret = Q_ERR_NAMETOOLONG;
			goto fail;
		}

		// print normalized path in case of error
		FS_NormalizePath(buf, buf);
		ret = FS_FOpenFile(buf, &f, mode);

		if (f)
		{
			return f; // succeeded
		}

		if (ret != Q_ERR_NOENT)
		{
			goto fail; // fatal error
		}

		if (!COM_CompareExtension(buf, ext))
		{
			goto fail; // name already has the extension
		}

		// now try to append extension
		len = Q_strlcat(buf, ext, size);
	}

	if (len >= size)
	{
		ret = Q_ERR_NAMETOOLONG;
		goto fail;
	}

	ret = FS_FOpenFile(buf, &f, mode);

	if (f)
		return f;

fail:
	Com_Printf("Couldn't open %s: %s\n", buf, Q_ErrorString(ret));
	return 0;
}

// writing to outside of destination directory is disallowed, extension is forced
static qhandle_t easy_open_write(char *buf, size_t size, unsigned mode,
	const char *dir, const char *name, const char *ext)
{
	char normalized[MAX_OSPATH];
	size_t len;
	int64_t ret;
	qhandle_t f;
	// make it impossible to escape the destination directory when writing files
	len = FS_NormalizePathBuffer(normalized, name, sizeof(normalized));

	if (len >= sizeof(normalized))
	{
		ret = Q_ERR_NAMETOOLONG;
		buf = normalized;
		goto fail;
	}

	// reject empty filenames
	if (len == 0)
	{
		ret = Q_ERR_NAMETOOSHORT;
		buf = normalized;
		goto fail;
	}

	// replace any bad characters with underscores to make automatic commands happy
	FS_CleanupPath(normalized);

	// don't append the extension if name already has it
	if (!COM_CompareExtension(normalized, ext))
		ext = (mode & FS_FLAG_GZIP) ? "" : NULL;

	len = Q_concat(buf, size, dir, normalized, ext,
			(mode & FS_FLAG_GZIP) ? ".gz" : NULL, NULL);

	if (len >= size)
	{
		ret = Q_ERR_NAMETOOLONG;
		goto fail;
	}

	ret = FS_FOpenFile(buf, &f, mode);

	if (f)
		return f;

fail:
	Com_EPrintf("Couldn't open %s: %s\n", buf, Q_ErrorString(ret));
	return 0;
}

/*
============
FS_EasyOpenFile

Helper function for various console commands. Concatenates
the arguments, checks for path buffer overflow, and attempts
to open the file, printing an error message in case of failure.
============
*/
qhandle_t FS_EasyOpenFile(char *buf, size_t size, unsigned mode,
	const char *dir, const char *name, const char *ext)
{
	if ((mode & FS_MODE_MASK) == FS_MODE_READ)
		return easy_open_read(buf, size, mode, dir, name, ext);

	return easy_open_write(buf, size, mode, dir, name, ext);
}

/*
============
FS_LoadFile

opens non-unique file handle as an optimization
a NULL buffer will just return the file length without loading
============
*/
int FS_LoadFileEx(const char *path, void **buffer, unsigned flags, memtag_t tag)
{
	file_t *file;
	qhandle_t f;
	byte *buf;
	int64_t len;
	int read;

	if (!path)
		Com_Error(ERR_FATAL, "%s: NULL", __func__);

	if (buffer)
		*buffer = NULL;

	if (!fs_searchpaths)
	{
		return Q_ERR_AGAIN; // not yet initialized
	}

	// allocate new file handle
	file = alloc_handle(&f);

	if (!file)
		return Q_ERR_MFILE;

	file->mode = (flags & ~FS_MODE_MASK) | FS_MODE_READ;
	// look for it in the filesystem or pack files
	len = expand_open_file_read(file, path, false);

	if (len < 0)
		return len;

	// sanity check file size
	if (len > MAX_LOADFILE)
	{
		len = Q_ERR_FBIG;
		goto done;
	}

	// NULL buffer just checks for file existence
	if (!buffer)
		goto done;

	// allocate chunk of memory, +1 for NUL
	buf = Z_TagMalloc(len + 1, tag);
	// read entire file
	read = FS_Read(buf, len, f);

	if (read != len)
	{
		len = read < 0 ? read : Q_ERR_UNEXPECTED_EOF;
		Z_Free(buf);
		goto done;
	}

	*buffer = buf;
	buf[len] = 0;
done:
	FS_FCloseFile(f);
	return len;
}

static int write_and_close(const void *data, size_t len, qhandle_t f)
{
	int ret1 = FS_Write(data, len, f);
	int ret2 = FS_FCloseFile(f);
	return ret1 < 0 ? ret1 : ret2;
}

/*
================
FS_WriteFile
================
*/
int FS_WriteFile(const char *path, const void *data, size_t len)
{
	qhandle_t f;
	int ret;
	// TODO: write to temp file perhaps?
	ret = FS_FOpenFile(path, &f, FS_MODE_WRITE);

	if (f)
		ret = write_and_close(data, len, f);

	return ret;
}

/*
============
FS_EasyWriteFile

Helper function for various console commands. Concatenates
the arguments, checks for path buffer overflow, and attempts
to write the file, printing an error message in case of failure.
============
*/
bool FS_EasyWriteFile(char *buf, size_t size, unsigned mode,
	const char *dir, const char *name, const char *ext,
	const void *data, size_t len)
{
	qhandle_t f;
	int ret;
	// TODO: write to temp file perhaps?
	f = easy_open_write(buf, size, mode, dir, name, ext);

	if (!f)
		return false;

	ret = write_and_close(data, len, f);

	if (ret < 0)
	{
		Com_EPrintf("Couldn't write %s: %s\n", buf, Q_ErrorString(ret));
		return false;
	}

	return true;
}

#if USE_CLIENT

/*
================
FS_RenameFile
================
*/
int FS_RenameFile(const char *from, const char *to)
{
	char normalized[MAX_OSPATH];
	char frompath[MAX_OSPATH];
	char topath[MAX_OSPATH];
	size_t len;
	// check from
	len = FS_NormalizePathBuffer(normalized, from, sizeof(normalized));

	if (len >= sizeof(normalized))
		return Q_ERR_NAMETOOLONG;

	if (!FS_ValidatePath(normalized))
		return Q_ERR_INVALID_PATH;

	len = Q_concat(frompath, sizeof(frompath), fs_gamedir, "/", normalized, NULL);

	if (len >= sizeof(frompath))
		return Q_ERR_NAMETOOLONG;

	// check to
	len = FS_NormalizePathBuffer(normalized, to, sizeof(normalized));

	if (len >= sizeof(normalized))
		return Q_ERR_NAMETOOLONG;

	if (!FS_ValidatePath(normalized))
		return Q_ERR_INVALID_PATH;

	len = Q_concat(topath, sizeof(topath), fs_gamedir, "/", normalized, NULL);

	if (len >= sizeof(topath))
		return Q_ERR_NAMETOOLONG;

	// rename it
	if (rename(frompath, topath))
		return Q_ERRNO;

	return Q_ERR_SUCCESS;
}

#endif // USE_CLIENT

/*
================
FS_VPrintf
================
*/
int FS_VPrintf(qhandle_t f, const char *format, va_list args)
{
	char string[MAXPRINTMSG];
	size_t len = Q_vsnprintf(string, sizeof(string), format, args);

	if (len >= sizeof(string))
		return Q_ERR_STRING_TRUNCATED;

	return FS_Write(string, len, f);
}

/*
================
FS_FPrintf
================
*/
int FS_FPrintf(qhandle_t f, const char *format, ...)
{
	va_list argptr;

	va_start(argptr, format);
	size_t len = FS_VPrintf(f, format, argptr);
	va_end(argptr);

	return len;
}

// references pack_t instance
static pack_t *pack_get(pack_t *pack)
{
	pack->refcount++;
	return pack;
}

// dereferences pack_t instance
static void pack_put(pack_t *pack)
{
	if (!pack)
		return;

	if (!pack->refcount)
		Com_Error(ERR_FATAL, "%s: refcount already zero", __func__);

	if (!--pack->refcount)
	{
		FS_DPrintf("Freeing packfile %s\n", pack->filename);
		fclose(pack->fp);
		Z_Free(pack);
	}
}

// allocates pack_t instance along with filenames and hashes in one chunk of memory
static pack_t *pack_alloc(FILE *fp, filetype_t type, const char *name,
	unsigned num_files, size_t names_len)
{
	pack_t *pack;
	unsigned hash_size;
	size_t len;
	hash_size = npot32(num_files / 3);
	len = strlen(name) + 1;
	pack = FS_Malloc(sizeof(pack_t) +
			num_files * sizeof(packfile_t) +
			hash_size * sizeof(packfile_t *) +
			len + names_len);
	pack->type = type;
	pack->refcount = 0;
	pack->fp = fp;
	pack->num_files = num_files;
	pack->hash_size = hash_size;
	pack->files = (packfile_t *)(pack + 1);
	pack->file_hash = (packfile_t **)(pack->files + num_files);
	pack->filename = (char *)(pack->file_hash + hash_size);
	pack->names = pack->filename + len;
	memcpy(pack->filename, name, len);
	memset(pack->file_hash, 0, hash_size * sizeof(packfile_t *));
	return pack;
}

// normalizes and inserts the filename into hash table
static void pack_hash_file(pack_t *pack, packfile_t *file)
{
	size_t hash;
	file->namelen = FS_NormalizePath(file->name, file->name);
	hash = FS_HashPath(file->name, pack->hash_size);
	file->hash_next = pack->file_hash[hash];
	pack->file_hash[hash] = file;
}

// Loads the header and directory, adding the files at the beginning
// of the list so they override previous pack files.
static pack_t *load_pak_file(const char *packfile)
{
	dpackheader_t   header;
	packfile_t      *file;
	dpackfile_t     *dfile;
	unsigned        i, num_files;
	char            *name;
	size_t          len, names_len;
	pack_t          *pack;
	FILE            *fp;
	dpackfile_t     info[MAX_FILES_IN_PACK];
	fp = fopen(packfile, "rb");

	if (!fp)
	{
		Com_Printf("Couldn't open %s: %s\n", packfile, strerror(errno));
		return NULL;
	}

	if (fread(&header, 1, sizeof(header), fp) != sizeof(header))
	{
		Com_Printf("Reading header failed on %s\n", packfile);
		goto fail;
	}

	if (LittleLong(header.ident) != IDPAKHEADER)
	{
		Com_Printf("%s is not a 'PACK' file\n", packfile);
		goto fail;
	}

	header.dirlen = LittleLong(header.dirlen);

	if (header.dirlen % sizeof(dpackfile_t))
	{
		Com_Printf("%s has bad directory length\n", packfile);
		goto fail;
	}

	num_files = header.dirlen / sizeof(dpackfile_t);

	if (num_files < 1)
	{
		Com_Printf("%s has no files\n", packfile);
		goto fail;
	}

	if (num_files > MAX_FILES_IN_PACK)
	{
		Com_Printf("%s has too many files: %u > %u\n", packfile, num_files, MAX_FILES_IN_PACK);
		goto fail;
	}

	header.dirofs = LittleLong(header.dirofs);

	if (header.dirofs > INT_MAX)
	{
		Com_Printf("%s has bad directory offset\n", packfile);
		goto fail;
	}

	if (os_fseek(fp, header.dirofs, SEEK_SET))
	{
		Com_Printf("Seeking to directory failed on %s\n", packfile);
		goto fail;
	}

	if (fread(info, 1, header.dirlen, fp) != header.dirlen)
	{
		Com_Printf("Reading directory failed on %s\n", packfile);
		goto fail;
	}

	names_len = 0;

	for (i = 0, dfile = info; i < num_files; i++, dfile++)
	{
		dfile->filepos = LittleLong(dfile->filepos);
		dfile->filelen = LittleLong(dfile->filelen);

		if (dfile->filelen > INT_MAX || dfile->filepos > INT_MAX - dfile->filelen)
		{
			Com_Printf("%s has bad directory structure\n", packfile);
			goto fail;
		}

		dfile->name[sizeof(dfile->name) - 1] = 0;
		names_len += strlen(dfile->name) + 1;
	}

	// allocate the pack
	pack = pack_alloc(fp, FS_PAK, packfile, num_files, names_len);
	// parse the directory
	file = pack->files;
	name = pack->names;

	for (i = 0, dfile = info; i < num_files; i++, dfile++)
	{
		len = strlen(dfile->name) + 1;
		file->name = memcpy(name, dfile->name, len);
		name += len;
		file->filepos = dfile->filepos;
		file->filelen = dfile->filelen;
#if USE_ZLIB
		file->coherent = true;
#endif
		pack_hash_file(pack, file);
		file++;
	}

	FS_DPrintf("%s: %u files, %u hash\n",
		packfile, pack->num_files, pack->hash_size);
	return pack;
fail:
	fclose(fp);
	return NULL;
}

#if USE_ZLIB

// Locate the central directory of a zipfile (at the end, just before the global comment)
static unsigned search_central_header(FILE *fp)
{
	unsigned file_size, back_read;
	unsigned max_back = 0xffff; // maximum size of global comment
	byte buf[ZIP_BUFREADCOMMENT + 4];
	int64_t ret;

	if (os_fseek(fp, 0, SEEK_END) == -1)
		return 0;

	ret = os_ftell(fp);

	if (ret == -1 || ret > INT_MAX)
		return 0;

	file_size = ret;

	if (max_back > file_size)
		max_back = file_size;

	back_read = 4;

	while (back_read < max_back)
	{
		unsigned i, read_size, read_pos;

		if (back_read + ZIP_BUFREADCOMMENT > max_back)
			back_read = max_back;
		else
			back_read += ZIP_BUFREADCOMMENT;

		read_pos = file_size - back_read;
		read_size = back_read;

		if (read_size > ZIP_BUFREADCOMMENT + 4)
			read_size = ZIP_BUFREADCOMMENT + 4;

		if (os_fseek(fp, read_pos, SEEK_SET) == -1)
			break;

		if (fread(buf, 1, read_size, fp) != read_size)
			break;

		i = read_size - 4;

		do
		{
			// check the magic
			if (LittleLongMem(buf + i) == ZIP_ENDHEADERMAGIC)
				return read_pos + i;
		}
		while (i--);
	}

	return 0;
}

// Get Info about the current file in the zipfile, with internal only info
static unsigned get_file_info(FILE *fp, unsigned pos, packfile_t *file, size_t *len, size_t remaining)
{
	unsigned comp_mtd, comp_len, file_len, name_size, xtra_size, comm_size, file_pos;
	byte header[ZIP_SIZECENTRALDIRITEM]; // we can't use a struct here because of packing
	*len = 0;

	if (pos > INT_MAX)
		return 0;

	if (os_fseek(fp, pos, SEEK_SET) == -1)
		return 0;

	if (fread(header, 1, sizeof(header), fp) != sizeof(header))
		return 0;

	// check the magic
	if (LittleLongMem(&header[0]) != ZIP_CENTRALHEADERMAGIC)
		return 0;

	comp_mtd = LittleShortMem(&header[10]);
	comp_len = LittleLongMem(&header[20]);
	file_len = LittleLongMem(&header[24]);
	name_size = LittleShortMem(&header[28]);
	xtra_size = LittleShortMem(&header[30]);
	comm_size = LittleShortMem(&header[32]);
	file_pos = LittleLongMem(&header[42]);

	if (file_len > INT_MAX || comp_len > INT_MAX || file_pos > INT_MAX - comp_len)
		return 0;

	if (!file_len || !comp_len)
	{
		goto skip; // skip directories and empty files
	}

	if (!comp_mtd)
	{
		if (file_len != comp_len)
		{
			FS_DPrintf("%s: skipping file stored with file_len != comp_len\n", __func__);
			goto skip;
		}
	}
	else if (comp_mtd != Z_DEFLATED)
	{
		FS_DPrintf("%s: skipping file compressed with unknown method\n", __func__);
		goto skip;
	}

	if (!name_size)
	{
		FS_DPrintf("%s: skipping file with empty name\n", __func__);
		goto skip;
	}

	if (name_size >= MAX_QPATH)
	{
		FS_DPrintf("%s: skipping file with oversize name\n", __func__);
		goto skip;
	}

	// fill in the info
	if (file)
	{
		if (name_size >= remaining)
			return 0; // directory changed on disk?

		file->compmtd = comp_mtd;
		file->complen = comp_len;
		file->filelen = file_len;
		file->filepos = file_pos;

		if (fread(file->name, 1, name_size, fp) != name_size)
			return 0;

		file->name[name_size] = 0;
	}

	*len = name_size + 1;
skip:
	return ZIP_SIZECENTRALDIRITEM + name_size + xtra_size + comm_size;
}

static pack_t *load_zip_file(const char *packfile)
{
	packfile_t      *file;
	char            *name;
	size_t          len, names_len;
	unsigned        i, num_disk, num_disk_cd, num_files, num_files_cd;
	unsigned        header_pos, central_ofs, central_size, central_end;
	unsigned        extra_bytes, ofs;
	pack_t          *pack;
	FILE            *fp;
	byte            header[ZIP_SIZECENTRALHEADER];
	fp = fopen(packfile, "rb");

	if (!fp)
	{
		Com_Printf("Couldn't open %s: %s\n", packfile, strerror(errno));
		return NULL;
	}

	header_pos = search_central_header(fp);

	if (!header_pos)
	{
		Com_Printf("No central header found in %s\n", packfile);
		goto fail2;
	}

	if (os_fseek(fp, header_pos, SEEK_SET) == -1)
	{
		Com_Printf("Couldn't seek to central header in %s\n", packfile);
		goto fail2;
	}

	if (fread(header, 1, sizeof(header), fp) != sizeof(header))
	{
		Com_Printf("Reading central header failed on %s\n", packfile);
		goto fail2;
	}

	num_disk = LittleShortMem(&header[4]);
	num_disk_cd = LittleShortMem(&header[6]);
	num_files = LittleShortMem(&header[8]);
	num_files_cd = LittleShortMem(&header[10]);

	if (num_files_cd != num_files || num_disk_cd != 0 || num_disk != 0)
	{
		Com_Printf("%s is an unsupported multi-part archive\n", packfile);
		goto fail2;
	}

	if (num_files < 1)
	{
		Com_Printf("%s has no files\n", packfile);
		goto fail2;
	}

	if (num_files > ZIP_MAXFILES)
	{
		Com_Printf("%s has too many files: %u > %u\n", packfile, num_files, ZIP_MAXFILES);
		goto fail2;
	}

	central_size = LittleLongMem(&header[12]);
	central_ofs = LittleLongMem(&header[16]);
	central_end = central_ofs + central_size;

	if (central_end > header_pos || central_end < central_ofs)
	{
		Com_Printf("%s has bad central directory offset\n", packfile);
		goto fail2;
	}

	// non-zero for sfx?
	extra_bytes = header_pos - central_end;

	if (extra_bytes)
	{
		Com_Printf("%s has %d extra bytes at the beginning, funny sfx archive?\n",
			packfile, extra_bytes);
	}

	// parse the directory
	num_files = 0;
	names_len = 0;
	header_pos = central_ofs + extra_bytes;

	for (i = 0; i < num_files_cd; i++)
	{
		ofs = get_file_info(fp, header_pos, NULL, &len, 0);

		if (!ofs)
		{
			Com_Printf("%s has bad central directory structure (pass %d)\n", packfile, 1);
			goto fail2;
		}

		header_pos += ofs;

		if (len)
		{
			names_len += len;
			num_files++;
		}
	}

	if (!num_files)
	{
		Com_Printf("%s has no valid files\n", packfile);
		goto fail2;
	}

	// allocate the pack
	pack = pack_alloc(fp, FS_ZIP, packfile, num_files, names_len);
	// parse the directory
	file = pack->files;
	name = pack->names;
	header_pos = central_ofs + extra_bytes;

	for (i = 0; i < num_files_cd; i++)
	{
		if (!num_files)
			break;

		file->name = name;
		ofs = get_file_info(fp, header_pos, file, &len, names_len);

		if (!ofs)
		{
			Com_Printf("%s has bad central directory structure (pass %d)\n", packfile, 2);
			goto fail1; // directory changed on disk?
		}

		header_pos += ofs;

		if (len)
		{
			// fix absolute position
			file->filepos += extra_bytes;
			file->coherent = false;
			pack_hash_file(pack, file);
			// advance pointers, decrement counters
			file++;
			num_files--;
			name += len;
			names_len -= len;
		}
	}

	FS_DPrintf("%s: %u files, %u skipped, %u hash\n",
		packfile, pack->num_files, num_files_cd - pack->num_files, pack->hash_size);
	return pack;
fail1:
	Z_Free(pack);
fail2:
	fclose(fp);
	return NULL;
}
#endif

// this is complicated as we need pakXX.pak loaded first,
// sorted in numerical order, then the rest of the paks in
// alphabetical order, e.g. pak0.pak, pak2.pak, pak17.pak, abc.pak...
static int pakcmp(const void *p1, const void *p2)
{
	char *s1 = *(char **)p1;
	char *s2 = *(char **)p2;

	if (!Q_stricmpn(s1, "pak", 3))
	{
		if (!Q_stricmpn(s2, "pak", 3))
		{
			unsigned long n1 = strtoul(s1 + 3, &s1, 10);
			unsigned long n2 = strtoul(s2 + 3, &s2, 10);

			if (n1 > n2)
				return 1;

			if (n1 < n2)
				return -1;

			goto alphacmp;
		}

		return -1;
	}

	if (!Q_stricmpn(s2, "pak", 3))
		return 1;

alphacmp:
	return Q_stricmp(s1, s2);
}

// sets fs_gamedir, adds the directory to the head of the path,
// then loads and adds pak*.pak, then anything else in alphabethical order.
static void q_printf(2, 3) add_game_dir(unsigned mode, const char *fmt, ...)
{
	va_list         argptr;
	searchpath_t    *search;
	pack_t          *pack;
	listfiles_t     list;
	int             i;
	char            path[MAX_OSPATH];
	size_t          len;
	va_start(argptr, fmt);
	len = Q_vsnprintf(fs_gamedir, sizeof(fs_gamedir), fmt, argptr);
	va_end(argptr);

	if (len >= sizeof(fs_gamedir))
	{
		Com_EPrintf("%s: refusing oversize path\n", __func__);
		return;
	}

#ifdef _WIN32
	FS_ReplaceSeparators(fs_gamedir, '/');
#endif
	// add the directory to the search path
	search = FS_Malloc(sizeof(searchpath_t) + len);
	search->mode = mode;
	search->pack = NULL;
	memcpy(search->filename, fs_gamedir, len + 1);
	search->next = fs_searchpaths;
	fs_searchpaths = search;
	// add any pack files
	memset(&list, 0, sizeof(list));
#if USE_ZLIB
	list.filter = ".pak;.pkz";
#else
	list.filter = ".pak";
#endif
	Sys_ListFiles_r(&list, fs_gamedir, 0);

	if (!list.count)
		return;

	qsort(list.files, list.count, sizeof(list.files[0]), pakcmp);

	for (i = 0; i < list.count; i++)
	{
		len = Q_concat(path, sizeof(path), fs_gamedir, "/", list.files[i], NULL);

		if (len >= sizeof(path))
		{
			Com_EPrintf("%s: refusing oversize path\n", __func__);
			continue;
		}

#if USE_ZLIB

		// FIXME: guess packfile type by contents instead?
		if (len > 4 && !Q_stricmp(path + len - 4, ".pkz"))
			pack = load_zip_file(path);
		else
#endif
			pack = load_pak_file(path);

		if (!pack)
			continue;

		search = FS_Malloc(sizeof(searchpath_t));
		search->mode = mode;
		search->filename[0] = 0;
		search->pack = pack_get(pack);
		search->next = fs_searchpaths;
		fs_searchpaths = search;
	}

	for (i = 0; i < list.count; i++)
		Z_Free(list.files[i]);

	Z_Free(list.files);
}

/*
=================
FS_CopyInfo
=================
*/
file_info_t *FS_CopyInfo(const char *name, int64_t size, time_t ctime, time_t mtime)
{
	file_info_t *out;
	size_t len;

	if (!name)
		return NULL;

	len = strlen(name);
	out = FS_Mallocz(sizeof(*out) + len);
	out->size = size;
	out->ctime = ctime;
	out->mtime = mtime;
	memcpy(out->name, name, len + 1);
	return out;
}

void **FS_CopyList(void **list, int count)
{
	void **out;
	int i;

	if (!count)
		return NULL;

	out = FS_Malloc(sizeof(void *) * (count + 1));

	for (i = 0; i < count; i++)
		out[i] = list[i];

	out[i] = NULL;
	return out;
}

bool FS_WildCmp(const char *filter, const char *string)
{
	do
	{
		if (Com_WildCmpEx(filter, string, ';', true))
			return true;

		filter = strchr(filter, ';');

		if (filter)
			filter++;
	}
	while (filter);

	return false;
}

bool FS_ExtCmp(const char *ext, const char *name)
{
	int        c1, c2;
	const char *e, *n, *l;

	if (!name[0] || !ext[0])
		return false;

	for (l = name; l[1]; l++)
		;

	for (e = ext; e[1]; e++)
		;

rescan:
	n = l;

	do
	{
		c1 = *e--;
		c2 = *n--;

		if (c1 == ';')
		{
			break; // matched
		}

		if (c1 != c2)
		{
			c1 = Q_tolower(c1);
			c2 = Q_tolower(c2);

			if (c1 != c2)
			{
				while (e > ext)
				{
					c1 = *e--;

					if (c1 == ';')
						goto rescan;
				}

				return false;
			}
		}

		if (n < name)
			return false;
	}
	while (e >= ext);

	return true;
}

static int infocmp(const void *p1, const void *p2)
{
	file_info_t *n1 = *(file_info_t **)p1;
	file_info_t *n2 = *(file_info_t **)p2;
	return FS_pathcmp(n1->name, n2->name);
}

static int alphacmp(const void *p1, const void *p2)
{
	char *s1 = *(char **)p1;
	char *s2 = *(char **)p2;
	return FS_pathcmp(s1, s2);
}

/*
=================
FS_ListFiles
=================
*/
void **FS_ListFiles(const char *path, const char *filter, unsigned flags, int *count_p)
{
	searchpath_t    *search;
	packfile_t      *file;
	void            *info;
	int             i, j, total;
	char            normalized[MAX_OSPATH], buffer[MAX_OSPATH];
	listfiles_t     list;
	size_t          len, pathlen;
	char            *s, *p;
	int             valid;
	memset(&list, 0, sizeof(list));
	valid = PATH_NOT_CHECKED;

	if (count_p)
		*count_p = 0;

	if (!path)
	{
		path = "";
		pathlen = 0;
	}
	else
	{
		// normalize the path
		pathlen = FS_NormalizePathBuffer(normalized, path, sizeof(normalized));

		if (pathlen >= sizeof(normalized))
			return NULL;

		path = normalized;
	}

	// can't mix directory search with other flags
	if ((flags & FS_SEARCH_DIRSONLY) && (flags & FS_SEARCH_MASK & ~FS_SEARCH_DIRSONLY))
		return NULL;

	for (search = fs_searchpaths; search; search = search->next)
	{
		if (flags & FS_PATH_MASK)
		{
			if ((flags & search->mode & FS_PATH_MASK) == 0)
				continue;
		}

		if (search->pack)
		{
			if ((flags & FS_TYPE_MASK) == FS_TYPE_REAL)
			{
				continue; // don't search in paks
			}

			for (i = 0; i < search->pack->num_files; i++)
			{
				file = &search->pack->files[i];
				s = file->name;

				// check path
				if (pathlen)
				{
					if (file->namelen < pathlen)
						continue;

					if (FS_pathcmpn(s, path, pathlen))
						continue;

					if (s[pathlen] != '/')
					{
						continue;   // matched prefix must be a directory
					}

					if (flags & FS_SEARCH_BYFILTER)
						s += pathlen + 1;
				}
				else if (path == normalized)
				{
					if (!(flags & FS_SEARCH_DIRSONLY) && strchr(s, '/'))
					{
						continue;   // must be a file in the root directory
					}
				}

				// check filter
				if (filter)
				{
					if (flags & FS_SEARCH_BYFILTER)
					{
						if (!FS_WildCmp(filter, s))
							continue;
					}
					else
					{
						if (!FS_ExtCmp(filter, s))
							continue;
					}
				}

				// copy name off
				if (flags & (FS_SEARCH_DIRSONLY | FS_SEARCH_STRIPEXT))
					s = strcpy(buffer, s);

				// hacky directory search support for pak files
				if (flags & FS_SEARCH_DIRSONLY)
				{
					p = s;

					if (pathlen)
						p += pathlen + 1;

					p = strchr(p, '/');

					if (!p)
					{
						continue;   // does not have directory component
					}

					*p = 0;

					for (j = 0; j < list.count; j++)
					{
						if (!FS_pathcmp(list.files[j], s))
							break;
					}

					if (j != list.count)
					{
						continue;   // already listed this directory
					}
				}

				// strip path
				if (!(flags & FS_SEARCH_SAVEPATH))
					s = COM_SkipPath(s);

				// strip extension
				if (flags & FS_SEARCH_STRIPEXT)
					*COM_FileExtension(s) = 0;

				if (!*s)
					continue;

				// copy info off
				if (flags & FS_SEARCH_EXTRAINFO)
					info = FS_CopyInfo(s, file->filelen, 0, 0);
				else
					info = FS_CopyString(s);

				list.files = FS_ReallocList(list.files, list.count + 1);
				list.files[list.count++] = info;

				if (list.count >= MAX_LISTED_FILES)
					break;
			}
		}
		else
		{
			if ((flags & FS_TYPE_MASK) == FS_TYPE_PAK)
			{
				continue; // don't search in filesystem
			}

			len = strlen(search->filename);

			if (pathlen)
			{
				if (len + pathlen + 1 >= MAX_OSPATH)
					continue;

				if (valid == PATH_NOT_CHECKED)
					valid = FS_ValidatePath(path);

				if (valid == PATH_INVALID)
					continue;

				s = memcpy(buffer, search->filename, len);
				s[len++] = '/';
				memcpy(s + len, path, pathlen + 1);
			}
			else
				s = search->filename;

			if (flags & FS_SEARCH_BYFILTER)
				len += pathlen + 1;

			list.filter = filter;
			list.flags = flags;
			list.baselen = len;
			Sys_ListFiles_r(&list, s, 0);
		}

		if (list.count >= MAX_LISTED_FILES)
			break;
	}

	if (!list.count)
		return NULL;

	if (flags & FS_SEARCH_EXTRAINFO)
	{
		// TODO
		qsort(list.files, list.count, sizeof(list.files[0]), infocmp);
		total = list.count;
	}
	else
	{
		// sort alphabetically
		qsort(list.files, list.count, sizeof(list.files[0]), alphacmp);

		// remove duplicates
		for (i = total = 0; i < list.count; i++, total++)
		{
			info = list.files[i];

			while (i + 1 < list.count && !FS_pathcmp(list.files[i + 1], info))
				Z_Free(list.files[++i]);

			list.files[total] = info;
		}
	}

	if (count_p)
		*count_p = total;

	list.files = FS_ReallocList(list.files, total + 1);
	list.files[total] = NULL;
	return list.files;
}

/*
=================
FS_FreeList
=================
*/
void FS_FreeList(void **list)
{
	void **p;

	if (!list)
		return;

	for (p = list; *p; p++)
		Z_Free(*p);

	Z_Free(list);
}

void FS_File_g(const char *path, const char *ext, unsigned flags, genctx_t *ctx)
{
	int i, numFiles;
	void **list;
	char *s;
	list = FS_ListFiles(path, ext, flags, &numFiles);

	if (!list)
		return;

	for (i = 0; i < numFiles; i++)
	{
		s = list[i];

		if (ctx->count < ctx->size && !strncmp(s, ctx->partial, ctx->length))
		{
			ctx->matches = Z_Realloc(ctx->matches, ALIGN(ctx->count + 1, MIN_MATCHES) * sizeof(char *));
			ctx->matches[ctx->count++] = s;
		}
		else
			Z_Free(s);
	}

	Z_Free(list);
}

static void print_file_list(const char *path, const char *ext, unsigned flags)
{
	void    **list;
	int     i, total;
	list = FS_ListFiles(path, ext, flags, &total);

	for (i = 0; i < total; i++)
		Com_Printf("%s\n", (char *)list[i]);

	Com_Printf("%i files listed\n", total);
	FS_FreeList(list);
}

/*
============
FS_Path_f
============
*/
static void FS_Path_f(void)
{
	searchpath_t *s;
	int numFilesInPAK = 0;
#if USE_ZLIB
	int numFilesInZIP = 0;
#endif
	Com_Printf("Current search path:\n");

	for (s = fs_searchpaths; s; s = s->next)
	{
		if (s->pack)
		{
#if USE_ZLIB

			if (s->pack->type == FS_ZIP)
				numFilesInZIP += s->pack->num_files;
			else
#endif
				numFilesInPAK += s->pack->num_files;

			Com_Printf("%s (%i files)\n", s->pack->filename, s->pack->num_files);
		}
		else
			Com_Printf("%s\n", s->filename);
	}

	if (numFilesInPAK)
		Com_Printf("%i files in PAK files\n", numFilesInPAK);

#if USE_ZLIB

	if (numFilesInZIP)
		Com_Printf("%i files in PKZ files\n", numFilesInZIP);

#endif
}

static void free_search_path(searchpath_t *path)
{
	pack_put(path->pack);
	Z_Free(path);
}

static void free_all_paths(void)
{
	searchpath_t *path, *next;

	for (path = fs_searchpaths; path; path = next)
	{
		next = path->next;
		free_search_path(path);
	}

	fs_searchpaths = NULL;
}

static void free_game_paths(void)
{
	searchpath_t *path, *next;

	for (path = fs_searchpaths; path != fs_base_searchpaths; path = next)
	{
		next = path->next;
		free_search_path(path);
	}

	fs_searchpaths = fs_base_searchpaths;
}

static void setup_base_paths(void)
{
	// base paths have both BASE and GAME bits set by default
	// the GAME bit will be removed once gamedir is set,
	// and will be put back once gamedir is reset to basegame
	add_game_dir(FS_PATH_BASE | FS_PATH_GAME, "%s/"BASEGAME, sys_basedir->string);
	fs_base_searchpaths = fs_searchpaths;
}

// Sets the gamedir and path to a different directory.
static void setup_game_paths(void)
{
	searchpath_t *path;
	cvar_t *q2_path = Cvar_Get("path_quake2", "", 0);

	if (q2_path->string[0])
		add_game_dir(FS_PATH_ANY, "%s", q2_path->string);

	if (fs_game->string[0])
	{
		// add system path first
		add_game_dir(FS_PATH_GAME, "%s/%s", sys_basedir->string, fs_game->string);

		// home paths override system paths
		if (sys_homedir->string[0])
		{
			add_game_dir(FS_PATH_BASE, "%s/"BASEGAME, sys_homedir->string);
			add_game_dir(FS_PATH_GAME, "%s/%s", sys_homedir->string, fs_game->string);
		}

		// remove the game bit from base paths
		for (path = fs_base_searchpaths; path; path = path->next)
			path->mode &= ~FS_PATH_GAME;

		// this var is set for compatibility with server browsers, etc
		Cvar_FullSet("gamedir", fs_game->string, CVAR_ROM | CVAR_SERVERINFO, FROM_CODE);
	}
	else
	{
		if (sys_homedir->string[0])
		{
			add_game_dir(FS_PATH_BASE | FS_PATH_GAME,
				"%s/"BASEGAME, sys_homedir->string);
		}

		// add the game bit to base paths
		for (path = fs_base_searchpaths; path; path = path->next)
			path->mode |= FS_PATH_GAME;

		Cvar_FullSet("gamedir", "", CVAR_ROM, FROM_CODE);
	}

	// this var is used by the game library to find it's home directory
	Cvar_FullSet("fs_gamedir", fs_gamedir, CVAR_ROM, FROM_CODE);
}

/*
================
FS_Restart

Unless total is true, reloads paks only up to base dir
================
*/
void FS_Restart(bool total)
{
	Com_Printf("----- FS_Restart -----\n");

	if (total)
	{
		// perform full reset
		free_all_paths();
		setup_base_paths();
	}
	else
	{
		// just change gamedir
		free_game_paths();
		Q_snprintf(fs_gamedir, sizeof(fs_gamedir), "%s/"BASEGAME, sys_basedir->string);
#ifdef _WIN32
		FS_ReplaceSeparators(fs_gamedir, '/');
#endif
	}

	setup_game_paths();
	FS_Path_f();
	Com_Printf("----------------------\n");
}

/*
============
FS_Restart_f

Console command to fully re-start the file system.
============
*/
static void FS_Restart_f(void)
{
	CL_RestartFilesystem(true);
}

static const cmdreg_t c_fs[] =
{
	{ "path", FS_Path_f },
	{ "fs_restart", FS_Restart_f },

	{ NULL }
};

/*
================
FS_Shutdown
================
*/
void FS_Shutdown(void)
{
	file_t *file;
	int i;

	if (!fs_searchpaths)
		return;

	// close file handles
	for (i = 0, file = fs_files; i < MAX_FILE_HANDLES; i++, file++)
	{
		if (file->type != FS_FREE)
		{
			Com_WPrintf("%s: closing handle %d\n", __func__, i + 1);
			FS_FCloseFile((qhandle_t)(i + 1));
		}
	}

	// free search paths
	free_all_paths();
#if USE_ZLIB
	inflateEnd(&fs_zipstream.stream);
#endif
	Z_LeakTest(TAG_FILESYSTEM);
	Cmd_Deregister(c_fs);
}

// this is called when local server starts up and gets it's latched variables,
// client receives a serverdata packet, or user changes the game by hand while
// disconnected
static void fs_game_changed(cvar_t *self)
{
	char *s = self->string;

	// validate it
	if (*s)
	{
		if (!Q_stricmp(s, BASEGAME))
			Cvar_Reset(self);
		else if (!COM_IsPath(s))
		{
			Com_Printf("'%s' should contain characters [A-Za-z0-9_-] only.\n", self->name);
			Cvar_Reset(self);
		}
	}

	// check for the first time startup
	if (!fs_base_searchpaths)
	{
		// start up with baseq2 by default
		setup_base_paths();
		Com_AddConfigFile(COM_PATHS_CFG, 0);
		// check for game override
		setup_game_paths();
		FS_Path_f();
		return;
	}

	// otherwise, restart the filesystem
	CL_RestartFilesystem(false);
	Com_AddConfigFile(COM_PATHS_CFG, FS_PATH_GAME);

	// if baseq2/autoexec.cfg exists DO NOT exec default.cfg and config.cfg.
	// this assumes user prefers to do configuration via autoexec.cfg and doesn't
	// want settings and binds messed up whenever gamedir changes after startup.
	if (!FS_FileExistsEx(COM_AUTOEXEC_CFG, FS_TYPE_REAL | FS_PATH_BASE))
	{
		// if config.cfg exists, ignore default.cfg
		if (!FS_FileExistsEx(COM_CONFIG_CFG, FS_PATH_GAME))
			Com_AddConfigFile(COM_DEFAULT_CFG, FS_PATH_GAME);

		Com_AddConfigFile(COM_CONFIG_CFG, FS_TYPE_REAL | FS_PATH_GAME);
	}

	// exec autoexec.cfg (must be a real file within the game directory)
	Com_AddConfigFile(COM_AUTOEXEC_CFG, FS_TYPE_REAL | FS_PATH_GAME);
	// exec postexec.cfg (must be a real file)
	Com_AddConfigFile(COM_POSTEXEC_CFG, FS_TYPE_REAL);
}

int FS_LoadFile(const char *path, void **buf)
{
	return FS_LoadFileEx(path, buf, 0, TAG_FILESYSTEM);
}

bool FS_FileExistsEx(const char *path, unsigned flags)
{
	return FS_LoadFileEx(path, NULL, flags, TAG_FREE) != Q_ERR_NOENT;
}

/*
================
FS_Init
================
*/
void FS_Init(void)
{
	Com_Printf("------- FS_Init -------\n");
	Cmd_Register(c_fs);
#ifdef _DEBUG
	fs_debug = Cvar_Get("fs_debug", "0", 0);
#endif
	// get the game cvar and start the filesystem
	fs_game = Cvar_Get("game", DEFGAME, CVAR_LATCH | CVAR_SERVERINFO);
	fs_game->changed = fs_game_changed;
	fs_game_changed(fs_game);
	Com_Printf("-----------------------\n");
}

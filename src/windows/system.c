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

#include "client.h"
#include "common/cvar.h"
#include "common/field.h"
#include "common/prompt.h"
#include <VersionHelpers.h>

HINSTANCE                       hGlobalInstance;

#if !_DEBUG
	HANDLE                          mainProcessThread;
	LPTOP_LEVEL_EXCEPTION_FILTER    prevExceptionFilter;
#endif

static char                     currentDirectory[MAX_OSPATH];

typedef enum
{
	SE_NOT,
	SE_YES,
	SE_FULL
} should_exit_t;

static volatile should_exit_t   shouldExit;
static volatile bool            errorEntered;

cvar_t  *sys_basedir;
cvar_t  *sys_libdir;
cvar_t  *sys_homedir;
cvar_t  *sys_forcegamelib;

/*
===============================================================================

CONSOLE I/O

===============================================================================
*/

#if USE_SYSCON

#define MAX_CONSOLE_INPUT_EVENTS    16

static HANDLE   hinput = INVALID_HANDLE_VALUE;
static HANDLE   houtput = INVALID_HANDLE_VALUE;

#if USE_CLIENT
	static cvar_t           *sys_viewlog;
#endif

static commandPrompt_t  sys_con;
static int              sys_hidden;
static bool             gotConsole;

static void write_console_data(void *data, size_t len)
{
	DWORD res;
	WriteFile(houtput, data, len, &res, NULL);
}

static void hide_console_input(void)
{
	CONSOLE_SCREEN_BUFFER_INFO info;

	if (!sys_hidden && GetConsoleScreenBufferInfo(houtput, &info))
	{
		size_t len = strlen(sys_con.inputLine.text);
		COORD pos = { 0, info.dwCursorPosition.Y };
		DWORD res = min(len + 1, info.dwSize.X);
		FillConsoleOutputCharacter(houtput, ' ', res, pos, &res);
		SetConsoleCursorPosition(houtput, pos);
	}

	sys_hidden++;
}

static void show_console_input(void)
{
	CONSOLE_SCREEN_BUFFER_INFO info;

	if (sys_hidden && !--sys_hidden && GetConsoleScreenBufferInfo(houtput, &info))
	{
		inputField_t *f = &sys_con.inputLine;
		size_t pos = f->cursorPos;
		char *text = f->text;
		// update line width after resize
		f->visibleChars = info.dwSize.X - 1;

		// scroll horizontally
		if (pos >= f->visibleChars)
		{
			pos = f->visibleChars - 1;
			text += f->cursorPos - pos;
		}

		size_t len = strlen(text);
		DWORD res = min(len, f->visibleChars) + 1;
		WriteConsoleOutputCharacter(houtput, va("]%s", text), res, (COORD)
		{
			0, info.dwCursorPosition.Y
		}, &res);
		SetConsoleCursorPosition(houtput, (COORD)
		{
			pos + 1, info.dwCursorPosition.Y
		});
	}
}

static void console_delete(inputField_t *f)
{
	if (f->text[f->cursorPos])
	{
		hide_console_input();
		memmove(f->text + f->cursorPos, f->text + f->cursorPos + 1, sizeof(f->text) - f->cursorPos - 1);
		show_console_input();
	}
}

static void console_move_cursor(inputField_t *f, size_t pos)
{
	size_t oldpos = f->cursorPos;
	f->cursorPos = pos = min(pos, f->maxChars - 1);

	if (oldpos < f->visibleChars && pos < f->visibleChars)
	{
		CONSOLE_SCREEN_BUFFER_INFO info;

		if (GetConsoleScreenBufferInfo(houtput, &info))
		{
			SetConsoleCursorPosition(houtput, (COORD)
			{
				pos + 1, info.dwCursorPosition.Y
			});
		}
	}
	else
	{
		hide_console_input();
		show_console_input();
	}
}

static void console_move_right(inputField_t *f)
{
	if (f->text[f->cursorPos] && f->cursorPos < f->maxChars - 1)
		console_move_cursor(f, f->cursorPos + 1);
}

static void console_move_left(inputField_t *f)
{
	if (f->cursorPos > 0)
		console_move_cursor(f, f->cursorPos - 1);
}

static void console_replace_char(inputField_t *f, int ch)
{
	CONSOLE_SCREEN_BUFFER_INFO info;

	if (GetConsoleScreenBufferInfo(houtput, &info))
	{
		DWORD res;
		FillConsoleOutputCharacter(houtput, ch, 1, info.dwCursorPosition, &res);
	}
}

static void scroll_console_window(int key)
{
	CONSOLE_SCREEN_BUFFER_INFO info;

	if (GetConsoleScreenBufferInfo(houtput, &info))
	{
		int lo = -info.srWindow.Top;
		int hi = max(info.dwCursorPosition.Y - info.srWindow.Bottom, 0);
		int page = info.srWindow.Bottom - info.srWindow.Top + 1;
		int rows = 0;

		switch (key)
		{
			case VK_HOME:
				rows = lo;
				break;

			case VK_END:
				rows = hi;
				break;

			case VK_PRIOR:
				rows = max(-page, lo);
				break;

			case VK_NEXT:
				rows = min(page, hi);
				break;
		}

		if (rows)
			SetConsoleWindowInfo(houtput, FALSE, &(SMALL_RECT)
		{
			.Top = rows, .Bottom = rows
		});
	}
}

static void clear_console_window(void)
{
	CONSOLE_SCREEN_BUFFER_INFO info;

	if (sys_hidden)
		scroll_console_window(VK_END);

	hide_console_input();

	if (GetConsoleScreenBufferInfo(houtput, &info))
	{
		COORD pos = { 0, info.srWindow.Top };
		DWORD res = (info.srWindow.Bottom - info.srWindow.Top) * info.dwSize.X;
		FillConsoleOutputCharacter(houtput, ' ', res, pos, &res);
		SetConsoleCursorPosition(houtput, pos);
	}

	show_console_input();
}

/*
================
Sys_ConsoleInput
================
*/
void Sys_RunConsole(void)
{
	INPUT_RECORD    recs[MAX_CONSOLE_INPUT_EVENTS];
	int     ch;
	DWORD   numread, numevents;
	int     i;
	inputField_t    *f = &sys_con.inputLine;
	char    *s;

	if (hinput == INVALID_HANDLE_VALUE)
		return;

	if (!gotConsole)
		return;

	while (1)
	{
		if (!GetNumberOfConsoleInputEvents(hinput, &numevents))
		{
			Com_EPrintf("Error %lu getting number of console events.\n", GetLastError());
			gotConsole = false;
			return;
		}

		if (numevents < 1)
			break;

		if (numevents > MAX_CONSOLE_INPUT_EVENTS)
			numevents = MAX_CONSOLE_INPUT_EVENTS;

		if (!ReadConsoleInput(hinput, recs, numevents, &numread))
		{
			Com_EPrintf("Error %lu reading console input.\n", GetLastError());
			gotConsole = false;
			return;
		}

		for (i = 0; i < numread; i++)
		{
			if (recs[i].EventType == WINDOW_BUFFER_SIZE_EVENT)
			{
				// determine terminal width
				COORD size = recs[i].Event.WindowBufferSizeEvent.dwSize;
				WORD width = size.X;

				if (width < 2)
				{
					Com_EPrintf("Invalid console buffer width.\n");
					continue;
				}

				// figure out input line width
				if (width != sys_con.widthInChars)
				{
					sys_con.widthInChars = width;
					sys_con.inputLine.visibleChars = 0; // force refresh
				}

				Com_DPrintf("System console resized (%d cols, %d rows).\n", size.X, size.Y);
				continue;
			}

			if (recs[i].EventType != KEY_EVENT)
				continue;

			if (!recs[i].Event.KeyEvent.bKeyDown)
				continue;

			WORD key = recs[i].Event.KeyEvent.wVirtualKeyCode;
			DWORD mod = recs[i].Event.KeyEvent.dwControlKeyState;
			size_t pos;

			if (mod & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
			{
				switch (key)
				{
					case 'A':
						console_move_cursor(f, 0);
						break;

					case 'E':
						console_move_cursor(f, strlen(f->text));
						break;

					case 'B':
						console_move_left(f);
						break;

					case 'F':
						console_move_right(f);
						break;

					case 'C':
						GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
						break;

					case 'D':
						console_delete(f);
						break;

					case 'W':
						pos = f->cursorPos;

						while (pos > 0 && f->text[pos - 1] <= ' ')
							pos--;

						while (pos > 0 && f->text[pos - 1] > ' ')
							pos--;

						if (pos < f->cursorPos)
						{
							hide_console_input();
							memmove(f->text + pos, f->text + f->cursorPos, sizeof(f->text) - f->cursorPos);
							f->cursorPos = pos;
							show_console_input();
						}

						break;

					case 'U':
						if (f->cursorPos > 0)
						{
							hide_console_input();
							memmove(f->text, f->text + f->cursorPos, sizeof(f->text) - f->cursorPos);
							f->cursorPos = 0;
							show_console_input();
						}

						break;

					case 'K':
						if (f->text[f->cursorPos])
						{
							hide_console_input();
							f->text[f->cursorPos] = 0;
							show_console_input();
						}

						break;

					case 'L':
						clear_console_window();
						break;

					case 'N':
						hide_console_input();
						Prompt_HistoryDown(&sys_con);
						show_console_input();
						break;

					case 'P':
						hide_console_input();
						Prompt_HistoryUp(&sys_con);
						show_console_input();
						break;

					case 'R':
						hide_console_input();
						Prompt_CompleteHistory(&sys_con, false);
						show_console_input();
						break;

					case 'S':
						hide_console_input();
						Prompt_CompleteHistory(&sys_con, true);
						show_console_input();
						break;

					case VK_HOME:
					case VK_END:
						scroll_console_window(key);
						break;
				}

				continue;
			}

			if (mod & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
			{
				switch (key)
				{
					case 'B':
						pos = f->cursorPos;

						while (pos > 0 && f->text[pos - 1] <= ' ')
							pos--;

						while (pos > 0 && f->text[pos - 1] > ' ')
							pos--;

						console_move_cursor(f, pos);
						break;

					case 'F':
						pos = f->cursorPos;

						while (f->text[pos] && f->text[pos] <= ' ')
							pos++;

						while (f->text[pos] > ' ')
							pos++;

						console_move_cursor(f, pos);
						break;
				}

				continue;
			}

			switch (key)
			{
				case VK_UP:
					hide_console_input();
					Prompt_HistoryUp(&sys_con);
					show_console_input();
					break;

				case VK_DOWN:
					hide_console_input();
					Prompt_HistoryDown(&sys_con);
					show_console_input();
					break;

				case VK_PRIOR:
				case VK_NEXT:
					scroll_console_window(key);
					break;

				case VK_RETURN:
					hide_console_input();
					s = Prompt_Action(&sys_con);

					if (s)
					{
						if (*s == '\\' || *s == '/')
							s++;

						Sys_Printf("]%s\n", s);
						Cbuf_AddText(&cmd_buffer, s);
						Cbuf_AddText(&cmd_buffer, "\n");
					}
					else
						write_console_data("]\n", 2);

					show_console_input();
					break;

				case VK_BACK:
					if (f->cursorPos > 0)
					{
						if (f->text[f->cursorPos] == 0 && f->cursorPos < f->visibleChars)
						{
							f->text[--f->cursorPos] = 0;
							write_console_data("\b \b", 3);
						}
						else
						{
							hide_console_input();
							memmove(f->text + f->cursorPos - 1, f->text + f->cursorPos, sizeof(f->text) - f->cursorPos);
							f->cursorPos--;
							show_console_input();
						}
					}

					break;

				case VK_DELETE:
					console_delete(f);
					break;

				case VK_END:
					console_move_cursor(f, strlen(f->text));
					break;

				case VK_HOME:
					console_move_cursor(f, 0);
					break;

				case VK_RIGHT:
					console_move_right(f);
					break;

				case VK_LEFT:
					console_move_left(f);
					break;

				case VK_TAB:
					hide_console_input();
					Prompt_CompleteCommand(&sys_con, false);
					show_console_input();
					break;

				default:
					ch = recs[i].Event.KeyEvent.uChar.AsciiChar;

					if (ch < 32)
						break;

					if (f->cursorPos == f->maxChars - 1)
					{
						// buffer limit reached, replace the character under
						// cursor. replace without moving cursor to prevent
						// newline when cursor is at the rightmost column.
						console_replace_char(f, ch);
						f->text[f->cursorPos + 0] = ch;
						f->text[f->cursorPos + 1] = 0;
					}
					else if (f->text[f->cursorPos] == 0 && f->cursorPos + 1 < f->visibleChars)
					{
						write_console_data(va("%c", ch), 1);
						f->text[f->cursorPos + 0] = ch;
						f->text[f->cursorPos + 1] = 0;
						f->cursorPos++;
					}
					else
					{
						hide_console_input();
						memmove(f->text + f->cursorPos + 1, f->text + f->cursorPos, sizeof(f->text) - f->cursorPos - 1);
						f->text[f->cursorPos++] = ch;
						f->text[f->maxChars] = 0;
						show_console_input();
					}

					break;
			}
		}
	}
}

#define FOREGROUND_BLACK    0
#define FOREGROUND_WHITE    (FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED)

static const WORD textColors[8] =
{
	FOREGROUND_BLACK,
	FOREGROUND_RED,
	FOREGROUND_GREEN,
	FOREGROUND_RED | FOREGROUND_GREEN,
	FOREGROUND_BLUE,
	FOREGROUND_BLUE | FOREGROUND_GREEN,
	FOREGROUND_RED | FOREGROUND_BLUE,
	FOREGROUND_WHITE
};

void Sys_SetConsoleColor(color_index_t color)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	WORD    attr, w;

	if (houtput == INVALID_HANDLE_VALUE)
		return;

	if (!gotConsole)
		return;

	if (!GetConsoleScreenBufferInfo(houtput, &info))
		return;

	attr = info.wAttributes & ~FOREGROUND_WHITE;

	switch (color)
	{
		case COLOR_NONE:
			w = attr | FOREGROUND_WHITE;
			break;

		case COLOR_ALT:
			w = attr | FOREGROUND_GREEN;
			break;

		default:
			w = attr | textColors[color];
			break;
	}

	if (color != COLOR_NONE)
		hide_console_input();

	SetConsoleTextAttribute(houtput, w);

	if (color == COLOR_NONE)
		show_console_input();
}

static void write_console_output(const char *text)
{
	char    buf[MAXPRINTMSG];
	size_t  len;

	for (len = 0; len < MAXPRINTMSG; len++)
	{
		int c = *text++;

		if (!c)
			break;

		buf[len] = Q_charascii(c);
	}

	write_console_data(buf, len);
}

/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput(const char *text)
{
	if (houtput == INVALID_HANDLE_VALUE)
		return;

	if (!*text)
		return;

	if (!gotConsole)
		write_console_output(text);
	else
	{
		static bool hack = false;

		if (!hack)
		{
			hide_console_input();
			hack = true;
		}

		write_console_output(text);

		if (text[strlen(text) - 1] == '\n')
		{
			show_console_input();
			hack = false;
		}
	}
}

void Sys_SetConsoleTitle(const char *title)
{
	if (gotConsole)
		SetConsoleTitle(title);
}

static BOOL WINAPI Sys_ConsoleCtrlHandler(DWORD dwCtrlType)
{
	if (errorEntered)
		exit(1);

	shouldExit = SE_FULL;
	return TRUE;
}

static void Sys_ConsoleInit(void)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	DWORD mode;
	WORD width;
#if USE_CLIENT

	if (!AllocConsole())
	{
		Com_EPrintf("Couldn't create system console.\n");
		return;
	}

#endif
	hinput = GetStdHandle(STD_INPUT_HANDLE);
	houtput = GetStdHandle(STD_OUTPUT_HANDLE);

	if (!GetConsoleScreenBufferInfo(houtput, &info))
	{
		Com_EPrintf("Couldn't get console buffer info.\n");
		return;
	}

	// determine terminal width
	width = info.dwSize.X;

	if (width < 2)
	{
		Com_EPrintf("Invalid console buffer width.\n");
		return;
	}

	if (!GetConsoleMode(hinput, &mode))
	{
		Com_EPrintf("Couldn't get console input mode.\n");
		return;
	}

	mode &= ~(ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
	mode |= ENABLE_WINDOW_INPUT;

	if (!SetConsoleMode(hinput, mode))
	{
		Com_EPrintf("Couldn't set console input mode.\n");
		return;
	}

	SetConsoleTitle(PRODUCT " console");
	SetConsoleCtrlHandler(Sys_ConsoleCtrlHandler, TRUE);
	sys_con.widthInChars = width;
	sys_con.printf = Sys_Printf;
	gotConsole = true;
	// figure out input line width
	IF_Init(&sys_con.inputLine, width - 1, MAX_FIELD_TEXT - 1);
	Com_DPrintf("System console initialized (%d cols, %d rows).\n",
		info.dwSize.X, info.dwSize.Y);
}

#endif // USE_SYSCON

/*
===============================================================================

MISC

===============================================================================
*/

#if USE_SYSCON
/*
================
Sys_Printf
================
*/
void Sys_Printf(const char *fmt, ...)
{
	va_list     argptr;
	char        msg[MAXPRINTMSG];
	va_start(argptr, fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);
	Sys_ConsoleOutput(msg);
}
#endif

/*
================
Sys_Error
================
*/
void Sys_Error(const char *error, ...)
{
	va_list     argptr;
	char        text[MAXERRORMSG];
	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);
	errorEntered = true;
#if USE_CLIENT && USE_REF
	VID_FatalShutdown();
#endif
#if USE_SYSCON
	Sys_SetConsoleColor(COLOR_RED);
	Sys_Printf("********************\n"
		"FATAL: %s\n"
		"********************\n", text);
	Sys_SetConsoleColor(COLOR_NONE);
#endif
#if USE_SYSCON

	if (gotConsole)
	{
		hide_console_input();
		Sleep(INFINITE);
	}

#endif
	MessageBox(NULL, text, PRODUCT " Fatal Error", MB_ICONERROR | MB_OK);
	exit(1);
}

/*
================
Sys_Quit

This function never returns.
================
*/
void Sys_Quit(void)
{
	Sys_ShutdownAsyncWork();
#if USE_CLIENT && USE_SYSCON

	if (dedicated && dedicated->integer)
		FreeConsole();

#endif
	exit(0);
}

void Sys_AddDefaultConfig(void)
{
}

/*
================
Sys_Init
================
*/
void Sys_Init(void)
{
#if !_DEBUG
	cvar_t *var q_unused;
#endif

	// check windows version
	if (!IsWindows7SP1OrGreater())
		Sys_Error(PRODUCT " requires Windows 7 SP1 or greater");

	// basedir <path>
	// allows the game to run from outside the data tree
	sys_basedir = Cvar_Get("basedir", currentDirectory, CVAR_NOSET);
	sys_libdir = Cvar_Get("libdir", currentDirectory, CVAR_NOSET);
	// homedir <path>
	// specifies per-user writable directory for screenshots, etc
	sys_homedir = Cvar_Get("homedir", "", CVAR_NOSET);
	sys_forcegamelib = Cvar_Get("sys_forcegamelib", "", CVAR_NOSET);
#if USE_SYSCON
	houtput = GetStdHandle(STD_OUTPUT_HANDLE);
#if USE_CLIENT
	sys_viewlog = Cvar_Get("sys_viewlog", "0", CVAR_NOSET);

	if (dedicated->integer || sys_viewlog->integer)
#endif
		Sys_ConsoleInit();

#endif // USE_SYSCON
#if !_DEBUG
	var = Cvar_Get("sys_disablecrashdump", "0", CVAR_NOSET);

	// install our exception filter
	if (!var->integer)
	{
		mainProcessThread = GetCurrentThread();
		prevExceptionFilter = SetUnhandledExceptionFilter(
				Sys_ExceptionFilter);
	}

#endif
}

/*
========================================================================

FILESYSTEM

========================================================================
*/

static inline time_t file_time_to_unix(FILETIME *f)
{
	ULARGE_INTEGER u = *(ULARGE_INTEGER *)f;
	return (u.QuadPart - 116444736000000000ULL) / 10000000;
}

static void *copy_info(const char *name, const LPWIN32_FIND_DATAA data)
{
	int64_t size = data->nFileSizeLow | (uint64_t)data->nFileSizeHigh << 32;
	time_t ctime = file_time_to_unix(&data->ftCreationTime);
	time_t mtime = file_time_to_unix(&data->ftLastWriteTime);
	return FS_CopyInfo(name, size, ctime, mtime);
}

/*
=================
Sys_ListFiles_r
=================
*/
void Sys_ListFiles_r(listfiles_t *list, const char *path, int depth)
{
	WIN32_FIND_DATAA    data;
	HANDLE      handle;
	char        fullpath[MAX_OSPATH], *name;
	size_t      pathlen, len;
	unsigned    mask;
	void        *info;
	const char  *filter = list->filter;

	// optimize single extension search
	if (!(list->flags & FS_SEARCH_BYFILTER) &&
		filter && !strchr(filter, ';'))
	{
		if (*filter == '.')
			filter++;

		len = Q_concat(fullpath, sizeof(fullpath),
				path, "\\*.", filter, NULL);
		filter = NULL; // do not check it later
	}
	else
	{
		len = Q_concat(fullpath, sizeof(fullpath),
				path, "\\*", NULL);
	}

	if (len >= sizeof(fullpath))
		return;

	// format path to windows style
	// done on the first run only
	if (!depth)
		FS_ReplaceSeparators(fullpath, '\\');

	handle = FindFirstFile(fullpath, &data);

	if (handle == INVALID_HANDLE_VALUE)
		return;

	// make it point right after the slash
	pathlen = strlen(path) + 1;

	do
	{
		if (!strcmp(data.cFileName, ".") ||
			!strcmp(data.cFileName, ".."))
		{
			continue; // ignore special entries
		}

		// construct full path
		len = strlen(data.cFileName);

		if (pathlen + len >= sizeof(fullpath))
			continue;

		memcpy(fullpath + pathlen, data.cFileName, len + 1);

		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			mask = FS_SEARCH_DIRSONLY;
		else
			mask = 0;

		// pattern search implies recursive search
		if ((list->flags & FS_SEARCH_BYFILTER) && mask &&
			depth < MAX_LISTED_DEPTH)
		{
			Sys_ListFiles_r(list, fullpath, depth + 1);

			// re-check count
			if (list->count >= MAX_LISTED_FILES)
				break;
		}

		// check type
		if ((list->flags & FS_SEARCH_DIRSONLY) != mask)
			continue;

		// check filter
		if (filter)
		{
			if (list->flags & FS_SEARCH_BYFILTER)
			{
				if (!FS_WildCmp(filter, fullpath + list->baselen))
					continue;
			}
			else
			{
				if (!FS_ExtCmp(filter, data.cFileName))
					continue;
			}
		}

		// strip path
		if (list->flags & FS_SEARCH_SAVEPATH)
			name = fullpath + list->baselen;
		else
			name = data.cFileName;

		// reformat it back to quake filesystem style
		FS_ReplaceSeparators(name, '/');

		// strip extension
		if (list->flags & FS_SEARCH_STRIPEXT)
		{
			*COM_FileExtension(name) = 0;

			if (!*name)
				continue;
		}

		// copy info off
		if (list->flags & FS_SEARCH_EXTRAINFO)
			info = copy_info(name, &data);
		else
			info = FS_CopyString(name);

		list->files = FS_ReallocList(list->files, list->count + 1);
		list->files[list->count++] = info;
	}
	while (list->count < MAX_LISTED_FILES &&
		FindNextFile(handle, &data) != FALSE);

	FindClose(handle);
}

/*
========================================================================

MAIN

========================================================================
*/

static BOOL fix_current_directory(void)
{
	char *p;

	if (!GetModuleFileName(NULL, currentDirectory, sizeof(currentDirectory) - 1))
		return FALSE;

	if ((p = strrchr(currentDirectory, '\\')) != NULL)
		* p = 0;

	if (!SetCurrentDirectory(currentDirectory))
		return FALSE;

	return TRUE;
}

#if (_MSC_VER >= 1400)
static void msvcrt_sucks(const wchar_t *expr, const wchar_t *func,
	const wchar_t *file, unsigned int line, uintptr_t unused)
{
}
#endif

static int Sys_Main(int argc, char **argv)
{
	// fix current directory to point to the basedir
	if (!fix_current_directory())
		return 1;

#if (_MSC_VER >= 1400)
	// work around strftime given invalid format string
	// killing the whole fucking process :((
	_set_invalid_parameter_handler(msvcrt_sucks);
#endif
	Qcommon_Init(argc, argv);

	// main program loop
	while (1)
	{
		Sys_CompleteAsyncWork();
		Qcommon_Frame();

		if (shouldExit)
			Com_Quit(NULL, ERR_DISCONNECT);
	}

	// may get here when our service stops
	return 0;
}

#if USE_CLIENT

#define MAX_LINE_TOKENS    128

static char     *sys_argv[MAX_LINE_TOKENS];
static int      sys_argc;

/*
===============
Sys_ParseCommandLine

===============
*/
static void Sys_ParseCommandLine(char *line)
{
	sys_argc = 1;
	sys_argv[0] = APPLICATION;

	while (*line)
	{
		while (*line && *line <= 32)
			line++;

		if (*line == 0)
			break;

		sys_argv[sys_argc++] = line;

		while (*line > 32)
			line++;

		if (*line == 0)
			break;

		*line = 0;

		if (sys_argc == MAX_LINE_TOKENS)
			break;

		line++;
	}
}

/*
==================
WinMain

==================
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	// previous instances do not exist in Win32
	if (hPrevInstance)
		return 1;

	hGlobalInstance = hInstance;
#ifndef UNICODE
	// TODO: wince support
	Sys_ParseCommandLine(lpCmdLine);
#endif
	return Sys_Main(sys_argc, sys_argv);
}

#else // USE_CLIENT

/*
==================
main

==================
*/
int main(int argc, char **argv)
{
	hGlobalInstance = GetModuleHandle(NULL);
	return Sys_Main(argc, argv);
}

#endif // !USE_CLIENT

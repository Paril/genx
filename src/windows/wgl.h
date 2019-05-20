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

//
// win_wgl.h
//

bool        WGL_Init(const char *dllname);
void        WGL_Shutdown(void);
void        WGL_InitExtensions(void);
void        WGL_ShutdownExtensions(void);
void		WGL_ParseExtensionString(const char *s);

extern void (APIENTRY * qwglDrawBuffer)(GLenum mode);
extern const GLubyte * (APIENTRY * qwglGetString)(GLenum name);

extern int (WINAPI * qwglChoosePixelFormat)(HDC, CONST PIXELFORMATDESCRIPTOR *);
extern int (WINAPI * qwglDescribePixelFormat)(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
extern BOOL (WINAPI * qwglSetPixelFormat)(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
extern BOOL (WINAPI * qwglSwapBuffers)(HDC);

extern HGLRC (WINAPI * qwglCreateContext)(HDC);
extern BOOL (WINAPI * qwglDeleteContext)(HGLRC);
extern PROC (WINAPI * qwglGetProcAddress)(LPCSTR);
extern BOOL (WINAPI * qwglMakeCurrent)(HDC, HGLRC);

extern const char * (WINAPI * qwglGetExtensionsStringARB)(HDC hdc);

extern BOOL (WINAPI * qwglChoosePixelFormatARB)(HDC, const int *, const FLOAT *, UINT, int *, UINT *);

extern BOOL (WINAPI * qwglSwapIntervalEXT)(int interval);

extern HGLRC (WINAPI * qwglCreateContextAttribsARB)(HDC, HGLRC, const int *);
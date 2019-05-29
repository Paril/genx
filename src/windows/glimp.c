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

/*
GLW_IMP.C

This file contains ALL Win32 specific stuff having to do with the
OpenGL refresh.  When a port is being made the following functions
must be implemented by the port:

GLimp_EndFrame
GLimp_Init
GLimp_Shutdown
GLimp_SwitchFullscreen
*/

#include "client.h"
#include "glimp.h"

glwstate_t glw;

static cvar_t   *gl_driver;
static cvar_t   *gl_swapinterval;
static cvar_t   *gl_colorbits;
static cvar_t   *gl_depthbits;
static cvar_t   *gl_stencilbits;

/*
VID_Shutdown

This routine does all OS specific shutdown procedures for the OpenGL
subsystem.  Under OpenGL this means NULLing out the current DC and
HGLRC, deleting the rendering context, and releasing the DC acquired
for the window.  The state structure is also nulled out.
*/
void VID_Shutdown(void)
{
	wglMakeCurrent(NULL, NULL);

	if (glw.hGLRC)
	{
		wglDeleteContext(glw.hGLRC);
		glw.hGLRC = NULL;
	}

	Win_Shutdown();

	if (gl_swapinterval)
		gl_swapinterval->changed = NULL;

	memset(&glw, 0, sizeof(glw));
}

static void ReportLastError(const char *what)
{
	Com_EPrintf("%s failed with error %lu\n", what, GetLastError());
}

static void ReportPixelFormat(int pixelformat, PIXELFORMATDESCRIPTOR *pfd)
{
	Com_DPrintf("GL_FPD(%d): flags(%#lx) color(%d) Z(%d) stencil(%d)\n",
		pixelformat, pfd->dwFlags, pfd->cColorBits, pfd->cDepthBits,
		pfd->cStencilBits);
}

#define FAIL_OK     0
#define FAIL_SOFT   -1
#define FAIL_HARD   -2

static int SetupGL(int colorbits, int depthbits, int stencilbits)
{
	PIXELFORMATDESCRIPTOR pfd;
	int pixelformat;
	// create the main window
	Win_Init();

	if (colorbits == 0)
		colorbits = 24;

	if (depthbits == 0)
		depthbits = colorbits > 16 ? 24 : 16;

	if (depthbits < 24)
		stencilbits = 0;

	// choose pixel format
	UINT numFormats;
	const int iAttributes[] =
	{
		WGL_DRAW_TO_WINDOW_ARB, TRUE,
		WGL_SUPPORT_OPENGL_ARB, TRUE,
		WGL_DOUBLE_BUFFER_ARB, TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, colorbits,
		WGL_DEPTH_BITS_ARB, depthbits,
		WGL_STENCIL_BITS_ARB, stencilbits,
		0, 0
	};

	if (wglChoosePixelFormatARB(win.dc, iAttributes, NULL, 1, &pixelformat, &numFormats) == FALSE)
	{
		ReportLastError("wglChoosePixelFormatARB");
		goto soft;
	}

	if (numFormats == 0)
	{
		Com_EPrintf("No suitable OpenGL pixelformat found\n");
		goto soft;
	}

	// set pixel format
	DescribePixelFormat(win.dc, pixelformat, sizeof(pfd), &pfd);
	ReportPixelFormat(pixelformat, &pfd);

	if (SetPixelFormat(win.dc, pixelformat, &pfd) == FALSE)
	{
		ReportLastError("SetPixelFormat");
		goto soft;
	}

	// check for software emulation
	if (pfd.dwFlags & PFD_GENERIC_FORMAT)
	{
		Com_EPrintf("No hardware OpenGL acceleration detected\n");
		goto soft;
	}
	else if (pfd.dwFlags & PFD_GENERIC_ACCELERATED)
	{
		Com_DPrintf("...MCD acceleration found\n");
		win.flags |= QVF_ACCELERATED;
	}
	else
	{
		Com_DPrintf("...ICD acceleration found\n");
		win.flags |= QVF_ACCELERATED;
	}

	const int iContextAttributes[] =
	{
		//WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		//WGL_CONTEXT_MINOR_VERSION_ARB, 2,
		//WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0, 0
	};

	// startup the OpenGL subsystem by creating a context and making it current
	if ((glw.hGLRC = wglCreateContextAttribsARB(win.dc, NULL, iContextAttributes)) == NULL)
	{
		ReportLastError("wglCreateContext");
		goto hard;
	}

	if (wglMakeCurrent(win.dc, glw.hGLRC) == FALSE)
	{
		ReportLastError("wglMakeCurrent");
		wglDeleteContext(glw.hGLRC);
		glw.hGLRC = NULL;
		goto hard;
	}

	const int iFetchAttributes[] =
	{
		WGL_COLOR_BITS_ARB,
		WGL_DEPTH_BITS_ARB,
		WGL_STENCIL_BITS_ARB
	};
	wglGetPixelFormatAttribivARB(win.dc, pixelformat, 0, q_countof(iFetchAttributes), iFetchAttributes, &gl_config.colorbits);
	return FAIL_OK;
soft:
	// it failed, clean up
	Win_Shutdown();
	return FAIL_SOFT;
hard:
	Win_Shutdown();
	return FAIL_HARD;
}

#define FAKE_WINDOW_CLASS   "Q2PRO FAKE WINDOW CLASS"
#define FAKE_WINDOW_NAME    "Q2PRO FAKE WINDOW NAME"

static void GetFakeWindowExtensions(void)
{
	WNDCLASSEX wc;
	PIXELFORMATDESCRIPTOR pfd;
	int pixelformat;
	HWND wnd;
	HDC dc;
	HGLRC rc;
	memset(&wc, 0, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = DefWindowProc;
	wc.hInstance = hGlobalInstance;
	wc.lpszClassName = FAKE_WINDOW_CLASS;

	if (!RegisterClassEx(&wc))
		goto fail0;

	wnd = CreateWindow(
			FAKE_WINDOW_CLASS,
			FAKE_WINDOW_NAME,
			0,
			0, 0, 0, 0,
			NULL,
			NULL,
			hGlobalInstance,
			NULL);

	if (!wnd)
		goto fail1;

	if ((dc = GetDC(wnd)) == NULL)
		goto fail2;

	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	if ((pixelformat = ChoosePixelFormat(dc, &pfd)) == 0)
		goto fail3;

	if (SetPixelFormat(dc, pixelformat, &pfd) == FALSE)
		goto fail3;

	if ((rc = wglCreateContext(dc)) == NULL)
		goto fail3;

	if (wglMakeCurrent(dc, rc) == FALSE)
		goto fail4;

	if (!gladLoadWGL(dc))
		goto fail5;

fail5:
	wglMakeCurrent(NULL, NULL);
fail4:
	wglDeleteContext(rc);
fail3:
	ReleaseDC(wnd, dc);
fail2:
	DestroyWindow(wnd);
fail1:
	UnregisterClass(FAKE_WINDOW_CLASS, hGlobalInstance);
fail0:
	;
}

static int LoadGL(const char *driver)
{
	int colorbits = Cvar_ClampInteger(gl_colorbits, 0, 32);
	int depthbits = Cvar_ClampInteger(gl_depthbits, 0, 32);
	int stencilbits = Cvar_ClampInteger(gl_stencilbits, 0, 8);
	int ret;
	// check for extensions by creating a fake window
	GetFakeWindowExtensions();

	if (!GLAD_WGL_ARB_pixel_format)
	{
		Com_EPrintf("WGL_ARB_pixel_format not found\n");
		goto fail;
	}
	else if (!GLAD_WGL_ARB_create_context)
	{
		Com_EPrintf("WGL_ARB_create_context not found\n");
		goto fail;
	}

	// create window, choose PFD, setup OpenGL context
	ret = SetupGL(colorbits, depthbits, stencilbits);

	// attempt to recover
	if (ret == FAIL_SOFT && (colorbits || depthbits || stencilbits))
		ret = SetupGL(0, 0, 0);

	if (ret)
		goto fail;

	return FAIL_OK;
fail:
	return FAIL_SOFT;
}

static void gl_swapinterval_changed(cvar_t *self)
{
	if (self->integer < 0 && GLAD_WGL_EXT_swap_control_tear)
	{
		Com_Printf("Negative swap interval is not supported on this system.\n");
		Cvar_Reset(self);
	}

	if (GLAD_WGL_EXT_swap_control && !wglSwapIntervalEXT(self->integer))
		ReportLastError("wglSwapIntervalEXT");
}

/*
VID_Init

This routine is responsible for initializing the OS specific portions
of OpenGL.  Under Win32 this means dealing with the pixelformats and
doing the wgl interface stuff.
*/
bool VID_Init(void)
{
	int ret;
	gl_driver = Cvar_Get("gl_driver", "opengl32", CVAR_ARCHIVE | CVAR_REFRESH);
	gl_swapinterval = Cvar_Get("gl_swapinterval", "1", CVAR_ARCHIVE);
	gl_colorbits = Cvar_Get("gl_colorbits", "0", CVAR_REFRESH);
	gl_depthbits = Cvar_Get("gl_depthbits", "0", CVAR_REFRESH);
	gl_stencilbits = Cvar_Get("gl_stencilbits", "8", CVAR_REFRESH);
	// don't allow absolute or relative paths
	FS_SanitizeFilenameVariable(gl_driver);
	// load and initialize the OpenGL driver
	ret = LoadGL(gl_driver->string);

	// it failed, abort
	if (ret)
		return false;

	if (GLAD_WGL_EXT_swap_control)
	{
		if (GLAD_WGL_EXT_swap_control_tear)
			Com_Printf("...enabling WGL_EXT_swap_control(_tear)\n");
		else
			Com_Printf("...enabling WGL_EXT_swap_control\n");

		gl_swapinterval->changed = gl_swapinterval_changed;
		gl_swapinterval_changed(gl_swapinterval);
	}
	else
	{
		Com_Printf("WGL_EXT_swap_control not found\n");
		Cvar_Set("gl_swapinterval", "0");
	}

	VID_SetMode();
	return true;
}

void VID_BeginFrame(void)
{
}

/*
VID_EndFrame

Responsible for doing a swapbuffers and possibly for other stuff
as yet to be determined.  Probably better not to make this a VID_
function and instead do a call to VID_SwapBuffers.
*/
void VID_EndFrame(void)
{
	BOOL ret;

	// don't flip if drawing to front buffer
	if (glw.drawbuffer == GL_FRONT)
		return;

	ret = SwapBuffers(win.dc);

	if (!ret)
	{
		DWORD error = GetLastError();

		// this happens sometimes when the window is iconified
		if (!IsIconic(win.wnd))
		{
			Com_Error(ERR_FATAL, "SwapBuffers failed with error %lu",
				error);
		}
	}
}

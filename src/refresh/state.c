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

#include "gl.h"

glState_t gls;

// for uploading
void GL_ForceTexture(GLuint tmu, GLuint texnum)
{
	GL_ActiveTexture(tmu);

	if (gls.texnums[tmu] == texnum)
		return;

	glBindTexture(GL_TEXTURE_2D, texnum);
	gls.texnums[tmu] = texnum;
	c.texSwitches++;
}

// for drawing
void GL_BindTexture(GLuint tmu, GLuint texnum)
{
#ifdef _DEBUG

	if (gl_nobind->integer && !tmu)
		texnum = TEXNUM_DEFAULT;

#endif

	if (gls.texnums[tmu] == texnum)
		return;

	GL_ActiveTexture(tmu);
	glBindTexture(GL_TEXTURE_2D, texnum);
	gls.texnums[tmu] = texnum;
	c.texSwitches++;
}

void GL_CommonStateBits(GLbitfield bits)
{
	GLbitfield diff = bits ^ gls.state_bits;

	// Generations
	if (!diff)
		return;

	if (diff & GLS_BLEND_MASK)
	{
		if (bits & GLS_BLEND_MASK)
		{
			glEnable(GL_BLEND);

			if (bits & GLS_BLEND_BLEND)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			else if (bits & GLS_BLEND_ADD)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			else if (bits & GLS_BLEND_MODULATE)
				glBlendFunc(GL_DST_COLOR, GL_ONE); // Generations
			else if (bits & GLS_BLEND_INVERT)
				glBlendFunc(GL_ONE_MINUS_SRC_COLOR, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
			glDisable(GL_BLEND);
	}

	if (diff & GLS_DEPTHMASK_FALSE)
	{
		if (bits & GLS_DEPTHMASK_FALSE)
			glDepthMask(GL_FALSE);
		else
			glDepthMask(GL_TRUE);
	}

	if (diff & GLS_DEPTHTEST_DISABLE)
	{
		if (bits & GLS_DEPTHTEST_DISABLE)
			glDisable(GL_DEPTH_TEST);
		else
			glEnable(GL_DEPTH_TEST);
	}

	if (diff & GLS_CULL_DISABLE)
	{
		if (bits & GLS_CULL_DISABLE)
			glDisable(GL_CULL_FACE);
		else
			glEnable(GL_CULL_FACE);
	}
}

void GL_Ortho(GLfloat xmin, GLfloat xmax, GLfloat ymin, GLfloat ymax, GLfloat znear, GLfloat zfar)
{
	GLfloat width, height, depth;
	GLfloat matrix[16];
	width = xmax - xmin;
	height = ymax - ymin;
	depth = zfar - znear;
	matrix[0] = 2 / width;
	matrix[4] = 0;
	matrix[8] = 0;
	matrix[12] = -(xmax + xmin) / width;
	matrix[1] = 0;
	matrix[5] = 2 / height;
	matrix[9] = 0;
	matrix[13] = -(ymax + ymin) / height;
	matrix[2] = 0;
	matrix[6] = 0;
	matrix[10] = -2 / depth;
	matrix[14] = -(zfar + znear) / depth;
	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 0;
	matrix[15] = 1;
	gl_static.backend.proj_matrix(matrix);
}

void GL_BeginRenderToTexture(GLuint frameBuffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
}

void GL_EndRenderToTexture(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GL_SetupOrtho(GLfloat x, GLfloat y, GLfloat width, GLfloat height, GLfloat zmin, GLfloat zmax)
{
	glViewport(x, y, width, height);
	GL_Ortho(x, width, height, y, zmin, zmax);
	draw.scale = 1;
	draw.colors[0].u32 = U32_WHITE;
	draw.colors[1].u32 = U32_WHITE;

	if (draw.scissor)
	{
		glDisable(GL_SCISSOR_TEST);
		draw.scissor = false;
	}

	gl_static.backend.view_matrix(NULL);
}

void GL_Setup2D(void)
{
	GL_SetupOrtho(0, 0, r_config.width, r_config.height, -1, 1);
}

void GL_Frustum(void)
{
	GLfloat xmin, xmax, ymin, ymax, zfar, znear;
	GLfloat width, height, depth;
	GLfloat matrix[16];
	znear = gl_znear->value;

	if (glr.fd.rdflags & RDF_NOWORLDMODEL)
		zfar = 2048;
	else
		zfar = gl_static.world.size * 2;

	ymax = znear * tan(glr.fd.fov_y * (M_PI / 360));
	ymin = -ymax;
	xmax = znear * tan(glr.fd.fov_x * (M_PI / 360));
	xmin = -xmax;
	width = xmax - xmin;
	height = ymax - ymin;
	depth = zfar - znear;
	matrix[0] = 2 * znear / width;
	matrix[4] = 0;
	matrix[8] = (xmax + xmin) / width;
	matrix[12] = 0;
	matrix[1] = 0;
	matrix[5] = 2 * znear / height;
	matrix[9] = (ymax + ymin) / height;
	matrix[13] = 0;
	matrix[2] = 0;
	matrix[6] = 0;
	matrix[10] = -(zfar + znear) / depth;
	matrix[14] = -2 * zfar * znear / depth;
	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = -1;
	matrix[15] = 0;
	gl_static.backend.proj_matrix(matrix);
}

static void GL_RotateForViewer(void)
{
	GLfloat *matrix = glr.viewmatrix;
	AnglesToAxis(glr.fd.viewangles, glr.viewaxis);
	// Generations
	vec3_t angles;
	VectorCopy(glr.fd.viewangles, angles);
	angles[0] = angles[2] = 0;
	AnglesToAxis(angles, glr.viewaxis_nopitch);
	matrix[0] = -glr.viewaxis[1][0];
	matrix[4] = -glr.viewaxis[1][1];
	matrix[8] = -glr.viewaxis[1][2];
	matrix[12] = DotProduct(glr.viewaxis[1], glr.fd.vieworg);
	matrix[1] = glr.viewaxis[2][0];
	matrix[5] = glr.viewaxis[2][1];
	matrix[9] = glr.viewaxis[2][2];
	matrix[13] = -DotProduct(glr.viewaxis[2], glr.fd.vieworg);
	matrix[2] = -glr.viewaxis[0][0];
	matrix[6] = -glr.viewaxis[0][1];
	matrix[10] = -glr.viewaxis[0][2];
	matrix[14] = DotProduct(glr.viewaxis[0], glr.fd.vieworg);
	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 0;
	matrix[15] = 1;
	GL_ForceMatrix(matrix);
}

void GL_Setup3D(bool clear)
{
	glViewport(glr.fd.x, r_config.height - (glr.fd.y + glr.fd.height),
		glr.fd.width, glr.fd.height);

	if (gl_static.backend.update)
		gl_static.backend.update();

	GL_Frustum();
	GL_RotateForViewer();
	// enable depth writes before clearing
	GL_StateBits(GLS_DEFAULT);

	if (clear)
		glClear(GL_DEPTH_BUFFER_BIT | gl_static.stencil_buffer_bit);
}

void GL_DrawOutlines(GLsizei count, GL_INDEX_TYPE *indices)
{
	GL_BindTexture(0, TEXNUM_WHITE);
	GL_StateBits(GLS_DEFAULT);
	GL_ArrayBits(GLA_VERTEX);
	GL_Color(1, 1, 1, 1);
	GL_DepthRange(0, 0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	if (indices)
		glDrawElements(GL_TRIANGLES, count, GL_INDEX_ENUM, indices);
	else
		glDrawArrays(GL_TRIANGLES, 0, count);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	GL_DepthRange(0, 1);
}

void GL_ClearState(void)
{
	glClearColor(0, 0, 0, 1);
	GL_ClearDepth(1);
	glClearStencil(0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	GL_DepthRange(0, 1);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	gl_static.backend.clear();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | gl_static.stencil_buffer_bit);
	memset(&gls, 0, sizeof(gls));
	GL_ShowErrors(__func__);
}

void GL_InitState(void)
{
	gl_static.backend = backend_shader;
	gl_static.backend.init();
	Com_Printf("Using %s rendering backend.\n", gl_static.backend.name);
}

void GL_ShutdownState(void)
{
	gl_static.backend.shutdown();
}

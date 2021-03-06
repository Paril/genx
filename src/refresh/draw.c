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

drawStatic_t draw;

static inline void _GL_StretchPic(
	float x, float y, float w, float h,
	float s1, float t1, float s2, float t2,
	uint32_t color, int texnum, int flags)
{
	vec_t *dst_vert;
	uint32_t *dst_color;
	GL_INDEX_TYPE *dst_indices;

	if (tess.numverts + 4 > TESS_MAX_VERTICES ||
		tess.numindices + 6 > TESS_MAX_INDICES ||
		(tess.numverts && tess.texnum[0] != texnum))
		GL_Flush2D();

	tess.texnum[0] = texnum;
	dst_vert = tess.vertices + tess.numverts * 4;
	Vector4Set(dst_vert, x, y, s1, t1);
	Vector4Set(dst_vert + 4, x + w, y, s2, t1);
	Vector4Set(dst_vert + 8, x + w, y + h, s2, t2);
	Vector4Set(dst_vert + 12, x, y + h, s1, t2);
	dst_color = (uint32_t *)tess.colors + tess.numverts;
	dst_color[0] = color;
	dst_color[1] = color;
	dst_color[2] = color;
	dst_color[3] = color;
	dst_indices = tess.indices + tess.numindices;
	dst_indices[0] = tess.numverts + 0;
	dst_indices[1] = tess.numverts + 2;
	dst_indices[2] = tess.numverts + 3;
	dst_indices[3] = tess.numverts + 0;
	dst_indices[4] = tess.numverts + 1;
	dst_indices[5] = tess.numverts + 2;

	if (flags & IF_TRANSPARENT)
	{
		if (flags & IF_PALETTED)
			tess.flags |= 1;
		else
			tess.flags |= 2;
	}

	if ((color & U32_ALPHA) != U32_ALPHA)
		tess.flags |= 2;

	tess.numverts += 4;
	tess.numindices += 6;
}

void GL_StretchPic(
	float x, float y, float w, float h,
	float s1, float t1, float s2, float t2,
	uint32_t color, image_t *image)
{
	_GL_StretchPic(x, y, w, h, s1, t1, s2, t2, color, image->texnum, image->flags);
}

void GL_Blend(void)
{
	color_t color;
	color.u8[0] = glr.fd.blend[0] * 255;
	color.u8[1] = glr.fd.blend[1] * 255;
	color.u8[2] = glr.fd.blend[2] * 255;
	color.u8[3] = glr.fd.blend[3] * 255;
	_GL_StretchPic(glr.fd.x, glr.fd.y, glr.fd.width, glr.fd.height, 0, 0, 1, 1,
		color.u32, TEXNUM_WHITE, 0);
}

void R_ClearColor(void)
{
	draw.colors[0].u32 = U32_WHITE;
	draw.colors[1].u32 = U32_WHITE;
}

void R_SetAlpha(float alpha)
{
	draw.colors[0].u8[3] =
		draw.colors[1].u8[3] = alpha * 255;
}

void R_SetColor(uint32_t color)
{
	draw.colors[0].u32 = color;
	draw.colors[1].u8[3] = draw.colors[0].u8[3];
}

color_t R_GetColor()
{
	return draw.colors[0];
}

void R_SetClipRect(const clipRect_t *clip)
{
	clipRect_t rc;
	float scale;
	GL_Flush2D();

	if (!clip)
	{
clear:

		if (draw.scissor)
		{
			glDisable(GL_SCISSOR_TEST);
			draw.scissor = false;
		}

		return;
	}

	scale = 1 / draw.scale;
	rc.left = clip->left * scale;
	rc.top = clip->top * scale;
	rc.right = clip->right * scale;
	rc.bottom = clip->bottom * scale;

	if (rc.left < 0)
		rc.left = 0;

	if (rc.top < 0)
		rc.top = 0;

	if (rc.right > r_config.width)
		rc.right = r_config.width;

	if (rc.bottom > r_config.height)
		rc.bottom = r_config.height;

	if (rc.right < rc.left)
		goto clear;

	if (rc.bottom < rc.top)
		goto clear;

	glEnable(GL_SCISSOR_TEST);
	glScissor(rc.left, r_config.height - rc.bottom,
		rc.right - rc.left, rc.bottom - rc.top);
	draw.scissor = true;
}

float R_ClampScale(cvar_t *var)
{
	if (!var)
		return 1.0f;

	if (var->value)
		return 1.0f / Cvar_ClampValue(var, 1.0f, 10.0f);

	if (r_config.width * r_config.height >= 2560 * 1440)
		return 0.25f;

	if (r_config.width * r_config.height >= 1280 * 720)
		return 0.5f;

	return 1.0f;
}

void R_SetScale(float scale)
{
	if (draw.scale == scale)
		return;

	GL_Flush2D();
	GL_Ortho(0, Q_rint(r_config.width * scale),
		Q_rint(r_config.height * scale), 0, -1, 1);
	draw.scale = scale;
}

void R_DrawStretchPic(int x, int y, int w, int h, qhandle_t pic, gametype_t game)
{
	image_t *image = IMG_ForHandle(pic, game);
	GL_StretchPic(x, y, w, h, image->sl, image->tl, image->sh, image->th,
		draw.colors[0].u32, image);
}

void R_DrawPic(int x, int y, qhandle_t pic, gametype_t game)
{
	image_t *image = IMG_ForHandle(pic, game);

	if (image)
		GL_StretchPic(x, y, image->width, image->height,
			image->sl, image->tl, image->sh, image->th, draw.colors[0].u32, image);
	else
		_GL_StretchPic(x, y, 1, 1, 0, 0, 1, 1, draw.colors[0].u32, TEXNUM_WHITE, 0);
}

#define DIV64 (1.0f / 64.0f)

void R_TileClear(int x, int y, int w, int h, qhandle_t pic, gametype_t game)
{
	GL_StretchPic(x, y, w, h, x * DIV64, y * DIV64,
		(x + w) * DIV64, (y + h) * DIV64, U32_WHITE, IMG_ForHandle(pic, game));
}

void R_DrawFill8(int x, int y, int w, int h, int ch)
{
	if (!w || !h)
		return;

	_GL_StretchPic(x, y, w, h, 0, 0, 1, 1, d_palettes[GAME_Q2][ch & 0xff], TEXNUM_WHITE, 0);
}

void R_DrawFill32(int x, int y, int w, int h, uint32_t color)
{
	if (!w || !h)
		return;

	_GL_StretchPic(x, y, w, h, 0, 0, 1, 1, color, TEXNUM_WHITE, 0);
}

static inline void draw_char(int x, int y, int flags, int ch, image_t *image)
{
	float s, t;

	if ((ch & 127) == 32)
		return;

	if (flags & UI_ALTCOLOR)
		ch |= 0x80;

	if (flags & UI_XORCOLOR)
		ch ^= 0x80;

	float w = image->width;
	float h = image->height;
	float chw = CHAR_WIDTH;

	if (image->flags & IF_SCRAP)
	{
		w = SCRAP_BLOCK_WIDTH;
		h = SCRAP_BLOCK_HEIGHT;
	}

	if (image->font_widths)
	{
		s = image->font_x[ch] / w;
		w = (chw = image->font_widths[ch]) / w;
		h = (CHAR_WIDTH / h);
		t = 0;
	}
	else
	{
		w = (CHAR_WIDTH / w);
		h = (CHAR_WIDTH / h);
		s = image->sl + (ch & 15) * w;
		t = image->tl + (ch >> 4) * h;
	}

	if (gl_fontshadow->integer > 0 && ch != 0x83)
	{
		uint32_t black = MakeColor(0, 0, 0, draw.colors[0].u8[3]);
		GL_StretchPic(x + 1, y + 1, CHAR_WIDTH, CHAR_HEIGHT, s, t,
			s + w, t + h, black, image);

		if (gl_fontshadow->integer > 1)
			GL_StretchPic(x + 2, y + 2, CHAR_WIDTH, CHAR_HEIGHT, s, t,
				s + w, t + h, black, image);
	}

	GL_StretchPic(x, y, chw, CHAR_HEIGHT, s, t,
		s + w, t + h, draw.colors[ch >> 7].u32, image);
}

void R_DrawChar(int x, int y, int flags, int ch, qhandle_t font, gametype_t game)
{
	draw_char(x, y, flags, ch & 255, IMG_ForHandle(font, game));
}

int R_StringWidth(size_t maxlen, const char *s, qhandle_t font, gametype_t game)
{
	image_t *image = IMG_ForHandle(font, game);
	int x = 0;

	if (!image)
		return 0;

	while (maxlen-- && *s)
	{
		byte ch = *s++;
		x += image->font_widths ? image->font_widths[ch] : CHAR_WIDTH;
	}

	return x;
}

int R_DrawString(int x, int y, int flags, size_t maxlen, const char *s, qhandle_t font, gametype_t game)
{
	image_t *image = IMG_ForHandle(font, game);

	if (!image)
		return x;

	while (maxlen-- && *s)
	{
		byte ch = *s++;
		draw_char(x, y, flags, ch, image);
		x += image->font_widths ? image->font_widths[ch] : CHAR_WIDTH;
	}

	return x;
}

#ifdef _DEBUG

image_t *r_charset;

void Draw_Stringf(int x, int y, const char *fmt, ...)
{
	va_list argptr;
	char buffer[MAX_STRING_CHARS];
	char *string;
	byte ch;
	float s, t;
	float w = r_charset->width;
	float h = r_charset->height;

	if (r_charset->flags & IF_SCRAP)
	{
		w = SCRAP_BLOCK_WIDTH;
		h = SCRAP_BLOCK_HEIGHT;
	}

	w = (CHAR_WIDTH / w);
	h = (CHAR_WIDTH / h);
	va_start(argptr, fmt);
	Q_vsnprintf(buffer, sizeof(buffer), fmt, argptr);
	va_end(argptr);
	string = buffer;

	while (*string)
	{
		ch = *string++;
		s = r_charset->sl + (ch & 15) * w;
		t = r_charset->tl + (ch >> 4) * h;
		GL_StretchPic(x, y, CHAR_WIDTH, CHAR_HEIGHT, s, t,
			s + w, t + h, U32_WHITE, r_charset);
		x += CHAR_WIDTH;
	}
}

void Draw_Stats(void)
{
	int x = 10, y = 10;

	if (!r_charset)
	{
		qhandle_t tmp;
		tmp = R_RegisterFont("%conchars");

		if (!tmp)
			return;

		r_charset = IMG_ForHandle(tmp, CL_GetClientGame());
	}

	Draw_Stringf(x, y, "Nodes visible  : %i", c.nodesVisible);
	y += 10;
	Draw_Stringf(x, y, "Nodes culled   : %i", c.nodesCulled);
	y += 10;
	Draw_Stringf(x, y, "Nodes drawn    : %i", c.nodesDrawn);
	y += 10;
	Draw_Stringf(x, y, "Leaves drawn   : %i", c.leavesDrawn);
	y += 10;
	Draw_Stringf(x, y, "Faces drawn    : %i", c.facesDrawn);
	y += 10;

	if (c.facesCulled)
	{
		Draw_Stringf(x, y, "Faces culled   : %i", c.facesCulled);
		y += 10;
	}

	if (c.boxesCulled)
	{
		Draw_Stringf(x, y, "Boxes culled   : %i", c.boxesCulled);
		y += 10;
	}

	if (c.spheresCulled)
	{
		Draw_Stringf(x, y, "Spheres culled : %i", c.spheresCulled);
		y += 10;
	}

	if (c.rotatedBoxesCulled)
	{
		Draw_Stringf(x, y, "RtBoxes culled : %i", c.rotatedBoxesCulled);
		y += 10;
	}

	Draw_Stringf(x, y, "Tris drawn   : %i", c.trisDrawn);
	y += 10;
	Draw_Stringf(x, y, "Tex switches : %i", c.texSwitches);
	y += 10;

	if (c.texUploads)
	{
		Draw_Stringf(x, y, "Tex uploads  : %i", c.texUploads);
		y += 10;
	}

	if (c.batchesDrawn)
	{
		Draw_Stringf(x, y, "Batches drawn: %i", c.batchesDrawn);
		y += 10;
		Draw_Stringf(x, y, "Faces / batch: %.1f", (float)c.facesDrawn / c.batchesDrawn);
		y += 10;
		Draw_Stringf(x, y, "Tris / batch : %.1f", (float)c.facesTris / c.batchesDrawn);
		y += 10;
	}

	Draw_Stringf(x, y, "2D batches   : %i", c.batchesDrawn2D);
	y += 10;
	Draw_Stringf(x, y, "Location     : %f %f %f", glr.fd.vieworg[0], glr.fd.vieworg[1], glr.fd.vieworg[2]);
	y += 10;
}

void Draw_Lightmaps(void)
{
	int i, x, y;

	for (i = 0; i < lm.nummaps; i++)
	{
		x = i & 1;
		y = i >> 1;
		_GL_StretchPic(256 * x, 256 * y, 256, 256,
			0, 0, 1, 1, U32_WHITE, lm.texnums[i], 0);
	}
}

void Draw_Scrap(void)
{
	_GL_StretchPic(0, 0, R_FRAMEBUFFERTEXTURE->width, R_FRAMEBUFFERTEXTURE->height,
		0, 0, 1, 1, U32_WHITE, TEXNUM_FRAMEBUFFER, IF_TRANSPARENT);
}

#endif

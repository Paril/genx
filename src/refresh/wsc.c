/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2008 Andrey Nazarov

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
#include "models.h"

void GL_StretchPic(
	float x, float y, float w, float h,
	float s1, float t1, float s2, float t2,
	uint32_t color, image_t *image);

#define GL_WeaponPic(frame, drawFrame, x, y) \
	GL_StretchPic(	(x) * heightAspectInverted, (y) * heightAspectInverted, frame->width * heightAspectInverted, frame->height * heightAspectInverted, \
		(drawFrame->flip & flip_x) ? frame->image->sh : frame->image->sl, (drawFrame->flip & flip_y) ? frame->image->th : frame->image->tl, \
		(drawFrame->flip & flip_x) ? frame->image->sl : frame->image->sh, (drawFrame->flip & flip_y) ? frame->image->tl : frame->image->th, \
		draw.colors[0].u32, frame->image)


static void PointFromSizeAnchor(vec2_t out, const vec2_t pos, const vec2_t size, mweaponscript_draw_anchor_t anchor)
{
	if (anchor == anchor_bottom_left ||
		anchor == anchor_center_left ||
		anchor == anchor_top_left)
		out[0] = pos[0];
	else if (anchor == anchor_bottom_right ||
			 anchor == anchor_center_right ||
			 anchor == anchor_top_right)
		out[0] = pos[0] + size[0];
	else
		out[0] = pos[0] + size[0] / 2;

	if (anchor == anchor_top_right ||
		anchor == anchor_top_center ||
		anchor == anchor_top_left)
		out[1] = pos[1];
	else if (anchor == anchor_bottom_right ||
			 anchor == anchor_bottom_center ||
			 anchor == anchor_bottom_left)
		out[1] = pos[1] + size[1];
	else
		out[1] = pos[1] + size[1] / 2;
}

static void R_SetWeaponSpriteScale(float scale, float width, float height)
{
	GL_Flush2D();
	GL_Ortho(0, width, height, 0, -1, 1);
	draw.scale = scale;
}

extern vrect_t     scr_vrect;

void R_DrawWeaponSprite(qhandle_t handle, gunindex_e index, float velocity, int time, float frametime, int oldframe, int frame, float frac, int hud_width, int hud_height, bool firing, float yofs, float scale, color_t color, bool invul, float height_diff)
{
	model_t *model = MOD_ForHandle(handle, CL_GetClientGame());

	if (!model)
		return;

	if (model->type != MOD_WEAPONSCRIPT)
		Com_Error(ERR_FATAL, "What");

	mweaponscript_t *script = model->weaponscript;
#define WEAPONTOP (script->view[1] / 2.0)
	// bob the weapon based on movement speed
	static float bob[MAX_PLAYER_GUNS] = { 0 };
	float wanted_bob;

	if (!firing)
		wanted_bob = min(6.4f, velocity * 0.02f);
	else
		wanted_bob = 0;

	if (wanted_bob > bob[index])
		bob[index] = min(wanted_bob, bob[index] + frametime * 15);
	else if (wanted_bob < bob[index])
		bob[index] = max(wanted_bob, bob[index] - frametime * 15);

	const float bob_x_scale = (CL_GetClientGame() == GAME_DUKE ? 1 : 2);
	const float bob_y_scale = (CL_GetClientGame() == GAME_DUKE ? 1.5 : 2.5);
	float angle = ((128.0f / 40000) * time);
	float bob_x = (bob[index] * sin(angle)) * bob_x_scale;
	angle *= 2;
	float bob_y = (bob[index] * fabs(cos(angle / 2.0))) * bob_y_scale;

	if (frame >= script->num_anim_frames)
		return;

	float oldScale = draw.scale;
	R_SetWeaponSpriteScale(1.0, r_config.width, r_config.height);
	float heightAspect = script->view[1] / (r_config.height - height_diff);
	float realWidth = r_config.width * heightAspect;
	float sidePadding = (realWidth - script->view[0]) / 2.0f;
	float heightAspectInverted = 1.0 / heightAspect;
	const vec2_t realScreenSize = { realWidth, script->view[1] };
	const vec2_t realScreenPosition = { -sidePadding, 0 };
	mweaponscript_frame_t *of = &script->frames[script->anim_frames[oldframe]];
	mweaponscript_frame_t *f = &script->frames[script->anim_frames[frame]];
	R_SetColor(color.u32);
	GL_Flush2D();
	static const vec2_t virtualPosition = { 0, 0 };
	const vec_t *virtualSize = &script->view[0];
	int i;
	yofs = WEAPONTOP * yofs;

	for (i = script->num_sprites - 1; i >= 0; i--)
	{
		if (!f->draws[i].draw)
			continue;

		const model_t *sprite = MOD_ForHandle(script->sprites[i].model, CL_GetClientGame());
		const mweaponscript_frame_draw_t *drawFrame = &f->draws[i];
		const mspritedirframe_t *spriteFrame = &sprite->spritedirframes[drawFrame->sprite_frame].directions[0];
		const vec2_t objectSize = { spriteFrame->image->width, spriteFrame->image->height };
		vec2_t scr, pos;
		PointFromSizeAnchor(scr, drawFrame->of_fullscreen ? realScreenPosition : virtualPosition, drawFrame->of_fullscreen ? realScreenSize : virtualSize, drawFrame->at);
		PointFromSizeAnchor(pos, virtualPosition, objectSize, drawFrame->my);
		float curX = sidePadding + (scr[0] - pos[0] + drawFrame->offset[0]);
		float curY = scr[1] - pos[1] + drawFrame->offset[1];
		vec2_t translate = { f->translate[0], f->translate[1] };

		if (f->lerp)
		{
			if ((f->lerp & lerp_draws) && of->draws[i].draw)
			{
				const mweaponscript_frame_draw_t *prevDrawFrame = &of->draws[i];
				const mspritedirframe_t *prevSpriteFrame = &sprite->spritedirframes[prevDrawFrame->sprite_frame].directions[0];
				const vec2_t prevObjectSize = { prevSpriteFrame->image->width, prevSpriteFrame->image->height };
				vec2_t prevScr, prevPos;
				PointFromSizeAnchor(prevScr, prevDrawFrame->of_fullscreen ? realScreenPosition : virtualPosition, prevDrawFrame->of_fullscreen ? realScreenSize : virtualSize, prevDrawFrame->at);
				PointFromSizeAnchor(prevPos, virtualPosition, prevObjectSize, prevDrawFrame->my);
				const float prevX = sidePadding + (prevScr[0] - prevPos[0] + prevDrawFrame->offset[0]);
				const float prevY = prevScr[1] - prevPos[1] + prevDrawFrame->offset[1];
				curX = LerpFloat(prevX, curX, frac);
				curY = LerpFloat(prevY, curY, frac);
			}

			if (f->lerp & lerp_translates)
			{
				translate[0] = LerpFloat(of->translate[0], translate[0], frac);
				translate[1] = LerpFloat(of->translate[1], translate[1], frac);
			}
		}

		GL_WeaponPic(spriteFrame, drawFrame, curX + translate[0] + bob_x, curY + translate[1] + yofs + bob_y);
	}

	if (invul)
		tess.flags |= 4;

	GL_Flush2D();
	R_SetColor(colorTable[COLOR_WHITE]);
	R_SetScale(oldScale);
}

static mweaponscript_draw_anchor_t ParseAnchor(const char *token)
{
	if (Q_stricmp(token, "top left") == 0)
		return anchor_top_left;
	else if (Q_stricmp(token, "top center") == 0)
		return anchor_top_center;
	else if (Q_stricmp(token, "top right") == 0)
		return anchor_top_right;
	else if (Q_stricmp(token, "center right") == 0)
		return anchor_center_right;
	else if (Q_stricmp(token, "bottom right") == 0)
		return anchor_bottom_right;
	else if (Q_stricmp(token, "bottom center") == 0)
		return anchor_bottom_center;
	else if (Q_stricmp(token, "bottom left") == 0)
		return anchor_bottom_left;
	else if (Q_stricmp(token, "center left") == 0)
		return anchor_center_left;

	return anchor_center;
}

int MOD_LoadWSC(model_t *model, const void *rawdata, size_t length)
{
	model->type = MOD_WEAPONSCRIPT;
	const char *file = (const char *)rawdata;
	char *token;

	if ((token = COM_Parse(&file))[0] != '{')
		return Q_ERR_INVALID_FORMAT;

	const char *num_searcher = file;
	int num_frames = 0, num_sprites = 0, num_anim_frames = 0;

	while (true)
	{
		token = COM_Parse(&num_searcher);

		if (!token || !num_searcher)
			break;

		if (Q_stricmp(token, "{") == 0)
			num_frames++;
		else if (Q_stricmp(token, "load_sprite") == 0)
			num_sprites++;
		else if (Q_stricmp(token, "animations") == 0)
		{
			token = COM_Parse(&num_searcher);
			const char *anim_searcher = token;

			while (true)
			{
				token = COM_Parse(&anim_searcher);

				if (!token || !anim_searcher)
					break;

				num_anim_frames++;
			}
		}
	}

	Z_TagChunkCreate(TAG_MODEL, &model->memory, sizeof(mweaponscript_t) +
		(sizeof(mweaponscript_sprite_t) * num_sprites) +
					 ((sizeof(mweaponscript_frame_t) + (sizeof(mweaponscript_frame_draw_t) * num_sprites)) * num_frames) +
					 (sizeof(int) * num_anim_frames));
	mweaponscript_t *script_ptr = model->weaponscript = Z_ChunkAlloc(&model->memory, sizeof(mweaponscript_t));
	script_ptr->num_frames = num_frames;
	script_ptr->num_sprites = num_sprites;
	script_ptr->num_anim_frames = num_anim_frames;
	script_ptr->sprites = Z_ChunkAlloc(&model->memory, sizeof(mweaponscript_sprite_t) * script_ptr->num_sprites);
	script_ptr->frames = Z_ChunkAlloc(&model->memory, sizeof(mweaponscript_frame_t) * script_ptr->num_frames);
	script_ptr->anim_frames = Z_ChunkAlloc(&model->memory, sizeof(int) * script_ptr->num_anim_frames);

	for (int i = 0; i < script_ptr->num_frames; ++i)
		script_ptr->frames[i].draws = Z_ChunkAlloc(&model->memory, sizeof(mweaponscript_frame_draw_t) * script_ptr->num_sprites);

	int cur_sprite = 0;
	int cur_frame = 0;
	int cur_anim = 0;

	while ((token = COM_Parse(&file))[0] != '}')
	{
		if (!Q_stricmp(token, "view"))
		{
			token = COM_Parse(&file);
			sscanf(token, "%f %f", &script_ptr->view[0], &script_ptr->view[1]);
		}
		else if (!Q_stricmp(token, "load_sprite"))
		{
			token = COM_Parse(&file);
			script_ptr->sprites[cur_sprite++].model = R_RegisterModel(token);
		}
		else if (!Q_stricmp(token, "animations"))
		{
			token = COM_Parse(&file);
			const char *anim_searcher = token;

			while (true)
			{
				token = COM_Parse(&anim_searcher);

				if (!token || !anim_searcher)
					break;

				script_ptr->anim_frames[cur_anim++] = atoi(token);
			}
		}
		else if (!Q_stricmp(token, "{"))
		{
			int frame_id = cur_frame++;
			mweaponscript_frame_t *frame = &script_ptr->frames[frame_id];

			while ((token = COM_Parse(&file))[0] != '}')
			{
				if (!Q_strncasecmp(token, "draw_sprite", 11))
				{
					int sprite_id = atoi(COM_Parse(&file));

					if (!Q_strncasecmp(token, "draw_sprite_flip_", 17))
					{
						const char *whom = token + 17;

						while (*whom)
						{
							if (*whom == 'x')
								frame->draws[sprite_id].flip |= flip_x;

							if (*whom == 'y')
								frame->draws[sprite_id].flip |= flip_y;

							whom++;
						}
					}

					int sprite_frame_id = atoi(COM_Parse(&file));
					frame->draws[sprite_id].draw = true;
					frame->draws[sprite_id].sprite_frame = sprite_frame_id;
					frame->draws[sprite_id].at = ParseAnchor(COM_Parse(&file));
					frame->draws[sprite_id].my = ParseAnchor(COM_Parse(&file));
					token = COM_Parse(&file);
					sscanf(token, "%f %f", &frame->draws[sprite_id].offset[0], &frame->draws[sprite_id].offset[1]);
					token = COM_Parse(&file);
					frame->draws[sprite_id].of_fullscreen = (Q_stricmp(token, "full") == 0);
				}
				else if (!Q_stricmp(token, "lerp"))
				{
					token = COM_Parse(&file);
					const char *lerp_searcher = token;

					while (true)
					{
						token = COM_Parse(&lerp_searcher);

						if (!token || !lerp_searcher)
							break;

						if (!Q_stricmp(token, "draws"))
							frame->lerp |= lerp_draws;
						else if (!Q_stricmp(token, "translates"))
							frame->lerp |= lerp_translates;
					}
				}
				else if (!Q_stricmp(token, "translate"))
				{
					token = COM_Parse(&file);
					sscanf(token, "%f %f", &frame->translate[0], &frame->translate[1]);
				}
			}
		}
	}

	return Q_ERR_SUCCESS;
}

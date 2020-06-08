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

#include "g_local.h"
#include "m_player.h"

static  edict_t     *current_player;
static  gclient_t   *current_client;

static  vec3_t  forward, right, up;
float   xyspeed;

float   bobmove;
int     bobcycle;       // odd cycles are right foot going forward
float   bobfracsin;     // sin(bobfrac*M_PI)

/*
===============
SV_CalcRoll

===============
*/
float SV_CalcRoll(vec3_t angles, vec3_t velocity)
{
	float   sign;
	float   side;
	float   value;
	side = DotProduct(velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabsf(side);
	value = sv_rollangle->value;

	if (side < sv_rollspeed->value)
		side = side * value / sv_rollspeed->value;
	else
		side = value;

	return side * sign;
}


/*
===============
P_DamageFeedback

Handles color blends and view kicks
===============
*/
void P_DamageFeedback(edict_t *player)
{
	gclient_t   *client;
	float   side;
	float   realcount, count, kick;
	vec3_t  v;
	int     r, l;
	static  vec3_t  power_color = { 0.0f, 1.0f, 0.0f };
	static  vec3_t  acolor = { 1.0f, 1.0f, 1.0f };
	static  vec3_t  bcolor = { 1.0f, 0.0f, 0.0f };
	client = player->server.client;

	// flash the backgrounds behind the status numbers
	if (player->server.state.game == GAME_Q2)
	{
		client->server.ps.stats.flashes = 0;

		if (client->damage_blood)
			client->server.ps.stats.flashes |= 1;

		if (client->damage_armor && !(player->flags & FL_GODMODE) && (client->invincible_time <= level.time))
			client->server.ps.stats.flashes |= 2;
	}

	// total points of damage shot at the player this frame
	count = (client->damage_blood + client->damage_armor + client->damage_parmor);

	if (count == 0)
		return;     // didn't take any damage

	// start a pain animation if still in the player model
	if (client->anim_priority < ANIM_PAIN && player->server.state.modelindex == MODEL_HANDLE_PLAYER)
	{
		static int      i;
		client->anim_priority = ANIM_PAIN;

		switch (player->server.state.game)
		{
			case GAME_Q2:
				if (client->server.ps.pmove.pm_flags & PMF_DUCKED)
				{
					player->server.state.frame = FRAME_crpain1 - 1;
					client->anim_end = FRAME_crpain4;
				}
				else
				{
					i = (i + 1) % 3;

					switch (i)
					{
						case 0:
							player->server.state.frame = FRAME_pain101 - 1;
							client->anim_end = FRAME_pain104;
							break;

						case 1:
							player->server.state.frame = FRAME_pain201 - 1;
							client->anim_end = FRAME_pain204;
							break;

						case 2:
							player->server.state.frame = FRAME_pain301 - 1;
							client->anim_end = FRAME_pain304;
							break;
					}
				}

				break;

			case GAME_Q1:
				if (client->pers.weapon && ITEM_INDEX(client->pers.weapon) == ITI_Q1_AXE)
				{
					player->server.state.frame = 29 - 1;
					client->anim_end = 34;
				}
				else
				{
					player->server.state.frame = 35 - 1;
					client->anim_end = 40;
				}

				break;

			case GAME_DOOM:
				player->server.state.frame = 6;
				client->anim_end = 6;
				client->hold_frame = level.time + 200;
				break;
		}
	}

	if (player->server.state.game == GAME_Q1)
		client->server.ps.stats.q1.face_anim = 5;

	realcount = count;

	if (count < 10)
		count = 10; // always make a visible effect

	// play an apropriate pain sound
	if ((level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) && (client->invincible_time <= level.time))
	{
		switch (player->server.state.game)
		{
			case GAME_DOOM:
				player->pain_debounce_time = level.time + 1000;
				gi.sound(player, CHAN_VOICE, gi.soundindex("doom/PLPAIN.wav"), 1, ATTN_NORM, 0);
				break;

			case GAME_Q2:
				r = 1 + (Q_rand() & 1);
				player->pain_debounce_time = level.time + 700;

				if (player->health < 25)
					l = 25;
				else if (player->health < 50)
					l = 50;
				else if (player->health < 75)
					l = 75;
				else
					l = 100;

				gi.sound(player, CHAN_VOICE, gi.soundindex(va("*pain%i_%i.wav", l, r)), 1, ATTN_NORM, 0);
				break;

			case GAME_Q1:

				// slime pain sounds
				if (player->watertype & CONTENTS_SLIME)
				{
					// FIX ME	put in some steam here
					//if (self.waterlevel == 3)
					//	DeathBubbles(1);
					if (random() > 0.5f)
						gi.sound(player, CHAN_VOICE, gi.soundindex("q1/player/lburn1.wav"), 1, ATTN_NORM, 0);
					else
						gi.sound(player, CHAN_VOICE, gi.soundindex("q1/player/lburn2.wav"), 1, ATTN_NORM, 0);
				}
				else if (player->watertype & CONTENTS_LAVA)
				{
					if (random() > 0.5f)
						gi.sound(player, CHAN_VOICE, gi.soundindex("q1/player/lburn1.wav"), 1, ATTN_NORM, 0);
					else
						gi.sound(player, CHAN_VOICE, gi.soundindex("q1/player/lburn2.wav"), 1, ATTN_NORM, 0);
				}
				else if (player->meansOfDeath.attacker_type == ET_PLAYER &&
					player->meansOfDeath.damage_means == MD_WEAPON &&
					player->meansOfDeath.attacker->server.state.game == GAME_Q1 &&
					player->meansOfDeath.weapon_id == ITI_Q1_AXE)
				{
					player->pain_debounce_time = level.time + 500;
					gi.sound(player, CHAN_VOICE, gi.soundindex("q1/player/axhit1.wav"), 1, ATTN_NORM, 0);
				}
				else
				{
					player->pain_debounce_time = level.time + 500;
					r = Q_rint((random() * 5) + 1);
					const char *noise;

					if (r == 1)
						noise = "q1/player/pain1.wav";
					else if (r == 2)
						noise = "q1/player/pain2.wav";
					else if (r == 3)
						noise = "q1/player/pain3.wav";
					else if (r == 4)
						noise = "q1/player/pain4.wav";
					else if (r == 5)
						noise = "q1/player/pain5.wav";
					else
						noise = "q1/player/pain6.wav";

					gi.sound(player, CHAN_VOICE, gi.soundindex(noise), 1, ATTN_NORM, 0);
				}

				break;

			case GAME_DUKE:
				if (player->meansOfDeath.damage_means == MD_FALLING)
					gi.sound(player, CHAN_VOICE, gi.soundindex("duke/PAIN39.wav"), 1, ATTN_NORM, 0);
				else
				{
					const char *sound;
					r = Q_rand() % 256;

					if (player->health < 50)
					{
						if (r < 64)
							sound = "duke/dscrem15.wav";
						else if (r < 128)
							sound = "duke/dscrem16.wav";
						else if (r < 190)
							sound = "duke/dscrem17.wav";
						else
							sound = "duke/DMDEATH.wav";
					}
					else
					{
						if (r < 64)
							sound = "duke/pain54.wav";
						else if (r < 128)
							sound = "duke/pain75.wav";
						else if (r < 190)
							sound = "duke/pain93.wav";
						else
							sound = "duke/pain68.wav";
					}

					gi.sound(player, CHAN_VOICE, gi.soundindex(sound), 1, ATTN_NORM, 0);
				}

				current_player->pain_debounce_time = level.time + 500;
				break;
		}
	}

	// the total alpha of the blend is always proportional to count
	if (client->damage_alpha < 0)
		client->damage_alpha = 0;

	client->damage_alpha += count * 0.01f;

	if (client->damage_alpha < 0.2f)
		client->damage_alpha = 0.2f;

	if (client->damage_alpha > 0.6f)
		client->damage_alpha = 0.6f;     // don't go too saturated

	// the color of the blend will vary based on how much was absorbed
	// by different armors
	VectorClear(v);

	if (client->damage_parmor)
		VectorMA(v, (float)client->damage_parmor / realcount, power_color, v);

	if (client->damage_armor)
		VectorMA(v, (float)client->damage_armor / realcount, acolor, v);

	if (client->damage_blood)
		VectorMA(v, (float)client->damage_blood / realcount, bcolor, v);

	VectorCopy(v, client->damage_blend);
	//
	// calculate view angle kicks
	//
	kick = abs(client->damage_knockback);

	if (kick && player->health > 0)   // kick of 0 means no view adjust at all
	{
		kick = kick * 100 / player->health;

		if (kick < count * 0.5f)
			kick = count * 0.5f;

		if (kick > 50)
			kick = 50;

		VectorSubtract(client->damage_from, player->server.state.origin, v);
		VectorNormalize(v);
		side = DotProduct(v, right);
		client->v_dmg_roll = kick * side * 0.3f;
		side = -DotProduct(v, forward);
		client->v_dmg_pitch = kick * side * 0.3f;
		client->v_dmg_time = level.time + DAMAGE_TIME;
	}

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_parmor = 0;
	client->damage_knockback = 0;
}




/*
===============
SV_CalcViewOffset

Auto pitching on slopes?

  fall from 128: 400 = 160000
  fall from 256: 580 = 336400
  fall from 384: 720 = 518400
  fall from 512: 800 = 640000
  fall from 640: 960 =

  damage = deltavelocity*deltavelocity  * 0.0001

===============
*/
void SV_CalcViewOffset(edict_t *ent)
{
	float       *angles;
	float       bob;
	float       ratio;
	float       delta;
	vec3_t      v;

	if (ent->server.state.game != GAME_Q2)
	{
		VectorClear(ent->server.client->server.ps.viewoffset);
		ent->server.client->server.ps.viewoffset[2] = ent->viewheight;

		if (ent->server.state.game == GAME_DOOM)
		{
			if (ent->health <= 0)
			{
				if (ent->server.client->fall_value > -24)
					ent->server.client->fall_value -= 3.0f;

				if (ent->server.client->fall_value < -24)
					ent->server.client->fall_value = -24;

				ent->server.client->server.ps.viewoffset[2] = 22 + ent->server.client->fall_value;
			}
			else if (ent->server.client->fall_value > 0)
			{
				if (ent->server.client->fall_value < 0)
					ent->server.client->fall_value = 0;

				ent->server.client->server.ps.viewoffset[2] -= ent->server.client->fall_value / 2;
				ent->server.client->fall_value -= 5.0f;
			}
		}
		else if (ent->server.state.game == GAME_DUKE)
		{
			if (ent->health > 0)
			{
				if (ent->server.client->fall_value < 0)
					ent->server.client->fall_value = 0;

				ent->server.client->server.ps.viewoffset[2] -= ent->server.client->fall_value;
				ent->server.client->fall_value -= 8.0f;
				bob = (bobfracsin * 0.012f) * xyspeed;

				if (bob > 6)
					bob = 6;

				ent->server.client->server.ps.viewoffset[2] -= bob;
			}
		}

		if (ent->deadflag)
			ent->server.client->server.ps.viewangles[YAW] = ent->server.client->killer_yaw;
		else
			ent->server.client->server.ps.kick_angles[0] = ent->server.client->gunstates[GUN_MAIN].kick_angles[0];

		return;
	}

	//===================================
	// base angles
	angles = ent->server.client->server.ps.kick_angles;

	// if dead, fix the angle and don't add any kick
	if (ent->deadflag)
	{
		VectorClear(angles);
		ent->server.client->server.ps.viewangles[ROLL] = 40;
		ent->server.client->server.ps.viewangles[PITCH] = -15;
		ent->server.client->server.ps.viewangles[YAW] = ent->server.client->killer_yaw;
	}
	else
	{
		// add angles based on weapon kick
		VectorClear(angles);

		for (int i = GUN_MAIN; i < MAX_PLAYER_GUNS; ++i)
			VectorAdd(ent->server.client->gunstates[i].kick_angles, angles, angles);

		// add angles based on damage kick

		if (ent->server.client->v_dmg_time > level.time)
		{
			ratio = (ent->server.client->v_dmg_time - level.time) / DAMAGE_TIME;

			if (ratio < 0)
			{
				ratio = 0;
				ent->server.client->v_dmg_pitch = 0;
				ent->server.client->v_dmg_roll = 0;
			}

			angles[PITCH] += ratio * ent->server.client->v_dmg_pitch;
			angles[ROLL] += ratio * ent->server.client->v_dmg_roll;
		}

		// add pitch based on fall kick

		if (ent->server.client->fall_time > level.time)
		{
			ratio = (ent->server.client->fall_time - level.time) / FALL_TIME;

			if (ratio < 0)
				ratio = 0;

			angles[PITCH] += ratio * ent->server.client->fall_value;
		}

		// add angles based on velocity
		delta = DotProduct(ent->velocity, forward);
		angles[PITCH] += delta * run_pitch->value;
		delta = DotProduct(ent->velocity, right);
		angles[ROLL] += delta * run_roll->value;
		// add angles based on bob
		delta = bobfracsin * bob_pitch->value * xyspeed;

		if (ent->server.client->server.ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6;     // crouching

		angles[PITCH] += delta;
		delta = bobfracsin * bob_roll->value * xyspeed;

		if (ent->server.client->server.ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6;     // crouching

		if (bobcycle & 1)
			delta = -delta;

		angles[ROLL] += delta;
	}

	//===================================
	// base origin
	VectorClear(v);
	// add view height
	v[2] += ent->viewheight;

	// add fall height

	if (ent->server.client->fall_time > level.time)
	{
		ratio = (ent->server.client->fall_time - level.time) / FALL_TIME;

		if (ratio < 0)
			ratio = 0;

		v[2] -= ratio * ent->server.client->fall_value * 0.4f;
	}

	// add bob height
	bob = bobfracsin * xyspeed * bob_up->value;

	if (bob > 6)
		bob = 6;

	//gi.DebugGraph (bob *2, 255);
	v[2] += bob;

	// add kick offset

	for (int i = GUN_MAIN; i < MAX_PLAYER_GUNS; ++i)
		VectorAdd(v, ent->server.client->gunstates[i].kick_origin, v);

	// absolutely bound offsets
	// so the view can never be outside the player box

	if (v[0] < -14)
		v[0] = -14;
	else if (v[0] > 14)
		v[0] = 14;

	if (v[1] < -14)
		v[1] = -14;
	else if (v[1] > 14)
		v[1] = 14;

	if (v[2] < -22)
		v[2] = -22;
	else if (v[2] > 30)
		v[2] = 30;

	VectorCopy(v, ent->server.client->server.ps.viewoffset);
}

/*
==============
SV_CalcGunOffset
==============
*/
void SV_CalcGunOffset(edict_t *ent)
{
	if (ent->server.state.game != GAME_Q2)
		return;

	int     i;
	float   delta;
	// gun angles from bobbing
	ent->server.client->server.ps.guns[GUN_MAIN].angles[ROLL] = xyspeed * bobfracsin * 0.005f;
	ent->server.client->server.ps.guns[GUN_MAIN].angles[YAW] = xyspeed * bobfracsin * 0.01f;

	if (bobcycle & 1)
	{
		ent->server.client->server.ps.guns[GUN_MAIN].angles[ROLL] = -ent->server.client->server.ps.guns[GUN_MAIN].angles[ROLL];
		ent->server.client->server.ps.guns[GUN_MAIN].angles[YAW] = -ent->server.client->server.ps.guns[GUN_MAIN].angles[YAW];
	}

	ent->server.client->server.ps.guns[GUN_MAIN].angles[PITCH] = xyspeed * bobfracsin * 0.005f;

	// gun angles from delta movement
	for (i = 0 ; i < 3 ; i++)
	{
		delta = ent->server.client->oldviewangles[i] - ent->server.client->server.ps.viewangles[i];

		if (delta > 180)
			delta -= 360;

		if (delta < -180)
			delta += 360;

		if (delta > 45)
			delta = 45;

		if (delta < -45)
			delta = -45;

		if (i == YAW)
			ent->server.client->server.ps.guns[GUN_MAIN].angles[ROLL] += 0.1f * delta;

		ent->server.client->server.ps.guns[GUN_MAIN].angles[i] += 0.2f * delta;
	}

	// gun height
	VectorClear(ent->server.client->server.ps.guns[GUN_MAIN].offset);
	//  ent->ps->gunorigin[2] += bob;

	// gun_x / gun_y / gun_z are development tools
	for (i = 0 ; i < 3 ; i++)
	{
		ent->server.client->server.ps.guns[GUN_MAIN].offset[i] += forward[i] * (gun_y->value);
		ent->server.client->server.ps.guns[GUN_MAIN].offset[i] += right[i] * gun_x->value;
		ent->server.client->server.ps.guns[GUN_MAIN].offset[i] += up[i] * (-gun_z->value);
	}
}


/*
=============
SV_AddBlend
=============
*/
void SV_AddBlend(float r, float g, float b, float a, float *v_blend)
{
	float   a2, a3;

	if (a <= 0)
		return;

	a2 = v_blend[3] + (1 - v_blend[3]) * a; // new total alpha
	a3 = v_blend[3] / a2;   // fraction of color from old
	v_blend[0] = v_blend[0] * a3 + r * (1 - a3);
	v_blend[1] = v_blend[1] * a3 + g * (1 - a3);
	v_blend[2] = v_blend[2] * a3 + b * (1 - a3);
	v_blend[3] = a2;
}

inline bool between(int v, int a, int b)
{
	return v >= a && v <= b;
}

/*
=============
SV_CalcBlend
=============
*/
void SV_CalcBlend(edict_t *ent)
{
	int     contents;
	vec3_t  vieworg;
	int     remaining;
	ent->server.client->server.ps.blend[0] = ent->server.client->server.ps.blend[1] =
			ent->server.client->server.ps.blend[2] = ent->server.client->server.ps.blend[3] = 0;
	// add for contents
	VectorAdd(ent->server.state.origin, ent->server.client->server.ps.viewoffset, vieworg);
	contents = gi.pointcontents(vieworg);

	if (contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER))
		ent->server.client->server.ps.rdflags |= RDF_UNDERWATER;
	else
		ent->server.client->server.ps.rdflags &= ~RDF_UNDERWATER;

	if (ent->server.state.game == GAME_DOOM)
	{
		if (ent->server.client->invincible_time > level.time && G_FlashyFrame(ent->server.client->invincible_time - level.time))
			ent->server.client->server.ps.rdflags |= RDF_INVUL;
		else
			ent->server.client->server.ps.rdflags &= ~RDF_INVUL;
	}

	if (contents & (CONTENTS_SOLID | CONTENTS_LAVA))
		SV_AddBlend(1.0f, 0.3f, 0.0f, 0.6f, ent->server.client->server.ps.blend);
	else if (contents & CONTENTS_SLIME)
		SV_AddBlend(0.0f, 0.1f, 0.05f, 0.6f, ent->server.client->server.ps.blend);
	else if (contents & CONTENTS_WATER)
		SV_AddBlend(0.5f, 0.3f, 0.2f, 0.4f, ent->server.client->server.ps.blend);

	// add for powerups
	if (ent->server.client->quad_time > level.time)
	{
		remaining = ent->server.client->quad_time - level.time;

		if (remaining == 3000)    // beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage2.wav"), 1, ATTN_NORM, 0);

		if (G_FlashyFrame(remaining))
			SV_AddBlend(0, 0, 1, 0.08f, ent->server.client->server.ps.blend);
	}
	else if (ent->server.client->invincible_time > level.time)
	{
		remaining = ent->server.client->invincible_time - level.time;

		if (remaining == 3000)    // beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect2.wav"), 1, ATTN_NORM, 0);

		if (G_FlashyFrame(remaining))
			SV_AddBlend(1, 1, 0, 0.08f, ent->server.client->server.ps.blend);
	}
	else if (ent->server.client->enviro_time > level.time)
	{
		remaining = ent->server.client->enviro_time - level.time;

		if (remaining == 3000)    // beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("%i_enviro_expire"), 1, ATTN_NORM, 0);

		if (G_FlashyFrame(remaining))
			SV_AddBlend(0, 1, 0, 0.08f, ent->server.client->server.ps.blend);
	}
	else if (ent->server.client->breather_time > level.time)
	{
		remaining = ent->server.client->breather_time - level.time;

		if (remaining == 3000)    // beginning to fade
		{
			switch (ent->server.state.game)
			{
				case GAME_Q2:
					gi.sound(ent, CHAN_ITEM, gi.soundindex("items/airout.wav"), 1, ATTN_NORM, 0);
					break;

				case GAME_Q1:
					gi.sound(ent, CHAN_ITEM, gi.soundindex("q1/items/inv2.wav"), 1, ATTN_NORM, 0);
					break;
			}
		}

		if (G_FlashyFrame(remaining))
			SV_AddBlend(0.4f, 1, 0.4f, 0.04f, ent->server.client->server.ps.blend);
	}

	if (ent->server.state.game == GAME_DOOM && ent->health <= 0)
	{
		VectorSet(ent->server.client->damage_blend, 1, 0, 0);
		ent->server.client->damage_alpha = 0.6f;
	}

	// add for damage
	if (ent->server.client->damage_alpha > 0)
		SV_AddBlend(ent->server.client->damage_blend[0], ent->server.client->damage_blend[1]
			, ent->server.client->damage_blend[2], ent->server.client->damage_alpha, ent->server.client->server.ps.blend);

	if (ent->server.client->bonus_alpha > 0)
		SV_AddBlend(0.85f, 0.7f, 0.3f, ent->server.client->bonus_alpha, ent->server.client->server.ps.blend);

	// drop the damage value
	ent->server.client->damage_alpha -= (ent->server.state.game == GAME_DOOM ? 0.02f : 0.06f) / game.framediv;

	if (ent->server.client->damage_alpha < 0)
		ent->server.client->damage_alpha = 0;

	// drop the bonus value
	ent->server.client->bonus_alpha -= 0.1f / game.framediv;

	if (ent->server.client->bonus_alpha < 0)
		ent->server.client->bonus_alpha = 0;

	if (ent->freeze_time > level.time)
	{
		ent->server.client->server.ps.blend[0] = ent->server.client->server.ps.blend[1] = 0;
		ent->server.client->server.ps.blend[2] = 1.0f;
		ent->server.client->server.ps.blend[3] = 0.25f;
	}
}


/*
=================
P_FallingDamage
=================
*/
void P_FallingDamage(edict_t *ent, byte msec)
{
	float   delta;
	int     damage;
	vec3_t  dir;

	if (ent->health <= 0)
		return;

	if (ent->server.state.modelindex != MODEL_HANDLE_PLAYER)
		return;     // not in the player model

	if (ent->movetype == MOVETYPE_NOCLIP)
		return;

	if ((ent->server.client->oldvelocity[2] < 0) && (ent->velocity[2] > ent->server.client->oldvelocity[2]) && (!ent->groundentity))
		delta = ent->server.client->oldvelocity[2];
	else
	{
		if (ent->server.state.game != GAME_DUKE && !ent->groundentity)
			return;

		delta = ent->velocity[2] - ent->server.client->oldvelocity[2];
	}

	delta = delta * delta * 0.0001f;

	switch (ent->server.state.game)
	{
		case GAME_Q1:

			// check to see if player landed and play landing sound
			if (ent->health > 0 && (ent->server.client->oldvelocity[2] < -300) && (ent->health > 0) && ent->waterlevel != 3)
			{
				if (ent->watertype & CONTENTS_WATER)
					gi.sound(ent, CHAN_BODY, gi.soundindex("q1/player/h2ojump.wav"), 1, ATTN_NORM, 0);
				else if (ent->server.client->oldvelocity[2] < -650)
				{
					T_Damage(ent, world, world, vec3_origin, vec3_origin, vec3_origin, 5, 0, DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(world, MD_FALLING, DT_DIRECT));
					gi.sound(ent, CHAN_VOICE, gi.soundindex("q1/player/land2.wav"), 1, ATTN_NORM, 0);
				}
				else
					gi.sound(ent, CHAN_VOICE, gi.soundindex("q1/player/land.wav"), 1, ATTN_NORM, 0);
			}

			return;

		case GAME_DOOM:
			if (delta > 10)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("doom/OOF.wav"), 1, ATTN_NORM, 0);
				ent->server.client->fall_value = min(40.0f, delta * 0.5f);
			}

			return;

		case GAME_DUKE:
			if (ent->waterlevel == 3)
				goto nofall;

#define DUKE_TIME_OFFSET(x) (x * 5.2f)
#define DUKE_TIME_OFFSET_INV(x) (x / 5.2f)

			if (!ent->groundentity && ent->velocity[2] < 0)
			{
				if (ent->server.client->fall_time < DUKE_TIME_OFFSET(255))
				{
					ent->server.client->fall_time += (msec >> 3);

					if (ent->server.client->fall_time >= DUKE_TIME_OFFSET(38) && !ent->server.client->duke_scream)
					{
						gi.sound(ent, CHAN_VOICE, gi.soundindex("duke/DSCREM04.wav"), 1, ATTN_NORM, 0);
						ent->server.client->duke_scream = true;
					}
				}
			}
			else if (ent->groundentity && delta > 4)
			{
				if (ent->server.client->fall_time < DUKE_TIME_OFFSET(255))
					ent->server.client->fall_time += (msec >> 3);

				if (ent->server.client->fall_time > DUKE_TIME_OFFSET(62))
					T_Damage(ent, world, world, dir, ent->server.state.origin, vec3_origin, ent->health - ent->gib_health, 0, DAMAGE_NONE, MakeGenericMeansOfDeath(world, MD_FALLING, DT_DIRECT));
				else if (ent->server.client->fall_time > DUKE_TIME_OFFSET(9) && delta > 30)
				{
					damage = DUKE_TIME_OFFSET_INV(ent->server.client->fall_time) - (Q_rand() & 3);
					T_Damage(ent, world, world, dir, ent->server.state.origin, vec3_origin, damage, 0, DAMAGE_NONE, MakeGenericMeansOfDeath(world, MD_FALLING, DT_DIRECT));
				}
				else
					gi.sound(ent, CHAN_VOICE, gi.soundindex("duke/LAND02.wav"), 1, ATTN_NORM, 0);

				ent->server.client->fall_value = min(24.0f, delta * 2.0f);
				goto nofall;
			}
			else
			{
nofall:
				ent->server.client->fall_time = 0;
				ent->server.client->duke_scream = false;
			}

			return;
	}

	// never take falling damage if completely underwater
	if (ent->waterlevel == 3)
		return;

	if (ent->waterlevel == 2)
		delta *= 0.25f;

	if (ent->waterlevel == 1)
		delta *= 0.5f;

	if (delta < 1)
		return;

	if (delta < 15)
	{
		ent->server.state.event = EV_FOOTSTEP;
		return;
	}

	ent->server.client->fall_value = delta * 0.5f;

	if (ent->server.client->fall_value > 40)
		ent->server.client->fall_value = 40;

	ent->server.client->fall_time = level.time + FALL_TIME;

	if (delta > 30)
	{
		if (ent->health > 0)
		{
			if (delta >= 55)
				ent->server.state.event = EV_FALLFAR;
			else
				ent->server.state.event = EV_FALL;
		}

		ent->pain_debounce_time = level.time + 300;   // no normal pain sound
		damage = (delta - 30) / 2;

		if (damage < 1)
			damage = 1;

		VectorSet(dir, 0, 0, 1);

		if (!dmflags.no_falling_damage)
			T_Damage(ent, world, world, dir, ent->server.state.origin, vec3_origin, damage, 0, DAMAGE_NONE, MakeGenericMeansOfDeath(world, MD_FALLING, DT_DIRECT));
	}
	else
	{
		ent->server.state.event = EV_FALLSHORT;
		return;
	}
}



/*
=============
P_WorldEffects
=============
*/
void P_WorldEffects(void)
{
	bool        breather;
	bool        envirosuit;
	int         waterlevel, old_waterlevel;

	if (current_player->movetype == MOVETYPE_NOCLIP)
	{
		current_player->air_finished = level.time + 1200; // don't need air
		return;
	}

	waterlevel = current_player->waterlevel;
	old_waterlevel = current_client->old_waterlevel;
	current_client->old_waterlevel = waterlevel;
	breather = current_player->server.state.game == GAME_Q2 && current_client->breather_time > level.time;
	envirosuit = current_client->enviro_time > level.time;

	//
	// if just entered a water volume, play a sound
	//
	if (!old_waterlevel && waterlevel)
	{
#ifdef ENABLE_COOP
		PlayerNoise(current_player, current_player->server.state.origin, PNOISE_SELF);
#endif

		switch (current_player->server.state.game)
		{
			case GAME_Q2:
				if (current_player->watertype & CONTENTS_LAVA)
					gi.sound(current_player, CHAN_BODY, gi.soundindex("player/lava_in.wav"), 1, ATTN_NORM, 0);
				else if (current_player->watertype & CONTENTS_SLIME)
					gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
				else if (current_player->watertype & CONTENTS_WATER)
					gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);

				break;

			case GAME_Q1:
				if (current_player->watertype & CONTENTS_LAVA)
					gi.sound(current_player, CHAN_BODY, gi.soundindex("q1/player/inlava.wav"), 1, ATTN_NORM, 0);
				else if (current_player->watertype & CONTENTS_WATER)
					gi.sound(current_player, CHAN_BODY, gi.soundindex("q1/player/inh2o.wav"), 1, ATTN_NORM, 0);
				else if (current_player->watertype & CONTENTS_SLIME)
					gi.sound(current_player, CHAN_BODY, gi.soundindex("q1/player/slimbrn2.wav"), 1, ATTN_NORM, 0);

				break;
		}

		current_player->flags |= FL_INWATER;
		// clear damage_debounce, so the pain sound will play immediately
		current_player->damage_debounce_time = 0;
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (old_waterlevel && ! waterlevel)
	{
#ifdef ENABLE_COOP
		PlayerNoise(current_player, current_player->server.state.origin, PNOISE_SELF);
#endif

		switch (current_player->server.state.game)
		{
			case GAME_Q2:
				gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
				break;

			case GAME_Q1:
				gi.sound(current_player, CHAN_BODY, gi.soundindex("q1/misc/outwater.wav"), 1, ATTN_NORM, 0);
				break;
		}

		current_player->flags &= ~FL_INWATER;
	}

	//
	// check for head just going under water
	//
	if (old_waterlevel != 3 && waterlevel == 3)
	{
		if (current_player->server.state.game == GAME_Q2)
			gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_un.wav"), 1, ATTN_NORM, 0);
	}

	//
	// check for head just coming out of water
	//
	if (old_waterlevel == 3 && waterlevel != 3)
	{
		switch (current_player->server.state.game)
		{
			case GAME_Q2:
				if (current_player->air_finished < level.time)
				{
					// gasp for air
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/gasp1.wav"), 1, ATTN_NORM, 0);
#ifdef ENABLE_COOP
					PlayerNoise(current_player, current_player->server.state.origin, PNOISE_SELF);
#endif
				}
				else  if (current_player->air_finished < level.time + 11000)
				{
					// just break surface
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/gasp2.wav"), 1, ATTN_NORM, 0);
				}

				break;

			case GAME_Q1:
				if (current_player->air_finished < level.time)
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("q1/player/gasp2.wav"), 1, ATTN_NORM, 0);
				else if (current_player->air_finished < level.time + 9000)
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("q1/player/gasp1.wav"), 1, ATTN_NORM, 0);

				break;
		}
	}

	//
	// check for drowning
	//
	if (waterlevel == 3)
	{
		// breather or envirosuit give air
		if (breather || envirosuit)
		{
			current_player->air_finished = level.time + 1000;

			if (((int)(current_client->breather_time - level.time) % 2500) == 0 && current_player->server.state.game == GAME_Q2)
			{
				if (!current_client->breather_sound)
					gi.sound(current_player, CHAN_AUTO, gi.soundindex("player/u_breath1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound(current_player, CHAN_AUTO, gi.soundindex("player/u_breath2.wav"), 1, ATTN_NORM, 0);

				current_client->breather_sound ^= 1;
#ifdef ENABLE_COOP
				PlayerNoise(current_player, current_player->server.state.origin, PNOISE_SELF);
#endif
				//FIXME: release a bubble?
			}
		}

		// if out of air, start drowning
		if (current_player->air_finished < level.time)
		{
			// drown!
			if (current_player->server.client->next_drown_time < level.time
				&& current_player->health > 0)
			{
				current_player->server.client->next_drown_time = level.time + 1000;
				// take more damage the longer underwater
				current_player->dmg += 2;

				if (current_player->dmg > 15)
					current_player->dmg = (current_player->server.state.game == GAME_Q1) ? 10 : 15;

				// play a gurp sound instead of a normal pain sound
				switch (current_player->server.state.game)
				{
					case GAME_Q2:
						if (current_player->health <= current_player->dmg)
							gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/drown1.wav"), 1, ATTN_NORM, 0);
						else if (Q_rand() & 1)
							gi.sound(current_player, CHAN_VOICE, gi.soundindex("*gurp1.wav"), 1, ATTN_NORM, 0);
						else
							gi.sound(current_player, CHAN_VOICE, gi.soundindex("*gurp2.wav"), 1, ATTN_NORM, 0);

						break;

					case GAME_Q1:
						if (current_player->health <= current_player->dmg)
						{
							//DeathBubbles(20);
							gi.sound(current_player, CHAN_VOICE, gi.soundindex("q1/player/h2odeath.wav"), 1, ATTN_NORM, 0);
						}
						else if (current_player->air_finished < level.time)
							gi.sound(current_player, CHAN_VOICE, gi.soundindex("q1/player/drown1.wav"), 1, ATTN_NORM, 0);
						else if (current_player->air_finished < level.time + 9)
							gi.sound(current_player, CHAN_VOICE, gi.soundindex("q1/player/drown2.wav"), 1, ATTN_NORM, 0);

						break;
				}

				current_player->pain_debounce_time = level.time;
				T_Damage(current_player, world, world, vec3_origin, current_player->server.state.origin, vec3_origin, current_player->dmg, 0, DAMAGE_NO_ARMOR, MakeGenericMeansOfDeath(world, MD_WATER, DT_DIRECT));
			}
		}
	}
	else
	{
		current_player->air_finished = level.time + 12000;
		current_player->dmg = 2;
	}

	//
	// check for sizzle damage
	//
	if (waterlevel && (current_player->watertype & (CONTENTS_LAVA | CONTENTS_SLIME)))
	{
		if (current_player->watertype & CONTENTS_LAVA)
		{
			switch (current_player->server.state.game)
			{
				case GAME_Q2:
					if (current_player->health > 0
						&& current_player->pain_debounce_time <= level.time
						&& current_client->invincible_time < level.time)
					{
						if (Q_rand() & 1)
							gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/burn1.wav"), 1, ATTN_NORM, 0);
						else
							gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/burn2.wav"), 1, ATTN_NORM, 0);

						current_player->pain_debounce_time = level.time + 1000;
					}

					if (envirosuit) // take 1/3 damage with envirosuit
						T_Damage(current_player, world, world, vec3_origin, current_player->server.state.origin, vec3_origin, 1 * waterlevel, 0, DAMAGE_NONE, MakeGenericMeansOfDeath(world, MD_LAVA, DT_DIRECT));
					else
						T_Damage(current_player, world, world, vec3_origin, current_player->server.state.origin, vec3_origin, 3 * waterlevel, 0, DAMAGE_NONE, MakeGenericMeansOfDeath(world, MD_LAVA, DT_DIRECT));

					break;

				case GAME_Q1:
					if (current_player->dmgtime < level.time)
					{
						if (envirosuit)
							current_player->dmgtime = level.time + 1000;
						else
							current_player->dmgtime = level.time + 200;

						T_Damage(current_player, world, world, vec3_origin, current_player->server.state.origin, vec3_origin, 10 * waterlevel, 0, DAMAGE_NONE, MakeGenericMeansOfDeath(world, MD_LAVA, DT_DIRECT));
					}

					break;

				case GAME_DOOM:
					if (current_player->dmgtime < level.time)
					{
						current_player->dmgtime = level.time + 1000;

						if (old_waterlevel && (!envirosuit || (Q_rand() % 256) > 6))
							T_Damage(current_player, world, world, vec3_origin, current_player->server.state.origin, vec3_origin, 10 * waterlevel, 0, DAMAGE_NONE, MakeGenericMeansOfDeath(world, MD_LAVA, DT_DIRECT));
					}

					break;
			}
		}

		if (current_player->watertype & CONTENTS_SLIME)
		{
			switch (current_player->server.state.game)
			{
				case GAME_Q2:
					if (!envirosuit)
					{
						// no damage from slime with envirosuit
						T_Damage(current_player, world, world, vec3_origin, current_player->server.state.origin, vec3_origin, 1 * waterlevel, 0, DAMAGE_NONE, MakeGenericMeansOfDeath(world, MD_SLIME, DT_DIRECT));
					}

					break;

				case GAME_Q1:
					if (current_player->dmgtime < level.time && !envirosuit)
					{
						current_player->dmgtime = level.time + 1000;
						T_Damage(current_player, world, world, vec3_origin, current_player->server.state.origin, vec3_origin, 4 * waterlevel, 0, DAMAGE_NONE, MakeGenericMeansOfDeath(world, MD_SLIME, DT_DIRECT));
					}

					break;

				case GAME_DOOM:
					if (current_player->dmgtime < level.time && !envirosuit)
					{
						current_player->dmgtime = level.time + 1000;

						if (old_waterlevel)
							T_Damage(current_player, world, world, vec3_origin, current_player->server.state.origin, vec3_origin, 2 * waterlevel, 0, DAMAGE_NONE, MakeGenericMeansOfDeath(world, MD_SLIME, DT_DIRECT));
					}

					break;
			}
		}
	}
}


/*
===============
G_SetClientEffects
===============
*/
void G_SetClientEffects(edict_t *ent)
{
	int     pa_type;
	int     remaining;
	ent->server.state.effects = 0;
	ent->server.state.renderfx = 0;

	if (ent->health <= 0 || level.intermissiontime)
		return;

	if (ent->powerarmor_time > level.time)
	{
		pa_type = PowerArmorType(ent);

		if (pa_type == POWER_ARMOR_SCREEN)
			ent->server.state.effects |= EF_POWERSCREEN;
		else if (pa_type == POWER_ARMOR_SHIELD)
		{
			ent->server.state.effects |= EF_COLOR_SHELL;
			ent->server.state.renderfx |= RF_SHELL_GREEN;
		}
	}

	if (ent->server.client->quad_time > level.time)
	{
		remaining = ent->server.client->quad_time - level.time;

		if (G_FlashyFrame(remaining))
			ent->server.state.effects |= EF_QUAD;
	}

	if (ent->server.client->invincible_time > level.time)
	{
		remaining = ent->server.client->invincible_time - level.time;

		if (G_FlashyFrame(remaining))
			ent->server.state.effects |= EF_PENT;
	}

	// show cheaters!!!
	if (ent->flags & FL_GODMODE)
	{
		ent->server.state.effects |= EF_COLOR_SHELL;
		ent->server.state.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
	}

	if (ent->server.state.game == GAME_DOOM && ent->server.client->breather_time > level.time)
		ent->server.state.renderfx |= RF_SPECTRE;

	if (ent->freeze_time > level.time)
		ent->server.state.renderfx |= RF_FROZEN;
}


/*
===============
G_SetClientEvent
===============
*/
void G_SetClientEvent(edict_t *ent)
{
	if (ent->server.state.event)
		return;

	if (ent->groundentity && xyspeed > 225 && ent->server.state.game == GAME_Q2)
	{
		if ((int)(current_client->bobtime + bobmove) != bobcycle)
			ent->server.state.event = EV_FOOTSTEP;
	}
}

/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound(edict_t *ent)
{
#ifdef ENABLE_COOP
	if (ent->server.client->pers.game_helpchanged != game.helpchanged)
	{
		ent->server.client->pers.game_helpchanged = game.helpchanged;
		ent->server.client->pers.helpchanged = 1;
	}

	// help beep (no more than three times)
	if (ent->server.client->pers.helpchanged && ent->server.client->pers.helpchanged <= 3 && !(level.framenum & (63 * game.framediv)))
	{
		ent->server.client->pers.helpchanged++;
		gi.sound(ent, CHAN_VOICE, gi.soundindex("misc/pc_up.wav"), 1, ATTN_STATIC, 0);
	}
#endif

	if (ent->waterlevel && (ent->watertype & (CONTENTS_LAVA | CONTENTS_SLIME)))
		ent->server.state.sound = snd_fry;
	else if (ent->server.state.game == GAME_Q2 && ITEM_INDEX(ent->server.client->pers.weapon) == ITI_Q2_RAILGUN)
		ent->server.state.sound = gi.soundindex("weapons/rg_hum.wav");
	else if (ent->server.state.game == GAME_Q2 && ITEM_INDEX(ent->server.client->pers.weapon) == ITI_Q2_BFG10K)
		ent->server.state.sound = gi.soundindex("weapons/bfg_hum.wav");
	else if (ent->server.state.game == GAME_Q1 && ent->server.client->breather_time > level.time)
		ent->server.state.sound = gi.soundindex("q1/items/inv3.wav");
	else if (ent->server.client->weapon_sound)
		ent->server.state.sound = ent->server.client->weapon_sound;
	else
		ent->server.state.sound = 0;
}

/*
===============
G_SetClientFrame
===============
*/
void G_SetClientFrame(edict_t *ent)
{
	gclient_t   *client;
	bool        duck, run;

	if (ent->server.state.modelindex != MODEL_HANDLE_PLAYER)
		return;     // not in the player model

	client = ent->server.client;
	duck = !!(client->server.ps.pmove.pm_flags & PMF_DUCKED);
	run = !!(xyspeed);
	int anim_type = client->anim_priority & 7;

	// check for stand/duck and stop/go transitions
	if (duck != client->anim_duck && anim_type < ANIM_DEATH)
		goto newanim;
	else if (run != client->anim_run && anim_type == ANIM_BASIC)
		goto newanim;
	else  if (!ent->groundentity && anim_type <= ANIM_WAVE && (ent->server.state.game == GAME_Q2 || (ent->server.state.game == GAME_DUKE && ent->waterlevel < 3)))
		goto newanim;
	else if (client->force_anim)
	{
		client->force_anim = false;
		client->hold_frame = 0;
		goto newanim;
	}

	// for Doom mostly
	if (client->hold_frame > level.time)
		return;

	if (client->anim_priority & ANIM_REVERSE)
	{
		if (ent->server.state.frame > client->anim_end)
		{
			ent->server.state.frame--;
			return;
		}
	}
	else if (ent->server.state.frame < client->anim_end)
	{
		// continue an animation
		ent->server.state.frame++;
		return;
	}

	if (anim_type == ANIM_DEATH)
		return;     // stay there

	if (anim_type == ANIM_JUMP && (ent->server.state.game == GAME_Q2 || ent->server.state.game == GAME_DUKE))
	{
		if (ent->server.state.game == GAME_DUKE)
		{
dukejump:

			if (ent->groundentity)
				goto newanim;

			if (ent->waterlevel >= 3)
				ent->server.state.frame = client->anim_end = 1;
			else if (ent->velocity[2] > 0)
				ent->server.state.frame = client->anim_end = 8;
			else if (ent->velocity[2] < -200)
				ent->server.state.frame = client->anim_end = 10;
			else
				ent->server.state.frame = client->anim_end = 9;

			ent->server.client->anim_priority = ANIM_WAVE;
			return;
		}

		if (!ent->groundentity)
			return;     // stay there

		ent->server.client->anim_priority = ANIM_WAVE;
		ent->server.state.frame = FRAME_jump3;
		ent->server.client->anim_end = FRAME_jump6;
		return;
	}

newanim:
	// return to either a running or standing frame
	client->anim_priority = ANIM_BASIC;
	client->anim_duck = duck;
	client->anim_run = run;

	if (!ent->groundentity && (ent->server.state.game == GAME_Q2 || (ent->server.state.game == GAME_DUKE && ent->waterlevel < 3)))
	{
		if (ent->server.state.game == GAME_DUKE)
			goto dukejump;

		client->anim_priority = ANIM_JUMP;

		if (ent->server.state.frame != FRAME_jump2)
			ent->server.state.frame = FRAME_jump1;

		client->anim_end = FRAME_jump2;
	}
	else if (run)
	{
		switch (ent->server.state.game)
		{
			case GAME_Q2:

				// running
				if (duck)
				{
					ent->server.state.frame = FRAME_crwalk1;
					client->anim_end = FRAME_crwalk6;
				}
				else
				{
					ent->server.state.frame = FRAME_run1;
					client->anim_end = FRAME_run6;
				}

				break;

			case GAME_Q1:
				if (ent->server.client->pers.weapon && ITEM_INDEX(ent->server.client->pers.weapon) == ITI_Q1_AXE)
				{
					ent->server.state.frame = 0;
					client->anim_end = 5;
				}
				else
				{
					ent->server.state.frame = 6;
					client->anim_end = 11;
				}

				break;

			case GAME_DOOM:
				ent->server.state.frame = 0;
				client->anim_end = 3;
				break;

			case GAME_DUKE:
				if (ent->waterlevel >= 3 && xyspeed > 10)
				{
					ent->server.state.frame = 20;
					client->anim_end = 23;
				}
				else if (duck)
				{
					ent->server.state.frame = 11;
					client->anim_end = 13;
				}
				else
				{
					ent->server.state.frame = 2;
					client->anim_end = 6;
				}

				break;
		}
	}
	else
	{
		switch (ent->server.state.game)
		{
			case GAME_Q2:

				// standing
				if (duck)
				{
					ent->server.state.frame = FRAME_crstnd01;
					client->anim_end = FRAME_crstnd19;
				}
				else
				{
					ent->server.state.frame = FRAME_stand01;
					client->anim_end = FRAME_stand40;
				}

				break;

			case GAME_Q1:
				if (ent->server.client->pers.weapon && ITEM_INDEX(ent->server.client->pers.weapon) == ITI_Q1_AXE)
				{
					ent->server.state.frame = 17;
					client->anim_end = 28;
				}
				else
				{
					ent->server.state.frame = 12;
					client->anim_end = 16;
				}

				break;

			case GAME_DOOM:
				ent->server.state.frame = 0;
				client->anim_end = 0;
				break;

			case GAME_DUKE:
				if (ent->waterlevel >= 3)
					ent->server.state.frame = client->anim_end = 1;
				else if (duck)
					ent->server.state.frame = client->anim_end = 9;
				else
					ent->server.state.frame = client->anim_end = 0;

				break;
		}
	}
}


/*
=================
ClientEndServerFrame

Called for each player at the end of the server frame
and right after spawning
=================
*/
void ClientEndServerFrame(edict_t *ent)
{
	float   bobtime;
	int     i;
	current_player = ent;
	current_client = ent->server.client;

	//
	// If the origin or velocity have changed since ClientThink(),
	// update the pmove values.  This will happen when the client
	// is pushed by a bmodel or kicked by an explosion.
	//
	// If it wasn't updated here, the view position would lag a frame
	// behind the body position when pushed -- "sinking into plats"
	//
	for (i = 0 ; i < 3 ; i++)
	{
		current_client->server.ps.pmove.origin[i] = ent->server.state.origin[i] * 8.0f;
		current_client->server.ps.pmove.velocity[i] = ent->velocity[i] * 8.0f;
	}

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if (level.intermissiontime)
	{
		// FIXME: add view drifting here?
		current_client->server.ps.blend[3] = 0;
		current_client->server.ps.fov = 90;
		G_SetStats(ent);
		return;
	}

	AngleVectors(ent->server.client->v_angle, forward, right, up);
	// burn from lava, etc
	P_WorldEffects();

	//
	// set model angles from view angles so other things in
	// the world can tell which direction you are looking
	//
	if (!ent->deadflag && ent->server.state.game != GAME_DOOM && ent->server.state.game != GAME_DUKE)
	{
		if (ent->server.client->v_angle[PITCH] > 180)
			ent->server.state.angles[PITCH] = (-360 + ent->server.client->v_angle[PITCH]) / 3;
		else
			ent->server.state.angles[PITCH] = ent->server.client->v_angle[PITCH] / 3;

		ent->server.state.angles[ROLL] = 0;
		ent->server.state.angles[ROLL] = SV_CalcRoll(ent->server.state.angles, ent->velocity) * 4;
	}
	else
		ent->server.state.angles[0] = ent->server.state.angles[2] = 0;

	ent->server.state.angles[YAW] = ent->server.client->v_angle[YAW];
	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	xyspeed = sqrtf(ent->velocity[0] * ent->velocity[0] + ent->velocity[1] * ent->velocity[1]);

	if (xyspeed < 5)
	{
		bobmove = 0;
		current_client->bobtime = 0;    // start at beginning of cycle again
	}
	else if (ent->groundentity)
	{
		// so bobbing only cycles when on ground
		if (xyspeed > 210)
			bobmove = 0.25f;
		else if (xyspeed > 100)
			bobmove = 0.125f;
		else
			bobmove = 0.0625f;

		bobmove /= game.framediv;
	}

	bobtime = (current_client->bobtime += bobmove);

	if (current_client->server.ps.pmove.pm_flags & PMF_DUCKED)
		bobtime *= 4 / game.framediv;

	bobcycle = (int)bobtime;
	bobfracsin = fabsf(sinf((float)(bobtime * M_PI)));
	// apply all the damage taken this frame
	P_DamageFeedback(ent);
	// determine the view offsets
	SV_CalcViewOffset(ent);
	// determine the gun offsets
	SV_CalcGunOffset(ent);
	// determine the full screen color blend
	// must be after viewoffset, so eye contents can be
	// accurately determined
	// FIXME: with client prediction, the contents
	// should be determined by the client
	SV_CalcBlend(ent);

	// chase cam stuff
	if (ent->server.client->resp.spectator)
		G_SetSpectatorStats(ent);
	else
		G_SetStats(ent);

	G_CheckChaseStats(ent);
	G_SetClientEvent(ent);
	G_SetClientEffects(ent);
	G_SetClientSound(ent);
	G_SetClientFrame(ent);
	VectorCopy(ent->server.client->server.ps.viewangles, ent->server.client->oldviewangles);
	// clear weapon kicks
	float kick_div = 1.0f / game.framediv;
	float origin_div = 4.0f / game.framediv;

	for (int gun = GUN_MAIN; gun < MAX_PLAYER_GUNS; ++gun)
		for (i = 0; i < 3; ++i)
		{
			if (ent->server.client->gunstates[gun].kick_origin[i] > 0)
				ent->server.client->gunstates[gun].kick_origin[i] = max(0.0f, ent->server.client->gunstates[gun].kick_origin[i] - origin_div);
			else if (ent->server.client->gunstates[gun].kick_origin[i] < 0)
				ent->server.client->gunstates[gun].kick_origin[i] = min(0.0f, ent->server.client->gunstates[gun].kick_origin[i] + origin_div);

			if (ent->server.client->gunstates[gun].kick_angles[i] > 0)
				ent->server.client->gunstates[gun].kick_angles[i] = max(0.0f, ent->server.client->gunstates[gun].kick_angles[i] - kick_div);
			else if (ent->server.client->gunstates[gun].kick_angles[i] < 0)
				ent->server.client->gunstates[gun].kick_angles[i] = min(0.0f, ent->server.client->gunstates[gun].kick_angles[i] + kick_div);
		}

	// if the scoreboard is up, update it
	if (ent->server.client->showscores && !(level.framenum & (31 * game.framediv)))
	{
		DeathmatchScoreboardMessage(ent, ent->enemy);
		gi.unicast(ent, false);
	}

	// Q1 invis
	if (ent->server.state.game == GAME_Q1)
	{
		if (ent->server.client->breather_time > level.time)
		{
			if (ent->server.state.modelindex == MODEL_HANDLE_PLAYER)
			{
				ent->server.state.modelindex = gi.modelindex("models/q1/eyes.mdl");
				ent->server.state.skinnum = 0;
			}

			ent->server.state.frame = 0;
		}
		else if (ent->server.client->breather_time <= level.time)
		{
			if (ent->server.state.modelindex != MODEL_HANDLE_PLAYER && ent->server.solid)
			{
				ent->server.state.modelindex = MODEL_HANDLE_PLAYER;
				ent->server.state.skinnum = ent - g_edicts - 1;
				ent->server.client->force_anim = true;
				G_SetClientFrame(ent);
			}
		}
	}
}


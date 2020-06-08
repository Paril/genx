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
==============================================================================

flyer

==============================================================================
*/
#ifdef ENABLE_COOP
#include "g_local.h"
#include "m_flyer.h"
#include "m_local.h"

static q_soundhandle sound_sight;
static q_soundhandle sound_idle;
static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_slash;
static q_soundhandle sound_sproing;
static q_soundhandle sound_die;

static mscript_t script;

static void flyer_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void flyer_idle(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void flyer_pop_blades(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_sproing, 1, ATTN_NORM, 0);
}

static void flyer_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
}

static void flyer_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void flyer_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void flyer_fire(edict_t *self, int flash_number)
{
	vec3_t  start;
	vec3_t  forward, right;
	vec3_t  end;
	vec3_t  dir;
	int     effect;

	if ((self->server.state.frame == FRAME_attak204) || (self->server.state.frame == FRAME_attak207) || (self->server.state.frame == FRAME_attak210))
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	AngleVectors(self->server.state.angles, forward, right, NULL);
	G_ProjectSource(self->server.state.origin, monster_flash_offset[flash_number], forward, right, start);
	VectorCopy(self->enemy->server.state.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, dir);
	monster_fire_blaster(self, start, dir, 1, 1000, flash_number, effect);
}

static void flyer_fireleft(edict_t *self)
{
	flyer_fire(self, MZ2_FLYER_BLASTER_1);
}

static void flyer_fireright(edict_t *self)
{
	flyer_fire(self, MZ2_FLYER_BLASTER_2);
}

static void flyer_slash_left(edict_t *self)
{
	vec3_t  aim;
	VectorSet(aim, MELEE_DISTANCE, self->server.mins[0], 0);
	fire_hit(self, aim, 5, 0);
	gi.sound(self, CHAN_WEAPON, sound_slash, 1, ATTN_NORM, 0);
}

static void flyer_slash_right(edict_t *self)
{
	vec3_t  aim;
	VectorSet(aim, MELEE_DISTANCE, self->server.maxs[0], 0);
	fire_hit(self, aim, 5, 0);
	gi.sound(self, CHAN_WEAPON, sound_slash, 1, ATTN_NORM, 0);
}

static void flyer_loop_melee(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "loop_melee");
}

static void flyer_attack(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack2");
}

static void flyer_melee(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "start_melee");
}

static void flyer_check_melee(edict_t *self)
{
	if (range(self, self->enemy) == RANGE_MELEE && random() <= 0.8f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "loop_melee");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "end_melee");
}

static void flyer_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	int     n;

	if (self->health < (self->max_health / 2))
		self->server.state.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3000;

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	n = Q_rand() % 3;

	if (n == 0)
	{
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
	}
	else if (n == 1)
	{
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain2");
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain3");
	}
}

static void flyer_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	BecomeExplosion1(self);
}

static const mevent_t events[] =
{
	EVENT_FUNC(flyer_run),
	EVENT_FUNC(flyer_fireleft),
	EVENT_FUNC(flyer_fireright),
	EVENT_FUNC(flyer_loop_melee),
	EVENT_FUNC(flyer_pop_blades),
	EVENT_FUNC(flyer_check_melee),
	EVENT_FUNC(flyer_slash_left),
	EVENT_FUNC(flyer_slash_right),
	NULL
};

/*QUAKED monster_flyer (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_flyer(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	// fix a map bug in jail5.bsp
	if (!Q_stricmp(level.mapname, "jail5") && (self->server.state.origin[2] == -104))
	{
		self->targetname = self->target;
		self->target = NULL;
	}

	const char *model_name = "models/monsters/flyer/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/flyer.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_sight = gi.soundindex("flyer/flysght1.wav");
	sound_idle = gi.soundindex("flyer/flysrch1.wav");
	sound_pain1 = gi.soundindex("flyer/flypain1.wav");
	sound_pain2 = gi.soundindex("flyer/flypain2.wav");
	sound_slash = gi.soundindex("flyer/flyatck2.wav");
	sound_sproing = gi.soundindex("flyer/flyatck1.wav");
	sound_die = gi.soundindex("flyer/flydeth1.wav");
	gi.soundindex("flyer/flyatck3.wav");
	self->server.state.modelindex = gi.modelindex(model_name);
	VectorSet(self->server.mins, -16, -16, -24);
	VectorSet(self->server.maxs, 16, 16, 32);
	self->movetype = MOVETYPE_STEP;
	self->server.solid = SOLID_BBOX;
	self->server.state.sound = gi.soundindex("flyer/flyidle1.wav");
	self->health = 50;
	self->mass = 50;
	self->pain = flyer_pain;
	self->die = flyer_die;
	self->monsterinfo.stand = flyer_stand;
	self->monsterinfo.walk = flyer_walk;
	self->monsterinfo.run = flyer_run;
	self->monsterinfo.attack = flyer_attack;
	self->monsterinfo.melee = flyer_melee;
	self->monsterinfo.sight = flyer_sight;
	self->monsterinfo.idle = flyer_idle;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	flymonster_start(self);
}

#endif
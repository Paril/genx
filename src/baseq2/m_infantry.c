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

INFANTRY

==============================================================================
*/

#include "g_local.h"
#include "m_infantry.h"
#include "m_local.h"

static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_die1;
static q_soundhandle sound_die2;
static q_soundhandle sound_gunshot;
static q_soundhandle sound_weapon_cock;
static q_soundhandle sound_punch_swing;
static q_soundhandle sound_punch_hit;
static q_soundhandle sound_sight;
static q_soundhandle sound_search;
static q_soundhandle sound_idle;

static mscript_t script;

void infantry_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void infantry_fidget(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "fidget");
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void infantry_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void infantry_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
}

static void infantry_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	int     n;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3000;

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	n = Q_rand() % 2;

	if (n == 0)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	}
	else
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain2");
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	}
}

static vec3_t  aimangles[] =
{
	{ 0.0, 5.0, 0.0 },
	{ 10.0, 15.0, 0.0 },
	{ 20.0, 25.0, 0.0 },
	{ 25.0, 35.0, 0.0 },
	{ 30.0, 40.0, 0.0 },
	{ 30.0, 45.0, 0.0 },
	{ 25.0, 50.0, 0.0 },
	{ 20.0, 40.0, 0.0 },
	{ 15.0, 35.0, 0.0 },
	{ 40.0, 35.0, 0.0 },
	{ 70.0, 35.0, 0.0 },
	{ 90.0, 35.0, 0.0 }
};

static void InfantryMachineGun(edict_t *self)
{
	vec3_t  start, target;
	vec3_t  forward, right;
	vec3_t  vec;
	int     flash_number;

	if (self->s.frame == FRAME_attak111)
	{
		flash_number = MZ2_INFANTRY_MACHINEGUN_1;
		AngleVectors(self->s.angles, forward, right, NULL);
		G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);

		if (self->enemy)
		{
			VectorMA(self->enemy->s.origin, -0.2f, self->enemy->velocity, target);
			target[2] += self->enemy->viewheight;
			VectorSubtract(target, start, forward);
			VectorNormalize(forward);
		}
		else
			AngleVectors(self->s.angles, forward, right, NULL);
	}
	else
	{
		flash_number = MZ2_INFANTRY_MACHINEGUN_2 + (self->s.frame - FRAME_death211);
		AngleVectors(self->s.angles, forward, right, NULL);
		G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);
		VectorSubtract(self->s.angles, aimangles[flash_number - MZ2_INFANTRY_MACHINEGUN_2], vec);
		AngleVectors(vec, forward, NULL, NULL);
	}

	monster_fire_bullet(self, start, forward, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

static void infantry_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_BODY, sound_sight, 1, ATTN_NORM, 0);
}

static void infantry_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->s.clip_contents = CONTENTS_DEADMONSTER;
	gi.linkentity(self);
	M_FlyCheck(self);
}

void infantry_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int     n;

	// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		for (n = 0; n < 2; n++)
			ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);

		for (n = 0; n < 4; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

		ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	// regular death
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	n = Q_rand() % 3;

	if (n == 0)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death1");
		gi.sound(self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);
	}
	else if (n == 1)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death2");
		gi.sound(self, CHAN_VOICE, sound_die1, 1, ATTN_NORM, 0);
	}
	else
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death3");
		gi.sound(self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);
	}
}

static void infantry_duck_down(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.pausetime = level.time + 1000;
	gi.linkentity(self);
}

static void infantry_duck_hold(edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void infantry_duck_up(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity(self);
}

static void infantry_dodge(edict_t *self, edict_t *attacker, float eta)
{
	if (random() > 0.25f)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "duck");
}

static void infantry_cock_gun(edict_t *self)
{
	int     n;
	gi.sound(self, CHAN_WEAPON, sound_weapon_cock, 1, ATTN_NORM, 0);
	n = (Q_rand() & 15) + 3 + 7;
	self->monsterinfo.pausetime = level.time + n * 100;
}

static void infantry_fire(edict_t *self)
{
	InfantryMachineGun(self);

	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void infantry_swing(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_punch_swing, 1, ATTN_NORM, 0);
}

static void infantry_smack(edict_t *self)
{
	vec3_t  aim;
	VectorSet(aim, MELEE_DISTANCE, 0, 0);

	if (fire_hit(self, aim, (5 + (Q_rand() % 5)), 50))
		gi.sound(self, CHAN_WEAPON, sound_punch_hit, 1, ATTN_NORM, 0);
}

static void infantry_attack(edict_t *self)
{
	if (range(self, self->enemy) == RANGE_MELEE)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack2");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
}

static const mevent_t events[] =
{
	EVENT_FUNC(infantry_stand),
	EVENT_FUNC(infantry_run),
	EVENT_FUNC(infantry_dead),
	EVENT_FUNC(InfantryMachineGun),
	EVENT_FUNC(infantry_duck_down),
	EVENT_FUNC(infantry_duck_hold),
	EVENT_FUNC(infantry_duck_up),
	EVENT_FUNC(infantry_cock_gun),
	EVENT_FUNC(infantry_fire),
	EVENT_FUNC(infantry_swing),
	EVENT_FUNC(infantry_smack),
	NULL
};

/*QUAKED monster_infantry (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_infantry(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/infantry/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/infantry.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_pain1 = gi.soundindex("infantry/infpain1.wav");
	sound_pain2 = gi.soundindex("infantry/infpain2.wav");
	sound_die1 = gi.soundindex("infantry/infdeth1.wav");
	sound_die2 = gi.soundindex("infantry/infdeth2.wav");
	sound_gunshot = gi.soundindex("infantry/infatck1.wav");
	sound_weapon_cock = gi.soundindex("infantry/infatck3.wav");
	sound_punch_swing = gi.soundindex("infantry/infatck2.wav");
	sound_punch_hit = gi.soundindex("infantry/melee2.wav");
	sound_sight = gi.soundindex("infantry/infsght1.wav");
	sound_search = gi.soundindex("infantry/infsrch1.wav");
	sound_idle = gi.soundindex("infantry/infidle1.wav");
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 32);
	self->health = 100;
	self->gib_health = -40;
	self->mass = 200;
	self->pain = infantry_pain;
	self->die = infantry_die;
	self->monsterinfo.stand = infantry_stand;
	self->monsterinfo.walk = infantry_walk;
	self->monsterinfo.run = infantry_run;
	self->monsterinfo.dodge = infantry_dodge;
	self->monsterinfo.attack = infantry_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = infantry_sight;
	self->monsterinfo.idle = infantry_fidget;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
}

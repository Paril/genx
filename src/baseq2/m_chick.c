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

chick

==============================================================================
*/
#ifdef ENABLE_COOP

#include "g_local.h"
#include "m_chick.h"
#include "m_local.h"

static q_soundhandle sound_missile_prelaunch;
static q_soundhandle sound_missile_launch;
static q_soundhandle sound_melee_swing;
static q_soundhandle sound_melee_hit;
static q_soundhandle sound_missile_reload;
static q_soundhandle sound_death1;
static q_soundhandle sound_death2;
static q_soundhandle sound_fall_down;
static q_soundhandle sound_idle1;
static q_soundhandle sound_idle2;
static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_pain3;
static q_soundhandle sound_sight;
static q_soundhandle sound_search;

static mscript_t script;

static void ChickMoan(edict_t *self)
{
	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_idle1, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_idle2, 1, ATTN_IDLE, 0);
}

static void chick_fidget(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;

	if (random() <= 0.3f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "fidget");
}

static void chick_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void chick_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void chick_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
		return;
	}

	if (self->monsterinfo.currentmove == M_GetMonsterMove(&script, "walk") ||
		self->monsterinfo.currentmove == M_GetMonsterMove(&script, "start_run"))
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "start_run");
}

static void chick_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	float   r;

	if (self->health < (self->max_health / 2))
		self->server.state.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3000;
	r = random();

	if (r < 0.33f)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else if (r < 0.66f)
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain3, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	if (damage <= 10)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
	else if (damage <= 25)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain2");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain3");
}

static void chick_dead(edict_t *self)
{
	VectorSet(self->server.mins, -16, -16, 0);
	VectorSet(self->server.maxs, 16, 16, 16);
	self->movetype = MOVETYPE_TOSS;
	self->server.flags.deadmonster = true;
	self->server.state.clip_contents = CONTENTS_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

static void chick_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
	self->takedamage = true;
	n = Q_rand() % 2;

	if (n == 0)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death1");
		gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
	}
	else
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death2");
		gi.sound(self, CHAN_VOICE, sound_death2, 1, ATTN_NORM, 0);
	}
}

static void chick_duck_down(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	self->server.maxs[2] -= 32;
	self->takedamage = true;
	self->monsterinfo.pausetime = level.time + 1000;
	gi.linkentity(self);
}

static void chick_duck_hold(edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void chick_duck_up(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->server.maxs[2] += 32;
	self->takedamage = true;
	gi.linkentity(self);
}

static void chick_dodge(edict_t *self, edict_t *attacker, float eta)
{
	if (random() > 0.25f)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "duck");
}

static void ChickSlash(edict_t *self)
{
	vec3_t  aim;
	VectorSet(aim, MELEE_DISTANCE, self->server.mins[0], 10);
	gi.sound(self, CHAN_WEAPON, sound_melee_swing, 1, ATTN_NORM, 0);
	fire_hit(self, aim, (10 + (Q_rand() % 6)), 100);
}

static void ChickRocket(edict_t *self)
{
	vec3_t  forward, right;
	vec3_t  start;
	vec3_t  dir;
	vec3_t  vec;
	AngleVectors(self->server.state.angles, forward, right, NULL);
	G_ProjectSource(self->server.state.origin, monster_flash_offset[MZ2_CHICK_ROCKET_1], forward, right, start);
	VectorCopy(self->enemy->server.state.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);
	monster_fire_rocket(self, start, dir, 50, 500, MZ2_CHICK_ROCKET_1);
}

static void Chick_PreAttack1(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_missile_prelaunch, 1, ATTN_NORM, 0);
}

static void ChickReload(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_missile_reload, 1, ATTN_NORM, 0);
}

static void chick_rerocket(edict_t *self)
{
	if (self->enemy->health > 0 &&
		range(self, self->enemy) > RANGE_MELEE &&
		visible(self, self->enemy) &&
		random() <= 0.6f)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
		return;
	}

	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "end_attack1");
}

static void chick_attack1(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
}

static void chick_reslash(edict_t *self)
{
	if (self->enemy->health > 0 && range(self, self->enemy) == RANGE_MELEE)
	{
		if (random() <= 0.9f)
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "slash");
		else
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "end_slash");

		return;
	}

	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "end_slash");
}

static void chick_slash(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "slash");
}

static void chick_melee(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "start_slash");
}

static void chick_attack(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "start_attack1");
}

static void chick_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static const mevent_t events[] =
{
	EVENT_FUNC(chick_fidget),
	EVENT_FUNC(chick_stand),
	EVENT_FUNC(ChickMoan),
	EVENT_FUNC(chick_run),
	EVENT_FUNC(chick_dead),
	EVENT_FUNC(chick_duck_down),
	EVENT_FUNC(chick_duck_hold),
	EVENT_FUNC(chick_duck_up),
	EVENT_FUNC(Chick_PreAttack1),
	EVENT_FUNC(chick_attack1),
	EVENT_FUNC(ChickRocket),
	EVENT_FUNC(ChickReload),
	EVENT_FUNC(chick_rerocket),
	EVENT_FUNC(chick_slash),
	EVENT_FUNC(ChickSlash),
	EVENT_FUNC(chick_reslash),
	NULL
};

/*QUAKED monster_chick (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_chick(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/bitch/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/bitch.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_missile_prelaunch = gi.soundindex("chick/chkatck1.wav");
	sound_missile_launch    = gi.soundindex("chick/chkatck2.wav");
	sound_melee_swing       = gi.soundindex("chick/chkatck3.wav");
	sound_melee_hit         = gi.soundindex("chick/chkatck4.wav");
	sound_missile_reload    = gi.soundindex("chick/chkatck5.wav");
	sound_death1            = gi.soundindex("chick/chkdeth1.wav");
	sound_death2            = gi.soundindex("chick/chkdeth2.wav");
	sound_fall_down         = gi.soundindex("chick/chkfall1.wav");
	sound_idle1             = gi.soundindex("chick/chkidle1.wav");
	sound_idle2             = gi.soundindex("chick/chkidle2.wav");
	sound_pain1             = gi.soundindex("chick/chkpain1.wav");
	sound_pain2             = gi.soundindex("chick/chkpain2.wav");
	sound_pain3             = gi.soundindex("chick/chkpain3.wav");
	sound_sight             = gi.soundindex("chick/chksght1.wav");
	sound_search            = gi.soundindex("chick/chksrch1.wav");
	self->movetype = MOVETYPE_STEP;
	self->server.solid = SOLID_BBOX;
	self->server.state.modelindex = gi.modelindex(model_name);
	VectorSet(self->server.mins, -16, -16, 0);
	VectorSet(self->server.maxs, 16, 16, 56);
	self->health = 175;
	self->gib_health = -70;
	self->mass = 200;
	self->pain = chick_pain;
	self->die = chick_die;
	self->monsterinfo.stand = chick_stand;
	self->monsterinfo.walk = chick_walk;
	self->monsterinfo.run = chick_run;
	self->monsterinfo.dodge = chick_dodge;
	self->monsterinfo.attack = chick_attack;
	self->monsterinfo.melee = chick_melee;
	self->monsterinfo.sight = chick_sight;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
}

#endif
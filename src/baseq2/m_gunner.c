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

GUNNER

==============================================================================
*/
#ifdef ENABLE_COOP

#include "g_local.h"
#include "m_gunner.h"
#include "m_local.h"

static q_soundhandle sound_pain;
static q_soundhandle sound_pain2;
static q_soundhandle sound_death;
static q_soundhandle sound_idle;
static q_soundhandle sound_open;
static q_soundhandle sound_search;
static q_soundhandle sound_sight;

static mscript_t script;

static void gunner_idlesound(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void gunner_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void gunner_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static void gunner_fidget(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;

	if (random() <= 0.05f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "fidget");
}

static void gunner_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void gunner_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void gunner_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
}

static void gunner_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3000;

	if (Q_rand() & 1)
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	if (damage <= 10)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain3");
	else if (damage <= 25)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain2");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
}

static void gunner_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->s.clip_contents = CONTENTS_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

static void gunner_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = true;
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death");
}

static void GunnerGrenade(edict_t *self)
{
	vec3_t  start;
	vec3_t  forward, right;
	vec3_t  aim;
	int     flash_number;

	if (self->s.frame == FRAME_attak105)
		flash_number = MZ2_GUNNER_GRENADE_1;
	else if (self->s.frame == FRAME_attak108)
		flash_number = MZ2_GUNNER_GRENADE_2;
	else if (self->s.frame == FRAME_attak111)
		flash_number = MZ2_GUNNER_GRENADE_3;
	else // (self->s.frame == FRAME_attak114)
		flash_number = MZ2_GUNNER_GRENADE_4;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);
	//FIXME : do a spread -225 -75 75 225 degrees around forward
	VectorCopy(forward, aim);
	monster_fire_grenade(self, start, aim, 50, 600, flash_number);
}

static void gunner_duck_down(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;

	if (skill->value >= 2)
	{
		if (random() > 0.5f)
			GunnerGrenade(self);
	}

	self->maxs[2] -= 32;
	self->takedamage = true;
	self->monsterinfo.pausetime = level.time + 1000;
	gi.linkentity(self);
}

static void gunner_duck_hold(edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void gunner_duck_up(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = true;
	gi.linkentity(self);
}

static void gunner_dodge(edict_t *self, edict_t *attacker, float eta)
{
	if (random() > 0.25f)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "duck");
}

static void gunner_opengun(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_open, 1, ATTN_IDLE, 0);
}

static void GunnerFire(edict_t *self)
{
	vec3_t  start;
	vec3_t  forward, right;
	vec3_t  target;
	vec3_t  aim;
	int     flash_number;
	flash_number = MZ2_GUNNER_MACHINEGUN_1 + (self->s.frame - FRAME_attak216);
	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);
	// project enemy back a bit and target there
	VectorCopy(self->enemy->s.origin, target);
	VectorMA(target, -0.2f, self->enemy->velocity, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, aim);
	VectorNormalize(aim);
	monster_fire_bullet(self, start, aim, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

static void gunner_attack(edict_t *self)
{
	if (range(self, self->enemy) == RANGE_MELEE || random() <= 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_chain");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_grenade");
}

static void gunner_fire_chain(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "fire_chain");
}

static void gunner_refire_chain(edict_t *self)
{
	if (self->enemy->health > 0 && visible(self, self->enemy) &&
		random() <= 0.5f)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "fire_chain");
		return;
	}

	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "endfire_chain");
}

static const mevent_t events[] =
{
	EVENT_FUNC(gunner_fidget),
	EVENT_FUNC(gunner_stand),
	EVENT_FUNC(gunner_idlesound),
	EVENT_FUNC(gunner_run),
	EVENT_FUNC(gunner_dead),
	EVENT_FUNC(gunner_duck_down),
	EVENT_FUNC(gunner_duck_hold),
	EVENT_FUNC(gunner_duck_up),
	EVENT_FUNC(gunner_fire_chain),
	EVENT_FUNC(gunner_opengun),
	EVENT_FUNC(gunner_refire_chain),
	EVENT_FUNC(GunnerFire),
	EVENT_FUNC(GunnerGrenade),
	NULL
};

/*QUAKED monster_gunner (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_gunner(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/gunner/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/gunner.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_death = gi.soundindex("gunner/death1.wav");
	sound_pain = gi.soundindex("gunner/gunpain2.wav");
	sound_pain2 = gi.soundindex("gunner/gunpain1.wav");
	sound_idle = gi.soundindex("gunner/gunidle1.wav");
	sound_open = gi.soundindex("gunner/gunatck1.wav");
	sound_search = gi.soundindex("gunner/gunsrch1.wav");
	sound_sight = gi.soundindex("gunner/sight1.wav");
	gi.soundindex("gunner/gunatck2.wav");
	gi.soundindex("gunner/gunatck3.wav");
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 32);
	self->health = 175;
	self->gib_health = -70;
	self->mass = 200;
	self->pain = gunner_pain;
	self->die = gunner_die;
	self->monsterinfo.stand = gunner_stand;
	self->monsterinfo.walk = gunner_walk;
	self->monsterinfo.run = gunner_run;
	self->monsterinfo.dodge = gunner_dodge;
	self->monsterinfo.attack = gunner_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = gunner_sight;
	self->monsterinfo.search = gunner_search;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
}

#endif
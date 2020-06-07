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

GLADIATOR

==============================================================================
*/
#ifdef ENABLE_COOP

#include "g_local.h"
#include "m_gladiator.h"
#include "m_local.h"

static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_die;
static q_soundhandle sound_gun;
static q_soundhandle sound_cleaver_swing;
static q_soundhandle sound_cleaver_hit;
static q_soundhandle sound_cleaver_miss;
static q_soundhandle sound_idle;
static q_soundhandle sound_search;
static q_soundhandle sound_sight;

static mscript_t script;

static void gladiator_idle(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void gladiator_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void gladiator_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static void gladiator_cleaver_swing(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_cleaver_swing, 1, ATTN_NORM, 0);
}

static void gladiator_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void gladiator_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void gladiator_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
}

static void GaldiatorMelee(edict_t *self)
{
	vec3_t  aim;
	VectorSet(aim, MELEE_DISTANCE, self->mins[0], -4);

	if (fire_hit(self, aim, (20 + (Q_rand() % 5)), 300))
		gi.sound(self, CHAN_AUTO, sound_cleaver_hit, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_AUTO, sound_cleaver_miss, 1, ATTN_NORM, 0);
}

static void gladiator_melee(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_melee");
}

static void GladiatorGun(edict_t *self)
{
	vec3_t  start;
	vec3_t  dir;
	vec3_t  forward, right;
	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[MZ2_GLADIATOR_RAILGUN_1], forward, right, start);
	// calc direction to where we targted
	VectorSubtract(self->pos1, start, dir);
	VectorNormalize(dir);
	monster_fire_railgun(self, start, dir, 50, 100, MZ2_GLADIATOR_RAILGUN_1);
}

static void gladiator_attack(edict_t *self)
{
	float   range;
	vec3_t  v;
	// a small safe zone
	VectorSubtract(self->s.origin, self->enemy->s.origin, v);
	range = VectorLength(v);

	if (range <= (MELEE_DISTANCE + 32))
		return;

	// charge up the railgun
	gi.sound(self, CHAN_WEAPON, sound_gun, 1, ATTN_NORM, 0);
	VectorCopy(self->enemy->s.origin, self->pos1);  //save for aiming the shot
	self->pos1[2] += self->enemy->viewheight;
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_gun");
}

static void gladiator_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
	{
		if ((self->velocity[2] > 100) && (self->monsterinfo.currentmove == M_GetMonsterMove(&script, "pain")))
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain_air");

		return;
	}

	self->pain_debounce_time = level.time + 3000;

	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	if (self->velocity[2] > 100)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain_air");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain");
}

static void gladiator_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->s.clip_contents = CONTENTS_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

static void gladiator_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = true;
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death");
}

static const mevent_t events[] =
{
	EVENT_FUNC(gladiator_run),
	EVENT_FUNC(gladiator_cleaver_swing),
	EVENT_FUNC(GaldiatorMelee),
	EVENT_FUNC(GladiatorGun),
	EVENT_FUNC(gladiator_dead),
	NULL
};

/*QUAKED monster_gladiator (1 .5 0) (-32 -32 -24) (32 32 64) Ambush Trigger_Spawn Sight
*/
void SP_monster_gladiator(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/gladiatr/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/gladiator.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_pain1 = gi.soundindex("gladiator/pain.wav");
	sound_pain2 = gi.soundindex("gladiator/gldpain2.wav");
	sound_die = gi.soundindex("gladiator/glddeth2.wav");
	sound_gun = gi.soundindex("gladiator/railgun.wav");
	sound_cleaver_swing = gi.soundindex("gladiator/melee1.wav");
	sound_cleaver_hit = gi.soundindex("gladiator/melee2.wav");
	sound_cleaver_miss = gi.soundindex("gladiator/melee3.wav");
	sound_idle = gi.soundindex("gladiator/gldidle1.wav");
	sound_search = gi.soundindex("gladiator/gldsrch1.wav");
	sound_sight = gi.soundindex("gladiator/sight.wav");
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, 64);
	self->health = 400;
	self->gib_health = -175;
	self->mass = 400;
	self->pain = gladiator_pain;
	self->die = gladiator_die;
	self->monsterinfo.stand = gladiator_stand;
	self->monsterinfo.walk = gladiator_walk;
	self->monsterinfo.run = gladiator_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = gladiator_attack;
	self->monsterinfo.melee = gladiator_melee;
	self->monsterinfo.sight = gladiator_sight;
	self->monsterinfo.idle = gladiator_idle;
	self->monsterinfo.search = gladiator_search;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
}

#endif
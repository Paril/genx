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

BERSERK

==============================================================================
*/
#ifdef ENABLE_COOP

#include "g_local.h"
#include "m_berserk.h"
#include "m_local.h"

static q_soundhandle sound_pain;
static q_soundhandle sound_die;
static q_soundhandle sound_idle;
static q_soundhandle sound_punch;
static q_soundhandle sound_sight;
static q_soundhandle sound_search;

static mscript_t script;

static void berserk_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void berserk_fidget(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;

	if (random() > 0.15f)
		return;

	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand_fidget");
	gi.sound(self, CHAN_WEAPON, sound_idle, 1, ATTN_IDLE, 0);
}

static void berserk_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
}

static void berserk_attack_spike(edict_t *self)
{
	static  vec3_t  aim = {MELEE_DISTANCE, 0, -24};
	fire_hit(self, aim, (15 + (Q_rand() % 6)), 400);      //  Faster attack -- upwards and backwards
}

static void berserk_swing(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_punch, 1, ATTN_NORM, 0);
}

static void berserk_attack_club(edict_t *self)
{
	vec3_t  aim;
	VectorSet(aim, MELEE_DISTANCE, self->server.mins[0], -4);
	fire_hit(self, aim, (5 + (Q_rand() % 6)), 400);       // Slower attack
}

static void berserk_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->server.state.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3000;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	if ((damage < 20) || (random() < 0.5f))
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain2");
}

static void berserk_dead(edict_t *self)
{
	VectorSet(self->server.mins, -16, -16, -24);
	VectorSet(self->server.maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->server.flags.deadmonster = true;
	self->server.state.clip_contents = CONTENTS_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

static void berserk_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void berserk_melee(edict_t *self)
{
	if ((Q_rand() % 2) == 0)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_spike");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_club");
}

static void berserk_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int     n;

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

	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = true;

	if (damage >= 50)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death1");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death2");
}

static void berserk_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void berserk_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static const mevent_t events[] =
{
	EVENT_FUNC(berserk_stand),
	EVENT_FUNC(berserk_fidget),
	EVENT_FUNC(berserk_swing),
	EVENT_FUNC(berserk_attack_spike),
	EVENT_FUNC(berserk_attack_club),
	EVENT_FUNC(berserk_run),
	EVENT_FUNC(berserk_dead),
	NULL
};

/*QUAKED monster_berserk (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_berserk(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/berserk/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/berserk.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	// pre-caches
	sound_pain  = gi.soundindex("berserk/berpain2.wav");
	sound_die   = gi.soundindex("berserk/berdeth2.wav");
	sound_idle  = gi.soundindex("berserk/beridle1.wav");
	sound_punch = gi.soundindex("berserk/attack.wav");
	sound_search = gi.soundindex("berserk/bersrch1.wav");
	sound_sight = gi.soundindex("berserk/sight.wav");
	self->server.state.modelindex = gi.modelindex(model_name);
	VectorSet(self->server.mins, -16, -16, -24);
	VectorSet(self->server.maxs, 16, 16, 32);
	self->movetype = MOVETYPE_STEP;
	self->server.solid = SOLID_BBOX;
	self->health = 240;
	self->gib_health = -60;
	self->mass = 250;
	self->pain = berserk_pain;
	self->die = berserk_die;
	self->monsterinfo.stand = berserk_stand;
	self->monsterinfo.walk = berserk_walk;
	self->monsterinfo.run = berserk_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = NULL;
	self->monsterinfo.melee = berserk_melee;
	self->monsterinfo.sight = berserk_sight;
	self->monsterinfo.search = berserk_search;
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	gi.linkentity(self);
	walkmonster_start(self);
}

#endif
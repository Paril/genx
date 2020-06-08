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

FLIPPER

==============================================================================
*/

#ifdef ENABLE_COOP

#include "g_local.h"
#include "m_flipper.h"
#include "m_local.h"

static q_soundhandle sound_chomp;
static q_soundhandle sound_attack;
static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_death;
static q_soundhandle sound_idle;
static q_soundhandle sound_search;
static q_soundhandle sound_sight;

static mscript_t script;

static void flipper_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void flipper_run_loop(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run_loop");
}

static void flipper_run(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run_start");
}

static void flipper_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void flipper_start_run(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "start_run");
}

static void flipper_bite(edict_t *self)
{
	vec3_t  aim;
	VectorSet(aim, MELEE_DISTANCE, 0, 0);
	fire_hit(self, aim, 5, 0);
}

static void flipper_preattack(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_chomp, 1, ATTN_NORM, 0);
}

static void flipper_melee(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack");
}

static void flipper_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	int     n;

	if (self->health < (self->max_health / 2))
		self->server.state.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3000;

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	n = (Q_rand() + 1) % 2;

	if (n == 0)
	{
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain2");
	}
}

static void flipper_dead(edict_t *self)
{
	VectorSet(self->server.mins, -16, -16, -24);
	VectorSet(self->server.maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->server.flags.deadmonster = true;
	self->server.state.clip_contents = CONTENTS_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

static void flipper_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void flipper_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int     n;

	// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		for (n = 0; n < 2; n++)
			ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);

		for (n = 0; n < 2; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

		ThrowHead(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
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

static const mevent_t events[] =
{
	EVENT_FUNC(flipper_run),
	EVENT_FUNC(flipper_run_loop),
	EVENT_FUNC(flipper_preattack),
	EVENT_FUNC(flipper_bite),
	EVENT_FUNC(flipper_dead),
	NULL
};

/*QUAKED monster_flipper (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_flipper(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/flipper/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/flipper.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_pain1     = gi.soundindex("flipper/flppain1.wav");
	sound_pain2     = gi.soundindex("flipper/flppain2.wav");
	sound_death     = gi.soundindex("flipper/flpdeth1.wav");
	sound_chomp     = gi.soundindex("flipper/flpatck1.wav");
	sound_attack    = gi.soundindex("flipper/flpatck2.wav");
	sound_idle      = gi.soundindex("flipper/flpidle1.wav");
	sound_search    = gi.soundindex("flipper/flpsrch1.wav");
	sound_sight     = gi.soundindex("flipper/flpsght1.wav");
	self->movetype = MOVETYPE_STEP;
	self->server.solid = SOLID_BBOX;
	self->server.state.modelindex = gi.modelindex(model_name);
	VectorSet(self->server.mins, -16, -16, 0);
	VectorSet(self->server.maxs, 16, 16, 32);
	self->health = 50;
	self->gib_health = -30;
	self->mass = 100;
	self->pain = flipper_pain;
	self->die = flipper_die;
	self->monsterinfo.stand = flipper_stand;
	self->monsterinfo.walk = flipper_walk;
	self->monsterinfo.run = flipper_start_run;
	self->monsterinfo.melee = flipper_melee;
	self->monsterinfo.sight = flipper_sight;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	swimmonster_start(self);
}

#endif
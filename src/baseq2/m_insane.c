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

insane

==============================================================================
*/
#ifdef ENABLE_COOP

#include "g_local.h"
#include "m_insane.h"
#include "m_local.h"

static q_soundhandle sound_fist;
static q_soundhandle sound_shake;
static q_soundhandle sound_moan;
static q_soundhandle sound_scream[8];

static mscript_t script;

static void insane_fist(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_fist, 1, ATTN_IDLE, 0);
}

static void insane_shake(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_shake, 1, ATTN_IDLE, 0);
}

static void insane_moan(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_moan, 1, ATTN_IDLE, 0);
}

static void insane_scream(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_scream[Q_rand() % 8], 1, ATTN_IDLE, 0);
}

static void insane_cross(edict_t *self)
{
	if (random() < 0.8f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "cross");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "struggle_cross");
}

static void insane_walk(edict_t *self)
{
	if (self->spawnflags & 16)              // Hold Ground?
		if (self->server.state.frame == FRAME_cr_pain10)
		{
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "down");
			return;
		}

	if (self->spawnflags & 4)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "crawl");
	else if (random() <= 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk_normal");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk_insane");
}

static void insane_run(edict_t *self)
{
	if (self->spawnflags & 16)              // Hold Ground?
		if (self->server.state.frame == FRAME_cr_pain10)
		{
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "down");
			return;
		}

	if (self->spawnflags & 4)               // Crawling?
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "runcrawl");
	else if (random() <= 0.5f)              // Else, mix it up
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run_normal");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run_insane");
}

static void insane_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	int l, r;

	//  if (self->health < (self->max_health / 2))
	//      self->server.state.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3000;
	r = 1 + (Q_rand() & 1);

	if (self->health < 25)
		l = 25;
	else if (self->health < 50)
		l = 50;
	else if (self->health < 75)
		l = 75;
	else
		l = 100;

	gi.sound(self, CHAN_VOICE, gi.soundindex(va("player/male/pain%i_%i.wav", l, r)), 1, ATTN_IDLE, 0);

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	// Don't go into pain frames if crucified.
	if (self->spawnflags & 8)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "struggle_cross");
		return;
	}

	if (((self->server.state.frame >= FRAME_crawl1) && (self->server.state.frame <= FRAME_crawl9)) || ((self->server.state.frame >= FRAME_stand99) && (self->server.state.frame <= FRAME_stand160)))
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "crawl_pain");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand_pain");
}

static void insane_onground(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "down");
}

static void insane_checkdown(edict_t *self)
{
	//  if ( (self->server.state.frame == FRAME_stand94) || (self->server.state.frame == FRAME_stand65) )
	if (self->spawnflags & 32)              // Always stand
		return;

	if (random() < 0.3f)
	{
		if (random() < 0.5f)
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "uptodown");
		else
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "jumpdown");
	}
}

static void insane_checkup(edict_t *self)
{
	// If Hold_Ground and Crawl are set
	if ((self->spawnflags & 4) && (self->spawnflags & 16))
		return;

	if (random() < 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "downtoup");
}

static void insane_stand(edict_t *self)
{
	if (self->spawnflags & 8)           // If crucified
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "cross");
		self->monsterinfo.aiflags |= AI_STAND_GROUND;
	}
	// If Hold_Ground and Crawl are set
	else if ((self->spawnflags & 4) && (self->spawnflags & 16))
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "down");
	else if (random() < 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand_normal");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand_insane");
}

static void insane_dead(edict_t *self)
{
	if (self->spawnflags & 8)
		self->flags |= FL_FLY;
	else
	{
		VectorSet(self->server.mins, -16, -16, -24);
		VectorSet(self->server.maxs, 16, 16, -8);
		self->movetype = MOVETYPE_TOSS;
	}

	self->server.flags.deadmonster = true;
	self->server.state.clip_contents = CONTENTS_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

static void insane_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int     n;

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_IDLE, 0);

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

	gi.sound(self, CHAN_VOICE, gi.soundindex(va("player/male/death%i.wav", (Q_rand() % 4) + 1)), 1, ATTN_IDLE, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = true;

	if (self->spawnflags & 8)
		insane_dead(self);
	else
	{
		if (((self->server.state.frame >= FRAME_crawl1) && (self->server.state.frame <= FRAME_crawl9)) || ((self->server.state.frame >= FRAME_stand99) && (self->server.state.frame <= FRAME_stand160)))
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "crawl_death");
		else
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand_death");
	}
}

static const mevent_t events[] =
{
	EVENT_FUNC(insane_stand),
	EVENT_FUNC(insane_checkdown),
	EVENT_FUNC(insane_shake),
	EVENT_FUNC(insane_onground),
	EVENT_FUNC(insane_moan),
	EVENT_FUNC(insane_fist),
	EVENT_FUNC(insane_scream),
	EVENT_FUNC(insane_checkup),
	EVENT_FUNC(insane_walk),
	EVENT_FUNC(insane_run),
	EVENT_FUNC(insane_dead),
	EVENT_FUNC(insane_cross),
	NULL
};

/*QUAKED misc_insane (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn CRAWL CRUCIFIED STAND_GROUND ALWAYS_STAND
*/
void SP_misc_insane(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/insane/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/insane.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_fist = gi.soundindex("insane/insane11.wav");
	sound_shake = gi.soundindex("insane/insane5.wav");
	sound_moan = gi.soundindex("insane/insane7.wav");
	sound_scream[0] = gi.soundindex("insane/insane1.wav");
	sound_scream[1] = gi.soundindex("insane/insane2.wav");
	sound_scream[2] = gi.soundindex("insane/insane3.wav");
	sound_scream[3] = gi.soundindex("insane/insane4.wav");
	sound_scream[4] = gi.soundindex("insane/insane6.wav");
	sound_scream[5] = gi.soundindex("insane/insane8.wav");
	sound_scream[6] = gi.soundindex("insane/insane9.wav");
	sound_scream[7] = gi.soundindex("insane/insane10.wav");
	self->movetype = MOVETYPE_STEP;
	self->server.solid = SOLID_BBOX;
	self->server.state.modelindex = gi.modelindex(model_name);
	VectorSet(self->server.mins, -16, -16, -24);
	VectorSet(self->server.maxs, 16, 16, 32);
	self->health = 100;
	self->gib_health = -50;
	self->mass = 300;
	self->pain = insane_pain;
	self->die = insane_die;
	self->monsterinfo.stand = insane_stand;
	self->monsterinfo.walk = insane_walk;
	self->monsterinfo.run = insane_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = NULL;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = NULL;
	self->monsterinfo.aiflags |= AI_GOOD_GUY;
	self->server.state.skinnum = Q_rand_uniform(3);
	gi.linkentity(self);

	if (self->spawnflags & 16)              // Stand Ground
		self->monsterinfo.aiflags |= AI_STAND_GROUND;

	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand_normal");
	self->monsterinfo.scale = MODEL_SCALE;

	if (self->spawnflags & 8)                   // Crucified ?
	{
		VectorSet(self->server.mins, -16, 0, 0);
		VectorSet(self->server.maxs, 16, 8, 32);
		self->flags |= FL_NO_KNOCKBACK;
		flymonster_start(self);
	}
	else
	{
		walkmonster_start(self);
		self->server.state.skinnum = Q_rand() % 3;
	}
}

#endif
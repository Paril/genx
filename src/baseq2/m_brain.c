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

brain

==============================================================================
*/
#ifdef ENABLE_COOP

#include "g_local.h"
#include "m_brain.h"
#include "m_local.h"

static q_soundhandle sound_chest_open;
static q_soundhandle sound_tentacles_extend;
static q_soundhandle sound_tentacles_retract;
static q_soundhandle sound_death;
static q_soundhandle sound_idle1;
static q_soundhandle sound_idle2;
static q_soundhandle sound_idle3;
static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_sight;
static q_soundhandle sound_search;
static q_soundhandle sound_melee1;
static q_soundhandle sound_melee2;
static q_soundhandle sound_melee3;

static mscript_t script;

static void brain_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void brain_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static void brain_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void brain_idle(edict_t *self)
{
	gi.sound(self, CHAN_AUTO, sound_idle3, 1, ATTN_IDLE, 0);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "idle");
}

static void brain_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk1");
}

//
// DUCK
//
static void brain_duck_down(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32;
	self->takedamage = true;
	gi.linkentity(self);
}

static void brain_duck_hold(edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void brain_duck_up(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = true;
	gi.linkentity(self);
}

static void brain_dodge(edict_t *self, edict_t *attacker, float eta)
{
	if (random() > 0.25f)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.pausetime = level.time + (eta * 1000) + 500;
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "duck");
}

//
// MELEE
//

static void brain_swing_right(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_melee1, 1, ATTN_NORM, 0);
}

static void brain_hit_right(edict_t *self)
{
	vec3_t  aim;
	VectorSet(aim, MELEE_DISTANCE, self->maxs[0], 8);

	if (fire_hit(self, aim, (15 + (Q_rand() % 5)), 40))
		gi.sound(self, CHAN_WEAPON, sound_melee3, 1, ATTN_NORM, 0);
}

static void brain_swing_left(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_melee2, 1, ATTN_NORM, 0);
}

static void brain_hit_left(edict_t *self)
{
	vec3_t  aim;
	VectorSet(aim, MELEE_DISTANCE, self->mins[0], 8);

	if (fire_hit(self, aim, (15 + (Q_rand() % 5)), 40))
		gi.sound(self, CHAN_WEAPON, sound_melee3, 1, ATTN_NORM, 0);
}

static void brain_chest_open(edict_t *self)
{
	self->spawnflags &= ~65536;
	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;
	gi.sound(self, CHAN_BODY, sound_chest_open, 1, ATTN_NORM, 0);
}

static void brain_tentacle_attack(edict_t *self)
{
	vec3_t  aim;
	VectorSet(aim, MELEE_DISTANCE, 0, 8);

	if (fire_hit(self, aim, (10 + (Q_rand() % 5)), -600) && skill->value > 0)
		self->spawnflags |= 65536;

	gi.sound(self, CHAN_WEAPON, sound_tentacles_retract, 1, ATTN_NORM, 0);
}

static void brain_chest_closed(edict_t *self)
{
	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;

	if (self->spawnflags & 65536)
	{
		self->spawnflags &= ~65536;
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
	}
}

static void brain_melee(edict_t *self)
{
	if (random() <= 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack2");
}

static void brain_run(edict_t *self)
{
	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
}

static void brain_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	float   r;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3000;

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	r = random();

	if (r < 0.33f)
	{
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
	}
	else if (r < 0.66f)
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

static void brain_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->s.clip_contents = CONTENTS_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

static void brain_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int     n;
	self->s.effects = 0;
	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;

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

	if (random() <= 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death1");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death2");
}

static const mevent_t events[] =
{
	EVENT_FUNC(brain_stand),
	EVENT_FUNC(brain_run),
	EVENT_FUNC(brain_duck_down),
	EVENT_FUNC(brain_duck_hold),
	EVENT_FUNC(brain_duck_up),
	EVENT_FUNC(brain_dead),
	EVENT_FUNC(brain_swing_right),
	EVENT_FUNC(brain_hit_right),
	EVENT_FUNC(brain_swing_left),
	EVENT_FUNC(brain_hit_left),
	EVENT_FUNC(brain_chest_open),
	EVENT_FUNC(brain_tentacle_attack),
	EVENT_FUNC(brain_chest_closed),
	NULL
};

/*QUAKED monster_brain (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_brain(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/brain/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/brain.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_chest_open = gi.soundindex("brain/brnatck1.wav");
	sound_tentacles_extend = gi.soundindex("brain/brnatck2.wav");
	sound_tentacles_retract = gi.soundindex("brain/brnatck3.wav");
	sound_death = gi.soundindex("brain/brndeth1.wav");
	sound_idle1 = gi.soundindex("brain/brnidle1.wav");
	sound_idle2 = gi.soundindex("brain/brnidle2.wav");
	sound_idle3 = gi.soundindex("brain/brnlens1.wav");
	sound_pain1 = gi.soundindex("brain/brnpain1.wav");
	sound_pain2 = gi.soundindex("brain/brnpain2.wav");
	sound_sight = gi.soundindex("brain/brnsght1.wav");
	sound_search = gi.soundindex("brain/brnsrch1.wav");
	sound_melee1 = gi.soundindex("brain/melee1.wav");
	sound_melee2 = gi.soundindex("brain/melee2.wav");
	sound_melee3 = gi.soundindex("brain/melee3.wav");
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 32);
	self->health = 300;
	self->gib_health = -150;
	self->mass = 400;
	self->pain = brain_pain;
	self->die = brain_die;
	self->monsterinfo.stand = brain_stand;
	self->monsterinfo.walk = brain_walk;
	self->monsterinfo.run = brain_run;
	self->monsterinfo.dodge = brain_dodge;
	self->monsterinfo.melee = brain_melee;
	self->monsterinfo.sight = brain_sight;
	self->monsterinfo.search = brain_search;
	self->monsterinfo.idle = brain_idle;
	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
	self->monsterinfo.power_armor_power = 100;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
}

#endif
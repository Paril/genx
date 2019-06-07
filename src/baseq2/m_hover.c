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

hover

==============================================================================
*/

#include "g_local.h"
#include "m_hover.h"
#include "m_local.h"

static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_death1;
static q_soundhandle sound_death2;
static q_soundhandle sound_sight;
static q_soundhandle sound_search1;
static q_soundhandle sound_search2;

static mscript_t script;

static void hover_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void hover_search(edict_t *self)
{
	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_search1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_search2, 1, ATTN_NORM, 0);
}

static void hover_reattack(edict_t *self)
{
	if (self->enemy->health > 0 && visible(self, self->enemy) && random() <= 0.6f)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
		return;
	}

	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "end_attack");
}

static void hover_fire_blaster(edict_t *self)
{
	vec3_t  start;
	vec3_t  forward, right;
	vec3_t  end;
	vec3_t  dir;
	int     effect;

	if (self->s.frame == FRAME_attak104)
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[MZ2_HOVER_BLASTER_1], forward, right, start);
	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, dir);
	monster_fire_blaster(self, start, dir, 1, 1000, MZ2_HOVER_BLASTER_1, effect);
}

static void hover_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void hover_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
}

static void hover_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void hover_start_attack(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "start_attack");
}

static void hover_attack(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
}

static void hover_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3000;

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	if (damage <= 25)
	{
		if (random() < 0.5f)
		{
			gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain3");
		}
		else
		{
			gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain2");
		}
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
	}
}

static void hover_deadthink(edict_t *self)
{
	if (!self->groundentity && (level.time / 1000.0f) < self->timestamp)
	{
		self->nextthink = level.time + game.frametime;
		return;
	}

	BecomeExplosion1(self);
}

static void hover_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->think = hover_deadthink;
	self->nextthink = level.time + game.frametime;
	self->timestamp = level.time + 15000;
	gi.linkentity(self);
}

static void hover_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_death2, 1, ATTN_NORM, 0);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death1");
}

static const mevent_t events[] =
{
	EVENT_FUNC(hover_run),
	EVENT_FUNC(hover_dead),
	EVENT_FUNC(hover_attack),
	EVENT_FUNC(hover_fire_blaster),
	EVENT_FUNC(hover_reattack),
	NULL
};

/*QUAKED monster_hover (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_hover(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/hover/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/hover.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_pain1 = gi.soundindex("hover/hovpain1.wav");
	sound_pain2 = gi.soundindex("hover/hovpain2.wav");
	sound_death1 = gi.soundindex("hover/hovdeth1.wav");
	sound_death2 = gi.soundindex("hover/hovdeth2.wav");
	sound_sight = gi.soundindex("hover/hovsght1.wav");
	sound_search1 = gi.soundindex("hover/hovsrch1.wav");
	sound_search2 = gi.soundindex("hover/hovsrch2.wav");
	gi.soundindex("hover/hovatck1.wav");
	self->s.sound = gi.soundindex("hover/hovidle1.wav");
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -24, -24, -24);
	VectorSet(self->maxs, 24, 24, 32);
	self->health = 240;
	self->gib_health = -100;
	self->mass = 150;
	self->pain = hover_pain;
	self->die = hover_die;
	self->monsterinfo.stand = hover_stand;
	self->monsterinfo.walk = hover_walk;
	self->monsterinfo.run = hover_run;
	self->monsterinfo.attack = hover_start_attack;
	self->monsterinfo.sight = hover_sight;
	self->monsterinfo.search = hover_search;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	flymonster_start(self);
}

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

parasite

==============================================================================
*/

#ifdef ENABLE_COOP
#include "g_local.h"
#include "m_parasite.h"
#include "m_local.h"

static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_die;
static q_soundhandle sound_launch;
static q_soundhandle sound_impact;
static q_soundhandle sound_suck;
static q_soundhandle sound_reelin;
static q_soundhandle sound_sight;
static q_soundhandle sound_tap;
static q_soundhandle sound_scratch;
static q_soundhandle sound_search;

static mscript_t script;

static void parasite_launch(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_launch, 1, ATTN_NORM, 0);
}

static void parasite_reel_in(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_reelin, 1, ATTN_NORM, 0);
}

static void parasite_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_WEAPON, sound_sight, 1, ATTN_NORM, 0);
}

static void parasite_tap(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_tap, 1, ATTN_IDLE, 0);
}

static void parasite_scratch(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_scratch, 1, ATTN_IDLE, 0);
}

static void parasite_search(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_search, 1, ATTN_IDLE, 0);
}

static void parasite_end_fidget(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "end_fidget");
}

static void parasite_do_fidget(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "fidget");
}

static void parasite_refidget(edict_t *self)
{
	if (random() <= 0.8f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "fidget");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "end_fidget");
}

static void parasite_idle(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "start_fidget");
}

static void parasite_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void parasite_start_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "start_run");
}

static void parasite_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
}

static void parasite_start_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "start_walk");
}

static void parasite_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void parasite_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->server.state.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3000;

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
}

static bool parasite_drain_attack_ok(vec3_t start, vec3_t end)
{
	vec3_t  dir, angles;
	// check for max distance
	VectorSubtract(start, end, dir);

	if (VectorLength(dir) > 256)
		return false;

	// check for min/max pitch
	vectoangles(dir, angles);

	if (angles[0] < -180)
		angles[0] += 360;

	if (fabsf(angles[0]) > 30)
		return false;

	return true;
}

static void parasite_drain_attack(edict_t *self)
{
	vec3_t  offset, start, f, r, end, dir;
	trace_t tr;
	int damage;
	AngleVectors(self->server.state.angles, f, r, NULL);
	VectorSet(offset, 24, 0, 6);
	G_ProjectSource(self->server.state.origin, offset, f, r, start);
	VectorCopy(self->enemy->server.state.origin, end);

	if (!parasite_drain_attack_ok(start, end))
	{
		end[2] = self->enemy->server.state.origin[2] + self->enemy->server.maxs[2] - 8;

		if (!parasite_drain_attack_ok(start, end))
		{
			end[2] = self->enemy->server.state.origin[2] + self->enemy->server.mins[2] + 8;

			if (!parasite_drain_attack_ok(start, end))
				return;
		}
	}

	VectorCopy(self->enemy->server.state.origin, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);

	if (tr.ent != self->enemy)
		return;

	if (self->server.state.frame == FRAME_drain03)
	{
		damage = 5;
		gi.sound(self->enemy, CHAN_AUTO, sound_impact, 1, ATTN_NORM, 0);
	}
	else
	{
		if (self->server.state.frame == FRAME_drain04)
			gi.sound(self, CHAN_WEAPON, sound_suck, 1, ATTN_NORM, 0);

		damage = 2;
	}

	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_PARASITE_ATTACK);
	MSG_WriteShort(self - g_edicts);
	MSG_WritePos(start);
	MSG_WritePos(end);
	gi.multicast(self->server.state.origin, MULTICAST_PVS);
	VectorSubtract(start, end, dir);
	T_Damage(self->enemy, self, self, dir, self->enemy->server.state.origin, vec3_origin, damage, 0, DAMAGE_NO_KNOCKBACK, MakeAttackerMeansOfDeath(self, self, MD_MELEE, DT_DIRECT));
}

static void parasite_attack(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "drain");
}

static void parasite_dead(edict_t *self)
{
	VectorSet(self->server.mins, -16, -16, -24);
	VectorSet(self->server.maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->server.flags.deadmonster = true;
	self->server.state.clip_contents = CONTENTS_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

static void parasite_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
	EVENT_FUNC(parasite_stand),
	EVENT_FUNC(parasite_tap),
	EVENT_FUNC(parasite_do_fidget),
	EVENT_FUNC(parasite_refidget),
	EVENT_FUNC(parasite_scratch),
	EVENT_FUNC(parasite_run),
	EVENT_FUNC(parasite_walk),
	EVENT_FUNC(parasite_start_run),
	EVENT_FUNC(parasite_launch),
	EVENT_FUNC(parasite_drain_attack),
	EVENT_FUNC(parasite_reel_in),
	EVENT_FUNC(parasite_dead),
	NULL
};

/*QUAKED monster_parasite (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_parasite(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/parasite/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/parasite.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_pain1 = gi.soundindex("parasite/parpain1.wav");
	sound_pain2 = gi.soundindex("parasite/parpain2.wav");
	sound_die = gi.soundindex("parasite/pardeth1.wav");
	sound_launch = gi.soundindex("parasite/paratck1.wav");
	sound_impact = gi.soundindex("parasite/paratck2.wav");
	sound_suck = gi.soundindex("parasite/paratck3.wav");
	sound_reelin = gi.soundindex("parasite/paratck4.wav");
	sound_sight = gi.soundindex("parasite/parsght1.wav");
	sound_tap = gi.soundindex("parasite/paridle1.wav");
	sound_scratch = gi.soundindex("parasite/paridle2.wav");
	sound_search = gi.soundindex("parasite/parsrch1.wav");
	self->server.state.modelindex = gi.modelindex(model_name);
	VectorSet(self->server.mins, -16, -16, -24);
	VectorSet(self->server.maxs, 16, 16, 24);
	self->movetype = MOVETYPE_STEP;
	self->server.solid = SOLID_BBOX;
	self->health = 175;
	self->gib_health = -50;
	self->mass = 250;
	self->pain = parasite_pain;
	self->die = parasite_die;
	self->monsterinfo.stand = parasite_stand;
	self->monsterinfo.walk = parasite_start_walk;
	self->monsterinfo.run = parasite_start_run;
	self->monsterinfo.attack = parasite_attack;
	self->monsterinfo.sight = parasite_sight;
	self->monsterinfo.search = parasite_search;
	self->monsterinfo.idle = parasite_idle;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
}

#endif
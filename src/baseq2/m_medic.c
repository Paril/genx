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

MEDIC

==============================================================================
*/

#include "g_local.h"
#include "m_medic.h"
#include "m_local.h"

static q_soundhandle sound_idle1;
static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_die;
static q_soundhandle sound_sight;
static q_soundhandle sound_search;
static q_soundhandle sound_hook_launch;
static q_soundhandle sound_hook_hit;
static q_soundhandle sound_hook_heal;
static q_soundhandle sound_hook_retract;

static mscript_t script;

static edict_t *medic_FindDeadMonster(edict_t *self)
{
	edict_t *ent = NULL;
	edict_t *best = NULL;

	while ((ent = findradius(ent, self->s.origin, 1024)) != NULL)
	{
		if (ent == self)
			continue;

		if (!(ent->svflags & (SVF_MONSTER | SVF_DEADMONSTER)))
			continue;

		if (ent->monsterinfo.aiflags & AI_GOOD_GUY)
			continue;

		if (ent->owner)
			continue;

		if (ent->health > 0)
			continue;

		if (!visible(self, ent))
			continue;

		if (!best)
		{
			best = ent;
			continue;
		}

		if (ent->max_health <= best->max_health)
			continue;

		best = ent;
	}

	return best;
}

static void medic_idle(edict_t *self)
{
	edict_t *ent;
	gi.sound(self, CHAN_VOICE, sound_idle1, 1, ATTN_IDLE, 0);
	ent = medic_FindDeadMonster(self);

	if (ent)
	{
		self->enemy = ent;
		self->enemy->owner = self;
		self->monsterinfo.aiflags |= AI_MEDIC;
		FoundTarget(self);
	}
}

static void medic_search(edict_t *self)
{
	edict_t *ent;
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_IDLE, 0);

	if (!self->oldenemy)
	{
		ent = medic_FindDeadMonster(self);

		if (ent)
		{
			self->oldenemy = self->enemy;
			self->enemy = ent;
			self->enemy->owner = self;
			self->monsterinfo.aiflags |= AI_MEDIC;
			FoundTarget(self);
		}
	}
}

static void medic_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void medic_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void medic_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void medic_run(edict_t *self)
{
	if (!(self->monsterinfo.aiflags & AI_MEDIC))
	{
		edict_t *ent;
		ent = medic_FindDeadMonster(self);

		if (ent)
		{
			self->oldenemy = self->enemy;
			self->enemy = ent;
			self->enemy->owner = self;
			self->monsterinfo.aiflags |= AI_MEDIC;
			FoundTarget(self);
			return;
		}
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
}

static void medic_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3000;

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	if (random() < 0.5f)
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

static void medic_fire_blaster(edict_t *self)
{
	vec3_t  start;
	vec3_t  forward, right;
	vec3_t  end;
	vec3_t  dir;
	int     effect;

	if ((self->s.frame == FRAME_attack9) || (self->s.frame == FRAME_attack12))
		effect = EF_BLASTER;
	else if ((self->s.frame == FRAME_attack19) || (self->s.frame == FRAME_attack22) || (self->s.frame == FRAME_attack25) || (self->s.frame == FRAME_attack28))
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[MZ2_MEDIC_BLASTER_1], forward, right, start);
	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, dir);
	monster_fire_blaster(self, start, dir, 2, 1000, MZ2_MEDIC_BLASTER_1, effect);
}

static void medic_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->s.clip_contents = CONTENTS_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

static void medic_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int     n;

	// if we had a pending patient, free him up for another medic
	if ((self->enemy) && (self->enemy->owner == self))
		self->enemy->owner = NULL;

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
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death");
}

static void medic_duck_down(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.pausetime = level.time + 1000;
	gi.linkentity(self);
}

static void medic_duck_hold(edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void medic_duck_up(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity(self);
}

static void medic_dodge(edict_t *self, edict_t *attacker, float eta)
{
	if (random() > 0.25f)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "duck");
}

static void medic_continue(edict_t *self)
{
	if (visible(self, self->enemy) && random() <= 0.95f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attackHyperBlaster");
}

static void medic_hook_launch(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_hook_launch, 1, ATTN_NORM, 0);
}

void ED_CallSpawn(edict_t *ent);

static vec3_t   medic_cable_offsets[] =
{
	{ 45.0,  -9.2, 15.5 },
	{ 48.4,  -9.7, 15.2 },
	{ 47.8,  -9.8, 15.8 },
	{ 47.3,  -9.3, 14.3 },
	{ 45.4, -10.1, 13.1 },
	{ 41.9, -12.7, 12.0 },
	{ 37.8, -15.8, 11.2 },
	{ 34.3, -18.4, 10.7 },
	{ 32.7, -19.7, 10.4 },
	{ 32.7, -19.7, 10.4 }
};

static void medic_cable_attack(edict_t *self)
{
	vec3_t  offset, start, end, f, r;
	trace_t tr;
	vec3_t  dir, angles;
	float   distance;

	if (!self->enemy->inuse)
		return;

	AngleVectors(self->s.angles, f, r, NULL);
	VectorCopy(medic_cable_offsets[self->s.frame - FRAME_attack42], offset);
	G_ProjectSource(self->s.origin, offset, f, r, start);
	// check for max distance
	VectorSubtract(start, self->enemy->s.origin, dir);
	distance = VectorLength(dir);

	if (distance > 256)
		return;

	// check for min/max pitch
	vectoangles(dir, angles);

	if (angles[0] < -180)
		angles[0] += 360;

	if (fabsf(angles[0]) > 45)
		return;

	tr = gi.trace(start, NULL, NULL, self->enemy->s.origin, self, MASK_SHOT);

	if (tr.fraction != 1.0f && tr.ent != self->enemy)
		return;

	if (self->s.frame == FRAME_attack43)
	{
		gi.sound(self->enemy, CHAN_AUTO, sound_hook_hit, 1, ATTN_NORM, 0);
		self->enemy->monsterinfo.aiflags |= AI_RESURRECTING;
	}
	else if (self->s.frame == FRAME_attack50)
	{
		self->enemy->spawnflags = 0;
		self->enemy->monsterinfo.aiflags = 0;
		self->enemy->target = NULL;
		self->enemy->targetname = NULL;
		self->enemy->combattarget = NULL;
		self->enemy->deathtarget = NULL;
		self->enemy->owner = self;
		ED_CallSpawn(self->enemy);
		self->enemy->owner = NULL;

		if (self->enemy->think)
		{
			self->enemy->nextthink = level.time;
			self->enemy->think(self->enemy);
		}

		self->enemy->monsterinfo.aiflags |= AI_RESURRECTING;

		if (self->oldenemy && self->oldenemy->client)
		{
			self->enemy->enemy = self->oldenemy;
			FoundTarget(self->enemy);
		}
	}
	else
	{
		if (self->s.frame == FRAME_attack44)
			gi.sound(self, CHAN_WEAPON, sound_hook_heal, 1, ATTN_NORM, 0);
	}

	// adjust start for beam origin being in middle of a segment
	VectorMA(start, 8, f, start);
	// adjust end z for end spot since the monster is currently dead
	VectorCopy(self->enemy->s.origin, end);
	end[2] = self->enemy->absmin[2] + self->enemy->size[2] / 2;
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_MEDIC_CABLE_ATTACK);
	gi.WriteShort(self - g_edicts);
	gi.WritePosition(start);
	gi.WritePosition(end);
	gi.multicast(self->s.origin, MULTICAST_PVS);
}

static void medic_hook_retract(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_hook_retract, 1, ATTN_NORM, 0);
	self->enemy->monsterinfo.aiflags &= ~AI_RESURRECTING;
}

static void medic_attack(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_MEDIC)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attackCable");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attackBlaster");
}

static bool medic_checkattack(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_MEDIC)
	{
		medic_attack(self);
		return true;
	}

	return M_CheckAttack(self);
}

static const mevent_t events[] =
{
	EVENT_FUNC(medic_idle),
	EVENT_FUNC(medic_run),
	EVENT_FUNC(medic_dead),
	EVENT_FUNC(medic_duck_down),
	EVENT_FUNC(medic_duck_hold),
	EVENT_FUNC(medic_duck_up),
	EVENT_FUNC(medic_fire_blaster),
	EVENT_FUNC(medic_continue),
	EVENT_FUNC(medic_hook_launch),
	EVENT_FUNC(medic_cable_attack),
	EVENT_FUNC(medic_hook_retract),
	NULL
};

/*QUAKED monster_medic (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_medic(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/medic/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/medic.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_idle1 = gi.soundindex("medic/idle.wav");
	sound_pain1 = gi.soundindex("medic/medpain1.wav");
	sound_pain2 = gi.soundindex("medic/medpain2.wav");
	sound_die = gi.soundindex("medic/meddeth1.wav");
	sound_sight = gi.soundindex("medic/medsght1.wav");
	sound_search = gi.soundindex("medic/medsrch1.wav");
	sound_hook_launch = gi.soundindex("medic/medatck2.wav");
	sound_hook_hit = gi.soundindex("medic/medatck3.wav");
	sound_hook_heal = gi.soundindex("medic/medatck4.wav");
	sound_hook_retract = gi.soundindex("medic/medatck5.wav");
	gi.soundindex("medic/medatck1.wav");
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -24, -24, -24);
	VectorSet(self->maxs, 24, 24, 32);
	self->health = 300;
	self->gib_health = -130;
	self->mass = 400;
	self->pain = medic_pain;
	self->die = medic_die;
	self->monsterinfo.stand = medic_stand;
	self->monsterinfo.walk = medic_walk;
	self->monsterinfo.run = medic_run;
	self->monsterinfo.dodge = medic_dodge;
	self->monsterinfo.attack = medic_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = medic_sight;
	self->monsterinfo.idle = medic_idle;
	self->monsterinfo.search = medic_search;
	self->monsterinfo.checkattack = medic_checkattack;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
}

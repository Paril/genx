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

mutant

==============================================================================
*/

#ifdef ENABLE_COOP
#include "g_local.h"
#include "m_mutant.h"
#include "m_local.h"

static q_soundhandle sound_swing;
static q_soundhandle sound_hit;
static q_soundhandle sound_hit2;
static q_soundhandle sound_death;
static q_soundhandle sound_idle;
static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_sight;
static q_soundhandle sound_search;
static q_soundhandle sound_step1;
static q_soundhandle sound_step2;
static q_soundhandle sound_step3;
static q_soundhandle sound_thud;

static mscript_t script;

//
// SOUNDS
//

static void mutant_step(edict_t *self)
{
	int     n;
	n = (Q_rand() + 1) % 3;

	if (n == 0)
		gi.sound(self, CHAN_VOICE, sound_step1, 1, ATTN_NORM, 0);
	else if (n == 1)
		gi.sound(self, CHAN_VOICE, sound_step2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_step3, 1, ATTN_NORM, 0);
}

static void mutant_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void mutant_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static void mutant_swing(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_swing, 1, ATTN_NORM, 0);
}

//
// STAND
//

static void mutant_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

//
// IDLE
//

static void mutant_idle_loop(edict_t *self)
{
	if (random() < 0.75f)
		self->monsterinfo.nextframe = FRAME_stand155;
}

static void mutant_idle(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "idle");
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

//
// WALK
//

static void mutant_walk_loop(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void mutant_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "start_walk");
}


//
// RUN
//

static void mutant_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
}

//
// MELEE
//

static void mutant_hit_left(edict_t *self)
{
	vec3_t  aim;
	VectorSet(aim, MELEE_DISTANCE, self->mins[0], 8);

	if (fire_hit(self, aim, (10 + (Q_rand() % 5)), 100))
		gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_WEAPON, sound_swing, 1, ATTN_NORM, 0);
}

static void mutant_hit_right(edict_t *self)
{
	vec3_t  aim;
	VectorSet(aim, MELEE_DISTANCE, self->maxs[0], 8);

	if (fire_hit(self, aim, (10 + (Q_rand() % 5)), 100))
		gi.sound(self, CHAN_WEAPON, sound_hit2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_WEAPON, sound_swing, 1, ATTN_NORM, 0);
}

static void mutant_check_refire(edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse || self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5f)) || (range(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attack09;
}

static void mutant_melee(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack");
}

//
// ATTACK
//

static void mutant_jump_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (self->health <= 0)
	{
		self->touch = NULL;
		return;
	}

	if (other->takedamage)
	{
		if (VectorLength(self->velocity) > 400)
		{
			vec3_t  point;
			vec3_t  normal;
			int     damage;
			VectorCopy(self->velocity, normal);
			VectorNormalize(normal);
			VectorMA(self->s.origin, self->maxs[0], normal, point);
			damage = 40 + 10 * random();
			T_Damage(other, self, self, self->velocity, point, normal, damage, damage, DAMAGE_NONE, MakeAttackerMeansOfDeath(self, self, MD_NONE, DT_DIRECT));
		}
	}

	if (!M_CheckBottom(self))
	{
		if (self->groundentity)
		{
			self->monsterinfo.nextframe = FRAME_attack02;
			self->touch = NULL;
		}

		return;
	}

	self->touch = NULL;
}

static void mutant_jump_takeoff(edict_t *self)
{
	vec3_t  forward;
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	AngleVectors(self->s.angles, forward, NULL, NULL);
	self->s.origin[2] += 1;
	VectorScale(forward, 600, self->velocity);
	self->velocity[2] = 250;
	self->groundentity = NULL;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->monsterinfo.attack_finished = level.time + 3000;
	self->touch = mutant_jump_touch;
}

static void mutant_check_landing(edict_t *self)
{
	if (self->groundentity)
	{
		gi.sound(self, CHAN_WEAPON, sound_thud, 1, ATTN_NORM, 0);
		self->monsterinfo.attack_finished = 0;
		self->monsterinfo.aiflags &= ~AI_DUCKED;
		return;
	}

	if (level.time > self->monsterinfo.attack_finished)
		self->monsterinfo.nextframe = FRAME_attack02;
	else
		self->monsterinfo.nextframe = FRAME_attack05;
}

static void mutant_jump(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "jump");
}

//
// CHECKATTACK
//

static bool mutant_check_melee(edict_t *self)
{
	if (range(self, self->enemy) == RANGE_MELEE)
		return true;

	return false;
}

static bool mutant_check_jump(edict_t *self)
{
	vec3_t  v;
	float   distance;

	if (self->absmin[2] > (self->enemy->absmin[2] + 0.75f * self->enemy->size[2]))
		return false;

	if (self->absmax[2] < (self->enemy->absmin[2] + 0.25f * self->enemy->size[2]))
		return false;

	v[0] = self->s.origin[0] - self->enemy->s.origin[0];
	v[1] = self->s.origin[1] - self->enemy->s.origin[1];
	v[2] = 0;
	distance = VectorLength(v);

	if (distance < 100)
		return false;

	if (distance > 100)
	{
		if (random() < 0.9f)
			return false;
	}

	return true;
}

static bool mutant_checkattack(edict_t *self)
{
	if (!self->enemy || self->enemy->health <= 0)
		return false;

	if (mutant_check_melee(self))
	{
		self->monsterinfo.attack_state = AS_MELEE;
		return true;
	}

	if (mutant_check_jump(self))
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		// FIXME play a jump sound here
		return true;
	}

	return false;
}

//
// PAIN
//

static void mutant_pain(edict_t *self, edict_t *other, float kick, int damage)
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

//
// DEATH
//

static void mutant_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->s.clip_contents = CONTENTS_DEADMONSTER;
	gi.linkentity(self);
	M_FlyCheck(self);
}

static void mutant_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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

	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = true;
	self->s.skinnum = 1;

	if (random() < 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death1");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death2");
}

//
// SPAWN
//

static const mevent_t events[] =
{
	EVENT_FUNC(mutant_stand),
	EVENT_FUNC(mutant_idle_loop),
	EVENT_FUNC(mutant_walk_loop),
	EVENT_FUNC(mutant_step),
	EVENT_FUNC(mutant_run),
	EVENT_FUNC(mutant_hit_left),
	EVENT_FUNC(mutant_hit_right),
	EVENT_FUNC(mutant_check_refire),
	EVENT_FUNC(mutant_jump_takeoff),
	EVENT_FUNC(mutant_check_landing),
	EVENT_FUNC(mutant_dead),
	NULL
};

/*QUAKED monster_mutant (1 .5 0) (-32 -32 -24) (32 32 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_mutant(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/mutant/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/mutant.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_swing = gi.soundindex("mutant/mutatck1.wav");
	sound_hit = gi.soundindex("mutant/mutatck2.wav");
	sound_hit2 = gi.soundindex("mutant/mutatck3.wav");
	sound_death = gi.soundindex("mutant/mutdeth1.wav");
	sound_idle = gi.soundindex("mutant/mutidle1.wav");
	sound_pain1 = gi.soundindex("mutant/mutpain1.wav");
	sound_pain2 = gi.soundindex("mutant/mutpain2.wav");
	sound_sight = gi.soundindex("mutant/mutsght1.wav");
	sound_search = gi.soundindex("mutant/mutsrch1.wav");
	sound_step1 = gi.soundindex("mutant/step1.wav");
	sound_step2 = gi.soundindex("mutant/step2.wav");
	sound_step3 = gi.soundindex("mutant/step3.wav");
	sound_thud = gi.soundindex("mutant/thud1.wav");
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, 48);
	self->health = 300;
	self->gib_health = -120;
	self->mass = 300;
	self->pain = mutant_pain;
	self->die = mutant_die;
	self->monsterinfo.stand = mutant_stand;
	self->monsterinfo.walk = mutant_walk;
	self->monsterinfo.run = mutant_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = mutant_jump;
	self->monsterinfo.melee = mutant_melee;
	self->monsterinfo.sight = mutant_sight;
	self->monsterinfo.search = mutant_search;
	self->monsterinfo.idle = mutant_idle;
	self->monsterinfo.checkattack = mutant_checkattack;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
}

#endif
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

jorg

==============================================================================
*/

#ifdef ENABLE_COOP
#include "g_local.h"
#include "m_boss31.h"
#include "m_local.h"

static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_pain3;
static q_soundhandle sound_idle;
static q_soundhandle sound_death;
static q_soundhandle sound_search1;
static q_soundhandle sound_search2;
static q_soundhandle sound_search3;
static q_soundhandle sound_attack1;
static q_soundhandle sound_attack2;
static q_soundhandle sound_firegun;
static q_soundhandle sound_step_left;
static q_soundhandle sound_step_right;
static q_soundhandle sound_death_hit;

static mscript_t script;

void BossExplode(edict_t *self);
void MakronToss(edict_t *self);
void MakronPrecache(void);

static void jorg_search(edict_t *self)
{
	float r;
	r = random();

	if (r <= 0.3f)
		gi.sound(self, CHAN_VOICE, sound_search1, 1, ATTN_NORM, 0);
	else if (r <= 0.6f)
		gi.sound(self, CHAN_VOICE, sound_search2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_search3, 1, ATTN_NORM, 0);
}

static void jorg_idle(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_NORM, 0);
}

static void jorg_death_hit(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_death_hit, 1, ATTN_NORM, 0);
}

static void jorg_step_left(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step_left, 1, ATTN_NORM, 0);
}

static void jorg_step_right(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step_right, 1, ATTN_NORM, 0);
}

static void jorg_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void jorg_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void jorg_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
}

static void jorg_reattack1(edict_t *self)
{
	if (visible(self, self->enemy) && random() < 0.9f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
	else
	{
		self->server.state.sound = 0;
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "end_attack1");
	}
}

static void jorg_attack1(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
}

static void jorg_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->server.state.skinnum = 1;

	self->server.state.sound = 0;

	if (level.time < self->pain_debounce_time)
		return;

	// Lessen the chance of him going into his pain frames if he takes little damage
	if (damage <= 40 && random() <= 0.6f)
		return;

	/*
	If he's entering his attack1 or using attack1, lessen the chance of him
	going into pain
	*/

	if ((self->server.state.frame >= FRAME_attak101) && (self->server.state.frame <= FRAME_attak108))
		if (random() <= 0.005f)
			return;

	if ((self->server.state.frame >= FRAME_attak109) && (self->server.state.frame <= FRAME_attak114))
		if (random() <= 0.00005f)
			return;

	if ((self->server.state.frame >= FRAME_attak201) && (self->server.state.frame <= FRAME_attak208))
		if (random() <= 0.005f)
			return;

	self->pain_debounce_time = level.time + 3000;

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	if (damage <= 50)
	{
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
	}
	else if (damage <= 100)
	{
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain2");
	}
	else
	{
		if (random() <= 0.3f)
		{
			gi.sound(self, CHAN_VOICE, sound_pain3, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain3");
		}
	}
}

static void jorgBFG(edict_t *self)
{
	vec3_t  forward, right;
	vec3_t  start;
	vec3_t  dir;
	vec3_t  vec;
	AngleVectors(self->server.state.angles, forward, right, NULL);
	G_ProjectSource(self->server.state.origin, monster_flash_offset[MZ2_JORG_BFG_1], forward, right, start);
	VectorCopy(self->enemy->server.state.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);
	gi.sound(self, CHAN_VOICE, sound_attack2, 1, ATTN_NORM, 0);
	monster_fire_bfg(self, start, dir, 50, 300, 100, 200, MZ2_JORG_BFG_1);
}

static void jorg_firebullet_right(edict_t *self)
{
	vec3_t  forward, right, target;
	vec3_t  start;
	AngleVectors(self->server.state.angles, forward, right, NULL);
	G_ProjectSource(self->server.state.origin, monster_flash_offset[MZ2_JORG_MACHINEGUN_R1], forward, right, start);
	VectorMA(self->enemy->server.state.origin, -0.2f, self->enemy->velocity, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, forward);
	VectorNormalize(forward);
	monster_fire_bullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_JORG_MACHINEGUN_R1);
}

static void jorg_firebullet_left(edict_t *self)
{
	vec3_t  forward, right, target;
	vec3_t  start;
	AngleVectors(self->server.state.angles, forward, right, NULL);
	G_ProjectSource(self->server.state.origin, monster_flash_offset[MZ2_JORG_MACHINEGUN_L1], forward, right, start);
	VectorMA(self->enemy->server.state.origin, -0.2f, self->enemy->velocity, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, forward);
	VectorNormalize(forward);
	monster_fire_bullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_JORG_MACHINEGUN_L1);
}

static void jorg_firebullet(edict_t *self)
{
	jorg_firebullet_left(self);
	jorg_firebullet_right(self);
}

static void jorg_attack(edict_t *self)
{
	if (random() <= 0.75f)
	{
		gi.sound(self, CHAN_VOICE, sound_attack1, 1, ATTN_NORM, 0);
		self->server.state.sound = gi.soundindex("boss3/w_loop.wav");
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "start_attack1");
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_attack2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack2");
	}
}

static void jorg_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = false;
	self->server.state.sound = 0;
	self->count = 0;
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death");
}

static bool Jorg_CheckAttack(edict_t *self)
{
	vec3_t  spot1, spot2;
	vec3_t  temp;
	float   chance;
	trace_t tr;
	int         enemy_range;
	float       enemy_yaw;

	if (self->enemy->health > 0)
	{
		// see if any entities are in the way of the shot
		VectorCopy(self->server.state.origin, spot1);
		spot1[2] += self->viewheight;
		VectorCopy(self->enemy->server.state.origin, spot2);
		spot2[2] += self->enemy->viewheight;
		tr = gi.trace(spot1, NULL, NULL, spot2, self, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_SLIME | CONTENTS_LAVA);

		// do we have a clear shot?
		if (tr.ent != self->enemy)
			return false;
	}

	enemy_range = range(self, self->enemy);
	VectorSubtract(self->enemy->server.state.origin, self->server.state.origin, temp);
	enemy_yaw = vectoyaw(temp);
	self->ideal_yaw = enemy_yaw;

	// melee attack
	if (enemy_range == RANGE_MELEE)
	{
		if (self->monsterinfo.melee)
			self->monsterinfo.attack_state = AS_MELEE;
		else
			self->monsterinfo.attack_state = AS_MISSILE;

		return true;
	}

	// missile attack
	if (!self->monsterinfo.attack)
		return false;

	if (level.time < self->monsterinfo.attack_finished)
		return false;

	if (enemy_range == RANGE_FAR)
		return false;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		chance = 0.4f;
	else if (enemy_range == RANGE_MELEE)
		chance = 0.8f;
	else if (enemy_range == RANGE_NEAR)
		chance = 0.4f;
	else if (enemy_range == RANGE_MID)
		chance = 0.2f;
	else
		return false;

	if (random() < chance)
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		self->monsterinfo.attack_finished = level.time + 2000 * random();
		return true;
	}

	if (self->flags & FL_FLY)
	{
		if (random() < 0.3f)
			self->monsterinfo.attack_state = AS_SLIDING;
		else
			self->monsterinfo.attack_state = AS_STRAIGHT;
	}

	return false;
}

static const mevent_t events[] =
{
	EVENT_FUNC(jorg_idle),
	EVENT_FUNC(jorg_step_left),
	EVENT_FUNC(jorg_step_right),
	EVENT_FUNC(jorg_run),
	EVENT_FUNC(MakronToss),
	EVENT_FUNC(BossExplode),
	EVENT_FUNC(jorg_attack1),
	EVENT_FUNC(jorg_reattack1),
	EVENT_FUNC(jorg_firebullet),
	EVENT_FUNC(jorgBFG),
	NULL
};

/*QUAKED monster_jorg (1 .5 0) (-80 -80 0) (90 90 140) Ambush Trigger_Spawn Sight
*/
void SP_monster_jorg(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/boss3/jorg/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/jorg.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_pain1 = gi.soundindex("boss3/bs3pain1.wav");
	sound_pain2 = gi.soundindex("boss3/bs3pain2.wav");
	sound_pain3 = gi.soundindex("boss3/bs3pain3.wav");
	sound_death = gi.soundindex("boss3/bs3deth1.wav");
	sound_attack1 = gi.soundindex("boss3/bs3atck1.wav");
	sound_attack2 = gi.soundindex("boss3/bs3atck2.wav");
	sound_search1 = gi.soundindex("boss3/bs3srch1.wav");
	sound_search2 = gi.soundindex("boss3/bs3srch2.wav");
	sound_search3 = gi.soundindex("boss3/bs3srch3.wav");
	sound_idle = gi.soundindex("boss3/bs3idle1.wav");
	sound_step_left = gi.soundindex("boss3/step1.wav");
	sound_step_right = gi.soundindex("boss3/step2.wav");
	sound_firegun = gi.soundindex("boss3/xfire.wav");
	sound_death_hit = gi.soundindex("boss3/d_hit.wav");
	MakronPrecache();
	self->movetype = MOVETYPE_STEP;
	self->server.solid = SOLID_BBOX;
	self->server.state.modelindex = gi.modelindex("models/monsters/boss3/rider/tris.md2");
	self->server.state.modelindex2 = gi.modelindex(model_name);
	VectorSet(self->server.mins, -80, -80, 0);
	VectorSet(self->server.maxs, 80, 80, 140);
	self->health = 3000;
	self->gib_health = -2000;
	self->mass = 1000;
	self->pain = jorg_pain;
	self->die = jorg_die;
	self->monsterinfo.stand = jorg_stand;
	self->monsterinfo.walk = jorg_walk;
	self->monsterinfo.run = jorg_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = jorg_attack;
	self->monsterinfo.search = jorg_search;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = NULL;
	self->monsterinfo.checkattack = Jorg_CheckAttack;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
}

#endif
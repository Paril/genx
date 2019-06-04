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

Makron -- Final Boss

==============================================================================
*/

#include "g_local.h"
#include "m_boss32.h"
#include "m_local.h"

static q_soundhandle sound_pain4;
static q_soundhandle sound_pain5;
static q_soundhandle sound_pain6;
static q_soundhandle sound_death;
static q_soundhandle sound_step_left;
static q_soundhandle sound_step_right;
static q_soundhandle sound_attack_bfg;
static q_soundhandle sound_brainsplorch;
static q_soundhandle sound_prerailgun;
static q_soundhandle sound_popup;
static q_soundhandle sound_taunt1;
static q_soundhandle sound_taunt2;
static q_soundhandle sound_taunt3;
static q_soundhandle sound_hit;

static mscript_t script;

static void makron_taunt(edict_t *self)
{
	float r;
	r = random();

	if (r <= 0.3f)
		gi.sound(self, CHAN_AUTO, sound_taunt1, 1, ATTN_NONE, 0);
	else if (r <= 0.6f)
		gi.sound(self, CHAN_AUTO, sound_taunt2, 1, ATTN_NONE, 0);
	else
		gi.sound(self, CHAN_AUTO, sound_taunt3, 1, ATTN_NONE, 0);
}

static void makron_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void makron_hit(edict_t *self)
{
	gi.sound(self, CHAN_AUTO, sound_hit, 1, ATTN_NONE, 0);
}

static void makron_popup(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_popup, 1, ATTN_NONE, 0);
}

static void makron_step_left(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step_left, 1, ATTN_NORM, 0);
}

static void makron_step_right(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step_right, 1, ATTN_NORM, 0);
}

static void makron_brainsplorch(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_brainsplorch, 1, ATTN_NORM, 0);
}

static void makron_prerailgun(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_prerailgun, 1, ATTN_NORM, 0);
}

static void makron_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void makron_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
}

static void makronBFG(edict_t *self)
{
	vec3_t  forward, right;
	vec3_t  start;
	vec3_t  dir;
	vec3_t  vec;
	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[MZ2_MAKRON_BFG], forward, right, start);
	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);
	gi.sound(self, CHAN_VOICE, sound_attack_bfg, 1, ATTN_NORM, 0);
	monster_fire_bfg(self, start, dir, 50, 300, 100, 300, MZ2_MAKRON_BFG);
}

static void MakronSaveloc(edict_t *self)
{
	VectorCopy(self->enemy->s.origin, self->pos1);  //save for aiming the shot
	self->pos1[2] += self->enemy->viewheight;
}

// FIXME: He's not firing from the proper Z
static void MakronRailgun(edict_t *self)
{
	vec3_t  start;
	vec3_t  dir;
	vec3_t  forward, right;
	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[MZ2_MAKRON_RAILGUN_1], forward, right, start);
	// calc direction to where we targted
	VectorSubtract(self->pos1, start, dir);
	VectorNormalize(dir);
	monster_fire_railgun(self, start, dir, 50, 100, MZ2_MAKRON_RAILGUN_1);
}

// FIXME: This is all wrong. He's not firing at the proper angles.
static void MakronHyperblaster(edict_t *self)
{
	vec3_t  dir;
	vec3_t  vec;
	vec3_t  start;
	vec3_t  forward, right;
	int     flash_number;
	flash_number = MZ2_MAKRON_BLASTER_1 + (self->s.frame - FRAME_attak405);
	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);

	if (self->enemy)
	{
		VectorCopy(self->enemy->s.origin, vec);
		vec[2] += self->enemy->viewheight;
		VectorSubtract(vec, start, vec);
		vectoangles(vec, vec);
		dir[0] = vec[0];
	}
	else
		dir[0] = 0;

	if (self->s.frame <= FRAME_attak413)
		dir[1] = self->s.angles[1] - 10 * (self->s.frame - FRAME_attak413);
	else
		dir[1] = self->s.angles[1] + 10 * (self->s.frame - FRAME_attak421);

	dir[2] = 0;
	AngleVectors(dir, forward, NULL, NULL);
	monster_fire_blaster(self, start, forward, 15, 1000, MZ2_MAKRON_BLASTER_1, EF_BLASTER);
}

static void makron_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	// Lessen the chance of him going into his pain frames
	if (damage <= 25)
		if (random() < 0.2f)
			return;

	self->pain_debounce_time = level.time + 3000;

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	if (damage <= 40)
	{
		gi.sound(self, CHAN_VOICE, sound_pain4, 1, ATTN_NONE, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain4");
	}
	else if (damage <= 110)
	{
		gi.sound(self, CHAN_VOICE, sound_pain5, 1, ATTN_NONE, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain5");
	}
	else if (damage <= 150)
	{
		if (random() <= 0.45f)
		{
			gi.sound(self, CHAN_VOICE, sound_pain6, 1, ATTN_NONE, 0);
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain6");
		}
	}
	else
	{
		if (random() <= 0.35f)
		{
			gi.sound(self, CHAN_VOICE, sound_pain6, 1, ATTN_NONE, 0);
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain6");
		}
	}
}

static void makron_sight(edict_t *self, edict_t *other)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "sight");
}

static void makron_attack(edict_t *self)
{
	float   r;
	r = random();

	if (r <= 0.3f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack3");
	else if (r <= 0.6f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack4");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack5");
}

/*
---
Makron Torso. This needs to be spawned in
---
*/

static void makron_torso_think(edict_t *self)
{
	if (++self->s.frame < 365)
		self->nextthink = level.time + game.frametime;
	else
	{
		self->s.frame = 346;
		self->nextthink = level.time + game.frametime;
	}
}

static void makron_torso(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	VectorSet(ent->mins, -8, -8, 0);
	VectorSet(ent->maxs, 8, 8, 8);
	ent->s.frame = 346;
	ent->s.modelindex = gi.modelindex("models/monsters/boss3/rider/tris.md2");
	ent->think = makron_torso_think;
	ent->nextthink = level.time + (2 * game.frametime);
	ent->s.sound = gi.soundindex("makron/spine.wav");
	gi.linkentity(ent);
}


//
// death
//

static void makron_dead(edict_t *self)
{
	VectorSet(self->mins, -60, -60, 0);
	VectorSet(self->maxs, 60, 60, 72);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->s.clip_contents = CONTENTS_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

static void makron_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	edict_t *tempent;
	int     n;
	self->s.sound = 0;

	// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		for (n = 0; n < 1 /*4*/; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

		for (n = 0; n < 4; n++)
			ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);

		ThrowHead(self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	// regular death
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NONE, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	tempent = G_Spawn();
	VectorCopy(self->s.origin, tempent->s.origin);
	VectorCopy(self->s.angles, tempent->s.angles);
	tempent->s.origin[1] -= 84;
	makron_torso(tempent);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death2");
}

static bool Makron_CheckAttack(edict_t *self)
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
		VectorCopy(self->s.origin, spot1);
		spot1[2] += self->viewheight;
		VectorCopy(self->enemy->s.origin, spot2);
		spot2[2] += self->enemy->viewheight;
		tr = gi.trace(spot1, NULL, NULL, spot2, self, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_SLIME | CONTENTS_LAVA);

		// do we have a clear shot?
		if (tr.ent != self->enemy)
			return false;
	}

	enemy_range = range(self, self->enemy);
	VectorSubtract(self->enemy->s.origin, self->s.origin, temp);
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


//
// monster_makron
//

void MakronPrecache(void)
{
	sound_pain4 = gi.soundindex("makron/pain3.wav");
	sound_pain5 = gi.soundindex("makron/pain2.wav");
	sound_pain6 = gi.soundindex("makron/pain1.wav");
	sound_death = gi.soundindex("makron/death.wav");
	sound_step_left = gi.soundindex("makron/step1.wav");
	sound_step_right = gi.soundindex("makron/step2.wav");
	sound_attack_bfg = gi.soundindex("makron/bfg_fire.wav");
	sound_brainsplorch = gi.soundindex("makron/brain1.wav");
	sound_prerailgun = gi.soundindex("makron/rail_up.wav");
	sound_popup = gi.soundindex("makron/popup.wav");
	sound_taunt1 = gi.soundindex("makron/voice4.wav");
	sound_taunt2 = gi.soundindex("makron/voice3.wav");
	sound_taunt3 = gi.soundindex("makron/voice.wav");
	sound_hit = gi.soundindex("makron/bhit.wav");
	gi.modelindex("models/monsters/boss3/rider/tris.md2");
}

static const mevent_t events[] =
{
	EVENT_FUNC(makron_step_left),
	EVENT_FUNC(makron_step_right),
	EVENT_FUNC(makron_run),
	EVENT_FUNC(makron_popup),
	EVENT_FUNC(makron_taunt),
	EVENT_FUNC(makron_dead),
	EVENT_FUNC(makron_hit),
	EVENT_FUNC(makron_brainsplorch),
	EVENT_FUNC(makronBFG),
	EVENT_FUNC(MakronHyperblaster),
	EVENT_FUNC(makron_prerailgun),
	EVENT_FUNC(MakronSaveloc),
	EVENT_FUNC(MakronRailgun),
	NULL
};

/*QUAKED monster_makron (1 .5 0) (-30 -30 0) (30 30 90) Ambush Trigger_Spawn Sight
*/
void SP_monster_makron(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/boss3/rider/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/makron.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	MakronPrecache();
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -30, -30, 0);
	VectorSet(self->maxs, 30, 30, 90);
	self->health = 3000;
	self->gib_health = -2000;
	self->mass = 500;
	self->pain = makron_pain;
	self->die = makron_die;
	self->monsterinfo.stand = makron_stand;
	self->monsterinfo.walk = makron_walk;
	self->monsterinfo.run = makron_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = makron_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = makron_sight;
	self->monsterinfo.checkattack = Makron_CheckAttack;
	self->entitytype = ET_MONSTER_MAKRON;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "sight");
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
}

/*
=================
MakronSpawn

=================
*/
void MakronSpawn(edict_t *self)
{
	vec3_t      vec;
	edict_t     *player;
	SP_monster_makron(self);
	// jump at player
	player = level.sight_client;

	if (!player)
		return;

	VectorSubtract(player->s.origin, self->s.origin, vec);
	self->s.angles[YAW] = vectoyaw(vec);
	VectorNormalize(vec);
	VectorScale(vec, 400, self->velocity);
	self->velocity[2] = 200;
	self->groundentity = NULL;
}

/*
=================
MakronToss

Jorg is just about dead, so set up to launch Makron out
=================
*/
void MakronToss(edict_t *self)
{
	edict_t *ent;
	ent = G_Spawn();
	ent->nextthink = level.time + (game.frametime * 8);
	ent->think = MakronSpawn;
	ent->target = self->target;
	VectorCopy(self->s.origin, ent->s.origin);
}

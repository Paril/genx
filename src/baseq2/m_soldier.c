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

SOLDIER

==============================================================================
*/
#ifdef ENABLE_COOP

#include "g_local.h"
#include "m_soldier.h"
#include "m_local.h"

static q_soundhandle sound_idle;
static q_soundhandle sound_sight1;
static q_soundhandle sound_sight2;
static q_soundhandle sound_pain_light;
static q_soundhandle sound_pain;
static q_soundhandle sound_pain_ss;
static q_soundhandle sound_death_light;
static q_soundhandle sound_death;
static q_soundhandle sound_death_ss;
static q_soundhandle sound_cock;

static mscript_t script;

static void soldier_idle(edict_t *self)
{
	if (random() > 0.8f)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void soldier_cock(edict_t *self)
{
	if (self->server.state.frame == FRAME_stand322)
		gi.sound(self, CHAN_WEAPON, sound_cock, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_WEAPON, sound_cock, 1, ATTN_NORM, 0);
}

//
// STAND
//

static void soldier_stand(edict_t *self)
{
	if ((self->monsterinfo.currentmove == M_GetMonsterMove(&script, "stand3")) || (random() < 0.8f))
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand3");
}

//
// WALK
//

static void soldier_walk1_random(edict_t *self)
{
	if (random() > 0.1f)
		self->monsterinfo.nextframe = FRAME_walk101;
}

static void soldier_walk(edict_t *self)
{
	if (random() < 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk1");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk2");
}

//
// RUN
//

static void soldier_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
		return;
	}

	if (self->monsterinfo.currentmove == M_GetMonsterMove(&script, "walk1") ||
		self->monsterinfo.currentmove == M_GetMonsterMove(&script, "walk2") ||
		self->monsterinfo.currentmove == M_GetMonsterMove(&script, "start_run"))
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "start_run");
}

//
// PAIN
//

static void soldier_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	float   r;
	int     n;

	if (self->health < (self->max_health / 2))
		self->server.state.skinnum |= 1;

	if (level.time < self->pain_debounce_time)
	{
		if ((self->velocity[2] > 100) && ((self->monsterinfo.currentmove == M_GetMonsterMove(&script, "pain1")) ||
											(self->monsterinfo.currentmove == M_GetMonsterMove(&script, "pain2")) ||
											(self->monsterinfo.currentmove == M_GetMonsterMove(&script, "pain3"))))
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain4");

		return;
	}

	self->pain_debounce_time = level.time + 3000;
	n = self->server.state.skinnum | 1;

	if (n == 1)
		gi.sound(self, CHAN_VOICE, sound_pain_light, 1, ATTN_NORM, 0);
	else if (n == 3)
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain_ss, 1, ATTN_NORM, 0);

	if (self->velocity[2] > 100)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain4");
		return;
	}

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	r = random();

	if (r < 0.33f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
	else if (r < 0.66f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain2");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain3");
}

//
// ATTACK
//

static int blaster_flash [] = {MZ2_SOLDIER_BLASTER_1, MZ2_SOLDIER_BLASTER_2, MZ2_SOLDIER_BLASTER_3, MZ2_SOLDIER_BLASTER_4, MZ2_SOLDIER_BLASTER_5, MZ2_SOLDIER_BLASTER_6, MZ2_SOLDIER_BLASTER_7, MZ2_SOLDIER_BLASTER_8};
static int shotgun_flash [] = {MZ2_SOLDIER_SHOTGUN_1, MZ2_SOLDIER_SHOTGUN_2, MZ2_SOLDIER_SHOTGUN_3, MZ2_SOLDIER_SHOTGUN_4, MZ2_SOLDIER_SHOTGUN_5, MZ2_SOLDIER_SHOTGUN_6, MZ2_SOLDIER_SHOTGUN_7, MZ2_SOLDIER_SHOTGUN_8};
static int machinegun_flash [] = {MZ2_SOLDIER_MACHINEGUN_1, MZ2_SOLDIER_MACHINEGUN_2, MZ2_SOLDIER_MACHINEGUN_3, MZ2_SOLDIER_MACHINEGUN_4, MZ2_SOLDIER_MACHINEGUN_5, MZ2_SOLDIER_MACHINEGUN_6, MZ2_SOLDIER_MACHINEGUN_7, MZ2_SOLDIER_MACHINEGUN_8};

static void soldier_fire(edict_t *self, int flash_number)
{
	vec3_t  start;
	vec3_t  forward, right, up;
	vec3_t  aim;
	vec3_t  dir;
	vec3_t  end;
	float   r, u;
	int     flash_index;

	if (self->server.state.skinnum < 2)
		flash_index = blaster_flash[flash_number];
	else if (self->server.state.skinnum < 4)
		flash_index = shotgun_flash[flash_number];
	else
		flash_index = machinegun_flash[flash_number];

	AngleVectors(self->server.state.angles, forward, right, NULL);
	G_ProjectSource(self->server.state.origin, monster_flash_offset[flash_index], forward, right, start);

	if (flash_number == 5 || flash_number == 6)
		VectorCopy(forward, aim);
	else
	{
		VectorCopy(self->enemy->server.state.origin, end);
		end[2] += self->enemy->viewheight;
		VectorSubtract(end, start, aim);
		vectoangles(aim, dir);
		AngleVectors(dir, forward, right, up);
		r = crandom() * 1000;
		u = crandom() * 500;
		VectorMA(start, 8192, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);
		VectorSubtract(end, start, aim);
		VectorNormalize(aim);
	}

	if (self->server.state.skinnum <= 1)
		monster_fire_blaster(self, start, aim, 5, 600, flash_index, EF_BLASTER);
	else if (self->server.state.skinnum <= 3)
		monster_fire_shotgun(self, start, aim, 2, 1, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SHOTGUN_COUNT, flash_index);
	else
	{
		if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			self->monsterinfo.pausetime = level.time + (3 + Q_rand() % 8) * 100;

		monster_fire_bullet(self, start, aim, 2, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_index);

		if (level.time >= self->monsterinfo.pausetime)
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		else
			self->monsterinfo.aiflags |= AI_HOLD_FRAME;
	}
}

// ATTACK1 (blaster/shotgun)

static void soldier_fire1(edict_t *self)
{
	soldier_fire(self, 0);
}

static void soldier_attack1_refire1(edict_t *self)
{
	if (self->server.state.skinnum > 1)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5f)) || (range(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attak102;
	else
		self->monsterinfo.nextframe = FRAME_attak110;
}

static void soldier_attack1_refire2(edict_t *self)
{
	if (self->server.state.skinnum < 2)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5f)) || (range(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attak102;
}

// ATTACK2 (blaster/shotgun)

static void soldier_fire2(edict_t *self)
{
	soldier_fire(self, 1);
}

static void soldier_attack2_refire1(edict_t *self)
{
	if (self->server.state.skinnum > 1)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5f)) || (range(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attak204;
	else
		self->monsterinfo.nextframe = FRAME_attak216;
}

static void soldier_attack2_refire2(edict_t *self)
{
	if (self->server.state.skinnum < 2)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5f)) || (range(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attak204;
}

// ATTACK3 (duck and shoot)

static void soldier_duck_down(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	self->server.maxs[2] -= 32;
	self->takedamage = true;
	self->monsterinfo.pausetime = level.time + 100;
	gi.linkentity(self);
}

static void soldier_duck_up(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->server.maxs[2] += 32;
	self->takedamage = true;
	gi.linkentity(self);
}

static void soldier_fire3(edict_t *self)
{
	soldier_duck_down(self);
	soldier_fire(self, 2);
}

static void soldier_attack3_refire(edict_t *self)
{
	if ((level.time + 400) < self->monsterinfo.pausetime)
		self->monsterinfo.nextframe = FRAME_attak303;
}

// ATTACK4 (machinegun)

static void soldier_fire4(edict_t *self)
{
	soldier_fire(self, 3);
}

// ATTACK6 (run & shoot)

static void soldier_fire8(edict_t *self)
{
	soldier_fire(self, 7);
}

static void soldier_attack6_refire(edict_t *self)
{
	if (self->enemy->health <= 0)
		return;

	if (range(self, self->enemy) < RANGE_MID)
		return;

	if (skill->value == 3)
		self->monsterinfo.nextframe = FRAME_runs03;
}

static void soldier_attack(edict_t *self)
{
	if (self->server.state.skinnum < 4)
	{
		if (random() < 0.5f)
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
		else
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack2");
	}
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack4");
}

//
// SIGHT
//

static void soldier_sight(edict_t *self, edict_t *other)
{
	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_sight2, 1, ATTN_NORM, 0);

	if ((skill->value > 0) && (range(self, self->enemy) >= RANGE_MID) && random() > 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack6");
}

//
// DUCK
//

static void soldier_duck_hold(edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void soldier_dodge(edict_t *self, edict_t *attacker, float eta)
{
	float   r;
	r = random();

	if (r > 0.25f)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	if (skill->value == 0)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "duck");
		return;
	}

	self->monsterinfo.pausetime = level.time + (eta * 1000) + 300;
	r = random();

	if (skill->value == 1)
	{
		if (r > 0.33f)
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "duck");
		else
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack3");

		return;
	}

	if (skill->value >= 2)
	{
		if (r > 0.66f)
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "duck");
		else
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack3");

		return;
	}

	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack3");
}

//
// DEATH
//

static void soldier_fire6(edict_t *self)
{
	soldier_fire(self, 5);
}

static void soldier_fire7(edict_t *self)
{
	soldier_fire(self, 6);
}

static void soldier_dead(edict_t *self)
{
	VectorSet(self->server.mins, -16, -16, -24);
	VectorSet(self->server.maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->server.flags.deadmonster = true;
	self->server.state.clip_contents = CONTENTS_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

static void soldier_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int     n;

	// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		for (n = 0; n < 3; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

		ThrowGib(self, "models/objects/gibs/chest/tris.md2", damage, GIB_ORGANIC);
		ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	// regular death
	self->deadflag = DEAD_DEAD;
	self->takedamage = true;
	self->server.state.skinnum |= 1;

	if (self->server.state.skinnum == 1)
		gi.sound(self, CHAN_VOICE, sound_death_light, 1, ATTN_NORM, 0);
	else if (self->server.state.skinnum == 3)
		gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	else // (self->server.state.skinnum == 5)
		gi.sound(self, CHAN_VOICE, sound_death_ss, 1, ATTN_NORM, 0);

	if (fabsf((self->server.state.origin[2] + self->viewheight) - point[2]) <= 4)
	{
		// head shot
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death3");
		return;
	}

	n = Q_rand() % 5;

	if (n == 0)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death1");
	else if (n == 1)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death2");
	else if (n == 2)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death4");
	else if (n == 3)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death5");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death6");
}

//
// SPAWN
//

static const mevent_t events[] =
{
	EVENT_FUNC(soldier_stand),
	EVENT_FUNC(soldier_idle),
	EVENT_FUNC(soldier_cock),
	EVENT_FUNC(soldier_walk1_random),
	EVENT_FUNC(soldier_run),
	EVENT_FUNC(soldier_fire1),
	EVENT_FUNC(soldier_attack1_refire1),
	EVENT_FUNC(soldier_attack1_refire2),
	EVENT_FUNC(soldier_fire2),
	EVENT_FUNC(soldier_attack2_refire1),
	EVENT_FUNC(soldier_attack2_refire2),
	EVENT_FUNC(soldier_fire3),
	EVENT_FUNC(soldier_attack3_refire),
	EVENT_FUNC(soldier_duck_up),
	EVENT_FUNC(soldier_fire4),
	EVENT_FUNC(soldier_fire8),
	EVENT_FUNC(soldier_attack6_refire),
	EVENT_FUNC(soldier_duck_down),
	EVENT_FUNC(soldier_duck_hold),
	EVENT_FUNC(soldier_dead),
	EVENT_FUNC(soldier_fire6),
	EVENT_FUNC(soldier_fire7),
	NULL
};

static void SP_monster_soldier_x(edict_t *self)
{
	const char *model_name = "models/monsters/soldier/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/soldier.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	self->server.state.modelindex = gi.modelindex(model_name);
	self->monsterinfo.scale = MODEL_SCALE;
	VectorSet(self->server.mins, -16, -16, -24);
	VectorSet(self->server.maxs, 16, 16, 32);
	self->movetype = MOVETYPE_STEP;
	self->server.solid = SOLID_BBOX;
	sound_idle =    gi.soundindex("soldier/solidle1.wav");
	sound_sight1 =  gi.soundindex("soldier/solsght1.wav");
	sound_sight2 =  gi.soundindex("soldier/solsrch1.wav");
	sound_cock =    gi.soundindex("infantry/infatck3.wav");
	self->mass = 100;
	self->pain = soldier_pain;
	self->die = soldier_die;
	self->monsterinfo.stand = soldier_stand;
	self->monsterinfo.walk = soldier_walk;
	self->monsterinfo.run = soldier_run;
	self->monsterinfo.dodge = soldier_dodge;
	self->monsterinfo.attack = soldier_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = soldier_sight;
	gi.linkentity(self);
	self->monsterinfo.stand(self);
	walkmonster_start(self);
}


/*QUAKED monster_soldier_light (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_soldier_light(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	sound_pain_light = gi.soundindex("soldier/solpain2.wav");
	sound_death_light = gi.soundindex("soldier/soldeth2.wav");
	gi.modelindex("models/objects/laser/tris.md2");
	gi.soundindex("misc/lasfly.wav");
	gi.soundindex("soldier/solatck2.wav");
	self->server.state.skinnum = 0;
	self->health = 20;
	self->gib_health = -30;
	SP_monster_soldier_x(self);
}

/*QUAKED monster_soldier (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_soldier(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	sound_pain = gi.soundindex("soldier/solpain1.wav");
	sound_death = gi.soundindex("soldier/soldeth1.wav");
	gi.soundindex("soldier/solatck1.wav");
	self->server.state.skinnum = 2;
	self->health = 30;
	self->gib_health = -30;
	SP_monster_soldier_x(self);
}

/*QUAKED monster_soldier_ss (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_soldier_ss(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	sound_pain_ss = gi.soundindex("soldier/solpain3.wav");
	sound_death_ss = gi.soundindex("soldier/soldeth3.wav");
	gi.soundindex("soldier/solatck3.wav");
	self->server.state.skinnum = 4;
	self->health = 40;
	self->gib_health = -30;
	SP_monster_soldier_x(self);
}

#endif
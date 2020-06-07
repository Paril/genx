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

TANK

==============================================================================
*/
#ifdef ENABLE_COOP

#include "g_local.h"
#include "m_tank.h"
#include "m_local.h"

static q_soundhandle sound_thud;
static q_soundhandle sound_pain;
static q_soundhandle sound_idle;
static q_soundhandle sound_die;
static q_soundhandle sound_step;
static q_soundhandle sound_sight;
static q_soundhandle sound_windup;
static q_soundhandle sound_strike;

static mscript_t script;

static void tank_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void tank_footstep(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step, 1, ATTN_NORM, 0);
}

static void tank_thud(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_thud, 1, ATTN_NORM, 0);
}

static void tank_windup(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_windup, 1, ATTN_NORM, 0);
}

static void tank_idle(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void tank_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void tank_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void tank_run(edict_t *self)
{
	if (self->enemy && self->enemy->client)
		self->monsterinfo.aiflags |= AI_BRUTAL;
	else
		self->monsterinfo.aiflags &= ~AI_BRUTAL;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
		return;
	}

	if (self->monsterinfo.currentmove == M_GetMonsterMove(&script, "walk") ||
		self->monsterinfo.currentmove == M_GetMonsterMove(&script, "start_run"))
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "start_run");
}

static void tank_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (damage <= 10)
		return;

	if (level.time < self->pain_debounce_time)
		return;

	if (damage <= 30)
		if (random() > 0.2f)
			return;

	// If hard or nightmare, don't go into pain while attacking
	if (skill->value >= 2)
	{
		if ((self->s.frame >= FRAME_attak301) && (self->s.frame <= FRAME_attak330))
			return;

		if ((self->s.frame >= FRAME_attak101) && (self->s.frame <= FRAME_attak116))
			return;
	}

	self->pain_debounce_time = level.time + 3000;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	if (damage <= 30)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
	else if (damage <= 60)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain2");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain3");
}

static void TankBlaster(edict_t *self)
{
	vec3_t  forward, right;
	vec3_t  start;
	vec3_t  end;
	vec3_t  dir;
	int     flash_number;

	if (self->s.frame == FRAME_attak110)
		flash_number = MZ2_TANK_BLASTER_1;
	else if (self->s.frame == FRAME_attak113)
		flash_number = MZ2_TANK_BLASTER_2;
	else // (self->s.frame == FRAME_attak116)
		flash_number = MZ2_TANK_BLASTER_3;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);
	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, dir);
	monster_fire_blaster(self, start, dir, 30, 800, flash_number, EF_BLASTER);
}

static void TankStrike(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_strike, 1, ATTN_NORM, 0);
}

static void TankRocket(edict_t *self)
{
	vec3_t  forward, right;
	vec3_t  start;
	vec3_t  dir;
	vec3_t  vec;
	int     flash_number;

	if (self->s.frame == FRAME_attak324)
		flash_number = MZ2_TANK_ROCKET_1;
	else if (self->s.frame == FRAME_attak327)
		flash_number = MZ2_TANK_ROCKET_2;
	else // (self->s.frame == FRAME_attak330)
		flash_number = MZ2_TANK_ROCKET_3;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);
	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);
	monster_fire_rocket(self, start, dir, 50, 550, flash_number);
}

static void TankMachineGun(edict_t *self)
{
	vec3_t  dir;
	vec3_t  vec;
	vec3_t  start;
	vec3_t  forward, right;
	int     flash_number;
	flash_number = MZ2_TANK_MACHINEGUN_1 + (self->s.frame - FRAME_attak406);
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

	if (self->s.frame <= FRAME_attak415)
		dir[1] = self->s.angles[1] - 8 * (self->s.frame - FRAME_attak411);
	else
		dir[1] = self->s.angles[1] + 8 * (self->s.frame - FRAME_attak419);

	dir[2] = 0;
	AngleVectors(dir, forward, NULL, NULL);
	monster_fire_bullet(self, start, forward, 20, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

static void tank_reattack_blaster(edict_t *self)
{
	if (skill->value >= 2 && visible(self, self->enemy) &&
		self->enemy->health > 0 && random() <= 0.6f)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "reattack_blast");
		return;
	}

	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_post_blast");
}

static void tank_poststrike(edict_t *self)
{
	self->enemy = NULL;
	tank_run(self);
}

static void tank_refire_rocket(edict_t *self)
{
	// Only on hard or nightmare
	if (skill->value >= 2 && self->enemy->health > 0 &&
		visible(self, self->enemy) && random() <= 0.4f)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_fire_rocket");
		return;
	}

	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_post_rocket");
}

static void tank_doattack_rocket(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_fire_rocket");
}

static void tank_attack(edict_t *self)
{
	vec3_t  vec;
	float   range;
	float   r;

	if (self->enemy->health < 0)
	{
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_strike");
		self->monsterinfo.aiflags &= ~AI_BRUTAL;
		return;
	}

	VectorSubtract(self->enemy->s.origin, self->s.origin, vec);
	range = VectorLength(vec);
	r = random();

	if (range <= 125)
	{
		if (r < 0.4f)
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_chain");
		else
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_blast");
	}
	else if (range <= 250)
	{
		if (r < 0.5f)
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_chain");
		else
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_blast");
	}
	else
	{
		if (r < 0.33f)
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_chain");
		else if (r < 0.66f)
		{
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_pre_rocket");
			self->pain_debounce_time = level.time + 5000;    // no pain for a while
		}
		else
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack_blast");
	}
}

static void tank_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -16);
	VectorSet(self->maxs, 16, 16, -0);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->s.clip_contents = CONTENTS_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

static void tank_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int     n;

	// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		for (n = 0; n < 1 /*4*/; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

		for (n = 0; n < 4; n++)
			ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);

		ThrowGib(self, "models/objects/gibs/chest/tris.md2", damage, GIB_ORGANIC);
		ThrowHead(self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);
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
	EVENT_FUNC(tank_footstep),
	EVENT_FUNC(tank_run),
	EVENT_FUNC(tank_reattack_blaster),
	EVENT_FUNC(TankBlaster),
	EVENT_FUNC(tank_poststrike),
	EVENT_FUNC(tank_windup),
	EVENT_FUNC(TankStrike),
	EVENT_FUNC(tank_doattack_rocket),
	EVENT_FUNC(tank_refire_rocket),
	EVENT_FUNC(TankRocket),
	EVENT_FUNC(TankMachineGun),
	EVENT_FUNC(tank_dead),
	EVENT_FUNC(tank_thud),
	NULL
};

/*QUAKED monster_tank (1 .5 0) (-32 -32 -16) (32 32 72) Ambush Trigger_Spawn Sight
*/
/*QUAKED monster_tank_commander (1 .5 0) (-32 -32 -16) (32 32 72) Ambush Trigger_Spawn Sight
*/
void SP_monster_tank(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/tank/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/tank.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -32, -32, -16);
	VectorSet(self->maxs, 32, 32, 72);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	sound_pain = gi.soundindex("tank/tnkpain2.wav");
	sound_thud = gi.soundindex("tank/tnkdeth2.wav");
	sound_idle = gi.soundindex("tank/tnkidle1.wav");
	sound_die = gi.soundindex("tank/death.wav");
	sound_step = gi.soundindex("tank/step.wav");
	sound_windup = gi.soundindex("tank/tnkatck4.wav");
	sound_strike = gi.soundindex("tank/tnkatck5.wav");
	sound_sight = gi.soundindex("tank/sight1.wav");
	gi.soundindex("tank/tnkatck1.wav");
	gi.soundindex("tank/tnkatk2a.wav");
	gi.soundindex("tank/tnkatk2b.wav");
	gi.soundindex("tank/tnkatk2c.wav");
	gi.soundindex("tank/tnkatk2d.wav");
	gi.soundindex("tank/tnkatk2e.wav");
	gi.soundindex("tank/tnkatck3.wav");

	if (self->entitytype == ET_MONSTER_TANK_COMMANDER)
	{
		self->health = 1000;
		self->gib_health = -225;
	}
	else
	{
		self->health = 750;
		self->gib_health = -200;
	}

	self->mass = 500;
	self->pain = tank_pain;
	self->die = tank_die;
	self->monsterinfo.stand = tank_stand;
	self->monsterinfo.walk = tank_walk;
	self->monsterinfo.run = tank_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = tank_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = tank_sight;
	self->monsterinfo.idle = tank_idle;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);

	if (self->entitytype == ET_MONSTER_TANK_COMMANDER)
		self->s.skinnum = 2;
}

#endif
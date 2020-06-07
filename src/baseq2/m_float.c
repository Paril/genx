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

floater

==============================================================================
*/
#ifdef ENABLE_COOP
#include "g_local.h"
#include "m_float.h"
#include "m_local.h"

static q_soundhandle sound_attack2;
static q_soundhandle sound_attack3;
static q_soundhandle sound_death1;
static q_soundhandle sound_idle;
static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_sight;

static mscript_t script;

static void floater_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void floater_idle(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void floater_fire_blaster(edict_t *self)
{
	vec3_t  start;
	vec3_t  forward, right;
	vec3_t  end;
	vec3_t  dir;
	int     effect;

	if ((self->s.frame == FRAME_attak104) || (self->s.frame == FRAME_attak107))
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[MZ2_FLOAT_BLASTER_1], forward, right, start);
	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, dir);
	monster_fire_blaster(self, start, dir, 1, 1000, MZ2_FLOAT_BLASTER_1, effect);
}

static void floater_stand(edict_t *self)
{
	if (random() <= 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand2");
}

static void floater_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
}

static void floater_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk");
}

static void floater_wham(edict_t *self)
{
	static  vec3_t  aim = {MELEE_DISTANCE, 0, 0};
	gi.sound(self, CHAN_WEAPON, sound_attack3, 1, ATTN_NORM, 0);
	fire_hit(self, aim, 5 + Q_rand() % 6, -50);
}

static void floater_zap(edict_t *self)
{
	vec3_t  forward, right;
	vec3_t  origin;
	vec3_t  dir;
	vec3_t  offset;
	VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
	AngleVectors(self->s.angles, forward, right, NULL);
	//FIXME use a flash and replace these two lines with the commented one
	VectorSet(offset, 18.5f, -0.9f, 10);
	G_ProjectSource(self->s.origin, offset, forward, right, origin);
	//  G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, origin);
	gi.sound(self, CHAN_WEAPON, sound_attack2, 1, ATTN_NORM, 0);
	//FIXME use the flash, Luke
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_SPLASH);
	MSG_WriteByte(32);
	MSG_WritePos(origin);
	MSG_WriteDir(dir);
	MSG_WriteByte(1);    //sparks
	gi.multicast(origin, MULTICAST_PVS);
	T_Damage(self->enemy, self, self, dir, self->enemy->s.origin, vec3_origin, 5 + Q_rand() % 6, -10, DAMAGE_ENERGY, MakeAttackerMeansOfDeath(self, self, MD_MELEE, DT_DIRECT));
}

static void floater_attack(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
}

static void floater_melee(edict_t *self)
{
	if (random() < 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack3");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack2");
}

static void floater_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	int     n;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3000;

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	n = (Q_rand() + 1) % 3;

	if (n == 0)
	{
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain2");
	}
}

static void floater_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->s.clip_contents = CONTENTS_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

static void floater_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
	BecomeExplosion1(self);
}

static const mevent_t events[] =
{
	EVENT_FUNC(floater_run),
	EVENT_FUNC(floater_fire_blaster),
	EVENT_FUNC(floater_wham),
	EVENT_FUNC(floater_zap),
	NULL
};

/*QUAKED monster_floater (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_floater(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/float/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/float.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_attack2 = gi.soundindex("floater/fltatck2.wav");
	sound_attack3 = gi.soundindex("floater/fltatck3.wav");
	sound_death1 = gi.soundindex("floater/fltdeth1.wav");
	sound_idle = gi.soundindex("floater/fltidle1.wav");
	sound_pain1 = gi.soundindex("floater/fltpain1.wav");
	sound_pain2 = gi.soundindex("floater/fltpain2.wav");
	sound_sight = gi.soundindex("floater/fltsght1.wav");
	gi.soundindex("floater/fltatck1.wav");
	self->s.sound = gi.soundindex("floater/fltsrch1.wav");
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -24, -24, -24);
	VectorSet(self->maxs, 24, 24, 32);
	self->health = 200;
	self->gib_health = -80;
	self->mass = 300;
	self->pain = floater_pain;
	self->die = floater_die;
	self->monsterinfo.stand = floater_stand;
	self->monsterinfo.walk = floater_walk;
	self->monsterinfo.run = floater_run;
	//  self->monsterinfo.dodge = floater_dodge;
	self->monsterinfo.attack = floater_attack;
	self->monsterinfo.melee = floater_melee;
	self->monsterinfo.sight = floater_sight;
	self->monsterinfo.idle = floater_idle;
	gi.linkentity(self);

	if (random() <= 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand2");

	self->monsterinfo.scale = MODEL_SCALE;
	flymonster_start(self);
}

#endif
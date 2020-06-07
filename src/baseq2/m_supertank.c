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

SUPERTANK

==============================================================================
*/

#ifdef ENABLE_COOP
#include "g_local.h"
#include "m_supertank.h"
#include "m_local.h"

static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_pain3;
static q_soundhandle sound_death;
static q_soundhandle sound_search1;
static q_soundhandle sound_search2;
static q_soundhandle tread_sound;

static mscript_t script;

static void TreadSound(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, tread_sound, 1, ATTN_NORM, 0);
}

static void supertank_search(edict_t *self)
{
	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_search1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_search2, 1, ATTN_NORM, 0);
}

static void supertank_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
}

static void supertank_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "forward");
}

static void supertank_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run");
}

static void supertank_reattack1(edict_t *self)
{
	if (visible(self, self->enemy))
	{
		if (random() < 0.9f)
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
		else
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "end_attack1");
	}
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "end_attack1");
}

static void supertank_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	// Lessen the chance of him going into his pain frames
	if (damage <= 25)
		if (random() < 0.2f)
			return;

	// Don't go into pain if he's firing his rockets
	if (skill->value >= 2)
		if ((self->s.frame >= FRAME_attak2_1) && (self->s.frame <= FRAME_attak2_14))
			return;

	self->pain_debounce_time = level.time + 3000;

	if (skill->value == 3)
		return;     // no pain anims in nightmare

	if (damage <= 10)
	{
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
	}
	else if (damage <= 25)
	{
		gi.sound(self, CHAN_VOICE, sound_pain3, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain2");
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain3");
	}
}

static void supertankRocket(edict_t *self)
{
	vec3_t  forward, right;
	vec3_t  start;
	vec3_t  dir;
	vec3_t  vec;
	int     flash_number;

	if (self->s.frame == FRAME_attak2_8)
		flash_number = MZ2_SUPERTANK_ROCKET_1;
	else if (self->s.frame == FRAME_attak2_11)
		flash_number = MZ2_SUPERTANK_ROCKET_2;
	else // (self->s.frame == FRAME_attak2_14)
		flash_number = MZ2_SUPERTANK_ROCKET_3;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);
	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);
	monster_fire_rocket(self, start, dir, 50, 500, flash_number);
}

static void supertankMachineGun(edict_t *self)
{
	vec3_t  dir;
	vec3_t  vec;
	vec3_t  start;
	vec3_t  forward, right;
	int     flash_number;
	flash_number = MZ2_SUPERTANK_MACHINEGUN_1 + (self->s.frame - FRAME_attak1_1);
	//FIXME!!!
	dir[0] = 0;
	dir[1] = self->s.angles[1];
	dir[2] = 0;
	AngleVectors(dir, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);

	if (self->enemy)
	{
		VectorCopy(self->enemy->s.origin, vec);
		VectorMA(vec, 0, self->enemy->velocity, vec);
		vec[2] += self->enemy->viewheight;
		VectorSubtract(vec, start, forward);
		VectorNormalize(forward);
	}

	monster_fire_bullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

static void supertank_attack(edict_t *self)
{
	vec3_t  vec;
	float   range;
	VectorSubtract(self->enemy->s.origin, self->s.origin, vec);
	range = VectorLength(vec);

	if (range <= 160)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
	else
	{
		// fire rockets more often at distance
		if (random() < 0.3f)
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
		else
			self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack2");
	}
}

static void supertank_dead(edict_t *self)
{
	VectorSet(self->mins, -60, -60, 0);
	VectorSet(self->maxs, 60, 60, 72);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->s.clip_contents = CONTENTS_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

void BossExplode(edict_t *self)
{
	vec3_t  org;
	int     n;
	self->think = BossExplode;
	VectorCopy(self->s.origin, org);
	org[2] += 24 + (Q_rand() & 15);

	switch (self->count++)
	{
		case 0:
			org[0] -= 24;
			org[1] -= 24;
			break;

		case 1:
			org[0] += 24;
			org[1] += 24;
			break;

		case 2:
			org[0] += 24;
			org[1] -= 24;
			break;

		case 3:
			org[0] -= 24;
			org[1] += 24;
			break;

		case 4:
			org[0] -= 48;
			org[1] -= 48;
			break;

		case 5:
			org[0] += 48;
			org[1] += 48;
			break;

		case 6:
			org[0] -= 48;
			org[1] += 48;
			break;

		case 7:
			org[0] += 48;
			org[1] -= 48;
			break;

		case 8:
			self->s.sound = 0;

			for (n = 0; n < 4; n++)
				ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", 500, GIB_ORGANIC);

			for (n = 0; n < 8; n++)
				ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", 500, GIB_METALLIC);

			ThrowGib(self, "models/objects/gibs/chest/tris.md2", 500, GIB_ORGANIC);
			ThrowHead(self, "models/objects/gibs/gear/tris.md2", 500, GIB_METALLIC);
			self->deadflag = DEAD_DEAD;
			return;
	}

	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_EXPLOSION1);
	MSG_WritePos(org);
	gi.multicast(self->s.origin, MULTICAST_PVS);
	self->nextthink = level.time + game.frametime;
}

static void supertank_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = false;
	self->count = 0;
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death");
}

static const mevent_t events[] =
{
	EVENT_FUNC(TreadSound),
	EVENT_FUNC(supertank_run),
	EVENT_FUNC(supertank_dead),
	EVENT_FUNC(BossExplode),
	EVENT_FUNC(supertankRocket),
	EVENT_FUNC(supertank_reattack1),
	EVENT_FUNC(supertankMachineGun),
	NULL
};

/*QUAKED monster_supertank (1 .5 0) (-64 -64 0) (64 64 72) Ambush Trigger_Spawn Sight
*/
void SP_monster_supertank(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/monsters/boss1/tris.md2";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q2/boss1.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_pain1 = gi.soundindex("bosstank/btkpain1.wav");
	sound_pain2 = gi.soundindex("bosstank/btkpain2.wav");
	sound_pain3 = gi.soundindex("bosstank/btkpain3.wav");
	sound_death = gi.soundindex("bosstank/btkdeth1.wav");
	sound_search1 = gi.soundindex("bosstank/btkunqv1.wav");
	sound_search2 = gi.soundindex("bosstank/btkunqv2.wav");
	tread_sound = gi.soundindex("bosstank/btkengn1.wav");
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -64, -64, 0);
	VectorSet(self->maxs, 64, 64, 112);
	self->health = 1500;
	self->gib_health = -500;
	self->mass = 800;
	self->pain = supertank_pain;
	self->die = supertank_die;
	self->monsterinfo.stand = supertank_stand;
	self->monsterinfo.walk = supertank_walk;
	self->monsterinfo.run = supertank_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = supertank_attack;
	self->monsterinfo.search = supertank_search;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = NULL;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand");
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
}

#endif
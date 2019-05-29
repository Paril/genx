/*
==============================================================================

KNIGHT

==============================================================================
*/

#include "g_local.h"

enum
{
	stand1, stand2, stand3, stand4, stand5, stand6, stand7, stand8, stand9,

	runb1, runb2, runb3, runb4, runb5, runb6, runb7, runb8,

	//runc1, runc2, runc3, runc4, runc5, runc6,

	runattack1, runattack2, runattack3, runattack4, runattack5,
	runattack6, runattack7, runattack8, runattack9, runattack10,
	runattack11,

	pain1, pain2, pain3,

	painb1, painb2, painb3, painb4, painb5, painb6, painb7, painb8, painb9,
	painb10, painb11,

	//attack1, attack2, attack3, attack4, attack5, attack6, attack7,
	//attack8, attack9, attack10, attack11,

	attackb1_, attackb1, attackb2, attackb3, attackb4, attackb5,
	attackb6, attackb7, attackb8, attackb9, attackb10,

	walk1, walk2, walk3, walk4, walk5, walk6, walk7, walk8, walk9,
	walk10, walk11, walk12, walk13, walk14,

	kneel1, kneel2, kneel3, kneel4, kneel5,

	standing2, standing3, standing4, standing5,

	death1, death2, death3, death4, death5, death6, death7, death8,
	death9, death10,

	deathb1, deathb2, deathb3, deathb4, deathb5, deathb6, deathb7, deathb8,
	deathb9, deathb10, deathb11
};

mframe_t knight_frames_stand[] =
{
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL }
};
mmove_t knight_stand1 = { stand1, stand9, knight_frames_stand, NULL };

void knight_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &knight_stand1;
}

static int sound_idle;
static int sound_sword1;
static int sound_sword2;
static int sound_khurt;
static int sound_ksight;
static int sound_kdeath;
static int sound_udeath;

void knight_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_ksight, 1, ATTN_NORM, 0);
}

void knight_idle_sound(edict_t *self)
{
	if (random() < 0.2f)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

mframe_t knight_frames_walk[] =
{
	{ ai_walk, 3,   knight_idle_sound },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 4,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 4,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 4,   NULL },
	{ ai_walk, 3,   NULL }
};
mmove_t knight_walk1 = { walk1, walk14, knight_frames_walk, NULL };

void knight_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &knight_walk1;
}

mframe_t knight_frames_run[] =
{
	{ ai_run, 16,   knight_idle_sound },
	{ ai_run, 20,   NULL },
	{ ai_run, 13,   NULL },
	{ ai_run, 7,   NULL },
	{ ai_run, 16,   NULL },
	{ ai_run, 20,   NULL },
	{ ai_run, 14,   NULL },
	{ ai_run, 6,   NULL }
};
mmove_t knight_run1 = { runb1, runb8, knight_frames_run, NULL };

void knight_run(edict_t *self)
{
	self->monsterinfo.currentmove = &knight_run1;
}

void knight_sword_sound(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_sword1, 1, ATTN_NORM, 0);
}

void knight_sword2_sound(edict_t *self)
{
	if (random() < 0.5f)
		gi.sound(self, CHAN_WEAPON, sound_sword1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_WEAPON, sound_sword2, 1, ATTN_NORM, 0);
}

/*
=============
ai_melee

=============
*/
void ai_melee(edict_t *self)
{
	vec3_t delta;
	float ldmg;

	if (!self->enemy)
		return;		// removed before stroke

	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);

	if (VectorLength(delta) > 60)
		return;

	ldmg = (random() + random() + random()) * 3;
	T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_MELEE, DT_DIRECT));
}

mframe_t knight_frames_atk[] =
{
	{ ai_charge, 0,   knight_sword_sound },
	{ ai_charge, 7,   NULL },
	{ ai_charge, 4,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 3,   NULL },
	{ ai_charge, 4,   ai_melee },
	{ ai_charge, 1,   ai_melee },
	{ ai_charge, 3,   ai_melee },
	{ ai_charge, 1,   NULL },
	{ ai_charge, 5,   NULL }
};
mmove_t knight_atk1 = { attackb1, attackb10, knight_frames_atk, knight_run };

void knight_atk(edict_t *self)
{
	self->monsterinfo.currentmove = &knight_atk1;
}

//===========================================================================

mframe_t knight_frames_pain[] =
{
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t knight_pain1 = { pain1, pain3, knight_frames_pain, knight_run };

void knight_paina(edict_t *self)
{
	self->monsterinfo.currentmove = &knight_pain1;
}

mframe_t knight_frames_painb[] =
{
	{ ai_move, 0,   NULL },
	{ ai_move, 3,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 2,   NULL },
	{ ai_move, 4,   NULL },
	{ ai_move, 2,   NULL },
	{ ai_move, 5,   NULL },
	{ ai_move, 5,   NULL },
	{ ai_move, 0,   NULL },

	{ ai_move, 0,   NULL }
};
mmove_t knight_painb1 = { painb1, painb11, knight_frames_painb, knight_run };

void knight_painb(edict_t *self)
{
	self->monsterinfo.currentmove = &knight_painb1;
}

void knight_pain(edict_t *self, edict_t *attacker, float kick, int damage)
{
	if (self->pain_debounce_time > level.time)
		return;

	float r = random();
	gi.sound(self, CHAN_VOICE, sound_khurt, 1, ATTN_NORM, 0);

	if (r < 0.85f)
	{
		knight_paina(self);
		self->pain_debounce_time = level.time + 1000;
	}
	else
	{
		knight_painb(self);
		self->pain_debounce_time = level.time + 1000;
	}
}

//===========================================================================

void knight_unsolid(edict_t *self)
{
	self->solid = SOLID_NOT;
	gi.linkentity(self);
}

void knight_dead(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
}

mframe_t knight_frames_diea[] =
{
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   knight_unsolid },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t knight_diea1 = { death1, death10, knight_frames_diea, knight_dead };

void knight_diea(edict_t *self)
{
	self->monsterinfo.currentmove = &knight_diea1;
}

mframe_t knight_frames_dieb[] =
{
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   knight_unsolid },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },

	{ ai_move, 0,   NULL }
};
mmove_t knight_dieb1 = { deathb1, deathb11, knight_frames_dieb, knight_dead };

void knight_dieb(edict_t *self)
{
	self->monsterinfo.currentmove = &knight_dieb1;
}

void knight_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// check for gib
	if (self->health < -40)
	{
		gi.sound(self, CHAN_VOICE, sound_udeath, 1, ATTN_NORM, 0);
		ThrowHead(self, "models/q1/h_knight.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib1.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib2.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib3.mdl", self->health, GIB_Q1);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;
	// regular death
	gi.sound(self, CHAN_VOICE, sound_kdeath, 1, ATTN_NORM, 0);

	if (random() < 0.5f)
		knight_diea(self);
	else
		knight_dieb(self);
}

void ai_charge_side(edict_t *self, float dist)
{
	vec3_t dtemp;
	float heading;
	// aim to the left of the enemy for a flyby
	VectorSubtract(self->enemy->s.origin, self->s.origin, dtemp);
	self->ideal_yaw = vectoyaw(dtemp);
	M_ChangeYaw(self);
	vec3_t v_right;
	AngleVectors(self->s.angles, NULL, v_right, NULL);
	VectorMA(self->enemy->s.origin, -30, v_right, dtemp);
	VectorSubtract(dtemp, self->s.origin, dtemp);
	heading = vectoyaw(dtemp);
	M_walkmove(self, heading, 20);
}

void ai_melee_side(edict_t *self, float dist)
{
	vec3_t delta;
	float ldmg;

	if (!self->enemy)
		return;		// removed before stroke

	ai_charge_side(self, dist);
	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);

	if (VectorLength(delta) > 60)
		return;

	if (!CanDamage(self->enemy, self))
		return;

	ldmg = (random() + random() + random()) * 3;
	T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_MELEE, DT_DIRECT));
}

mframe_t knight_frames_runatk1[] =
{
	{ ai_charge, 20,   knight_sword2_sound },
	{ ai_charge_side, 0,   NULL },
	{ ai_charge_side, 0,   NULL },
	{ ai_charge_side, 0,   NULL },
	{ ai_melee_side, 0,   NULL },
	{ ai_melee_side, 0,   NULL },
	{ ai_melee_side, 0,   NULL },
	{ ai_melee_side, 0,   NULL },
	{ ai_melee_side, 0,   NULL },
	{ ai_charge_side, 0,   NULL },

	{ ai_charge, 10,   NULL }
};
mmove_t knight_runatk1 = { runattack1, runattack11, knight_frames_runatk1, knight_run };

void knight_runatk(edict_t *self)
{
	self->monsterinfo.currentmove = &knight_runatk1;
}

/*QUAKED monster_knight (1 0 0) (-16 -16 -24) (16 16 40) Ambush
*/
void q1_monster_knight(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	gi.modelindex("models/q1/h_knight.mdl");
	sound_kdeath = gi.soundindex("q1/knight/kdeath.wav");
	sound_khurt = gi.soundindex("q1/knight/khurt.wav");
	sound_ksight = gi.soundindex("q1/knight/ksight.wav");
	sound_sword1 = gi.soundindex("q1/knight/sword1.wav");
	sound_sword2 = gi.soundindex("q1/knight/sword2.wav");
	sound_idle = gi.soundindex("q1/knight/idle.wav");
	sound_udeath = gi.soundindex("q1/player/udeath.wav");		// gib death
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->s.modelindex = gi.modelindex("models/q1/knight.mdl");
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 40);
	self->health = 75;
	self->monsterinfo.stand = knight_stand;
	self->monsterinfo.walk = knight_walk;
	self->monsterinfo.run = knight_run;
	self->monsterinfo.melee = knight_atk;
	self->monsterinfo.attack = knight_runatk;
	self->pain = knight_pain;
	self->die = knight_die;
	self->monsterinfo.sight = knight_sight;
	gi.linkentity(self);
	self->monsterinfo.currentmove = &knight_stand1;
	self->monsterinfo.scale = 1;
	walkmonster_start(self);
}
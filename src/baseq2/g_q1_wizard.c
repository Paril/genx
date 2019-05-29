/*
==============================================================================

WIZARD

==============================================================================
*/

#include "g_local.h"

enum
{
	hover1, hover2, hover3, hover4, hover5, hover6, hover7, hover8,
	hover9, hover10, hover11, hover12, hover13, hover14, hover15,

	fly1, fly2, fly3, fly4, fly5, fly6, fly7, fly8, fly9, fly10,
	fly11, fly12, fly13, fly14,

	magatt1, magatt2, magatt3, magatt4, magatt5, magatt6, magatt7,
	magatt8, magatt9, magatt10, magatt11, magatt12, magatt13,

	pain1, pain2, pain3, pain4,

	death1, death2, death3, death4, death5, death6, death7, death8
};

/*
==============================================================================

WIZARD

If the player moves behind cover before the missile is launched, launch it
at the last visible spot with no velocity leading, in hopes that the player
will duck back out and catch it.
==============================================================================
*/

/*
=============
LaunchMissile

Sets the given entities velocity and angles so that it will hit self.enemy
if self.enemy maintains it's current velocity
0.1 is moderately accurate, 0.0 is totally accurate
=============
*/
void LaunchMissile(edict_t *self, edict_t *missile, float mspeed, float accuracy)
{
	vec3_t vec, move;
	float	fly;
	vec3_t v_forward, v_right, v_up;
	AngleVectors(self->s.angles, v_forward, v_right, v_up);

	// set missile speed
	for (int i = 0; i < 3; ++i)
		vec[i] = self->enemy->s.origin[i] + self->enemy->mins[i] + self->enemy->size[i] * 0.7f - missile->s.origin[i];

	// calc aproximate time for missile to reach vec
	fly = VectorLength(vec) / mspeed;
	// get the entities xy velocity
	VectorCopy(self->enemy->velocity, move);
	move[2] = 0;
	// project the target forward in time
	VectorMA(vec, fly, move, vec);
	VectorNormalize(vec);

	for (int i = 0; i < 3; ++i)
		vec[i] = vec[i] + accuracy * v_up[i] * (random() - 0.5f) + accuracy * v_right[i] * (random() - 0.5f);

	VectorScale(vec, mspeed, missile->velocity);
	VectorClear(missile->s.angles);
	missile->s.angles[1] = vectoyaw(missile->velocity);
	// set missile duration
	missile->nextthink = level.time + 5000;
	missile->think = G_FreeEdict;
}

void wiz_run(edict_t *);
void wiz_side(edict_t *);

/*
=================
WizardCheckAttack
=================
*/
bool	WizardCheckAttack(edict_t *self)
{
	vec3_t spot1, spot2;
	edict_t *targ;
	float chance;

	if (level.time < self->monsterinfo.attack_finished)
		return false;

	if (!visible(self, self->enemy))
		return false;

	int enemy_range = range(self, self->enemy);

	if (enemy_range == RANGE_FAR)
	{
		if (self->monsterinfo.attack_state != AS_STRAIGHT)
		{
			self->monsterinfo.attack_state = AS_STRAIGHT;
			wiz_run(self);
		}

		return false;
	}

	targ = self->enemy;
	// see if any entities are in the way of the shot
	VectorCopy(self->s.origin, spot1);
	VectorCopy(targ->s.origin, spot2);
	spot1[2] += self->viewheight;
	spot2[2] += targ->viewheight;
	trace_t tr = gi.trace(spot1, vec3_origin, vec3_origin, spot2, self, MASK_SHOT);

	if (tr.ent != targ)
	{
		// don't have a clear shot, so move to a side
		if (self->monsterinfo.attack_state != AS_STRAIGHT)
		{
			self->monsterinfo.attack_state = AS_STRAIGHT;
			wiz_run(self);
		}

		return false;
	}

	if (enemy_range == RANGE_MELEE)
		chance = 0.9;
	else if (enemy_range == RANGE_NEAR)
		chance = 0.6;
	else if (enemy_range == RANGE_MID)
		chance = 0.2;
	else
		chance = 0;

	if (random() < chance)
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

	if (enemy_range == RANGE_MID)
	{
		if (self->monsterinfo.attack_state != AS_STRAIGHT)
		{
			self->monsterinfo.attack_state = AS_STRAIGHT;
			wiz_run(self);
		}
	}
	else
	{
		if (self->monsterinfo.attack_state != AS_SLIDING)
		{
			self->monsterinfo.attack_state = AS_SLIDING;
			wiz_side(self);
		}
	}

	return false;
}

void AttackFinished(edict_t *, float);

/*
=================
WizardAttackFinished
=================
*/
void WizardAttackFinished(edict_t *self)
{
	AttackFinished(self, 2);

	if (range(self, self->enemy) >= RANGE_MID || !visible(self, self->enemy))
	{
		self->monsterinfo.attack_state = AS_STRAIGHT;
		wiz_run(self);
	}
	else
	{
		self->monsterinfo.attack_state = AS_SLIDING;
		wiz_side(self);
	}
}

/*
==============================================================================

FAST ATTACKS

==============================================================================
*/

edict_t *fire_spike(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, bool super);

static int sound_wattack;
static int sound_widle1;
static int sound_widle2;
static int sound_wpain;
static int sound_udeath;
static int sound_wdeath;
static int sound_wsight;

void Wiz_FastFire(edict_t *self)
{
	vec3_t vec;
	vec3_t dst;

	if (self->owner->health > 0)
	{
		//self.owner.effects = self.owner.effects | EF_MUZZLEFLASH;
		VectorMA(self->enemy->s.origin, -13, self->movedir, dst);
		VectorSubtract(dst, self->s.origin, vec);
		VectorNormalize(vec);
		gi.sound(self, CHAN_WEAPON, sound_wattack, 1, ATTN_NORM, 0);
		edict_t *newmis = fire_spike(self, self->s.origin, vec, 9, 600, false);
		newmis->owner = self->owner;
		newmis->count = TE_Q1_WIZSPIKE;
		newmis->s.effects |= EF_Q1_WIZ;
		gi.setmodel(newmis, "models/q1/w_spike.mdl");
		gi.linkentity(newmis);
	}

	G_FreeEdict(self);
}


void Wiz_StartFast(edict_t *self)
{
	edict_t *missile;
	gi.sound(self, CHAN_WEAPON, sound_wattack, 1, ATTN_NORM, 0);
	vec3_t v_forward, v_right;
	AngleVectors(self->s.angles, v_forward, v_right, NULL);
	missile = G_Spawn();
	missile->owner = self;
	missile->nextthink = level.time + 600;
	VectorClear(missile->mins);
	VectorClear(missile->maxs);

	for (int i = 0; i < 3; ++i)
		missile->s.origin[i] = self->s.origin[i] + v_forward[i] * 14 + v_right[i] * 14;

	missile->s.origin[2] += 30;
	missile->enemy = self->enemy;
	missile->nextthink = level.time + 800;
	missile->think = Wiz_FastFire;
	VectorCopy(v_right, missile->movedir);
	missile = G_Spawn();
	missile->owner = self;
	missile->nextthink = level.time + 1000;
	VectorClear(missile->mins);
	VectorClear(missile->maxs);

	for (int i = 0; i < 3; ++i)
		missile->s.origin[i] = self->s.origin[i] + v_forward[i] * 14 + v_right[i] * -14;

	missile->s.origin[2] += 30;
	missile->enemy = self->enemy;
	missile->nextthink = level.time + 300;
	missile->think = Wiz_FastFire;
	VectorSubtract(vec3_origin, v_right, missile->movedir);
}



void Wiz_idlesound(edict_t *self)
{
	float wr = random() * 5;

	if (self->touch_debounce_time < level.time)
	{
		self->touch_debounce_time = level.time + 2000;

		if (wr > 4.5f)
			gi.sound(self, CHAN_VOICE, sound_widle1, 1,  ATTN_IDLE, 0);

		if (wr < 1.5f)
			gi.sound(self, CHAN_VOICE, sound_widle2, 1, ATTN_IDLE, 0);
	}
}

mframe_t wiz_frames_stand[] =
{
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL }
};
mmove_t wiz_stand1 = { hover1, hover8, wiz_frames_stand, NULL };

void wiz_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &wiz_stand1;
}

mframe_t wiz_frames_walk[] =
{
	{ ai_walk, 8,   Wiz_idlesound },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL }
};
mmove_t wiz_walk1 = { hover1, hover8, wiz_frames_walk, NULL };

void wiz_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &wiz_walk1;
}

mframe_t wiz_frames_side[] =
{
	{ ai_run, 8,   Wiz_idlesound },
	{ ai_run, 8,   NULL },
	{ ai_run, 8,   NULL },
	{ ai_run, 8,   NULL },
	{ ai_run, 8,   NULL },
	{ ai_run, 8,   NULL },
	{ ai_run, 8,   NULL },
	{ ai_run, 8,   NULL }
};
mmove_t wiz_side1 = { hover1, hover8, wiz_frames_side, NULL };

void wiz_side(edict_t *self)
{
	self->monsterinfo.currentmove = &wiz_side1;
}

mframe_t wiz_frames_run[] =
{
	{ ai_run, 16,   Wiz_idlesound },
	{ ai_run, 16,   NULL },
	{ ai_run, 16,   NULL },
	{ ai_run, 16,   NULL },
	{ ai_run, 16,   NULL },
	{ ai_run, 16,   NULL },
	{ ai_run, 16,   NULL },
	{ ai_run, 16,   NULL },
	{ ai_run, 16,   NULL },
	{ ai_run, 16,   NULL },
	{ ai_run, 16,   NULL },
	{ ai_run, 16,   NULL },
	{ ai_run, 16,   NULL },
	{ ai_run, 16,   NULL }
};
mmove_t wiz_run1 = { fly1, fly14, wiz_frames_run, NULL };

void wiz_run(edict_t *self)
{
	self->monsterinfo.currentmove = &wiz_run1;
}

void wiz_fastfinish(edict_t *self)
{
	WizardAttackFinished(self);
	wiz_run(self);
}

mframe_t wiz_frames_fast2[] =
{
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t wiz_fast2 = { magatt5, magatt2, wiz_frames_fast2, wiz_fastfinish };

void wiz_fastb(edict_t *self)
{
	self->monsterinfo.currentmove = &wiz_fast2;
}

mframe_t wiz_frames_fast[] =
{
	{ ai_move, 0,   Wiz_StartFast },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
};
mmove_t wiz_fast1 = { magatt1, magatt6, wiz_frames_fast, wiz_fastb };

void wiz_fast(edict_t *self)
{
	self->monsterinfo.currentmove = &wiz_fast1;
}

mframe_t wiz_frames_pain[] =
{
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t wiz_pain1 = { pain1, pain4, wiz_frames_pain, wiz_run };

void wiz_paina(edict_t *self)
{
	self->monsterinfo.currentmove = &wiz_pain1;
}

void wiz_unsolid(edict_t *self)
{
	self->solid = SOLID_NOT;
	gi.linkentity(self);
}

void wiz_dead(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
}

mframe_t wiz_frames_death[] =
{
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   wiz_unsolid },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t wiz_death1 = { death1, death8, wiz_frames_death, wiz_dead };

void wiz_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// check for gib
	if (self->health < -40)
	{
		gi.sound(self, CHAN_VOICE, sound_udeath, 1, ATTN_NORM, 0);
		ThrowHead(self, "models/q1/h_wizard.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib2.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib2.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib2.mdl", self->health, GIB_Q1);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag)
		return;

	self->deadflag = DEAD_DEAD;
	self->monsterinfo.currentmove = &wiz_death1;
	self->velocity[0] = -200 + 400 * random();
	self->velocity[1] = -200 + 400 * random();
	self->velocity[2] = 100 + 100 * random();
	self->groundentity = NULL;
	gi.sound(self, CHAN_VOICE, sound_wdeath, 1, ATTN_NORM, 0);
}

void Wiz_Pain(edict_t *self, edict_t *attacker, float kick, int damage)
{
	gi.sound(self, CHAN_VOICE, sound_wpain, 1, ATTN_NORM, 0);

	if (random() * 70 > damage)
		return;		// didn't flinch

	wiz_paina(self);
}


void Wiz_Missile(edict_t *self)
{
	wiz_fast(self);
}

void wiz_sight(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_wsight, 1, ATTN_NORM, 0);
}

/*QUAKED monster_wizard (1 0 0) (-16 -16 -24) (16 16 40) Ambush
*/
void q1_monster_wizard(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	gi.modelindex("models/q1/wizard.mdl");
	gi.modelindex("models/q1h_wizard.mdl");
	gi.modelindex("models/q1w_spike.mdl");
	sound_wattack = gi.soundindex("q1/wizard/wattack.wav");
	sound_wdeath = gi.soundindex("q1/wizard/wdeath.wav");
	sound_widle1 = gi.soundindex("q1/wizard/widle1.wav");
	sound_widle2 = gi.soundindex("q1/wizard/widle2.wav");
	sound_wpain = gi.soundindex("q1/wizard/wpain.wav");
	sound_wsight = gi.soundindex("q1/wizard/wsight.wav");
	sound_udeath = gi.soundindex("q1/player/udeath.wav");		// gib death
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	gi.setmodel(self, "models/q1/wizard.mdl");
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 40);
	self->health = 80;
	self->monsterinfo.stand = wiz_stand;
	self->monsterinfo.walk = wiz_walk;
	self->monsterinfo.run = wiz_run;
	self->monsterinfo.attack = Wiz_Missile;
	self->monsterinfo.checkattack = WizardCheckAttack;
	self->pain = Wiz_Pain;
	self->die = wiz_die;
	self->monsterinfo.scale = 1;
	wiz_stand(self);
	flymonster_start(self);
	self->viewheight = 25;
}

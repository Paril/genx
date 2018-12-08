/*
==============================================================================

SOLDIER / PLAYER

==============================================================================
*/

#include "g_local.h"

enum {
	stand1, stand2, stand3, stand4, stand5, stand6, stand7,

	walk1, walk2, walk3, walk4, walk5, walk6, walk7, walk8, walk9, walk10,
	walk11, walk12, walk13, walk14, walk15, walk16,

	run1, run2, run3, run4, run5, run6, run7, run8,

	attack1, attack2, attack3, attack4, attack5, attack6,
	attack7, attack8, attack9, attack10,

	death1, death2, death3, death4, death5, death6, death7, death8,
	death9, death10, death11, death12, death13, death14,

	fdeath1, fdeath2, fdeath3, fdeath4, fdeath5, fdeath6, fdeath7, fdeath8,
	fdeath9, fdeath10, fdeath11,

	paina1, paina2, paina3, paina4,

	painb1, painb2, painb3, painb4, painb5,

	painc1, painc2, painc3, painc4, painc5, painc6, painc7, painc8,

	paind1, paind2, paind3, paind4, paind5, paind6, paind7, paind8,
	paind9, paind10, paind11, paind12, paind13, paind14, paind15, paind16,
	paind17, paind18, paind19
};

static int sound_enfire;
static int sound_udeath;
static int sound_death1;
static int sound_idle1;
static int sound_pain1;
static int sound_pain2;
static int sound_sight1;
static int sound_sight2;
static int sound_sight3;
static int sound_sight4;

void fire_q1_laser(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed);

void enforcer_fire(edict_t *self)
{
	gi.positioned_sound(self->s.origin, self, CHAN_VOICE, sound_enfire, 1, ATTN_NORM, 0);

	//self.effects = self.effects | EF_MUZZLEFLASH;
	vec3_t org, v_forward, v_right;
	AngleVectors(self->s.angles, v_forward, v_right, NULL);

	for (int i = 0; i < 3; ++i)
		org[i] = self->s.origin[i] + v_forward[i] * 30 + v_right[i] * 8.5f;
	
	org[2] += 16;

	vec3_t dir;
	VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
	VectorNormalize(dir);
	fire_q1_laser(self, org, dir, 15, 500);
}

//============================================================================

mframe_t enf_frames_stand1[] = {
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL }
};
mmove_t enf_stand1 = { stand1, stand7, enf_frames_stand1, NULL };

void enf_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &enf_stand1;
}

void enf_idle_sound(edict_t *self)
{
	if (random() < 0.2f)
		gi.sound(self, CHAN_VOICE, sound_idle1, 1, ATTN_IDLE, 0);
}

mframe_t enf_frames_walk1[] = {
	{ ai_walk, 2,   enf_idle_sound },
	{ ai_walk, 4,   NULL },
	{ ai_walk, 4,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 1,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 1,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 4,   NULL },
	{ ai_walk, 4,   NULL },
	{ ai_walk, 1,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 4,   NULL },
	{ ai_walk, 2,   NULL }
};
mmove_t enf_walk1 = { walk1, walk16, enf_frames_walk1, NULL };

void enf_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &enf_walk1;
}

mframe_t enf_frames_run1[] = {
	{ ai_run, 18,   enf_idle_sound },
	{ ai_run, 14,   NULL },
	{ ai_run, 7,   NULL },
	{ ai_run, 12,   NULL },
	{ ai_run, 14,   NULL },
	{ ai_run, 14,   NULL },
	{ ai_run, 7,   NULL },
	{ ai_run, 11,   NULL }
};
mmove_t enf_run1 = { run1, run8, enf_frames_run1, NULL };

void enf_run(edict_t *self)
{
	self->monsterinfo.currentmove = &enf_run1;
}

void SUB_CheckRefire(edict_t *self, int frame);

void enf_refire(edict_t *self)
{
	SUB_CheckRefire(self, attack1);
}

void enf_pew(edict_t *self)
{
	if (self->dmg == 0)
	{
		self->monsterinfo.nextframe = attack5;
		self->dmg = 1;
		return;
	}

	self->dmg = 0;
}

mframe_t enf_frames_atk1[] = {
	{ ai_charge, 0,   enf_idle_sound },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   enforcer_fire },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   enf_pew },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   enf_refire }
};
mmove_t enf_atk1 = { attack1, attack10, enf_frames_atk1, enf_run };

void enf_atk(edict_t *self)
{
	self->monsterinfo.currentmove = &enf_atk1;
}

mframe_t enf_frames_paina1[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t enf_paina1 = { paina1, paina4, enf_frames_paina1, enf_run };

mframe_t enf_frames_painb1[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t enf_painb1 = { painb1, painb5, enf_frames_painb1, enf_run };

mframe_t enf_frames_painc1[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t enf_painc1 = { painc1, painc8, enf_frames_painc1, enf_run };

mframe_t enf_frames_paind1[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 2,   NULL },
	{ ai_move, 1,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 1,   NULL },
	{ ai_move, 1,   NULL },
	{ ai_move, 1,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, -1,   NULL },
	{ ai_move, -1,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t enf_paind1 = { paind1, paind19, enf_frames_paind1, enf_run };

void enf_pain(edict_t *self, edict_t *attacker, float kick, int damage)
{
	float r = random ();
	
	if (self->pain_debounce_time > level.time)
		return;

	if (r < 0.5f)
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	if (r < 0.2f)
	{
		self->pain_debounce_time = level.time + 1000;
		self->monsterinfo.currentmove = &enf_paina1;
	}
	else if (r < 0.4f)
	{
		self->pain_debounce_time = level.time + 1000;
		self->monsterinfo.currentmove = &enf_painb1;
	}
	else if (r < 0.7f)
	{
		self->pain_debounce_time = level.time + 1000;
		self->monsterinfo.currentmove = &enf_painc1;
	}
	else
	{
		self->pain_debounce_time = level.time + 2000;
		self->monsterinfo.currentmove = &enf_paind1;
	}
}

//============================================================================

edict_t *Drop_Backpack(edict_t *self);

void enf_drop(edict_t *self)
{
	self->solid = SOLID_NOT;
	//edict_t *backpack = Drop_Backpack(self);
	//backpack->pack_cells = 5;
	gi.linkentity(self);
}

void enf_dead(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
}

mframe_t enf_frames_die1[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   enf_drop },
	{ ai_move, 14,   NULL },
	{ ai_move, 2,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 3,   NULL },
	{ ai_move, 5,   NULL },
	{ ai_move, 5,   NULL },
	{ ai_move, 5,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t enf_die1 = { death1, death14, enf_frames_die1, enf_dead };

mframe_t enf_frames_fdie1[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   enf_drop },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t enf_fdie1 = { fdeath1, fdeath11, enf_frames_fdie1, enf_dead };

void enf_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
// check for gib
	if (self->health < -35)
	{
		gi.sound (self, CHAN_VOICE, sound_udeath, 1, ATTN_NORM, 0);
		ThrowHead(self, "models/q1/h_mega.mdl", self->health, GIB_Q1);
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
	gi.sound (self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);

	if (random() > 0.5f)
		self->monsterinfo.currentmove = &enf_die1;
	else
		self->monsterinfo.currentmove = &enf_fdie1;
}

void enf_sight(edict_t *self, edict_t *other)
{
	int rsnd = Q_rint(random() * 3);

	if (rsnd == 1)
		gi.sound(self, CHAN_BODY, sound_sight1, 1, ATTN_NORM, 0);
	else if (rsnd == 2)
		gi.sound(self, CHAN_BODY, sound_sight2, 1, ATTN_NORM, 0);
	else if (rsnd == 0)
		gi.sound(self, CHAN_BODY, sound_sight3, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_BODY, sound_sight4, 1, ATTN_NORM, 0);
}

/*QUAKED monster_enforcer (1 0 0) (-16 -16 -24) (16 16 40) Ambush

*/
void q1_monster_enforcer(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}
	
	gi.modelindex("models/q1/enforcer.mdl");
	gi.modelindex("models/q1/h_mega.mdl");
	gi.modelindex("models/q1/laser.mdl");

	sound_death1 = gi.soundindex("q1/enforcer/death1.wav");
	sound_enfire = gi.soundindex("q1/enforcer/enfire.wav");
	gi.soundindex("q1/enforcer/enfstop.wav");
	sound_idle1 = gi.soundindex("q1/enforcer/idle1.wav");
	sound_pain1 = gi.soundindex("q1/enforcer/pain1.wav");
	sound_pain2 = gi.soundindex("q1/enforcer/pain2.wav");
	sound_sight1 = gi.soundindex("q1/enforcer/sight1.wav");
	sound_sight2 = gi.soundindex("q1/enforcer/sight2.wav");
	sound_sight3 = gi.soundindex("q1/enforcer/sight3.wav");
	sound_sight4 = gi.soundindex("q1/enforcer/sight4.wav");
	sound_udeath = gi.soundindex("q1/player/udeath.wav");		// gib death
	
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;

	gi.setmodel (self, "models/q1/enforcer.mdl");

	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 40);
	self->health = 80;

	self->monsterinfo.stand = enf_stand;
	self->monsterinfo.walk = enf_walk;
	self->monsterinfo.run = enf_run;
	self->monsterinfo.attack = enf_atk;
	self->monsterinfo.sight = enf_sight;
	self->pain = enf_pain;
	self->die = enf_die;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &enf_stand1;
	self->monsterinfo.scale = 1;

	walkmonster_start(self);
}

/*
==============================================================================

DEMON

==============================================================================
*/

#include "g_local.h"

enum
{
	stand1, stand2, stand3, stand4, stand5, stand6, stand7, stand8, stand9,
	stand10, stand11, stand12, stand13,

	walk1, walk2, walk3, walk4, walk5, walk6, walk7, walk8,

	run1, run2, run3, run4, run5, run6,

	leap1, leap2, leap3, leap4, leap5, leap6, leap7, leap8, leap9, leap10,
	leap11, leap12,

	pain1, pain2, pain3, pain4, pain5, pain6,

	death1, death2, death3, death4, death5, death6, death7, death8, death9,

	attacka1, attacka2, attacka3, attacka4, attacka5, attacka6, attacka7, attacka8,
	attacka9, attacka10, attacka11, attacka12, attacka13, attacka14, attacka15
};

//============================================================================

static int sound_djump;
static int sound_dhit2;
static int sound_idle1;
static int sound_dpain1;
static int sound_ddeath;
static int sound_sight2;
static int sound_udeath;

/*
==============================================================================

DEMON

==============================================================================
*/

/*
==============
CheckDemonMelee

Returns TRUE if a melee attack would hit right now
==============
*/
bool CheckDemonMelee(edict_t *self)
{
	if (range(self, self->enemy) == RANGE_MELEE)
	{
		// FIXME: check canreach
		self->monsterinfo.attack_state = AS_MELEE;
		return true;
	}

	return false;
}

/*
==============
CheckDemonJump

==============
*/
bool CheckDemonJump(edict_t *self)
{
	vec3_t dist;
	float d;

	if (self->s.origin[2] + self->mins[2] > self->enemy->s.origin[2] + self->enemy->mins[2]
		+ 0.75f * self->enemy->size[2])
		return false;

	if (self->s.origin[2] + self->maxs[2] < self->enemy->s.origin[2] + self->enemy->mins[2]
		+ 0.25f * self->enemy->size[2])
		return true;

	VectorSubtract(self->enemy->s.origin, self->s.origin, dist);
	dist[2] = 0;
	d = VectorLength(dist);

	if (d < 100)
		return false;

	if (d > 200)
	{
		if (random() < 0.9f)
			return false;
	}

	return true;
}

bool DemonCheckAttack(edict_t *self)
{
	// if close enough for slashing, go for it
	if (CheckDemonMelee(self))
	{
		self->monsterinfo.attack_state = AS_MELEE;
		return true;
	}

	if (CheckDemonJump(self))
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		gi.sound(self, CHAN_VOICE, sound_djump, 1, ATTN_NORM, 0);
		return true;
	}

	return false;
}


//===========================================================================

void SpawnMeatSpray(edict_t *self, vec3_t org, vec3_t vel);
void Demon_Melee(edict_t *self, float side)
{
	float ldmg;
	vec3_t delta;
	M_walkmove(self, self->ideal_yaw, 12);	// allow a little closing
	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);

	if (VectorLength(delta) > 100)
		return;

	if (!CanDamage(self->enemy, self))
		return;

	gi.sound(self, CHAN_WEAPON, sound_dhit2, 1, ATTN_NORM, 0);
	ldmg = 10 + 5 * random();
	T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_MELEE, DT_DIRECT));
	vec3_t v_forward, v_right;
	AngleVectors(self->s.angles, v_forward, v_right, NULL);
	vec3_t o, v;
	VectorMA(self->s.origin, 16, v_forward, o);
	VectorScale(v_right, side, v);
	SpawnMeatSpray(self, o, v);
}

void Demon_JumpTouch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	float ldmg;

	if (self->health <= 0)
	{
		self->touch = NULL;
		return;
	}

	if (other->takedamage)
	{
		if (VectorLength(self->velocity) > 400)
		{
			ldmg = 40 + 10 * random();
			T_Damage(other, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_NONE, DT_DIRECT));
		}
	}

	if (!M_CheckBottom(self))
	{
		if (self->groundentity)
		{
			// jump randomly to not get hung up
			//dprint ("popjump\n");
			self->touch = NULL;
			self->monsterinfo.nextframe = leap1;
			//			self.velocity_x = (random() - 0.5) * 600;
			//			self.velocity_y = (random() - 0.5) * 600;
			//			self.velocity_z = 200;
			//			self.flags = self.flags - FL_ONGROUND;
		}

		return;	// not on ground yet
	}

	self->monsterinfo.nextframe = leap11;
	self->touch = NULL;
}

mframe_t demon_frames_stand1[] =
{
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
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
mmove_t demon_stand1 = { stand1, stand13, demon_frames_stand1, NULL };

void demon_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &demon_stand1;
}

void demon_idle_sound(edict_t *self)
{
	if (random() < 0.2f)
		gi.sound(self, CHAN_VOICE, sound_idle1, 1, ATTN_IDLE, 0);
}

mframe_t demon_frames_walk1[] =
{
	{ ai_walk, 8,   demon_idle_sound },
	{ ai_walk, 6,   NULL },
	{ ai_walk, 6,   NULL },
	{ ai_walk, 7,   NULL },
	{ ai_walk, 4,   NULL },
	{ ai_walk, 6,   NULL },
	{ ai_walk, 10,   NULL },
	{ ai_walk, 10,   NULL }
};
mmove_t demon_walk1 = { walk1, walk8, demon_frames_walk1, NULL };

void demon_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &demon_walk1;
}

mframe_t demon_frames_run1[] =
{
	{ ai_run, 20,   demon_idle_sound },
	{ ai_run, 15,   NULL },
	{ ai_run, 36,   NULL },
	{ ai_run, 20,   NULL },
	{ ai_run, 45,   NULL },
	{ ai_run, 36,   NULL }
};
mmove_t demon_run1 = { run1, run6, demon_frames_run1, NULL };

void demon_run(edict_t *self)
{
	self->monsterinfo.currentmove = &demon_run1;
}

void demon_do_jump(edict_t *self)
{
	self->monsterinfo.attack_finished = level.time + 3000;
	vec3_t v_forward;
	AngleVectors(self->s.angles, v_forward, NULL, NULL);
	self->s.origin[2]++;
	VectorScale(v_forward, 600, self->velocity);
	self->velocity[2] = 250;
	self->groundentity = NULL;
	gi.linkentity(self);
	self->touch = Demon_JumpTouch;
}

void demon_do_lock(edict_t *self)
{
	if (level.time > self->monsterinfo.attack_finished)
		self->monsterinfo.nextframe = leap1;
	else
		self->monsterinfo.nextframe = leap10;
}

mframe_t demon_frames_jump1[] =
{
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 15,   demon_do_jump },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   demon_do_lock },

	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL }
};
mmove_t demon_jump1 = { leap1, leap12, demon_frames_jump1, demon_run };

void demon_jump(edict_t *self)
{
	self->monsterinfo.currentmove = &demon_jump1;
}

void Demon_Swipe(edict_t *self)
{
	if (self->s.frame == attacka5)
		Demon_Melee(self, 200);
	else
		Demon_Melee(self, -200);
}

mframe_t demon_frames_atta1[] =
{
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   Demon_Swipe },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },

	{ ai_charge, 0,   Demon_Swipe },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL }
};
mmove_t demon_atta1 = { attacka1, attacka15, demon_frames_atta1, demon_run };

void demon_atta(edict_t *self)
{
	self->monsterinfo.currentmove = &demon_atta1;
}

mframe_t demon_frames_pain1[] =
{
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t demon_pain1 = { pain6, pain1, demon_frames_pain1, demon_run };

void demon_paina(edict_t *self)
{
	self->monsterinfo.currentmove = &demon_pain1;
}

void demon_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->touch == Demon_JumpTouch)
		return;

	if (self->pain_debounce_time > level.time)
		return;

	self->pain_debounce_time = level.time + 1000;
	gi.sound(self, CHAN_VOICE, sound_dpain1, 1, ATTN_NORM, 0);

	if (random() * 200 > damage)
		return;		// didn't flinch

	demon_paina(self);
}

void demon_unsolid(edict_t *self)
{
	self->solid = SOLID_NOT;
	gi.linkentity(self);
}

void demon_dead(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
}

mframe_t demon_frames_die1[] =
{
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   demon_unsolid },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t demon_die1 = { death1, death9, demon_frames_die1, demon_dead };

void demon_diea(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_ddeath, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &demon_die1;
}

void demon_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// check for gib
	if (self->health < -80)
	{
		gi.sound(self, CHAN_VOICE, sound_udeath, 1, ATTN_NORM, 0);
		ThrowHead(self, "models/q1/h_demon.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib1.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib1.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib1.mdl", self->health, GIB_Q1);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;
	// regular death
	demon_diea(self);
	gi.linkentity(self);
}


void demon_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight2, 1, ATTN_NORM, 0);
}

/*QUAKED monster_demon1 (1 0 0) (-32 -32 -24) (32 32 64) Ambush

*/
void q1_monster_demon(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	gi.modelindex("models/q1/demon.mdl");
	gi.modelindex("models/q1/h_demon.mdl");
	sound_ddeath = gi.soundindex("q1/demon/ddeath.wav");
	sound_dhit2 = gi.soundindex("q1/demon/dhit2.wav");
	sound_djump = gi.soundindex("q1/demon/djump.wav");
	sound_dpain1 = gi.soundindex("q1/demon/dpain1.wav");
	sound_idle1 = gi.soundindex("q1/demon/idle1.wav");
	sound_sight2 = gi.soundindex("q1/demon/sight2.wav");
	sound_udeath = gi.soundindex("q1/player/udeath.wav");
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	gi.setmodel(self, "models/q1/demon.mdl");
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, 64);
	self->health = 300;
	self->monsterinfo.stand = demon_stand;
	self->monsterinfo.walk = demon_walk;
	self->monsterinfo.run = demon_run;
	self->monsterinfo.sight = demon_sight;
	self->die = demon_die;
	self->monsterinfo.melee = demon_atta;		// one of two attacks
	self->monsterinfo.attack = demon_jump;			// jump attack
	self->monsterinfo.checkattack = DemonCheckAttack;
	self->pain = demon_pain;
	self->s.game = GAME_Q1;
	walkmonster_start(self);
	gi.linkentity(self);
	self->monsterinfo.currentmove = &demon_stand1;
	self->monsterinfo.scale = 1;
}
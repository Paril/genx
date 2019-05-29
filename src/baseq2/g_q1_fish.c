#include "g_local.h"

enum
{
	attack1, attack2, attack3, attack4, attack5, attack6,
	attack7, attack8, attack9, attack10, attack11, attack12, attack13,
	attack14, attack15, attack16, attack17, attack18,

	death1, death2, death3, death4, death5, death6, death7,
	death8, death9, death10, death11, death12, death13, death14, death15,
	death16, death17, death18, death19, death20, death21,

	swim1, swim2, swim3, swim4, swim5, swim6, swim7, swim8,
	swim9, swim10, swim11, swim12, swim13, swim14, swim15, swim16, swim17,
	swim18,

	pain1, pain2, pain3, pain4, pain5, pain6, pain7, pain8,
	pain9
};

mframe_t f_frames_stand1[] =
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
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL },
	{ ai_stand, 0,   NULL }
};
mmove_t f_stand1 = { swim1, swim18, f_frames_stand1, NULL };

void f_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &f_stand1;
}

mframe_t f_frames_walk1[] =
{
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },

	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 8,   NULL }
};
mmove_t f_walk1 = { swim1, swim18, f_frames_walk1, NULL };

void f_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &f_walk1;
}

static int sound_bite;
static int sound_death;
static int sound_idle;

void fish_idle_sound(edict_t *self)
{
	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_NORM, 0);
}

void fish_run_skip(edict_t *self)
{
	if (self->s.frame == swim2)
		fish_idle_sound(self);

	self->s.frame++;
}

mframe_t f_frames_run1[] =
{
	{ ai_run, 12,   fish_run_skip },
	{ ai_run, 12,   fish_run_skip },
	{ ai_run, 12,   fish_run_skip },
	{ ai_run, 12,   fish_run_skip },
	{ ai_run, 12,   fish_run_skip },
	{ ai_run, 12,   fish_run_skip },
	{ ai_run, 12,   fish_run_skip },
	{ ai_run, 12,   fish_run_skip },
	{ ai_run, 12,   fish_run_skip },

	{ ai_run, 12,   fish_run_skip },
	{ ai_run, 12,   fish_run_skip },
	{ ai_run, 12,   fish_run_skip },
	{ ai_run, 12,   fish_run_skip },
	{ ai_run, 12,   fish_run_skip },
	{ ai_run, 12,   fish_run_skip },
	{ ai_run, 12,   fish_run_skip }
};
mmove_t f_run1 = { swim2, swim17, f_frames_run1, NULL };

void f_run(edict_t *self)
{
	self->monsterinfo.currentmove = &f_run1;
}

void fish_melee(edict_t *self)
{
	if (!self->enemy)
		return;		// removed before stroke

	vec3_t delta;
	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);

	if (VectorLength(delta) > 60)
		return;

	gi.sound(self, CHAN_VOICE, sound_bite, 1, ATTN_NORM, 0);
	float ldmg = (random() + random()) * 3;
	T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_MELEE, DT_DIRECT));
}

mframe_t f_frames_attack1[] =
{
	{ ai_charge, 10,   fish_run_skip },
	{ ai_charge, 10,   fish_run_skip },
	{ ai_charge, 10,   fish_melee },
	{ ai_charge, 10,   fish_run_skip },
	{ ai_charge, 10,   fish_run_skip },
	{ ai_charge, 10,   fish_run_skip },
	{ ai_charge, 10,   fish_run_skip },
	{ ai_charge, 10,   fish_run_skip },
	{ ai_charge, 10,   fish_melee },
	{ ai_charge, 10,   fish_run_skip },

	{ ai_charge, 10,   fish_run_skip },
	{ ai_charge, 10,   fish_run_skip },
	{ ai_charge, 10,   fish_run_skip },
	{ ai_charge, 10,   fish_run_skip },
	{ ai_charge, 10,   fish_melee },
	{ ai_charge, 10,   fish_run_skip },
	{ ai_charge, 10,   fish_run_skip },
	{ ai_charge, 10,   fish_run_skip }
};
mmove_t f_attack1 = { attack1, attack18, f_frames_attack1, f_run };

void f_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &f_attack1;
}

void fish_dead(edict_t *self)
{
	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

mframe_t f_frames_death1[] =
{
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },

	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },

	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t f_death1 = { death1, death21, f_frames_death1, fish_dead };

void f_death(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &f_death1;
}

mframe_t f_frames_pain1[] =
{
	{ ai_move, 0,    NULL },
	{ ai_move, -6,   NULL },
	{ ai_move, -6,   NULL },
	{ ai_move, -6,   NULL },
	{ ai_move, -6,   NULL },
	{ ai_move, -6,   NULL },
	{ ai_move, -6,   NULL },
	{ ai_move, -6,   NULL },
	{ ai_move, -6,   NULL }
};
mmove_t f_pain1 = { pain1, pain9, f_frames_pain1, f_run };

void f_pain(edict_t *self, edict_t *other, float damage, int kick)
{
	self->monsterinfo.currentmove = &f_pain1;
}



/*QUAKED monster_fish (1 0 0) (-16 -16 -24) (16 16 24) Ambush
*/
void q1_monster_fish(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	gi.modelindex("models/q1/fish.mdl");
	sound_death = gi.soundindex("q1/fish/death.wav");
	sound_bite = gi.soundindex("q1/fish/bite.wav");
	sound_idle = gi.soundindex("q1/fish/idle.wav");
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	gi.setmodel(self, "models/q1/fish.mdl");
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 24);
	self->health = 25;
	self->monsterinfo.stand = f_stand;
	self->monsterinfo.walk = f_walk;
	self->monsterinfo.run = f_run;
	self->die = f_death;
	self->pain = f_pain;
	self->monsterinfo.melee = f_attack;
	self->monsterinfo.currentmove = &f_stand1;
	self->monsterinfo.scale = 1;
	swimmonster_start(self);
	gi.linkentity(self);
}


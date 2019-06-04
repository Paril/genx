/*
==============================================================================

BLOB

==============================================================================
*/
#include "g_local.h"

enum
{
	walk1, walk2, walk3, walk4, walk5, walk6, walk7, walk8, walk9, walk10,
	walk11, walk12, walk13, walk14, walk15, walk16, walk17, walk18, walk19,
	walk20, walk21, walk22, walk23, walk24, walk25,

	run1, run2, run3, run4, run5, run6, run7, run8, run9, run10, run11, run12, run13,
	run14, run15, run16, run17, run18, run19, run20, run21, run22, run23,
	run24, run25,

	jump1, jump2, jump3, jump4, jump5, jump6,

	fly1, fly2, fly3, fly4,

	explode
};

mframe_t tbaby_frames_stand1[] =
{
	{ ai_stand, 0,   NULL }
};
mmove_t tbaby_stand1 = { walk1, walk1, tbaby_frames_stand1, NULL };

void tbaby_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &tbaby_stand1;
}

mframe_t tbaby_frames_walk1[] =
{
	{ ai_walk, 0,   NULL },
	{ ai_walk, 0,   NULL },
	{ ai_walk, 0,   NULL },
	{ ai_walk, 0,   NULL },
	{ ai_walk, 0,   NULL },
	{ ai_walk, 0,   NULL },
	{ ai_walk, 0,   NULL },
	{ ai_walk, 0,   NULL },
	{ ai_walk, 0,   NULL },
	{ ai_walk, 0,   NULL },

	{ ai_walk, 0,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   NULL },

	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   NULL }
};
mmove_t tbaby_walk1 = { walk1, walk25, tbaby_frames_walk1, NULL };

void tbaby_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &tbaby_walk1;
}

mframe_t tbaby_frames_run1[] =
{
	{ ai_run, 0,   NULL },
	{ ai_run, 0,   NULL },
	{ ai_run, 0,   NULL },
	{ ai_run, 0,   NULL },
	{ ai_run, 0,   NULL },
	{ ai_run, 0,   NULL },
	{ ai_run, 0,   NULL },
	{ ai_run, 0,   NULL },
	{ ai_run, 0,   NULL },
	{ ai_run, 0,   NULL },

	{ ai_run, 0,   NULL },
	{ ai_run, 2,   NULL },
	{ ai_run, 2,   NULL },
	{ ai_run, 2,   NULL },
	{ ai_run, 2,   NULL },
	{ ai_run, 2,   NULL },
	{ ai_run, 2,   NULL },
	{ ai_run, 2,   NULL },
	{ ai_run, 2,   NULL },
	{ ai_run, 2,   NULL },

	{ ai_run, 2,   NULL },
	{ ai_run, 2,   NULL },
	{ ai_run, 2,   NULL },
	{ ai_run, 2,   NULL },
	{ ai_run, 2,   NULL }
};
mmove_t tbaby_run1 = { run1, run25, tbaby_frames_run1, NULL };

void tbaby_run(edict_t *self)
{
	self->monsterinfo.currentmove = &tbaby_run1;
}

//============================================================================


void tbaby_jump(edict_t *self);

static q_soundhandle sound_hit1;
static q_soundhandle sound_land1;
static q_soundhandle sound_death1;
static q_soundhandle sound_sight1;

void Tar_JumpTouch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->takedamage && other->entitytype != self->entitytype)
	{
		if (VectorLength(self->velocity) > 400)
		{
			float ldmg = 10 + 10 * random();
			T_Damage(other, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_MELEE, DT_DIRECT));
			gi.sound(self, CHAN_WEAPON, sound_hit1, 1, ATTN_NORM, 0);
		}
	}
	else
		gi.sound(self, CHAN_WEAPON, sound_land1, 1, ATTN_NORM, 0);

	if (!M_CheckBottom(self))
	{
		if (self->groundentity)
		{
			// jump randomly to not get hung up
			//dprint ("popjump\n");
			self->touch = NULL;
			tbaby_run(self);
			self->movetype = MOVETYPE_STEP;
			//			self.velocity_x = (random() - 0.5) * 600;
			//			self.velocity_y = (random() - 0.5) * 600;
			//			self.velocity_z = 200;
			//			self.flags = self.flags - FL_ONGROUND;
		}

		return;	// not on ground yet
	}

	self->touch = NULL;
	tbaby_jump(self);
}

void	tbaby_jump_do(edict_t *self)
{
	self->movetype = MOVETYPE_BOUNCE;
	self->touch = Tar_JumpTouch;
	vec3_t v_forward;
	AngleVectors(self->s.angles, v_forward, NULL, NULL);
	self->s.origin[2] = self->s.origin[2] + 1;
	VectorScale(v_forward, 600, self->velocity);
	self->velocity[2] = self->velocity[2] + 600 + random() * 150;
	self->groundentity = NULL;
	self->count = 0;
}

void tbaby_fly_go(edict_t *self)
{
	self->count = self->count + 1;

	if (self->count == 4)
	{
		//dprint ("spawn hop\n");
		tbaby_jump_do(self);
	}
}

mframe_t tbaby_frames_fly1[] =
{
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   tbaby_fly_go }
};
mmove_t tbaby_fly1 = { fly1, fly4, tbaby_frames_fly1, NULL };

void tbaby_fly(edict_t *self)
{
	self->monsterinfo.currentmove = &tbaby_fly1;
}

mframe_t tbaby_frames_jump1[] =
{
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   tbaby_jump_do },
	{ ai_charge, 0,   NULL }
};
mmove_t tbaby_jump1 = { jump1, jump6, tbaby_frames_jump1, tbaby_fly };

void tbaby_jump(edict_t *self)
{
	self->monsterinfo.currentmove = &tbaby_jump1;
}

//=============================================================================

void tbaby_explode(edict_t *self)
{
	T_RadiusDamage(self, self, 120, world, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, 120, MakeGenericMeansOfDeath(self, MD_NONE, DT_INDIRECT));
	gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
	VectorNormalize(self->velocity);
	VectorMA(self->s.origin, -8, self->velocity, self->s.origin);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_Q1_TAREXPLOSION);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);
	self->think = G_FreeEdict;
	self->nextthink = level.time + 100;
}

void tbaby_nohurt(edict_t *self)
{
	self->takedamage = DAMAGE_NO;
}

mframe_t tbaby_frames_die1[] =
{
	{ ai_move, 0,   tbaby_nohurt }
};
mmove_t tbaby_die1 = { explode, explode, tbaby_frames_die1, tbaby_explode };

void tbaby_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->monsterinfo.currentmove = &tbaby_die1;
}

void tbaby_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0);
}

//=============================================================================


/*QUAKED monster_tarbaby (1 0 0) (-16 -16 -24) (16 16 24) Ambush
*/
void q1_monster_tarbaby(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	gi.modelindex("models/q1/tarbaby.mdl");
	sound_death1 = gi.soundindex("q1/blob/death1.wav");
	sound_hit1 = gi.soundindex("q1/blob/hit1.wav");
	sound_land1 = gi.soundindex("q1/blob/land1.wav");
	sound_sight1 = gi.soundindex("q1/blob/sight1.wav");
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	gi.setmodel(self, "models/q1/tarbaby.mdl");
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 40);
	self->health = 80;
	self->monsterinfo.stand = tbaby_stand;
	self->monsterinfo.walk = tbaby_walk;
	self->monsterinfo.run = tbaby_run;
	self->monsterinfo.attack = tbaby_jump;
	self->monsterinfo.melee = tbaby_jump;
	self->monsterinfo.sight = tbaby_sight;
	self->die = tbaby_die;
	walkmonster_start(self);
	self->monsterinfo.currentmove = &tbaby_stand1;
	self->monsterinfo.scale = 1;
	gi.linkentity(self);
}

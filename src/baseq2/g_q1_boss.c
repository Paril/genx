#include "g_local.h"

enum {
	rise1, rise2, rise3, rise4, rise5, rise6, rise7, rise8, rise9, rise10,
	rise11, rise12, rise13, rise14, rise15, rise16, rise17,

	walk1, walk2, walk3, walk4, walk5, walk6, walk7, walk8,
	walk9, walk10, walk11, walk12, walk13, walk14, walk15,
	walk16, walk17, walk18, walk19, walk20, walk21, walk22,
	walk23, walk24, walk25, walk26, walk27, walk28, walk29, walk30, walk31,

	death1, death2, death3, death4, death5, death6, death7, death8, death9,

	attack1, attack2, attack3, attack4, attack5, attack6, attack7, attack8,
	attack9, attack10, attack11, attack12, attack13, attack14, attack15,
	attack16, attack17, attack18, attack19, attack20, attack21, attack22,
	attack23,

	shocka1, shocka2, shocka3, shocka4, shocka5, shocka6, shocka7, shocka8,
	shocka9, shocka10,

	shockb1, shockb2, shockb3, shockb4, shockb5, shockb6,

	shockc1, shockc2, shockc3, shockc4, shockc5, shockc6, shockc7, shockc8,
	shockc9, shockc10
};

void boss_missile(edict_t *self);
void ai_charge(edict_t *self, float dist);

void boss_face(edict_t *self, float dist)
{
// go for another player if multi player
	if (self->enemy->health <= 0 || random() < 0.02f)
	{
		self->enemy = G_FindByType(self->enemy, ET_PLAYER);
	
		if (!self->enemy)
			self->enemy = G_FindByType(self->enemy, ET_PLAYER);
	}

	ai_charge(self, dist);

	if (self->enemy && self->enemy->health)
		boss_missile(self);
}

static int sound_out1;
static int sound_sight1;
static int sound_throw;
static int sound_death;
static int sound_pain;

void boss_sound_pain(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_pain, 1, ATTN_NORM, 0);
}

void boss_out_sound(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_out1, 1, ATTN_NORM, 0);
}

void boss_sight_sound(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_sight1, 1, ATTN_NORM, 0);
}

mframe_t boss_frames_rise1[] = {
	{ ai_move, 0,   boss_out_sound },
	{ ai_move, 0,   boss_sight_sound },
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
mmove_t boss_rise1 = { rise1, rise17, boss_frames_rise1, boss_missile };

void boss_rise(edict_t *self)
{
	self->monsterinfo.currentmove = &boss_rise1;
}

mframe_t boss_frames_idle1[] = {
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },

	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },

	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },
	{ boss_face, 0,   NULL },

	{ boss_face, 0,   NULL }
};
mmove_t boss_idle1 = { walk1, walk31, boss_frames_idle1, NULL };

void boss_idle(edict_t *self)
{
	self->monsterinfo.currentmove = &boss_idle1;
}

edict_t *fire_spike(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, bool super);
void q1_rocket_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

void boss_fire_missile(edict_t *self)
{
	vec3_t	offang;
	vec3_t	org, vec, d;
	float	t;

	vec3_t p;

	if (self->s.frame == attack9)
		VectorSet(p, 100, 100, 200);
	else
		VectorSet(p, 100, -100, 200);

	VectorSubtract(self->enemy->s.origin, self->s.origin, offang);
	vectoangles(offang, offang);

	vec3_t v_forward, v_right;

	AngleVectors(offang, v_forward, v_right, NULL);

	for (int i = 0; i < 3; ++i)
		org[i] = self->s.origin[i] + p[0]*v_forward[i] + p[1]*v_right[i] + p[2] * ((i == 2) ? 1 : 0);

	// lead the player on hard mode
	if (skill->integer > 1)
	{
		vec3_t tp;
		VectorSubtract(self->enemy->s.origin, org, tp);
		t = VectorLength(tp) / 300;
		VectorCopy(self->enemy->velocity, vec);
		vec[2] = 0;
		VectorMA(self->enemy->s.origin, t, vec, d);
	}
	else
	{
		VectorCopy(self->enemy->s.origin, d);
	}

	VectorSubtract(d, org, vec);
	VectorNormalize(vec);

	edict_t *newmis = fire_spike(self, org, vec, 0, 300, false);
	gi.setmodel(newmis, "models/q1/lavaball.mdl");
	VectorSet(newmis->avelocity, 200, 100, 300);
	newmis->dmg = newmis->dmg_radius = 100;
	newmis->s.effects |= EF_ROCKET;
	newmis->touch = q1_rocket_touch; // rocket explosion
	gi.sound(self, CHAN_WEAPON, sound_throw, 1, ATTN_NORM, 0);

	// check for dead enemy
	if (self->enemy->health <= 0)
		boss_idle(self);
}

mframe_t boss_frames_missile1[] = {
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   boss_fire_missile },
	{ ai_charge, 0,   NULL },

	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   boss_fire_missile },

	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL }
};
mmove_t boss_missile1 = { attack1, attack23, boss_frames_missile1, NULL };

void boss_missile(edict_t *self)
{
	self->monsterinfo.currentmove = &boss_missile1;
}

mframe_t boss_frames_shocka1[] = {
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
mmove_t boss_shocka1 = { shocka1, shocka10, boss_frames_shocka1, boss_missile };

void boss_shocka(edict_t *self)
{
	self->monsterinfo.currentmove = &boss_shocka1;
}

mframe_t boss_frames_shockb2[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t boss_shockb2 = { shockb1, shockb4, boss_frames_shockb2, boss_missile };

void boss_shockbb(edict_t *self)
{
	self->monsterinfo.currentmove = &boss_shockb2;
}

mframe_t boss_frames_shockb1[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t boss_shockb1 = { shockb1, shockb6, boss_frames_shockb1, boss_shockbb };

void boss_shockb(edict_t *self)
{
	self->monsterinfo.currentmove = &boss_shockb1;
}

void boss_death(edict_t *self);

mframe_t boss_frames_shockc1[] = {
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
mmove_t boss_shockc1 = { shockc1, shockc10, boss_frames_shockc1, boss_death };

void boss_shockc(edict_t *self)
{
	self->monsterinfo.currentmove = &boss_shockc1;
}

void boss_really_dead(edict_t *self)
{
	level.killed_monsters++;
	G_UseTargets(self, self);
	G_FreeEdict(self);
}

void boss_dead(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_out1, 1, ATTN_NORM, 0);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_Q1_LAVASPLASH);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_ALL);
}

void boss_death_sound(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
}

mframe_t boss_frames_death1[] = {
	{ ai_move, 0,   boss_death_sound },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   boss_dead }
};
mmove_t boss_death1 = { death1, death9, boss_frames_death1, boss_really_dead };

void boss_death(edict_t *self)
{
	self->monsterinfo.currentmove = &boss_death1;
}

void M_MoveFrame(edict_t *self);

void boss_awake(edict_t *self, edict_t *other, edict_t *activator)
{
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->takedamage = DAMAGE_NO;
	
	gi.setmodel (self, "models/q1/boss.mdl");
	VectorSet(self->mins, -128, -128, -24);
	VectorSet(self->maxs, 128, 128, 256);
	
	if (skill->integer == 0)
		self->health = 1;
	else
		self->health = 3;

	self->enemy = activator;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_Q1_LAVASPLASH);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_ALL);

	self->yaw_speed = 20;
	boss_rise(self);

	self->think = M_MoveFrame;
	self->nextthink = level.time + 100;
	gi.linkentity(self);
}


/*QUAKED monster_boss (1 0 0) (-128 -128 -24) (128 128 256)
*/
void q1_monster_boss(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	gi.modelindex("models/q1/boss.mdl");
	gi.modelindex("models/q1/lavaball.mdl");

	sound_out1 = gi.soundindex("q1/boss1/out1.wav");
	sound_sight1 = gi.soundindex("q1/boss1/sight1.wav");
	sound_throw = gi.soundindex("q1/boss1/throw.wav");
	sound_pain = gi.soundindex("q1/boss1/pain.wav");
	sound_death = gi.soundindex("q1/boss1/death.wav");

	level.total_monsters++;

	self->use = boss_awake;

	self->monsterinfo.attack = boss_missile;
}

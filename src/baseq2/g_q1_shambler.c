/*
==============================================================================

SHAMBLER

==============================================================================
*/

#include "g_local.h"

enum
{
	stand1, stand2, stand3, stand4, stand5, stand6, stand7, stand8, stand9,
	stand10, stand11, stand12, stand13, stand14, stand15, stand16, stand17,

	walk1, walk2, walk3, walk4, walk5, walk6, walk7,
	walk8, walk9, walk10, walk11, walk12,

	run1, run2, run3, run4, run5, run6,

	smash1, smash2, smash3, smash4, smash5, smash6, smash7,
	smash8, smash9, smash10, smash11, smash12,

	swingr1, swingr2, swingr3, swingr4, swingr5,
	swingr6, swingr7, swingr8, swingr9,

	swingl1, swingl2, swingl3, swingl4, swingl5,
	swingl6, swingl7, swingl8, swingl9,

	magic1, magic2, magic3, magic4, magic5,
	magic6, magic7, magic8, magic9, magic10, magic11, magic12,

	pain1, pain2, pain3, pain4, pain5, pain6,

	death1, death2, death3, death4, death5, death6,
	death7, death8, death9, death10, death11
};

mframe_t sham_frames_stand1[] =
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
	{ ai_stand, 0,   NULL }
};
mmove_t sham_stand1 = { stand1, stand17, sham_frames_stand1, NULL };

void sham_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &sham_stand1;
}

static q_soundhandle sound_sidle;
static q_soundhandle sound_smack;
static q_soundhandle sound_melee1;
static q_soundhandle sound_melee2;
static q_soundhandle sound_sattck1;
static q_soundhandle sound_sboom;
static q_soundhandle sound_shurt2;
static q_soundhandle sound_sdeath;
static q_soundhandle sound_ssight;
static q_soundhandle sound_udeath;

void sham_idle_sound(edict_t *self)
{
	if (random() > 0.8f)
		gi.sound(self, CHAN_VOICE, sound_sidle, 1, ATTN_IDLE, 0);
}

mframe_t sham_frames_walk1[] =
{
	{ ai_walk, 10,  NULL },
	{ ai_walk, 9,   NULL },
	{ ai_walk, 9,   NULL },
	{ ai_walk, 5,   NULL },
	{ ai_walk, 6,   NULL },
	{ ai_walk, 12,  NULL },
	{ ai_walk, 8,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 13,  NULL },
	{ ai_walk, 9,   NULL },

	{ ai_walk, 7,   NULL },
	{ ai_walk, 7,   sham_idle_sound }
};
mmove_t sham_walk1 = { walk1, walk12, sham_frames_walk1, NULL };

void sham_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &sham_walk1;
}

mframe_t sham_frames_run1[] =
{
	{ ai_run, 20,  NULL },
	{ ai_run, 24,  NULL },
	{ ai_run, 20,  NULL },
	{ ai_run, 20,  NULL },
	{ ai_run, 24,  NULL },
	{ ai_run, 20,  sham_idle_sound }
};
mmove_t sham_run1 = { run1, run6, sham_frames_run1, NULL };

void sham_run(edict_t *self)
{
	self->monsterinfo.currentmove = &sham_run1;
}

void SpawnMeatSpray(edict_t *self, vec3_t org, vec3_t vel);
void sham_smash_hit(edict_t *self)
{
	vec3_t delta;
	float ldmg;

	if (!self->enemy)
		return;

	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);

	if (VectorLength(delta) > 100)
		return;

	if (!CanDamage(self->enemy, self))
		return;

	ldmg = (random() + random() + random()) * 40;
	T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_MELEE, DT_DIRECT));
	gi.sound(self, CHAN_VOICE, sound_smack, 1, ATTN_NORM, 0);
	vec3_t v, o;
	vec3_t v_forward, v_right;
	AngleVectors(self->s.angles, v_forward, v_right, NULL);
	VectorMA(self->s.origin, 16, v_forward, o);
	VectorScale(v_right, crandom() * 100, v);
	SpawnMeatSpray(self, o, v);
	SpawnMeatSpray(self, o, v);
}

void sham_smash_sound(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_melee1, 1, ATTN_NORM, 0);
}

mframe_t sham_frames_smash1[] =
{
	{ ai_charge, 2,  NULL },
	{ ai_charge, 6,  NULL },
	{ ai_charge, 6,  NULL },
	{ ai_charge, 5,  NULL },
	{ ai_charge, 4,  NULL },
	{ ai_charge, 1,  NULL },
	{ ai_charge, 0,  NULL },
	{ ai_charge, 0,  NULL },
	{ ai_charge, 0,  NULL },
	{ ai_charge, 0,  sham_smash_hit },

	{ ai_charge, 5,  NULL },
	{ ai_charge, 4,  NULL }
};
mmove_t sham_smash1 = { run1, run6, sham_frames_smash1, sham_run };

void sham_smash(edict_t *self)
{
	self->monsterinfo.currentmove = &sham_smash1;
}

void ShamClaw(edict_t *self, float side)
{
	vec3_t delta;
	float ldmg;

	if (!self->enemy)
		return;

	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);

	if (VectorLength(delta) > 100)
		return;

	ldmg = (random() + random() + random()) * 20;
	T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_MELEE, DT_DIRECT));
	gi.sound(self, CHAN_VOICE, sound_smack, 1, ATTN_NORM, 0);

	if (side)
	{
		vec3_t v, o;
		vec3_t v_forward, v_right;
		AngleVectors(self->s.angles, v_forward, v_right, NULL);
		VectorMA(self->s.origin, 16, v_forward, o);
		VectorScale(v_right, side, v);
		SpawnMeatSpray(self, o, v);
	}
}

void sham_swing_sound(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_melee2, 1, ATTN_NORM, 0);
}

extern mmove_t sham_swingr1;

void sham_swing_left(edict_t *self)
{
	ShamClaw(self, 250);
}

void sham_swing_left_re(edict_t *self)
{
	if (random() < 0.5f)
		self->monsterinfo.currentmove = &sham_swingr1;
}

mframe_t sham_frames_swingl1[] =
{
	{ ai_charge, 5,  sham_swing_sound },
	{ ai_charge, 3,  NULL },
	{ ai_charge, 7,  NULL },
	{ ai_charge, 3,  NULL },
	{ ai_charge, 7,  NULL },
	{ ai_charge, 9,  NULL },
	{ ai_charge, 15, sham_swing_left },
	{ ai_charge, 4,  NULL },
	{ ai_charge, 8,  sham_swing_left_re }
};
mmove_t sham_swingl1 = { swingl1, swingl9, sham_frames_swingl1, sham_run };

void sham_swingl(edict_t *self)
{
	self->monsterinfo.currentmove = &sham_swingl1;
}

void sham_swing_right(edict_t *self)
{
	ShamClaw(self, -250);
}

void sham_swing_right_re(edict_t *self)
{
	if (random() < 0.5f)
		self->monsterinfo.currentmove = &sham_swingl1;
}

mframe_t sham_frames_swingr1[] =
{
	{ ai_charge, 1,  sham_smash_sound },
	{ ai_charge, 8,  NULL },
	{ ai_charge, 14,  NULL },
	{ ai_charge, 7,  NULL },
	{ ai_charge, 3,  NULL },
	{ ai_charge, 6,  NULL },
	{ ai_charge, 16, sham_swing_right },
	{ ai_charge, 3,  NULL },
	{ ai_charge, 11,  sham_swing_right_re }
};
mmove_t sham_swingr1 = { swingr1, swingr9, sham_frames_swingr1, sham_run };

void sham_swingr(edict_t *self)
{
	self->monsterinfo.currentmove = &sham_swingr1;
}

void sham_melee(edict_t *self)
{
	float chance;
	chance = random();

	if (chance > 0.6f || self->health == 600)
		sham_smash(self);
	else if (chance > 0.3f)
		sham_swingr(self);
	else
		sham_swingl(self);
}


//============================================================================

void LightningDamage(edict_t *ent, vec3_t p1, vec3_t p2, edict_t *from, int damage);

void CastLightning(edict_t *self)
{
	vec3_t org, dir;
	//self.effects = self.effects | EF_MUZZLEFLASH;
	VectorCopy(self->s.origin, org);
	org[2] += 40;

	for (int i = 0; i < 3; ++i)
		dir[i] = self->enemy->s.origin[i] + ((i == 2) ? 16 : 0) - org[i];

	VectorNormalize(dir);
	vec3_t end;
	VectorMA(self->s.origin, 600, dir, end);
	trace_t tr = gi.trace(org, vec3_origin, vec3_origin, end, self, MASK_SHOT);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_Q1_LIGHTNING1);
	gi.WriteShort(self - g_edicts);
	gi.WritePosition(org);
	gi.WritePosition(tr.endpos);
	gi.multicast(tr.endpos, MULTICAST_PVS);
	LightningDamage(self, org, tr.endpos, self, 10);
}

void sham_magic_sound(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_sattck1, 1, ATTN_NORM, 0);
}

void sham_magic_start(edict_t *self)
{
	if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
	{
		self->monsterinfo.pausetime = level.time + 200;
		edict_t *o;
		//self.effects = self.effects | EF_MUZZLEFLASH;
		o = G_Spawn();
		self->owner = o;
		gi.setmodel(o, "models/q1/s_light.mdl");
		VectorCopy(self->s.origin, o->s.origin);
		VectorCopy(self->s.angles, o->s.angles);
		o->nextthink = level.time + 700;
		o->think = G_FreeEdict;
		gi.linkentity(o);
	}
	else if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void sham_magic_inc(edict_t *self)
{
	self->owner->s.frame++;
}

void sham_fire(edict_t *self)
{
	if (self->s.frame == magic11)
	{
		if (skill->integer == 3)
			CastLightning(self);
	}
	else if (self->s.frame == magic9 || self->s.frame == magic10)
		CastLightning(self);
	else if (self->s.frame == magic6)
	{
		G_FreeEdict(self->owner);
		self->owner = NULL;
		CastLightning(self);
		gi.sound(self, CHAN_WEAPON, sound_sboom, 1, ATTN_NORM, 0);
		self->monsterinfo.nextframe = magic9;
	}
}

mframe_t sham_frames_magic1[] =
{
	{ ai_charge, 0,  sham_magic_sound },
	{ ai_charge, 0,  NULL },
	{ ai_charge, 0,  sham_magic_start },
	{ ai_move, 0,  sham_magic_inc },
	{ ai_move, 0,  sham_magic_inc },
	{ ai_move, 0,  sham_fire },
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  sham_fire },
	{ ai_move, 0,  sham_fire },

	{ ai_move, 0,  sham_fire },
	{ ai_move, 0,  NULL }
};
mmove_t sham_magic1 = { magic1, magic12, sham_frames_magic1, sham_run };

void sham_magic(edict_t *self)
{
	self->monsterinfo.currentmove = &sham_magic1;
}

mframe_t sham_frames_pain1[] =
{
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  NULL }
};
mmove_t sham_pain1 = { pain1, pain6, sham_frames_pain1, sham_run };

void sham_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	gi.sound(self, CHAN_VOICE, sound_shurt2, 1, ATTN_NORM, 0);

	if (self->health <= 0)
		return;		// allready dying, don't go into pain frame

	if (random() * 400 > damage)
		return;		// didn't flinch

	if (self->pain_debounce_time > level.time)
		return;

	self->pain_debounce_time = level.time + 2000;
	self->monsterinfo.currentmove = &sham_pain1;
}


//============================================================================

void sham_unsolid(edict_t *self)
{
	self->solid = SOLID_NOT;
	gi.linkentity(self);
}

void sham_dead(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
}

mframe_t sham_frames_death1[] =
{
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  sham_unsolid },
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  NULL },
	{ ai_move, 0,  NULL },

	{ ai_move, 0,  NULL }
};
mmove_t sham_death1 = { death1, death11, sham_frames_death1, sham_dead };

void sham_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// check for gib
	if (self->health < -60)
	{
		gi.sound(self, CHAN_VOICE, sound_udeath, 1, ATTN_NORM, 0);
		ThrowHead(self, "models/q1/h_shams.mdl", self->health, GIB_Q1);
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
	gi.sound(self, CHAN_VOICE, sound_sdeath, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &sham_death1;
}

//============================================================================

void sham_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_ssight, 1, ATTN_NORM, 0);
}

/*QUAKED monster_shambler (1 0 0) (-32 -32 -24) (32 32 64) Ambush
*/
void q1_monster_shambler(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	gi.modelindex("models/q1/s_light.mdl");
	gi.modelindex("models/q1/h_shams.mdl");
	gi.modelindex("models/q1/bolt.mdl");
	sound_sattck1 = gi.soundindex("q1/shambler/sattck1.wav");
	sound_sboom = gi.soundindex("q1/shambler/sboom.wav");
	sound_sdeath = gi.soundindex("q1/shambler/sdeath.wav");
	sound_shurt2 = gi.soundindex("q1/shambler/shurt2.wav");
	sound_sidle = gi.soundindex("q1/shambler/sidle.wav");
	sound_ssight = gi.soundindex("q1/shambler/ssight.wav");
	sound_melee1 = gi.soundindex("q1/shambler/melee1.wav");
	sound_melee2 = gi.soundindex("q1/shambler/melee2.wav");
	sound_smack = gi.soundindex("q1/shambler/smack.wav");
	sound_udeath = gi.soundindex("q1/player/udeath.wav");
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	gi.setmodel(self, "models/q1/shambler.mdl");
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, 64);
	self->health = 600;
	self->monsterinfo.stand = sham_stand;
	self->monsterinfo.walk = sham_walk;
	self->monsterinfo.run = sham_run;
	self->die = sham_die;
	self->monsterinfo.melee = sham_melee;
	self->monsterinfo.attack = sham_magic;
	self->monsterinfo.sight = sham_sight;
	self->pain = sham_pain;
	walkmonster_start(self);
	gi.linkentity(self);
	self->monsterinfo.currentmove = &sham_stand1;
	self->monsterinfo.scale = 1;
}

/*
==============================================================================

OGRE

==============================================================================
*/

#include "g_local.h"

enum {
	stand1, stand2, stand3, stand4, stand5, stand6, stand7, stand8, stand9,

	walk1, walk2, walk3, walk4, walk5, walk6, walk7,
	walk8, walk9, walk10, walk11, walk12, walk13, walk14, walk15, walk16,

	run1, run2, run3, run4, run5, run6, run7, run8,

	swing1, swing2, swing3, swing4, swing5, swing6, swing7,
	swing8, swing9, swing10, swing11, swing12, swing13, swing14,

	smash1, smash2, smash3, smash4, smash5, smash6, smash7,
	smash8, smash9, smash10, smash11, smash12, smash13, smash14,

	shoot1, shoot2, shoot3, shoot4, shoot5, shoot6,

	pain1, pain2, pain3, pain4, pain5,

	painb1, painb2, painb3,

	painc1, painc2, painc3, painc4, painc5, painc6,

	paind1, paind2, paind3, paind4, paind5, paind6, paind7, paind8, paind9, paind10,
	paind11, paind12, paind13, paind14, paind15, paind16,

	paine1, paine2, paine3, paine4, paine5, paine6, paine7, paine8, paine9, paine10,
	paine11, paine12, paine13, paine14, paine15,

	death1, death2, death3, death4, death5, death6,
	death7, death8, death9, death10, death11, death12,
	death13, death14,

	bdeath1, bdeath2, bdeath3, bdeath4, bdeath5, bdeath6,
	bdeath7, bdeath8, bdeath9, bdeath10,

	pull1, pull2, pull3, pull4, pull5, pull6, pull7, pull8, pull9, pull10, pull11
};

//=============================================================================


void OgreGrenadeExplode(edict_t *self)
{
	T_RadiusDamage(self, self->owner, 40, NULL, DAMAGE_Q1, 40, self->meansOfDeath);

	gi.sound(self, CHAN_VOICE, gi.soundindex("q1/weapons/r_exp3.wav"), 1, ATTN_NORM, 0);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_Q1_EXPLODE);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	G_FreeEdict(self);
}

void OgreGrenadeTouch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;		// don't explode on owner

	if (other->takedamage == DAMAGE_AIM)
	{
		OgreGrenadeExplode(self);
		return;
	}

	gi.sound(self, CHAN_VOICE, gi.soundindex("q1/weapons/bounce.wav"), 1, ATTN_NORM, 0);
	if (VectorEmpty(self->velocity))
		VectorClear(self->avelocity);
}

/*
================
OgreFireGrenade
================
*/
void OgreFireGrenade(edict_t *self)
{
	//self.effects = self.effects | EF_MUZZLEFLASH;

	gi.sound(self, CHAN_VOICE, gi.soundindex("q1/weapons/grenade.wav"), 1, ATTN_NORM, 0);

	edict_t *missile = G_Spawn();
	missile->owner = self;
	missile->movetype = MOVETYPE_BOUNCE;
	missile->solid = SOLID_BBOX;
	missile->clipmask = MASK_SHOT;
	missile->s.game = GAME_Q1;
	missile->s.effects |= EF_GRENADE;
	missile->entitytype = ET_GRENADE;
	VectorClear(missile->mins);
	VectorClear(missile->maxs);

	// set missile speed	

	vec3_t dir;
	VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
	VectorNormalize(dir);
	VectorScale(dir, 600, missile->velocity);
	missile->velocity[2] = 200;

	missile->avelocity[0] = missile->avelocity[1] = missile->avelocity[2] = 300;

	vectoangles(missile->velocity, missile->s.angles);

	missile->touch = OgreGrenadeTouch;

	// set missile duration
	missile->nextthink = level.time + 2500;
	missile->think = OgreGrenadeExplode;

	missile->s.modelindex = gi.modelindex("models/q1/grenade.mdl");
	VectorCopy(self->s.origin, missile->s.origin);

	missile->meansOfDeath = MakeAttackerMeansOfDeath(self, missile, MD_NONE, DT_DIRECT);

	gi.linkentity(missile);
}


//=============================================================================


/*
================
SpawnMeatSpray
================
*/
void SpawnMeatSpray(edict_t *self, vec3_t org, vec3_t vel)
{
	edict_t *missile = G_Spawn();
	missile->owner = self;
	missile->movetype = MOVETYPE_BOUNCE;
	missile->solid = SOLID_NOT;

	VectorCopy(vel, missile->velocity);
	missile->velocity[2] = missile->velocity[2] + 250 + 50 * random();

	VectorSet(missile->avelocity, 3000, 1000, 2000);

	// set missile duration
	missile->nextthink = level.time + 1000;
	missile->think = G_FreeEdict;
	missile->s.game = GAME_Q1;
	missile->s.effects |= EF_GIB;

	missile->s.modelindex = gi.modelindex("models/q1/zom_gib.mdl");
	VectorClear(missile->mins);
	VectorClear(missile->maxs);
	VectorCopy(org, missile->s.origin);

	gi.linkentity(missile);
}

/*
================
chainsaw

FIXME
================
*/
void chainsaw(edict_t *self, float side)
{
	vec3_t delta;
	float ldmg;

	if (!self->enemy)
		return;
	if (!CanDamage(self->enemy, self))
		return;

	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);

	if (VectorLength(delta) > 100)
		return;

	ldmg = (random() + random() + random()) * 4;
	T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_MELEE, DT_DIRECT));

	if (side)
	{
		vec3_t v_forward, v_right;

		AngleVectors(self->s.angles, v_forward, v_right, NULL);

		vec3_t sp;
		VectorMA(self->s.origin, 16, v_forward, sp);

		if (side == 1)
		{
			VectorScale(v_right, crandom() * 100, v_right);
			SpawnMeatSpray(self, sp, v_right);
		}
		else
		{
			VectorScale(v_right, side, v_right);
			SpawnMeatSpray(self, sp, v_right);
		}
	}
}

static int sound_ogidle;
static int sound_ogdrag;
static int sound_ogidle2;
static int sound_ogsawatk;
static int sound_ogpain1;
static int sound_udeath;
static int sound_ogdth;
static int sound_ogwake;

void ogre_idle_sound(edict_t *self)
{
	if (random() < 0.2f)
		gi.sound(self, CHAN_VOICE, sound_ogidle, 1, ATTN_IDLE, 0);
}

mframe_t ogre_frames_stand1[] = {
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
mmove_t ogre_stand1 = { stand1, stand9, ogre_frames_stand1, NULL };

void ogre_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_stand1;
}

void ogre_drag_sound(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_ogdrag, 1, ATTN_IDLE, 0);
}

void ogre_drag(edict_t *self)
{
	if (random() < 0.1f)
		ogre_drag_sound(self);
}

mframe_t ogre_frames_walk1[] = {
	{ ai_walk, 3,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   ogre_idle_sound },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 0,   ogre_drag },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 1,   NULL },
	{ ai_walk, 2,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 3,   NULL },
	{ ai_walk, 4,   NULL }
};
mmove_t ogre_walk1 = { walk1, walk16, ogre_frames_walk1, NULL };

void ogre_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_walk1;
}

void ogre_idle2_sound(edict_t *self)
{
	if (random() < 0.2f)
		gi.sound(self, CHAN_VOICE, sound_ogidle2, 1, ATTN_IDLE, 0);
}

mframe_t ogre_frames_run1[] = {
	{ ai_run, 9,   ogre_idle2_sound },
	{ ai_run, 12,   NULL },
	{ ai_run, 8,   NULL },
	{ ai_run, 22,   NULL },
	{ ai_run, 16,   NULL },
	{ ai_run, 4,   NULL },
	{ ai_run, 13,   NULL },
	{ ai_run, 24,   NULL }
};
mmove_t ogre_run1 = { run1, run8, ogre_frames_run1, NULL };

void ogre_run(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_run1;
}

void ogre_swing_sound(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_ogsawatk, 1, ATTN_NORM, 0);
}

void ogre_swing_attack(edict_t *self)
{
	if (self->s.frame == swing6)
		chainsaw(self, 200);
	else if (self->s.frame == swing10)
		chainsaw(self, -200);
	else
		chainsaw(self, 0);
	self->s.angles[1] = self->s.angles[1] + random() * 25;
}

mframe_t ogre_frames_swing1[] = {
	{ ai_charge, 11,   ogre_swing_sound },
	{ ai_charge, 1,   NULL },
	{ ai_charge, 4,   NULL },
	{ ai_charge, 13,   NULL },
	{ ai_charge, 19,   ogre_swing_attack },
	{ ai_charge, 10,   ogre_swing_attack },
	{ ai_charge, 10,   ogre_swing_attack },
	{ ai_charge, 10,   ogre_swing_attack },
	{ ai_charge, 10,   ogre_swing_attack },
	{ ai_charge, 10,   ogre_swing_attack },
	{ ai_charge, 10,   ogre_swing_attack },
	{ ai_charge, 3,   NULL },
	{ ai_charge, 8,   NULL },
	{ ai_charge, 9,   NULL }
};
mmove_t ogre_swing1 = { swing1, swing14, ogre_frames_swing1, ogre_run };

void ogre_swing(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_swing1;
}

void ogre_smash_attack(edict_t *self)
{
	if (self->s.frame == smash10)
		chainsaw(self, 1);
	else
		chainsaw(self, 0);

	if (self->s.frame == smash11)
		self->monsterinfo.pausetime = level.time + (random() * 200);
}

mframe_t ogre_frames_smash1[] = {
	{ ai_charge, 6,   ogre_swing_sound },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 1,   NULL },
	{ ai_charge, 4,   NULL },
	{ ai_charge, 14,   ogre_smash_attack },
	{ ai_charge, 14,   ogre_smash_attack },
	{ ai_charge, 20,   ogre_smash_attack },
	{ ai_charge, 23,   ogre_smash_attack },
	{ ai_charge, 10,   ogre_smash_attack },
	{ ai_charge, 12,   ogre_smash_attack },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 4,   NULL },
	{ ai_charge, 12,   NULL }
};
mmove_t ogre_smash1 = { smash1, smash14, ogre_frames_smash1, ogre_run };

void ogre_smash(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_smash1;
}

mframe_t ogre_frames_nail1[] = {
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   OgreFireGrenade },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL },
	{ ai_charge, 0,   NULL }
};
mmove_t ogre_nail1 = { shoot1, shoot6, ogre_frames_nail1, ogre_run };

void ogre_nail(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_nail1;
}

mframe_t ogre_frames_pain1[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t ogre_pain1 = { pain1, pain5, ogre_frames_pain1, ogre_run };

void ogre_paina(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_pain1;
}

mframe_t ogre_frames_painb1[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t ogre_painb1 = { painb1, painb3, ogre_frames_painb1, ogre_run };

void ogre_painb(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_painb1;
}

mframe_t ogre_frames_painc1[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t ogre_painc1 = { painc1, painc6, ogre_frames_painc1, ogre_run };

void ogre_painc(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_painc1;
}

mframe_t ogre_frames_paind1[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, -10,   NULL },
	{ ai_move, -9,   NULL },
	{ ai_move, -4,   NULL },
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
mmove_t ogre_paind1 = { paind1, paind16, ogre_frames_paind1, ogre_run };

void ogre_paind(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_paind1;
}

mframe_t ogre_frames_paine1[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, -10,   NULL },
	{ ai_move, -9,   NULL },
	{ ai_move, -4,   NULL },
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
mmove_t ogre_paine1 = { paine1, paine15, ogre_frames_paine1, ogre_run };

void ogre_paine(edict_t *self)
{
	self->monsterinfo.currentmove = &ogre_paine1;
}

void ogre_pain(edict_t *self, edict_t *attacker, float kick, int damage)
{
	// don't make multiple pain sounds right after each other
	if (self->pain_debounce_time > level.time)
		return;

	gi.sound(self, CHAN_VOICE, sound_ogpain1, 1, ATTN_NORM, 0);

	float r = random();

	if (r < 0.25f)
	{
		ogre_paina(self);
		self->pain_debounce_time = level.time + 1000;
	}
	else if (r < 0.5f)
	{
		ogre_painb(self);
		self->pain_debounce_time = level.time + 1000;
	}
	else if (r < 0.75f)
	{
		ogre_painc(self);
		self->pain_debounce_time = level.time + 1000;
	}
	else if (r < 0.88f)
	{
		ogre_paind(self);
		self->pain_debounce_time = level.time + 2000;
	}
	else
	{
		ogre_paine(self);
		self->pain_debounce_time = level.time + 2000;
	}
}

edict_t *Drop_Backpack(edict_t *self);

void ogre_drop(edict_t *self)
{
	self->solid = SOLID_NOT;
	//edict_t *backpack = Drop_Backpack(self);
	//backpack->pack_rockets = 2;
	gi.linkentity(self);
}

void ogre_dead(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
}

mframe_t ogre_frames_deatha1[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   ogre_drop },
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
mmove_t ogre_deatha1 = { death1, death14, ogre_frames_deatha1, ogre_dead };

mframe_t ogre_frames_deathb1[] = {
	{ ai_move, 0,   NULL },
	{ ai_move, 5,   NULL },
	{ ai_move, 0,   ogre_drop },
	{ ai_move, 1,   NULL },
	{ ai_move, 3,   NULL },
	{ ai_move, 7,   NULL },
	{ ai_move, 25,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL },
	{ ai_move, 0,   NULL }
};
mmove_t ogre_deathb1 = { bdeath1, bdeath10, ogre_frames_deathb1, ogre_dead };

void ogre_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// check for gib
	if (self->health < -80)
	{
		gi.sound(self, CHAN_VOICE, sound_udeath, 1, ATTN_NORM, 0);
		ThrowHead(self, "models/q1/h_ogre.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib3.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib3.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib3.mdl", self->health, GIB_Q1);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag)
		return;

	self->deadflag = DEAD_DEAD;
	
	gi.sound(self, CHAN_VOICE, sound_ogdth, 1, ATTN_NORM, 0);

	if (random() < 0.5f)
		self->monsterinfo.currentmove = &ogre_deatha1;
	else
		self->monsterinfo.currentmove = &ogre_deathb1;
}

void ogre_melee(edict_t *self)
{
	if (random() > 0.5f)
		ogre_smash(self);
	else
		ogre_swing(self);
}

void ogre_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_ogwake, 1, ATTN_NORM, 0);
}

void ogre_attack(edict_t *self)
{
	if (range(self, self->enemy) == RANGE_MELEE)
		ogre_melee(self);
	else
		ogre_nail(self);
}

/*QUAKED monster_ogre (1 0 0) (-32 -32 -24) (32 32 64) Ambush

*/
void q1_monster_ogre(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	gi.modelindex("models/q1/ogre.mdl");
	gi.modelindex("models/q1/h_ogre.mdl");
	gi.modelindex("models/q1/grenade.mdl");

	sound_ogdrag = gi.soundindex("q1/ogre/ogdrag.wav");
	sound_ogdth = gi.soundindex("q1/ogre/ogdth.wav");
	sound_ogidle = gi.soundindex("q1/ogre/ogidle.wav");
	sound_ogidle2 = gi.soundindex("q1/ogre/ogidle2.wav");
	sound_ogpain1 = gi.soundindex("q1/ogre/ogpain1.wav");
	sound_ogsawatk = gi.soundindex("q1/ogre/ogsawatk.wav");
	sound_ogwake = gi.soundindex("q1/ogre/ogwake.wav");
	sound_udeath = gi.soundindex("q1/player/udeath.wav");		// gib death

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;

	gi.setmodel(self, "models/q1/ogre.mdl");

	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, 64);

	self->health = 200;

	self->monsterinfo.stand = ogre_stand;
	self->monsterinfo.walk = ogre_walk;
	self->monsterinfo.run = ogre_run;
	self->monsterinfo.attack = ogre_attack;
	self->monsterinfo.melee = ogre_melee;
	self->monsterinfo.sight = ogre_sight;
	self->pain = ogre_pain;
	self->die = ogre_die;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &ogre_stand1;
	self->monsterinfo.scale = 1;

	walkmonster_start(self);
}


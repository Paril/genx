/*
==============================================================================

ZOMBIE

==============================================================================
*/
#include "g_local.h"
#include "m_local.h"

enum
{
	stand1, stand2, stand3, stand4, stand5, stand6, stand7, stand8,
	stand9, stand10, stand11, stand12, stand13, stand14, stand15,

	walk1, walk2, walk3, walk4, walk5, walk6, walk7, walk8, walk9, walk10, walk11,
	walk12, walk13, walk14, walk15, walk16, walk17, walk18, walk19,

	run1, run2, run3, run4, run5, run6, run7, run8, run9, run10, run11, run12,
	run13, run14, run15, run16, run17, run18,

	atta1, atta2, atta3, atta4, atta5, atta6, atta7, atta8, atta9, atta10, atta11,
	atta12, atta13,

	attb1, attb2, attb3, attb4, attb5, attb6, attb7, attb8, attb9, attb10, attb11,
	attb12, attb13, attb14,

	attc1, attc2, attc3, attc4, attc5, attc6, attc7, attc8, attc9, attc10, attc11,
	attc12,

	paina1, paina2, paina3, paina4, paina5, paina6, paina7, paina8, paina9, paina10,
	paina11, paina12,

	painb1, painb2, painb3, painb4, painb5, painb6, painb7, painb8, painb9, painb10,
	painb11, painb12, painb13, painb14, painb15, painb16, painb17, painb18, painb19,
	painb20, painb21, painb22, painb23, painb24, painb25, painb26, painb27, painb28,

	painc1, painc2, painc3, painc4, painc5, painc6, painc7, painc8, painc9, painc10,
	painc11, painc12, painc13, painc14, painc15, painc16, painc17, painc18,

	paind1, paind2, paind3, paind4, paind5, paind6, paind7, paind8, paind9, paind10,
	paind11, paind12, paind13,

	paine1, paine2, paine3, paine4, paine5, paine6, paine7, paine8, paine9, paine10,
	paine11, paine12, paine13, paine14, paine15, paine16, paine17, paine18, paine19,
	paine20, paine21, paine22, paine23, paine24, paine25, paine26, paine27, paine28,
	paine29, paine30,

	cruc_1, cruc_2, cruc_3, cruc_4, cruc_5, cruc_6
};

#define SPAWN_CRUCIFIED	1

static q_soundhandle sound_z_idle;
static q_soundhandle sound_z_idle1;
static q_soundhandle sound_z_shot1;
static q_soundhandle sound_z_gib;
static q_soundhandle sound_z_pain;
static q_soundhandle sound_z_pain1;
static q_soundhandle sound_z_fall;
static q_soundhandle sound_z_miss;
static q_soundhandle sound_z_hit;
static q_soundhandle sound_idle_w2;

static mscript_t script;

//=============================================================================

static void zombie_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
}

static void zombie_walk_sound(edict_t *self)
{
	if (random() < 0.2f)
		gi.sound(self, CHAN_VOICE, sound_z_idle, 1, ATTN_IDLE, 0);
}

static void zombie_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk1");
}

/*
=============================================================================

ATTACKS

=============================================================================
*/

static void zombie_reset_pain(edict_t *self)
{
	self->count = 0;
}

static void zombie_run_sound(edict_t *self)
{
	if (random() < 0.2f)
		gi.sound(self, CHAN_VOICE, sound_z_idle, 1, ATTN_IDLE, 0);
	else if (random() > 0.8f)
		gi.sound(self, CHAN_VOICE, sound_z_idle1, 1, ATTN_IDLE, 0);
}

static void zombie_run(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run1");
}

static void ZombieGrenadeTouch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;		// don't explode on owner

	if (other->takedamage)
	{
		T_Damage(other, self, self->owner, vec3_origin, vec3_origin, vec3_origin, 10, 0, DAMAGE_NO_PARTICLES | DAMAGE_Q1, self->meansOfDeath);
		gi.sound(self, CHAN_WEAPON, sound_z_hit, 1, ATTN_NORM, 0);
		G_FreeEdict(self);
		return;
	}

	gi.sound(self, CHAN_WEAPON, sound_z_miss, 1, ATTN_NORM, 0);	// bounce sound
	G_FreeEdict(self);
}

/*
================
ZombieFireGrenade
================
*/
static void ZombieFireGrenade(edict_t *self)
{
	edict_t *missile;
	vec3_t org;
	vec3_t start;

	if (self->s.frame == atta13)
		VectorSet(start, -10, 22, 30);
	else if (self->s.frame == attb14)
		VectorSet(start, -10, -13, 29);
	else
		VectorSet(start, -12, 19, 29);

	gi.sound(self, CHAN_WEAPON, sound_z_shot1, 1, ATTN_NORM, 0);
	missile = G_Spawn();
	missile->owner = self;
	missile->movetype = MOVETYPE_BOUNCE;
	missile->solid = SOLID_BBOX;
	missile->s.game = GAME_Q1;
	missile->s.effects |= EF_GIB;
	missile->clipmask = MASK_SHOT;
	// calc org
	vec3_t v_forward, v_right, v_up;
	AngleVectors(self->s.angles, v_forward, v_right, v_up);

	for (int i = 0; i < 3; ++i)
		org[i] = self->s.origin[i] + start[0] * v_forward[i] + start[1] * v_right[i] + (start[2] - 24) * v_up[i];

	// set missile speed
	VectorSubtract(self->enemy->s.origin, org, missile->velocity);
	VectorNormalize(missile->velocity);
	VectorScale(missile->velocity, 600, missile->velocity);
	missile->velocity[2] = 200;
	VectorSet(missile->avelocity, 3000, 1000, 2000);
	missile->touch = ZombieGrenadeTouch;
	// set missile duration
	missile->nextthink = level.time + 2500;
	missile->think = G_FreeEdict;
	missile->s.modelindex = gi.modelindex("models/q1/zom_gib.mdl");
	VectorClear(missile->mins);
	VectorClear(missile->maxs);
	VectorCopy(org, missile->s.origin);
	missile->entitytype = ET_Q1_ZOMBIE_GIB;
	missile->meansOfDeath = MakeAttackerMeansOfDeath(self, missile, MD_NONE, DT_DIRECT);
	gi.linkentity(missile);
}

static void zombie_atta(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "atta1");
}

static void zombie_attb(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attb1");
}

static void zombie_attc(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attc1");
}

static void zombie_missile(edict_t *self)
{
	float r = random();

	if (r < 0.3f)
		zombie_atta(self);
	else if (r < 0.6f)
		zombie_attb(self);
	else
		zombie_attc(self);
}


/*
=============================================================================

PAIN

=============================================================================
*/

static void zombie_paind1_start(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_z_pain, 1, ATTN_NORM, 0);
}

static void zombie_paina(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "paina1");
}

static void zombie_painb1_fall(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_z_fall, 1, ATTN_NORM, 0);
}

static void zombie_painc1_start(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_z_pain1, 1, ATTN_NORM, 0);
}

static void zombie_painb(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "painb1");
}

static void zombie_painc(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "painc1");
}

static void zombie_paind(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "paind1");
}

static void zombie_paine1_start(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_z_pain, 1, ATTN_NORM, 0);
	self->health = 60;
}

static void zombie_paine1_fall(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_z_fall, 1, ATTN_NORM, 0);
	self->solid = SOLID_NOT;
	self->takedamage = DAMAGE_NO;
	gi.linkentity(self);
}

static void zombie_paine1_pause(edict_t *self)
{
	if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
		self->monsterinfo.pausetime = level.time + 5000;

	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;

	self->health = 60;
}

static void zombie_paine1_check(edict_t *self)
{
	// see if ok to stand up
	self->health = 60;
	gi.sound(self, CHAN_VOICE, sound_z_idle, 1, ATTN_NORM, 0);

	if (!M_walkmove(self, 0, 0))
	{
		self->monsterinfo.nextframe = paine11;
		return;
	}

	self->solid = SOLID_BBOX;
	self->takedamage = DAMAGE_YES;
	gi.linkentity(self);
}

static void zombie_paine(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "paine1");
}

static void zombie_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.sound(self, CHAN_VOICE, sound_z_gib, 1, ATTN_NORM, 0);
	ThrowHead(self, "models/q1/h_zombie.mdl", self->health, GIB_Q1);
	ThrowGib(self, "models/q1/gib1.mdl", self->health, GIB_Q1);
	ThrowGib(self, "models/q1/gib2.mdl", self->health, GIB_Q1);
	ThrowGib(self, "models/q1/gib3.mdl", self->health, GIB_Q1);
	self->deadflag = DEAD_DEAD;
}

/*
=================
zombie_pain

Zombies can only be killed (gibbed) by doing 60 hit points of damage
in a single frame (rockets, grenades, quad shotgun, quad nailgun).

A hit of 25 points or more (super shotgun, quad nailgun) will allways put it
down to the ground.

A hit of from 10 to 40 points in one frame will cause it to go down if it
has been twice in two seconds, otherwise it goes into one of the four
fast pain frames.

A hit of less than 10 points of damage (winged by a shotgun) will be ignored.

FIXME: don't use pain_finished because of nightmare hack
=================
*/
static void zombie_pain(edict_t *self, edict_t *attacker, float kick, int take)
{
	float r;
	self->health = 60;		// allways reset health

	if (take < 9)
		return;				// totally ignore

	if (self->count == 2)
		return;			// down on ground, so don't reset any counters

	// go down immediately if a big enough hit
	if (take >= 25)
	{
		self->count = 2;
		zombie_paine(self);
		return;
	}

	if (self->count)
	{
		// if hit again in next gre seconds while not in pain frames, definately drop
		self->pain_debounce_time = level.time + 3000;
		return;			// currently going through an animation, don't change
	}

	if (self->pain_debounce_time > level.time)
	{
		// hit again, so drop down
		self->count = 2;
		zombie_paine(self);
		return;
	}

	// gp into one of the fast pain animations
	self->count = 1;
	r = random();

	if (r < 0.25f)
		zombie_paina(self);
	else if (r <  0.5f)
		zombie_painb(self);
	else if (r <  0.75f)
		zombie_painc(self);
	else
		zombie_paind(self);
}

static void zombie_cruc(edict_t *self)
{
	if (self->s.frame == cruc_1)
	{
		vec3_t soundpos, fwd;
		AngleVectors(self->s.angles, fwd, NULL, NULL);
		VectorMA(self->s.origin, 14, fwd, soundpos);

		if (random() < 0.1f)
			gi.positioned_sound(soundpos, self, CHAN_VOICE, sound_idle_w2, 1, ATTN_STATIC, 0);
	}

	self->s.frame++;

	if (self->s.frame > cruc_6)
		self->s.frame = cruc_1;

	self->nextthink = level.time + 100 + random() * 100;
}

static void zombie_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_BODY, sound_z_idle, 1, ATTN_NORM, 0);
}
//============================================================================

static const mevent_t events[] =
{
	EVENT_FUNC(zombie_walk_sound),
	EVENT_FUNC(zombie_reset_pain),
	EVENT_FUNC(zombie_run_sound),
	EVENT_FUNC(zombie_run),
	EVENT_FUNC(ZombieFireGrenade),
	EVENT_FUNC(zombie_paind1_start),
	EVENT_FUNC(zombie_painc1_start),
	EVENT_FUNC(zombie_painb1_fall),
	EVENT_FUNC(zombie_paine1_start),
	EVENT_FUNC(zombie_paine1_fall),
	EVENT_FUNC(zombie_paine1_pause),
	EVENT_FUNC(zombie_paine1_check),
	NULL
};

/*QUAKED monster_zombie (1 0 0) (-16 -16 -24) (16 16 32) Crucified ambush

If crucified, stick the bounding box 12 pixels back into a wall to look right.
*/
void q1_monster_zombie(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/q1/zombie.mdl";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q1/zombie.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	gi.modelindex("models/q1/zombie.mdl");
	gi.modelindex("models/q1/h_zombie.mdl");
	gi.modelindex("models/q1/zom_gib.mdl");
	sound_z_idle = gi.soundindex("q1/zombie/z_idle.wav");
	sound_z_idle1 = gi.soundindex("q1/zombie/z_idle1.wav");
	sound_z_shot1 = gi.soundindex("q1/zombie/z_shot1.wav");
	sound_z_gib = gi.soundindex("q1/zombie/z_gib.wav");
	sound_z_pain = gi.soundindex("q1/zombie/z_pain.wav");
	sound_z_pain1 = gi.soundindex("q1/zombie/z_pain1.wav");
	sound_z_fall = gi.soundindex("q1/zombie/z_fall.wav");
	sound_z_miss = gi.soundindex("q1/zombie/z_miss.wav");
	sound_z_hit = gi.soundindex("q1/zombie/z_hit.wav");
	sound_idle_w2 = gi.soundindex("q1/zombie/idle_w2.wav");
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->s.modelindex = gi.modelindex(model_name);

	if (self->spawnflags & SPAWN_CRUCIFIED)
	{
		self->movetype = MOVETYPE_NONE;
		self->solid = SOLID_NOT;
		self->think = zombie_cruc;
		self->s.frame = cruc_1;
		self->nextthink = level.time + 1;
	}
	else
	{
		VectorSet(self->mins, -16, -16, -24);
		VectorSet(self->maxs, 16, 16, 40);
		self->health = 60;
		self->monsterinfo.stand = zombie_stand;
		self->monsterinfo.walk = zombie_walk;
		self->monsterinfo.run = zombie_run;
		self->pain = zombie_pain;
		self->die = zombie_die;
		self->monsterinfo.attack = zombie_missile;
		self->monsterinfo.sight = zombie_sight;
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
		self->monsterinfo.scale = 1;
		walkmonster_start(self);
	}

	gi.linkentity(self);
}

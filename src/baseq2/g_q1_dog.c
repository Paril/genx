/*
==============================================================================

DOG

==============================================================================
*/
#include "g_local.h"
#include "m_local.h"

static mscript_t script;

enum
{
	attack1, attack2, attack3, attack4, attack5, attack6, attack7, attack8,

	death1, death2, death3, death4, death5, death6, death7, death8, death9,

	deathb1, deathb2, deathb3, deathb4, deathb5, deathb6, deathb7, deathb8,
	deathb9,

	pain1, pain2, pain3, pain4, pain5, pain6,

	painb1, painb2, painb3, painb4, painb5, painb6, painb7, painb8, painb9, painb10,
	painb11, painb12, painb13, painb14, painb15, painb16,

	run1, run2, run3, run4, run5, run6, run7, run8, run9, run10, run11, run12,

	leap1, leap2, leap3, leap4, leap5, leap6, leap7, leap8, leap9,

	stand1, stand2, stand3, stand4, stand5, stand6, stand7, stand8, stand9,

	walk1, walk2, walk3, walk4, walk5, walk6, walk7, walk8
};

static q_soundhandle sound_idle;
static q_soundhandle sound_dattack1;
static q_soundhandle sound_dpain1;
static q_soundhandle sound_udeath;
static q_soundhandle sound_ddeath;
static q_soundhandle sound_dsight;

static void dog_leap(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "leap1");
}

static void Dog_JumpTouch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (self->health <= 0)
		return;

	if (other->takedamage)
	{
		if (VectorLength(self->velocity) > 300)
		{
			float ldmg = 10 + 10 * random();
			T_Damage(other, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_NONE, MakeAttackerMeansOfDeath(self, self, MD_NONE, DT_DIRECT));
		}
	}

	if (!M_CheckBottom(self))
	{
		if (self->groundentity)
		{
			self->touch = NULL;
			dog_leap(self);
		}

		return;	// not on ground yet
	}

	self->monsterinfo.pausetime = 0;
	self->touch = NULL;
}

static void dog_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
}

static void dog_idle_sound(edict_t *self)
{
	if (random() < 0.2f)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void dog_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk1");
}

static void dog_run(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run1");
}

static void dog_bite(edict_t *self)
{
	if (!self->enemy)
		return;

	if (!CanDamage(self->enemy, self))
		return;

	vec3_t delta;
	VectorSubtract(self->s.origin, self->enemy->s.origin, delta);

	if (VectorLength(delta) > 100)
		return;

	float ldmg = (random() + random() + random()) * 8;
	T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_NONE, MakeAttackerMeansOfDeath(self, self, MD_MELEE, DT_DIRECT));
}

static void dog_bite_func(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_dattack1, 1, ATTN_NORM, 0);
	dog_bite(self);
}

static void dog_atta(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "atta1");
}

static void dog_leap_func(edict_t *self)
{
	self->touch = Dog_JumpTouch;
	vec3_t v_forward;
	AngleVectors(self->s.angles, v_forward, NULL, NULL);
	self->s.origin[2] = self->s.origin[2] + 1;
	self->groundentity = NULL;
	VectorScale(v_forward, 300, self->velocity);
	self->velocity[2] += 200;
	self->monsterinfo.pausetime = GTIME_MAX;
}

static void dog_pain(edict_t *self, edict_t *attacker, float kick, int damage)
{
	gi.sound(self, CHAN_VOICE, sound_dpain1, 1, ATTN_NORM, 0);

	if (random() > 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "painb1");
}

static void dog_dead(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
}

static void dog_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// check for gib
	if (self->health < -35)
	{
		gi.sound(self, CHAN_VOICE, sound_udeath, 1, ATTN_NORM, 0);
		ThrowHead(self, "models/q1/h_dog.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib3.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib3.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib3.mdl", self->health, GIB_Q1);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;
	// regular death
	gi.sound(self, CHAN_VOICE, sound_ddeath, 1, ATTN_NORM, 0);
	self->solid = SOLID_NOT;
	gi.linkentity(self);

	if (random() > 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "die1");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "dieb1");
}

//============================================================================

/*
==============
CheckDogMelee

Returns TRUE if a melee attack would hit right now
==============
*/
static bool CheckDogMelee(edict_t *self)
{
	return range(self, self->enemy) == RANGE_MELEE;
}

/*
==============
CheckDogJump

==============
*/
static bool CheckDogJump(edict_t *self)
{
	vec3_t dist;
	float d;

	if (self->s.origin[2] + self->mins[2] > self->enemy->s.origin[2] + self->enemy->mins[2]
		+ 0.75f * self->enemy->size[2])
		return false;

	if (self->s.origin[2] + self->maxs[2] < self->enemy->s.origin[2] + self->enemy->mins[2]
		+ 0.25f * self->enemy->size[2])
		return false;

	VectorSubtract(self->enemy->s.origin, self->s.origin, dist);
	dist[2] = 0;
	d = VectorLength(dist);

	if (d < 80)
		return false;

	if (d > 150)
		return false;

	return true;
}

static void dog_attack(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "leap1");
}

static void dog_melee(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "atta1");
}


static bool DogCheckAttack(edict_t *self)
{
	// if close enough for slashing, go for it
	if (CheckDogMelee(self))
	{
		self->monsterinfo.attack_state = AS_MELEE;
		return true;
	}

	if (CheckDogJump(self))
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

	return false;
}

static void dog_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_BODY, sound_dsight, 1, ATTN_NORM, 0);
}

static const mevent_t events[] =
{
	EVENT_FUNC(dog_idle_sound),
	EVENT_FUNC(dog_run),
	EVENT_FUNC(dog_bite_func),
	EVENT_FUNC(dog_leap_func),
	EVENT_FUNC(dog_dead),
	NULL
};

/*QUAKED monster_dog (1 0 0) (-32 -32 -24) (32 32 40) Ambush
*/
void q1_monster_dog(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/q1/dog.mdl";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q1/dog.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	gi.modelindex("models/q1/h_dog.mdl");
	sound_dattack1 = gi.soundindex("q1/dog/dattack1.wav");
	sound_ddeath = gi.soundindex("q1/dog/ddeath.wav");
	sound_dpain1 = gi.soundindex("q1/dog/dpain1.wav");
	sound_dsight = gi.soundindex("q1/dog/dsight.wav");
	sound_idle = gi.soundindex("q1/dog/idle.wav");
	sound_udeath = gi.soundindex("q1/player/udeath.wav");
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, 40);
	self->health = 25;
	self->monsterinfo.stand = dog_stand;
	self->monsterinfo.walk = dog_walk;
	self->monsterinfo.run = dog_run;
	self->monsterinfo.sight = dog_sight;
	self->pain = dog_pain;
	self->die = dog_die;
	self->monsterinfo.attack = dog_attack;
	self->monsterinfo.melee = dog_melee;
	self->monsterinfo.checkattack = DogCheckAttack;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
	self->monsterinfo.scale = 1;
	walkmonster_start(self);
}

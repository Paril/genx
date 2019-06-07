/*
==============================================================================

SOLDIER / PLAYER

==============================================================================
*/
#include "g_local.h"
#include "m_local.h"

static mscript_t script;

enum
{
	stand1, stand2, stand3, stand4, stand5, stand6, stand7, stand8,

	death1, death2, death3, death4, death5, death6, death7, death8,
	death9, death10,

	deathc1, deathc2, deathc3, deathc4, deathc5, deathc6, deathc7, deathc8,
	deathc9, deathc10, deathc11,

	load1, load2, load3, load4, load5, load6, load7, load8, load9, load10, load11,

	pain1, pain2, pain3, pain4, pain5, pain6,

	painb1, painb2, painb3, painb4, painb5, painb6, painb7, painb8, painb9, painb10,
	painb11, painb12, painb13, painb14,

	painc1, painc2, painc3, painc4, painc5, painc6, painc7, painc8, painc9, painc10,
	painc11, painc12, painc13,

	run1, run2, run3, run4, run5, run6, run7, run8,

	shoot1, shoot2, shoot3, shoot4, shoot5, shoot6, shoot7, shoot8, shoot9,

	prowl_1, prowl_2, prowl_3, prowl_4, prowl_5, prowl_6, prowl_7, prowl_8,
	prowl_9, prowl_10, prowl_11, prowl_12, prowl_13, prowl_14, prowl_15, prowl_16,
	prowl_17, prowl_18, prowl_19, prowl_20, prowl_21, prowl_22, prowl_23, prowl_24,
};

static q_soundhandle sound_death1;
static q_soundhandle sound_idle;
static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_sattck1;
static q_soundhandle sound_sight1;
static q_soundhandle sound_udeath;

static void army_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
}

static void army_run_sound(edict_t *self)
{
	if (random() < 0.2f)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void army_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk1");
}

static void army_run(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run1");
}

static void army_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_BODY, sound_sight1, 1, ATTN_NORM, 0);
}

void FireBullets(edict_t *ent, int shotcount, int damage, vec3_t dir, vec3_t spread, vec3_t v_up, vec3_t v_right, meansOfDeath_t mod);

static void army_fire(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_sattck1, 1, ATTN_NORM, 0);
	// fire somewhat behind the player, so a dodging player is harder to hit
	edict_t *en = self->enemy;
	vec3_t dir;
	VectorScale(en->velocity, 0.2, dir);
	VectorSubtract(en->s.origin, dir, dir);
	VectorSubtract(dir, self->s.origin, dir);
	VectorNormalize(dir);
	vec3_t right, up;
	AngleVectors(self->s.angles, NULL, right, up);
	vec3_t spread = { 0.1, 0.1, 0 };
	FireBullets(self, 4, 4, dir, spread, up, right, MakeAttackerMeansOfDeath(self, self, MD_NONE, DT_DIRECT));
}

void SUB_CheckRefire(edict_t *self, int frame)
{
	if (skill->integer != 3 || !self->enemy)
		return;

	if (self->count == 1)
		return;

	if (!visible(self, self->enemy))
		return;

	self->count = 1;
	self->monsterinfo.nextframe = frame;
}

static void army_checkrefire(edict_t *self)
{
	SUB_CheckRefire(self, shoot1);
}

static void army_atk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "atk1");
}

edict_t *Drop_Backpack(edict_t *self);

static void army_drop(edict_t *self)
{
	self->solid = SOLID_NOT;
	//edict_t *backpack = Drop_Backpack(self);
	//backpack->pack_shells = 5;
	gi.linkentity(self);
}

static void army_dead(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
}

static void army_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// check for gib
	if (self->health < -35)
	{
		gi.sound(self, CHAN_VOICE, sound_udeath, 1, ATTN_NORM, 0);
		ThrowHead(self, "models/q1/h_guard.mdl", self->health, GIB_Q1);
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
	gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);

	if (random() < 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "bdie");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "cdie");
}

static void army_pain(edict_t *self, edict_t *attacker, float kick, int damage)
{
	if (self->pain_debounce_time > level.time)
		return;

	float r = random();

	if (r < 0.2f)
	{
		self->pain_debounce_time = level.time + 600;
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "paina");
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	}
	else if (r < 0.6f)
	{
		self->pain_debounce_time = level.time + 1100;
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "painb");
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	}
	else
	{
		self->pain_debounce_time = level.time + 1100;
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "painc");
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	}
}

static const mevent_t events[] =
{
	EVENT_FUNC(army_run_sound),
	EVENT_FUNC(army_run),
	EVENT_FUNC(army_fire),
	EVENT_FUNC(army_checkrefire),
	EVENT_FUNC(army_dead),
	EVENT_FUNC(army_drop),
	NULL
};

/*QUAKED monster_army (1 0 0) (-16 -16 -24) (16 16 40) Ambush
*/
void q1_monster_army(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/q1/soldier.mdl";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q1/soldier.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	gi.modelindex("models/q1/h_guard.mdl");
	gi.modelindex("models/q1/gib1.mdl");
	gi.modelindex("models/q1/gib2.mdl");
	gi.modelindex("models/q1/gib3.mdl");
	sound_death1 = gi.soundindex("q1/soldier/death1.wav");
	sound_idle = gi.soundindex("q1/soldier/idle.wav");
	sound_pain1 = gi.soundindex("q1/soldier/pain1.wav");
	sound_pain2 = gi.soundindex("q1/soldier/pain2.wav");
	sound_sattck1 = gi.soundindex("q1/soldier/sattck1.wav");
	sound_sight1 = gi.soundindex("q1/soldier/sight1.wav");
	sound_udeath = gi.soundindex("q1/player/udeath.wav");		// gib death
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 40);
	self->health = 30;
	self->monsterinfo.stand = army_stand;
	self->monsterinfo.walk = army_walk;
	self->monsterinfo.run = army_run;
	self->monsterinfo.attack = army_atk;
	self->monsterinfo.sight = army_sight;
	self->pain = army_pain;
	self->die = army_die;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
	self->monsterinfo.scale = 1;
	walkmonster_start(self);
}
/*
==============================================================================

SOLDIER / PLAYER

==============================================================================
*/
#ifdef ENABLE_COOP
#include "g_local.h"
#include "m_local.h"

static mscript_t script;

enum
{
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

static q_soundhandle sound_enfire;
static q_soundhandle sound_udeath;
static q_soundhandle sound_death1;
static q_soundhandle sound_idle1;
static q_soundhandle sound_pain1;
static q_soundhandle sound_pain2;
static q_soundhandle sound_sight1;
static q_soundhandle sound_sight2;
static q_soundhandle sound_sight3;
static q_soundhandle sound_sight4;

void fire_q1_laser(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed);

static void enforcer_fire(edict_t *self)
{
	gi.positioned_sound(self->server.state.origin, self, CHAN_VOICE, sound_enfire, 1, ATTN_NORM, 0);
	//self.effects = self.effects | EF_MUZZLEFLASH;
	vec3_t org, v_forward, v_right;
	AngleVectors(self->server.state.angles, v_forward, v_right, NULL);

	for (int i = 0; i < 3; ++i)
		org[i] = self->server.state.origin[i] + v_forward[i] * 30 + v_right[i] * 8.5f;

	org[2] += 16;
	vec3_t dir;
	VectorSubtract(self->enemy->server.state.origin, self->server.state.origin, dir);
	VectorNormalize(dir);
	fire_q1_laser(self, org, dir, 15, 500);
}

//============================================================================

static void enf_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
}

static void enf_idle_sound(edict_t *self)
{
	if (random() < 0.2f)
		gi.sound(self, CHAN_VOICE, sound_idle1, 1, ATTN_IDLE, 0);
}

static void enf_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk1");
}

static void enf_run(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run1");
}

void SUB_CheckRefire(edict_t *self, int frame);

static void enf_refire(edict_t *self)
{
	SUB_CheckRefire(self, attack1);
}

static void enf_pew(edict_t *self)
{
	if (self->dmg == 0)
	{
		self->monsterinfo.nextframe = attack5;
		self->dmg = 1;
		return;
	}

	self->dmg = 0;
}

static void enf_atk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "atk1");
}

static void enf_pain(edict_t *self, edict_t *attacker, float kick, int damage)
{
	float r = random();

	if (self->pain_debounce_time > level.time)
		return;

	if (r < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	if (r < 0.2f)
	{
		self->pain_debounce_time = level.time + 1000;
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "paina1");
	}
	else if (r < 0.4f)
	{
		self->pain_debounce_time = level.time + 1000;
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "painb1");
	}
	else if (r < 0.7f)
	{
		self->pain_debounce_time = level.time + 1000;
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "painc1");
	}
	else
	{
		self->pain_debounce_time = level.time + 2000;
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "paind1");
	}
}

//============================================================================

edict_t *Drop_Backpack(edict_t *self);

static void enf_drop(edict_t *self)
{
	self->server.solid = SOLID_NOT;
	//edict_t *backpack = Drop_Backpack(self);
	//backpack->pack_cells = 5;
	gi.linkentity(self);
}

static void enf_dead(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->server.flags.deadmonster = true;
	self->nextthink = 0;
}

static void enf_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// check for gib
	if (self->health < -35)
	{
		gi.sound(self, CHAN_VOICE, sound_udeath, 1, ATTN_NORM, 0);
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
	gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);

	if (random() > 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "die1");
	else
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "fdie1");
}

static void enf_sight(edict_t *self, edict_t *other)
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

static const mevent_t events[] =
{
	EVENT_FUNC(enf_idle_sound),
	EVENT_FUNC(enf_run),
	EVENT_FUNC(enforcer_fire),
	EVENT_FUNC(enf_pew),
	EVENT_FUNC(enf_refire),
	EVENT_FUNC(enf_dead),
	EVENT_FUNC(enf_drop),
	NULL
};

/*QUAKED monster_enforcer (1 0 0) (-16 -16 -24) (16 16 40) Ambush
*/
void q1_monster_enforcer(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/q1/enforcer.mdl";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q1/enforcer.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

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
	self->server.solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->server.state.modelindex = gi.modelindex(model_name);
	VectorSet(self->server.mins, -16, -16, -24);
	VectorSet(self->server.maxs, 16, 16, 40);
	self->health = 80;
	self->monsterinfo.stand = enf_stand;
	self->monsterinfo.walk = enf_walk;
	self->monsterinfo.run = enf_run;
	self->monsterinfo.attack = enf_atk;
	self->monsterinfo.sight = enf_sight;
	self->pain = enf_pain;
	self->die = enf_die;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
	self->monsterinfo.scale = 1;
	walkmonster_start(self);
}

#endif
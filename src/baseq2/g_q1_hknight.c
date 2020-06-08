/*
==============================================================================

KNIGHT

==============================================================================
*/
#ifdef ENABLE_COOP
#include "g_local.h"
#include "m_local.h"

enum
{
	stand1, stand2, stand3, stand4, stand5, stand6, stand7, stand8, stand9,

	walk1, walk2, walk3, walk4, walk5, walk6, walk7, walk8, walk9,
	walk10, walk11, walk12, walk13, walk14, walk15, walk16, walk17,
	walk18, walk19, walk20,

	run1, run2, run3, run4, run5, run6, run7, run8,

	pain1, pain2, pain3, pain4, pain5,

	death1, death2, death3, death4, death5, death6, death7, death8,
	death9, death10, death11, death12,

	deathb1, deathb2, deathb3, deathb4, deathb5, deathb6, deathb7, deathb8,
	deathb9,

	char_a1, char_a2, char_a3, char_a4, char_a5, char_a6, char_a7, char_a8,
	char_a9, char_a10, char_a11, char_a12, char_a13, char_a14, char_a15, char_a16,

	magica1, magica2, magica3, magica4, magica5, magica6, magica7, magica8,
	magica9, magica10, magica11, magica12, magica13, magica14,

	magicb1, magicb2, magicb3, magicb4, magicb5, magicb6, magicb7, magicb8,
	magicb9, magicb10, magicb11, magicb12, magicb13,

	char_b1, char_b2, char_b3, char_b4, char_b5, char_b6,

	slice1, slice2, slice3, slice4, slice5, slice6, slice7, slice8, slice9, slice10,

	smash1, smash2, smash3, smash4, smash5, smash6, smash7, smash8, smash9, smash10,
	smash11,

	w_attack1, w_attack2, w_attack3, w_attack4, w_attack5, w_attack6, w_attack7,
	w_attack8, w_attack9, w_attack10, w_attack11, w_attack12, w_attack13, w_attack14,
	w_attack15, w_attack16, w_attack17, w_attack18, w_attack19, w_attack20,
	w_attack21, w_attack22,

	magicc1, magicc2, magicc3, magicc4, magicc5, magicc6, magicc7, magicc8,
	magicc9, magicc10, magicc11
};

static q_soundhandle sound_attack1;
static q_soundhandle sound_sword1;
static q_soundhandle sound_sword2;
static q_soundhandle sound_idle1;
static q_soundhandle sound_udeath;
static q_soundhandle sound_death1;
static q_soundhandle sound_sight1;
static q_soundhandle sound_pain1;
static q_soundhandle sound_slash1;

static mscript_t script;

edict_t *fire_spike(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, bool super);

static void hknight_shot(edict_t *self, float offset)
{
	vec3_t offang;
	vec3_t org, vec;
	VectorSubtract(self->enemy->server.state.origin, self->server.state.origin, org);
	vectoangles(org, offang);
	offang[1] += offset * 6;
	vec3_t v_forward;
	AngleVectors(offang, v_forward, NULL, NULL);

	for (int i = 0; i < 3; ++i)
		org[i] = self->server.state.origin[i] + self->server.mins[i] + self->size[i] * 0.5f + v_forward[i] * 20;

	// set missile speed
	VectorNormalize2(v_forward, vec);
	vec[2] = vec[2] + (random() - 0.5f) * 0.1f;
	VectorNormalize(vec);
	edict_t *newmis = fire_spike(self, org, vec, 9, 300, false);
	gi.setmodel(newmis, "models/q1/k_spike.mdl");
	newmis->server.state.game = GAME_Q1;
	newmis->server.state.effects |= EF_Q1_HKNIGHT;
	newmis->count = TE_Q1_KNIGHTSPIKE;
	VectorClear(newmis->server.mins);
	VectorClear(newmis->server.maxs);
	gi.sound(self, CHAN_WEAPON, sound_attack1, 1, ATTN_NORM, 0);
	gi.linkentity(newmis);
}

static void CheckForCharge(edict_t *self)
{
	// check for mad charge
	if (!self->enemy || !visible(self, self->enemy))
		return;

	if (level.time < self->attack_finished_time)
		return;

	if (fabsf(self->server.state.origin[2] - self->enemy->server.state.origin[2]) > 20)
		return;		// too much height change

	if (Distance(self->server.state.origin, self->enemy->server.state.origin) < 80)
		return;		// use regular attack

	// charge
	AttackFinished(self, 2);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "char_a1");
}

static void hknight_run(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run1");
}

static void CheckContinueCharge(edict_t *self)
{
	if (level.time > self->attack_finished_time)
	{
		AttackFinished(self, 3);
		hknight_run(self);
		return;		// done charging
	}

	if (random() > 0.5f)
		gi.sound(self, CHAN_WEAPON, sound_sword2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_WEAPON, sound_sword1, 1, ATTN_NORM, 0);
}

//===========================================================================

static void hknight_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
}

//===========================================================================

static void hk_idle_sound(edict_t *self)
{
	if (random() < 0.2f)
		gi.sound(self, CHAN_VOICE, sound_idle1, 1, ATTN_NORM, 0);

	if (self->server.state.frame == run1)
		CheckForCharge(self);
}

static void hknight_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk1");
}


//============================================================================

static void hknight_unsolid(edict_t *self)
{
	self->server.solid = SOLID_NOT;
	gi.linkentity(self);
}

static void hknight_dead(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->server.flags.deadmonster = true;
	self->nextthink = 0;
}

static void hknight_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// check for gib
	if (self->health < -40)
	{
		gi.sound(self, CHAN_VOICE, sound_udeath, 1, ATTN_NORM, 0);
		ThrowHead(self, "models/q1/h_hellkn.mdl", self->health, GIB_Q1);
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
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "dieb1");
}

static void hknight_magic(edict_t *self)
{
	hknight_shot(self, self->server.state.frame - magicc8);
}

static void hknight_magicc(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "magicc1");
}

static void hknight_pain(edict_t *self, edict_t *attacker, float kick, int damage)
{
	if (self->pain_debounce_time > level.time)
		return;

	gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);

	// allways go into pain frame if it has been a while
	if (!(level.time - self->pain_debounce_time > 5000) && (random() * 30 > damage))
		return;		// didn't flinch

	self->pain_debounce_time = level.time + 1000;
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
}

static void hknight_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0);
}

//===========================================================================

static void hknight_slice(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "slice1");
}

//===========================================================================

static void hknight_smash(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "smash1");
}

//============================================================================

static void hknight_watk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "watk1");
}

//============================================================================

static void hknight_melee(edict_t *self)
{
	static byte hknight_type = 0;
	hknight_type++;
	gi.sound(self, CHAN_WEAPON, sound_slash1, 1, ATTN_NORM, 0);

	if (hknight_type == 1)
		hknight_slice(self);
	else if (hknight_type == 2)
		hknight_smash(self);
	else if (hknight_type == 3)
	{
		hknight_watk(self);
		hknight_type = 0;
	}
}

void ai_melee(edict_t *self);

static const mevent_t events[] =
{
	EVENT_FUNC(hknight_run),
	EVENT_FUNC(ai_melee),
	EVENT_FUNC(hk_idle_sound),
	EVENT_FUNC(hknight_dead),
	EVENT_FUNC(hknight_unsolid),
	EVENT_FUNC(hknight_magic),
	NULL
};

/*QUAKED monster_hell_knight (1 0 0) (-16 -16 -24) (16 16 40) Ambush
*/
void q1_monster_hell_knight(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/q1/hknight.mdl";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q1/hknight.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	gi.modelindex("models/q1/k_spike.mdl");
	gi.modelindex("models/q1/h_hellkn.mdl");
	sound_attack1 = gi.soundindex("q1/hknight/attack1.wav");
	sound_death1 = gi.soundindex("q1/hknight/death1.wav");
	sound_pain1 = gi.soundindex("q1/hknight/pain1.wav");
	sound_sight1 = gi.soundindex("q1/hknight/sight1.wav");
	sound_slash1 = gi.soundindex("q1/hknight/slash1.wav");
	sound_idle1 = gi.soundindex("q1/hknight/idle.wav");
	sound_sword1 = gi.soundindex("q1/knight/sword1.wav");
	sound_sword2 = gi.soundindex("q1/knight/sword2.wav");
	sound_udeath = gi.soundindex("q1/player/udeath.wav");		// gib death
	self->server.solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->server.state.modelindex = gi.modelindex(model_name);
	VectorSet(self->server.mins, -16, -16, -24);
	VectorSet(self->server.maxs, 16, 16, 40);
	self->health = 250;
	self->monsterinfo.stand = hknight_stand;
	self->monsterinfo.walk = hknight_walk;
	self->monsterinfo.run = hknight_run;
	self->monsterinfo.melee = hknight_melee;
	self->monsterinfo.attack = hknight_magicc;
	self->monsterinfo.sight = hknight_sight;
	self->pain = hknight_pain;
	self->die = hknight_die;
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
	self->monsterinfo.scale = 1;
	walkmonster_start(self);
}
#endif
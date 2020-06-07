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

	runb1, runb2, runb3, runb4, runb5, runb6, runb7, runb8,

	//runc1, runc2, runc3, runc4, runc5, runc6,

	runattack1, runattack2, runattack3, runattack4, runattack5,
	runattack6, runattack7, runattack8, runattack9, runattack10,
	runattack11,

	pain1, pain2, pain3,

	painb1, painb2, painb3, painb4, painb5, painb6, painb7, painb8, painb9,
	painb10, painb11,

	//attack1, attack2, attack3, attack4, attack5, attack6, attack7,
	//attack8, attack9, attack10, attack11,

	attackb1_, attackb1, attackb2, attackb3, attackb4, attackb5,
	attackb6, attackb7, attackb8, attackb9, attackb10,

	walk1, walk2, walk3, walk4, walk5, walk6, walk7, walk8, walk9,
	walk10, walk11, walk12, walk13, walk14,

	kneel1, kneel2, kneel3, kneel4, kneel5,

	standing2, standing3, standing4, standing5,

	death1, death2, death3, death4, death5, death6, death7, death8,
	death9, death10,

	deathb1, deathb2, deathb3, deathb4, deathb5, deathb6, deathb7, deathb8,
	deathb9, deathb10, deathb11
};

static q_soundhandle sound_idle;
static q_soundhandle sound_sword1;
static q_soundhandle sound_sword2;
static q_soundhandle sound_khurt;
static q_soundhandle sound_ksight;
static q_soundhandle sound_kdeath;
static q_soundhandle sound_udeath;

static mscript_t script;

static void knight_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
}

static void knight_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_ksight, 1, ATTN_NORM, 0);
}

static void knight_idle_sound(edict_t *self)
{
	if (random() < 0.2f)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void knight_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk1");
}

static void knight_run(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run1");
}

static void knight_sword_sound(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_sword1, 1, ATTN_NORM, 0);
}

static void knight_sword2_sound(edict_t *self)
{
	if (random() < 0.5f)
		gi.sound(self, CHAN_WEAPON, sound_sword1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_WEAPON, sound_sword2, 1, ATTN_NORM, 0);
}

/*
=============
ai_melee

=============
*/
void ai_melee(edict_t *self)
{
	vec3_t delta;
	float ldmg;

	if (!self->enemy)
		return;		// removed before stroke

	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);

	if (VectorLength(delta) > 60)
		return;

	ldmg = (random() + random() + random()) * 3;
	T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_MELEE, DT_DIRECT));
}

static void knight_atk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "atk1");
}

//===========================================================================

static void knight_paina(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
}

static void knight_painb(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "painb1");
}

static void knight_pain(edict_t *self, edict_t *attacker, float kick, int damage)
{
	if (self->pain_debounce_time > level.time)
		return;

	float r = random();
	gi.sound(self, CHAN_VOICE, sound_khurt, 1, ATTN_NORM, 0);

	if (r < 0.85f)
	{
		knight_paina(self);
		self->pain_debounce_time = level.time + 1000;
	}
	else
	{
		knight_painb(self);
		self->pain_debounce_time = level.time + 1000;
	}
}

//===========================================================================

static void knight_unsolid(edict_t *self)
{
	self->solid = SOLID_NOT;
	gi.linkentity(self);
}

static void knight_dead(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
}

static void knight_diea(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "diea1");
}

static void knight_dieb(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "dieb1");
}

static void knight_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// check for gib
	if (self->health < -40)
	{
		gi.sound(self, CHAN_VOICE, sound_udeath, 1, ATTN_NORM, 0);
		ThrowHead(self, "models/q1/h_knight.mdl", self->health, GIB_Q1);
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
	gi.sound(self, CHAN_VOICE, sound_kdeath, 1, ATTN_NORM, 0);

	if (random() < 0.5f)
		knight_diea(self);
	else
		knight_dieb(self);
}

static void ai_charge_side(edict_t *self, float dist)
{
	vec3_t dtemp;
	float heading;
	// aim to the left of the enemy for a flyby
	VectorSubtract(self->enemy->s.origin, self->s.origin, dtemp);
	self->ideal_yaw = vectoyaw(dtemp);
	M_ChangeYaw(self);
	vec3_t v_right;
	AngleVectors(self->s.angles, NULL, v_right, NULL);
	VectorMA(self->enemy->s.origin, -30, v_right, dtemp);
	VectorSubtract(dtemp, self->s.origin, dtemp);
	heading = vectoyaw(dtemp);
	M_walkmove(self, heading, 20);
}

static void ai_melee_side(edict_t *self, float dist)
{
	vec3_t delta;
	float ldmg;

	if (!self->enemy)
		return;		// removed before stroke

	ai_charge_side(self, dist);
	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);

	if (VectorLength(delta) > 60)
		return;

	if (!CanDamage(self->enemy, self))
		return;

	ldmg = (random() + random() + random()) * 3;
	T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_MELEE, DT_DIRECT));
}

static void knight_runatk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "runatk1");
}

static const mevent_t events[] =
{
	EVENT_FUNC(knight_idle_sound),
	EVENT_FUNC(knight_run),
	EVENT_FUNC(knight_sword_sound),
	EVENT_FUNC(ai_melee),
	EVENT_FUNC(knight_dead),
	EVENT_FUNC(knight_unsolid),
	EVENT_FUNC(knight_sword2_sound),
	EVENT_FUNC(ai_melee_side),
	EVENT_FUNC(ai_charge_side),
	NULL
};

/*QUAKED monster_knight (1 0 0) (-16 -16 -24) (16 16 40) Ambush
*/
void q1_monster_knight(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/q1/knight.mdl";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q1/knight.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	gi.modelindex("models/q1/h_knight.mdl");
	sound_kdeath = gi.soundindex("q1/knight/kdeath.wav");
	sound_khurt = gi.soundindex("q1/knight/khurt.wav");
	sound_ksight = gi.soundindex("q1/knight/ksight.wav");
	sound_sword1 = gi.soundindex("q1/knight/sword1.wav");
	sound_sword2 = gi.soundindex("q1/knight/sword2.wav");
	sound_idle = gi.soundindex("q1/knight/idle.wav");
	sound_udeath = gi.soundindex("q1/player/udeath.wav");		// gib death
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 40);
	self->health = 75;
	self->monsterinfo.stand = knight_stand;
	self->monsterinfo.walk = knight_walk;
	self->monsterinfo.run = knight_run;
	self->monsterinfo.melee = knight_atk;
	self->monsterinfo.attack = knight_runatk;
	self->pain = knight_pain;
	self->die = knight_die;
	self->monsterinfo.sight = knight_sight;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
	self->monsterinfo.scale = 1;
	walkmonster_start(self);
}

#endif
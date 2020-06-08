/*
==============================================================================

WIZARD

==============================================================================
*/
#ifdef ENABLE_COOP
#include "g_local.h"
#include "m_local.h"

enum
{
	hover1, hover2, hover3, hover4, hover5, hover6, hover7, hover8,
	hover9, hover10, hover11, hover12, hover13, hover14, hover15,

	fly1, fly2, fly3, fly4, fly5, fly6, fly7, fly8, fly9, fly10,
	fly11, fly12, fly13, fly14,

	magatt1, magatt2, magatt3, magatt4, magatt5, magatt6, magatt7,
	magatt8, magatt9, magatt10, magatt11, magatt12, magatt13,

	pain1, pain2, pain3, pain4,

	death1, death2, death3, death4, death5, death6, death7, death8
};

static mscript_t script;

/*
==============================================================================

WIZARD

If the player moves behind cover before the missile is launched, launch it
at the last visible spot with no velocity leading, in hopes that the player
will duck back out and catch it.
==============================================================================
*/

void AttackFinished(edict_t *, float);

/*
=============
LaunchMissile

Sets the given entities velocity and angles so that it will hit self.enemy
if self.enemy maintains it's current velocity
0.1 is moderately accurate, 0.0 is totally accurate
=============
*/
static void LaunchMissile(edict_t *self, edict_t *missile, float mspeed, float accuracy)
{
	vec3_t vec, move;
	float	fly;
	vec3_t v_forward, v_right, v_up;
	AngleVectors(self->server.state.angles, v_forward, v_right, v_up);

	// set missile speed
	for (int i = 0; i < 3; ++i)
		vec[i] = self->enemy->server.state.origin[i] + self->enemy->server.mins[i] + self->enemy->size[i] * 0.7f - missile->server.state.origin[i];

	// calc aproximate time for missile to reach vec
	fly = VectorLength(vec) / mspeed;
	// get the entities xy velocity
	VectorCopy(self->enemy->velocity, move);
	move[2] = 0;
	// project the target forward in time
	VectorMA(vec, fly, move, vec);
	VectorNormalize(vec);

	for (int i = 0; i < 3; ++i)
		vec[i] = vec[i] + accuracy * v_up[i] * (random() - 0.5f) + accuracy * v_right[i] * (random() - 0.5f);

	VectorScale(vec, mspeed, missile->velocity);
	VectorClear(missile->server.state.angles);
	missile->server.state.angles[1] = vectoyaw(missile->velocity);
	// set missile duration
	missile->nextthink = level.time + 5000;
	missile->think = G_FreeEdict;
}

/*
==============================================================================

FAST ATTACKS

==============================================================================
*/

edict_t *fire_spike(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, bool super);

static q_soundhandle sound_wattack;
static q_soundhandle sound_widle1;
static q_soundhandle sound_widle2;
static q_soundhandle sound_wpain;
static q_soundhandle sound_udeath;
static q_soundhandle sound_wdeath;
static q_soundhandle sound_wsight;

static void Wiz_FastFire(edict_t *self)
{
	vec3_t vec;
	vec3_t dst;

	if (self->server.owner->health > 0)
	{
		//self.owner.effects = self.owner.effects | EF_MUZZLEFLASH;
		VectorMA(self->enemy->server.state.origin, -13, self->movedir, dst);
		VectorSubtract(dst, self->server.state.origin, vec);
		VectorNormalize(vec);
		gi.sound(self, CHAN_WEAPON, sound_wattack, 1, ATTN_NORM, 0);
		edict_t *newmis = fire_spike(self, self->server.state.origin, vec, 9, 600, false);
		newmis->server.owner = self->server.owner;
		newmis->count = TE_Q1_WIZSPIKE;
		newmis->server.state.effects |= EF_Q1_WIZ;
		gi.setmodel(newmis, "models/q1/w_spike.mdl");
		gi.linkentity(newmis);
	}

	G_FreeEdict(self);
}

static void Wiz_StartFast(edict_t *self)
{
	edict_t *missile;
	gi.sound(self, CHAN_WEAPON, sound_wattack, 1, ATTN_NORM, 0);
	vec3_t v_forward, v_right;
	AngleVectors(self->server.state.angles, v_forward, v_right, NULL);
	missile = G_Spawn();
	missile->server.owner = self;
	missile->nextthink = level.time + 600;
	VectorClear(missile->server.mins);
	VectorClear(missile->server.maxs);

	for (int i = 0; i < 3; ++i)
		missile->server.state.origin[i] = self->server.state.origin[i] + v_forward[i] * 14 + v_right[i] * 14;

	missile->server.state.origin[2] += 30;
	missile->enemy = self->enemy;
	missile->nextthink = level.time + 800;
	missile->think = Wiz_FastFire;
	VectorCopy(v_right, missile->movedir);
	missile = G_Spawn();
	missile->server.owner = self;
	missile->nextthink = level.time + 1000;
	VectorClear(missile->server.mins);
	VectorClear(missile->server.maxs);

	for (int i = 0; i < 3; ++i)
		missile->server.state.origin[i] = self->server.state.origin[i] + v_forward[i] * 14 + v_right[i] * -14;

	missile->server.state.origin[2] += 30;
	missile->enemy = self->enemy;
	missile->nextthink = level.time + 300;
	missile->think = Wiz_FastFire;
	VectorSubtract(vec3_origin, v_right, missile->movedir);
}

static void Wiz_idlesound(edict_t *self)
{
	float wr = random() * 5;

	if (self->touch_debounce_time < level.time)
	{
		self->touch_debounce_time = level.time + 2000;

		if (wr > 4.5f)
			gi.sound(self, CHAN_VOICE, sound_widle1, 1,  ATTN_IDLE, 0);

		if (wr < 1.5f)
			gi.sound(self, CHAN_VOICE, sound_widle2, 1, ATTN_IDLE, 0);
	}
}

static void wiz_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
}

static void wiz_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk1");
}

static void wiz_side(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "side1");
}

static void wiz_run(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run1");
}

static void wiz_fastb(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "fast2");
}

static void wiz_fast(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "fast1");
}

static void wiz_paina(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
}

static void wiz_unsolid(edict_t *self)
{
	self->server.solid = SOLID_NOT;
	gi.linkentity(self);
}

static void wiz_dead(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->server.flags.deadmonster = true;
	self->nextthink = 0;
}

static void wiz_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// check for gib
	if (self->health < -40)
	{
		gi.sound(self, CHAN_VOICE, sound_udeath, 1, ATTN_NORM, 0);
		ThrowHead(self, "models/q1/h_wizard.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib2.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib2.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib2.mdl", self->health, GIB_Q1);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag)
		return;

	self->deadflag = DEAD_DEAD;
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death1");
	self->velocity[0] = -200 + 400 * random();
	self->velocity[1] = -200 + 400 * random();
	self->velocity[2] = 100 + 100 * random();
	self->groundentity = NULL;
	gi.sound(self, CHAN_VOICE, sound_wdeath, 1, ATTN_NORM, 0);
}

static void Wiz_Pain(edict_t *self, edict_t *attacker, float kick, int damage)
{
	gi.sound(self, CHAN_VOICE, sound_wpain, 1, ATTN_NORM, 0);

	if (random() * 70 > damage)
		return;		// didn't flinch

	wiz_paina(self);
}

static void Wiz_Missile(edict_t *self)
{
	wiz_fast(self);
}

static void wiz_sight(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_wsight, 1, ATTN_NORM, 0);
}

/*
=================
WizardAttackFinished
=================
*/
static void WizardAttackFinished(edict_t *self)
{
	AttackFinished(self, 2);

	if (range(self, self->enemy) >= RANGE_MID || !visible(self, self->enemy))
	{
		self->monsterinfo.attack_state = AS_STRAIGHT;
		wiz_run(self);
	}
	else
	{
		self->monsterinfo.attack_state = AS_SLIDING;
		wiz_side(self);
	}
}

static void wiz_fastfinish(edict_t *self)
{
	WizardAttackFinished(self);
	wiz_run(self);
}

/*
=================
WizardCheckAttack
=================
*/
static bool	WizardCheckAttack(edict_t *self)
{
	vec3_t spot1, spot2;
	edict_t *targ;
	float chance;

	if (level.time < self->monsterinfo.attack_finished)
		return false;

	if (!visible(self, self->enemy))
		return false;

	int enemy_range = range(self, self->enemy);

	if (enemy_range == RANGE_FAR)
	{
		if (self->monsterinfo.attack_state != AS_STRAIGHT)
		{
			self->monsterinfo.attack_state = AS_STRAIGHT;
			wiz_run(self);
		}

		return false;
	}

	targ = self->enemy;
	// see if any entities are in the way of the shot
	VectorCopy(self->server.state.origin, spot1);
	VectorCopy(targ->server.state.origin, spot2);
	spot1[2] += self->viewheight;
	spot2[2] += targ->viewheight;
	trace_t tr = gi.trace(spot1, vec3_origin, vec3_origin, spot2, self, MASK_SHOT);

	if (tr.ent != targ)
	{
		// don't have a clear shot, so move to a side
		if (self->monsterinfo.attack_state != AS_STRAIGHT)
		{
			self->monsterinfo.attack_state = AS_STRAIGHT;
			wiz_run(self);
		}

		return false;
	}

	if (enemy_range == RANGE_MELEE)
		chance = 0.9;
	else if (enemy_range == RANGE_NEAR)
		chance = 0.6;
	else if (enemy_range == RANGE_MID)
		chance = 0.2;
	else
		chance = 0;

	if (random() < chance)
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

	if (enemy_range == RANGE_MID)
	{
		if (self->monsterinfo.attack_state != AS_STRAIGHT)
		{
			self->monsterinfo.attack_state = AS_STRAIGHT;
			wiz_run(self);
		}
	}
	else
	{
		if (self->monsterinfo.attack_state != AS_SLIDING)
		{
			self->monsterinfo.attack_state = AS_SLIDING;
			wiz_side(self);
		}
	}

	return false;
}

static const mevent_t events[] =
{
	EVENT_FUNC(Wiz_idlesound),
	EVENT_FUNC(wiz_fastfinish),
	EVENT_FUNC(wiz_fastb),
	EVENT_FUNC(Wiz_StartFast),
	EVENT_FUNC(wiz_run),
	EVENT_FUNC(wiz_dead),
	EVENT_FUNC(wiz_unsolid),
	NULL
};

/*QUAKED monster_wizard (1 0 0) (-16 -16 -24) (16 16 40) Ambush
*/
void q1_monster_wizard(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/q1/wizard.mdl";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q1/wizard.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	gi.modelindex("models/q1h_wizard.mdl");
	gi.modelindex("models/q1w_spike.mdl");
	sound_wattack = gi.soundindex("q1/wizard/wattack.wav");
	sound_wdeath = gi.soundindex("q1/wizard/wdeath.wav");
	sound_widle1 = gi.soundindex("q1/wizard/widle1.wav");
	sound_widle2 = gi.soundindex("q1/wizard/widle2.wav");
	sound_wpain = gi.soundindex("q1/wizard/wpain.wav");
	sound_wsight = gi.soundindex("q1/wizard/wsight.wav");
	sound_udeath = gi.soundindex("q1/player/udeath.wav");		// gib death
	self->server.solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->server.state.modelindex = gi.modelindex(model_name);
	VectorSet(self->server.mins, -16, -16, -24);
	VectorSet(self->server.maxs, 16, 16, 40);
	self->health = 80;
	self->monsterinfo.stand = wiz_stand;
	self->monsterinfo.walk = wiz_walk;
	self->monsterinfo.run = wiz_run;
	self->monsterinfo.attack = Wiz_Missile;
	self->monsterinfo.checkattack = WizardCheckAttack;
	self->pain = Wiz_Pain;
	self->die = wiz_die;
	self->monsterinfo.scale = 1;
	wiz_stand(self);
	flymonster_start(self);
	self->viewheight = 25;
}

#endif
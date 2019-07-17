/*
==============================================================================

SHAL-RATH

==============================================================================
*/
#include "g_local.h"
#include "m_local.h"

enum
{
	attack1, attack2, attack3, attack4, attack5, attack6, attack7, attack8,
	attack9, attack10, attack11,

	pain1, pain2, pain3, pain4, pain5,

	death1, death2, death3, death4, death5, death6, death7,

	walk1, walk2, walk3, walk4, walk5, walk6, walk7, walk8, walk9, walk10,
	walk11, walk12
};

static q_soundhandle sound_idle;
static q_soundhandle sound_attack;
static q_soundhandle sound_sight;
static q_soundhandle sound_pain;
static q_soundhandle sound_udeath;
static q_soundhandle sound_death;
static q_soundhandle sound_attack2;

static mscript_t script;

static void shal_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
}

static void shal_idle_sound(edict_t *self)
{
	if (random() < 0.2f)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void shal_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk1");
}

static void shal_run(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run1");
}

static void shal_attack_sound(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_attack, 1, ATTN_NORM, 0);
}

static void shal_sight_sound(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

/*
================
ShalMissile
================
*/
void ShalHome(edict_t *self)
{
	vec3_t dir, vtemp;
	VectorCopy(self->enemy->s.origin, vtemp);
	vtemp[2] += 10;

	if (self->enemy->health < 1)
	{
		G_FreeEdict(self);
		return;
	}

	VectorSubtract(vtemp, self->s.origin, dir);
	VectorNormalize(dir);

	if (skill->integer == 3)
		VectorScale(dir, 350, self->velocity);
	else
		VectorScale(dir, 250, self->velocity);

	self->nextthink = level.time + 200;
	self->think = ShalHome;
}

void ShalMissileTouch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;		// don't explode on owner

	if (other->entitytype == ET_Q1_MONSTER_ZOMBIE)
		T_Damage(other, self, self, vec3_origin, vec3_origin, vec3_origin, 110, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, self->meansOfDeath);

	T_RadiusDamage(self, self->owner, 40, world, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, 40, self->meansOfDeath);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_Q1_EXPLODE);
	MSG_WritePos(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);
	G_FreeEdict(self);
}

void ShalMissile(edict_t *self)
{
	edict_t *missile;
	vec3_t dir;
	float dist, flytime;
	VectorCopy(self->enemy->s.origin, dir);
	dir[2] += 10;
	VectorSubtract(dir, self->s.origin, dir);
	VectorNormalize(dir);
	dist = Distance(self->enemy->s.origin, self->s.origin);
	flytime = dist * 0.002f;

	if (flytime < 0.1f)
		flytime = 0.1f;

	//self.effects = self.effects | EF_MUZZLEFLASH;
	gi.sound(self, CHAN_WEAPON, sound_attack2, 1, ATTN_NORM, 0);
	missile = G_Spawn();
	missile->owner = self;
	missile->solid = SOLID_BBOX;
	missile->movetype = MOVETYPE_FLYMISSILE;
	gi.setmodel(missile, "models/q1/v_spike.mdl");
	VectorClear(missile->mins);
	VectorClear(missile->maxs);
	VectorCopy(self->s.origin, missile->s.origin);
	missile->s.origin[2] += 10l;
	VectorScale(dir, 400, missile->velocity);
	VectorSet(missile->avelocity, 300, 300, 300);
	missile->nextthink = level.time + flytime * 1000;
	missile->think = ShalHome;
	missile->enemy = self->enemy;
	missile->touch = ShalMissileTouch;
	missile->clipmask = MASK_SHOT;
	missile->s.game = GAME_Q1;
	missile->s.effects |= EF_Q1_VORE;
	missile->entitytype = ET_Q1_VORE_BALL;
	missile->meansOfDeath = MakeAttackerMeansOfDeath(self, missile, MD_NONE, DT_DIRECT);
	gi.linkentity(missile);
}

static void shal_attack(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
}

static void shal_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->pain_debounce_time > level.time)
		return;

	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
	self->pain_debounce_time = level.time + 3000;
}

static void shal_dead(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
}

static void shalrath_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// check for gib
	if (self->health < -90)
	{
		gi.sound(self, CHAN_VOICE, sound_udeath, 1, ATTN_NORM, 0);
		ThrowHead(self, "models/q1/h_shal.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib1.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib2.mdl", self->health, GIB_Q1);
		ThrowGib(self, "models/q1/gib3.mdl", self->health, GIB_Q1);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death1");
	self->solid = SOLID_NOT;
	gi.linkentity(self);
	// insert death sounds here
}


//=================================================================

static const mevent_t events[] =
{
	EVENT_FUNC(shal_idle_sound),
	EVENT_FUNC(shal_run),
	EVENT_FUNC(shal_attack_sound),
	EVENT_FUNC(ShalMissile),
	EVENT_FUNC(shal_dead),
	NULL
};

/*QUAKED monster_shalrath (1 0 0) (-32 -32 -24) (32 32 48) Ambush
*/
void q1_monster_shalrath(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/q1/shalrath.mdl";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q1/shalrath.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	gi.modelindex("models/q1/shalrath.mdl");
	gi.modelindex("models/q1/h_shal.mdl");
	gi.modelindex("models/q1/v_spike.mdl");
	sound_attack = gi.soundindex("q1/shalrath/attack.wav");
	sound_attack2 = gi.soundindex("q1/shalrath/attack2.wav");
	sound_death = gi.soundindex("q1/shalrath/death.wav");
	sound_idle = gi.soundindex("q1/shalrath/idle.wav");
	sound_pain = gi.soundindex("q1/shalrath/pain.wav");
	sound_sight = gi.soundindex("q1/shalrath/sight.wav");
	sound_udeath = gi.soundindex("q1/player/udeath.wav");
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, 64);
	self->health = 400;
	self->monsterinfo.stand = shal_stand;
	self->monsterinfo.walk = shal_walk;
	self->monsterinfo.run = shal_run;
	self->die = shalrath_die;
	self->pain = shal_pain;
	self->monsterinfo.attack = shal_attack;
	self->think = walkmonster_start;
	self->nextthink = level.time + (0.1f + random() * 0.1f) * 1000;
	gi.linkentity(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
	self->monsterinfo.scale = 1;
}

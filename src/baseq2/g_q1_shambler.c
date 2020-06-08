/*
==============================================================================

SHAMBLER

==============================================================================
*/
#ifdef ENABLE_COOP

#include "g_local.h"
#include "m_local.h"

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

static mscript_t script;

static void sham_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
}

static void sham_idle_sound(edict_t *self)
{
	if (random() > 0.8f)
		gi.sound(self, CHAN_VOICE, sound_sidle, 1, ATTN_IDLE, 0);
}

static void sham_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk1");
}

static void sham_run(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run1");
}

void SpawnMeatSpray(edict_t *self, vec3_t org, vec3_t vel);

static void sham_smash_hit(edict_t *self)
{
	vec3_t delta;
	float ldmg;

	if (!self->enemy)
		return;

	VectorSubtract(self->enemy->server.state.origin, self->server.state.origin, delta);

	if (VectorLength(delta) > 100)
		return;

	if (!CanDamage(self->enemy, self))
		return;

	ldmg = (random() + random() + random()) * 40;
	T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_MELEE, DT_DIRECT));
	gi.sound(self, CHAN_VOICE, sound_smack, 1, ATTN_NORM, 0);
	vec3_t v, o;
	vec3_t v_forward, v_right;
	AngleVectors(self->server.state.angles, v_forward, v_right, NULL);
	VectorMA(self->server.state.origin, 16, v_forward, o);
	VectorScale(v_right, crandom() * 100, v);
	SpawnMeatSpray(self, o, v);
	SpawnMeatSpray(self, o, v);
}

static void sham_smash_sound(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_melee1, 1, ATTN_NORM, 0);
}

static void sham_smash(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "smash1");
}

static void ShamClaw(edict_t *self, float side)
{
	vec3_t delta;
	float ldmg;

	if (!self->enemy)
		return;

	VectorSubtract(self->enemy->server.state.origin, self->server.state.origin, delta);

	if (VectorLength(delta) > 100)
		return;

	ldmg = (random() + random() + random()) * 20;
	T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_MELEE, DT_DIRECT));
	gi.sound(self, CHAN_VOICE, sound_smack, 1, ATTN_NORM, 0);

	if (side)
	{
		vec3_t v, o;
		vec3_t v_forward, v_right;
		AngleVectors(self->server.state.angles, v_forward, v_right, NULL);
		VectorMA(self->server.state.origin, 16, v_forward, o);
		VectorScale(v_right, side, v);
		SpawnMeatSpray(self, o, v);
	}
}

static void sham_swing_sound(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_melee2, 1, ATTN_NORM, 0);
}

static void sham_swing_left(edict_t *self)
{
	ShamClaw(self, 250);
}

static void sham_swing_left_re(edict_t *self)
{
	if (random() < 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "swingr1");
}

static void sham_swingl(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "swingl1");
}

static void sham_swing_right(edict_t *self)
{
	ShamClaw(self, -250);
}

static void sham_swing_right_re(edict_t *self)
{
	if (random() < 0.5f)
		self->monsterinfo.currentmove = M_GetMonsterMove(&script, "swingl1");
}

static void sham_swingr(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "swingr1");
}

static void sham_melee(edict_t *self)
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

static void CastLightning(edict_t *self)
{
	vec3_t org, dir;
	//self.effects = self.effects | EF_MUZZLEFLASH;
	VectorCopy(self->server.state.origin, org);
	org[2] += 40;

	for (int i = 0; i < 3; ++i)
		dir[i] = self->enemy->server.state.origin[i] + ((i == 2) ? 16 : 0) - org[i];

	VectorNormalize(dir);
	vec3_t end;
	VectorMA(self->server.state.origin, 600, dir, end);
	trace_t tr = gi.trace(org, vec3_origin, vec3_origin, end, self, MASK_SHOT);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_Q1_LIGHTNING1);
	MSG_WriteShort(self - g_edicts);
	MSG_WritePos(org);
	MSG_WritePos(tr.endpos);
	gi.multicast(tr.endpos, MULTICAST_PVS);
	LightningDamage(self, org, tr.endpos, self, 10);
}

static void sham_magic_sound(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_sattck1, 1, ATTN_NORM, 0);
}

static void sham_magic_start(edict_t *self)
{
	if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
	{
		self->monsterinfo.pausetime = level.time + 200;
		edict_t *o;
		//self.effects = self.effects | EF_MUZZLEFLASH;
		o = G_Spawn();
		self->server.owner = o;
		gi.setmodel(o, "models/q1/s_light.mdl");
		VectorCopy(self->server.state.origin, o->server.state.origin);
		VectorCopy(self->server.state.angles, o->server.state.angles);
		o->nextthink = level.time + 700;
		o->think = G_FreeEdict;
		gi.linkentity(o);
	}
	else if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void sham_magic_inc(edict_t *self)
{
	self->server.owner->server.state.frame++;
}

static void sham_fire(edict_t *self)
{
	if (self->server.state.frame == magic11)
	{
		if (skill->integer == 3)
			CastLightning(self);
	}
	else if (self->server.state.frame == magic9 || self->server.state.frame == magic10)
		CastLightning(self);
	else if (self->server.state.frame == magic6)
	{
		G_FreeEdict(self->server.owner);
		self->server.owner = NULL;
		CastLightning(self);
		gi.sound(self, CHAN_WEAPON, sound_sboom, 1, ATTN_NORM, 0);
		self->monsterinfo.nextframe = magic9;
	}
}

static void sham_magic(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "magic1");
}

static void sham_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	gi.sound(self, CHAN_VOICE, sound_shurt2, 1, ATTN_NORM, 0);

	if (self->health <= 0)
		return;		// allready dying, don't go into pain frame

	if (random() * 400 > damage)
		return;		// didn't flinch

	if (self->pain_debounce_time > level.time)
		return;

	self->pain_debounce_time = level.time + 2000;
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
}

static void sham_unsolid(edict_t *self)
{
	self->server.solid = SOLID_NOT;
	gi.linkentity(self);
}

static void sham_dead(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->server.flags.deadmonster = true;
	self->nextthink = 0;
}

static void sham_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death1");
}

//============================================================================

static void sham_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_ssight, 1, ATTN_NORM, 0);
}

static const mevent_t events[] =
{
	EVENT_FUNC(sham_idle_sound),
	EVENT_FUNC(sham_run),
	EVENT_FUNC(sham_swing_sound),
	EVENT_FUNC(sham_swing_left),
	EVENT_FUNC(sham_swing_left_re),
	EVENT_FUNC(sham_smash_sound),
	EVENT_FUNC(sham_swing_right),
	EVENT_FUNC(sham_swing_right_re),
	EVENT_FUNC(sham_magic_sound),
	EVENT_FUNC(sham_magic_start),
	EVENT_FUNC(sham_magic_inc),
	EVENT_FUNC(sham_fire),
	EVENT_FUNC(sham_dead),
	EVENT_FUNC(sham_unsolid),
	NULL
};

/*QUAKED monster_shambler (1 0 0) (-32 -32 -24) (32 32 64) Ambush
*/
void q1_monster_shambler(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/q1/shambler.mdl";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q1/shambler.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
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
	self->server.solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->server.state.modelindex = gi.modelindex(model_name);
	VectorSet(self->server.mins, -32, -32, -24);
	VectorSet(self->server.maxs, 32, 32, 64);
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
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
	self->monsterinfo.scale = 1;
}

#endif
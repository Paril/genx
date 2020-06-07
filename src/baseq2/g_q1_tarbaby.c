/*
==============================================================================

BLOB

==============================================================================
*/
#ifdef ENABLE_COOP

#include "g_local.h"
#include "m_local.h"

enum
{
	walk1, walk2, walk3, walk4, walk5, walk6, walk7, walk8, walk9, walk10,
	walk11, walk12, walk13, walk14, walk15, walk16, walk17, walk18, walk19,
	walk20, walk21, walk22, walk23, walk24, walk25,

	run1, run2, run3, run4, run5, run6, run7, run8, run9, run10, run11, run12, run13,
	run14, run15, run16, run17, run18, run19, run20, run21, run22, run23,
	run24, run25,

	jump1, jump2, jump3, jump4, jump5, jump6,

	fly1, fly2, fly3, fly4,

	explode
};

static q_soundhandle sound_hit1;
static q_soundhandle sound_land1;
static q_soundhandle sound_death1;
static q_soundhandle sound_sight1;

static mscript_t script;

static void tbaby_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
}

static void tbaby_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk1");
}

static void tbaby_run(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run1");
}

static void tbaby_jump(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "jump1");
	self->monsterinfo.nextframe = jump1;
}

//============================================================================

static void Tar_JumpTouch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->takedamage && other->entitytype != self->entitytype)
	{
		if (VectorLength(self->velocity) > 400)
		{
			float ldmg = 10 + 10 * random();
			T_Damage(other, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_MELEE, DT_DIRECT));
			gi.sound(self, CHAN_WEAPON, sound_hit1, 1, ATTN_NORM, 0);
		}
	}
	else
		gi.sound(self, CHAN_WEAPON, sound_land1, 1, ATTN_NORM, 0);

	if (!M_CheckBottom(self))
	{
		if (self->groundentity)
		{
			self->touch = NULL;
			tbaby_run(self);
			self->movetype = MOVETYPE_STEP;
		}

		return;	// not on ground yet
	}

	self->touch = NULL;
	tbaby_jump(self);
}

static void	tbaby_jump_do(edict_t *self)
{
	self->movetype = MOVETYPE_BOUNCE;
	self->touch = Tar_JumpTouch;
	vec3_t v_forward;
	AngleVectors(self->s.angles, v_forward, NULL, NULL);
	self->s.origin[2] = self->s.origin[2] + 1;
	VectorScale(v_forward, 600, self->velocity);
	self->velocity[2] = self->velocity[2] + 200 + random() * 150;
	self->groundentity = NULL;
	self->count = 0;
}

static void tbaby_fly_go(edict_t *self)
{
	self->count = self->count + 1;

	if (self->count == 4)
	{
		//dprint ("spawn hop\n");
		tbaby_jump_do(self);
	}
}

static void tbaby_fly(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "fly1");
}

//=============================================================================

static void tbaby_explode(edict_t *self)
{
	T_RadiusDamage(self, self, 120, world, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, 120, MakeGenericMeansOfDeath(self, MD_NONE, DT_INDIRECT));
	gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
	VectorNormalize(self->velocity);
	VectorMA(self->s.origin, -8, self->velocity, self->s.origin);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_Q1_TAREXPLOSION);
	MSG_WritePos(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);
	self->think = G_FreeEdict;
	self->nextthink = level.time + 100;
}

static void tbaby_nohurt(edict_t *self)
{
	self->takedamage = false;
}

static void tbaby_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "die1");
}

static void tbaby_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0);
}

//=============================================================================

static const mevent_t events[] =
{
	EVENT_FUNC(tbaby_fly_go),
	EVENT_FUNC(tbaby_fly),
	EVENT_FUNC(tbaby_jump_do),
	EVENT_FUNC(tbaby_explode),
	EVENT_FUNC(tbaby_nohurt),
	NULL
};

/*QUAKED monster_tarbaby (1 0 0) (-16 -16 -24) (16 16 24) Ambush
*/
void q1_monster_tarbaby(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/q1/tarbaby.mdl";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q1/tarbaby.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_death1 = gi.soundindex("q1/blob/death1.wav");
	sound_hit1 = gi.soundindex("q1/blob/hit1.wav");
	sound_land1 = gi.soundindex("q1/blob/land1.wav");
	sound_sight1 = gi.soundindex("q1/blob/sight1.wav");
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 40);
	self->health = 80;
	self->monsterinfo.stand = tbaby_stand;
	self->monsterinfo.walk = tbaby_walk;
	self->monsterinfo.run = tbaby_run;
	self->monsterinfo.attack = tbaby_jump;
	self->monsterinfo.melee = tbaby_jump;
	self->monsterinfo.sight = tbaby_sight;
	self->die = tbaby_die;
	walkmonster_start(self);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
	self->monsterinfo.scale = 1;
	gi.linkentity(self);
}

#endif
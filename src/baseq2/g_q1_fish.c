#include "g_local.h"
#include "m_local.h"

enum
{
	attack1, attack2, attack3, attack4, attack5, attack6,
	attack7, attack8, attack9, attack10, attack11, attack12, attack13,
	attack14, attack15, attack16, attack17, attack18,

	death1, death2, death3, death4, death5, death6, death7,
	death8, death9, death10, death11, death12, death13, death14, death15,
	death16, death17, death18, death19, death20, death21,

	swim1, swim2, swim3, swim4, swim5, swim6, swim7, swim8,
	swim9, swim10, swim11, swim12, swim13, swim14, swim15, swim16, swim17,
	swim18,

	pain1, pain2, pain3, pain4, pain5, pain6, pain7, pain8,
	pain9
};

static q_soundhandle sound_bite;
static q_soundhandle sound_death;
static q_soundhandle sound_idle;

static mscript_t script;

static void f_stand(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
}

static void f_walk(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "walk1");
}

static void fish_idle_sound(edict_t *self)
{
	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_NORM, 0);
}

static void fish_run_skip(edict_t *self)
{
	if (self->s.frame == swim2)
		fish_idle_sound(self);

	self->s.frame++;
}

static void f_run(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "run1");
}

static void fish_melee(edict_t *self)
{
	if (!self->enemy)
		return;		// removed before stroke

	vec3_t delta;
	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);

	if (VectorLength(delta) > 60)
		return;

	gi.sound(self, CHAN_VOICE, sound_bite, 1, ATTN_NORM, 0);
	float ldmg = (random() + random()) * 3;
	T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ldmg, 0, DAMAGE_Q1 | DAMAGE_NO_PARTICLES, MakeGenericMeansOfDeath(self, MD_MELEE, DT_DIRECT));
}

static void f_attack(edict_t *self)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "attack1");
}

static void fish_dead(edict_t *self)
{
	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

static void f_death(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "death1");
}

static void f_pain(edict_t *self, edict_t *other, float damage, int kick)
{
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "pain1");
}

static const mevent_t events[] =
{
	EVENT_FUNC(fish_run_skip),
	EVENT_FUNC(f_run),
	EVENT_FUNC(fish_melee),
	EVENT_FUNC(fish_dead),
	NULL
};

/*QUAKED monster_fish (1 0 0) (-16 -16 -24) (16 16 24) Ambush
*/
void q1_monster_fish(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	const char *model_name = "models/q1/fish.mdl";

	if (!script.initialized)
	{
		const char *script_name = "monsterscripts/q1/fish.mon";
		M_ParseMonsterScript(script_name, model_name, events, &script);
	}

	sound_death = gi.soundindex("q1/fish/death.wav");
	sound_bite = gi.soundindex("q1/fish/bite.wav");
	sound_idle = gi.soundindex("q1/fish/idle.wav");
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->s.modelindex = gi.modelindex(model_name);
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 24);
	self->health = 25;
	self->monsterinfo.stand = f_stand;
	self->monsterinfo.walk = f_walk;
	self->monsterinfo.run = f_run;
	self->die = f_death;
	self->pain = f_pain;
	self->monsterinfo.melee = f_attack;
	self->monsterinfo.currentmove = M_GetMonsterMove(&script, "stand1");
	self->monsterinfo.scale = 1;
	swimmonster_start(self);
	gi.linkentity(self);
}


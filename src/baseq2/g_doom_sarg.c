#include "g_local.h"

static int sound_alert;
static int sound_action;
static int sound_pain;
static int sound_death;
static int sound_attack;

enum {
	frames_stand_start,
	frames_stand_end = frames_stand_start + 5,
	frames_run_start,
	frames_run_end = frames_run_start + 7,
	frames_walk_start,
	frames_walk_end = frames_walk_start + 7,
	frames_gib_start,
	frames_gib_end = frames_gib_start + 15,
	frames_die_start,
	frames_die_end = frames_die_start + 5,
	frames_pain_start,
	frames_pain_end = frames_pain_start + 1,
	frames_shoot_start,
	frames_shoot_end = frames_shoot_start + 6,

	frames_run1 = 0,
	frames_run2,
	frames_run3,
	frames_run4,
	frames_attack1,
	frames_attack2,
	frames_attack3,
	frames_pain,
	frames_death1,
	frames_death2,
	frames_death3,
	frames_death4,
	frames_death5,
	frames_death6
};

#define FRAME_COUNT(type) (frames_##type##_end - frames_##type##_start) + 1

void sarg_idle(edict_t *self)
{
	if ((Q_rand() % 256) < 3)
		gi.sound(self, CHAN_VOICE, sound_action, 1, ATTN_NORM, 0);
}

mframe_t sarg_frames_stand1[FRAME_COUNT(stand)] = {
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 }
};
mmove_t sarg_stand1 = { frames_stand_start, frames_stand_end, sarg_frames_stand1, NULL };

void sarg_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &sarg_stand1;
}

#define MOVE_SPEED 17.5f
#define WALK_SPEED MOVE_SPEED / 2

mframe_t sarg_frames_run1[FRAME_COUNT(run)] = {
	{ ai_run, MOVE_SPEED,  sarg_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  sarg_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  sarg_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  sarg_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  sarg_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  sarg_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  sarg_idle, frames_run4 },
	{ ai_run, MOVE_SPEED,  sarg_idle, frames_run4 }
};
mmove_t sarg_run1 = { frames_run_start, frames_run_end, sarg_frames_run1, NULL };

void sarg_run(edict_t *self)
{
	self->monsterinfo.currentmove = &sarg_run1;
}

mframe_t sarg_frames_walk1[FRAME_COUNT(walk)] = {
	{ ai_walk, WALK_SPEED,  NULL, frames_run1 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run1 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run2 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run2 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run3 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run3 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run4 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run4 }
};
mmove_t sarg_walk1 = { frames_walk_start, frames_walk_end, sarg_frames_walk1, NULL };

void sarg_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &sarg_walk1;
}

void sarg_dead(edict_t *self)
{
	self->nextthink = 0;
	self->svflags |= SVF_DEADMONSTER;
}

mframe_t sarg_frames_die1[FRAME_COUNT(die)] = {
	{ ai_move, 0,  NULL, frames_death1 },
	{ ai_move, 0,  NULL, frames_death2 },
	{ ai_move, 0,  NULL, frames_death3 },
	{ ai_move, 0,  NULL, frames_death4 },
	{ ai_move, 0,  NULL, frames_death5 },
	{ ai_move, 0,  NULL, frames_death6 }
};
mmove_t sarg_die1 = { frames_die_start, frames_die_end, sarg_frames_die1, sarg_dead };

void sarg_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;

	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);

	self->monsterinfo.currentmove = &sarg_die1;

	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	gi.linkentity(self);
}

mframe_t sarg_frames_pain1[FRAME_COUNT(pain)] = {
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain }
};
mmove_t sarg_pain1 = { frames_pain_start, frames_pain_end, sarg_frames_pain1, sarg_run };

void sarg_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (random() < 0.7831f)
	{
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

		if (self->pain_debounce_time > level.time)
			return;

		self->monsterinfo.currentmove = &sarg_pain1;
	}
}

//============================================================================

bool P_CheckMeleeRange(edict_t *self);
int Doom_MissileDamageRandomizer(int);

void sarg_eat(edict_t *self)
{
	if (P_CheckMeleeRange(self))
		T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ((Q_rand() % 10) + 1) * 4, 0, DAMAGE_NO_PARTICLES, MakeAttackerMeansOfDeath(self, self, MD_MELEE, DT_DIRECT));
}

mframe_t sarg_frames_shoot1[FRAME_COUNT(shoot)] = {
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack2 },
	{ ai_charge, 0,  NULL, frames_attack2 },
	{ ai_charge, 0,  sarg_eat, frames_attack3 },
	{ ai_charge, 0,  NULL, frames_attack3 }
};
mmove_t sarg_shoot1 = { frames_shoot_start, frames_shoot_end, sarg_frames_shoot1, sarg_run };

void sarg_attack(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &sarg_shoot1;
}

void sarg_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_alert, 1, ATTN_NORM, 0);
}

//===========================================================================

/*QUAKED doom_monster_sarg (1 0 0) (-32 -32 -24) (32 32 40) Ambush

*/
void doom_monster_sarg(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;

	sound_alert = gi.soundindex("doom/SGTSIT.wav");
	sound_action = gi.soundindex("doom/DMACT.wav");
	sound_pain = gi.soundindex("doom/DMPAIN.wav");
	sound_death = gi.soundindex("doom/SGTDTH.wav");
	sound_attack = gi.soundindex("doom/SGTATK.wav");

	VectorSet(self->mins, -30, -30, -4);
	VectorSet(self->maxs, 30, 30, 52);

	self->s.modelindex = gi.modelindex("sprites/doom/sarg.d2s");
	self->health = 150;
	self->dmg = 0;

	self->monsterinfo.stand = sarg_stand;
	self->monsterinfo.walk = sarg_walk;
	self->monsterinfo.run = sarg_run;
	self->monsterinfo.sight = sarg_sight;
	self->pain = sarg_pain;
	self->die = sarg_die;
	self->monsterinfo.melee = sarg_attack;
	self->monsterinfo.special_frames = true;
	self->s.game = GAME_DOOM;

	if (self->entitytype == ET_DOOM_MONSTER_SPECTRE)
		self->s.renderfx |= RF_SPECTRE;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &sarg_stand1;
	self->monsterinfo.scale = 1;

	walkmonster_start(self);
}

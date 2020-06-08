#ifdef ENABLE_COOP
#include "g_local.h"

static q_soundhandle sound_alert;
static q_soundhandle sound_chase;
static q_soundhandle sound_action;
static q_soundhandle sound_pain;
static q_soundhandle sound_death;

enum
{
	frames_stand_start,
	frames_stand_end = frames_stand_start + 5,
	frames_run_start,
	frames_run_end = frames_run_start + 11,
	frames_walk_start,
	frames_walk_end = frames_walk_start + 11,
	frames_die_start,
	frames_die_end = frames_die_start + 21,
	frames_pain_start,
	frames_pain_end = frames_pain_start + 1,
	frames_shoot_start,
	frames_shoot_end = frames_shoot_start + 6,

	frames_run1 = 0,
	frames_run2,
	frames_run3,
	frames_run4,
	frames_run5,
	frames_run6,
	frames_attack1,
	frames_attack2,
	frames_pain,
	frames_death1,
	frames_death2,
	frames_death3,
	frames_death4,
	frames_death5,
	frames_death6,
	frames_death7,
	frames_death8,
	frames_death9,
	frames_death10
};

#define FRAME_COUNT(type) (frames_##type##_end - frames_##type##_start) + 1

void spid_idle(edict_t *self)
{
	if (self->last_move_time < level.time)
	{
		gi.sound(self, CHAN_BODY, sound_chase, 1, ATTN_NORM, 0);
		self->last_move_time = level.time + 800;
	}
}

mframe_t spid_frames_stand1[FRAME_COUNT(stand)] =
{
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 }
};
mmove_t spid_stand1 = { frames_stand_start, frames_stand_end, spid_frames_stand1, NULL };

void spid_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &spid_stand1;
}

#define MOVE_SPEED 14.0f
#define WALK_SPEED MOVE_SPEED / 2

mframe_t spid_frames_run1[FRAME_COUNT(run)] =
{
	{ ai_run, MOVE_SPEED,  spid_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  spid_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  spid_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  spid_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  spid_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  spid_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  spid_idle, frames_run4 },
	{ ai_run, MOVE_SPEED,  spid_idle, frames_run4 },
	{ ai_run, MOVE_SPEED,  spid_idle, frames_run5 },
	{ ai_run, MOVE_SPEED,  spid_idle, frames_run5 },
	{ ai_run, MOVE_SPEED,  spid_idle, frames_run6 },
	{ ai_run, MOVE_SPEED,  spid_idle, frames_run6 }
};
mmove_t spid_run1 = { frames_run_start, frames_run_end, spid_frames_run1, NULL };

void spid_run(edict_t *self)
{
	self->monsterinfo.currentmove = &spid_run1;
}

mframe_t spid_frames_walk1[FRAME_COUNT(walk)] =
{
	{ ai_walk, WALK_SPEED,  NULL, frames_run1 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run1 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run2 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run2 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run3 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run3 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run4 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run4 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run5 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run5 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run6 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run6 }
};
mmove_t spid_walk1 = { frames_walk_start, frames_walk_end, spid_frames_walk1, NULL };

void spid_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &spid_walk1;
}

void spid_dead(edict_t *self)
{
	self->nextthink = 0;
	self->server.flags.deadmonster = true;
}

mframe_t spid_frames_die1[FRAME_COUNT(die)] =
{
	{ ai_move, 0,  NULL, frames_death1 },
	{ ai_move, 0,  NULL, frames_death1 },
	{ ai_move, 0,  NULL, frames_death1 },
	{ ai_move, 0,  NULL, frames_death1 },
	{ ai_move, 0,  NULL, frames_death2 },
	{ ai_move, 0,  NULL, frames_death2 },
	{ ai_move, 0,  NULL, frames_death3 },
	{ ai_move, 0,  NULL, frames_death3 },
	{ ai_move, 0,  NULL, frames_death4 },
	{ ai_move, 0,  NULL, frames_death4 },
	{ ai_move, 0,  NULL, frames_death5 },
	{ ai_move, 0,  NULL, frames_death5 },
	{ ai_move, 0,  NULL, frames_death6 },
	{ ai_move, 0,  NULL, frames_death6 },
	{ ai_move, 0,  NULL, frames_death7 },
	{ ai_move, 0,  NULL, frames_death7 },
	{ ai_move, 0,  NULL, frames_death8 },
	{ ai_move, 0,  NULL, frames_death8 },
	{ ai_move, 0,  NULL, frames_death9 },
	{ ai_move, 0,  NULL, frames_death9 },
	{ ai_move, 0,  NULL, frames_death10 },
	{ ai_move, 0,  NULL, frames_death10 }
};
mmove_t spid_die1 = { frames_die_start, frames_die_end, spid_frames_die1, spid_dead };

void spid_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &spid_die1;
	self->takedamage = false;
	self->server.solid = SOLID_NOT;
	gi.linkentity(self);
}

mframe_t spid_frames_pain1[FRAME_COUNT(pain)] =
{
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain }
};
mmove_t spid_pain1 = { frames_pain_start, frames_pain_end, spid_frames_pain1, spid_run };

void spid_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (random() < 0.1563f)
	{
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

		if (self->pain_debounce_time > level.time)
			return;

		self->monsterinfo.currentmove = &spid_pain1;
	}
}

//============================================================================
void poss_fire_gun(edict_t *self);

void spid_fire_gun(edict_t *self)
{
	poss_fire_gun(self);

	if (self->server.state.frame == frames_attack1 && self->enemy && self->enemy->health > 0 && visible(self, self->enemy))
		self->monsterinfo.nextframe = frames_shoot_end - 1;
}

mframe_t spid_frames_shoot1[FRAME_COUNT(shoot)] =
{
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  spid_fire_gun, frames_attack2 },
	{ ai_charge, 0,  spid_fire_gun, frames_attack1 }
};
mmove_t spid_shoot1 = { frames_shoot_start, frames_shoot_end, spid_frames_shoot1, spid_run };

void spid_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &spid_shoot1;
}

void spid_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_alert, 1, ATTN_NORM, 0);
}

//===========================================================================

/*QUAKED doom_monster_spid (1 0 0) (-32 -32 -24) (32 32 40) Ambush

*/
void doom_monster_spid(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	self->server.solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	sound_alert = gi.soundindex("doom/SPISIT.wav");
	sound_chase = gi.soundindex("doom/METAL.wav");
	sound_action = gi.soundindex("doom/DMACT.wav");
	sound_pain = gi.soundindex("doom/DMPAIN.wav");
	sound_death = gi.soundindex("doom/SPIDTH.wav");
	VectorSet(self->server.mins, -128, -128, -4);
	VectorSet(self->server.maxs, 128, 128, 96);
	self->server.state.modelindex = gi.modelindex("sprites/doom/spid.d2s");
	self->health = 3000;
	self->mass = 1000;
	self->dmg = 1;
	self->monsterinfo.stand = spid_stand;
	self->monsterinfo.walk = spid_walk;
	self->monsterinfo.run = spid_run;
	self->monsterinfo.sight = spid_sight;
	self->pain = spid_pain;
	self->die = spid_die;
	self->monsterinfo.attack = spid_attack;
	self->monsterinfo.special_frames = true;
	self->server.state.game = GAME_DOOM;
	gi.linkentity(self);
	self->monsterinfo.currentmove = &spid_stand1;
	self->monsterinfo.scale = 1;
	walkmonster_start(self);
}

#endif
#ifdef ENABLE_COOP
#include "g_local.h"

static q_soundhandle sound_active;
static q_soundhandle sound_alert;
static q_soundhandle sound_metal;
static q_soundhandle sound_hoof;
static q_soundhandle sound_pain;
static q_soundhandle sound_death;

enum
{
	frames_stand_start,
	frames_stand_end = frames_stand_start + 5,
	frames_run_start,
	frames_run_end = frames_run_start + 7,
	frames_walk_start,
	frames_walk_end = frames_walk_start + 7,
	frames_die_start,
	frames_die_end = frames_die_start + 19,
	frames_pain_start,
	frames_pain_end = frames_pain_start + 1,
	frames_shoot_start,
	frames_shoot_end = frames_shoot_start + 18,

	frames_run1 = 0,
	frames_run2,
	frames_run3,
	frames_run4,
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
	frames_death9
};

#define FRAME_COUNT(type) (frames_##type##_end - frames_##type##_start) + 1

void cybr_metal(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_metal, 1, ATTN_NORM, 0);
}

void cybr_hoof(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_hoof, 1, ATTN_NORM, 0);
}

void cybr_idle(edict_t *self)
{
	if ((Q_rand() % 256) < 3)
		gi.sound(self, CHAN_VOICE, sound_active, 1, ATTN_NORM, 0);
}

mframe_t cybr_frames_stand1[FRAME_COUNT(stand)] =
{
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 }
};
mmove_t cybr_stand1 = { frames_stand_start, frames_stand_end, cybr_frames_stand1, NULL };

void cybr_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &cybr_stand1;
}

#define MOVE_SPEED 14.0f
#define WALK_SPEED MOVE_SPEED / 2

mframe_t cybr_frames_run1[FRAME_COUNT(run)] =
{
	{ ai_run, MOVE_SPEED,  cybr_hoof, frames_run1 },
	{ ai_run, MOVE_SPEED,  cybr_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  cybr_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  cybr_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  cybr_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  cybr_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  cybr_metal, frames_run4 },
	{ ai_run, MOVE_SPEED,  cybr_idle, frames_run4 }
};
mmove_t cybr_run1 = { frames_run_start, frames_run_end, cybr_frames_run1, NULL };

void cybr_run(edict_t *self)
{
	self->monsterinfo.currentmove = &cybr_run1;
}

mframe_t cybr_frames_walk1[FRAME_COUNT(walk)] =
{
	{ ai_walk, WALK_SPEED,  cybr_hoof, frames_run1 },
	{ ai_walk, WALK_SPEED,  cybr_idle, frames_run1 },
	{ ai_walk, WALK_SPEED,  cybr_idle, frames_run2 },
	{ ai_walk, WALK_SPEED,  cybr_idle, frames_run2 },
	{ ai_walk, WALK_SPEED,  cybr_idle, frames_run3 },
	{ ai_walk, WALK_SPEED,  cybr_idle, frames_run3 },
	{ ai_walk, WALK_SPEED,  cybr_metal, frames_run4 },
	{ ai_walk, WALK_SPEED,  cybr_idle, frames_run4 }
};
mmove_t cybr_walk1 = { frames_walk_start, frames_walk_end, cybr_frames_walk1, NULL };

void cybr_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &cybr_walk1;
}

void cybr_dead(edict_t *self)
{
	self->nextthink = 0;
	self->svflags |= SVF_DEADMONSTER;
}

mframe_t cybr_frames_die1[FRAME_COUNT(die)] =
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
};
mmove_t cybr_die1 = { frames_die_start, frames_die_end, cybr_frames_die1, cybr_dead };

void cybr_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &cybr_die1;
	self->takedamage = false;
	self->solid = SOLID_NOT;
	gi.linkentity(self);
}

mframe_t cybr_frames_pain1[FRAME_COUNT(pain)] =
{
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain }
};
mmove_t cybr_pain1 = { frames_pain_start, frames_pain_end, cybr_frames_pain1, cybr_run };

void cybr_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (random() < 0.0784313f)
	{
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

		if (self->pain_debounce_time > level.time)
			return;

		self->monsterinfo.currentmove = &cybr_pain1;
	}
}

//============================================================================
void fire_doom_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int radius_damage, int speed);
int Doom_MissileDamageRandomizer(int);

void cybr_fire_gun(edict_t *self)
{
	G_SendMuzzleFlash(self, MZ_ROCKET);
	vec3_t start, forward, right, offset;
	VectorSubtract(self->enemy->s.origin, self->s.origin, forward);
	VectorNormalize(forward);
	VectorSet(offset, 0, 0, self->viewheight);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
	fire_doom_rocket(self, start, forward, Doom_MissileDamageRandomizer(20), 128, 650);
}

mframe_t cybr_frames_shoot1[FRAME_COUNT(shoot)] =
{
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  cybr_fire_gun, frames_attack2 },
	{ ai_charge, 0,  NULL, frames_attack2 },
	{ ai_charge, 0,  NULL, frames_attack2 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  cybr_fire_gun, frames_attack2 },
	{ ai_charge, 0,  NULL, frames_attack2 },
	{ ai_charge, 0,  NULL, frames_attack2 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  cybr_fire_gun, frames_attack2 },
	{ ai_charge, 0,  NULL, frames_attack2 },
	{ ai_charge, 0,  NULL, frames_attack2 }
};
mmove_t cybr_shoot1 = { frames_shoot_start, frames_shoot_end, cybr_frames_shoot1, cybr_run };

void cybr_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &cybr_shoot1;
}

void cybr_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_alert, 1, ATTN_NORM, 0);
}

//===========================================================================

/*QUAKED doom_monster_cybr (1 0 0) (-32 -32 -24) (32 32 40) Ambush

*/
void doom_monster_cybr(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	sound_active = gi.soundindex("doom/DMACT.wav");
	sound_alert = gi.soundindex("doom/CYBSIT.wav");
	sound_metal = gi.soundindex("doom/METAL.wav");
	sound_hoof = gi.soundindex("doom/HOOF.wav");
	sound_pain = gi.soundindex("doom/DMPAIN.wav");
	sound_death = gi.soundindex("doom/CYBDTH.wav");
	VectorSet(self->mins, -20, -20, -4);
	VectorSet(self->maxs, 20, 20, 106);
	self->s.modelindex = gi.modelindex("sprites/doom/cybr.d2s");
	self->health = 4000;
	self->mass = 1000;
	self->dmg = 1;
	self->monsterinfo.stand = cybr_stand;
	self->monsterinfo.walk = cybr_walk;
	self->monsterinfo.run = cybr_run;
	self->monsterinfo.sight = cybr_sight;
	self->pain = cybr_pain;
	self->die = cybr_die;
	self->monsterinfo.attack = cybr_attack;
	self->monsterinfo.special_frames = true;
	self->s.game = GAME_DOOM;
	gi.linkentity(self);
	self->monsterinfo.currentmove = &cybr_stand1;
	self->monsterinfo.scale = 1;
	walkmonster_start(self);
}

#endif
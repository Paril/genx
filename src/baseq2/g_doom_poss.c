#ifdef ENABLE_COOP
#include "g_local.h"

static q_soundhandle sound_alert1;
static q_soundhandle sound_alert2;
static q_soundhandle sound_alert3;
static q_soundhandle sound_action;
static q_soundhandle sound_pain;
static q_soundhandle sound_death1;
static q_soundhandle sound_death2;
static q_soundhandle sound_death3;
static q_soundhandle sound_gib;

enum
{
	frames_stand_start,
	frames_stand_end = frames_stand_start + 5,
	frames_run_start,
	frames_run_end = frames_run_start + 7,
	frames_walk_start,
	frames_walk_end = frames_walk_start + 7,
	frames_gib_start,
	frames_gib_end = frames_gib_start + 17,
	frames_die_start,
	frames_die_end = frames_die_start + 4,
	frames_pain_start,
	frames_pain_end = frames_pain_start + 2,
	frames_shoot_start,
	frames_shoot_end = frames_shoot_start + 4,

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
	frames_gib1,
	frames_gib2,
	frames_gib3,
	frames_gib4,
	frames_gib5,
	frames_gib6,
	frames_gib7,
	frames_gib8,
	frames_gib9
};

#define FRAME_COUNT(type) (frames_##type##_end - frames_##type##_start) + 1

void poss_idle(edict_t *self)
{
	if ((Q_rand() % 256) < 3)
		gi.sound(self, CHAN_VOICE, sound_action, 1, ATTN_NORM, 0);
}

mframe_t poss_frames_stand1[FRAME_COUNT(stand)] =
{
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 }
};
mmove_t poss_stand1 = { frames_stand_start, frames_stand_end, poss_frames_stand1, NULL };

void poss_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &poss_stand1;
}

#define MOVE_SPEED 9.3f
#define WALK_SPEED MOVE_SPEED / 2

mframe_t poss_frames_run1[FRAME_COUNT(run)] =
{
	{ ai_run, MOVE_SPEED,  poss_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  poss_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  poss_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  poss_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  poss_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  poss_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  poss_idle, frames_run4 },
	{ ai_run, MOVE_SPEED,  poss_idle, frames_run4 }
};
mmove_t poss_run1 = { frames_run_start, frames_run_end, poss_frames_run1, NULL };

void poss_run(edict_t *self)
{
	self->monsterinfo.currentmove = &poss_run1;
}

mframe_t poss_frames_walk1[FRAME_COUNT(walk)] =
{
	{ ai_walk, WALK_SPEED,  NULL, frames_run1 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run1 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run2 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run2 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run3 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run3 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run4 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run4 }
};
mmove_t poss_walk1 = { frames_walk_start, frames_walk_end, poss_frames_walk1, NULL };

void poss_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &poss_walk1;
}

void poss_dead(edict_t *self)
{
	self->nextthink = 0;
	self->server.flags.deadmonster = true;
}

mframe_t poss_frames_gib1[FRAME_COUNT(gib)] =
{
	{ ai_move, 0,  NULL, frames_gib1 },
	{ ai_move, 0,  NULL, frames_gib1 },
	{ ai_move, 0,  NULL, frames_gib2 },
	{ ai_move, 0,  NULL, frames_gib2 },
	{ ai_move, 0,  NULL, frames_gib3 },
	{ ai_move, 0,  NULL, frames_gib3 },
	{ ai_move, 0,  NULL, frames_gib4 },
	{ ai_move, 0,  NULL, frames_gib4 },
	{ ai_move, 0,  NULL, frames_gib5 },
	{ ai_move, 0,  NULL, frames_gib5 },
	{ ai_move, 0,  NULL, frames_gib6 },
	{ ai_move, 0,  NULL, frames_gib6 },
	{ ai_move, 0,  NULL, frames_gib7 },
	{ ai_move, 0,  NULL, frames_gib7 },
	{ ai_move, 0,  NULL, frames_gib8 },
	{ ai_move, 0,  NULL, frames_gib8 },
	{ ai_move, 0,  NULL, frames_gib9 },
	{ ai_move, 0,  NULL, frames_gib9 }
};
mmove_t poss_gib1 = { frames_gib_start, frames_gib_end, poss_frames_gib1, poss_dead };

mframe_t poss_frames_die1[FRAME_COUNT(die)] =
{
	{ ai_move, 0,  NULL, frames_death1 },
	{ ai_move, 0,  NULL, frames_death2 },
	{ ai_move, 0,  NULL, frames_death3 },
	{ ai_move, 0,  NULL, frames_death4 },
	{ ai_move, 0,  NULL, frames_death5 }
};
mmove_t poss_die1 = { frames_die_start, frames_die_end, poss_frames_die1, poss_dead };

void Doom_TossItem(edict_t *self, itemid_e item, int new_count)
{
	edict_t *ent = Drop_Item(self, GetItemByIndex(item));

	if (new_count)
		ent->count = new_count;
}

void poss_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;

	// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, sound_gib, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &poss_gib1;
	}
	else
	{
		switch (Q_rand() % 3)
		{
			case 0:
				gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
				break;

			case 1:
				gi.sound(self, CHAN_VOICE, sound_death2, 1, ATTN_NORM, 0);
				break;

			case 2:
				gi.sound(self, CHAN_VOICE, sound_death3, 1, ATTN_NORM, 0);
				break;
		}

		self->monsterinfo.currentmove = &poss_die1;
	}

	/*if (self->dmg == 1)
		Doom_TossItem(self, ITI_DOOM_SHOTGUN, 4);
	else
		Doom_TossItem(self, ITI_BULLETS, 5);*/
	self->takedamage = false;
	self->server.solid = SOLID_NOT;
	gi.linkentity(self);
}

mframe_t poss_frames_pain1[FRAME_COUNT(pain)] =
{
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain }
};
mmove_t poss_pain1 = { frames_pain_start, frames_pain_end, poss_frames_pain1, poss_run };

void poss_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (random() < ((self->dmg == 1) ? 0.6641f : 0.7813f))
	{
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

		if (self->pain_debounce_time > level.time)
			return;

		self->monsterinfo.currentmove = &poss_pain1;
	}
}

//============================================================================

void fire_doom_shotgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int count, int hspread, int vspread, meansOfDeath_t mod);

void poss_fire_gun(edict_t *self)
{
	G_SendMuzzleFlash(self, self->dmg == 1 ? MZ_SHOTGUN : MZ_BLASTER);
	vec3_t start, forward, right, offset;
	VectorSubtract(self->enemy->server.state.origin, self->server.state.origin, forward);
	VectorNormalize(forward);
	VectorSet(offset, 0, 0, self->viewheight);
	G_ProjectSource(self->server.state.origin, offset, forward, right, start);
	int damage = ((Q_rand() % 5) + 1) * 3;
	fire_doom_shotgun(self, start, forward, damage, 0, TE_DOOM_GUNSHOT, (self->dmg == 1 ? 3 : 1), 800, 0, MakeAttackerMeansOfDeath(self, self, MD_NONE, DT_DIRECT));
}

mframe_t poss_frames_shoot1[FRAME_COUNT(shoot)] =
{
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  poss_fire_gun, frames_attack2 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 }
};
mmove_t poss_shoot1 = { frames_shoot_start, frames_shoot_end, poss_frames_shoot1, poss_run };

void poss_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &poss_shoot1;
}

void poss_sight(edict_t *self, edict_t *other)
{
	switch (Q_rand() % 3)
	{
		case 0:
			gi.sound(self, CHAN_VOICE, sound_alert1, 1, ATTN_NORM, 0);
			break;

		case 1:
			gi.sound(self, CHAN_VOICE, sound_alert2, 1, ATTN_NORM, 0);
			break;

		case 2:
			gi.sound(self, CHAN_VOICE, sound_alert3, 1, ATTN_NORM, 0);
			break;
	}
}

//===========================================================================

/*QUAKED doom_monster_poss (1 0 0) (-32 -32 -24) (32 32 40) Ambush

*/
void doom_monster_poss(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	self->server.solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	sound_gib = gi.soundindex("doom/SLOP.wav");
	sound_alert1 = gi.soundindex("doom/POSIT1.wav");
	sound_alert2 = gi.soundindex("doom/POSIT2.wav");
	sound_alert3 = gi.soundindex("doom/POSIT3.wav");
	sound_action = gi.soundindex("doom/POSACT.wav");
	sound_pain = gi.soundindex("doom/POPAIN.wav");
	sound_death1 = gi.soundindex("doom/PODTH1.wav");
	sound_death2 = gi.soundindex("doom/PODTH2.wav");
	sound_death3 = gi.soundindex("doom/PODTH3.wav");
	VectorSet(self->server.mins, -20, -20, -2);
	VectorSet(self->server.maxs, 20, 20, 62);

	if (self->entitytype == ET_DOOM_MONSTER_SPOS)
	{
		self->server.state.modelindex = gi.modelindex("sprites/doom/SPOS.d2s");
		self->health = 30;
		self->dmg = 1;
	}
	else
	{
		self->server.state.modelindex = gi.modelindex("sprites/doom/POSS.d2s");
		self->health = 20;
		self->dmg = 0;
	}

	self->gib_health = -self->health;
	self->monsterinfo.stand = poss_stand;
	self->monsterinfo.walk = poss_walk;
	self->monsterinfo.run = poss_run;
	self->monsterinfo.sight = poss_sight;
	self->pain = poss_pain;
	self->die = poss_die;
	self->monsterinfo.attack = poss_attack;
	self->monsterinfo.special_frames = true;
	self->server.state.game = GAME_DOOM;
	gi.linkentity(self);
	self->monsterinfo.currentmove = &poss_stand1;
	self->monsterinfo.scale = 1;
	walkmonster_start(self);
}

#endif
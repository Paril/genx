#include "g_local.h"

static int sound_alert1;
static int sound_alert2;
static int sound_alert3;
static int sound_action;
static int sound_pain;
static int sound_death1;
static int sound_death2;
static int sound_death3;
static int sound_gib;

enum {
	frames_stand_start,
	frames_stand_end = frames_stand_start + 5,
	frames_run_start,
	frames_run_end = frames_run_start + 7,
	frames_walk_start,
	frames_walk_end = frames_walk_start + 7,
	frames_gib_start,
	frames_gib_end = frames_gib_start + 11,
	frames_die_start,
	frames_die_end = frames_die_start + 6,
	frames_pain_start,
	frames_pain_end = frames_pain_start + 2,
	frames_shoot_start,
	frames_shoot_end = frames_shoot_start + 3,

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
	frames_gib1,
	frames_gib2,
	frames_gib3,
	frames_gib4,
	frames_gib5,
	frames_gib6,
};

#define FRAME_COUNT(type) (frames_##type##_end - frames_##type##_start) + 1

void cpos_idle(edict_t *self)
{
	if ((Q_rand() % 256) < 3)
		gi.sound(self, CHAN_VOICE, sound_action, 1, ATTN_NORM, 0);
}

mframe_t cpos_frames_stand1[FRAME_COUNT(stand)] = {
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 }
};
mmove_t cpos_stand1 = { frames_stand_start, frames_stand_end, cpos_frames_stand1, NULL };

void cpos_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &cpos_stand1;
}

#define MOVE_SPEED 9.3f
#define WALK_SPEED MOVE_SPEED / 2

mframe_t cpos_frames_run1[FRAME_COUNT(run)] = {
	{ ai_run, MOVE_SPEED,  cpos_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  cpos_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  cpos_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  cpos_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  cpos_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  cpos_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  cpos_idle, frames_run4 },
	{ ai_run, MOVE_SPEED,  cpos_idle, frames_run4 }
};
mmove_t cpos_run1 = { frames_run_start, frames_run_end, cpos_frames_run1, NULL };

void cpos_run(edict_t *self)
{
	self->monsterinfo.currentmove = &cpos_run1;
}

mframe_t cpos_frames_walk1[FRAME_COUNT(walk)] = {
	{ ai_walk, WALK_SPEED,  NULL, frames_run1 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run1 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run2 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run2 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run3 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run3 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run4 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run4 }
};
mmove_t cpos_walk1 = { frames_walk_start, frames_walk_end, cpos_frames_walk1, NULL };

void cpos_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &cpos_walk1;
}

void cpos_dead(edict_t *self)
{
	self->nextthink = 0;
	self->svflags |= SVF_DEADMONSTER;
}

mframe_t cpos_frames_gib1[FRAME_COUNT(gib)] = {
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
	{ ai_move, 0,  NULL, frames_gib6 }
};
mmove_t cpos_gib1 = { frames_gib_start, frames_gib_end, cpos_frames_gib1, cpos_dead };

mframe_t cpos_frames_die1[FRAME_COUNT(die)] = {
	{ ai_move, 0,  NULL, frames_death1 },
	{ ai_move, 0,  NULL, frames_death2 },
	{ ai_move, 0,  NULL, frames_death3 },
	{ ai_move, 0,  NULL, frames_death4 },
	{ ai_move, 0,  NULL, frames_death5 },
	{ ai_move, 0,  NULL, frames_death6 },
	{ ai_move, 0,  NULL, frames_death7 }
};
mmove_t cpos_die1 = { frames_die_start, frames_die_end, cpos_frames_die1, cpos_dead };

void Doom_TossItem(edict_t *self, itemid_e item, int new_count);

void cpos_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;
	Doom_TossItem(self, ITI_DOOM_CHAINGUN, 20);

	// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, sound_gib, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &cpos_gib1;
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

		self->monsterinfo.currentmove = &cpos_die1;
	}

	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	gi.linkentity(self);
}

mframe_t cpos_frames_pain1[FRAME_COUNT(pain)] = {
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain }
};
mmove_t cpos_pain1 = { frames_pain_start, frames_pain_end, cpos_frames_pain1, cpos_run };

void cpos_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (random() < 0.6641f)
	{
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

		if (self->pain_debounce_time > level.time)
			return;

		self->monsterinfo.currentmove = &cpos_pain1;
	}
}

//============================================================================

void fire_doom_bullet(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, meansOfDeath_t mod);

void cpos_fire_gun(edict_t *self)
{
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(MZ_SHOTGUN);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	vec3_t start, forward, right, offset;

	VectorSubtract(self->enemy->s.origin, self->s.origin, forward);
	VectorNormalize(forward);

	VectorSet(offset, 0, 0, self->viewheight);

	G_ProjectSource(self->s.origin, offset, forward, right, start);

	int damage = ((Q_rand() % 5) + 1) * 3;
	fire_doom_bullet(self, start, forward, damage, 0, TE_DOOM_GUNSHOT, 800, 0, MakeAttackerMeansOfDeath(self, self, MD_NONE, DT_DIRECT));
}

void cpos_refire(edict_t *self)
{
	if (!self->enemy || self->enemy->health <= 0 || !visible(self, self->enemy))
	{
		cpos_run(self);
		return;
	}
}

mframe_t cpos_frames_shoot1[FRAME_COUNT(shoot)] = {
	{ ai_charge, 0,  cpos_fire_gun, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack2 },
	{ ai_charge, 0,  cpos_fire_gun, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack2 }
};
mmove_t cpos_shoot1 = { frames_shoot_start, frames_shoot_end, cpos_frames_shoot1, cpos_refire };

void cpos_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &cpos_shoot1;
}

void cpos_sight(edict_t *self, edict_t *other)
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

/*QUAKED doom_monster_cpos (1 0 0) (-32 -32 -24) (32 32 40) Ambush

*/
void doom_monster_cpos(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_BBOX;
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

	VectorSet(self->mins, -20, -20, -2);
	VectorSet(self->maxs, 20, 20, 62);

	self->s.modelindex = gi.modelindex("sprites/doom/CPOS.d2s");
	self->health = 70;

	self->gib_health = -self->health;

	self->monsterinfo.stand = cpos_stand;
	self->monsterinfo.walk = cpos_walk;
	self->monsterinfo.run = cpos_run;
	self->monsterinfo.sight = cpos_sight;
	self->pain = cpos_pain;
	self->die = cpos_die;
	self->monsterinfo.attack = cpos_attack;
	self->monsterinfo.special_frames = true;
	self->s.game = GAME_DOOM;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &cpos_stand1;
	self->monsterinfo.scale = 1;

	walkmonster_start(self);
}

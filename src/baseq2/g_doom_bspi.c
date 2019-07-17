#include "g_local.h"

static q_soundhandle sound_alert;
static q_soundhandle sound_chase;
static q_soundhandle sound_action;
static q_soundhandle sound_pain;
static q_soundhandle sound_death;
static q_soundhandle sound_shoot;

enum
{
	frames_stand_start,
	frames_stand_end = frames_stand_start + 5,
	frames_run_start,
	frames_run_end = frames_run_start + 11,
	frames_walk_start,
	frames_walk_end = frames_walk_start + 11,
	frames_die_start,
	frames_die_end = frames_die_start + 14,
	frames_pain_start,
	frames_pain_end = frames_pain_start + 1,
	frames_shoot_start,
	frames_shoot_end = frames_shoot_start + 7,

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
	frames_death7
};

#define FRAME_COUNT(type) (frames_##type##_end - frames_##type##_start) + 1

void bspi_idle(edict_t *self)
{
	if (self->last_move_time < level.time)
	{
		gi.sound(self, CHAN_BODY, sound_chase, 1, ATTN_NORM, 0);
		self->last_move_time = level.time + 400;
	}
}

mframe_t bspi_frames_stand1[FRAME_COUNT(stand)] =
{
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 }
};
mmove_t bspi_stand1 = { frames_stand_start, frames_stand_end, bspi_frames_stand1, NULL };

void bspi_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &bspi_stand1;
}

#define MOVE_SPEED 14.0f
#define WALK_SPEED MOVE_SPEED / 2

mframe_t bspi_frames_run1[FRAME_COUNT(run)] =
{
	{ ai_run, MOVE_SPEED,  bspi_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  bspi_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  bspi_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  bspi_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  bspi_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  bspi_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  bspi_idle, frames_run4 },
	{ ai_run, MOVE_SPEED,  bspi_idle, frames_run4 },
	{ ai_run, MOVE_SPEED,  bspi_idle, frames_run5 },
	{ ai_run, MOVE_SPEED,  bspi_idle, frames_run5 },
	{ ai_run, MOVE_SPEED,  bspi_idle, frames_run6 },
	{ ai_run, MOVE_SPEED,  bspi_idle, frames_run6 }
};
mmove_t bspi_run1 = { frames_run_start, frames_run_end, bspi_frames_run1, NULL };

void bspi_run(edict_t *self)
{
	self->monsterinfo.currentmove = &bspi_run1;
}

mframe_t bspi_frames_walk1[FRAME_COUNT(walk)] =
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
mmove_t bspi_walk1 = { frames_walk_start, frames_walk_end, bspi_frames_walk1, NULL };

void bspi_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &bspi_walk1;
}

void bspi_dead(edict_t *self)
{
	self->nextthink = 0;
	self->svflags |= SVF_DEADMONSTER;
}

mframe_t bspi_frames_die1[FRAME_COUNT(die)] =
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
	{ ai_move, 0,  NULL, frames_death7 }
};
mmove_t bspi_die1 = { frames_die_start, frames_die_end, bspi_frames_die1, bspi_dead };

void bspi_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &bspi_die1;
	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	gi.linkentity(self);
}

mframe_t bspi_frames_pain1[FRAME_COUNT(pain)] =
{
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain }
};
mmove_t bspi_pain1 = { frames_pain_start, frames_pain_end, bspi_frames_pain1, bspi_run };

void bspi_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (random() < 0.5f)
	{
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

		if (self->pain_debounce_time > level.time)
			return;

		self->monsterinfo.currentmove = &bspi_pain1;
	}
}

//============================================================================

void doom_bspi_ball_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	if (other->takedamage && other->entitytype != ET_DOOM_MONSTER_BSPI)
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);

	VectorNormalize(ent->velocity);
	VectorMA(ent->s.origin, -8, ent->velocity, origin);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_DOOM_BSPI_BOOM);
	MSG_WritePos(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);
	G_FreeEdict(ent);
}

void fire_doom_bspi_ball(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t *rocket;
	rocket = G_Spawn();
	VectorCopy(start, rocket->s.origin);
	VectorCopy(dir, rocket->movedir);
	vectoangles(dir, rocket->s.angles);
	VectorScale(dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->clipmask = MASK_SHOT;
	rocket->solid = SOLID_BBOX;
	rocket->s.effects |= EF_ANIM01;
	rocket->s.renderfx |= RF_FULLBRIGHT;
	rocket->s.game = GAME_DOOM;
	VectorClear(rocket->mins);
	VectorClear(rocket->maxs);
	rocket->s.modelindex = gi.modelindex("sprites/doom/APLS.d2s");
	rocket->owner = self;
	rocket->touch = doom_bspi_ball_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, self, MD_NONE, DT_DIRECT);
	gi.linkentity(rocket);
}

int Doom_MissileDamageRandomizer(int);

void bspi_fire_gun(edict_t *self)
{
	if (self->style == frames_shoot_end - 1)
	{
		if (visible(self, self->enemy) && self->enemy->health > 0)
			self->monsterinfo.nextframe = frames_shoot_start + 1;

		return;
	}

	if (self->s.frame != frames_attack2)
		return;

	gi.sound(self, CHAN_WEAPON, sound_shoot, 1, ATTN_NORM, 0);
	vec3_t org, v_forward, v_right;
	AngleVectors(self->s.angles, v_forward, v_right, NULL);

	for (int i = 0; i < 3; ++i)
		org[i] = self->s.origin[i] + v_forward[i] * 0 + v_right[i] * 0;

	org[2] += 8;
	vec3_t dir;
	vec3_t enemy_org;
	VectorCopy(self->enemy->s.origin, enemy_org);
	enemy_org[2] += self->enemy->viewheight;
	VectorSubtract(enemy_org, org, dir);
	VectorNormalize(dir);
	fire_doom_bspi_ball(self, org, dir, Doom_MissileDamageRandomizer(5), 875);
}

mframe_t bspi_frames_shoot1[FRAME_COUNT(shoot)] =
{
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  bspi_fire_gun, frames_attack2 },
	{ ai_charge, 0,  bspi_fire_gun, frames_attack1 },
	{ ai_charge, 0,  bspi_fire_gun, frames_attack1 },
	{ ai_charge, 0,  bspi_fire_gun, frames_attack2 },
	{ ai_charge, 0,  bspi_fire_gun, frames_attack1 },
};
mmove_t bspi_shoot1 = { frames_shoot_start, frames_shoot_end, bspi_frames_shoot1, bspi_run };

void bspi_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &bspi_shoot1;
}

void bspi_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_alert, 1, ATTN_NORM, 0);
}

//===========================================================================

/*QUAKED doom_monster_bspi (1 0 0) (-32 -32 -24) (32 32 40) Ambush

*/
void doom_monster_bspi(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	sound_alert = gi.soundindex("doom/BSPSIT.wav");
	sound_chase = gi.soundindex("doom/BSPWLK.wav");
	sound_action = gi.soundindex("doom/BSPACT.wav");
	sound_pain = gi.soundindex("doom/DMPAIN.wav");
	sound_death = gi.soundindex("doom/BSPDTH.wav");
	sound_shoot = gi.soundindex("doom/PLASMA.wav");
	VectorSet(self->mins, -64, -64, -4);
	VectorSet(self->maxs, 64, 64, 60);
	self->s.modelindex = gi.modelindex("sprites/doom/bspi.d2s");
	self->health = 500;
	self->dmg = 0;
	self->monsterinfo.stand = bspi_stand;
	self->monsterinfo.walk = bspi_walk;
	self->monsterinfo.run = bspi_run;
	self->monsterinfo.sight = bspi_sight;
	self->pain = bspi_pain;
	self->die = bspi_die;
	self->monsterinfo.attack = bspi_attack;
	self->monsterinfo.special_frames = true;
	self->s.game = GAME_DOOM;
	gi.linkentity(self);
	self->monsterinfo.currentmove = &bspi_stand1;
	self->monsterinfo.scale = 1;
	walkmonster_start(self);
}

#include "g_local.h"

static q_soundhandle sound_alert;
static q_soundhandle sound_action;
static q_soundhandle sound_pain;
static q_soundhandle sound_death;
static q_soundhandle sound_shoot;
static q_soundhandle sound_claw;

enum
{
	frames_stand_start,
	frames_stand_end = frames_stand_start + 5,
	frames_run_start,
	frames_run_end = frames_run_start + 7,
	frames_walk_start,
	frames_walk_end = frames_walk_start + 7,
	frames_die_start,
	frames_die_end = frames_die_start + 6,
	frames_pain_start,
	frames_pain_end = frames_pain_start + 1,
	frames_shoot_start,
	frames_shoot_end = frames_shoot_start + 5,

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
	frames_death6,
	frames_death7
};

#define FRAME_COUNT(type) (frames_##type##_end - frames_##type##_start) + 1

void dboss_idle(edict_t *self)
{
	if ((Q_rand() % 256) < 3)
		gi.sound(self, CHAN_VOICE, sound_action, 1, ATTN_NORM, 0);
}

mframe_t boss_frames_stand1[FRAME_COUNT(stand)] =
{
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 }
};
mmove_t boss_stand1 = { frames_stand_start, frames_stand_end, boss_frames_stand1, NULL };

void boss_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &boss_stand1;
}

#define MOVE_SPEED 9.3f
#define WALK_SPEED MOVE_SPEED / 2

mframe_t boss_frames_run1[FRAME_COUNT(run)] =
{
	{ ai_run, MOVE_SPEED,  dboss_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  dboss_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  dboss_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  dboss_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  dboss_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  dboss_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  dboss_idle, frames_run4 },
	{ ai_run, MOVE_SPEED,  dboss_idle, frames_run4 }
};
mmove_t boss_run1 = { frames_run_start, frames_run_end, boss_frames_run1, NULL };

void boss_run(edict_t *self)
{
	self->monsterinfo.currentmove = &boss_run1;
}

mframe_t boss_frames_walk1[FRAME_COUNT(walk)] =
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
mmove_t boss_walk1 = { frames_walk_start, frames_walk_end, boss_frames_walk1, NULL };

void boss_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &boss_walk1;
}

void dboss_dead(edict_t *self)
{
	self->nextthink = 0;
	self->svflags |= SVF_DEADMONSTER;
}

mframe_t boss_frames_die1[FRAME_COUNT(die)] =
{
	{ ai_move, 0,  NULL, frames_death1 },
	{ ai_move, 0,  NULL, frames_death2 },
	{ ai_move, 0,  NULL, frames_death3 },
	{ ai_move, 0,  NULL, frames_death4 },
	{ ai_move, 0,  NULL, frames_death5 },
	{ ai_move, 0,  NULL, frames_death6 },
	{ ai_move, 0,  NULL, frames_death7 }
};
mmove_t boss_die1 = { frames_die_start, frames_die_end, boss_frames_die1, dboss_dead };

void dboss_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;
	// check for gib
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &boss_die1;
	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	gi.linkentity(self);
}

mframe_t boss_frames_pain1[FRAME_COUNT(pain)] =
{
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain }
};
mmove_t boss_pain1 = { frames_pain_start, frames_pain_end, boss_frames_pain1, boss_run };

void boss_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (random() < 0.1953f)
	{
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

		if (self->pain_debounce_time > level.time)
			return;

		self->monsterinfo.currentmove = &boss_pain1;
	}
}

//============================================================================

//
// P_CheckMeleeRange
//
bool P_CheckMeleeRange(edict_t *self);

void doom_boss_ball_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
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

	if (other->takedamage)
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);

	VectorNormalize(ent->velocity);
	VectorMA(ent->s.origin, -8, ent->velocity, origin);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_DOOM_BOSS_BOOM);
	MSG_WritePos(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);
	G_FreeEdict(ent);
}

void fire_doom_boss_ball(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
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
	rocket->s.modelindex = gi.modelindex("sprites/doom/BAL7.d2s");
	rocket->owner = self;
	rocket->touch = doom_boss_ball_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, self, MD_NONE, DT_DIRECT);
	gi.linkentity(rocket);
}

int Doom_MissileDamageRandomizer(int);

void boss_fire_gun(edict_t *self)
{
	if (P_CheckMeleeRange(self))
	{
		gi.sound(self, CHAN_WEAPON, sound_claw, 1, ATTN_NORM, 0);
		T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, (Q_rand() % 8 + 1) * 10, 0, DAMAGE_NO_PARTICLES, MakeAttackerMeansOfDeath(self, self, MD_MELEE, DT_DIRECT));
		return;
	}

	gi.sound(self, CHAN_WEAPON, sound_shoot, 1, ATTN_NORM, 0);
	vec3_t org, v_forward, v_right;
	AngleVectors(self->s.angles, v_forward, v_right, NULL);

	for (int i = 0; i < 3; ++i)
		org[i] = self->s.origin[i] + v_forward[i] * 0 + v_right[i] * 0;

	org[2] += 16;
	vec3_t dir;
	vec3_t enemy_org;
	VectorCopy(self->enemy->s.origin, enemy_org);
	enemy_org[2] += self->enemy->viewheight;
	VectorSubtract(enemy_org, org, dir);
	VectorNormalize(dir);
	fire_doom_boss_ball(self, org, dir, Doom_MissileDamageRandomizer(8), 525);
}

mframe_t boss_frames_shoot1[FRAME_COUNT(shoot)] =
{
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack2 },
	{ ai_charge, 0,  NULL, frames_attack2 },
	{ ai_charge, 0,  boss_fire_gun, frames_attack3 },
	{ ai_charge, 0,  NULL, frames_attack3 }
};
mmove_t boss_shoot1 = { frames_shoot_start, frames_shoot_end, boss_frames_shoot1, boss_run };

void boss_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &boss_shoot1;
}

void boss_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_alert, 1, ATTN_NORM, 0);
}

//===========================================================================

/*QUAKED doom_monster_boss (1 0 0) (-32 -32 -24) (32 32 40) Ambush

*/
void doom_monster_boss(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	sound_alert = gi.soundindex("doom/BRSSIT.wav");
	sound_action = gi.soundindex("doom/DMACT.wav");
	sound_pain = gi.soundindex("doom/DMPAIN.wav");
	sound_death = gi.soundindex("doom/BRSDTH.wav");
	sound_shoot = gi.soundindex("doom/FIRSHT.wav");
	sound_claw = gi.soundindex("doom/CLAW.wav");
	VectorSet(self->mins, -24, -24, -4);
	VectorSet(self->maxs, 24, 24, 60);

	if (self->entitytype == ET_DOOM_MONSTER_BOS2)
	{
		self->s.modelindex = gi.modelindex("sprites/doom/bos2.d2s");
		self->health = 500;
		self->dmg = 1;
	}
	else
	{
		self->s.modelindex = gi.modelindex("sprites/doom/boss.d2s");
		self->health = 1000;
		self->dmg = 0;
	}

	self->monsterinfo.stand = boss_stand;
	self->monsterinfo.walk = boss_walk;
	self->monsterinfo.run = boss_run;
	self->monsterinfo.sight = boss_sight;
	self->pain = boss_pain;
	self->die = dboss_die;
	self->monsterinfo.attack = boss_attack;
	self->monsterinfo.melee = boss_attack;
	self->monsterinfo.special_frames = true;
	self->s.game = GAME_DOOM;
	gi.linkentity(self);
	self->monsterinfo.currentmove = &boss_stand1;
	self->monsterinfo.scale = 1;
	walkmonster_start(self);
}

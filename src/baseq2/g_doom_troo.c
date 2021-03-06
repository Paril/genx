#ifdef ENABLE_COOP
#include "g_local.h"

static q_soundhandle sound_alert1;
static q_soundhandle sound_alert2;
static q_soundhandle sound_action;
static q_soundhandle sound_pain;
static q_soundhandle sound_death1;
static q_soundhandle sound_death2;
static q_soundhandle sound_gib;
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
	frames_gib_start,
	frames_gib_end = frames_gib_start + 15,
	frames_die_start,
	frames_die_end = frames_die_start + 4,
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
	frames_gib1,
	frames_gib2,
	frames_gib3,
	frames_gib4,
	frames_gib5,
	frames_gib6,
	frames_gib7,
	frames_gib8
};

#define FRAME_COUNT(type) (frames_##type##_end - frames_##type##_start) + 1

void troo_idle(edict_t *self)
{
	if ((Q_rand() % 256) < 3)
		gi.sound(self, CHAN_VOICE, sound_action, 1, ATTN_NORM, 0);
}

mframe_t troo_frames_stand1[FRAME_COUNT(stand)] =
{
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 }
};
mmove_t troo_stand1 = { frames_stand_start, frames_stand_end, troo_frames_stand1, NULL };

void troo_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &troo_stand1;
}

#define MOVE_SPEED 9.3f
#define WALK_SPEED MOVE_SPEED / 2

mframe_t troo_frames_run1[FRAME_COUNT(run)] =
{
	{ ai_run, MOVE_SPEED,  troo_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  troo_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  troo_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  troo_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  troo_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  troo_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  troo_idle, frames_run4 },
	{ ai_run, MOVE_SPEED,  troo_idle, frames_run4 }
};
mmove_t troo_run1 = { frames_run_start, frames_run_end, troo_frames_run1, NULL };

void troo_run(edict_t *self)
{
	self->monsterinfo.currentmove = &troo_run1;
}

mframe_t troo_frames_walk1[FRAME_COUNT(walk)] =
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
mmove_t troo_walk1 = { frames_walk_start, frames_walk_end, troo_frames_walk1, NULL };

void troo_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &troo_walk1;
}

void troo_dead(edict_t *self)
{
	self->nextthink = 0;
	self->server.flags.deadmonster = true;
}

mframe_t troo_frames_gib1[FRAME_COUNT(gib)] =
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
	{ ai_move, 0,  NULL, frames_gib8 }
};
mmove_t troo_gib1 = { frames_gib_start, frames_gib_end, troo_frames_gib1, troo_dead };

mframe_t troo_frames_die1[FRAME_COUNT(die)] =
{
	{ ai_move, 0,  NULL, frames_death1 },
	{ ai_move, 0,  NULL, frames_death2 },
	{ ai_move, 0,  NULL, frames_death3 },
	{ ai_move, 0,  NULL, frames_death4 },
	{ ai_move, 0,  NULL, frames_death5 }
};
mmove_t troo_die1 = { frames_die_start, frames_die_end, troo_frames_die1, troo_dead };

void troo_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;

	// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, sound_gib, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &troo_gib1;
	}
	else
	{
		switch (Q_rand() % 2)
		{
			case 0:
				gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
				break;

			case 1:
				gi.sound(self, CHAN_VOICE, sound_death2, 1, ATTN_NORM, 0);
				break;
		}

		self->monsterinfo.currentmove = &troo_die1;
	}

	self->takedamage = false;
	self->server.solid = SOLID_NOT;
	gi.linkentity(self);
}

mframe_t troo_frames_pain1[FRAME_COUNT(pain)] =
{
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain }
};
mmove_t troo_pain1 = { frames_pain_start, frames_pain_end, troo_frames_pain1, troo_run };

void troo_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (random() < 0.7813f)
	{
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

		if (self->pain_debounce_time > level.time)
			return;

		self->monsterinfo.currentmove = &troo_pain1;
	}
}

//============================================================================

void fire_doom_lead(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, int mod);

//
// P_CheckMeleeRange
//
bool P_CheckMeleeRange(edict_t *self)
{
	if (!self->enemy)
		return false;

	edict_t *pl = self->enemy;
	float dist = Distance(pl->server.state.origin, self->server.state.origin);

	if (dist >= 64 - 20 + sqrtf(DotProduct(pl->size, pl->size)))
		return false;

	if (!visible(self, self->enemy))
		return false;

	return true;
}


void doom_imp_ball_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

	if (other == ent->server.owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}

	if (ent->server.owner->server.client)
		PlayerNoise(ent->server.owner, ent->server.state.origin, PNOISE_IMPACT);

	if (other->takedamage)
		T_Damage(other, ent, ent->server.owner, ent->velocity, ent->server.state.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);

	VectorNormalize(ent->velocity);
	VectorMA(ent->server.state.origin, -8, ent->velocity, origin);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_DOOM_IMP_BOOM);
	MSG_WritePos(origin);
	gi.multicast(ent->server.state.origin, MULTICAST_PHS);
	G_FreeEdict(ent);
}

void fire_doom_imp_ball(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t *rocket;
	rocket = G_Spawn();
	VectorCopy(start, rocket->server.state.origin);
	VectorCopy(dir, rocket->movedir);
	vectoangles(dir, rocket->server.state.angles);
	VectorScale(dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->server.clipmask = MASK_SHOT;
	rocket->server.solid = SOLID_BBOX;
	rocket->server.state.effects |= EF_ANIM01;
	rocket->server.state.renderfx |= RF_FULLBRIGHT;
	rocket->server.state.game = GAME_DOOM;
	VectorClear(rocket->server.mins);
	VectorClear(rocket->server.maxs);
	rocket->server.state.modelindex = gi.modelindex("sprites/doom/BAL1.d2s");
	rocket->server.owner = self;
	rocket->touch = doom_imp_ball_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, rocket, MD_NONE, DT_DIRECT);
	gi.linkentity(rocket);
}

int Doom_MissileDamageRandomizer(int);

void troo_fire_gun(edict_t *self)
{
	if (P_CheckMeleeRange(self))
	{
		gi.sound(self, CHAN_WEAPON, sound_claw, 1, ATTN_NORM, 0);
		T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, (Q_rand() % 8 + 1) * 3, 0, DAMAGE_NO_PARTICLES, MakeAttackerMeansOfDeath(self, self, MD_MELEE, DT_DIRECT));
		return;
	}

	gi.sound(self, CHAN_WEAPON, sound_shoot, 1, ATTN_NORM, 0);
	vec3_t org, v_forward, v_right;
	AngleVectors(self->server.state.angles, v_forward, v_right, NULL);

	for (int i = 0; i < 3; ++i)
		org[i] = self->server.state.origin[i] + v_forward[i] * 0 + v_right[i] * 0;

	org[2] += 16;
	vec3_t dir;
	vec3_t enemy_org;
	VectorCopy(self->enemy->server.state.origin, enemy_org);
	enemy_org[2] += self->enemy->viewheight;
	VectorSubtract(enemy_org, org, dir);
	VectorNormalize(dir);
	fire_doom_imp_ball(self, org, dir, Doom_MissileDamageRandomizer(3), 350);
}

mframe_t troo_frames_shoot1[FRAME_COUNT(shoot)] =
{
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  troo_fire_gun, frames_attack2 },
	{ ai_charge, 0,  NULL, frames_attack2 },
	{ ai_charge, 0,  NULL, frames_attack3 },
	{ ai_charge, 0,  NULL, frames_attack3 }
};
mmove_t troo_shoot1 = { frames_shoot_start, frames_shoot_end, troo_frames_shoot1, troo_run };

void troo_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &troo_shoot1;
}

void troo_sight(edict_t *self, edict_t *other)
{
	switch (Q_rand() % 2)
	{
		case 0:
			gi.sound(self, CHAN_VOICE, sound_alert1, 1, ATTN_NORM, 0);
			break;

		case 1:
			gi.sound(self, CHAN_VOICE, sound_alert2, 1, ATTN_NORM, 0);
			break;
	}
}

//===========================================================================

/*QUAKED doom_monster_troo (1 0 0) (-32 -32 -24) (32 32 40) Ambush

*/
void doom_monster_troo(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	self->server.solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	sound_gib = gi.soundindex("doom/SLOP.wav");
	sound_alert1 = gi.soundindex("doom/BGSIT1.wav");
	sound_alert2 = gi.soundindex("doom/BGSIT2.wav");
	sound_action = gi.soundindex("doom/BGACT.wav");
	sound_pain = gi.soundindex("doom/POPAIN.wav");
	sound_death1 = gi.soundindex("doom/BGDTH1.wav");
	sound_death2 = gi.soundindex("doom/BGDTH2.wav");
	sound_shoot = gi.soundindex("doom/FIRSHT.wav");
	sound_claw = gi.soundindex("doom/CLAW.wav");
	VectorSet(self->server.mins, -20, -20, -4);
	VectorSet(self->server.maxs, 20, 20, 52);
	self->server.state.modelindex = gi.modelindex("sprites/doom/TROO.d2s");
	self->health = 60;
	self->dmg = 0;
	self->gib_health = -self->health;
	self->monsterinfo.stand = troo_stand;
	self->monsterinfo.walk = troo_walk;
	self->monsterinfo.run = troo_run;
	self->monsterinfo.sight = troo_sight;
	self->pain = troo_pain;
	self->die = troo_die;
	self->monsterinfo.attack = troo_attack;
	self->monsterinfo.melee = troo_attack;
	self->monsterinfo.special_frames = true;
	self->server.state.game = GAME_DOOM;
	gi.linkentity(self);
	self->monsterinfo.currentmove = &troo_stand1;
	self->monsterinfo.scale = 1;
	walkmonster_start(self);
}

#endif
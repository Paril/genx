#include "g_local.h"

static int sound_alert;
static int sound_action;
static int sound_pain;
static int sound_death;
static int sound_whoosh;
static int sound_shoot;
static int sound_punch;

enum {
	frames_stand_start,
	frames_stand_end = frames_stand_start + 5,
	frames_run_start,
	frames_run_end = frames_run_start + 11,
	frames_walk_start,
	frames_walk_end = frames_walk_start + 11,
	frames_die_start,
	frames_die_end = frames_die_start + 10,
	frames_pain_start,
	frames_pain_end = frames_pain_start + 3,
	frames_punch_start,
	frames_punch_end = frames_punch_start + 3,
	frames_missile_start,
	frames_missile_end = frames_missile_start + 5,

	frames_run1 = 0,
	frames_run2,
	frames_run3,
	frames_run4,
	frames_run5,
	frames_run6,
	frames_punch1,
	frames_punch2,
	frames_punch3,
	frames_missile1,
	frames_missile2,
	frames_pain,
	frames_death1,
	frames_death2,
	frames_death3,
	frames_death4,
	frames_death5
};

#define FRAME_COUNT(type) (frames_##type##_end - frames_##type##_start) + 1

void skel_idle(edict_t *self)
{
	if ((Q_rand() % 256) < 3)
		gi.sound(self, CHAN_VOICE, sound_action, 1, ATTN_NORM, 0);
}

mframe_t skel_frames_stand1[FRAME_COUNT(stand)] = {
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 }
};
mmove_t skel_stand1 = { frames_stand_start, frames_stand_end, skel_frames_stand1, NULL };

void skel_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &skel_stand1;
}

#define MOVE_SPEED 17.5f
#define WALK_SPEED MOVE_SPEED / 2

mframe_t skel_frames_run1[FRAME_COUNT(run)] = {
	{ ai_run, MOVE_SPEED,  skel_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  skel_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  skel_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  skel_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  skel_idle, frames_run6 },
	{ ai_run, MOVE_SPEED,  skel_idle, frames_run6 },
	{ ai_run, MOVE_SPEED,  skel_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  skel_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  skel_idle, frames_run4 },
	{ ai_run, MOVE_SPEED,  skel_idle, frames_run4 },
	{ ai_run, MOVE_SPEED,  skel_idle, frames_run5 },
	{ ai_run, MOVE_SPEED,  skel_idle, frames_run5 }
};
mmove_t skel_run1 = { frames_run_start, frames_run_end, skel_frames_run1, NULL };

void skel_run(edict_t *self)
{
	self->monsterinfo.currentmove = &skel_run1;
}

mframe_t skel_frames_walk1[FRAME_COUNT(walk)] = {
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
mmove_t skel_walk1 = { frames_walk_start, frames_walk_end, skel_frames_walk1, NULL };

void skel_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &skel_walk1;
}

void skel_dead(edict_t *self)
{
	self->nextthink = 0;
	self->svflags |= SVF_DEADMONSTER;
}

mframe_t skel_frames_die1[FRAME_COUNT(die)] = {
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
	{ ai_move, 0,  NULL, frames_death5 }
};
mmove_t skel_die1 = { frames_die_start, frames_die_end, skel_frames_die1, skel_dead };

void skel_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;

	// check for gib
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &skel_die1;

	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	gi.linkentity(self);
}

mframe_t skel_frames_pain1[FRAME_COUNT(pain)] = {
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain }
};
mmove_t skel_pain1 = { frames_pain_start, frames_pain_end, skel_frames_pain1, skel_run };

void skel_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (random() < 0.3906f)
	{
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

		if (self->pain_debounce_time > level.time)
			return;

		self->monsterinfo.currentmove = &skel_pain1;
	}
}

bool P_CheckMeleeRange(edict_t *self);
int Doom_MissileDamageRandomizer(int);

void skel_melee_hit(edict_t *self)
{
	if (!P_CheckMeleeRange(self))
		return;

	gi.sound(self, CHAN_WEAPON, sound_punch, 1, ATTN_NORM, 0);
	T_Damage(self->enemy, self, self, vec3_origin, vec3_origin, vec3_origin, ((Q_rand() % 10) + 1) * 6, 0, DAMAGE_NO_PARTICLES, MakeAttackerMeansOfDeath(self, self, MD_MELEE, DT_DIRECT));
}

mframe_t skel_frames_melee1[FRAME_COUNT(punch)] = {
	{ ai_charge, 0,  NULL, frames_punch1 },
	{ ai_charge, 0,  NULL, frames_punch2 },
	{ ai_charge, 0,  skel_melee_hit, frames_punch3 },
	{ ai_charge, 0,  NULL, frames_punch3 }
};
mmove_t skel_melee1 = { frames_punch_start, frames_punch_end, skel_frames_melee1, skel_run };

void skel_melee(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_whoosh, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &skel_melee1;
}

void doom_skel_missile_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY)) {
		G_FreeEdict(ent);
		return;
	}

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);

	VectorNormalize(ent->velocity);
	VectorMA(ent->s.origin, -8, ent->velocity, origin);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DOOM_FBXP_BOOM);
	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);

	G_FreeEdict(ent);
}

void doom_skel_think(edict_t *self)
{
	if (!self->enemy || self->enemy->health <= 0)
		return;

	vec3_t ideal_angle;
	VectorSubtract(self->enemy->s.origin, self->s.origin, ideal_angle);

	VectorNormalize(ideal_angle);
	vectoangles2(ideal_angle, ideal_angle);

	vec3_t cur_angle;
	float speed = VectorNormalize2(self->velocity, cur_angle);
	vectoangles2(cur_angle, cur_angle);

	LerpAngles(cur_angle, ideal_angle, 0.25f, ideal_angle);
	AngleVectors(ideal_angle, self->velocity, NULL, NULL);
	vectoangles(self->velocity, self->s.angles);
	VectorScale(self->velocity, speed, self->velocity);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DOOM_PUFF);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS);

	self->nextthink = level.time + 100;
}

void fire_doom_skel_missile(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
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
	rocket->enemy = self->enemy;
	VectorClear(rocket->mins);
	VectorClear(rocket->maxs);
	rocket->s.modelindex = gi.modelindex("sprites/doom/FATB.d2s");
	rocket->owner = self;
	rocket->touch = doom_skel_missile_touch;
	rocket->nextthink = level.time + 100;
	rocket->think = doom_skel_think;
	rocket->dmg = damage;

	rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, self, MD_NONE, DT_DIRECT);

	gi.linkentity(rocket);
}

void skel_fire_gun(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_shoot, 1, ATTN_NORM, 0);

	vec3_t org, v_forward, v_right;
	AngleVectors(self->s.angles, v_forward, v_right, NULL);

	for (int i = 0; i < 3; ++i)
		org[i] = self->s.origin[i] + v_forward[i] * 0 + v_right[i] * 0;

	org[2] += 16 + 16;

	vec3_t dir;

	vec3_t enemy_org;
	VectorCopy(self->enemy->s.origin, enemy_org);
	enemy_org[2] += self->enemy->viewheight;

	VectorSubtract(enemy_org, org, dir);
	VectorNormalize(dir);
	fire_doom_skel_missile(self, org, dir, Doom_MissileDamageRandomizer(10), 350);
}

mframe_t skel_frames_fire1[FRAME_COUNT(missile)] = {
	{ ai_charge, 0,  skel_fire_gun, frames_missile1 },
	{ ai_charge, 0,  NULL, frames_missile1 },
	{ ai_charge, 0,  NULL, frames_missile2 },
	{ ai_charge, 0,  NULL, frames_missile2 },
	{ ai_charge, 0,  NULL, frames_missile2 },
	{ ai_charge, 0,  NULL, frames_missile2 }
};
mmove_t skel_fire1 = { frames_missile_start, frames_missile_end, skel_frames_fire1, skel_run };

void skel_fire(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_shoot, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &skel_fire1;
}

void skel_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_alert, 1, ATTN_NORM, 0);
}

//===========================================================================

/*QUAKED doom_monster_skel (1 0 0) (-32 -32 -24) (32 32 40) Ambush

*/
void doom_monster_skel(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;

	sound_alert = gi.soundindex("doom/SKESIT.wav");
	sound_action = gi.soundindex("doom/SKEACT.wav");
	sound_pain = gi.soundindex("doom/POPAIN.wav");
	sound_death = gi.soundindex("doom/SKEDTH.wav");

	sound_whoosh = gi.soundindex("doom/SKESWG.wav");
	sound_shoot = gi.soundindex("doom/SKEATK.wav");
	sound_punch = gi.soundindex("doom/SKEPCH.wav");

	VectorSet(self->mins, -20, -20, -4);
	VectorSet(self->maxs, 20, 20, 52);

	self->s.modelindex = gi.modelindex("sprites/doom/skel.d2s");
	self->health = 300;

	self->monsterinfo.stand = skel_stand;
	self->monsterinfo.walk = skel_walk;
	self->monsterinfo.run = skel_run;
	self->monsterinfo.sight = skel_sight;
	self->pain = skel_pain;
	self->die = skel_die;
	self->monsterinfo.attack = skel_fire;
	self->monsterinfo.melee = skel_melee;
	self->monsterinfo.special_frames = true;
	self->s.game = GAME_DOOM;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &skel_stand1;
	self->monsterinfo.scale = 1;

	walkmonster_start(self);
}

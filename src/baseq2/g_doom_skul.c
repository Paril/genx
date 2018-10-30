#include "g_local.h"

static int sound_action;
static int sound_pain;
static int sound_death;
static int sound_shoot;

enum {
	frames_stand_start,
	frames_stand_end = frames_stand_start + 3,
	frames_run_start,
	frames_run_end = frames_run_start + 3,
	frames_walk_start,
	frames_walk_end = frames_walk_start + 3,
	frames_die_start,
	frames_die_end = frames_die_start + 11,
	frames_pain_start,
	frames_pain_end = frames_pain_start + 1,
	frames_shoot_start,
	frames_shoot_end = frames_shoot_start + 2,

	frames_run1 = 0,
	frames_run2 = 1,
	frames_attack1,
	frames_attack2,
	frames_pain,
	frames_death1,
	frames_death2,
	frames_death3,
	frames_death4,
	frames_death5,
	frames_death6
};

#define FRAME_COUNT(type) (frames_##type##_end - frames_##type##_start) + 1

void skul_idle(edict_t *self)
{
	if ((Q_rand() % 256) < 3)
		gi.sound(self, CHAN_VOICE, sound_action, 1, ATTN_NORM, 0);
}

mframe_t skul_frames_stand1[FRAME_COUNT(stand)] = {
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run1 },
	{ ai_stand, 0,  NULL, frames_run2 },
	{ ai_stand, 0,  NULL, frames_run2 }
};
mmove_t skul_stand1 = { frames_stand_start, frames_stand_end, skul_frames_stand1, NULL };

void skul_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &skul_stand1;
}

#define MOVE_SPEED 4.67f
#define WALK_SPEED MOVE_SPEED / 2

mframe_t skul_frames_run1[FRAME_COUNT(run)] = {
	{ ai_run, MOVE_SPEED,  skul_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  skul_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  skul_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  skul_idle, frames_run2 }
};
mmove_t skul_run1 = { frames_run_start, frames_run_end, skul_frames_run1, NULL };

void skul_run(edict_t *self)
{
	self->monsterinfo.currentmove = &skul_run1;
}

mframe_t skul_frames_walk1[FRAME_COUNT(walk)] = {
	{ ai_walk, WALK_SPEED,  NULL, frames_run1 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run1 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run2 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run2 }
};
mmove_t skul_walk1 = { frames_walk_start, frames_walk_end, skul_frames_walk1, NULL };

void skul_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &skul_walk1;
}

void ai_nop(edict_t *self, float dist)
{
}

mframe_t skul_frames_die1[FRAME_COUNT(die)] = {
	{ ai_nop, 0,  NULL, frames_death1 },
	{ ai_nop, 0,  NULL, frames_death1 },
	{ ai_nop, 0,  NULL, frames_death2 },
	{ ai_nop, 0,  NULL, frames_death2 },
	{ ai_nop, 0,  NULL, frames_death3 },
	{ ai_nop, 0,  NULL, frames_death3 },
	{ ai_nop, 0,  NULL, frames_death4 },
	{ ai_nop, 0,  NULL, frames_death4 },
	{ ai_nop, 0,  NULL, frames_death5 },
	{ ai_nop, 0,  NULL, frames_death5 },
	{ ai_nop, 0,  NULL, frames_death6 },
	{ ai_nop, 0,  NULL, frames_death6 }
};
mmove_t skul_die1 = { frames_die_start, frames_die_end, skul_frames_die1, G_FreeEdict };

void skul_reset(edict_t *self)
{
	self->movetype = MOVETYPE_STEP;
	self->think = monster_think;
	self->nextthink = level.time + 100;
	self->touch = NULL;
	self->owner = NULL;
}

void skul_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;

	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &skul_die1;

	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->svflags |= SVF_DEADMONSTER;
	skul_reset(self);
	gi.linkentity(self);
}

mframe_t skul_frames_pain1[FRAME_COUNT(pain)] = {
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain }
};
mmove_t skul_pain1 = { frames_pain_start, frames_pain_end, skul_frames_pain1, skul_run };

void skul_fly(edict_t *self);

void skul_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &skul_pain1;

	self->monsterinfo.pausetime = 0;
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	self->monsterinfo.nextframe = 0;

	if (self->think == skul_fly)
		VectorClear(self->velocity);

	skul_reset(self);
}

//============================================================================

bool skul_would_hit(edict_t *self)
{
	vec3_t end, vel;
	vec3_t dir;
	VectorSubtract(self->dest1, self->s.origin, dir);
	VectorNormalize(dir);
	VectorScale(dir, 275 * 2.5f, vel);

	VectorMA(self->s.origin, game.frameseconds, vel, end);
	trace_t tr = gi.trace(self->s.origin, self->mins, self->maxs, end, self, MASK_SHOT);

	if (tr.ent && tr.ent == self->enemy)
		return false;

	return (tr.fraction < 1.0f);
}

void skul_fly(edict_t *self)
{
	if (self->s.frame == frames_attack1)
		self->s.frame = frames_attack2;
	else
		self->s.frame = frames_attack1;

	if (!self->accel || VectorEmpty(self->velocity))
	{
		vec3_t dir;
		vec3_t me;
		VectorCopy(self->s.origin, me);
		me[2] += self->viewheight;
		VectorSubtract(self->dest1, me, dir);
		VectorNormalize(dir);
		VectorScale(dir, 275 * 2.5f, self->velocity);
		self->accel = 1;
	}

	if (skul_would_hit(self))
		self->think = skul_reset;

	self->nextthink = level.time + 100;
}

void skul_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int damage = ((Q_rand() % 8) + 1) * 3;

	if (other && other->takedamage)
		T_Damage(other, self, self, vec3_origin, vec3_origin, vec3_origin, damage, 0, DAMAGE_NO_PARTICLES, MakeAttackerMeansOfDeath(self, self, MD_MELEE, DT_DIRECT));

	skul_run(self);
	skul_reset(self);
}

void skul_wait(edict_t *self)
{
	if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
	{
		// lose enemy
		VectorCopy(self->enemy->s.origin, self->dest1);
		self->dest1[2] += self->enemy->viewheight;
		//self->enemy = NULL;
		//self->movetarget = self->goalentity = NULL;

		gi.sound(self, CHAN_WEAPON, sound_shoot, 1, ATTN_NORM, 0);
		self->monsterinfo.pausetime = level.time + 300;
	}

	if (level.time >= self->monsterinfo.pausetime)
	{
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;

		self->accel = 0;
		self->movetype = MOVETYPE_FLYMISSILE;
		self->touch = skul_touch;
		self->think = skul_fly;
		self->nextthink = level.time + 100;
	}
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

mframe_t skul_frames_shoot1[FRAME_COUNT(shoot)] = {
	{ ai_charge, 0,  skul_wait, frames_attack1 },
	{ ai_nop, 0,  NULL, frames_attack2 },
	{ ai_nop, 0,  NULL, frames_attack1 },
};
mmove_t skul_shoot1 = { frames_shoot_start, frames_shoot_end, skul_frames_shoot1, skul_run };

bool skul_would_hit(edict_t *self);

void skul_attack(edict_t *self)
{
	if (!skul_would_hit(self))
		self->monsterinfo.currentmove = &skul_shoot1;
}

//===========================================================================

/*QUAKED doom_monster_skul (1 0 0) (-32 -32 -24) (32 32 40) Ambush

*/
void doom_monster_skul(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;

	sound_action = gi.soundindex("doom/DMACT.wav");
	sound_pain = gi.soundindex("doom/DMPAIN.wav");
	sound_death = gi.soundindex("doom/FIRXPL.wav");
	sound_shoot = gi.soundindex("doom/SKLATK.wav");

	VectorSet(self->mins, -16, -16, -4);
	VectorSet(self->maxs, 16, 16, 52);

	self->s.modelindex = gi.modelindex("sprites/doom/skul.d2s");
	self->health = 100;
	self->dmg = 0;
	self->mass = 50;
	self->s.renderfx |= RF_FULLBRIGHT;

	self->monsterinfo.stand = skul_stand;
	self->monsterinfo.walk = skul_walk;
	self->monsterinfo.run = skul_run;
	self->pain = skul_pain;
	self->die = skul_die;
	self->monsterinfo.attack = skul_attack;
	self->monsterinfo.special_frames = true;
	self->s.game = GAME_DOOM;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &skul_stand1;
	self->monsterinfo.scale = 1;

	flymonster_start(self);
}

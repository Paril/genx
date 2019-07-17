#include "g_local.h"

static q_soundhandle sound_alert;
static q_soundhandle sound_action;
static q_soundhandle sound_pain;
static q_soundhandle sound_death;

enum
{
	frames_stand_start,
	frames_stand_end = frames_stand_start + 0,
	frames_run_start,
	frames_run_end = frames_run_start + 7,
	frames_walk_start,
	frames_walk_end = frames_walk_start + 7,
	frames_die_start,
	frames_die_end = frames_die_start + 5,
	frames_pain_start,
	frames_pain_end = frames_pain_start + 3,
	frames_shoot_start,
	frames_shoot_end = frames_shoot_start + 4,

	frames_run1 = 0,
	frames_run2,
	frames_run3,
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

void pain_idle(edict_t *self)
{
	if ((Q_rand() % 256) < 3)
		gi.sound(self, CHAN_VOICE, sound_action, 1, ATTN_NORM, 0);
}

mframe_t pain_frames_stand1[FRAME_COUNT(stand)] =
{
	{ ai_stand, 0,  NULL, frames_run1 }
};
mmove_t pain_stand1 = { frames_stand_start, frames_stand_end, pain_frames_stand1, NULL };

void pain_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &pain_stand1;
}

#define MOVE_SPEED 9.3f
#define WALK_SPEED MOVE_SPEED / 2

mframe_t pain_frames_run1[FRAME_COUNT(run)] =
{
	{ ai_run, MOVE_SPEED,  pain_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  pain_idle, frames_run1 },
	{ ai_run, MOVE_SPEED,  pain_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  pain_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  pain_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  pain_idle, frames_run3 },
	{ ai_run, MOVE_SPEED,  pain_idle, frames_run2 },
	{ ai_run, MOVE_SPEED,  pain_idle, frames_run2 }
};
mmove_t pain_run1 = { frames_run_start, frames_run_end, pain_frames_run1, NULL };

void pain_run(edict_t *self)
{
	self->monsterinfo.currentmove = &pain_run1;
}

mframe_t pain_frames_walk1[FRAME_COUNT(walk)] =
{
	{ ai_walk, WALK_SPEED,  NULL, frames_run1 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run1 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run2 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run2 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run3 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run3 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run2 },
	{ ai_walk, WALK_SPEED,  NULL, frames_run2 }
};
mmove_t pain_walk1 = { frames_walk_start, frames_walk_end, pain_frames_walk1, NULL };

void pain_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &pain_walk1;
}

void pain_dead(edict_t *self)
{
	G_FreeEdict(self);
}

void pain_launch_soul(edict_t *self, float yaw);

void pain_throw(edict_t *self)
{
	pain_launch_soul(self, self->s.angles[1] + 90);
	pain_launch_soul(self, self->s.angles[1] + 180);
	pain_launch_soul(self, self->s.angles[1] + 270);
}

mframe_t pain_frames_die1[FRAME_COUNT(die)] =
{
	{ ai_move, 0,  NULL, frames_death1 },
	{ ai_move, 0,  NULL, frames_death2 },
	{ ai_move, 0,  NULL, frames_death3 },
	{ ai_move, 0,  NULL, frames_death4 },
	{ ai_move, 0,  pain_throw, frames_death5 },
	{ ai_move, 0,  NULL, frames_death6 }
};
mmove_t pain_die1 = { frames_die_start, frames_die_end, pain_frames_die1, pain_dead };

void pain_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	self->deadflag = DEAD_DEAD;
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &pain_die1;
	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	gi.linkentity(self);
}

mframe_t pain_frames_pain1[FRAME_COUNT(pain)] =
{
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain },
	{ ai_move, 0,  NULL, frames_pain }
};
mmove_t pain_pain1 = { frames_pain_start, frames_pain_end, pain_frames_pain1, pain_run };

void pain_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (random() < 0.5f)
	{
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

		if (self->pain_debounce_time > level.time)
			return;

		self->monsterinfo.currentmove = &pain_pain1;
	}
}

//============================================================================

int pain_count_lost_souls()
{
	edict_t *start = G_FindByType(NULL, ET_DOOM_MONSTER_SKUL);

	if (!start)
		return 0;

	edict_t *cur = start;
	int count = 0;

	while (true)
	{
		cur = G_FindByType(cur, ET_DOOM_MONSTER_SKUL);

		if (!cur || cur == start)
			break;

		++count;
	}

	return count;
}

void doom_monster_skul(edict_t *self);
void skul_attack(edict_t *self);
void monster_start_go(edict_t *self);
void skul_fly(edict_t *self);
void skul_wait(edict_t *self);

void pain_launch_soul(edict_t *self, float yaw)
{
	if (pain_count_lost_souls() >= 21)
		return;

	vec3_t start;
	vec3_t fwd;
	vec3_t dir = { 0, yaw, 0 };
	AngleVectors(dir, fwd, NULL, NULL);
	VectorMA(self->s.origin, 32, fwd, start);
	start[2] += 6;
	vec3_t ls_mins = { -16, -16, -4 };
	vec3_t ls_maxs = { 16, 16, 52 };
	trace_t tr = gi.trace(start, ls_mins, ls_maxs, start, self, MASK_SHOT);

	// pull it into our mouth again
	if (self->health > 0)
		VectorMA(start, -26, fwd, tr.endpos);

	if (tr.fraction != 1.0f)
	{
		MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(TE_DOOM_IMP_BOOM);
		MSG_WritePos(start);
		gi.multicast(start, MULTICAST_PHS);
		return;
	}

	edict_t *skul = G_Spawn();
	VectorCopy(tr.endpos, skul->s.origin);
	VectorCopy(self->s.angles, skul->s.angles);
	skul->enemy = self->enemy;
	skul->entitytype = ET_DOOM_MONSTER_SKUL;
	doom_monster_skul(skul);
	gi.linkentity(skul);
	monster_start_go(skul);

	if (skul->enemy)
		FoundTarget(skul);

	if (self->health > 0)
	{
		skul_wait(skul);
		skul->monsterinfo.pausetime = 0;
		skul_wait(skul);
		skul_fly(skul);
	}
}

void pain_throw_soul(edict_t *self)
{
	vec3_t sub;
	VectorSubtract(self->enemy->s.origin, self->s.origin, sub);
	vectoangles(sub, sub);
	pain_launch_soul(self, sub[1]);
}

mframe_t pain_frames_shoot1[FRAME_COUNT(shoot)] =
{
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack1 },
	{ ai_charge, 0,  NULL, frames_attack2 },
	{ ai_charge, 0,  NULL, frames_attack3 },
	{ ai_charge, 0,  pain_throw_soul, frames_attack3 }
};
mmove_t pain_shoot1 = { frames_shoot_start, frames_shoot_end, pain_frames_shoot1, pain_run };

void pain_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &pain_shoot1;
}

void pain_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_alert, 1, ATTN_NORM, 0);
}

//===========================================================================

/*QUAKED doom_monster_pain (1 0 0) (-32 -32 -24) (32 32 40) Ambush

*/
void doom_monster_pain(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	sound_alert = gi.soundindex("doom/PESIT.wav");
	sound_action = gi.soundindex("doom/DMACT.wav");
	sound_pain = gi.soundindex("doom/PEPAIN.wav");
	sound_death = gi.soundindex("doom/PEDTH.wav");
	VectorSet(self->mins, -31, -31, -4);
	VectorSet(self->maxs, 31, 31, 52);
	self->s.modelindex = gi.modelindex("sprites/doom/pain.d2s");
	self->health = 400;
	self->mass = 400;
	self->dmg = 0;
	self->monsterinfo.stand = pain_stand;
	self->monsterinfo.walk = pain_walk;
	self->monsterinfo.run = pain_run;
	self->monsterinfo.sight = pain_sight;
	self->pain = pain_pain;
	self->die = pain_die;
	self->monsterinfo.attack = pain_attack;
	self->monsterinfo.melee = pain_attack;
	self->monsterinfo.special_frames = true;
	self->s.game = GAME_DOOM;
	gi.linkentity(self);
	self->monsterinfo.currentmove = &pain_stand1;
	self->monsterinfo.scale = 1;
	flymonster_start(self);
}

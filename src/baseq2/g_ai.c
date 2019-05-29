/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
// g_ai.c

#include "g_local.h"

bool FindTarget(edict_t *self);
extern cvar_t   *maxclients;

bool ai_checkattack(edict_t *self, float dist);

static bool        enemy_vis;
static int         enemy_range;
static float       enemy_yaw;

//============================================================================


void M_NavigatorNodeReached(edict_t *self);
bool M_NavigatorPathToSpot(edict_t *self, vec3_t spot);
void M_StopBotheringWithPaths(edict_t *self);

/*
=================
AI_SetSightClient

Called once each frame to set level.sight_client to the
player to be checked for in findtarget.

If all clients are either dead or in notarget, sight_client
will be null.

In coop games, sight_client will cycle between the clients.
=================
*/
void AI_SetSightClient(void)
{
	edict_t *ent;
	int     start, check;

	if (level.sight_client == NULL)
		start = 1;
	else
		start = level.sight_client - g_edicts;

	check = start;

	while (1)
	{
		check++;

		if (check > game.maxclients)
			check = 1;

		ent = &g_edicts[check];

		if (ent->inuse
			&& ent->health > 0
			&& !(ent->flags & FL_NOTARGET))
		{
			level.sight_client = ent;
			return;     // got one
		}

		if (check == start)
		{
			level.sight_client = NULL;
			return;     // nobody to see
		}
	}
}

//============================================================================

/*
=============
ai_move

Move the specified distance at current facing.
This replaces the QC functions: ai_forward, ai_back, ai_pain, and ai_painforward
==============
*/
void ai_move(edict_t *self, float dist)
{
	M_walkmove(self, self->s.angles[YAW], dist);
}


/*
=============
ai_stand

Used for standing around and looking for players
Distance is for slight position adjustments needed by the animations
==============
*/
bool SV_CloseEnough(edict_t *ent, vec3_t goal_absmin, vec3_t goal_absmax, float dist);
void SV_NewChaseDir(edict_t *actor, vec3_t enemy_origin, float dist);

void ai_stand(edict_t *self, float dist)
{
	vec3_t  v;

	if (dist)
		M_walkmove(self, self->s.angles[YAW], dist);

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		if (self->enemy)
		{
			VectorSubtract(self->enemy->s.origin, self->s.origin, v);
			self->ideal_yaw = vectoyaw(v);

			if (self->s.angles[YAW] != self->ideal_yaw && self->monsterinfo.aiflags & AI_TEMP_STAND_GROUND)
			{
				self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
				self->monsterinfo.run(self);
			}

			M_ChangeYaw(self);
			ai_checkattack(self, 0);
		}
		else
			FindTarget(self);

		return;
	}
	else if (self->monsterinfo.aiflags & AI_NODE_PATH)
	{
		// we're waiting to reach the next node
		vec3_t path_absmin = { -4, -4, -4 }, path_absmax = { 4, 4, 4 };
		VectorAdd(self->monsterinfo.navigator_pos, path_absmin, path_absmin);
		VectorAdd(self->monsterinfo.navigator_pos, path_absmax, path_absmax);

		// if the next step hits the path, change path
		if (SV_CloseEnough(self, path_absmin, path_absmax, dist))
		{
			M_NavigatorNodeReached(self);
			SV_NewChaseDir(self, self->monsterinfo.navigator_pos, dist);
			self->monsterinfo.run(self);
		}
	}

	if (FindTarget(self))
		return;

	if (level.time > self->monsterinfo.pausetime)
	{
		self->monsterinfo.walk(self);
		return;
	}

	if (!(self->spawnflags & 1) && (self->monsterinfo.idle) && (level.time > self->monsterinfo.idle_time))
	{
		if (self->monsterinfo.idle_time)
		{
			self->monsterinfo.idle(self);
			self->monsterinfo.idle_time = level.time + 15000 + random() * 15000;
		}
		else
			self->monsterinfo.idle_time = level.time + random() * 15000;
	}
}


/*
=============
ai_walk

The monster is walking it's beat
=============
*/
void ai_walk(edict_t *self, float dist)
{
	if (!self->goalentity && !self->monsterinfo.navigator)
	{
		self->monsterinfo.pausetime = GTIME_MAX;
		self->monsterinfo.stand(self);
		return;
	}

	M_MoveToGoal(self, dist);

	// check for noticing a player
	if (FindTarget(self))
		return;

	if ((self->monsterinfo.search) && (level.time > self->monsterinfo.idle_time))
	{
		if (self->monsterinfo.idle_time)
		{
			self->monsterinfo.search(self);
			self->monsterinfo.idle_time = level.time + 15000 + random() * 15000;
		}
		else
			self->monsterinfo.idle_time = level.time + random() * 15000;
	}
}


/*
=============
ai_charge

Turns towards target and advances
Use this call with a distnace of 0 to replace ai_face
==============
*/
void ai_charge(edict_t *self, float dist)
{
	vec3_t  v;
	VectorSubtract(self->enemy->s.origin, self->s.origin, v);
	self->ideal_yaw = vectoyaw(v);
	M_ChangeYaw(self);

	if (dist)
		M_walkmove(self, self->s.angles[YAW], dist);
}


/*
=============
ai_turn

don't move, but turn towards ideal_yaw
Distance is for slight position adjustments needed by the animations
=============
*/
void ai_turn(edict_t *self, float dist)
{
	if (dist)
		M_walkmove(self, self->s.angles[YAW], dist);

	if (FindTarget(self))
		return;

	M_ChangeYaw(self);
}


/*

.enemy
Will be world if not currently angry at anyone.

.movetarget
The next path spot to walk toward.  If .enemy, ignore .movetarget.
When an enemy is killed, the monster will try to return to it's path.

.hunt_time
Set to time + something when the player is in sight, but movement straight for
him is blocked.  This causes the monster to use wall following code for
movement direction instead of sighting on the player.

.ideal_yaw
A yaw angle of the intended direction, which will be turned towards at up
to 45 deg / state.  If the enemy is in view and hunt_time is not active,
this will be the exact line towards the enemy.

.pausetime
A monster will leave it's stand state and head towards it's .movetarget when
time > .pausetime.

walkmove(angle, speed) primitive is all or nothing
*/

/*
=============
range

returns the range catagorization of an entity reletive to self
0   melee range, will become hostile even if back is turned
1   visibility and infront, or visibility and show hostile
2   infront and show hostile
3   only triggered by damage
=============
*/
int range(edict_t *self, edict_t *other)
{
	vec3_t  v;
	float   len;
	VectorSubtract(self->s.origin, other->s.origin, v);
	len = VectorLength(v);

	if (len < MELEE_DISTANCE)
		return RANGE_MELEE;

	if (len < 500)
		return RANGE_NEAR;

	if (len < 1000)
		return RANGE_MID;

	return RANGE_FAR;
}

/*
=============
visible

returns 1 if the entity is visible to self, even if not infront ()
=============
*/
bool visible(edict_t *self, edict_t *other)
{
	if (other->flags & FL_NOTARGET)
		return false;

	vec3_t  spot1;
	vec3_t  spot2;
	trace_t trace;
	VectorCopy(self->s.origin, spot1);
	spot1[2] += self->viewheight;
	VectorCopy(other->s.origin, spot2);
	spot2[2] += other->viewheight;
	trace = gi.trace(spot1, vec3_origin, vec3_origin, spot2, self, MASK_OPAQUE);

	if (trace.fraction == 1.0f)
		return true;

	return false;
}


/*
=============
infront

returns 1 if the entity is in front (in sight) of self
=============
*/
bool infront(edict_t *self, edict_t *other)
{
	vec3_t  vec;
	float   dot;
	vec3_t  forward;
	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);

	if (dot > 0.3f)
		return true;

	return false;
}


//============================================================================

static void HuntTarget(edict_t *self)
{
	vec3_t  vec;
	self->goalentity = self->enemy;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.stand(self);
	else
		self->monsterinfo.run(self);

	VectorSubtract(self->enemy->s.origin, self->s.origin, vec);
	self->ideal_yaw = vectoyaw(vec);

	// wait a while before first attack
	if (!(self->monsterinfo.aiflags & AI_STAND_GROUND))
		AttackFinished(self, 1);
}

void FoundTarget(edict_t *self)
{
	// let other monsters see this monster for a while
	if (self->enemy->client)
	{
		level.sight_entity = self;
		level.sight_entity_time = level.time + 100;
	}

	self->show_hostile = level.time + 1;        // wake up other monsters
	VectorCopy(self->enemy->s.origin, self->monsterinfo.last_sighting);
	self->monsterinfo.trail_time = level.time;

	if (!self->combattarget)
	{
		HuntTarget(self);
		return;
	}

	self->goalentity = self->movetarget = G_PickTarget(self->combattarget);

	if (!self->movetarget)
	{
		self->goalentity = self->movetarget = self->enemy;
		HuntTarget(self);
		gi.dprintf("%d at %s, combattarget %s not found\n", self->entitytype, vtos(self->s.origin), self->combattarget);
		return;
	}

	// clear out our combattarget, these are a one shot deal
	self->combattarget = NULL;
	self->monsterinfo.aiflags |= AI_COMBAT_POINT;
	// clear the targetname, that point is ours!
	self->movetarget->targetname = NULL;
	self->monsterinfo.pausetime = 0;
	// run for it
	self->monsterinfo.run(self);
}


/*
===========
FindTarget

Self is currently not attacking anything, so try to find a target

Returns TRUE if an enemy was sighted

When a player fires a missile, the point of impact becomes a fakeplayer so
that monsters that see the impact will respond as if they had seen the
player.

To avoid spending too much time, only a single client (or fakeclient) is
checked each frame.  This means multi player games will have slightly
slower noticing monsters.
============
*/
bool FindTarget(edict_t *self)
{
	edict_t     *client;
	bool        heardit;
	int         r;

	if (self->monsterinfo.aiflags & AI_GOOD_GUY)
	{
		if (self->goalentity && self->goalentity->inuse && self->goalentity->entitytype)
		{
			if (self->goalentity->entitytype == ET_TARGET_ACTOR)
				return false;
		}

		//FIXME look for monsters?
		return false;
	}

	// if we're going to a combat point, just proceed
	if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
		return false;

	// if the first spawnflag bit is set, the monster will only wake up on
	// really seeing the player, not another monster getting angry or hearing
	// something
	// revised behavior so they will wake up if they "see" a player make a noise
	// but not weapon impact/explosion noises
	heardit = false;

	if ((level.sight_entity_time > level.time) && !(self->spawnflags & 1))
	{
		client = level.sight_entity;

		if (client->enemy == self->enemy)
			return false;
	}
	else if (level.sound_entity_time > level.time)
	{
		client = level.sound_entity;
		heardit = true;
	}
	else if (!(self->enemy) && (level.sound2_entity_time > level.time) && !(self->spawnflags & 1))
	{
		client = level.sound2_entity;
		heardit = true;
	}
	else
	{
		client = level.sight_client;

		if (!client)
			return false;   // no clients to get mad at
	}

	// if the entity went away, forget it
	if (!client->inuse)
		return false;

	if (client == self->enemy)
		return true;    // JDC false;

	if (client->client)
	{
		if (client->flags & FL_NOTARGET)
			return false;
	}
	else if (client->svflags & SVF_MONSTER)
	{
		if (!client->enemy)
			return false;

		if (client->enemy->flags & FL_NOTARGET)
			return false;
	}
	else if (heardit)
	{
		if (client->owner->flags & FL_NOTARGET)
			return false;
	}
	else
		return false;

	if (!heardit)
	{
		r = range(self, client);

		if (r == RANGE_FAR)
			return false;

		// this is where we would check invisibility

		if (!visible(self, client))
			return false;

		if (r == RANGE_NEAR)
		{
			if (client->show_hostile < level.time && !infront(self, client))
				return false;
		}
		else if (r == RANGE_MID)
		{
			if (!infront(self, client))
				return false;
		}

		self->enemy = client;

		if (self->enemy->entitytype != ET_PLAYER_NOISE)
		{
			self->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

			if (!self->enemy->client)
			{
				self->enemy = self->enemy->enemy;

				if (!self->enemy->client)
				{
					self->enemy = NULL;
					return false;
				}
			}
		}
	}
	else   // heardit
	{
		vec3_t  temp;

		if (self->spawnflags & 1)
		{
			if (!visible(self, client))
				return false;
		}
		else
		{
			if (!gi.inPHS(self->s.origin, client->s.origin))
				return false;
		}

		VectorSubtract(client->s.origin, self->s.origin, temp);

		if (VectorLength(temp) > 1000)   // too far to hear
			return false;

		// check area portals - if they are different and not connected then we can't hear it
		if (client->areanum != self->areanum)
			if (!gi.AreasConnected(self->areanum, client->areanum))
				return false;

		self->ideal_yaw = vectoyaw(temp);
		M_ChangeYaw(self);
		// hunt the sound for a bit; hopefully find the real player
		self->monsterinfo.aiflags |= AI_SOUND_TARGET;
		self->enemy = client;
		VectorCopy(self->enemy->s.origin, self->monsterinfo.last_sighting);
		M_NavigatorPathToSpot(self, self->monsterinfo.last_sighting);
	}

	//
	// got one
	//
	FoundTarget(self);

	if (!(self->monsterinfo.aiflags & AI_SOUND_TARGET) && (self->monsterinfo.sight))
		self->monsterinfo.sight(self, self->enemy);

	return true;
}


//=============================================================================

/*
============
FacingIdeal

============
*/
bool FacingIdeal(edict_t *self)
{
	float   delta;
	delta = anglemod(self->s.angles[YAW] - self->ideal_yaw);

	if (delta > 45 && delta < 315)
		return false;

	return true;
}


//=============================================================================

bool M_CheckAttack(edict_t *self)
{
	vec3_t  spot1, spot2;
	float   chance;
	trace_t tr;

	if (self->enemy->health > 0)
	{
		// see if any entities are in the way of the shot
		VectorCopy(self->s.origin, spot1);
		spot1[2] += self->viewheight;
		VectorCopy(self->enemy->s.origin, spot2);
		spot2[2] += self->enemy->viewheight;
		tr = gi.trace(spot1, NULL, NULL, spot2, self, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_WINDOW);

		// do we have a clear shot?
		if (tr.ent != self->enemy)
			return false;
	}

	// melee attack
	if (enemy_range == RANGE_MELEE)
	{
		// don't always melee in easy mode
		if (skill->value == 0 && (Q_rand() & 3))
			return false;

		if (self->monsterinfo.melee)
			self->monsterinfo.attack_state = AS_MELEE;
		else
			self->monsterinfo.attack_state = AS_MISSILE;

		return true;
	}

	// missile attack
	if (!self->monsterinfo.attack)
		return false;

	if (level.time < self->monsterinfo.attack_finished)
		return false;

	if (enemy_range == RANGE_FAR)
		return false;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		chance = 0.4f;
	else if (enemy_range == RANGE_MELEE)
		chance = 0.2f;
	else if (enemy_range == RANGE_NEAR)
		chance = 0.1f;
	else if (enemy_range == RANGE_MID)
		chance = 0.02f;
	else
		return false;

	if (skill->value == 0)
		chance *= 0.5f;
	else if (skill->value >= 2)
		chance *= 2;

	if (random() < chance)
	{
		self->monsterinfo.attack_state = AS_MISSILE;

		if (skill->value < 3)
			self->monsterinfo.attack_finished = level.time + 2000 * random();

		return true;
	}

	if (self->flags & FL_FLY)
	{
		if (random() < 0.3f)
			self->monsterinfo.attack_state = AS_SLIDING;
		else
			self->monsterinfo.attack_state = AS_STRAIGHT;
	}

	return false;
}


/*
===========
CheckAttack

The player is in view, so decide to move or launch an attack
Returns FALSE if movement should continue
============
*/
bool M_Q1_CheckAttack(edict_t *self)
{
	vec3_t spot1, spot2;
	edict_t *targ;
	float chance;
	targ = self->enemy;
	// see if any entities are in the way of the shot
	VectorCopy(self->s.origin, spot1);
	spot1[2] += self->viewheight;
	VectorCopy(targ->s.origin, spot2);
	spot2[2] += targ->viewheight;
	trace_t tr = gi.trace(spot1, vec3_origin, vec3_origin, spot2, self, MASK_SHOT | MASK_WATER);

	if (tr.ent != targ)
		return false;		// don't have a clear shot

	//if ( trace_inopen && trace_inwater)
	//	return FALSE;			// sight line crossed contents

	if (enemy_range == RANGE_MELEE)
	{
		// melee attack
		if (self->monsterinfo.melee)
		{
			if (self->entitytype == ET_Q1_MONSTER_KNIGHT)
			{
				float		len;
				// decide if now is a good swing time
				len = Distance(spot2, spot1);

				if (len < 80)
					self->monsterinfo.melee(self);
				else
					self->monsterinfo.attack(self);
			}
			else
				self->monsterinfo.melee(self);

			return true;
		}
	}

	// missile attack
	if (!self->monsterinfo.attack)
		return false;

	if (level.time < self->attack_finished_time)
		return false;

	if (enemy_range == RANGE_FAR)
		return false;

	if (enemy_range == RANGE_MELEE)
	{
		chance = 0.9f;
		self->attack_finished_time = 0;
	}
	else if (enemy_range == RANGE_NEAR)
	{
		if (self->monsterinfo.melee)
			chance = 0.2f;
		else
			chance = 0.4f;
	}
	else if (enemy_range == RANGE_MID)
	{
		if (self->monsterinfo.melee)
			chance = 0.05f;
		else
			chance = 0.1f;
	}
	else
		chance = 0;

	if (random() < chance)
	{
		self->monsterinfo.attack(self);
		AttackFinished(self, 2 * random());
		return true;
	}

	return false;
}

//
// P_CheckMissileRange
//
bool P_CheckMeleeRange(edict_t *self);

static bool P_CheckMissileRange(edict_t *self)
{
	float	dist;

	if (!enemy_vis)
		return false;

	if ((self->monsterinfo.aiflags & AI_JUST_HIT) && FacingIdeal(self))
	{
		// the target just hit the enemy,
		// so fight back!
		self->monsterinfo.aiflags &= ~AI_JUST_HIT;
		return true;
	}

	//if (actor->reactiontime)
	//	return false;	// do not attack yet
	// OPTIMIZE: get this from a global checksight
	dist = Distance(self->s.origin, self->enemy->s.origin) - 64;

	if (!self->monsterinfo.melee)
		dist -= 128;	// no melee attack, so fire more

	/*if (actor->type == MT_VILE)
	{
		if (dist > 14 * 64)
			return false;	// too far away
	}*/

	if (self->entitytype == ET_DOOM_MONSTER_SKEL)
	{
		if (dist < 196)
			return false;	// close for fist attack

		dist /= 1;
	}

	if (self->entitytype == ET_DOOM_MONSTER_SKUL
		|| self->entitytype == ET_DOOM_MONSTER_SPID
		/*|| self->entitytype == ET_DOOM_MONSTER_CYBORG*/)
		dist /= 2;

	if (dist > 200)
		dist = 200;

	if (dist < 0)
		dist = 0;

	//if (actor->type == MT_CYBORG && dist > 160)
	//	dist = 160;

	if ((Q_rand() % (256 * (4 - skill->integer))) < dist)
		return false;

	return true;
}

//
// A_Chase
// Actor has a melee attack,
// so it tries to close as fast as possible
//
bool M_Doom_CheckAttack(edict_t *self)
{
	//int		delta;

	// modify target threshold
	/*if (actor->threshold)
	{
		if (!actor->target
			|| actor->target->health <= 0)
		{
			actor->threshold = 0;
		}
		else
			actor->threshold--;
	}

	// turn towards movement direction if not there yet
	if (actor->movedir < 8)
	{
		actor->angle &= (7 << 29);
		delta = actor->angle - (actor->movedir << 29);

		if (delta > 0)
			actor->angle -= ANG90 / 2;
		else if (delta < 0)
			actor->angle += ANG90 / 2;
	}

	if (!actor->target
		|| !(actor->target->flags&MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true))
			return; 	// got a new target

		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	// do not attack twice in a row
	if (actor->flags & MF_JUSTATTACKED)
	{
		actor->flags &= ~MF_JUSTATTACKED;
		if (gameskill != sk_nightmare && !fastparm)
			P_NewChaseDir(actor);
		return;
	}*/
	if (level.time < self->monsterinfo.attack_finished)
	{
		if (self->monsterinfo.aiflags & AI_JUST_ATTACKED)
		{
			self->monsterinfo.aiflags &= ~AI_JUST_ATTACKED;

			if (self->enemy)
				SV_NewChaseDir(self, self->enemy->s.origin, 0);
		}

		return false;
	}

	// check for melee attack
	if (self->monsterinfo.melee && P_CheckMeleeRange(self))
	{
		if (self->monsterinfo.melee)
			self->monsterinfo.attack_state = AS_MELEE;
		else
			self->monsterinfo.attack_state = AS_MISSILE;

		return true;
	}
	// check for missile attack
	else if (self->monsterinfo.attack)
	{
		self->monsterinfo.attack_state = AS_MISSILE;

		if (!P_CheckMissileRange(self) || !CanDamage(self, self->enemy))
			goto nomissile;

		AttackFinished(self, 1 + ((3 - skill->integer) * random()));
		return true;
	}

	// ?
nomissile:
	// possibly choose another target
	/*if (netgame
		&& !actor->threshold
		&& !P_CheckSight(actor, actor->target))
	{
		if (P_LookForPlayers(actor, true))
			return;	// got a new target
	}

	// chase towards player
	if (--actor->movecount<0
		|| !P_Move(actor))
	{
		P_NewChaseDir(actor);
	}

	// make active sound
	if (actor->info->activesound
		&& P_Random() < 3)
	{
		S_StartSound(actor, actor->info->activesound);
	}*/
	self->monsterinfo.attack_state = AS_STRAIGHT;
	return false;
}


/*
=============
ai_run_melee

Turn and close until within an angle to launch a melee attack
=============
*/
static void ai_run_melee(edict_t *self)
{
	self->ideal_yaw = enemy_yaw;
	M_ChangeYaw(self);

	if (FacingIdeal(self))
	{
		self->monsterinfo.melee(self);
		self->monsterinfo.attack_state = AS_STRAIGHT;
	}
}


/*
=============
ai_run_missile

Turn in place until within an angle to launch a missile attack
=============
*/
static void ai_run_missile(edict_t *self)
{
	self->ideal_yaw = enemy_yaw;
	M_ChangeYaw(self);

	if (FacingIdeal(self))
	{
		self->monsterinfo.attack(self);
		self->monsterinfo.attack_state = AS_STRAIGHT;
	}
}


/*
=============
ai_run_slide

Strafe sideways, but stay at aproximately the same range
=============
*/
static void ai_run_slide(edict_t *self, float distance)
{
	float   ofs;
	self->ideal_yaw = enemy_yaw;
	M_ChangeYaw(self);

	if (self->monsterinfo.lefty)
		ofs = 90;
	else
		ofs = -90;

	if (M_walkmove(self, self->ideal_yaw + ofs, distance))
		return;

	self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
	M_walkmove(self, self->ideal_yaw - ofs, distance);
}


/*
=============
ai_checkattack

Decides if we're going to attack or do something else
used by ai_run and ai_stand
=============
*/
bool ai_checkattack(edict_t *self, float dist)
{
	vec3_t      temp;
	bool        hesDeadJim;

	// this causes monsters to run blindly to the combat point w/o firing
	if (self->goalentity)
	{
		if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
			return false;

		if (self->monsterinfo.aiflags & AI_SOUND_TARGET)
		{
			if ((level.time - self->enemy->teleport_time) > 5000)
			{
				if (self->goalentity == self->enemy)
				{
					if (self->movetarget)
						self->goalentity = self->movetarget;
					else
						self->goalentity = NULL;
				}

				self->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

				if (self->monsterinfo.aiflags & AI_TEMP_STAND_GROUND)
					self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);

				M_StopBotheringWithPaths(self);
			}
			else
			{
				self->show_hostile = level.time + 1;
				return false;
			}
		}
	}

	enemy_vis = false;
	// see if the enemy is dead
	hesDeadJim = false;

	if ((!self->enemy) || (!self->enemy->inuse))
	{
		if (self->monsterinfo.navigator)
			return false; // trying to go somewhere

		hesDeadJim = true;
	}
	else if (self->monsterinfo.aiflags & AI_MEDIC)
	{
		if (self->enemy->health > 0)
		{
			hesDeadJim = true;
			self->monsterinfo.aiflags &= ~AI_MEDIC;
		}
	}
	else
	{
		if (self->monsterinfo.aiflags & AI_BRUTAL)
		{
			if (self->enemy->health <= -80)
				hesDeadJim = true;
		}
		else
		{
			if (self->enemy->health <= 0)
				hesDeadJim = true;
		}
	}

	if (hesDeadJim)
	{
		self->enemy = NULL;

		// FIXME: look all around for other targets
		if (self->oldenemy && self->oldenemy->health > 0)
		{
			self->enemy = self->oldenemy;
			self->oldenemy = NULL;
			HuntTarget(self);
		}
		else
		{
			if (self->movetarget)
			{
				self->goalentity = self->movetarget;
				self->monsterinfo.walk(self);
			}
			else
			{
				// we need the pausetime otherwise the stand code
				// will just revert to walking with no target and
				// the monsters will wonder around aimlessly trying
				// to hunt the world entity
				self->monsterinfo.pausetime = GTIME_MAX;
				self->monsterinfo.stand(self);
			}

			return true;
		}
	}

	self->show_hostile = level.time + 1;        // wake up other monsters
	// check knowledge of enemy
	enemy_vis = visible(self, self->enemy);

	if (enemy_vis)
	{
		self->monsterinfo.search_time = level.time + 5000;
		VectorCopy(self->enemy->s.origin, self->monsterinfo.last_sighting);
	}

	// look for other coop players here
	//  if (coop && self->monsterinfo.search_time < level.time)
	//  {
	//      if (FindTarget (self))
	//          return true;
	//  }
	enemy_range = range(self, self->enemy);
	VectorSubtract(self->enemy->s.origin, self->s.origin, temp);
	enemy_yaw = vectoyaw(temp);

	// JDC self->ideal_yaw = enemy_yaw;

	if (self->monsterinfo.attack_state == AS_MISSILE)
	{
		ai_run_missile(self);
		return true;
	}

	if (self->monsterinfo.attack_state == AS_MELEE)
	{
		ai_run_melee(self);
		return true;
	}

	// if enemy is not currently visible, we will never attack
	if (!enemy_vis)
		return false;

	return self->monsterinfo.checkattack(self);
}

/*
=============
ai_run

The monster has an enemy it is trying to kill
=============
*/
void ai_run(edict_t *self, float dist)
{
	self->count = 0;

	if (!self->goalentity && !self->monsterinfo.navigator && !self->enemy)
	{
		self->monsterinfo.pausetime = GTIME_MAX;
		self->monsterinfo.stand(self);
		return;
	}

	// if we're going to a combat point, just proceed
	if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
	{
		M_MoveToGoal(self, dist);
		return;
	}

	if (self->monsterinfo.aiflags & AI_SOUND_TARGET)
	{
		vec3_t v;
		VectorSubtract(self->s.origin, self->monsterinfo.last_sighting, v);

		if (VectorLength(v) < 64)
		{
			M_StopBotheringWithPaths(self);
			self->monsterinfo.aiflags |= (AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
			self->monsterinfo.pausetime = GTIME_MAX;
			self->monsterinfo.stand(self);
			return;
		}

		M_MoveToGoal(self, dist);

		if (!FindTarget(self))
			return;
	}

	if (ai_checkattack(self, dist))
		return;

	if (self->monsterinfo.attack_state == AS_SLIDING)
	{
		ai_run_slide(self, dist);
		return;
	}

	if (enemy_vis)
	{
		M_MoveToGoal(self, dist);

		if (self->monsterinfo.aiflags & AI_LOST_SIGHT)
		{
			M_StopBotheringWithPaths(self);
			self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;

			if (self->enemy)
				VectorCopy(self->enemy->s.origin, self->monsterinfo.last_sighting);

			self->monsterinfo.trail_time = level.time;
		}

		return;
	}

	// if we don't have a navigator, start searchin'
	if (!self->monsterinfo.navigator)
	{
		if ((self->monsterinfo.search_time) && (level.time > (self->monsterinfo.search_time + 20000)))
		{
			M_MoveToGoal(self, dist);
			self->monsterinfo.search_time = 0;
			return;
		}

		if (!(self->monsterinfo.aiflags & AI_LOST_SIGHT))
		{
			self->monsterinfo.aiflags |= AI_LOST_SIGHT;
			//path us to where we last saw the guy
			M_NavigatorPathToSpot(self, self->monsterinfo.last_sighting);
			self->monsterinfo.trail_time = level.time;
		}
	}

	M_MoveToGoal(self, dist);
}

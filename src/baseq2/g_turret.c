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
// g_turret.c

#ifdef ENABLE_COOP
#include "g_local.h"


void AnglesNormalize(vec3_t vec)
{
	while (vec[0] > 360)
		vec[0] -= 360;

	while (vec[0] < 0)
		vec[0] += 360;

	while (vec[1] > 360)
		vec[1] -= 360;

	while (vec[1] < 0)
		vec[1] += 360;
}

float SnapToEights(float x)
{
	x *= 8.0f;

	if (x > 0.0f)
		x += 0.5f;
	else
		x -= 0.5f;

	return 0.125f * (int)x;
}


void turret_blocked(edict_t *self, edict_t *other)
{
	edict_t *attacker;

	if (other->takedamage)
	{
		if (self->teammaster->server.owner)
			attacker = self->teammaster->server.owner;
		else
			attacker = self->teammaster;

		T_Damage(other, self, attacker, vec3_origin, other->server.state.origin, vec3_origin, self->teammaster->dmg, 10, DAMAGE_NONE, MakeBlankMeansOfDeath(self));
	}
}

/*QUAKED turret_breach (0 0 0) ?
This portion of the turret can change both pitch and yaw.
The model  should be made with a flat pitch.
It (and the associated base) need to be oriented towards 0.
Use "angle" to set the starting angle.

"speed"     default 50
"dmg"       default 10
"angle"     point this forward
"target"    point this at an info_notnull at the muzzle tip
"minpitch"  min acceptable pitch angle : default -30
"maxpitch"  max acceptable pitch angle : default 30
"minyaw"    min acceptable yaw angle   : default 0
"maxyaw"    max acceptable yaw angle   : default 360
*/

void turret_breach_fire(edict_t *self)
{
	vec3_t  f, r, u;
	vec3_t  start;
	int     damage;
	int     speed;
	AngleVectors(self->server.state.angles, f, r, u);
	VectorMA(self->server.state.origin, self->move_origin[0], f, start);
	VectorMA(start, self->move_origin[1], r, start);
	VectorMA(start, self->move_origin[2], u, start);
	damage = 100 + random() * 50;
	speed = 550 + 50 * skill->value;
	fire_rocket(self->teammaster->server.owner, start, f, damage, speed, 150, damage);
	gi.positioned_sound(start, self, CHAN_WEAPON, gi.soundindex("weapons/rocklf1a.wav"), 1, ATTN_NORM, 0);
}

void turret_breach_think(edict_t *self)
{
	edict_t *ent;
	vec3_t  current_angles;
	vec3_t  delta;
	VectorCopy(self->server.state.angles, current_angles);
	AnglesNormalize(current_angles);
	AnglesNormalize(self->move_angles);

	if (self->move_angles[PITCH] > 180)
		self->move_angles[PITCH] -= 360;

	// clamp angles to mins & maxs
	if (self->move_angles[PITCH] > self->pos1[PITCH])
		self->move_angles[PITCH] = self->pos1[PITCH];
	else if (self->move_angles[PITCH] < self->pos2[PITCH])
		self->move_angles[PITCH] = self->pos2[PITCH];

	if ((self->move_angles[YAW] < self->pos1[YAW]) || (self->move_angles[YAW] > self->pos2[YAW]))
	{
		float   dmin, dmax;
		dmin = fabsf(self->pos1[YAW] - self->move_angles[YAW]);

		if (dmin < -180)
			dmin += 360;
		else if (dmin > 180)
			dmin -= 360;

		dmax = fabsf(self->pos2[YAW] - self->move_angles[YAW]);

		if (dmax < -180)
			dmax += 360;
		else if (dmax > 180)
			dmax -= 360;

		if (fabsf(dmin) < fabsf(dmax))
			self->move_angles[YAW] = self->pos1[YAW];
		else
			self->move_angles[YAW] = self->pos2[YAW];
	}

	VectorSubtract(self->move_angles, current_angles, delta);

	if (delta[0] < -180)
		delta[0] += 360;
	else if (delta[0] > 180)
		delta[0] -= 360;

	if (delta[1] < -180)
		delta[1] += 360;
	else if (delta[1] > 180)
		delta[1] -= 360;

	delta[2] = 0;

	if (delta[0] > self->speed * game.frameseconds)
		delta[0] = self->speed * game.frameseconds;

	if (delta[0] < -1 * self->speed * game.frameseconds)
		delta[0] = -1 * self->speed * game.frameseconds;

	if (delta[1] > self->speed * game.frameseconds)
		delta[1] = self->speed * game.frameseconds;

	if (delta[1] < -1 * self->speed * game.frameseconds)
		delta[1] = -1 * self->speed * game.frameseconds;

	VectorScale(delta, 1.0f / game.frameseconds, self->avelocity);
	self->nextthink = level.time + 1;

	for (ent = self->teammaster; ent; ent = ent->teamchain)
		ent->avelocity[1] = self->avelocity[1];

	// if we have adriver, adjust his velocities
	if (self->server.owner)
	{
		float   angle;
		float   target_z;
		float   diff;
		vec3_t  target;
		vec3_t  dir;
		// angular is easy, just copy ours
		self->server.owner->avelocity[0] = self->avelocity[0];
		self->server.owner->avelocity[1] = self->avelocity[1];
		// x & y
		angle = self->server.state.angles[1] + self->server.owner->move_origin[1];
		angle = DEG2RAD(angle);
		target[0] = SnapToEights(self->server.state.origin[0] + cosf(angle) * self->server.owner->move_origin[0]);
		target[1] = SnapToEights(self->server.state.origin[1] + sinf(angle) * self->server.owner->move_origin[0]);
		target[2] = self->server.owner->server.state.origin[2];
		VectorSubtract(target, self->server.owner->server.state.origin, dir);
		self->server.owner->velocity[0] = dir[0] * 1.0f / game.frameseconds;
		self->server.owner->velocity[1] = dir[1] * 1.0f / game.frameseconds;
		// z
		angle = DEG2RAD(self->server.state.angles[PITCH]);
		target_z = SnapToEights(self->server.state.origin[2] + self->server.owner->move_origin[0] * tanf(angle) + self->server.owner->move_origin[2]);
		diff = target_z - self->server.owner->server.state.origin[2];
		self->server.owner->velocity[2] = diff * 1.0f / game.frameseconds;

		if (self->spawnflags & 65536)
		{
			turret_breach_fire(self);
			self->spawnflags &= ~65536;
		}
	}
}

void turret_breach_finish_init(edict_t *self)
{
	// get and save info for muzzle location
	if (!self->target)
		Com_Printf("entityid %i at %s needs a target\n", self->entitytype, vtos(self->server.state.origin));
	else
	{
		self->target_ent = G_PickTarget(self->target);
		VectorSubtract(self->target_ent->server.state.origin, self->server.state.origin, self->move_origin);
		G_FreeEdict(self->target_ent);
	}

	self->teammaster->dmg = self->dmg;
	self->think = turret_breach_think;
	self->think(self);
}

void SP_turret_breach(edict_t *self)
{
	self->server.solid = SOLID_BSP;
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);

	if (!self->speed)
		self->speed = 50;

	if (!self->dmg)
		self->dmg = 10;

	if (!spawnTemp.minpitch)
		spawnTemp.minpitch = -30;

	if (!spawnTemp.maxpitch)
		spawnTemp.maxpitch = 30;

	if (!spawnTemp.maxyaw)
		spawnTemp.maxyaw = 360;

	self->pos1[PITCH] = -1 * spawnTemp.minpitch;
	self->pos1[YAW]   = spawnTemp.minyaw;
	self->pos2[PITCH] = -1 * spawnTemp.maxpitch;
	self->pos2[YAW]   = spawnTemp.maxyaw;
	self->ideal_yaw = self->server.state.angles[YAW];
	self->move_angles[YAW] = self->ideal_yaw;
	self->blocked = turret_blocked;
	self->think = turret_breach_finish_init;
	self->nextthink = level.time + 1;
	gi.linkentity(self);
}


/*QUAKED turret_base (0 0 0) ?
This portion of the turret changes yaw only.
MUST be teamed with a turret_breach.
*/

void SP_turret_base(edict_t *self)
{
	self->server.solid = SOLID_BSP;
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);
	self->blocked = turret_blocked;
	gi.linkentity(self);
}


/*QUAKED turret_driver (1 .5 0) (-16 -16 -24) (16 16 32)
Must NOT be on the team with the rest of the turret parts.
Instead it must target the turret_breach.
*/

void infantry_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
void infantry_stand(edict_t *self);
void monster_use(edict_t *self, edict_t *other, edict_t *activator);

void turret_driver_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	edict_t *ent;
	// level the gun
	self->target_ent->move_angles[0] = 0;

	// remove the driver from the end of them team chain
	for (ent = self->target_ent->teammaster; ent->teamchain != self; ent = ent->teamchain)
		;

	ent->teamchain = NULL;
	self->teammaster = NULL;
	self->flags &= ~FL_TEAMSLAVE;
	self->target_ent->server.owner = NULL;
	self->target_ent->teammaster->server.owner = NULL;
	infantry_die(self, inflictor, attacker, damage, point);

	self->die = infantry_die;
	self->think = monster_think;
}

bool FindTarget(edict_t *self);

void turret_driver_think(edict_t *self)
{
	vec3_t  target;
	vec3_t  dir;
	float   reaction_time;
	self->nextthink = level.time + 1;

	if (self->enemy && (!self->enemy->server.inuse || self->enemy->health <= 0))
		self->enemy = NULL;

	if (!self->enemy)
	{
		if (!FindTarget(self))
			return;

		self->monsterinfo.trail_time = level.time;
		self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
	}
	else
	{
		if (visible(self, self->enemy))
		{
			if (self->monsterinfo.aiflags & AI_LOST_SIGHT)
			{
				self->monsterinfo.trail_time = level.time;
				self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
			}
		}
		else
		{
			self->monsterinfo.aiflags |= AI_LOST_SIGHT;
			return;
		}
	}

	// let the turret know where we want it to aim
	VectorCopy(self->enemy->server.state.origin, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, self->target_ent->server.state.origin, dir);
	vectoangles(dir, self->target_ent->move_angles);

	// decide if we should shoot
	if (level.time < self->monsterinfo.attack_finished)
		return;

	reaction_time = (3 - skill->value) * 1.0f;

	if ((level.time - self->monsterinfo.trail_time) < reaction_time)
		return;

	self->monsterinfo.attack_finished = level.time + (reaction_time * 1000) + 1000;
	//FIXME how do we really want to pass this along?
	self->target_ent->spawnflags |= 65536;
}

void turret_driver_link(edict_t *self)
{
	vec3_t  vec;
	edict_t *ent;
	self->think = turret_driver_think;
	self->nextthink = level.time + 1;
	self->target_ent = G_PickTarget(self->target);
	self->target_ent->server.owner = self;
	self->target_ent->teammaster->server.owner = self;
	VectorCopy(self->target_ent->server.state.angles, self->server.state.angles);
	vec[0] = self->target_ent->server.state.origin[0] - self->server.state.origin[0];
	vec[1] = self->target_ent->server.state.origin[1] - self->server.state.origin[1];
	vec[2] = 0;
	self->move_origin[0] = VectorLength(vec);
	VectorSubtract(self->server.state.origin, self->target_ent->server.state.origin, vec);
	vectoangles(vec, vec);
	AnglesNormalize(vec);
	self->move_origin[1] = vec[1];
	self->move_origin[2] = self->server.state.origin[2] - self->target_ent->server.state.origin[2];

	// add the driver to the end of them team chain
	for (ent = self->target_ent->teammaster; ent->teamchain; ent = ent->teamchain)
		;

	ent->teamchain = self;
	self->teammaster = self->target_ent->teammaster;
	self->flags |= FL_TEAMSLAVE;
}

void SP_monster_infantry(edict_t *self);

void SP_turret_driver(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	SP_monster_infantry(self);

	self->movetype = MOVETYPE_PUSH;
	self->viewheight = 24;
	self->die = turret_driver_die;
	self->flags |= FL_NO_KNOCKBACK;
	VectorCopy(self->server.state.origin, self->server.state.old_origin);
	self->monsterinfo.aiflags |= AI_STAND_GROUND;
	self->think = turret_driver_link;
	self->nextthink = level.time + 1;
	self->server.state.frame = 0;
	gi.linkentity(self);
}

#endif
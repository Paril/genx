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
// m_move.c -- monster movement

#include "g_local.h"

#define STEPSIZE    18

bool M_NavigatorPathToSpot(edict_t *self, vec3_t spot);

/*
=============
M_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/
int c_yes, c_no;

bool M_CheckBottom(edict_t *ent)
{
	vec3_t  mins, maxs, start, stop;
	trace_t trace;
	int     x, y;
	float   mid, bottom;
	VectorAdd(ent->s.origin, ent->mins, mins);
	VectorAdd(ent->s.origin, ent->maxs, maxs);
	// if all of the points under the corners are solid world, don't bother
	// with the tougher checks
	// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;

	for (x = 0 ; x <= 1 ; x++)
		for (y = 0 ; y <= 1 ; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];

			if (gi.pointcontents(start) != CONTENTS_SOLID)
				goto realcheck;
		}

	c_yes++;
	return true;        // we got out easy
realcheck:
	c_no++;
	//
	// check it for real...
	//
	start[2] = mins[2];
	// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0]) * 0.5f;
	start[1] = stop[1] = (mins[1] + maxs[1]) * 0.5f;
	stop[2] = start[2] - 2 * STEPSIZE;
	trace = gi.trace(start, vec3_origin, vec3_origin, stop, ent, MASK_MONSTERSOLID);

	if (trace.fraction == 1.0f)
		return false;

	mid = bottom = trace.endpos[2];

	// the corners must be within 16 of the midpoint
	for (x = 0 ; x <= 1 ; x++)
		for (y = 0 ; y <= 1 ; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];
			trace = gi.trace(start, vec3_origin, vec3_origin, stop, ent, MASK_MONSTERSOLID);

			if (trace.fraction != 1.0f && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];

			if (trace.fraction == 1.0f || mid - trace.endpos[2] > STEPSIZE)
				return false;
		}

	c_yes++;
	return true;
}


/*
=============
SV_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done, false is returned, and
pr_global_struct->trace_normal is set to the normal of the blocking wall
=============
*/
//FIXME since we need to test end position contents here, can we avoid doing
//it again later in catagorize position?
bool SV_movestep(edict_t *ent, vec3_t move, bool relink)
{
	float       dz;
	vec3_t      oldorg, neworg, end;
	trace_t     trace;
	int         i;
	float       stepsize;
	vec3_t      test;
	int         contents;

	if (ent->s.game == GAME_DOOM && ent->health)
	{
		if (ent->flags & FL_FLY)
			VectorClear(ent->velocity);
		else
			ent->velocity[0] = ent->velocity[1] = 0;
	}

	// try the move
	VectorCopy(ent->s.origin, oldorg);
	VectorAdd(ent->s.origin, move, neworg);
	vec3_t target_origin;
	bool has_target = false;

	if (ent->monsterinfo.aiflags & AI_NODE_PATH)
	{
		has_target = true;
		VectorCopy(ent->monsterinfo.navigator_pos, target_origin);
	}
	else if (ent->enemy || ent->goalentity)
	{
		if (!ent->goalentity)
			ent->goalentity = ent->enemy;

		has_target = true;
		VectorCopy(ent->goalentity->s.origin, target_origin);
	}

	// flying monsters don't step up
	if (ent->flags & (FL_SWIM | FL_FLY))
	{
		// try one move with vertical motion, then one without
		for (i = 0 ; i < 2 ; i++)
		{
			VectorAdd(ent->s.origin, move, neworg);

			if (i == 0 && has_target)
			{
				dz = ent->s.origin[2] - target_origin[2];

				if (ent->goalentity && ent->goalentity->client)
				{
					if (dz > 40)
						neworg[2] -= 8;

					if (!((ent->flags & FL_SWIM) && (ent->waterlevel < 2)) && dz < 30)
						neworg[2] += 8;
				}
				else
				{
					if (dz > 8)
						neworg[2] -= 8;
					else if (dz > 0)
						neworg[2] -= dz;
					else if (dz < -8)
						neworg[2] += 8;
					else
						neworg[2] += dz;
				}
			}

			trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, neworg, ent, MASK_MONSTERSOLID);

			// fly monsters don't enter water voluntarily
			if (ent->flags & FL_FLY)
			{
				if (!ent->waterlevel)
				{
					test[0] = trace.endpos[0];
					test[1] = trace.endpos[1];
					test[2] = trace.endpos[2] + ent->mins[2] + 1;
					contents = gi.pointcontents(test);

					if (contents & MASK_WATER)
						return false;
				}
			}

			// swim monsters don't exit water voluntarily
			if (ent->flags & FL_SWIM)
			{
				if (ent->waterlevel < 2)
				{
					test[0] = trace.endpos[0];
					test[1] = trace.endpos[1];
					test[2] = trace.endpos[2] + ent->mins[2] + 1;
					contents = gi.pointcontents(test);

					if (!(contents & MASK_WATER))
						return false;
				}
			}

			if (trace.fraction == 1)
			{
				VectorCopy(trace.endpos, ent->s.origin);

				if (relink)
				{
					gi.linkentity(ent);
					G_TouchTriggers(ent);
				}

				return true;
			}

			if (!ent->enemy)
				break;
		}

		return false;
	}

	// push down from a step height above the wished position
	if (!(ent->monsterinfo.aiflags & AI_NOSTEP))
		stepsize = STEPSIZE;
	else
		stepsize = 1;

	neworg[2] += stepsize;
	VectorCopy(neworg, end);
	end[2] -= stepsize * 2;
	trace = gi.trace(neworg, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);

	// Paril: if we've been told that we're stuck in a wall, we will
	// ignore bad clipping planes so long as the move doesn't push us entirely into a wall.
	if (trace.allsolid)
		return false;

	if (trace.startsolid)
	{
		neworg[2] -= stepsize;
		trace = gi.trace(neworg, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);

		if (trace.allsolid || trace.startsolid)
			return false;
	}

	// Paril: if we were stuck and the new position is good, we can stop pseudo-noclipping now
	if (ent->flags & FL_STUCK)
	{
		if (!gi.trace(ent->s.origin, ent->pos1, ent->pos2, ent->s.origin, ent, MASK_MONSTERSOLID).startsolid)
		{
			ent->flags &= ~FL_STUCK;
			VectorCopy(ent->pos1, ent->mins);
			VectorCopy(ent->pos2, ent->maxs);
			relink = true;
		}
	}

	// don't go in to water
	/*if (ent->waterlevel == 0) {
		test[0] = trace.endpos[0];
		test[1] = trace.endpos[1];
		test[2] = trace.endpos[2] + ent->mins[2] + 1;
		contents = gi.pointcontents(test);

		if (contents & MASK_WATER)
			return false;
	}*/

	if (trace.fraction == 1)
	{
		// if monster had the ground pulled out, go ahead and fall
		if (ent->flags & FL_PARTIALGROUND)
		{
			VectorAdd(ent->s.origin, move, ent->s.origin);

			if (relink)
			{
				gi.linkentity(ent);
				G_TouchTriggers(ent);
			}

			ent->groundentity = NULL;
			return true;
		}

		if (target_origin[2] >= ent->s.origin[2])
			return false;       // walked off an edge
	}

	// check point traces down for dangling corners
	VectorCopy(trace.endpos, ent->s.origin);

	if (!M_CheckBottom(ent))
	{
		if (ent->flags & FL_PARTIALGROUND)
		{
			// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
			{
				gi.linkentity(ent);
				G_TouchTriggers(ent);
			}

			return true;
		}

		/*if (target_origin[2] >= ent->s.origin[2])
		{
			VectorCopy(oldorg, ent->s.origin);
			return false;
		}*/
	}

	if (ent->flags & FL_PARTIALGROUND)
		ent->flags &= ~FL_PARTIALGROUND;

	ent->groundentity = trace.ent;
	ent->groundentity_linkcount = trace.ent->linkcount;

	// the move is ok
	if (relink)
	{
		gi.linkentity(ent);
		G_TouchTriggers(ent);
	}

	return true;
}


//============================================================================

/*
===============
M_ChangeYaw

===============
*/
static void M_ChangeAngle(edict_t *ent, int which, float ideal, bool mod)
{
	float   current;
	float   move;
	float   speed;
	current = mod ? anglemod(ent->s.angles[which]) : ent->s.angles[which];

	if (current == ideal)
		return;

	move = ideal - current;
	speed = ent->yaw_speed * 3;

	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}

	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}

	ent->s.angles[which] = mod ? anglemod(current + move) : (current + move);
}

/*
===============
M_ChangeYaw

===============
*/
void M_ChangeYaw(edict_t *ent)
{
	M_ChangeAngle(ent, YAW, ent->ideal_yaw, true);
	M_ChangeAngle(ent, PITCH, ent->ideal_pitch, false);
}


/*
======================
SV_StepDirection

Turns to the movement direction, and walks the current distance if
facing it.

======================
*/
bool SV_StepDirection(edict_t *ent, float yaw, float dist)
{
	vec3_t      move, oldorigin;
	float       delta;
	ent->ideal_yaw = yaw;
	M_ChangeYaw(ent);
	yaw = DEG2RAD(yaw);
	move[0] = cosf(yaw) * dist;
	move[1] = sinf(yaw) * dist;
	move[2] = 0;
	VectorCopy(ent->s.origin, oldorigin);

	if (SV_movestep(ent, move, false))
	{
		delta = ent->s.angles[YAW] - ent->ideal_yaw;

		if (delta > 45 && delta < 315)
		{
			// not turned far enough, so don't take the step
			VectorCopy(oldorigin, ent->s.origin);
		}

		gi.linkentity(ent);
		G_TouchTriggers(ent);
		return true;
	}

	gi.linkentity(ent);
	G_TouchTriggers(ent);
	return false;
}

/*
======================
SV_FixCheckBottom

======================
*/
void SV_FixCheckBottom(edict_t *ent)
{
	ent->flags |= FL_PARTIALGROUND;
}



/*
================
SV_NewChaseDir

================
*/
#define DI_NODIR    -1
void SV_NewChaseDir(edict_t *actor, vec3_t enemy_origin, float dist)
{
	float   deltax, deltay;
	float   d[3];
	float   tdir, olddir, turnaround;

	//FIXME: how did we get here with no enemy
	if (!enemy_origin)
		return;

	deltax = enemy_origin[0] - actor->s.origin[0];
	deltay = enemy_origin[1] - actor->s.origin[1];
	olddir = actor->ideal_yaw;
	//if (actor->monsterinfo.aiflags & AI_NODE_PATH)
	//{
	d[0] = deltax;
	d[1] = deltay;
	d[2] = 0;
	tdir = vectoyaw(d);

	if (SV_StepDirection(actor, tdir, dist))
		return;

	//}
	//else
	//    olddir = anglemod((int)(actor->ideal_yaw / 45) * 45);
	turnaround = anglemod(olddir - 180);

	if (deltax > 10)
		d[1] = 0;
	else if (deltax < -10)
		d[1] = 180;
	else
		d[1] = DI_NODIR;

	if (deltay < -10)
		d[2] = 270;
	else if (deltay > 10)
		d[2] = 90;
	else
		d[2] = DI_NODIR;

	// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		if (d[1] == 0)
			tdir = d[2] == 90 ? 45 : 315;
		else
			tdir = d[2] == 90 ? 135 : 215;

		if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
			return;
	}

	// try other directions
	if (((Q_rand() & 3) & 1) || fabsf(deltay) > fabsf(deltax))
	{
		tdir = d[1];
		d[1] = d[2];
		d[2] = tdir;
	}

	if (d[1] != DI_NODIR && d[1] != turnaround
		&& SV_StepDirection(actor, d[1], dist))
		return;

	if (d[2] != DI_NODIR && d[2] != turnaround
		&& SV_StepDirection(actor, d[2], dist))
		return;

	/* there is no direct path to the player, so pick another direction */

	if (olddir != DI_NODIR && SV_StepDirection(actor, olddir, dist))
		return;

	if (Q_rand() & 1)   /*randomly determine direction of search*/
	{
		for (tdir = 0 ; tdir <= 315 ; tdir += 45)
			if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
				return;
	}
	else
	{
		for (tdir = 315 ; tdir >= 0 ; tdir -= 45)
			if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
				return;
	}

	if (turnaround != DI_NODIR && SV_StepDirection(actor, turnaround, dist))
		return;

	actor->ideal_yaw = olddir;      // can't move

	// if a bridge was pulled out from underneath a monster, it may not have
	// a valid standing position at all

	if (!M_CheckBottom(actor))
		SV_FixCheckBottom(actor);
}

/*
======================
SV_CloseEnough

======================
*/
bool SV_CloseEnough(edict_t *ent, vec3_t goal_absmin, vec3_t goal_absmax, float dist)
{
	int     i;

	for (i = 0 ; i < 3 ; i++)
	{
		if (goal_absmin[i] > ent->absmax[i] + dist)
			return false;

		if (goal_absmax[i] < ent->absmin[i] - dist)
			return false;
	}

	return true;
}

void M_StopBotheringWithPaths(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_NODE_PATH)
	{
		self->monsterinfo.aiflags &= ~AI_NODE_PATH;
		gi.Nav_DestroyNavigator(self->monsterinfo.navigator);
		self->monsterinfo.navigator = NULL;
	}
}

void plat_go_up(edict_t *self);

void M_NavigatorNodeReached(edict_t *self)
{
	nav_node_id reached = gi.Nav_GetNavigatorPathNode(self->monsterinfo.navigator, self->monsterinfo.navigator_node_current);
	self->monsterinfo.navigator_node_current++;

	if (self->monsterinfo.navigator_node_current >= self->monsterinfo.navigator_node_max)
	{
		M_StopBotheringWithPaths(self);

		if (invasion->integer && self->enemy && M_NavigatorPathToSpot(self, self->enemy->s.origin))
		{
			// brutal monsters will keep hunting forever
		}
		else
		{
			// TODO: do something better here
			self->enemy = self->goalentity = NULL;
			self->monsterinfo.stand(self);
		}

		return;
	}

	gi.Nav_GetNodePosition(gi.Nav_GetNavigatorPathNode(self->monsterinfo.navigator, self->monsterinfo.navigator_node_current), self->monsterinfo.navigator_pos);

	// if the node we touched is a plat node, wait for it to go all the way up
	if (gi.Nav_GetNodeType(reached) == NAV_NODE_PLATFORM)
	{
		vec3_t start, end;
		gi.Nav_GetNodePosition(reached, start);
		gi.Nav_GetNodePosition(reached, end);
		end[2] -= 32;
		trace_t tr = gi.trace(start, vec3_origin, vec3_origin, end, self, MASK_SOLID | CONTENTS_MONSTER);

		if (tr.fraction == 1.0f || tr.ent->entitytype != ET_FUNC_PLAT)
		{
			M_StopBotheringWithPaths(self);
			// plat is bad for some reason
		}
		else
		{
			// platform valid to use for us
			if (tr.ent->moveinfo.state == STATE_BOTTOM)
				plat_go_up(tr.ent);
			else
			{
				// plat is already moving, we got here too late, find another way
				M_StopBotheringWithPaths(self);
			}
		}

		self->monsterinfo.stand(self);
		self->monsterinfo.pausetime = level.time + 5000;
	}
}

bool M_NavCloseFunc(nav_node_id node)
{
	vec3_t start, end;

	switch (gi.Nav_GetNodeType(node))
	{
		case NAV_NODE_NORMAL:
			return true;

		case NAV_NODE_PLATFORM:
			gi.Nav_GetNodePosition(node, start);
			gi.Nav_GetNodePosition(node, end);
			end[2] -= 32;
			trace_t tr = gi.trace(start, vec3_origin, vec3_origin, end, NULL, MASK_SOLID | CONTENTS_MONSTER);

			if (tr.fraction == 1.0f)
				return false;
			else if (tr.ent->entitytype != ET_FUNC_PLAT)
				return false;

			// platform valid to use for us
			if (tr.ent->moveinfo.state == STATE_BOTTOM)
				return true;

			break;
	}

	return false;
}

bool M_NavigatorPathToSpot(edict_t *self, vec3_t spot)
{
	if (self->monsterinfo.navigator)
		M_StopBotheringWithPaths(self);

	self->monsterinfo.navigator = gi.Nav_CreateNavigator();
	gi.Nav_SetNavigatorStartNode(self->monsterinfo.navigator, gi.Nav_GetClosestNode(self->s.origin));
	gi.Nav_SetNavigatorEndNode(self->monsterinfo.navigator, gi.Nav_GetClosestNode(spot));
	gi.Nav_SetNavigatorClosedNodeFunction(self->monsterinfo.navigator, M_NavCloseFunc);

	if (!gi.Nav_NavigatorAStar(self->monsterinfo.navigator))
	{
		M_StopBotheringWithPaths(self);
		return false;
	}

	self->monsterinfo.aiflags |= AI_NODE_PATH;
	self->monsterinfo.navigator_node_current = 0;
	self->monsterinfo.navigator_node_max = gi.Nav_GetNavigatorPathCount(self->monsterinfo.navigator);
	gi.Nav_GetNodePosition(gi.Nav_GetNavigatorPathNode(self->monsterinfo.navigator, self->monsterinfo.navigator_node_current), self->monsterinfo.navigator_pos);
	return true;
}

/*
======================
M_MoveToGoal
======================
*/
void M_MoveToGoal(edict_t *ent, float dist)
{
	if (!ent->groundentity && !(ent->flags & (FL_FLY | FL_SWIM)))
		return;

	if (ent->monsterinfo.aiflags & AI_NODE_PATH)
	{
		/*MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(TE_BFG_LASER);
		MSG_WritePos(ent->s.origin);
		MSG_WritePos(ent->monsterinfo.navigator_pos);
		gi.multicast(ent->s.origin, MULTICAST_PVS);*/
		vec3_t path_absmin = { -4, -4, -4 }, path_absmax = { 4, 4, 4 };
		VectorAdd(ent->monsterinfo.navigator_pos, path_absmin, path_absmin);
		VectorAdd(ent->monsterinfo.navigator_pos, path_absmax, path_absmax);

		// if the next step hits the path, change path
		if (SV_CloseEnough(ent, path_absmin, path_absmax, dist))
		{
			M_NavigatorNodeReached(ent);
			SV_NewChaseDir(ent, ent->monsterinfo.navigator_pos, dist);
		}
		else
		{
			// bump around...
			if ((Q_rand() & 3) == 1 || !SV_StepDirection(ent, ent->ideal_yaw, dist))
			{
				if (ent->inuse)
					SV_NewChaseDir(ent, ent->monsterinfo.navigator_pos, dist);
			}
		}
	}
	else if (ent->goalentity)
	{
		edict_t *goal = ent->goalentity;

		// if the next step hits the enemy, return immediately
		if (ent->enemy && SV_CloseEnough(ent, ent->enemy->absmin, ent->enemy->absmax, dist))
			return;

		// bump around...
		if ((Q_rand() & 3) == 1 || !SV_StepDirection(ent, ent->ideal_yaw, dist))
		{
			if (ent->inuse)
				SV_NewChaseDir(ent, goal->s.origin, dist);
		}
	}
}


/*
===============
M_walkmove
===============
*/
bool M_walkmove(edict_t *ent, float yaw, float dist)
{
	vec3_t  move;

	if (!ent->groundentity && !(ent->flags & (FL_FLY | FL_SWIM)))
		return false;

	yaw = DEG2RAD(yaw);
	move[0] = cosf(yaw) * dist;
	move[1] = sinf(yaw) * dist;
	move[2] = 0;
	return SV_movestep(ent, move, true);
}

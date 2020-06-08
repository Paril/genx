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
// g_phys.c

#include "g_local.h"

/*


pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects

doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
corpses are SOLID_NOT and MOVETYPE_TOSS
crates are SOLID_BBOX and MOVETYPE_TOSS
walking monsters are SOLID_SLIDEBOX and MOVETYPE_STEP
flying/floating monsters are SOLID_SLIDEBOX and MOVETYPE_FLY

solid_edge items only clip against bsp models.

*/


/*
============
SV_TestEntityPosition

============
*/
edict_t *SV_TestEntityPosition(edict_t *ent)
{
	trace_t trace;
	int     mask;

	if (ent->server.clipmask)
		mask = ent->server.clipmask;
	else
		mask = MASK_SOLID;

	trace = gi.trace(ent->server.state.origin, ent->server.mins, ent->server.maxs, ent->server.state.origin, ent, mask);

	if (trace.startsolid)
		return g_edicts;

	return NULL;
}


/*
================
SV_CheckVelocity
================
*/
void SV_CheckVelocity(edict_t *ent)
{
	int     i;

	//
	// bound velocity
	//
	for (i = 0 ; i < 3 ; i++)
	{
		if (ent->velocity[i] > sv_maxvelocity->value)
			ent->velocity[i] = sv_maxvelocity->value;
		else if (ent->velocity[i] < -sv_maxvelocity->value)
			ent->velocity[i] = -sv_maxvelocity->value;
	}
}

/*
=============
SV_RunThink

Runs thinking code for this frame if necessary
=============
*/
bool SV_RunThink(edict_t *ent)
{
	gtime_t thinktime;
	thinktime = ent->nextthink;

	if (!thinktime)
		return true;

	if (thinktime > level.time)
		return true;

	ent->nextthink = 0;

	if (ent->think)
		ent->think(ent);

	return false;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void SV_Impact(edict_t *e1, trace_t *trace)
{
	edict_t     *e2;
	//  cplane_t    backplane;
	e2 = trace->ent;

	if (e1->touch && e1->server.solid != SOLID_NOT)
		e1->touch(e1, e2, &trace->plane, trace->surface);

	if (e2->touch && e2->server.solid != SOLID_NOT)
		e2->touch(e2, e1, NULL, NULL);
}


/*
==================
ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define STOP_EPSILON    0.1f

int ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float   backoff;
	float   change;
	int     i, blocked;
	blocked = 0;

	if (normal[2] > 0)
		blocked |= 1;       // floor

	if (!normal[2])
		blocked |= 2;       // step

	backoff = DotProduct(in, normal) * overbounce;

	for (i = 0 ; i < 3 ; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;

		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}

	return blocked;
}


/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
============
*/
#define MAX_CLIP_PLANES 5
int SV_FlyMove(edict_t *ent, float time, int mask)
{
	edict_t     *hit;
	int         bumpcount, numbumps;
	vec3_t      dir;
	float       d;
	int         numplanes;
	vec3_t      planes[MAX_CLIP_PLANES];
	vec3_t      primal_velocity, original_velocity, new_velocity;
	int         i, j;
	trace_t     trace;
	vec3_t      end;
	float       time_left;
	int         blocked;
	numbumps = 4;
	blocked = 0;
	VectorCopy(ent->velocity, original_velocity);
	VectorCopy(ent->velocity, primal_velocity);
	numplanes = 0;
	time_left = time;
	ent->groundentity = NULL;

	for (bumpcount = 0 ; bumpcount < numbumps ; bumpcount++)
	{
		for (i = 0 ; i < 3 ; i++)
			end[i] = ent->server.state.origin[i] + time_left * ent->velocity[i];

		trace = gi.trace(ent->server.state.origin, ent->server.mins, ent->server.maxs, end, ent, mask);

		if (trace.allsolid)
		{
			// entity is trapped in another solid
			VectorClear(ent->velocity);
			return 3;
		}

		if (trace.fraction > 0)
		{
			// actually covered some distance
			VectorCopy(trace.endpos, ent->server.state.origin);
			VectorCopy(ent->velocity, original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			break;     // moved the entire distance

		hit = trace.ent;

		if (trace.plane.normal[2] > 0.7f)
		{
			blocked |= 1;       // floor

			if (hit->server.solid == SOLID_BSP)
			{
				ent->groundentity = hit;
				ent->groundentity_linkcount = hit->server.linkcount;
			}
		}

		if (!trace.plane.normal[2])
		{
			blocked |= 2;       // step
		}

		//
		// run the impact function
		//
		SV_Impact(ent, &trace);

		if (!ent->server.inuse)
			break;      // removed by the impact function

		time_left -= time_left * trace.fraction;

		// cliped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{
			// this shouldn't really happen
			VectorClear(ent->velocity);
			return 3;
		}

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		//
		// modify original_velocity so it parallels all of the clip planes
		//
		for (i = 0 ; i < numplanes ; i++)
		{
			ClipVelocity(original_velocity, planes[i], new_velocity, 1);

			for (j = 0 ; j < numplanes ; j++)
				if ((j != i) && !VectorCompare(planes[i], planes[j]))
				{
					if (DotProduct(new_velocity, planes[j]) < 0)
						break;  // not ok
				}

			if (j == numplanes)
				break;
		}

		if (i != numplanes)
		{
			// go along this plane
			VectorCopy(new_velocity, ent->velocity);
		}
		else
		{
			// go along the crease
			if (numplanes != 2)
			{
				//              Com_Printf("clip velocity, numplanes == %i\n",numplanes);
				VectorClear(ent->velocity);
				return 7;
			}

			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, ent->velocity);
			VectorScale(dir, d, ent->velocity);
		}

		//
		// if original velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		//
		if (DotProduct(ent->velocity, primal_velocity) <= 0)
		{
			VectorClear(ent->velocity);
			return blocked;
		}
	}

	return blocked;
}


/*
============
SV_AddGravity

============
*/
void SV_AddGravity(edict_t *ent)
{
	ent->velocity[2] -= ent->gravity * sv_gravity->value * game.frameseconds;
}

/*
===============================================================================

PUSHMOVE

===============================================================================
*/

/*
============
SV_PushEntity

Does not change the entities velocity at all
============
*/
trace_t SV_PushEntity(edict_t *ent, vec3_t push)
{
	trace_t trace;
	vec3_t  start;
	vec3_t  end;
	int     mask;
	VectorCopy(ent->server.state.origin, start);
	VectorAdd(start, push, end);
retry:

	if (ent->server.clipmask)
		mask = ent->server.clipmask;
	else
		mask = MASK_SOLID;

	trace = gi.trace(start, ent->server.mins, ent->server.maxs, end, ent, mask);
	VectorCopy(trace.endpos, ent->server.state.origin);
	gi.linkentity(ent);

	if (trace.fraction != 1.0f)
	{
		SV_Impact(ent, &trace);

		// if the pushed entity went away and the pusher is still there
		if (!trace.ent->server.inuse && ent->server.inuse)
		{
			// move the pusher back and try again
			VectorCopy(start, ent->server.state.origin);
			gi.linkentity(ent);
			goto retry;
		}
	}

	if (ent->server.inuse)
		G_TouchTriggers(ent);

	return trace;
}


typedef struct
{
	edict_t *ent;
	vec3_t  origin;
	vec3_t  angles;
#if USE_SMOOTH_DELTA_ANGLES
	int     deltayaw;
#endif
} pushed_t;
pushed_t    pushed[MAX_EDICTS], *pushed_p;

edict_t *obstacle;

/*
============
SV_Push

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
============
*/
bool SV_Push(edict_t *pusher, vec3_t move, vec3_t amove)
{
	int         i, e;
	edict_t     *check, *block;
	vec3_t      mins, maxs;
	pushed_t    *p;
	vec3_t      org, org2, move2, forward, right, up;

	// clamp the move to 1/8 units, so the position will
	// be accurate for client side prediction
	for (i = 0 ; i < 3 ; i++)
	{
		float   temp;
		temp = move[i] * 8.0f;

		if (temp > 0.0f)
			temp += 0.5f;
		else
			temp -= 0.5f;

		move[i] = 0.125f * (int)temp;
	}

	// find the bounding box
	for (i = 0 ; i < 3 ; i++)
	{
		mins[i] = pusher->server.absmin[i] + move[i];
		maxs[i] = pusher->server.absmax[i] + move[i];
	}

	// we need this for pushing things later
	VectorNegate(amove, org);
	AngleVectors(org, forward, right, up);
	// save the pusher's original position
	pushed_p->ent = pusher;
	VectorCopy(pusher->server.state.origin, pushed_p->origin);
	VectorCopy(pusher->server.state.angles, pushed_p->angles);
#if USE_SMOOTH_DELTA_ANGLES

	if (pusher->server.client)
		pushed_p->deltayaw = pusher->server.client->server.ps.pmove.delta_angles[YAW];

#endif
	pushed_p++;
	// move the pusher to it's final position
	VectorAdd(pusher->server.state.origin, move, pusher->server.state.origin);
	VectorAdd(pusher->server.state.angles, amove, pusher->server.state.angles);
	gi.linkentity(pusher);
	// see if any solid entities are inside the final position
	check = g_edicts + 1;

	for (e = 1; e < globals.pool.num_edicts; e++, check++)
	{
		if (!check->server.inuse)
			continue;

		if (check->movetype == MOVETYPE_PUSH
			|| check->movetype == MOVETYPE_STOP
			|| check->movetype == MOVETYPE_NONE
			|| check->movetype == MOVETYPE_NOCLIP)
			continue;

		if (!check->server.linkcount)
			continue;       // not linked in anywhere

		// if the entity is standing on the pusher, it will definitely be moved
		if (check->groundentity != pusher)
		{
			// see if the ent needs to be tested
			if (check->server.absmin[0] >= maxs[0]
				|| check->server.absmin[1] >= maxs[1]
				|| check->server.absmin[2] >= maxs[2]
				|| check->server.absmax[0] <= mins[0]
				|| check->server.absmax[1] <= mins[1]
				|| check->server.absmax[2] <= mins[2])
				continue;

			// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition(check))
				continue;
		}

		if ((pusher->movetype == MOVETYPE_PUSH) || (check->groundentity == pusher))
		{
			// move this entity
			pushed_p->ent = check;
			VectorCopy(check->server.state.origin, pushed_p->origin);
			VectorCopy(check->server.state.angles, pushed_p->angles);
#if USE_SMOOTH_DELTA_ANGLES

			if (check->server.client)
				pushed_p->deltayaw = check->server.client->server.ps.pmove.delta_angles[YAW];

#endif
			pushed_p++;
			// try moving the contacted entity
			VectorAdd(check->server.state.origin, move, check->server.state.origin);
#if USE_SMOOTH_DELTA_ANGLES

			if (check->server.client)
			{
				// FIXME: doesn't rotate monsters?
				// FIXME: skuller: needs client side interpolation
				check->server.client->server.ps.pmove.delta_angles[YAW] += ANGLE2SHORT(amove[YAW]);
			}

#endif
			// figure movement due to the pusher's amove
			VectorSubtract(check->server.state.origin, pusher->server.state.origin, org);
			org2[0] = DotProduct(org, forward);
			org2[1] = -DotProduct(org, right);
			org2[2] = DotProduct(org, up);
			VectorSubtract(org2, org, move2);
			VectorAdd(check->server.state.origin, move2, check->server.state.origin);

			// may have pushed them off an edge
			if (check->groundentity != pusher)
				check->groundentity = NULL;

			block = SV_TestEntityPosition(check);

			if (!block)
			{
				// pushed ok
				gi.linkentity(check);
				// impact?
				continue;
			}

			// if it is ok to leave in the old position, do it
			// this is only relevent for riding entities, not pushed
			// FIXME: this doesn't acount for rotation
			VectorSubtract(check->server.state.origin, move, check->server.state.origin);
			block = SV_TestEntityPosition(check);

			if (!block)
			{
				pushed_p--;
				continue;
			}
		}

		// save off the obstacle so we can call the block function
		obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for (p = pushed_p - 1 ; p >= pushed ; p--)
		{
			VectorCopy(p->origin, p->ent->server.state.origin);
			VectorCopy(p->angles, p->ent->server.state.angles);
#if USE_SMOOTH_DELTA_ANGLES

			if (p->ent->server.client)
				p->ent->server.client->server.ps.pmove.delta_angles[YAW] = p->deltayaw;

#endif
			gi.linkentity(p->ent);
		}

		return false;
	}

	//FIXME: is there a better way to handle this?
	// see if anything we moved has touched a trigger
	for (p = pushed_p - 1 ; p >= pushed ; p--)
		G_TouchTriggers(p->ent);

	return true;
}

/*
================
SV_Physics_Pusher

Bmodel objects don't interact with each other, but
push all box objects
================
*/
void SV_Physics_Pusher(edict_t *ent)
{
	vec3_t      move, amove;
	edict_t     *part, *mv;

	// if not a team captain, so movement will be handled elsewhere
	if (ent->flags & FL_TEAMSLAVE)
		return;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	//retry:
	pushed_p = pushed;

	for (part = ent ; part ; part = part->teamchain)
	{
		if (!VectorEmpty(part->velocity) || !VectorEmpty(part->avelocity))
		{
			// object is moving
			VectorScale(part->velocity, game.frameseconds, move);
			VectorScale(part->avelocity, game.frameseconds, amove);

			if (!SV_Push(part, move, amove))
				break;  // move was blocked
		}
	}

	if (pushed_p > &pushed[MAX_EDICTS])
		Com_Error(ERR_FATAL, "pushed_p > &pushed[MAX_EDICTS], memory corrupted");

	if (part)
	{
		// the move failed, bump all nextthink times and back out moves
		for (mv = ent ; mv ; mv = mv->teamchain)
		{
			if (mv->nextthink > 0)
				mv->nextthink += game.frametime;
		}

		// if the pusher has a "blocked" function, call it
		// otherwise, just stay in place until the obstacle is gone
		if (part->blocked)
			part->blocked(part, obstacle);

#if 0

		// if the pushed entity went away and the pusher is still there
		if (!obstacle->server.inuse && part->server.inuse)
			goto retry;

#endif
	}
	else
	{
		// the move succeeded, so call all think functions
		for (part = ent ; part ; part = part->teamchain)
			SV_RunThink(part);
	}
}

//==================================================================

/*
=============
SV_Physics_None

Non moving objects can only think
=============
*/
void SV_Physics_None(edict_t *ent)
{
	// regular thinking
	SV_RunThink(ent);
}

/*
=============
SV_Physics_Noclip

A moving object that doesn't obey physics
=============
*/
void SV_Physics_Noclip(edict_t *ent)
{
	// regular thinking
	if (!SV_RunThink(ent))
		return;

	if (!ent->server.inuse)
		return;

	VectorMA(ent->server.state.angles, game.frameseconds, ent->avelocity, ent->server.state.angles);
	VectorMA(ent->server.state.origin, game.frameseconds, ent->velocity, ent->server.state.origin);
	gi.linkentity(ent);
}

/*
==============================================================================

TOSS / BOUNCE

==============================================================================
*/

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
=============
*/
void SV_Physics_Toss(edict_t *ent)
{
	trace_t     trace;
	vec3_t      move;
	float       backoff;
	edict_t     *slave;
	bool        wasinwater;
	bool        isinwater;
	vec3_t      old_origin;
	// regular thinking
	SV_RunThink(ent);

	if (!ent->server.inuse)
		return;

	// if not a team captain, so movement will be handled elsewhere
	if (ent->flags & FL_TEAMSLAVE)
		return;

	if (ent->velocity[2] > 0)
		ent->groundentity = NULL;

	// check for the groundentity going away
	if (ent->groundentity)
		if (!ent->groundentity->server.inuse)
			ent->groundentity = NULL;

	// if onground, return without moving
	if (ent->groundentity)
		return;

	VectorCopy(ent->server.state.origin, old_origin);
	SV_CheckVelocity(ent);

	// add gravity
	if (ent->movetype != MOVETYPE_FLY
		&& ent->movetype != MOVETYPE_FLYMISSILE
		&& ent->movetype != MOVETYPE_FLYBOUNCE)
		SV_AddGravity(ent);

	// move angles
	VectorMA(ent->server.state.angles, game.frameseconds, ent->avelocity, ent->server.state.angles);
	// move origin
	VectorScale(ent->velocity, game.frameseconds, move);
	trace = SV_PushEntity(ent, move);

	if (!ent->server.inuse)
		return;

	if (trace.fraction < 1)
	{
		if (ent->movetype == MOVETYPE_FLYBOUNCE)
			backoff = 2.0f;
		else if (ent->movetype == MOVETYPE_BOUNCE)
			backoff = 1.5f;
		else
			backoff = 1;

		ClipVelocity(ent->velocity, trace.plane.normal, ent->velocity, backoff);

		// stop if on ground
		if (trace.plane.normal[2] > 0.7f && ent->movetype != MOVETYPE_FLYBOUNCE)
		{
			if (ent->velocity[2] < 60 || ent->movetype != MOVETYPE_BOUNCE)
			{
				ent->groundentity = trace.ent;
				ent->groundentity_linkcount = trace.ent->server.linkcount;
				VectorClear(ent->velocity);
				VectorClear(ent->avelocity);
			}
		}

		//      if (ent->touch)
		//          ent->touch (ent, trace.ent, &trace.plane, trace.surface);
	}

	// check for water transition
	wasinwater = (ent->watertype & MASK_WATER);
	ent->watertype = gi.pointcontents(ent->server.state.origin);
	isinwater = ent->watertype & MASK_WATER;

	if (isinwater)
		ent->waterlevel = 1;
	else
		ent->waterlevel = 0;

	if (!ent->watercheck)
		ent->watercheck = true;
	else
	{
		q_soundhandle water_sound = gi.soundindex("misc/h2ohit1.wav");

		if (ent->server.state.game == GAME_Q1)
			water_sound = gi.soundindex("q1/misc/h2ohit1.wav");

		if (!wasinwater && isinwater)
			gi.positioned_sound(old_origin, g_edicts, CHAN_AUTO, water_sound, 1, ATTN_NORM, 0);
		else if (wasinwater && !isinwater)
			gi.positioned_sound(ent->server.state.origin, g_edicts, CHAN_AUTO, water_sound, 1, ATTN_NORM, 0);
	}

	// move teamslaves
	for (slave = ent->teamchain; slave; slave = slave->teamchain)
	{
		VectorCopy(ent->server.state.origin, slave->server.state.origin);
		gi.linkentity(slave);
	}
}

#if ENABLE_COOP
/*
===============================================================================

STEPPING MOVEMENT

===============================================================================
*/

/*
=============
SV_Physics_Step

Monsters freefall when they don't have a ground entity, otherwise
all movement is done with discrete steps.

This is also used for objects that have become still on the ground, but
will fall if the floor is pulled out from under them.
FIXME: is this true?
=============
*/

//FIXME: hacked in for E3 demo
#define sv_stopspeed        100
#define sv_friction         6
#define sv_waterfriction    1

void SV_AddRotationalFriction(edict_t *ent)
{
	int     n;
	float   adjustment;
	VectorMA(ent->server.state.angles, game.frameseconds, ent->avelocity, ent->server.state.angles);
	adjustment = game.frameseconds * sv_stopspeed * sv_friction;

	for (n = 0; n < 3; n++)
	{
		if (ent->avelocity[n] > 0)
		{
			ent->avelocity[n] -= adjustment;

			if (ent->avelocity[n] < 0)
				ent->avelocity[n] = 0;
		}
		else
		{
			ent->avelocity[n] += adjustment;

			if (ent->avelocity[n] > 0)
				ent->avelocity[n] = 0;
		}
	}
}

void SV_Physics_Step(edict_t *ent)
{
	bool        wasonground;
	bool        hitsound = false;
	float       *vel;
	float       speed, newspeed, control;
	float       friction;
	edict_t     *groundentity;
	int         mask;

	// airborn monsters should always check for ground
	if (!ent->groundentity)
		M_CheckGround(ent);

	groundentity = ent->groundentity;
	SV_CheckVelocity(ent);

	if (groundentity)
		wasonground = true;
	else
		wasonground = false;

	if (!VectorEmpty(ent->avelocity))
		SV_AddRotationalFriction(ent);

	// add gravity except:
	//   flying monsters
	//   swimming monsters who are in the water
	if (! wasonground)
		if (!(ent->flags & FL_FLY))
			if (!((ent->flags & FL_SWIM) && (ent->waterlevel > 2)))
			{
				if (ent->velocity[2] < sv_gravity->value * -0.1f)
					hitsound = true;

				if (ent->waterlevel == 0)
					SV_AddGravity(ent);
			}

	// friction for flying monsters that have been given vertical velocity
	if (ent->server.state.game != GAME_DOOM)
	{
		if ((ent->flags & FL_FLY) && (ent->velocity[2] != 0))
		{
			speed = fabsf(ent->velocity[2]);
			control = speed < sv_stopspeed ? sv_stopspeed : speed;
			friction = sv_friction / 3;
			newspeed = speed - (game.frameseconds * control * friction);

			if (newspeed < 0)
				newspeed = 0;

			newspeed /= speed;
			ent->velocity[2] *= newspeed;
		}

		// friction for flying monsters that have been given vertical velocity
		if ((ent->flags & FL_SWIM) && (ent->velocity[2] != 0))
		{
			speed = fabsf(ent->velocity[2]);
			control = speed < sv_stopspeed ? sv_stopspeed : speed;
			newspeed = speed - (game.frameseconds * control * sv_waterfriction * ent->waterlevel);

			if (newspeed < 0)
				newspeed = 0;

			newspeed /= speed;
			ent->velocity[2] *= newspeed;
		}
	}

	if (ent->velocity[2] || ent->velocity[1] || ent->velocity[0])
	{
		// apply friction
		// let dead monsters who aren't completely onground slide
		if ((wasonground) || (ent->flags & (FL_SWIM | FL_FLY)))
		{
			if (ent->server.state.game == GAME_DOOM)
			{
				if (!((ent->flags & FL_FLY) && ent->health) && M_CheckBottom(ent))
				{
					vel = ent->velocity;

					if (ent->flags & FL_FLY)
						speed = sqrtf(vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2]);
					else
						speed = sqrtf(vel[0] * vel[0] + vel[1] * vel[1]);

					if (speed > 0.1f)
					{
						vel = ent->velocity;
						friction = (ent->health <= 0) ? 2.0f : 1.2f;

						for (int i = 0; i < ((ent->flags & FL_FLY) ? 3 : 2); ++i)
							vel[i] /= friction;
					}
				}
			}
			else
			{
				if (!(ent->health <= 0.0f && !M_CheckBottom(ent)))
				{
					vel = ent->velocity;
					speed = sqrtf(vel[0] * vel[0] + vel[1] * vel[1]);

					if (speed)
					{
						friction = sv_friction;
						control = speed < sv_stopspeed ? sv_stopspeed : speed;
						newspeed = speed - game.frameseconds * control * friction;

						if (newspeed < 0)
							newspeed = 0;

						newspeed /= speed;
						vel[0] *= newspeed;
						vel[1] *= newspeed;
					}
				}
			}
		}

		if (ent->server.flags.monster)
			mask = MASK_MONSTERSOLID;
		else
			mask = MASK_SOLID;

		SV_FlyMove(ent, game.frameseconds, mask);

		if (ent->server.state.game == GAME_DOOM && !ent->groundentity)
			M_CheckGround(ent);

		gi.linkentity(ent);
		G_TouchTriggers(ent);

		if (!ent->server.inuse)
			return;

		if (ent->groundentity && ent->server.state.game != GAME_DOOM)
			if (!wasonground)
				if (hitsound)
					gi.sound(ent, CHAN_AUTO, ent->server.state.game == GAME_Q1 ? gi.soundindex("q1/demon/dland2.wav") : gi.soundindex("world/land.wav"), 1, ATTN_NORM, 0);
	}

	// regular thinking
	SV_RunThink(ent);
}
#endif

//============================================================================
/*
================
G_RunEntity

================
*/
void G_RunEntity(edict_t *ent)
{
	if (ent->prethink)
		ent->prethink(ent);

	switch ((int)ent->movetype)
	{
		//JABot[start]
		case MOVETYPE_WALK:
			SV_RunThink(ent);
			break;

		//[end]
		case MOVETYPE_PUSH:
		case MOVETYPE_STOP:
			SV_Physics_Pusher(ent);
			break;

		case MOVETYPE_NONE:
			SV_Physics_None(ent);
			break;

		case MOVETYPE_NOCLIP:
			SV_Physics_Noclip(ent);
			break;
			
#if ENABLE_COOP
		case MOVETYPE_STEP:
			SV_Physics_Step(ent);
			break;
#endif

		case MOVETYPE_TOSS:
		case MOVETYPE_BOUNCE:
		case MOVETYPE_FLY:
		case MOVETYPE_FLYMISSILE:
		case MOVETYPE_FLYBOUNCE:
			SV_Physics_Toss(ent);
			break;

		default:
			Com_Error(ERR_FATAL, "SV_Physics: bad movetype %i", (int)ent->movetype);
	}
}

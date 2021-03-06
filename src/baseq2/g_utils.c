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
// g_utils.c -- misc utility functions for game module

#include "g_local.h"


void G_ProjectSource(const vec3_t point, const vec3_t distance, const vec3_t forward, const vec3_t right, vec3_t result)
{
	result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1];
	result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1];
	result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + distance[2];
}


/*
=============
G_Find

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the edict after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
edict_t *G_FindByType(edict_t *from, entitytype_e type)
{
	if (!from)
		from = g_edicts;
	else
		from++;

	for (; from < &g_edicts[globals.pool.num_edicts]; from++)
	{
		if (!from->server.inuse)
			continue;

		if (from->entitytype == type)
			return from;
	}

	return NULL;
}

edict_t *G_Find(edict_t *from, int fieldofs, char *match)
{
	char    *s;

	if (!from)
		from = g_edicts;
	else
		from++;

	for (; from < &g_edicts[globals.pool.num_edicts] ; from++)
	{
		if (!from->server.inuse)
			continue;

		s = *(char **)((byte *)from + fieldofs);

		if (!s)
			continue;

		if (!Q_stricmp(s, match))
			return from;
	}

	return NULL;
}


/*
=================
findradius

Returns entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
edict_t *findradius(edict_t *from, vec3_t org, float rad)
{
	vec3_t  eorg;
	int     j;

	if (!from)
		from = g_edicts;
	else
		from++;

	for (; from < &g_edicts[globals.pool.num_edicts]; from++)
	{
		if (!from->server.inuse)
			continue;

		//if (from->server.solid == SOLID_NOT)
		//	continue;

		for (j = 0 ; j < 3 ; j++)
			eorg[j] = org[j] - (from->server.state.origin[j] + (from->server.mins[j] + from->server.maxs[j]) * 0.5f);

		if (VectorLength(eorg) > rad)
			continue;

		return from;
	}

	return NULL;
}


/*
=============
G_PickTarget

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the edict after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
#define MAXCHOICES  8

edict_t *G_PickTarget(char *targetname)
{
	edict_t *ent = NULL;
	int     num_choices = 0;
	edict_t *choice[MAXCHOICES];

	if (!targetname)
	{
		Com_Printf("G_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while (1)
	{
		ent = G_Find(ent, FOFS(targetname), targetname);

		if (!ent)
			break;

		choice[num_choices++] = ent;

		if (num_choices == MAXCHOICES)
			break;
	}

	if (!num_choices)
	{
		Com_Printf("G_PickTarget: target %s not found\n", targetname);
		return NULL;
	}

	return choice[Q_rand_uniform(num_choices)];
}



void Think_Delay(edict_t *ent)
{
	G_UseTargets(ent, ent->activator);
	G_FreeEdict(ent);
}

/*
==============================
G_UseTargets

the global "activator" should be set to the entity that initiated the firing.

If self.delay is set, a DelayedUse entity will be created that will actually
do the SUB_UseTargets after that many seconds have passed.

Centerprints any self.message to the activator.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void G_UseTargets(edict_t *ent, edict_t *activator)
{
	edict_t     *t;

	//
	// check for a delay
	//
	if (ent->delay)
	{
		// create a temp object to fire at a later time
		t = G_Spawn();
		t->nextthink = level.time + (ent->delay * 1000);
		t->think = Think_Delay;
		t->activator = activator;

		if (!activator)
			Com_Printf("Think_Delay with no activator\n");

		t->message = ent->message;
		t->target = ent->target;
		t->killtarget = ent->killtarget;
		return;
	}

	//
	// print the message
	//
	if (ent->message && !activator->server.flags.monster)
	{
		gi.centerprintf(activator, "%s", ent->message);

		if (ent->noise_index)
			gi.sound(activator, CHAN_AUTO, ent->noise_index, 1, ATTN_NORM, 0);
		else
			gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}

	//
	// kill killtargets
	//
	if (ent->killtarget)
	{
		t = NULL;

		while ((t = G_Find(t, FOFS(targetname), ent->killtarget)))
		{
			G_FreeEdict(t);

			if (!ent->server.inuse)
			{
				Com_Printf("entity was removed while using killtargets\n");
				return;
			}
		}
	}

	//
	// fire targets
	//
	if (ent->target)
	{
		t = NULL;

		while ((t = G_Find(t, FOFS(targetname), ent->target)))
		{
			// doors fire area portals in a specific way
			if (t->entitytype == ET_FUNC_AREAPORTAL &&
				(ent->entitytype == ET_FUNC_DOOR || ent->entitytype == ET_FUNC_DOOR_ROTATING))
				continue;

			if (t == ent)
				Com_Printf("WARNING: Entity used itself.\n");
			else
			{
				if (t->use)
					t->use(t, ent, activator);
			}

			if (!ent->server.inuse)
			{
				Com_Printf("entity was removed while using targets\n");
				return;
			}
		}
	}
}


/*
=============
TempVector

This is just a convenience function
for making temporary vectors for function calls
=============
*/
float   *tv(float x, float y, float z)
{
	static  int     index;
	static  vec3_t  vecs[8];
	float   *v;
	// use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = (index + 1) & 7;
	v[0] = x;
	v[1] = y;
	v[2] = z;
	return v;
}


/*
=============
VectorToString

This is just a convenience function
for printing vectors
=============
*/
char    *vtos(vec3_t v)
{
	static  int     index;
	static  char    str[8][32];
	char    *s;
	// use an array so that multiple vtos won't collide
	s = str[index];
	index = (index + 1) & 7;
	Q_snprintf(s, 32, "(%i %i %i)", (int)v[0], (int)v[1], (int)v[2]);
	return s;
}


vec3_t VEC_UP       = {0, -1, 0};
vec3_t MOVEDIR_UP   = {0, 0, 1};
vec3_t VEC_DOWN     = {0, -2, 0};
vec3_t MOVEDIR_DOWN = {0, 0, -1};

void G_SetMovedir(vec3_t angles, vec3_t movedir)
{
	if (VectorCompare(angles, VEC_UP))
		VectorCopy(MOVEDIR_UP, movedir);
	else if (VectorCompare(angles, VEC_DOWN))
		VectorCopy(MOVEDIR_DOWN, movedir);
	else
		AngleVectors(angles, movedir, NULL, NULL);

	VectorClear(angles);
}


float vectoyaw(vec3_t vec)
{
	float   yaw;

	if (/*vec[YAW] == 0 &&*/ vec[PITCH] == 0)
	{
		yaw = 0;

		if (vec[YAW] > 0)
			yaw = 90;
		else if (vec[YAW] < 0)
			yaw = -90;
	}
	else
	{
		yaw = (int)RAD2DEG(atan2f(vec[YAW], vec[PITCH]));

		if (yaw < 0)
			yaw += 360;
	}

	return yaw;
}


void vectoangles(vec3_t value1, vec3_t angles)
{
	float   forward;
	float   yaw, pitch;

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;

		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		if (value1[0])
			yaw = (int)RAD2DEG(atan2f(value1[1], value1[0]));
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = -90;

		if (yaw < 0)
			yaw += 360;

		forward = sqrtf(value1[0] * value1[0] + value1[1] * value1[1]);
		pitch = (int)RAD2DEG(atan2f(value1[2], forward));

		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

/*
======
vectoangles2 - this is duplicated in the game DLL, but I need it here.
======
*/
void vectoangles2(const vec3_t value1, vec3_t angles)
{
	float   forward;
	float   yaw, pitch;

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;

		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		if (value1[0])
			yaw = atan2f(value1[1], value1[0]) * 180 / M_PI;
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = 270;

		if (yaw < 0)
			yaw += 360;

		forward = sqrtf(value1[0] * value1[0] + value1[1] * value1[1]);
		pitch = atan2f(value1[2], forward) * 180 / M_PI;

		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

char *G_CopyString(char *in)
{
	char    *out;
	out = Z_TagMalloc(strlen(in) + 1, TAG_LEVEL);
	strcpy(out, in);
	return out;
}

void G_InitEdict(edict_t *e)
{
	e->server.inuse = true;
	e->entitytype = ET_NULL;
	e->gravity = 1.0f;
	e->server.state.number = e - g_edicts;
	e->server.state.clip_contents = CONTENTS_MONSTER;

	if (e->server.state.number >= 1 && e->server.state.number <= maxclients->integer)
		e->server.client = &game.clients[e->server.state.number - 1];
}

/*
=================
G_Spawn

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *G_Spawn(void)
{
	int         i;
	edict_t     *e;
	e = &g_edicts[game.maxclients + 1];

	for (i = game.maxclients + 1 ; i < globals.pool.num_edicts ; i++, e++)
	{
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e->server.inuse && (e->freetime < 2000 || level.time - e->freetime > 500))
		{
			G_InitEdict(e);
			return e;
		}
	}

	if (i == game.maxentities)
		Com_Error(ERR_FATAL, "ED_Alloc: no free edicts");

	globals.pool.num_edicts++;
	G_InitEdict(e);
	return e;
}

/*
=================
G_FreeEdict

Marks the edict as free
=================
*/
const char *freed_classname = "freed";

void G_FreeEdict(edict_t *ed)
{
	gi.unlinkentity(ed);        // unlink from world

	if ((ed - g_edicts) <= (maxclients->value + BODY_QUEUE_SIZE))
	{
		//      Com_Printf("tried to free special edict\n");
		return;
	}
	
#ifdef ENABLE_COOP
	if (ed->monsterinfo.wave_entry.next)
		Wave_MonsterFreed(ed);
#endif

	G_FreeAI(ed); //jabot092(2)
	memset(ed, 0, sizeof(*ed));
	ed->freetime = level.time;
	ed->server.inuse = false;
}


/*
============
G_TouchTriggers

============
*/
void    G_TouchTriggers(edict_t *ent)
{
	int         i, num;
	edict_t     *touch[MAX_EDICTS], *hit;

	// dead things don't activate triggers!
	if ((ent->server.client || ent->server.flags.monster) && (ent->health <= 0))
		return;

	num = gi.BoxEdicts(ent->server.absmin, ent->server.absmax, touch
			, MAX_EDICTS, AREA_TRIGGERS);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i = 0 ; i < num ; i++)
	{
		hit = touch[i];

		if (!hit->server.inuse)
			continue;

		if (!hit->touch)
			continue;

		hit->touch(hit, ent, NULL, NULL);
	}
}

/*
============
G_TouchSolids

Call after linking a new trigger in during gameplay
to force all entities it covers to immediately touch it
============
*/
void    G_TouchSolids(edict_t *ent)
{
	int         i, num;
	edict_t     *touch[MAX_EDICTS], *hit;
	num = gi.BoxEdicts(ent->server.absmin, ent->server.absmax, touch
			, MAX_EDICTS, AREA_SOLID);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i = 0 ; i < num ; i++)
	{
		hit = touch[i];

		if (!hit->server.inuse)
			continue;

		if (ent->touch)
			ent->touch(hit, ent, NULL, NULL);

		if (!ent->server.inuse)
			break;
	}
}




/*
==============================================================================

Kill box

==============================================================================
*/

/*
=================
KillBox

Kills all entities that would touch the proposed new positioning
of ent.  Ent should be unlinked before calling this!
=================
*/

bool G_KillBox(edict_t *ent, vec3_t origin, vec3_t mins, vec3_t maxs)
{
	trace_t     tr;

	while (1)
	{
		tr = gi.trace(origin, mins, maxs, origin, NULL, MASK_PLAYERSOLID);

		if (!tr.ent)
			break;

		// nail it
		T_Damage(tr.ent, ent, ent, vec3_origin, origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MakeAttackerMeansOfDeath(world, ent, MD_TELEFRAG, DT_DIRECT));

		// if we didn't kill it, fail
		if (tr.ent->server.solid)
			return false;
	}

	return true;        // all clear
}

bool KillBox(edict_t *ent)
{
	return G_KillBox(ent, ent->server.state.origin, ent->server.mins, ent->server.maxs);
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
	VectorCopy(self->server.state.origin, spot1);
	spot1[2] += self->viewheight;
	VectorCopy(other->server.state.origin, spot2);
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
	AngleVectors(self->server.state.angles, forward, NULL, NULL);
	VectorSubtract(other->server.state.origin, self->server.state.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);

	if (dot > 0.3f)
		return true;

	return false;
}
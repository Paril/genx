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
// g_misc.c

#include "g_local.h"


/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.
*/

//=====================================================

void Use_Areaportal(edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->count ^= 1;        // toggle state
	//  Com_Printf("portalstate: %i = %i\n", ent->style, ent->count);
	gi.SetAreaPortalState(ent->style, ent->count);
}

/*QUAKED func_areaportal (0 0 0) ?

This is a non-visible object that divides the world into
areas that are seperated when this portal is not activated.
Usually enclosed in the middle of a door.
*/
void SP_func_areaportal(edict_t *ent)
{
	ent->use = Use_Areaportal;
	ent->count = 0;     // always start closed;
}

//=====================================================


/*
=================
Misc functions
=================
*/
void VelocityForDamage(int damage, vec3_t v)
{
	v[0] = 100.0f * crandom();
	v[1] = 100.0f * crandom();
	v[2] = 200.0f + 100.0f * random();

	if (damage < 50)
		VectorScale(v, 0.7f, v);
	else
		VectorScale(v, 1.2f, v);
}

void ClipGibVelocity(edict_t *ent)
{
	if (ent->velocity[0] < -300)
		ent->velocity[0] = -300;
	else if (ent->velocity[0] > 300)
		ent->velocity[0] = 300;

	if (ent->velocity[1] < -300)
		ent->velocity[1] = -300;
	else if (ent->velocity[1] > 300)
		ent->velocity[1] = 300;

	if (ent->velocity[2] < 200)
		ent->velocity[2] = 200; // always some upwards
	else if (ent->velocity[2] > 500)
		ent->velocity[2] = 500;
}


/*
=================
gibs
=================
*/
void gib_think(edict_t *self)
{
	self->server.state.frame++;
	self->nextthink = level.time + game.frametime;

	if (self->server.state.frame == 10)
	{
		self->think = G_FreeEdict;
		self->nextthink = level.time + 8000 + (Q_rand() % 10000);
	}
}

void gib_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t  normal_angles, right;

	if (!self->groundentity)
		return;

	self->touch = NULL;

	if (plane)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);
		vectoangles(plane->normal, normal_angles);
		AngleVectors(normal_angles, NULL, right, NULL);
		vectoangles(right, self->server.state.angles);

		if (self->server.state.modelindex == sm_meat_index)
		{
			self->server.state.frame++;
			self->think = gib_think;
			self->nextthink = level.time + 1;
		}
	}
}

void VelocityForDamage_Q1(float dm, vec3_t v)
{
	v[0] = 100 * crandom();
	v[1] = 100 * crandom();
	v[2] = 200 + 100 * random();

	if (dm > -50)
	{
		//		dprint ("level 1\n");
		VectorScale(v, 0.7, v);
	}
	else if (dm > -200)
	{
		//		dprint ("level 3\n");
		VectorScale(v, 2, v);
	}
	else
		VectorScale(v, 10, v);
}

void duke_gib_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (self == other)
		return;

	if (plane && plane->normal[2] > 0.7f)
	{
		VectorMA(self->server.state.origin, 1, plane->normal, self->server.state.origin);
		MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(TE_DUKE_BLOOD_SPLAT);
		MSG_WritePos(self->server.state.origin);
		gi.multicast(self->server.state.origin, MULTICAST_PVS);
		G_FreeEdict(self);
	}
}

void gib_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	G_FreeEdict(self);
}

void gib_duke_think(edict_t *gib)
{
	gib->server.state.frame = (gib->server.state.frame + 1) % (gib->count == GIB_DUKE_LG ? 8 : gib->count == GIB_DUKE_SM ? 2 : 4);
	gib->nextthink = level.time + 100;
}

void ThrowGib(edict_t *self, char *gibname, int damage, gibtype_e type)
{
	edict_t *gib;
	vec3_t  vd;
	vec3_t  origin;
	vec3_t  size;
	float   vscale;
	gib = G_Spawn();
	VectorScale(self->server.size, 0.5f, size);
	VectorAdd(self->server.absmin, size, origin);
	gib->server.state.origin[0] = origin[0] + crandom() * size[0];
	gib->server.state.origin[1] = origin[1] + crandom() * size[1];
	gib->server.state.origin[2] = origin[2] + crandom() * size[2];
	gi.setmodel(gib, gibname);
	gib->server.solid = SOLID_NOT;
	gib->server.state.effects |= EF_GIB;
	gib->flags |= FL_NO_KNOCKBACK;

	if (type == GIB_Q1)
	{
		VelocityForDamage_Q1(self->health, gib->velocity);
		gib->movetype = MOVETYPE_BOUNCE;
		gib->server.state.game = GAME_Q1;
	}
	else if (type == GIB_DUKE || type == GIB_DUKE_SM || type == GIB_DUKE_LG)
	{
		gib->movetype = MOVETYPE_BOUNCE;
		gib->server.state.game = GAME_DUKE;
		gib->server.state.effects = 0;
		gib->velocity[0] = 200 * crandom();
		gib->velocity[1] = 200 * crandom();
		gib->velocity[2] = 300 + 200 * random();
		gib->touch = duke_gib_touch;
		gib->server.solid = SOLID_TRIGGER;
		gib->server.owner = gib;
		gib->server.state.frame = Q_rand() % 8;
		gib_duke_think(gib);
	}
	else
	{
		if (type == GIB_ORGANIC)
		{
			gib->movetype = MOVETYPE_TOSS;
			gib->touch = gib_touch;
			vscale = 0.5f;
		}
		else
		{
			gib->movetype = MOVETYPE_BOUNCE;
			vscale = 1.0f;
		}

		VelocityForDamage(damage, vd);
		VectorMA(self->velocity, vscale, vd, gib->velocity);
		ClipGibVelocity(gib);
	}

	if (type == GIB_DUKE || type == GIB_DUKE_SM || type == GIB_DUKE_LG)
	{
		gib->count = type;
		gib->think = gib_duke_think;
		gib->nextthink = level.time + 100;
	}
	else
	{
		gib->avelocity[0] = random() * 600;
		gib->avelocity[1] = random() * 600;
		gib->avelocity[2] = random() * 600;
		gib->think = G_FreeEdict;
		gib->nextthink = level.time + 10000 + (Q_rand() % 10000);
	}

	gi.linkentity(gib);
}

void ThrowHead(edict_t *self, char *gibname, int damage, gibtype_e type)
{
	vec3_t  vd;
	float   vscale;
	self->server.state.skinnum = 0;
	self->server.state.frame = 0;
	VectorClear(self->server.mins);
	VectorClear(self->server.maxs);
	self->server.state.modelindex2 = 0;
	gi.setmodel(self, gibname);
	self->server.solid = SOLID_NOT;
	self->server.state.effects |= EF_GIB;
	self->server.state.effects &= ~EF_FLIES;
	self->server.state.renderfx &= ~(RF_NOLERP | RF_FRAMELERP);
	self->server.state.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;
	self->server.flags.monster = false;
	self->takedamage = true;
	self->die = gib_die;

	if (type == GIB_ORGANIC)
	{
		self->movetype = MOVETYPE_TOSS;
		self->touch = gib_touch;
		vscale = 0.5f;
	}
	else
	{
		self->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0f;
	}

	if (type == GIB_Q1)
	{
		VelocityForDamage_Q1(self->health, self->velocity);
		VectorClear(self->avelocity);
		self->avelocity[1] = 600;
		self->server.state.game = GAME_Q1;
	}
	else
	{
		VelocityForDamage(damage, vd);
		VectorMA(self->velocity, vscale, vd, self->velocity);
		ClipGibVelocity(self);
	}

	self->avelocity[YAW] = crandom() * 600;
	self->think = G_FreeEdict;
	self->nextthink = level.time + 10000 + (Q_rand() % 10000);
	gi.linkentity(self);
}

void ThrowClientHead(edict_t *self, int damage)
{
	vec3_t  vd;
	char    *gibname;
	self->takedamage = false;

	if (self->server.state.game == GAME_Q2)
	{
		if (Q_rand() & 1)
		{
			gibname = "models/objects/gibs/head2/tris.md2";
			self->server.state.skinnum = 1;        // second skin is player
		}
		else
		{
			gibname = "models/objects/gibs/skull/tris.md2";
			self->server.state.skinnum = 0;
		}

		VelocityForDamage(damage, vd);
		VectorAdd(self->velocity, vd, self->velocity);
	}
	else if (self->server.state.game == GAME_Q1)
	{
		gibname = "models/q1/h_player.mdl";
		VelocityForDamage_Q1(self->health, self->velocity);
		VectorClear(self->avelocity);
		self->avelocity[1] = 600;
	}
	else
	{
		self->server.clipmask = MASK_PLAYERSOLID;
		self->server.solid = SOLID_TRIGGER;
		self->server.state.modelindex = 0;
		gi.linkentity(self);
		return;
	}

	self->server.state.origin[2] += 32;
	self->server.state.frame = 0;
	gi.setmodel(self, gibname);
	VectorSet(self->server.mins, -16, -16, 0);
	VectorSet(self->server.maxs, 16, 16, 16);
	self->server.solid = SOLID_NOT;
	self->server.state.effects = EF_GIB;
	self->server.state.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;
	self->movetype = MOVETYPE_BOUNCE;

	if (self->server.client)   // bodies in the queue don't have a client anymore
	{
		self->server.client->anim_priority = ANIM_DEATH;
		self->server.client->anim_end = self->server.state.frame;
	}
	else
	{
		self->think = NULL;
		self->nextthink = 0;
	}

	gi.linkentity(self);
}


/*
=================
debris
=================
*/
void debris_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	G_FreeEdict(self);
}

void ThrowDebris(edict_t *self, char *modelname, float speed, vec3_t origin)
{
	edict_t *chunk;
	vec3_t  v;
	chunk = G_Spawn();
	VectorCopy(origin, chunk->server.state.origin);
	gi.setmodel(chunk, modelname);
	v[0] = 100 * crandom();
	v[1] = 100 * crandom();
	v[2] = 100 + 100 * crandom();
	VectorMA(self->velocity, speed, v, chunk->velocity);
	chunk->movetype = MOVETYPE_BOUNCE;
	chunk->server.solid = SOLID_NOT;
	chunk->avelocity[0] = random() * 600;
	chunk->avelocity[1] = random() * 600;
	chunk->avelocity[2] = random() * 600;
	chunk->think = G_FreeEdict;
	chunk->nextthink = level.time + 5000 + (Q_rand() % 5000);
	chunk->server.state.frame = 0;
	chunk->flags = 0;
	chunk->takedamage = true;
	chunk->die = debris_die;
	gi.linkentity(chunk);
}


void BecomeExplosion1(edict_t *self)
{
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_EXPLOSION1);
	MSG_WritePos(self->server.state.origin);
	gi.multicast(self->server.state.origin, MULTICAST_PVS);
	G_FreeEdict(self);
}


void BecomeExplosion2(edict_t *self)
{
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_EXPLOSION2);
	MSG_WritePos(self->server.state.origin);
	gi.multicast(self->server.state.origin, MULTICAST_PVS);
	G_FreeEdict(self);
}


/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) TELEPORT
Target: next path corner
Pathtarget: gets used when an entity that has
	this path_corner targeted touches it
*/

void ogre_drag_sound(edict_t *self);

void path_corner_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      v;
	edict_t     *next;

	if (other->movetarget != self)
		return;

	if (other->enemy)
		return;

	if (self->pathtarget)
	{
		char *savetarget;
		savetarget = self->target;
		self->target = self->pathtarget;
		G_UseTargets(self, other);
		self->target = savetarget;
	}

	if (self->target)
		next = G_PickTarget(self->target);
	else
		next = NULL;

	if ((next) && (next->spawnflags & 1))
	{
		VectorCopy(next->server.state.origin, v);
		v[2] += next->server.mins[2];
		v[2] -= other->server.mins[2];
		VectorCopy(v, other->server.state.origin);
		next = G_PickTarget(next->target);
		other->server.state.event = EV_OTHER_TELEPORT;
	}

	other->goalentity = other->movetarget = next;
	
#ifdef ENABLE_COOP
	if (other->entitytype == ET_Q1_MONSTER_OGRE)
		ogre_drag_sound(other);// play chainsaw drag sound

	if (self->wait)
	{
		other->monsterinfo.pausetime = level.time + (self->wait * 1000);
		other->monsterinfo.stand(other);
		return;
	}

	if (!other->movetarget)
	{
		other->monsterinfo.pausetime = GTIME_MAX;
		other->monsterinfo.stand(other);
	}
	else
	{
		VectorSubtract(other->goalentity->server.state.origin, other->server.state.origin, v);
		other->ideal_yaw = vectoyaw(v);
	}
#endif
}

void SP_path_corner(edict_t *self)
{
	if (!self->targetname)
	{
		Com_Printf("path_corner with no targetname at %s\n", vtos(self->server.state.origin));
		G_FreeEdict(self);
		return;
	}

	self->server.solid = SOLID_TRIGGER;
	self->touch = path_corner_touch;
	VectorSet(self->server.mins, -8, -8, -8);
	VectorSet(self->server.maxs, 8, 8, 8);
	self->server.flags.noclient = true;
	gi.linkentity(self);
}

#ifdef ENABLE_COOP
/*QUAKED point_combat (0.5 0.3 0) (-8 -8 -8) (8 8 8) Hold
Makes this the target of a monster and it will head here
when first activated before going after the activator.  If
hold is selected, it will stay here.
*/
void point_combat_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t *activator;

	if (other->movetarget != self)
		return;

	if (self->target)
	{
		other->target = self->target;
		other->goalentity = other->movetarget = G_PickTarget(other->target);

		if (!other->goalentity)
		{
			Com_Printf("entityid %i at %s target %s does not exist\n", self->entitytype, vtos(self->server.state.origin), self->target);
			other->movetarget = self;
		}

		self->target = NULL;
	}
	else if ((self->spawnflags & 1) && !(other->flags & (FL_SWIM | FL_FLY)))
	{
		other->monsterinfo.pausetime = GTIME_MAX;
		other->monsterinfo.aiflags |= AI_STAND_GROUND;
		other->monsterinfo.stand(other);
	}

	if (other->movetarget == self)
	{
		other->target = NULL;
		other->movetarget = NULL;
		other->goalentity = other->enemy;
		other->monsterinfo.aiflags &= ~AI_COMBAT_POINT;
	}

	if (self->pathtarget)
	{
		char *savetarget;
		savetarget = self->target;
		self->target = self->pathtarget;

		if (other->enemy && other->enemy->server.client)
			activator = other->enemy;
		else if (other->oldenemy && other->oldenemy->server.client)
			activator = other->oldenemy;
		else if (other->activator && other->activator->server.client)
			activator = other->activator;
		else
			activator = other;

		G_UseTargets(self, activator);
		self->target = savetarget;
	}
}
#endif

void SP_point_combat(edict_t *self)
{
#ifdef ENABLE_COOP
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	self->server.solid = SOLID_TRIGGER;
	self->touch = point_combat_touch;
	VectorSet(self->server.mins, -8, -8, -16);
	VectorSet(self->server.maxs, 8, 8, 16);
	self->server.flags.noclient = true;
	gi.linkentity(self);
#else
	G_FreeEdict(self);
#endif
}

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for spotlights, etc.
*/
void SP_info_null(edict_t *self)
{
	G_FreeEdict(self);
}


/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for lightning.
*/
void SP_info_notnull(edict_t *self)
{
	VectorCopy(self->server.state.origin, self->server.absmin);
	VectorCopy(self->server.state.origin, self->server.absmax);
}


/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Non-displayed light.
Default light value is 300.
Default style is 0.
If targeted, will toggle between on and off.
Default _cone value is 10 (used to set size of light for spotlights)
*/

#ifdef ENABLE_COOP
#define START_OFF   1

void light_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->spawnflags & START_OFF)
	{
		gi.configstring(CS_LIGHTS + self->style, "m");
		self->spawnflags &= ~START_OFF;
	}
	else
	{
		gi.configstring(CS_LIGHTS + self->style, "a");
		self->spawnflags |= START_OFF;
	}
}
#endif

void SP_light(edict_t *self)
{
#ifdef ENABLE_COOP
	// no targeted lights in deathmatch, because they cause global messages
	if (!self->targetname || deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	if (self->style >= 32)
	{
		self->use = light_use;

		if (self->spawnflags & START_OFF)
			gi.configstring(CS_LIGHTS + self->style, "a");
		else
			gi.configstring(CS_LIGHTS + self->style, "m");
	}
#else
	G_FreeEdict(self);
#endif
}


/*QUAKED func_wall (0 .5 .8) ? TRIGGER_SPAWN TOGGLE START_ON ANIMATED ANIMATED_FAST
This is just a solid wall if not inhibited

TRIGGER_SPAWN   the wall will not be present until triggered
				it will then blink in to existance; it will
				kill anything that was in it's way

TOGGLE          only valid for TRIGGER_SPAWN walls
				this allows the wall to be turned on and off

START_ON        only valid for TRIGGER_SPAWN walls
				the wall will initially be present
*/

void func_wall_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->server.solid == SOLID_NOT)
	{
		self->server.solid = SOLID_BSP;
		self->server.flags.noclient = false;
		KillBox(self);
	}
	else
	{
		self->server.solid = SOLID_NOT;
		self->server.flags.noclient = true;
	}

	gi.linkentity(self);

	if (!(self->spawnflags & 2))
		self->use = NULL;
}

void SP_func_wall(edict_t *self)
{
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);

	if (self->spawnflags & 8)
		self->server.state.effects |= EF_ANIM_ALL;

	if (self->spawnflags & 16)
		self->server.state.effects |= EF_ANIM_ALLFAST;

	// just a wall
	if ((self->spawnflags & 7) == 0)
	{
		self->server.solid = SOLID_BSP;
		gi.linkentity(self);
		return;
	}

	// it must be TRIGGER_SPAWN
	if (!(self->spawnflags & 1))
	{
		//      Com_Printf("func_wall missing TRIGGER_SPAWN\n");
		self->spawnflags |= 1;
	}

	// yell if the spawnflags are odd
	if (self->spawnflags & 4)
	{
		if (!(self->spawnflags & 2))
		{
			Com_Printf("func_wall START_ON without TOGGLE\n");
			self->spawnflags |= 2;
		}
	}

	self->use = func_wall_use;

	if (self->spawnflags & 4)
		self->server.solid = SOLID_BSP;
	else
	{
		self->server.solid = SOLID_NOT;
		self->server.flags.noclient = true;
	}

	gi.linkentity(self);
}


/*QUAKED func_object (0 .5 .8) ? TRIGGER_SPAWN ANIMATED ANIMATED_FAST
This is solid bmodel that will fall if it's support it removed.
*/

void func_object_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// only squash thing we fall on top of
	if (!plane)
		return;

	if (plane->normal[2] < 1.0f)
		return;

	if (other->takedamage == false)
		return;

	T_Damage(other, self, self, vec3_origin, self->server.state.origin, vec3_origin, self->dmg, 1, DAMAGE_NONE, MakeBlankMeansOfDeath(self));
}

void func_object_release(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->touch = func_object_touch;
}

void func_object_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->server.solid = SOLID_BSP;
	self->server.flags.noclient = false;
	self->use = NULL;
	KillBox(self);
	func_object_release(self);
}

void SP_func_object(edict_t *self)
{
	gi.setmodel(self, self->model);
	self->server.mins[0] += 1;
	self->server.mins[1] += 1;
	self->server.mins[2] += 1;
	self->server.maxs[0] -= 1;
	self->server.maxs[1] -= 1;
	self->server.maxs[2] -= 1;

	if (!self->dmg)
		self->dmg = 100;

	if (self->spawnflags == 0)
	{
		self->server.solid = SOLID_BSP;
		self->movetype = MOVETYPE_PUSH;
		self->think = func_object_release;
		self->nextthink = level.time + (game.frametime * 2);
	}
	else
	{
		self->server.solid = SOLID_NOT;
		self->movetype = MOVETYPE_PUSH;
		self->use = func_object_use;
		self->server.flags.noclient = true;
	}

	if (self->spawnflags & 2)
		self->server.state.effects |= EF_ANIM_ALL;

	if (self->spawnflags & 4)
		self->server.state.effects |= EF_ANIM_ALLFAST;

	self->server.clipmask = MASK_MONSTERSOLID;
	gi.linkentity(self);
}


/*QUAKED func_explosive (0 .5 .8) ? Trigger_Spawn ANIMATED ANIMATED_FAST
Any brush that you want to explode or break apart.  If you want an
ex0plosion, set dmg and it will do a radius explosion of that amount
at the center of the bursh.

If targeted it will not be shootable.

health defaults to 100.

mass defaults to 75.  This determines how much debris is emitted when
it explodes.  You get one large chunk per 100 of mass (up to 8) and
one small chunk per 25 of mass (up to 16).  So 800 gives the most.
*/
#ifdef ENABLE_COOP
void func_explosive_explode(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	vec3_t  origin;
	vec3_t  chunkorigin;
	vec3_t  size;
	int     count;
	int     mass;
	// bmodel origins are (0 0 0), we need to adjust that here
	VectorScale(self->size, 0.5f, size);
	VectorAdd(self->server.absmin, size, origin);
	VectorCopy(origin, self->server.state.origin);
	self->takedamage = false;

	if (self->dmg)
		T_RadiusDamage(self, attacker, self->dmg, NULL, DAMAGE_NONE, self->dmg + 40, MakeBlankMeansOfDeath(self));

	VectorSubtract(self->server.state.origin, inflictor->server.state.origin, self->velocity);
	VectorNormalize(self->velocity);
	VectorScale(self->velocity, 150, self->velocity);
	// start chunks towards the center
	VectorScale(size, 0.5f, size);
	mass = self->mass;

	if (!mass)
		mass = 75;

	// big chunks
	if (mass >= 100)
	{
		count = mass / 100;

		if (count > 8)
			count = 8;

		while (count--)
		{
			chunkorigin[0] = origin[0] + crandom() * size[0];
			chunkorigin[1] = origin[1] + crandom() * size[1];
			chunkorigin[2] = origin[2] + crandom() * size[2];
			ThrowDebris(self, "models/objects/debris1/tris.md2", 1, chunkorigin);
		}
	}

	// small chunks
	count = mass / 25;

	if (count > 16)
		count = 16;

	while (count--)
	{
		chunkorigin[0] = origin[0] + crandom() * size[0];
		chunkorigin[1] = origin[1] + crandom() * size[1];
		chunkorigin[2] = origin[2] + crandom() * size[2];
		ThrowDebris(self, "models/objects/debris2/tris.md2", 2, chunkorigin);
	}

	G_UseTargets(self, attacker);

	if (self->dmg)
		BecomeExplosion1(self);
	else
		G_FreeEdict(self);
}

void func_explosive_use(edict_t *self, edict_t *other, edict_t *activator)
{
	func_explosive_explode(self, self, other, self->health, vec3_origin);
}

void func_explosive_spawn(edict_t *self, edict_t *other, edict_t *activator)
{
	self->server.solid = SOLID_BSP;
	self->server.flags.noclient = false;
	self->use = NULL;
	KillBox(self);
	gi.linkentity(self);
}
#endif

void SP_func_explosive(edict_t *self)
{
#ifdef ENABLE_COOP
	if (deathmatch->value)
	{
		// auto-remove for deathmatch
		G_FreeEdict(self);
		return;
	}

	self->movetype = MOVETYPE_PUSH;
	gi.modelindex("models/objects/debris1/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");
	gi.setmodel(self, self->model);

	if (self->spawnflags & 1)
	{
		self->server.flags.noclient = true;
		self->server.solid = SOLID_NOT;
		self->use = func_explosive_spawn;
	}
	else
	{
		self->server.solid = SOLID_BSP;

		if (self->targetname)
			self->use = func_explosive_use;
	}

	if (self->spawnflags & 2)
		self->server.state.effects |= EF_ANIM_ALL;

	if (self->spawnflags & 4)
		self->server.state.effects |= EF_ANIM_ALLFAST;

	if (self->use != func_explosive_use)
	{
		if (!self->health)
			self->health = 100;

		self->die = func_explosive_explode;
		self->takedamage = true;
	}

	gi.linkentity(self);
#else
	G_FreeEdict(self);
#endif
}


/*QUAKED misc_explobox (0 .5 .8) (-16 -16 0) (16 16 40)
Large exploding box.  You can override its mass (100),
health (80), and dmg (150).
*/
#ifdef ENABLE_COOP
void barrel_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)

{
	float   ratio;
	vec3_t  v;

	if ((!other->groundentity) || (other->groundentity == self))
		return;

	ratio = (float)other->mass / (float)self->mass;
	VectorSubtract(self->server.state.origin, other->server.state.origin, v);
	M_walkmove(self, vectoyaw(v), 20 * ratio * game.frameseconds);
}

void barrel_explode(edict_t *self)
{
	vec3_t  org;
	float   spd;
	vec3_t  save;
	T_RadiusDamage(self, self->activator, self->dmg, NULL, DAMAGE_NONE, self->dmg + 40, MakeBlankMeansOfDeath(self));
	VectorCopy(self->server.state.origin, save);
	VectorMA(self->server.absmin, 0.5f, self->size, self->server.state.origin);
	// a few big chunks
	spd = 1.5f * (float)self->dmg / 200.0f;
	org[0] = self->server.state.origin[0] + crandom() * self->size[0];
	org[1] = self->server.state.origin[1] + crandom() * self->size[1];
	org[2] = self->server.state.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris1/tris.md2", spd, org);
	org[0] = self->server.state.origin[0] + crandom() * self->size[0];
	org[1] = self->server.state.origin[1] + crandom() * self->size[1];
	org[2] = self->server.state.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris1/tris.md2", spd, org);
	// bottom corners
	spd = 1.75f * (float)self->dmg / 200.0f;
	VectorCopy(self->server.absmin, org);
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	VectorCopy(self->server.absmin, org);
	org[0] += self->size[0];
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	VectorCopy(self->server.absmin, org);
	org[1] += self->size[1];
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	VectorCopy(self->server.absmin, org);
	org[0] += self->size[0];
	org[1] += self->size[1];
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	// a bunch of little chunks
	spd = 2 * self->dmg / 200;
	org[0] = self->server.state.origin[0] + crandom() * self->size[0];
	org[1] = self->server.state.origin[1] + crandom() * self->size[1];
	org[2] = self->server.state.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->server.state.origin[0] + crandom() * self->size[0];
	org[1] = self->server.state.origin[1] + crandom() * self->size[1];
	org[2] = self->server.state.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->server.state.origin[0] + crandom() * self->size[0];
	org[1] = self->server.state.origin[1] + crandom() * self->size[1];
	org[2] = self->server.state.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->server.state.origin[0] + crandom() * self->size[0];
	org[1] = self->server.state.origin[1] + crandom() * self->size[1];
	org[2] = self->server.state.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->server.state.origin[0] + crandom() * self->size[0];
	org[1] = self->server.state.origin[1] + crandom() * self->size[1];
	org[2] = self->server.state.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->server.state.origin[0] + crandom() * self->size[0];
	org[1] = self->server.state.origin[1] + crandom() * self->size[1];
	org[2] = self->server.state.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->server.state.origin[0] + crandom() * self->size[0];
	org[1] = self->server.state.origin[1] + crandom() * self->size[1];
	org[2] = self->server.state.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->server.state.origin[0] + crandom() * self->size[0];
	org[1] = self->server.state.origin[1] + crandom() * self->size[1];
	org[2] = self->server.state.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	VectorCopy(save, self->server.state.origin);

	if (self->groundentity)
		BecomeExplosion2(self);
	else
		BecomeExplosion1(self);
}

void barrel_delay(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->takedamage = false;
	self->nextthink = level.time + (2 * game.frametime);
	self->think = barrel_explode;
	self->activator = attacker;
}
#endif

void SP_misc_explobox(edict_t *self)
{
#ifdef ENABLE_COOP
	if (deathmatch->value)
	{
		// auto-remove for deathmatch
		G_FreeEdict(self);
		return;
	}

	gi.modelindex("models/objects/debris1/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");
	gi.modelindex("models/objects/debris3/tris.md2");
	self->server.solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->model = "models/objects/barrels/tris.md2";
	self->server.state.modelindex = gi.modelindex(self->model);
	VectorSet(self->server.mins, -16, -16, 0);
	VectorSet(self->server.maxs, 16, 16, 40);

	if (!self->mass)
		self->mass = 400;

	if (!self->health)
		self->health = 10;

	if (!self->dmg)
		self->dmg = 150;

	self->die = barrel_delay;
	self->takedamage = true;
	self->monsterinfo.aiflags = AI_NOSTEP;
	self->touch = barrel_touch;
	self->think = M_droptofloor;
	self->nextthink = level.time + (2 * game.frametime);
	gi.linkentity(self);
#else
	G_FreeEdict(self);
#endif
}


//
// miscellaneous specialty items
//

/*QUAKED misc_blackhole (1 .5 0) (-8 -8 -8) (8 8 8)
*/

void misc_blackhole_use(edict_t *ent, edict_t *other, edict_t *activator)
{
	/*
	MSG_WriteByte (svc_temp_entity);
	MSG_WriteByte (TE_BOSSTPORT);
	MSG_WritePos (ent->server.state.origin);
	gi.multicast (ent->server.state.origin, MULTICAST_PVS);
	*/
	G_FreeEdict(ent);
}

void misc_blackhole_think(edict_t *self)
{
	if (++self->server.state.frame < 19)
		self->nextthink = level.time + game.frametime;
	else
	{
		self->server.state.frame = 0;
		self->nextthink = level.time + game.frametime;
	}
}

void SP_misc_blackhole(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->server.solid = SOLID_NOT;
	VectorSet(ent->server.mins, -64, -64, 0);
	VectorSet(ent->server.maxs, 64, 64, 8);
	ent->server.state.modelindex = gi.modelindex("models/objects/black/tris.md2");
	ent->server.state.renderfx = RF_TRANSLUCENT;
	ent->use = misc_blackhole_use;
	ent->think = misc_blackhole_think;
	ent->nextthink = level.time + (2 * game.frametime);
	gi.linkentity(ent);
}

/*QUAKED misc_eastertank (1 .5 0) (-32 -32 -16) (32 32 32)
*/

void misc_eastertank_think(edict_t *self)
{
	if (++self->server.state.frame < 293)
		self->nextthink = level.time + game.frametime;
	else
	{
		self->server.state.frame = 254;
		self->nextthink = level.time + game.frametime;
	}
}

void SP_misc_eastertank(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->server.solid = SOLID_BBOX;
	VectorSet(ent->server.mins, -32, -32, -16);
	VectorSet(ent->server.maxs, 32, 32, 32);
	ent->server.state.modelindex = gi.modelindex("models/monsters/tank/tris.md2");
	ent->server.state.frame = 254;
	ent->think = misc_eastertank_think;
	ent->nextthink = level.time + (2 * game.frametime);
	gi.linkentity(ent);
}

/*QUAKED misc_easterchick (1 .5 0) (-32 -32 0) (32 32 32)
*/


void misc_easterchick_think(edict_t *self)
{
	if (++self->server.state.frame < 247)
		self->nextthink = level.time + game.frametime;
	else
	{
		self->server.state.frame = 208;
		self->nextthink = level.time + game.frametime;
	}
}

void SP_misc_easterchick(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->server.solid = SOLID_BBOX;
	VectorSet(ent->server.mins, -32, -32, 0);
	VectorSet(ent->server.maxs, 32, 32, 32);
	ent->server.state.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	ent->server.state.frame = 208;
	ent->think = misc_easterchick_think;
	ent->nextthink = level.time + (2 * game.frametime);
	gi.linkentity(ent);
}

/*QUAKED misc_easterchick2 (1 .5 0) (-32 -32 0) (32 32 32)
*/


void misc_easterchick2_think(edict_t *self)
{
	if (++self->server.state.frame < 287)
		self->nextthink = level.time + game.frametime;
	else
	{
		self->server.state.frame = 248;
		self->nextthink = level.time + game.frametime;
	}
}

void SP_misc_easterchick2(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->server.solid = SOLID_BBOX;
	VectorSet(ent->server.mins, -32, -32, 0);
	VectorSet(ent->server.maxs, 32, 32, 32);
	ent->server.state.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	ent->server.state.frame = 248;
	ent->think = misc_easterchick2_think;
	ent->nextthink = level.time + (2 * game.frametime);
	gi.linkentity(ent);
}


/*QUAKED monster_commander_body (1 .5 0) (-32 -32 0) (32 32 48)
Not really a monster, this is the Tank Commander's decapitated body.
There should be a item_commander_head that has this as it's target.
*/

void commander_body_think(edict_t *self)
{
	if (++self->server.state.frame < 24)
		self->nextthink = level.time + game.frametime;
	else
		self->nextthink = 0;

	if (self->server.state.frame == 22)
		gi.sound(self, CHAN_BODY, gi.soundindex("tank/thud.wav"), 1, ATTN_NORM, 0);
}

void commander_body_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->think = commander_body_think;
	self->nextthink = level.time + game.frametime;
	gi.sound(self, CHAN_BODY, gi.soundindex("tank/pain.wav"), 1, ATTN_NORM, 0);
}

void commander_body_drop(edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->server.state.origin[2] += 2;
}

void SP_monster_commander_body(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->server.solid = SOLID_BBOX;
	self->model = "models/monsters/commandr/tris.md2";
	self->server.state.modelindex = gi.modelindex(self->model);
	VectorSet(self->server.mins, -32, -32, 0);
	VectorSet(self->server.maxs, 32, 32, 48);
	self->use = commander_body_use;
	self->takedamage = true;
	self->flags = FL_GODMODE;
	self->server.state.renderfx |= RF_FRAMELERP;
	gi.linkentity(self);
	gi.soundindex("tank/thud.wav");
	gi.soundindex("tank/pain.wav");
	self->think = commander_body_drop;
	self->nextthink = level.time + (5 * game.frametime);
}


/*QUAKED misc_banner (1 .5 0) (-4 -4 -4) (4 4 4)
The origin is the bottom of the banner.
The banner is 128 tall.
*/
void misc_banner_think(edict_t *ent)
{
	ent->server.state.frame = (ent->server.state.frame + 1) % 16;
	ent->nextthink = level.time + game.frametime;
}

void SP_misc_banner(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->server.solid = SOLID_NOT;
	ent->server.state.modelindex = gi.modelindex("models/objects/banner/tris.md2");
	ent->server.state.frame = Q_rand() % 16;
	gi.linkentity(ent);
	ent->think = misc_banner_think;
	ent->nextthink = level.time + game.frametime;
}

/*QUAKED misc_deadsoldier (1 .5 0) (-16 -16 0) (16 16 16) ON_BACK ON_STOMACH BACK_DECAP FETAL_POS SIT_DECAP IMPALED
This is the dead player model. Comes in 6 exciting different poses!
*/
#ifdef ENABLE_COOP
void misc_deadsoldier_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int     n;

	if (self->health > -80)
		return;

	gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

	for (n = 0; n < 4; n++)
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

	ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
}
#endif

void SP_misc_deadsoldier(edict_t *ent)
{
#ifdef ENABLE_COOP
	if (deathmatch->value)
	{
		// auto-remove for deathmatch
		G_FreeEdict(ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->server.solid = SOLID_BBOX;
	ent->server.state.modelindex = gi.modelindex("models/deadbods/dude/tris.md2");

	// Defaults to frame 0
	if (ent->spawnflags & 2)
		ent->server.state.frame = 1;
	else if (ent->spawnflags & 4)
		ent->server.state.frame = 2;
	else if (ent->spawnflags & 8)
		ent->server.state.frame = 3;
	else if (ent->spawnflags & 16)
		ent->server.state.frame = 4;
	else if (ent->spawnflags & 32)
		ent->server.state.frame = 5;
	else
		ent->server.state.frame = 0;

	VectorSet(ent->server.mins, -16, -16, 0);
	VectorSet(ent->server.maxs, 16, 16, 16);
	ent->deadflag = DEAD_DEAD;
	ent->takedamage = true;
	ent->server.flags.monster = ent->server.flags.deadmonster = true;
	ent->server.state.clip_contents = CONTENTS_DEADMONSTER;
	ent->die = misc_deadsoldier_die;
	ent->monsterinfo.aiflags |= AI_GOOD_GUY;
	gi.linkentity(ent);
#else
	G_FreeEdict(ent);
#endif
}

/*QUAKED misc_viper (1 .5 0) (-16 -16 0) (16 16 32)
This is the Viper for the flyby bombing.
It is trigger_spawned, so you must have something use it for it to show up.
There must be a path for it to follow once it is activated.

"speed"     How fast the Viper should fly
*/

extern void train_use(edict_t *self, edict_t *other, edict_t *activator);
extern void func_train_find(edict_t *self);

void misc_viper_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->server.flags.noclient = false;
	self->use = train_use;
	train_use(self, other, activator);
}

void SP_misc_viper(edict_t *ent)
{
	if (!ent->target)
	{
		Com_Printf("misc_viper without a target at %s\n", vtos(ent->server.absmin));
		G_FreeEdict(ent);
		return;
	}

	if (!ent->speed)
		ent->speed = 300;

	ent->movetype = MOVETYPE_PUSH;
	ent->server.solid = SOLID_NOT;
	ent->server.state.modelindex = gi.modelindex("models/ships/viper/tris.md2");
	VectorSet(ent->server.mins, -16, -16, 0);
	VectorSet(ent->server.maxs, 16, 16, 32);
	ent->think = func_train_find;
	ent->nextthink = level.time + game.frametime;
	ent->use = misc_viper_use;
	ent->server.flags.noclient = true;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;
	gi.linkentity(ent);
}


/*QUAKED misc_bigviper (1 .5 0) (-176 -120 -24) (176 120 72)
This is a large stationary viper as seen in Paul's intro
*/
void SP_misc_bigviper(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->server.solid = SOLID_BBOX;
	VectorSet(ent->server.mins, -176, -120, -24);
	VectorSet(ent->server.maxs, 176, 120, 72);
	ent->server.state.modelindex = gi.modelindex("models/ships/bigviper/tris.md2");
	gi.linkentity(ent);
}


/*QUAKED misc_viper_bomb (1 0 0) (-8 -8 -8) (8 8 8)
"dmg"   how much boom should the bomb make?
*/
void misc_viper_bomb_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	G_UseTargets(self, self->activator);
	self->server.state.origin[2] = self->server.absmin[2] + 1;
	T_RadiusDamage(self, self, self->dmg, NULL, DAMAGE_NONE, self->dmg + 40, MakeBlankMeansOfDeath(self));
	BecomeExplosion2(self);
}

void misc_viper_bomb_prethink(edict_t *self)
{
	vec3_t  v;
	float   diff;
	self->groundentity = NULL;
	diff = self->timestamp - level.time;

	if (diff < -1.0f)
		diff = -1.0f;

	VectorScale(self->moveinfo.dir, 1.0f + diff, v);
	v[2] = diff;
	diff = self->server.state.angles[2];
	vectoangles(v, self->server.state.angles);
	self->server.state.angles[2] = diff + 10;
}

void misc_viper_bomb_use(edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t *viper;
	self->server.solid = SOLID_BBOX;
	self->server.flags.noclient = false;
	self->server.state.effects |= EF_ROCKET;
	self->use = NULL;
	self->movetype = MOVETYPE_TOSS;
	self->prethink = misc_viper_bomb_prethink;
	self->touch = misc_viper_bomb_touch;
	self->activator = activator;
	viper = G_FindByType(NULL, ET_MISC_VIPER);
	VectorScale(viper->moveinfo.dir, viper->moveinfo.speed, self->velocity);
	self->timestamp = level.time;
	VectorCopy(viper->moveinfo.dir, self->moveinfo.dir);
}

void SP_misc_viper_bomb(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->server.solid = SOLID_NOT;
	VectorSet(self->server.mins, -8, -8, -8);
	VectorSet(self->server.maxs, 8, 8, 8);
	self->server.state.modelindex = gi.modelindex("models/objects/bomb/tris.md2");

	if (!self->dmg)
		self->dmg = 1000;

	self->use = misc_viper_bomb_use;
	self->server.flags.noclient = true;
	gi.linkentity(self);
}


/*QUAKED misc_strogg_ship (1 .5 0) (-16 -16 0) (16 16 32)
This is a Storgg ship for the flybys.
It is trigger_spawned, so you must have something use it for it to show up.
There must be a path for it to follow once it is activated.

"speed"     How fast it should fly
*/

extern void train_use(edict_t *self, edict_t *other, edict_t *activator);
extern void func_train_find(edict_t *self);

void misc_strogg_ship_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->server.flags.noclient = false;
	self->use = train_use;
	train_use(self, other, activator);
}

void SP_misc_strogg_ship(edict_t *ent)
{
	if (!ent->target)
	{
		Com_Printf("%s without a target at %s\n", spawnTemp.classname, vtos(ent->server.absmin));
		G_FreeEdict(ent);
		return;
	}

	if (!ent->speed)
		ent->speed = 300;

	ent->movetype = MOVETYPE_PUSH;
	ent->server.solid = SOLID_NOT;
	ent->server.state.modelindex = gi.modelindex("models/ships/strogg1/tris.md2");
	VectorSet(ent->server.mins, -16, -16, 0);
	VectorSet(ent->server.maxs, 16, 16, 32);
	ent->think = func_train_find;
	ent->nextthink = level.time + game.frametime;
	ent->use = misc_strogg_ship_use;
	ent->server.flags.noclient = true;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;
	gi.linkentity(ent);
}


/*QUAKED misc_satellite_dish (1 .5 0) (-64 -64 0) (64 64 128)
*/
void misc_satellite_dish_think(edict_t *self)
{
	self->server.state.frame++;

	if (self->server.state.frame < 38)
		self->nextthink = level.time + game.frametime;
}

void misc_satellite_dish_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->server.state.frame = 0;
	self->think = misc_satellite_dish_think;
	self->nextthink = level.time + game.frametime;
}

void SP_misc_satellite_dish(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->server.solid = SOLID_BBOX;
	VectorSet(ent->server.mins, -64, -64, 0);
	VectorSet(ent->server.maxs, 64, 64, 128);
	ent->server.state.modelindex = gi.modelindex("models/objects/satellite/tris.md2");
	ent->use = misc_satellite_dish_use;
	gi.linkentity(ent);
}


/*QUAKED light_mine1 (0 1 0) (-2 -2 -12) (2 2 12)
*/
void SP_light_mine1(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->server.solid = SOLID_BBOX;
	ent->server.state.modelindex = gi.modelindex("models/objects/minelite/light1/tris.md2");
	gi.linkentity(ent);
}


/*QUAKED light_mine2 (0 1 0) (-2 -2 -12) (2 2 12)
*/
void SP_light_mine2(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->server.solid = SOLID_BBOX;
	ent->server.state.modelindex = gi.modelindex("models/objects/minelite/light2/tris.md2");
	gi.linkentity(ent);
}


/*QUAKED misc_gib_arm (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_arm(edict_t *ent)
{
	gi.setmodel(ent, "models/objects/gibs/arm/tris.md2");
	ent->server.solid = SOLID_NOT;
	ent->server.state.effects |= EF_GIB;
	ent->takedamage = true;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->server.flags.monster = true;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random() * 200;
	ent->avelocity[1] = random() * 200;
	ent->avelocity[2] = random() * 200;
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 30000;
	gi.linkentity(ent);
}

/*QUAKED misc_gib_leg (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_leg(edict_t *ent)
{
	gi.setmodel(ent, "models/objects/gibs/leg/tris.md2");
	ent->server.solid = SOLID_NOT;
	ent->server.state.effects |= EF_GIB;
	ent->takedamage = true;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->server.flags.monster = true;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random() * 200;
	ent->avelocity[1] = random() * 200;
	ent->avelocity[2] = random() * 200;
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 300000;
	gi.linkentity(ent);
}

/*QUAKED misc_gib_head (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_head(edict_t *ent)
{
	gi.setmodel(ent, "models/objects/gibs/head/tris.md2");
	ent->server.solid = SOLID_NOT;
	ent->server.state.effects |= EF_GIB;
	ent->takedamage = true;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->server.flags.monster = true;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random() * 200;
	ent->avelocity[1] = random() * 200;
	ent->avelocity[2] = random() * 200;
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 30000;
	gi.linkentity(ent);
}

//=====================================================

/*QUAKED target_character (0 0 1) ?
used with target_string (must be on same "team")
"count" is position in the string (starts at 1)
*/

void SP_target_character(edict_t *self)
{
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);
	self->server.solid = SOLID_BSP;
	self->server.state.frame = 12;
	gi.linkentity(self);
	return;
}


/*QUAKED target_string (0 0 1) (-8 -8 -8) (8 8 8)
*/

void target_string_use(edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t *e;
	int     n, l;
	char    c;
	l = strlen(self->message);

	for (e = self->teammaster; e; e = e->teamchain)
	{
		if (!e->count)
			continue;

		n = e->count - 1;

		if (n > l)
		{
			e->server.state.frame = 12;
			continue;
		}

		c = self->message[n];

		if (c >= '0' && c <= '9')
			e->server.state.frame = c - '0';
		else if (c == '-')
			e->server.state.frame = 10;
		else if (c == ':')
			e->server.state.frame = 11;
		else
			e->server.state.frame = 12;
	}
}

void SP_target_string(edict_t *self)
{
	if (!self->message)
		self->message = "";

	self->use = target_string_use;
}


/*QUAKED func_clock (0 0 1) (-8 -8 -8) (8 8 8) TIMER_UP TIMER_DOWN START_OFF MULTI_USE
target a target_string with this

The default is to be a time of day clock

TIMER_UP and TIMER_DOWN run for "count" seconds and the fire "pathtarget"
If START_OFF, this entity must be used before it starts

"style"     0 "xx"
			1 "xx:xx"
			2 "xx:xx:xx"
*/

static void func_clock_reset(edict_t *self)
{
	self->activator = NULL;

	if (self->spawnflags & 1)
	{
		self->health = 0;
		self->wait = self->count;
	}
	else if (self->spawnflags & 2)
	{
		self->health = self->count;
		self->wait = 0;
	}
}

static void func_clock_format_countdown(edict_t *self)
{
	if (self->style == 0)
	{
		Q_snprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i", self->health);
		return;
	}

	if (self->style == 1)
	{
		Q_snprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i", self->health / 60, self->health % 60);

		if (self->message[3] == ' ')
			self->message[3] = '0';

		return;
	}

	if (self->style == 2)
	{
		Q_snprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i", self->health / 3600, (self->health - (self->health / 3600) * 3600) / 60, self->health % 60);

		if (self->message[3] == ' ')
			self->message[3] = '0';

		if (self->message[6] == ' ')
			self->message[6] = '0';

		return;
	}
}

void func_clock_think(edict_t *self)
{
	if (!self->enemy)
	{
		self->enemy = G_Find(NULL, FOFS(targetname), self->target);

		if (!self->enemy)
			return;
	}

	if (self->spawnflags & 1)
	{
		func_clock_format_countdown(self);
		self->health++;
	}
	else if (self->spawnflags & 2)
	{
		func_clock_format_countdown(self);
		self->health--;
	}
	else
	{
		struct tm   *ltime;
		time_t      gmtime;
		gmtime = time(NULL);
		ltime = localtime(&gmtime);

		if (ltime)
			Q_snprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i", ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
		else
			strcpy(self->message, "00:00:00");

		if (self->message[3] == ' ')
			self->message[3] = '0';

		if (self->message[6] == ' ')
			self->message[6] = '0';
	}

	self->enemy->message = self->message;
	self->enemy->use(self->enemy, self, self);

	if (((self->spawnflags & 1) && (self->health > self->wait)) ||
		((self->spawnflags & 2) && (self->health < self->wait)))
	{
		if (self->pathtarget)
		{
			char *savetarget;
			char *savemessage;
			savetarget = self->target;
			savemessage = self->message;
			self->target = self->pathtarget;
			self->message = NULL;
			G_UseTargets(self, self->activator);
			self->target = savetarget;
			self->message = savemessage;
		}

		if (!(self->spawnflags & 8))
			return;

		func_clock_reset(self);

		if (self->spawnflags & 4)
			return;
	}

	self->nextthink = level.time + 1000;
}

void func_clock_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (!(self->spawnflags & 8))
		self->use = NULL;

	if (self->activator)
		return;

	self->activator = activator;
	self->think(self);
}

void SP_func_clock(edict_t *self)
{
	if (!self->target)
	{
		Com_Printf("%s with no target at %s\n", spawnTemp.classname, vtos(self->server.state.origin));
		G_FreeEdict(self);
		return;
	}

	if ((self->spawnflags & 2) && (!self->count))
	{
		Com_Printf("%s with no count at %s\n", spawnTemp.classname, vtos(self->server.state.origin));
		G_FreeEdict(self);
		return;
	}

	if ((self->spawnflags & 1) && (!self->count))
		self->count = 60 * 60;;

	func_clock_reset(self);
	self->message = Z_TagMalloc(CLOCK_MESSAGE_SIZE, TAG_LEVEL);
	self->think = func_clock_think;

	if (self->spawnflags & 4)
		self->use = func_clock_use;
	else
		self->nextthink = level.time + 1000;
}

//=================================================================================

void teleporter_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t     *dest;
	int         i;

	if (!other->server.client)
		return;

	dest = G_Find(NULL, FOFS(targetname), self->target);

	if (!dest)
	{
		Com_Printf("Couldn't find destination\n");
		return;
	}

	// unlink to make sure it can't possibly interfere with KillBox
	gi.unlinkentity(other);
	VectorCopy(dest->server.state.origin, other->server.state.origin);
	VectorCopy(dest->server.state.origin, other->server.state.old_origin);
	other->server.state.origin[2] += 10;
	// clear the velocity and hold them in place briefly
	VectorClear(other->velocity);
	other->server.client->server.ps.pmove.pm_time = 160 >> 3;     // hold time
	other->server.client->server.ps.pmove.pm_flags |= PMF_TIME_TELEPORT;
	// draw the teleport splash at source and on the player
	self->server.owner->server.state.event = EV_PLAYER_TELEPORT;
	other->server.state.event = EV_PLAYER_TELEPORT;

	// set angles
	for (i = 0 ; i < 3 ; i++)
		other->server.client->server.ps.pmove.delta_angles[i] = ANGLE2SHORT(dest->server.state.angles[i] - other->server.client->resp.cmd_angles[i]);

	VectorClear(other->server.state.angles);
	VectorClear(other->server.client->server.ps.viewangles);
	VectorClear(other->server.client->v_angle);
	// kill anything at the destination
	KillBox(other);
	gi.linkentity(other);
}

/*QUAKED misc_teleporter (1 0 0) (-32 -32 -24) (32 32 -16)
Stepping onto this disc will teleport players to the targeted misc_teleporter_dest object.
*/
void SP_misc_teleporter(edict_t *ent)
{
	edict_t     *trig;

	if (!ent->target)
	{
		Com_Printf("teleporter without a target.\n");
		G_FreeEdict(ent);
		return;
	}

	gi.setmodel(ent, "models/objects/dmspot/tris.md2");
	ent->server.state.skinnum = 1;
	ent->server.state.effects = EF_TELEPORTER;
	ent->server.state.sound = gi.soundindex("world/amb10.wav");
	ent->server.solid = SOLID_BBOX;
	VectorSet(ent->server.mins, -32, -32, -24);
	VectorSet(ent->server.maxs, 32, 32, -16);
	gi.linkentity(ent);
	trig = G_Spawn();
	trig->touch = teleporter_touch;
	trig->server.solid = SOLID_TRIGGER;
	trig->target = ent->target;
	trig->server.owner = ent;
	VectorCopy(ent->server.state.origin, trig->server.state.origin);
	VectorSet(trig->server.mins, -8, -8, 8);
	VectorSet(trig->server.maxs, 8, 8, 24);
	gi.linkentity(trig);
}

/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
*/
void SP_misc_teleporter_dest(edict_t *ent)
{
	gi.setmodel(ent, "models/objects/dmspot/tris.md2");
	ent->server.state.skinnum = 0;
	ent->server.solid = SOLID_BBOX;
	//  ent->server.state.effects |= EF_FLIES;
	VectorSet(ent->server.mins, -32, -32, -24);
	VectorSet(ent->server.maxs, 32, 32, -16);
	gi.linkentity(ent);
}


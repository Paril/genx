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

#ifdef ENABLE_COOP

#include "g_local.h"


//
// monster weapons
//

//FIXME mosnters should call these with a totally accurate direction
// and we can mess it up based on skill.  Spread should be for normal
// and we can tighten or loosen based on skill.  We could muck with
// the damages too, but I'm not sure that's such a good idea.
void monster_fire_bullet(edict_t *self, vec3_t start, vec3_t dir, int damage, int kick, int hspread, int vspread, int flashtype)
{
	fire_bullet(self, start, dir, damage, kick, hspread, vspread, MakeAttackerMeansOfDeath(self, self, MD_NONE, DT_DIRECT));
	MSG_WriteByte(svc_muzzleflash2);
	MSG_WriteShort(self - g_edicts);
	MSG_WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void monster_fire_shotgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int flashtype)
{
	fire_shotgun(self, start, aimdir, damage, kick, hspread, vspread, count, MakeAttackerMeansOfDeath(self, self, MD_NONE, DT_DIRECT));
	MSG_WriteByte(svc_muzzleflash2);
	MSG_WriteShort(self - g_edicts);
	MSG_WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void monster_fire_blaster(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype, int effect)
{
	fire_blaster(self, start, dir, damage, speed, effect, false);
	MSG_WriteByte(svc_muzzleflash2);
	MSG_WriteShort(self - g_edicts);
	MSG_WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void monster_fire_grenade(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int flashtype)
{
	fire_grenade(self, start, aimdir, damage, speed, 2.5f, damage + 40);
	MSG_WriteByte(svc_muzzleflash2);
	MSG_WriteShort(self - g_edicts);
	MSG_WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void monster_fire_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype)
{
	fire_rocket(self, start, dir, damage, speed, damage + 20, damage);
	MSG_WriteByte(svc_muzzleflash2);
	MSG_WriteShort(self - g_edicts);
	MSG_WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void monster_fire_railgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int flashtype)
{
	fire_rail(self, start, aimdir, damage, kick);
	MSG_WriteByte(svc_muzzleflash2);
	MSG_WriteShort(self - g_edicts);
	MSG_WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}

void monster_fire_bfg(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int kick, float damage_radius, int flashtype)
{
	fire_bfg(self, start, aimdir, damage, speed, damage_radius);
	MSG_WriteByte(svc_muzzleflash2);
	MSG_WriteShort(self - g_edicts);
	MSG_WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}



//
// Monster utility functions
//

void M_FliesOff(edict_t *self)
{
	self->server.state.effects &= ~EF_FLIES;
	self->server.state.sound = 0;
}

void M_FliesOn(edict_t *self)
{
	if (self->waterlevel)
		return;

	self->server.state.effects |= EF_FLIES;
	self->server.state.sound = gi.soundindex("infantry/inflies1.wav");
	self->think = M_FliesOff;
	self->nextthink = level.time + 60000;
}

void M_FlyCheck(edict_t *self)
{
	if (self->waterlevel)
		return;

	if (random() > 0.5f)
		return;

	self->think = M_FliesOn;
	self->nextthink = level.time + 5000 + (Q_rand() % 10000);
}

void AttackFinished(edict_t *self, float time)
{
	if (skill->integer < 3)
	{
		self->monsterinfo.attack_finished = level.time + time * 1000;
		self->monsterinfo.aiflags |= AI_JUST_ATTACKED;
	}
}


void M_CheckGround(edict_t *ent)
{
	vec3_t      point;
	trace_t     trace;

	if (ent->flags & (FL_SWIM | FL_FLY))
		return;

	if (ent->velocity[2] > 100)
	{
		ent->groundentity = NULL;
		return;
	}

	// if the hull point one-quarter unit down is solid the entity is on ground
	point[0] = ent->server.state.origin[0];
	point[1] = ent->server.state.origin[1];
	point[2] = ent->server.state.origin[2] - 0.25f;
	trace = gi.trace(ent->server.state.origin, ent->server.mins, ent->server.maxs, point, ent, MASK_MONSTERSOLID);

	// check steepness
	if (trace.plane.normal[2] < 0.7f && !trace.startsolid)
	{
		ent->groundentity = NULL;
		return;
	}

	//  ent->groundentity = trace.ent;
	//  ent->groundentity_linkcount = trace.ent->server.linkcount;
	//  if (!trace.startsolid && !trace.allsolid)
	//      VectorCopy (trace.endpos, ent->server.state.origin);
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy(trace.endpos, ent->server.state.origin);
		ent->groundentity = trace.ent;
		ent->groundentity_linkcount = trace.ent->server.linkcount;
		ent->velocity[2] = 0;
	}
}


void M_CatagorizePosition(edict_t *ent)
{
	vec3_t      point;
	int         cont;
	//
	// get waterlevel
	//
	point[0] = ent->server.state.origin[0];
	point[1] = ent->server.state.origin[1];
	point[2] = ent->server.state.origin[2] + ent->server.mins[2] + 1;
	cont = gi.pointcontents(point);

	if (!(cont & MASK_WATER))
	{
		ent->waterlevel = 0;
		ent->watertype = 0;
		return;
	}

	ent->watertype = cont;
	ent->waterlevel = 1;
	point[2] += 26;
	cont = gi.pointcontents(point);

	if (!(cont & MASK_WATER))
		return;

	ent->waterlevel = 2;
	point[2] += 22;
	cont = gi.pointcontents(point);

	if (cont & MASK_WATER)
		ent->waterlevel = 3;
}


void M_WorldEffects(edict_t *ent)
{
	int     dmg;

	if (gi.pointcontents(ent->server.state.origin) == CONTENTS_SOLID)
		T_Damage(ent, world, world, vec3_origin, ent->server.state.origin, vec3_origin, 1000, 0, DAMAGE_NO_ARMOR, MakeGenericMeansOfDeath(world, MD_FALLING, DT_DIRECT));

	if (ent->health > 0)
	{
		if (!(ent->flags & FL_SWIM))
		{
			if (ent->waterlevel < 3)
				ent->air_finished = level.time + 12;
			else if (ent->air_finished < level.time)
			{
				// drown!
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + 2 * floor(level.time - ent->air_finished);

					if (dmg > 15)
						dmg = 15;

					T_Damage(ent, world, world, vec3_origin, ent->server.state.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MakeGenericMeansOfDeath(world, MD_WATER, DT_DIRECT));
					ent->pain_debounce_time = level.time + 1000;
				}
			}
		}
		else
		{
			if (ent->waterlevel > 0)
				ent->air_finished = level.time + 9;
			else if (ent->air_finished < level.time)
			{
				// suffocate!
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + 2 * floor(level.time - ent->air_finished);

					if (dmg > 15)
						dmg = 15;

					T_Damage(ent, world, world, vec3_origin, ent->server.state.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MakeGenericMeansOfDeath(world, MD_WATER, DT_DIRECT));
					ent->pain_debounce_time = level.time + 1000;
				}
			}
		}
	}

	if (ent->waterlevel == 0)
	{
		if (ent->flags & FL_INWATER)
		{
			gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
			ent->flags &= ~FL_INWATER;
		}

		return;
	}

	if ((ent->watertype & CONTENTS_LAVA) && !(ent->flags & FL_IMMUNE_LAVA))
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 200;
			T_Damage(ent, world, world, vec3_origin, ent->server.state.origin, vec3_origin, 10 * ent->waterlevel, 0, DAMAGE_NONE, MakeGenericMeansOfDeath(world, MD_LAVA, DT_DIRECT));
		}
	}

	if ((ent->watertype & CONTENTS_SLIME) && !(ent->flags & FL_IMMUNE_SLIME))
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 1000;
			T_Damage(ent, world, world, vec3_origin, ent->server.state.origin, vec3_origin, 4 * ent->waterlevel, 0, DAMAGE_NONE, MakeGenericMeansOfDeath(world, MD_SLIME, DT_DIRECT));
		}
	}

	if (!(ent->flags & FL_INWATER))
	{
		if (!ent->server.flags.deadmonster)
		{
			if (ent->watertype & CONTENTS_LAVA)
				if (random() <= 0.5f)
					gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1, ATTN_NORM, 0);
			else if (ent->watertype & CONTENTS_SLIME)
				gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
			else if (ent->watertype & CONTENTS_WATER)
				gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		}

		ent->flags |= FL_INWATER;
		ent->damage_debounce_time = 0;
	}
}


void M_droptofloor(edict_t *ent)
{
	vec3_t      end;
	trace_t     trace;
	ent->server.state.origin[2] += 1;
	VectorCopy(ent->server.state.origin, end);
	end[2] -= 256;
	trace = gi.trace(ent->server.state.origin, ent->server.mins, ent->server.maxs, end, ent, MASK_MONSTERSOLID);

	if (trace.fraction == 1 || trace.allsolid)
		return;

	VectorCopy(trace.endpos, ent->server.state.origin);
	gi.linkentity(ent);
	M_CheckGround(ent);
	M_CatagorizePosition(ent);
}


void M_SetEffects(edict_t *ent)
{
	ent->server.state.effects &= ~(EF_COLOR_SHELL | EF_POWERSCREEN);
	ent->server.state.renderfx &= ~(RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_FROZEN);

	if (ent->monsterinfo.aiflags & AI_RESURRECTING)
	{
		ent->server.state.effects |= EF_COLOR_SHELL;
		ent->server.state.renderfx |= RF_SHELL_RED;
	}

	if (ent->freeze_time > level.time)
		ent->server.state.renderfx |= RF_FROZEN;

	if (ent->health <= 0)
		return;

	if (ent->powerarmor_time > level.time)
	{
		if (ent->monsterinfo.power_armor_type == POWER_ARMOR_SCREEN)
			ent->server.state.effects |= EF_POWERSCREEN;
		else if (ent->monsterinfo.power_armor_type == POWER_ARMOR_SHIELD)
		{
			ent->server.state.effects |= EF_COLOR_SHELL;
			ent->server.state.renderfx |= RF_SHELL_GREEN;
		}
	}
}

static inline int signum(int x)
{
	return (x > 0) - (x < 0);
}

void M_MoveFrame(edict_t *self)
{
	const mmove_t *move;
	int     index;
	int		*frame_ptr;

	if (self->monsterinfo.special_frames)
		frame_ptr = &self->style;
	else
		frame_ptr = &self->server.state.frame;

	move = self->monsterinfo.currentmove;
	self->nextthink = level.time + 100;
	int move_direction = signum(move->lastframe - move->firstframe);
	int start = min(move->firstframe, move->lastframe);
	int end = max(move->lastframe, move->firstframe);

	if (move_direction == -1)
		move_direction = -1;

	if ((self->monsterinfo.nextframe) && (self->monsterinfo.nextframe >= start) && (self->monsterinfo.nextframe <= end))
	{
		*frame_ptr = self->monsterinfo.nextframe;
		self->monsterinfo.nextframe = 0;
	}
	else
	{
		if (*frame_ptr == move->lastframe)
		{
			if (move->endfunc)
			{
				move->endfunc(self);

				if (self->server.flags.deadmonster)
					return;

				// regrab move, endfunc is very likely to change it
				move = self->monsterinfo.currentmove;

				if (!move)
					return;

				move_direction = signum(move->lastframe - move->firstframe);
				start = min(move->firstframe, move->lastframe);
				end = max(move->lastframe, move->firstframe);
			}
		}

		if (*frame_ptr < start || *frame_ptr > end)
		{
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
			*frame_ptr = move->firstframe;
		}
		else
		{
			if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			{
				*frame_ptr += move_direction;

				if (*frame_ptr > end)
					*frame_ptr = start;
				else if (*frame_ptr < start)
					*frame_ptr = end;
			}
		}
	}

	index = abs(*frame_ptr - move->firstframe);

	if (self->monsterinfo.special_frames)
		self->server.state.frame = move->frame[index].real_frame;

	if (move->frame[index].aifunc)
	{
		if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			move->frame[index].aifunc(self, move->frame[index].dist * self->monsterinfo.scale);
		else
			move->frame[index].aifunc(self, 0);
	}

	if (move->frame[index].thinkfunc)
		move->frame[index].thinkfunc(self);
}


void monster_think(edict_t *self)
{
	if (self->freeze_time <= level.time)
		M_MoveFrame(self);
	else
		self->nextthink = level.time + game.frametime;

	if (self->server.linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->server.linkcount;
		M_CheckGround(self);
	}

	M_CatagorizePosition(self);
	M_WorldEffects(self);
	M_SetEffects(self);
}


/*
================
monster_use

Using a monster makes it angry at the current activator
================
*/
void monster_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->enemy)
		return;

	if (self->health <= 0)
		return;

	if (activator->flags & FL_NOTARGET)
		return;

	if (!(activator->server.client) && !(activator->monsterinfo.aiflags & AI_GOOD_GUY))
		return;

	// delay reaction so if the monster is teleported, its sound is still heard
	self->enemy = activator;
	FoundTarget(self);
}


void monster_start_go(edict_t *self);


void monster_triggered_spawn(edict_t *self)
{
	self->server.state.origin[2] += 1;
	KillBox(self);
	self->server.solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->server.flags.noclient = false;
	self->air_finished = level.time + 12;
	self->spawnflags &= ~2;
	gi.linkentity(self);
	monster_start_go(self);

	if (self->enemy && !(self->spawnflags & 1) && !(self->enemy->flags & FL_NOTARGET))
		FoundTarget(self);
	else
		self->enemy = NULL;
}

void monster_triggered_spawn_use(edict_t *self, edict_t *other, edict_t *activator)
{
	// we have a one frame delay here so we don't telefrag the guy who activated us
	self->think = monster_triggered_spawn;
	self->nextthink = level.time + 100;

	if (activator->server.client)
		self->enemy = activator;

	self->use = monster_use;
}

void monster_triggered_start(edict_t *self)
{
	self->server.solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	self->server.flags.noclient = true;
	self->nextthink = 0;
	self->use = monster_triggered_spawn_use;
}


/*
================
monster_death_use

When a monster dies, it fires all of its targets with the current
enemy as activator.
================
*/

void monster_death_use(edict_t *self)
{
	if (invasion->value)
	{
		if (!LIST_EMPTY(&self->monsterinfo.wave_entry))
			Wave_MonsterDead(self);
	}

	if (self->entitytype != ET_DOOM_MONSTER_SKUL && self->entitytype != ET_DOOM_MONSTER_PAIN)
		self->flags &= ~(FL_FLY | FL_SWIM);

	self->monsterinfo.aiflags &= AI_GOOD_GUY;

	if (self->item)
	{
		Drop_Item(self, self->item);
		self->item = NULL;
	}

	if (self->deathtarget)
		self->target = self->deathtarget;

	if (!self->target)
		return;

	G_UseTargets(self, self->enemy);
}


//============================================================================

bool monster_start(edict_t *self)
{
	if (deathmatch->value || (invasion->value && level.time == 0))
	{
		G_FreeEdict(self);
		return false;
	}

	if ((self->spawnflags & 4) && !(self->monsterinfo.aiflags & AI_GOOD_GUY))
	{
		self->spawnflags &= ~4;
		self->spawnflags |= 1;
		//      Com_Printf("fixed spawnflags on %s at %s\n", self->classname, vtos(self->server.state.origin));
	}

	if (!(self->monsterinfo.aiflags & AI_GOOD_GUY))
		level.total_monsters++;

	self->nextthink = level.time + 100;
	self->server.flags.monster = true;
	self->server.state.renderfx |= RF_FRAMELERP;
	self->takedamage = true;
	self->air_finished = level.time + 12;
	self->use = monster_use;
	self->max_health = self->health;
	self->server.clipmask = MASK_MONSTERSOLID;
	self->deadflag = DEAD_NO;
	self->server.flags.deadmonster = false;
	self->server.state.clip_contents = CONTENTS_MONSTER;

	if (self->server.state.game == GAME_DOOM && !self->mass)
		self->mass = 100;

	if (!self->monsterinfo.checkattack)
	{
		if (self->server.state.game == GAME_Q1)
			self->monsterinfo.checkattack = M_Q1_CheckAttack;
		else if (self->server.state.game == GAME_DOOM)
			self->monsterinfo.checkattack = M_Doom_CheckAttack;
		else
			self->monsterinfo.checkattack = M_CheckAttack;
	}

	VectorCopy(self->server.state.origin, self->server.state.old_origin);

	if (spawnTemp.item)
	{
		self->item = FindItemByClassname(spawnTemp.item);

		if (!self->item)
			Com_Printf("entityid %i at %s has bad item: %s\n", self->entitytype, vtos(self->server.state.origin), spawnTemp.item);
	}

	// randomize what frame they start on
	if (self->monsterinfo.currentmove && self->server.state.game != GAME_DOOM)
		self->server.state.frame = self->monsterinfo.currentmove->firstframe + (Q_rand() % (self->monsterinfo.currentmove->lastframe - self->monsterinfo.currentmove->firstframe + 1));

	if (invasion->value)
	{
		if (!self->health)
			Com_Error(ERR_FATAL, "whoops");

		self->think(self);
	}

	return true;
}

bool M_tryoriginfix(edict_t *self)
{
	return false;
	/*bool success = false;
	vec3_t main_origin;
	VectorCopy(self->server.state.origin, main_origin);

	// Preliminary trace stuff
	trace_t updown_traces[2];
	trace_t *updown_longest = NULL;

	for (int i = 0; i < 2; ++i)
	{
		vec3_t dir = { 0, 0, (i == 1) ? -1 : 1 };

		VectorMA(main_origin, 8192, dir, updown_traces[i].endpos);
		updown_traces[i] = gi.trace(main_origin, self->server.mins, self->server.maxs, updown_traces[i].endpos, self, MASK_SOLID | CONTENTS_MONSTER);

		if (updown_traces[i].fraction == 1.0)
			assert(false);

		if (!updown_longest || updown_longest->fraction > updown_traces[i].fraction)
			updown_longest = &updown_traces[i];
	}

	int num_attempts = 0;
	trace_t angle_traces[8];
	trace_t *longest_trace = NULL;

	for (int i = 0; i < 8; ++i)
	{
		vec3_t angle = { 0, (360 / 8) * i, };
		AngleVectors(angle, angle, NULL, NULL);

		VectorMA(main_origin, 8192, angle, angle_traces[i].endpos);
		angle_traces[i] = gi.trace(main_origin, self->server.mins, self->server.maxs, angle_traces[i].endpos, self, MASK_SOLID | CONTENTS_MONSTER);

		if (angle_traces[i].fraction == 1.0)
			assert(false);

		if (!longest_trace || longest_trace->fraction > angle_traces[i].fraction)
			longest_trace = &angle_traces[i];
	}

	bool can_fit_x = false, can_fit_y = false;

	float x_fit = fabsf(angle_traces[0].endpos[0] - angle_traces[4].endpos[0]);
	float y_fit = fabsf(angle_traces[2].endpos[1] - angle_traces[6].endpos[1]);

	can_fit_x = x_fit >= self->server.maxs[0] - self->server.mins[0];
	can_fit_y = y_fit >= self->server.maxs[1] - self->server.mins[1];

	// Step 1: if we can fit in this spot with our x/y traces,
	// something must be messed up with the Z.
	if (can_fit_x && can_fit_y && !angle_traces[0].startsolid && !angle_traces[4].startsolid && !angle_traces[2].startsolid && !angle_traces[6].startsolid)
	{
		assert(false);
	}

	// Step 2: if only one edge of our bbox is stuck in a wall, we should
	// be able to just sidestep in that direction to get us in the right spot
	else if (can_fit_x != can_fit_y)
	{

	}

	while (num_attempts < 512)
	{
		++num_attempts;
	}

	if (!success)
	{
		VectorCopy(main_origin, self->server.state.origin);
		return false;
	}*/
}

void monster_start_go(edict_t *self)
{
	vec3_t  v;

	if (self->health <= 0)
		return;

	if (self->server.state.game == GAME_DOOM)
		self->yaw_speed = 30;

	// check for target to combat_point and change to combattarget
	if (self->target)
	{
		bool        notcombat;
		bool        fixup;
		edict_t     *target;
		target = NULL;
		notcombat = false;
		fixup = false;

		while ((target = G_Find(target, FOFS(targetname), self->target)) != NULL)
		{
			if (target->entitytype == ET_POINT_COMBAT)
			{
				self->combattarget = self->target;
				fixup = true;
			}
			else
				notcombat = true;
		}

		if (notcombat && self->combattarget)
			Com_Printf("entityid %i at %s has target with mixed types\n", self->entitytype, vtos(self->server.state.origin));

		if (fixup)
			self->target = NULL;
	}

	// validate combattarget
	if (self->combattarget)
	{
		edict_t     *target;
		target = NULL;

		while ((target = G_Find(target, FOFS(targetname), self->combattarget)) != NULL)
		{
			if (target->entitytype != ET_POINT_COMBAT)
			{
				Com_Printf("entityid %i at (%i %i %i) has a bad combattarget %s : entityid %i at (%i %i %i)\n",
					self->entitytype, (int)self->server.state.origin[0], (int)self->server.state.origin[1], (int)self->server.state.origin[2],
					self->combattarget, target->entitytype, (int)target->server.state.origin[0], (int)target->server.state.origin[1],
					(int)target->server.state.origin[2]);
			}
		}
	}

	if (self->target)
	{
		self->goalentity = self->movetarget = G_PickTarget(self->target);

		if (!self->movetarget)
		{
			Com_Printf("entityid %i can't find target %s at %s\n", self->entitytype, self->target, vtos(self->server.state.origin));
			self->target = NULL;
			self->monsterinfo.pausetime = GTIME_MAX;
			self->monsterinfo.stand(self);
		}
		else if (self->movetarget->entitytype == ET_PATH_CORNER)
		{
			VectorSubtract(self->goalentity->server.state.origin, self->server.state.origin, v);
			self->ideal_yaw = self->server.state.angles[YAW] = vectoyaw(v);
			self->monsterinfo.walk(self);
			self->target = NULL;
		}
		else
		{
			self->goalentity = self->movetarget = NULL;
			self->monsterinfo.pausetime = GTIME_MAX;
			self->monsterinfo.stand(self);
		}
	}
	else
	{
		self->monsterinfo.pausetime = GTIME_MAX;
		self->monsterinfo.stand(self);
	}

	if (!(self->spawnflags & 2))
	{
		if (!(self->flags & (FL_FLY | FL_SWIM)))
			M_droptofloor(self);

		if (!(self->flags & (FL_FLY | FL_SWIM)) || self->groundentity)
		{
			bool found_fit = false;
			vec3_t new_mins, new_maxs;
			vec3_t old_mins, old_maxs;
			VectorCopy(self->server.mins, old_mins);
			VectorCopy(self->server.maxs, old_maxs);
			int i;
			float refit_size = VectorLength(self->size) / 14;

			for (i = 0; i < 12; ++i)
			{
				for (int x = 0; x < 2; ++x)
				{
					new_mins[x] = self->server.mins[x] + refit_size * i;
					new_maxs[x] = self->server.maxs[x] - refit_size * i;
				}

				new_maxs[2] = self->server.maxs[2] - refit_size * i;
				new_mins[2] = old_mins[2];
				VectorCopy(new_mins, self->server.mins);
				VectorCopy(new_maxs, self->server.maxs);

				if (gi.trace(self->server.state.origin, self->server.mins, self->server.maxs, self->server.state.origin, self, MASK_MONSTERSOLID).startsolid)
				{
					VectorCopy(old_mins, self->server.mins);
					VectorCopy(old_maxs, self->server.maxs);
					continue;
				}

				// new bbox found!
				found_fit = true;
				break;
			}

			if (found_fit)
			{
				if (i != 0)
				{
					self->flags |= FL_STUCK;
					VectorCopy(old_mins, self->pos1);
					VectorCopy(old_maxs, self->pos2);
					Com_Printf("entityid %i in solid, refit with new bbox at %s\n", self->entitytype, vtos(self->server.state.origin));
					gi.linkentity(self);
				}
			}
			else
				Com_Printf("entityid %i in solid, couldn't refit, at %s\n", self->entitytype, vtos(self->server.state.origin));
		}

		if (!(self->flags & (FL_FLY | FL_SWIM)))
			M_droptofloor(self);
	}

	self->think = monster_think;
	self->nextthink = level.time + 100;
}


void walkmonster_start_go(edict_t *self)
{
	if (!self->yaw_speed)
		self->yaw_speed = 20;

	self->viewheight = 25;
	monster_start_go(self);

	if (self->spawnflags & 2)
		monster_triggered_start(self);
}

void walkmonster_start(edict_t *self)
{
	self->think = walkmonster_start_go;
	monster_start(self);
}


void flymonster_start_go(edict_t *self)
{
	if (!self->yaw_speed)
		self->yaw_speed = 10;

	self->viewheight = 25;
	monster_start_go(self);

	if (self->spawnflags & 2)
		monster_triggered_start(self);
}


void flymonster_start(edict_t *self)
{
	self->flags |= FL_FLY;
	self->think = flymonster_start_go;
	monster_start(self);
}


void swimmonster_start_go(edict_t *self)
{
	if (!self->yaw_speed)
		self->yaw_speed = 10;

	self->viewheight = 10;
	monster_start_go(self);

	if (self->spawnflags & 2)
		monster_triggered_start(self);
}

void swimmonster_start(edict_t *self)
{
	self->flags |= FL_SWIM;
	self->think = swimmonster_start_go;
	monster_start(self);
}

#endif
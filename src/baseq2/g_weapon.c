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
#include "g_local.h"


#ifdef ENABLE_COOP
/*
=================
check_dodge

This is a support routine used when a client is firing
a non-instant attack weapon.  It checks to see if a
monster's dodge function should be called.
=================
*/
void check_dodge(edict_t *self, vec3_t start, vec3_t dir, int speed)
{
	vec3_t  end;
	vec3_t  v;
	trace_t tr;
	float   eta;

	if (!self->server.client)
		return;

	// easy mode only ducks one quarter the time
	if (skill->value == 0)
	{
		if (random() > 0.25f)
			return;
	}

	VectorMA(start, 8192, dir, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);

	if (tr.ent && tr.ent->server.flags.monster && (tr.ent->health > 0) && (tr.ent->monsterinfo.dodge) && infront(tr.ent, self))
	{
		VectorSubtract(tr.endpos, start, v);
		eta = (VectorLength(v) - tr.ent->server.maxs[0]) / speed;
		tr.ent->monsterinfo.dodge(tr.ent, self, eta);
	}
}
#endif


/*
=================
fire_hit

Used for all impact (hit/punch/slash) attacks
=================
*/
bool fire_hit(edict_t *self, vec3_t aim, int damage, int kick)
{
	trace_t     tr;
	vec3_t      forward, right, up;
	vec3_t      v;
	vec3_t      point;
	float       range;
	vec3_t      dir;
	//see if enemy is in range
	VectorSubtract(self->enemy->server.state.origin, self->server.state.origin, dir);
	range = VectorLength(dir);

	if (range > aim[0])
		return false;

	if (aim[1] > self->server.mins[0] && aim[1] < self->server.maxs[0])
	{
		// the hit is straight on so back the range up to the edge of their bbox
		range -= self->enemy->server.maxs[0];
	}
	else
	{
		// this is a side hit so adjust the "right" value out to the edge of their bbox
		if (aim[1] < 0)
			aim[1] = self->enemy->server.mins[0];
		else
			aim[1] = self->enemy->server.maxs[0];
	}

	VectorMA(self->server.state.origin, range, dir, point);
	tr = gi.trace(self->server.state.origin, NULL, NULL, point, self, MASK_SHOT);

	if (tr.fraction < 1)
	{
		if (!tr.ent->takedamage)
			return false;

		// if it will hit any client/monster then hit the one we wanted to hit
		if (tr.ent->server.flags.monster || tr.ent->server.client)
			tr.ent = self->enemy;
	}

	AngleVectors(self->server.state.angles, forward, right, up);
	VectorMA(self->server.state.origin, range, forward, point);
	VectorMA(point, aim[1], right, point);
	VectorMA(point, aim[2], up, point);
	VectorSubtract(point, self->enemy->server.state.origin, dir);
	// do the damage
	T_Damage(tr.ent, self, self, dir, point, vec3_origin, damage, kick / 2, DAMAGE_NO_KNOCKBACK, MakeAttackerMeansOfDeath(self, self, MD_MELEE, DT_DIRECT));

	if (!tr.ent->server.flags.monster && !tr.ent->server.client)
		return false;

	// do our special form of knockback here
	VectorMA(self->enemy->server.absmin, 0.5f, self->enemy->server.size, v);
	VectorSubtract(v, point, v);
	VectorNormalize(v);
	VectorMA(self->enemy->velocity, kick, v, self->enemy->velocity);

	if (self->enemy->velocity[2] > 0)
		self->enemy->groundentity = NULL;

	return true;
}

/*
=================
fire_lead

This is an internal support routine used for bullet/pellet based weapons.
=================
*/
static void fire_lead(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, meansOfDeath_t mod)
{
	trace_t     tr;
	vec3_t      dir;
	vec3_t      forward, right, up;
	vec3_t      end;
	float       r;
	float       u;
	vec3_t      water_start;
	bool	    water = false;
	int         content_mask = MASK_BULLET | MASK_WATER;
	tr = gi.trace(self->server.state.origin, NULL, NULL, start, self, MASK_BULLET);

	if (!(tr.fraction < 1.0f))
	{
		vectoangles(aimdir, dir);
		AngleVectors(dir, forward, right, up);
		r = crandom() * hspread;
		u = crandom() * vspread;
		VectorMA(start, 8192, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);

		if (gi.pointcontents(start) & MASK_WATER)
		{
			water = true;
			VectorCopy(start, water_start);
			content_mask &= ~MASK_WATER;
		}

		tr = gi.trace(start, NULL, NULL, end, self, content_mask);

		// see if we hit water
		if (tr.contents & MASK_WATER)
		{
			int     color;
			water = true;
			VectorCopy(tr.endpos, water_start);

			if (!VectorCompare(start, tr.endpos))
			{
				if (tr.contents & CONTENTS_WATER)
				{
					if (strcmp(tr.surface->name, "*brwater") == 0)
						color = SPLASH_BROWN_WATER;
					else
						color = SPLASH_BLUE_WATER;
				}
				else if (tr.contents & CONTENTS_SLIME)
					color = SPLASH_SLIME;
				else if (tr.contents & CONTENTS_LAVA)
					color = SPLASH_LAVA;
				else
					color = SPLASH_UNKNOWN;

				if (color != SPLASH_UNKNOWN)
				{
					MSG_WriteByte(svc_temp_entity);
					MSG_WriteByte(TE_SPLASH);
					MSG_WriteByte(8);
					MSG_WritePos(tr.endpos);
					MSG_WriteDir(tr.plane.normal);
					MSG_WriteByte(color);
					gi.multicast(tr.endpos, MULTICAST_PVS);
				}

				// change bullet's course when it enters water
				VectorSubtract(end, start, dir);
				vectoangles(dir, dir);
				AngleVectors(dir, forward, right, up);
				r = crandom() * hspread * 2;
				u = crandom() * vspread * 2;
				VectorMA(water_start, 8192, forward, end);
				VectorMA(end, r, right, end);
				VectorMA(end, u, up, end);
			}

			// re-trace ignoring water this time
			tr = gi.trace(water_start, NULL, NULL, end, self, MASK_BULLET);
		}
	}

	// send gun puff / flash
	if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))
	{
		if (tr.fraction < 1.0f)
		{
			if (tr.ent->takedamage)
			{
				AddMultiDamage(tr.ent, damage, kick, mod, DAMAGE_BULLET, true, tr.endpos, tr.plane.normal);
				T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_PARTICLES_ONLY | DAMAGE_NO_KNOCKBACK | DAMAGE_BULLET, blankMeansOfDeath);
			}
			else
			{
				if (strncmp(tr.surface->name, "sky", 3) != 0)
				{
					MSG_WriteByte(svc_temp_entity);
					MSG_WriteByte(te_impact);
					MSG_WritePos(tr.endpos);
					MSG_WriteDir(tr.plane.normal);
					gi.multicast(tr.endpos, MULTICAST_PVS);

#ifdef ENABLE_COOP
					PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
#endif
				}
			}
		}
	}

	// if went through water, determine where the end and make a bubble trail
	if (water)
	{
		vec3_t  pos;
		VectorSubtract(tr.endpos, water_start, dir);
		VectorNormalize(dir);
		VectorMA(tr.endpos, -2, dir, pos);

		if (gi.pointcontents(pos) & MASK_WATER)
			VectorCopy(pos, tr.endpos);
		else
			tr = gi.trace(pos, NULL, NULL, water_start, tr.ent, MASK_WATER);

		VectorAdd(water_start, tr.endpos, pos);
		VectorScale(pos, 0.5f, pos);
		MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(TE_BUBBLETRAIL);
		MSG_WritePos(water_start);
		MSG_WritePos(tr.endpos);
		gi.multicast(pos, MULTICAST_PVS);
	}
}


/*
=================
fire_bullet

Fires a single round.  Used for machinegun and chaingun.  Would be fine for
pistols, rifles, etc....
=================
*/
void fire_bullet(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, meansOfDeath_t mod)
{
	fire_lead(self, start, aimdir, damage, kick, TE_GUNSHOT, hspread, vspread, mod);
	ApplyMultiDamage(self, aimdir, DAMAGE_BULLET, mod);
}


/*
=================
fire_shotgun

Shoots shotgun pellets.  Used by shotgun and super shotgun.
=================
*/
void fire_shotgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, meansOfDeath_t mod)
{
	int i;

	for (i = 0; i < count; i++)
		fire_lead(self, start, aimdir, damage, kick, TE_SHOTGUN, hspread, vspread, mod);

	ApplyMultiDamage(self, aimdir, DAMAGE_BULLET, mod);
}


/*
=================
fire_blaster

Fires a single blaster bolt.  Used by the blaster and hyper blaster.
=================
*/
void blaster_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->server.owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

#ifdef ENABLE_COOP
	PlayerNoise(self->server.owner, self->server.state.origin, PNOISE_IMPACT);
#endif

	if (other->takedamage)
		T_Damage(other, self, self->server.owner, self->velocity, self->server.state.origin, plane->normal, self->dmg, 1, DAMAGE_ENERGY, self->meansOfDeath);
	else
	{
		MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(TE_BLASTER);
		MSG_WritePos(self->server.state.origin);

		if (!plane)
			MSG_WriteDir(vec3_origin);
		else
			MSG_WriteDir(plane->normal);

		gi.multicast(self->server.state.origin, MULTICAST_PVS);
	}

	G_FreeEdict(self);
}

void fire_blaster(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect, bool hyper)
{
	edict_t *bolt;
	trace_t tr;
	VectorNormalize(dir);
	bolt = G_Spawn();
	bolt->server.flags.deadmonster = true;
	bolt->server.state.clip_contents = CONTENTS_DEADMONSTER;
	// yes, I know it looks weird that projectiles are deadmonsters
	// what this means is that when prediction is used against the object
	// (blaster/hyperblaster shots), the player won't be solid clipped against
	// the object.  Right now trying to run into a firing hyperblaster
	// is very jerky since you are predicted 'against' the shots.
	VectorCopy(start, bolt->server.state.origin);
	VectorCopy(start, bolt->server.state.old_origin);
	vectoangles(dir, bolt->server.state.angles);
	VectorScale(dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->server.clipmask = MASK_SHOT;
	bolt->server.solid = SOLID_BBOX;
	bolt->server.state.effects |= effect;
	VectorClear(bolt->server.mins);
	VectorClear(bolt->server.maxs);
	bolt->server.state.modelindex = gi.modelindex("models/objects/laser/tris.md2");
	bolt->server.state.sound = gi.soundindex("misc/lasfly.wav");
	bolt->server.owner = self;
	bolt->touch = blaster_touch;
	bolt->nextthink = level.time + 2000;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;

	if (hyper)
		bolt->spawnflags = 1;

	gi.linkentity(bolt);

#if ENABLE_COOP
	check_dodge(self, bolt->server.state.origin, dir, speed);
#endif

	if (self->server.client)
		bolt->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->server.client->pers.weapon), bolt, DT_DIRECT);
	else
		bolt->meansOfDeath = MakeWeaponMeansOfDeath(self, (hyper) ? ITI_Q2_HYPERBLASTER : ITI_Q2_BLASTER, bolt, DT_DIRECT);

	tr = gi.trace(self->server.state.origin, NULL, NULL, bolt->server.state.origin, bolt, MASK_SHOT);

	if (tr.fraction < 1.0f)
	{
		VectorMA(bolt->server.state.origin, -10, dir, bolt->server.state.origin);
		bolt->touch(bolt, tr.ent, NULL, NULL);
	}
}


/*
=================
fire_grenade
=================
*/
void Grenade_Explode(edict_t *ent)
{
	vec3_t      origin;
	
#ifdef ENABLE_COOP
	PlayerNoise(ent->server.owner, ent->server.state.origin, PNOISE_IMPACT);
#endif

	//FIXME: if we are onground then raise our Z just a bit since we are a point?
	if (ent->enemy)
	{
		float   points;
		vec3_t  v;
		vec3_t  dir;
		VectorAdd(ent->enemy->server.mins, ent->enemy->server.maxs, v);
		VectorMA(ent->enemy->server.state.origin, 0.5f, v, v);
		VectorSubtract(ent->server.state.origin, v, v);
		points = ent->dmg - 0.5f * VectorLength(v);
		VectorSubtract(ent->enemy->server.state.origin, ent->server.state.origin, dir);
		T_Damage(ent->enemy, ent, ent->server.owner, dir, ent->server.state.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, ent->meansOfDeath);
	}

	T_RadiusDamage(ent, ent->server.owner, ent->dmg, ent->enemy, DAMAGE_NONE, ent->dmg_radius, ent->meansOfDeath);
	VectorMA(ent->server.state.origin, -0.02f, ent->velocity, origin);
	MSG_WriteByte(svc_temp_entity);

	if (ent->waterlevel)
	{
		if (ent->groundentity)
			MSG_WriteByte(TE_GRENADE_EXPLOSION_WATER);
		else
			MSG_WriteByte(TE_ROCKET_EXPLOSION_WATER);
	}
	else
	{
		if (ent->groundentity)
			MSG_WriteByte(TE_GRENADE_EXPLOSION);
		else
			MSG_WriteByte(TE_ROCKET_EXPLOSION);
	}

	MSG_WritePos(origin);
	gi.multicast(ent->server.state.origin, MULTICAST_PHS);
	G_FreeEdict(ent);
}

void Grenade_Touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->server.owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}

	if (!other->takedamage)
	{
		if (ent->spawnflags & 1)
		{
			if (random() > 0.5f)
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb1a.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb2a.wav"), 1, ATTN_NORM, 0);
		}
		else
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/grenlb1b.wav"), 1, ATTN_NORM, 0);

		return;
	}

	ent->enemy = other;
	Grenade_Explode(ent);
}

void fire_grenade(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius)
{
	edict_t *grenade;
	vec3_t  dir;
	vec3_t  forward, right, up;
	float   scale;
	vectoangles(aimdir, dir);
	AngleVectors(dir, forward, right, up);
	grenade = G_Spawn();
	VectorCopy(start, grenade->server.state.origin);
	VectorScale(aimdir, speed, grenade->velocity);
	scale = 200 + crandom() * 10.0f;
	VectorMA(grenade->velocity, scale, up, grenade->velocity);
	scale = crandom() * 10.0f;
	VectorMA(grenade->velocity, scale, right, grenade->velocity);
	VectorSet(grenade->avelocity, 300, 300, 300);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->server.clipmask = MASK_SHOT;
	grenade->server.solid = SOLID_BBOX;
	grenade->server.state.effects |= EF_GRENADE;
	VectorClear(grenade->server.mins);
	VectorClear(grenade->server.maxs);
	grenade->server.state.modelindex = gi.modelindex("models/objects/grenade/tris.md2");
	grenade->server.owner = self;
	grenade->touch = Grenade_Touch;
	grenade->nextthink = level.time + timer * 1000.0f;
	grenade->think = Grenade_Explode;
	grenade->dmg = damage;
	grenade->dmg_radius = damage_radius;
	grenade->entitytype = ET_GRENADE;

	if (self->server.client)
		grenade->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->server.client->pers.weapon), grenade, DT_DIRECT);
	else
		grenade->meansOfDeath = MakeWeaponMeansOfDeath(self, ITI_Q2_GRENADE_LAUNCHER, grenade, DT_DIRECT);

	gi.linkentity(grenade);
}

void fire_grenade2(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, gtime_t timer, float damage_radius, bool held)
{
	edict_t *grenade;
	vec3_t  dir;
	vec3_t  forward, right, up;
	float   scale;
	vectoangles(aimdir, dir);
	AngleVectors(dir, forward, right, up);
	grenade = G_Spawn();
	VectorCopy(start, grenade->server.state.origin);
	VectorScale(aimdir, speed, grenade->velocity);
	scale = 200 + crandom() * 10.0f;
	VectorMA(grenade->velocity, scale, up, grenade->velocity);
	scale = crandom() * 10.0f;
	VectorMA(grenade->velocity, scale, right, grenade->velocity);
	VectorSet(grenade->avelocity, 300, 300, 300);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->server.clipmask = MASK_SHOT;
	grenade->server.solid = SOLID_BBOX;
	grenade->server.state.effects |= EF_GRENADE;
	VectorClear(grenade->server.mins);
	VectorClear(grenade->server.maxs);
	grenade->server.state.modelindex = gi.modelindex("models/objects/grenade2/tris.md2");
	grenade->server.owner = self;
	grenade->touch = Grenade_Touch;
	grenade->nextthink = level.time + timer;
	grenade->think = Grenade_Explode;
	grenade->dmg = damage;
	grenade->dmg_radius = damage_radius;
	grenade->entitytype = ET_HAND_GRENADE;

	if (held)
		grenade->spawnflags = 3;
	else
		grenade->spawnflags = 1;

	grenade->server.state.sound = gi.soundindex("weapons/hgrenc1b.wav");

	if (self->server.client)
		grenade->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->server.client->pers.weapon), grenade, DT_DIRECT);
	else
		grenade->meansOfDeath = MakeWeaponMeansOfDeath(self, ITI_Q2_GRENADES, grenade, DT_DIRECT);

	if (held)
		grenade->meansOfDeath.damage_means |= MD_HELD;

	if (timer <= 0.0f)
		Grenade_Explode(grenade);
	else
	{
		gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/hgrent1a.wav"), 1, ATTN_NORM, 0);
		gi.linkentity(grenade);
	}
}


/*
=================
fire_rocket
=================
*/
void rocket_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;
	int         n;

	if (other == ent->server.owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}
	
#ifdef ENABLE_COOP
	PlayerNoise(ent->server.owner, ent->server.state.origin, PNOISE_IMPACT);
#endif

	// calculate position for the explosion entity
	VectorMA(ent->server.state.origin, -0.02f, ent->velocity, origin);

	if (other->takedamage)
		T_Damage(other, ent, ent->server.owner, ent->velocity, ent->server.state.origin, plane->normal, ent->dmg, 0, DAMAGE_NONE, ent->meansOfDeath);
	else
	{
		// don't throw any debris in net games
		if (game.maxclients == 1)
		{
			if ((surf) && !(surf->flags & (SURF_WARP | SURF_TRANS33 | SURF_TRANS66 | SURF_FLOWING)))
			{
				n = Q_rand() % 5;

				while (n--)
					ThrowDebris(ent, "models/objects/debris2/tris.md2", 2, ent->server.state.origin);
			}
		}
	}

	T_RadiusDamage(ent, ent->server.owner, ent->radius_dmg, other, DAMAGE_NONE, ent->dmg_radius, ent->meansOfDeath);
	MSG_WriteByte(svc_temp_entity);

	if (ent->waterlevel)
		MSG_WriteByte(TE_ROCKET_EXPLOSION_WATER);
	else
		MSG_WriteByte(TE_ROCKET_EXPLOSION);

	MSG_WritePos(origin);
	gi.multicast(ent->server.state.origin, MULTICAST_PHS);
	G_FreeEdict(ent);
}

void fire_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage)
{
	edict_t *rocket;
	rocket = G_Spawn();
	VectorCopy(start, rocket->server.state.origin);
	VectorCopy(dir, rocket->movedir);
	vectoangles(dir, rocket->server.state.angles);
	VectorScale(dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->server.clipmask = MASK_SHOT;
	rocket->server.solid = SOLID_BBOX;
	rocket->server.state.effects |= EF_ROCKET;
	VectorClear(rocket->server.mins);
	VectorClear(rocket->server.maxs);
	rocket->server.state.game = GAME_Q2;
	rocket->server.state.modelindex = gi.modelindex("%e_rocket");
	rocket->server.owner = self;
	rocket->touch = rocket_touch;
	rocket->nextthink = level.time + (8000000.0f / speed);
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->radius_dmg = radius_damage;
	rocket->dmg_radius = damage_radius;
	rocket->server.state.sound = gi.soundindex("weapons/rockfly.wav");
	rocket->entitytype = ET_ROCKET;

	if (self->server.client)
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->server.client->pers.weapon), rocket, DT_DIRECT);
	else
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, ITI_Q2_ROCKET_LAUNCHER, rocket, DT_DIRECT);

#if ENABLE_COOP
	check_dodge(self, rocket->server.state.origin, dir, speed);
#endif

	gi.linkentity(rocket);
}


/*
=================
fire_rail
=================
*/
void fire_rail(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick)
{
	vec3_t      from;
	vec3_t      end;
	trace_t     tr;
	edict_t     *ignore;
	int         mask;
	bool        water;
	meansOfDeath_t meansOfDeath;

	if (self->server.client)
		meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->server.client->pers.weapon), self, DT_DIRECT);
	else
		meansOfDeath = MakeWeaponMeansOfDeath(self, ITI_Q2_RAILGUN, self, DT_DIRECT);

	VectorMA(start, 8192, aimdir, end);
	VectorCopy(start, from);
	ignore = self;
	water = false;
	mask = MASK_BULLET | CONTENTS_SLIME | CONTENTS_LAVA;

	while (ignore)
	{
		tr = gi.trace(from, NULL, NULL, end, ignore, mask);

		if (tr.contents & (CONTENTS_SLIME | CONTENTS_LAVA))
		{
			mask &= ~(CONTENTS_SLIME | CONTENTS_LAVA);
			water = true;
		}
		else
		{
			//ZOID--added so rail goes through SOLID_BBOX entities (gibs, etc)
			if (tr.ent->server.flags.monster || tr.ent->server.client || tr.ent->server.solid == SOLID_BBOX)
				ignore = tr.ent;
			else
				ignore = NULL;

			if ((tr.ent != self) && (tr.ent->takedamage))
			{
				T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_NONE, meansOfDeath);
				meansOfDeath.damage_type = DT_INDIRECT;
			}
		}

		VectorCopy(tr.endpos, from);
	}

	// send gun puff / flash
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_RAILTRAIL);
	MSG_WritePos(start);
	MSG_WritePos(tr.endpos);
	gi.multicast(self->server.state.origin, MULTICAST_PHS);

	//  gi.multicast (start, MULTICAST_PHS);
	if (water)
	{
		MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(TE_RAILTRAIL);
		MSG_WritePos(start);
		MSG_WritePos(tr.endpos);
		gi.multicast(tr.endpos, MULTICAST_PHS);
	}
	
#ifdef ENABLE_COOP
	PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
#endif
}


/*
=================
fire_bfg
=================
*/
void bfg_explode(edict_t *self)
{
	edict_t *ent;
	float   points;
	vec3_t  v;
	float   dist;

	if (self->server.state.frame == 0)
	{
		// the BFG effect
		ent = NULL;
		self->meansOfDeath.damage_type = DT_EFFECT;

		while ((ent = findradius(ent, self->server.state.origin, self->dmg_radius)) != NULL)
		{
			if (!ent->takedamage)
				continue;

			if (ent == self->server.owner)
				continue;

			if (!CanDamage(ent, self))
				continue;

			if (!CanDamage(ent, self->server.owner))
				continue;

			VectorAdd(ent->server.mins, ent->server.maxs, v);
			VectorMA(ent->server.state.origin, 0.5f, v, v);
			VectorSubtract(self->server.state.origin, v, v);
			dist = VectorLength(v);
			points = self->radius_dmg * (1.0f - sqrtf(dist / self->dmg_radius));

			if (ent == self->server.owner)
				points = points * 0.5f;

			MSG_WriteByte(svc_temp_entity);
			MSG_WriteByte(TE_BFG_EXPLOSION);
			MSG_WritePos(ent->server.state.origin);
			gi.multicast(ent->server.state.origin, MULTICAST_PHS);
			T_Damage(ent, self, self->server.owner, self->velocity, ent->server.state.origin, vec3_origin, (int)points, 0, DAMAGE_ENERGY, self->meansOfDeath);
		}
	}

	self->nextthink = level.time + game.frametime;
	self->server.state.frame++;

	if (self->server.state.frame == 5)
		self->think = G_FreeEdict;
}

void bfg_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->server.owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}
	
#ifdef ENABLE_COOP
	PlayerNoise(self->server.owner, self->server.state.origin, PNOISE_IMPACT);
#endif

	self->meansOfDeath.damage_type = DT_DIRECT;

	// core explosion - prevents firing it into the wall/floor

	if (other->takedamage)
		T_Damage(other, self, self->server.owner, self->velocity, self->server.state.origin, plane->normal, 200, 0, DAMAGE_NONE, self->meansOfDeath);

	T_RadiusDamage(self, self->server.owner, 200, other, DAMAGE_NONE, 100, self->meansOfDeath);
	gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/bfg__x1b.wav"), 1, ATTN_NORM, 0);
	self->server.solid = SOLID_NOT;
	self->touch = NULL;
	VectorMA(self->server.state.origin, -1 * game.frameseconds, self->velocity, self->server.state.origin);
	VectorClear(self->velocity);
	self->server.state.modelindex = gi.modelindex("sprites/s_bfg3.sp2");
	self->server.state.frame = 0;
	self->server.state.sound = 0;
	self->server.state.effects &= ~EF_ANIM_ALLFAST;
	self->think = bfg_explode;
	self->nextthink = level.time + game.frametime;
	self->enemy = other;
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_BFG_BIGEXPLOSION);
	MSG_WritePos(self->server.state.origin);
	gi.multicast(self->server.state.origin, MULTICAST_PVS);
}

void bfg_think(edict_t *self)
{
	edict_t *ent;
	edict_t *ignore;
	vec3_t  point;
	vec3_t  dir;
	vec3_t  start;
	vec3_t  end;
	int     dmg = 5;
	trace_t tr;

#ifdef ENABLE_COOP
	if (!deathmatch->value)
		dmg = 10;
#endif

	ent = NULL;

	while ((ent = findradius(ent, self->server.state.origin, 256)) != NULL)
	{
		if (ent == self)
			continue;

		if (ent == self->server.owner)
			continue;

		if (!ent->takedamage)
			continue;

		if (!ent->server.flags.monster && !ent->server.client && ent->entitytype != ET_MISC_EXPLOBOX)
			continue;

		VectorMA(ent->server.absmin, 0.5f, ent->server.size, point);
		VectorSubtract(point, self->server.state.origin, dir);
		VectorNormalize(dir);
		ignore = self;
		VectorCopy(self->server.state.origin, start);
		VectorMA(start, 2048, dir, end);

		while (1)
		{
			tr = gi.trace(start, NULL, NULL, end, ignore, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_DEADMONSTER);

			if (!tr.ent)
				break;

			// hurt it if we can
			if ((tr.ent->takedamage) && !(tr.ent->flags & FL_IMMUNE_LASER) && (tr.ent != self->server.owner))
				T_Damage(tr.ent, self, self->server.owner, dir, tr.endpos, vec3_origin, dmg, 1, DAMAGE_ENERGY, self->meansOfDeath);

			// if we hit something that's not a monster or player we're done
			if (!tr.ent->server.flags.monster && !tr.ent->server.client)
			{
				MSG_WriteByte(svc_temp_entity);
				MSG_WriteByte(TE_LASER_SPARKS);
				MSG_WriteByte(4);
				MSG_WritePos(tr.endpos);
				MSG_WriteDir(tr.plane.normal);
				MSG_WriteByte(self->server.state.skinnum);
				gi.multicast(tr.endpos, MULTICAST_PVS);
				break;
			}

			ignore = tr.ent;
			VectorCopy(tr.endpos, start);
		}

		MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(TE_BFG_LASER);
		MSG_WritePos(self->server.state.origin);
		MSG_WritePos(tr.endpos);
		gi.multicast(self->server.state.origin, MULTICAST_PHS);
	}

	self->nextthink = level.time + 100;
}

void fire_bfg(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius)
{
	edict_t *bfg;
	bfg = G_Spawn();
	VectorCopy(start, bfg->server.state.origin);
	VectorCopy(dir, bfg->movedir);
	vectoangles(dir, bfg->server.state.angles);
	VectorScale(dir, speed, bfg->velocity);
	bfg->movetype = MOVETYPE_FLYMISSILE;
	bfg->server.clipmask = MASK_SHOT;
	bfg->server.solid = SOLID_BBOX;
	bfg->server.state.effects |= EF_BFG | EF_ANIM_ALLFAST;
	VectorClear(bfg->server.mins);
	VectorClear(bfg->server.maxs);
	bfg->server.state.modelindex = gi.modelindex("sprites/s_bfg1.sp2");
	bfg->server.owner = self;
	bfg->touch = bfg_touch;
	bfg->nextthink = level.time + (8000000.0f / speed);
	bfg->think = G_FreeEdict;
	bfg->radius_dmg = damage;
	bfg->dmg_radius = damage_radius;
	bfg->server.state.sound = gi.soundindex("weapons/bfg__l1a.wav");
	bfg->think = bfg_think;
	bfg->nextthink = level.time + 1;
	bfg->teammaster = bfg;
	bfg->teamchain = NULL;

	if (self->server.client)
		bfg->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->server.client->pers.weapon), bfg, DT_LASER);
	else
		bfg->meansOfDeath = MakeWeaponMeansOfDeath(self, ITI_Q2_BFG10K, bfg, DT_LASER);

#if ENABLE_COOP
	check_dodge(self, bfg->server.state.origin, dir, speed);
#endif

	gi.linkentity(bfg);
}

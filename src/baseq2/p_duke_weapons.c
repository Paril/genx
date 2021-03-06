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
// p_doom_weapons.c

#include "g_local.h"

void Weapon_Doom_Anim_Regular(edict_t *ent);

void fire_doom_bullet(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, meansOfDeath_t mod);
void fire_doom_shotgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int count, int hspread, int vspread, meansOfDeath_t mod);

extern bool     is_quad;

#define DUKE_ANIM_TIME 400
#define DUKE_ANIM_FRAC DUKE_ANIM_TIME

q_modelhandle pipe_detonator_index, foot_index;

extern bool do_propagate;

#define DUKE_DISTANCE_SCALE		4.5

#define DukeDistanceSquared(v1,v2) \
	(((v1)[0]-(v2)[0])*((v1)[0]-(v2)[0])+ \
		((v1)[1]-(v2)[1])*((v1)[1]-(v2)[1])+ \
		(((v1)[2]-(v2)[2])*((v1)[2]-(v2)[2]))*2.0)
#define DukeDistance(v1,v2) (sqrtf(DukeDistanceSquared(v1,v2)))

/*
=================
findradius

Returns entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
static edict_t *duke_findradius(edict_t *from, vec3_t org, float rad, float *found_rad)
{
	if (!from)
		from = g_edicts;
	else
		from++;

	for (; from < &g_edicts[globals.pool.num_edicts]; from++)
	{
		if (!from->server.inuse)
			continue;

		if (from->server.solid == SOLID_NOT)
			continue;

		if ((*found_rad = DukeDistance(from->server.state.origin, org)) > rad)
			continue;

		return from;
	}

	return NULL;
}

void T_DukeRadiusDamage(edict_t *inflictor, edict_t *attacker, edict_t *ignore, float radius, long hp1, long hp2, long hp3, long hp4, int dflags, meansOfDeath_t mod)
{
	radius = sqrtf(radius) * DUKE_DISTANCE_SCALE;
	long   points;
	edict_t *ent = NULL;
	vec3_t  dir;
	float	d;

	if (dflags & DAMAGE_Q1)
		radius += 40;

	mod.damage_type = DT_INDIRECT;

	while ((ent = duke_findradius(ent, inflictor->server.state.origin, radius, &d)) != NULL)
	{
		if (ent == ignore)
			continue;

		if (!ent->takedamage)
			continue;

		if (d < radius / 3)
		{
			if (hp4 == hp3)
				hp4++;

			points = hp3 + (Q_rand() % (hp4 - hp3));
		}
		else if (d < radius / 1.5)
		{
			if (hp3 == hp2)
				hp3++;

			points = hp2 + (Q_rand() % (hp3 - hp2));
		}
		else// if ( d < radius )
		{
			if (hp2 == hp1)
				hp2++;

			points = hp1 + (Q_rand() % (hp2 - hp1));
		}

		if (points > 0)
		{
			if (CanDamage(ent, inflictor))
			{
				VectorSubtract(ent->server.state.origin, inflictor->server.state.origin, dir);
				
#ifdef ENABLE_COOP
				// shambler takes half damage
				if (ent->entitytype == ET_Q1_MONSTER_SHAMBLER)
					points *= 0.5f;
#endif

				T_Damage(ent, inflictor, attacker, dir, inflictor->server.state.origin, vec3_origin, (int)points, (int)points, dflags | DAMAGE_RADIUS, mod);
			}
		}
	}
}

#define PIPE_RADIUS 2500
#define PIPE_STRENGTH 140

void Pipe_Explode(edict_t *ent)
{
	ent->takedamage = false;
	vec3_t      origin;
	
#ifdef ENABLE_COOP
	PlayerNoise(ent->activator, ent->server.state.origin, PNOISE_IMPACT);
#endif

	ent->server.owner = ent->activator;
	T_DukeRadiusDamage(ent, ent->activator, ent, PIPE_RADIUS, PIPE_STRENGTH >> 2, PIPE_STRENGTH - (PIPE_STRENGTH >> 1), PIPE_STRENGTH - (PIPE_STRENGTH >> 2), PIPE_STRENGTH, DAMAGE_NO_PARTICLES | DAMAGE_DUKE, ent->meansOfDeath);
	VectorMA(ent->server.state.origin, -0.02, ent->velocity, origin);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_DUKE_EXPLODE_PIPE);
	MSG_WritePos(origin);
	gi.multicast(ent->server.state.origin, MULTICAST_PHS);
	G_FreeEdict(ent->chain);
	G_FreeEdict(ent);
}

void Pipe_Touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent->chain);
		G_FreeEdict(ent);
		return;
	}

	if (VectorLengthSquared(ent->velocity))
	{
		if (other != ent->activator)
			gi.sound(ent, CHAN_VOICE, gi.soundindex("duke/PBOMBBNC.wav"), 1, ATTN_NORM, 0);
	}
}

void PipeTrigger_Touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent)
		return;

	if (other->server.client && level.time > ent->chain->last_move_time)
	{
		edict_t *pipe = ent->chain;
		do_propagate = false;
		Touch_Item(pipe, other, plane, surf);
		do_propagate = true;

		if (!pipe->server.inuse)
			G_FreeEdict(ent);
	}
}

void Pipe_Think(edict_t *self)
{
	VectorCopy(self->server.state.origin, self->chain->server.state.origin);
	gi.linkentity(self->chain);
	self->nextthink = level.time + game.frametime;

	if (VectorLengthSquared(self->velocity))
		self->server.state.frame = (self->server.state.frame + 1) % 2;

	if (!self->activator || !self->activator->server.client || !self->activator->server.inuse || !self->activator->server.client->pers.connected)
		return;

	if (GetIndexByItem(self->activator->server.client->pers.weapon) == ITI_DUKE_PIPEBOMBS &&
		self->activator->server.client->pers.alt_weapon == true &&
		self->activator->server.client->server.ps.guns[GUN_MAIN].frame == 1)
		Pipe_Explode(self);
}

void pipe_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (attacker->server.client || attacker->server.flags.monster)
		self->activator = attacker;

	Pipe_Explode(self);
}

void fire_pipe(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float damage_radius)
{
	edict_t *grenade;
	vec3_t  dir;
	vec3_t  forward, right, up;
	vectoangles(aimdir, dir);
	AngleVectors(dir, forward, right, up);
	grenade = G_Spawn();
	VectorCopy(start, grenade->server.state.origin);
	VectorScale(aimdir, speed, grenade->velocity);
	VectorMA(grenade->velocity, 250, up, grenade->velocity);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->server.clipmask = CONTENTS_SOLID | CONTENTS_BULLETS;
	grenade->server.state.clip_contents = CONTENTS_BULLETS;
	grenade->server.solid = SOLID_BBOX;
	grenade->health = 1;
	grenade->takedamage = true;
	grenade->die = pipe_die;
	grenade->server.state.game = GAME_DUKE;
	VectorSet(grenade->server.mins, -8, -8, -4);
	VectorSet(grenade->server.maxs, 8, 8, 0);
	grenade->spawnflags |= DROPPED_ITEM | DROPPED_PLAYER_ITEM;
	grenade->item = grenade->real_item = GetItemByIndex(ITI_DUKE_PIPEBOMBS);
	grenade->count = 1;
	grenade->server.state.modelindex = gi.modelindex("%e_pipebomb");
	grenade->activator = self;
	grenade->touch = Pipe_Touch;
	grenade->nextthink = level.time + game.frametime;
	grenade->think = Pipe_Think;
	grenade->dmg = damage;
	grenade->dmg_radius = damage_radius;
	grenade->entitytype = ET_GRENADE;
	grenade->last_move_time = level.time + 500;

	if (self->server.client)
		grenade->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->server.client->pers.weapon), grenade, DT_DIRECT);
	else
		grenade->meansOfDeath = MakeWeaponMeansOfDeath(self, ITI_Q2_GRENADE_LAUNCHER, grenade, DT_DIRECT);

	gi.linkentity(grenade);
	edict_t *trigger;
	trigger = G_Spawn();
	trigger->server.clipmask = 0;
	trigger->server.state.clip_contents = CONTENTS_PLAYERCLIP;
	trigger->server.solid = SOLID_TRIGGER;
	VectorCopy(grenade->server.mins, trigger->server.mins);
	VectorCopy(grenade->server.maxs, trigger->server.maxs);
	trigger->chain = grenade;
	grenade->chain = trigger;
	trigger->server.flags.noclient = true;
	trigger->touch = PipeTrigger_Touch;
	VectorCopy(grenade->server.state.origin, trigger->server.state.origin);
	gi.linkentity(trigger);
}

void weapon_duke_pipebomb_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	if (gun->frame == 3)
	{
		if (ent->server.client->buttons & BUTTON_ATTACK)
		{
			if (ent->server.client->pers.pipebomb_hold < 10)
				ent->server.client->pers.pipebomb_hold++;

			return;
		}

		if (HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon))
		{
			RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

			vec3_t start, forward, right, offset;
			AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
			VectorSet(offset, 0, 0, ent->viewheight - 8);
			P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
			fire_pipe(ent, start, forward, 150, 250 + (ent->server.client->pers.pipebomb_hold * 25), 450);
		}
	}

	ent->server.client->pers.pipebomb_hold = 0;
	gun->frame++;

	if (gun->frame >= 9)
	{
		ent->server.client->pers.alt_weapon = true;
		gun->offset[2] = DUKE_ANIM_FRAC;
		gun->index = pipe_detonator_index;
		gun->frame = 0;
		gun_state->weaponstate = WEAPON_ACTIVATING;
		ent->last_move_time = 0;
	}
}

void weapon_duke_detonator_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	gun->frame++;

	if (gun->frame >= 3)
	{
		if (HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon))
			gun_state->newweapon = ent->server.client->pers.weapon;
		else
			NoAmmoWeaponChange(ent, gun_state);
	}
}

void duke_rocket_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

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

	if (other->takedamage)
		T_Damage(other, ent, ent->server.owner, ent->velocity, ent->server.state.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);

	// calculate position for the explosion entity
#define RPG_RADIUS 1780
#define RPG_STRENGTH 140
#define DEVASTATOR_STRENGTH 20

	if (ent->spawnflags == 0)
		T_DukeRadiusDamage(ent, ent->server.owner, ent, RPG_RADIUS, RPG_STRENGTH >> 2, RPG_STRENGTH >> 1, RPG_STRENGTH - (RPG_STRENGTH >> 2), RPG_STRENGTH, DAMAGE_NO_PARTICLES | DAMAGE_DUKE, ent->meansOfDeath);
	else
		T_DukeRadiusDamage(ent, ent->server.owner, ent, (RPG_RADIUS >> 1), DEVASTATOR_STRENGTH >> 2, DEVASTATOR_STRENGTH >> 1, DEVASTATOR_STRENGTH - (DEVASTATOR_STRENGTH >> 2), DEVASTATOR_STRENGTH, DAMAGE_NO_PARTICLES | DAMAGE_DUKE, ent->meansOfDeath);

	VectorNormalize(ent->velocity);
	VectorMA(ent->server.state.origin, -8, ent->velocity, origin);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(ent->spawnflags == 1 ? TE_DUKE_EXPLODE_SMALL : TE_DUKE_EXPLODE);
	MSG_WritePos(origin);
	gi.multicast(ent->server.state.origin, MULTICAST_PHS);
	G_FreeEdict(ent);
}

void fire_duke_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int radius_damage, int speed, bool tiny)
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
	rocket->server.state.game = GAME_DUKE;
	VectorClear(rocket->server.mins);
	VectorClear(rocket->server.maxs);
	rocket->server.state.modelindex = tiny ? gi.modelindex("%e_smallrocket") : gi.modelindex("%e_rocket");
	rocket->server.owner = self;
	rocket->touch = duke_rocket_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->radius_dmg = radius_damage;
	rocket->server.state.frame = 3;
	rocket->entitytype = ET_ROCKET;

	if (tiny)
	{
		rocket->spawnflags = 1;
		rocket->dmg_radius = radius_damage + 120;
	}
	else
		rocket->dmg_radius = radius_damage + 400;

#if ENABLE_COOP
	check_dodge(self, rocket->server.state.origin, dir, speed);
#endif

	if (self->server.client)
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->server.client->pers.weapon), rocket, DT_DIRECT);
	else
		rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, rocket, MD_NONE, DT_DIRECT);

	gi.linkentity(rocket);
}

void weapon_duke_devastate_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	gun->frame++;

	if (gun->frame == 3 && (!(ent->server.client->buttons & BUTTON_ATTACK) || !HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon) || gun_state->newweapon))
	{
		gun->frame = 6;
		return;
	}
	else if (gun->frame >= 6 && (ent->server.client->buttons & BUTTON_ATTACK) && HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon) && !gun_state->newweapon)
		gun->frame = 1;

	if (gun->frame == 1 || gun->frame == 3)
	{
		G_SendMuzzleFlash(ent, MZ_GRENADE);

		vec3_t start, forward, right, offset;
		vec3_t angles;

		for (int i = 0; i < 2; ++i)
		{
			if (!HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon))
				break;

			VectorCopy(ent->server.client->v_angle, angles);
			angles[0] += crandom() * 2;
			angles[1] += crandom() * 2;
			AngleVectors(angles, forward, right, NULL);
			VectorSet(offset, 0, gun->frame == 1 ? 8 : -8, ent->viewheight - 8);
			P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
			fire_duke_rocket(ent, start, forward, 38, 50, 850 - i * 50, true);

			RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);
		}
	}
}

void weapon_duke_rocket_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	gun->frame++;

	if (gun->frame == 1)
	{
		G_SendMuzzleFlash(ent, MZ_ROCKET);

		RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

		vec3_t start, forward, right, offset;
		AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 8);
		P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
		fire_duke_rocket(ent, start, forward, 140, sqrtf(1780) * 2, 850, false);
		Weapon_Doom_Anim_Regular(ent);
	}
}

void weapon_duke_foot_fire_generic(edict_t *ent, player_gun_t *gun)
{
	gun->frame++;

	if (gun->frame >= 3 && gun->frame <= 5)
	{
		ent->server.client->anim_priority = ANIM_ATTACK;
		ent->server.state.frame = 6;
		ent->server.client->anim_end = 7;
	}
	else
	{
		ent->server.client->anim_priority = ANIM_ATTACK;
		ent->server.state.frame = 5;
		ent->server.client->anim_end = 6;
	}

	if (gun->frame == 3)
	{
		vec3_t source, org;
		vec3_t forward;
		AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
		VectorCopy(ent->server.state.origin, source);
		source[2] += ent->viewheight - 8;
		VectorMA(source, 64, forward, org);
		trace_t tr = gi.trace(source, vec3_origin, vec3_origin, org, ent, MASK_SHOT);

		if (tr.fraction != 1.0f)
		{
			VectorMA(tr.endpos, -4, forward, org);

			if (tr.ent->takedamage)
			{
				MSG_WriteByte(svc_temp_entity);
				MSG_WriteByte(TE_DUKE_BLOOD);
				MSG_WritePos(org);
				gi.multicast(org, MULTICAST_PVS);
				int damage = (ent->server.client->quad_time > level.time) ? 42 : 12;
				vec3_t dir;
				VectorSubtract(tr.ent->server.state.origin, ent->server.state.origin, dir);
				vectoangles(dir, dir);
				dir[0] = 0;
				dir[2] = 0;
				AngleVectors(dir, dir, NULL, NULL);
				T_Damage(tr.ent, ent, ent, dir, vec3_origin, vec3_origin, damage, damage, DAMAGE_NO_PARTICLES, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), ent, DT_DIRECT));
			}

			MSG_WriteByte(svc_temp_entity);
			MSG_WriteByte(TE_DUKE_GUNSHOT);
			MSG_WritePos(org);
			gi.multicast(org, MULTICAST_PVS);
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("duke/KICKHIT.wav"), 1, ATTN_NORM, 0);
		}
	}
}

void weapon_duke_foot_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	if (gun->frame == 5 && (ent->server.client->buttons & BUTTON_ATTACK) && !gun_state->newweapon)
		gun->frame = 0;

	weapon_duke_foot_fire_generic(ent, gun);
}

void weapon_duke_offfoot_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	weapon_duke_foot_fire_generic(ent, gun);

	if (gun->frame == 5)
	{
		gun->index = 0;
		gun->frame = 0;
	}
}

static void Weapon_Duke_Offhand_Foot(edict_t *ent)
{
	if (ent->deadflag || !ent->server.solid || ent->freeze_time > level.time)
		return;

	player_gun_t *gun = &ent->server.client->server.ps.guns[GUN_OFFHAND];
	gun_state_t *gun_state = &ent->server.client->gunstates[GUN_OFFHAND];

	if (gun->index != foot_index)
	{
		vec3_t source, org;
		vec3_t forward;
		AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
		VectorCopy(ent->server.state.origin, source);
		source[2] += ent->viewheight - 8;
		VectorMA(source, 64, forward, org);
		trace_t tr = gi.trace(source, vec3_origin, vec3_origin, org, ent, MASK_SHOT);

		if (tr.fraction != 1.0f)
		{
			VectorMA(tr.endpos, -4, forward, org);

			if (tr.ent->takedamage && tr.ent->freeze_time > level.time)
			{
				gun->index = foot_index;
				gun->frame = 0;
				gun->offset[1] = 0;
				gun_state->weaponstate = WEAPON_FIRING;
			}
		}
	}

	if (gun->index == foot_index)
	{
		Weapon_Doom(ent, 7, weapon_duke_offfoot_fire, gun, gun_state);
		gun->offset[1] = 0;
	}
}

void duke_freezer_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}

	vec3_t vel;
	VectorNormalize2(ent->velocity, vel);
	VectorMA(ent->server.state.origin, -8, vel, origin);

	if (other->takedamage)
	{
		bool freeze_fx = false;
		ent->server.owner = ent->activator;

		if (other->freeze_time <= level.time)
		{
			if (other->health > ent->dmg)
			{
				T_Damage(other, ent, ent->server.owner, ent->velocity, ent->server.state.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);
				gi.sound(ent, CHAN_VOICE, gi.soundindex("duke/BULITHIT.wav"), 1, ATTN_NORM, 0);
			}
			else
			{
				if (other->server.client || other->server.flags.monster)
				{
					other->freeze_time = level.time + 5000;
					other->health = 1;
				}
				else
					T_Damage(other, ent, ent->server.owner, ent->velocity, ent->server.state.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);

				gi.sound(ent, CHAN_VOICE, gi.soundindex("duke/FREEZE.wav"), 1, ATTN_NORM, 0);
				freeze_fx = true;
			}
		}
		else
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("duke/BULITHIT.wav"), 1, ATTN_NORM, 0);
			freeze_fx = true;
		}

		if (freeze_fx)
		{
			MSG_WriteByte(svc_temp_entity);
			MSG_WriteByte(TE_DUKE_FREEZE_HIT);
			MSG_WritePos(origin);
			gi.multicast(ent->server.state.origin, MULTICAST_PHS);
		}

		G_FreeEdict(ent);
		return;
	}

	ent->server.owner = NULL;
	float dot = DotProduct(vel, plane->normal);

	if (!ent->count || (ent->count <= 2 && dot > -0.4f))
	{
		MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(TE_DUKE_FREEZE_HIT);
		MSG_WritePos(origin);
		gi.multicast(ent->server.state.origin, MULTICAST_PHS);
		G_FreeEdict(ent);
	}
	else
	{
		--ent->count;
		ent->dmg /= 2;
	}
}

void fire_duke_freezer(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t *rocket;
	rocket = G_Spawn();
	VectorCopy(start, rocket->server.state.origin);
	VectorCopy(dir, rocket->movedir);
	vectoangles(dir, rocket->server.state.angles);
	VectorScale(dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYBOUNCE;
	rocket->server.clipmask = MASK_SHOT;
	rocket->server.solid = SOLID_BBOX;
	rocket->server.state.effects |= EF_ANIM_ALLFAST;
	rocket->server.state.renderfx |= RF_FULLBRIGHT | RF_ANIM_BOUNCE;
	rocket->server.state.game = GAME_DOOM;
	VectorClear(rocket->server.mins);
	VectorClear(rocket->server.maxs);
	rocket->server.state.modelindex = gi.modelindex("%e_freezer");
	rocket->server.owner = rocket->activator = self;
	rocket->touch = duke_freezer_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = (damage + (Q_rand_uniform(4) + 1));
	rocket->count = 3;
	rocket->entitytype = ET_DOOM_PLASMA;

	if (self->server.client)
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->server.client->pers.weapon), rocket, DT_DIRECT);
	else
		rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, rocket, MD_NONE, DT_DIRECT);

#if ENABLE_COOP
	check_dodge(self, rocket->server.state.origin, dir, speed);
#endif

	gi.linkentity(rocket);
}

void weapon_duke_freezer_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	if (!HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon) || gun_state->newweapon || !((ent->server.client->buttons | ent->server.client->latched_buttons) & BUTTON_ATTACK))
	{
		gun->frame = 5;
		return;
	}

	gun->frame++;

	if (gun->frame >= 4)
		gun->frame = 1;

	G_SendMuzzleFlash(ent, MZ_BFG);

	vec3_t start, forward, right, offset;
	AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
	VectorSet(offset, 0, 0, ent->viewheight - 8);
	P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
	fire_duke_freezer(ent, start, forward, 20, 900);

	RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);
}

void weapon_duke_pistol_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	if (gun->frame == 0 || gun->frame == 2)
	{
		if ((ent->server.client->buttons & BUTTON_ATTACK) && HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon) && !gun_state->newweapon && ent->server.client->pers.pistol_clip)
			gun->frame = 1;
		else
			gun->frame = 21;
	}
	else
		gun->frame++;

	if (gun->frame == 4)
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("duke/CLIPOUT.wav"), 1, ATTN_NORM, 0);
		gi.sound(ent, CHAN_AUTO, gi.soundindex("duke/CLIPIN.wav"), 1, ATTN_NORM, 0);
	}
	else if (gun->frame == 7)
		ent->server.client->pers.pistol_clip = 12;
	else if (gun->frame == 13)
		gun->frame = 21;
	else if (gun->frame == 15)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("duke/KNUCKLE.wav"), 1, ATTN_NORM, 0);
	else if (gun->frame == 1)
	{
		G_SendMuzzleFlash(ent, MZ_BLASTER);

		RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

		vec3_t start, forward, right, offset;
		AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
		--ent->server.client->pers.pistol_clip;
		fire_doom_bullet(ent, start, forward, 6, 0, TE_DUKE_GUNSHOT, 250, 250, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), ent, DT_DIRECT));
	}
}

void weapon_duke_chaingun_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	if (!(ent->server.client->buttons & BUTTON_ATTACK) || !HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon) || gun_state->newweapon)
	{
		gun->frame = 5;
		return;
	}

	G_SendMuzzleFlash(ent, MZ_CHAINGUN1);

	RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

	vec3_t start, forward, right, offset;
	AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
	VectorSet(offset, 0, 0, ent->viewheight - 2);
	P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
	fire_doom_bullet(ent, start, forward, 9, 0, TE_DUKE_GUNSHOT, 300, 300, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), ent, DT_DIRECT));

	if (++gun->frame == 5)
		gun->frame = 2;

	//ent->attack_finished_time = level.time + DOOM_TIC_TO_TIME(4);
	//Weapon_Doom_Anim_Fast(ent);
}

void weapon_duke_shotgun_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	gun->frame++;

	if (gun->frame == 5)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("duke/SHOTGNCK.wav"), 1, ATTN_NORM, 0);
	else if (gun->frame == 7)
		G_SendMuzzleFlash(ent, MZ_SSHOTGUN);

	if (gun->frame == 1)
	{
		G_SendMuzzleFlash(ent, MZ_SHOTGUN);

		RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

		vec3_t start, forward, right, offset;
		AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
		fire_doom_shotgun(ent, start, forward, 7, 0, TE_DUKE_GUNSHOT, 7, 250, 250, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), ent, DT_DIRECT));
		Weapon_Doom_Anim_Regular(ent);
	}
}

#define TRIP_RADIUS 3880
#define TRIP_STRENGTH 100

void tripwire_laser_explode(edict_t *laser)
{
	laser->deadflag = laser->goalentity->deadflag = DEAD_DEAD;
	T_DukeRadiusDamage(laser, laser->activator, laser, TRIP_RADIUS, TRIP_STRENGTH >> 2, TRIP_STRENGTH >> 1, TRIP_STRENGTH - (TRIP_STRENGTH >> 2), TRIP_STRENGTH, DAMAGE_NO_PARTICLES | DAMAGE_DUKE, laser->goalentity->meansOfDeath);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_DUKE_EXPLODE_PIPE);
	MSG_WritePos(laser->server.state.origin);
	gi.multicast(laser->server.state.origin, MULTICAST_PHS);
	G_FreeEdict(laser->goalentity);
	G_FreeEdict(laser);
}

void tripwire_laser_think(edict_t *laser)
{
	trace_t tr;
	tr = gi.trace(laser->server.state.origin, NULL, NULL, laser->server.state.old_origin, NULL, CONTENTS_MONSTER);

	if (tr.fraction < 1.0 || tr.startsolid)
	{
		laser->activator = tr.ent;
		gi.sound(laser, CHAN_AUTO, gi.soundindex("duke/LSRBMBWN.wav"), 1, ATTN_NORM, 0);
		laser->think = tripwire_laser_explode;
		laser->nextthink = level.time + 400;
		return;
	}

	laser->nextthink = level.time + 1;
}

void tripwire_activate(edict_t *ent)
{
	edict_t *laser = G_Spawn();
	VectorCopy(ent->server.state.origin, laser->server.state.origin);
	vec3_t up;
	AngleVectors(ent->server.state.angles, NULL, NULL, up);
	VectorMA(laser->server.state.origin, 1.25f, up, laser->server.state.origin);
	VectorMA(ent->server.state.origin, 8192, ent->movedir, laser->server.state.old_origin);
	trace_t tr;
	tr = gi.trace(ent->server.state.origin, NULL, NULL, laser->server.state.old_origin, NULL, CONTENTS_SOLID);
	laser->server.state.game = GAME_DUKE;
	laser->server.state.modelindex = (q_modelhandle)1;         // must be non-zero
	laser->server.state.renderfx |= RF_BEAM;
	laser->server.state.skinnum = 0xF0F0F0F0;
	laser->server.state.frame = 2;
	gi.linkentity(laser);
	VectorCopy(tr.endpos, laser->server.state.old_origin);
	gi.linkentity(laser);
	ent->goalentity = laser;
	laser->goalentity = ent;
	laser->think = tripwire_laser_think;
	laser->nextthink = level.time + 1;
}

void tripwire_die(edict_t *ent, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (ent->deadflag)
		return;

	if (!ent->goalentity)
		tripwire_activate(ent);

	ent->goalentity->activator = attacker;
	tripwire_laser_explode(ent->goalentity);
}

void weapon_duke_tripwire_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	if (gun->frame == 0)
	{
		trace_t tr;
		vec3_t start, forward, right, offset;
		AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
		VectorMA(start, 48, forward, offset);
		tr = gi.trace(start, NULL, NULL, offset, NULL, CONTENTS_SOLID);

		if (tr.fraction == 1.0)
		{
			gun_state->weaponstate = WEAPON_READY;
			gun->frame = 0;
			return;
		}

		gi.sound(ent, CHAN_AUTO, gi.soundindex("duke/LSRBMBPT.wav"), 1, ATTN_IDLE, 0);

		RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

		edict_t *trip = G_Spawn();
		vectoangles2(tr.plane.normal, trip->server.state.angles);
		VectorCopy(tr.endpos, trip->server.state.origin);
		VectorMA(trip->server.state.origin, 0.5, tr.plane.normal, trip->server.state.origin);
		VectorCopy(tr.plane.normal, trip->movedir);
		trip->server.state.modelindex = gi.modelindex("%e_tripwire");
		trip->think = tripwire_activate;
		trip->nextthink = level.time + 1500;
		trip->server.owner = ent;
		trip->meansOfDeath = MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), trip, DT_DIRECT);
		trip->health = 1;
		trip->server.solid = SOLID_BBOX;
		trip->takedamage = true;
		trip->die = tripwire_die;
		gi.linkentity(trip);
	}

	gun->frame++;
}

// only Duke uses this atm
void Weapons_Init()
{
	pipe_detonator_index = gi.modelindex("%wv_detonator");
	foot_index = gi.modelindex("weaponscripts/duke/v_off_foot.wsc");
}

static void Weapon_Duke_Foot(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Doom(ent, 7, weapon_duke_foot_fire, gun, gun_state);
}

static void Weapon_Duke_Pistol(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	if (gun_state->weaponstate == WEAPON_READY && !ent->server.client->pers.pistol_clip &&
		HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon) && gun->frame < 3)
	{
		gun_state->weaponstate = WEAPON_FIRING;
		gun->frame = 3;
	}

	Weapon_Doom(ent, 21, weapon_duke_pistol_fire, gun, gun_state);

	if (gun->frame > 3)
		gun->offset[1] = 0;
}

static void Weapon_Duke_Shotgun(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Doom(ent, 10, weapon_duke_shotgun_fire, gun, gun_state);
}

static void Weapon_Duke_Cannon(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Doom(ent, 5, weapon_duke_chaingun_fire, gun, gun_state);
}

static void Weapon_Duke_RPG(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Doom(ent, 8, weapon_duke_rocket_fire, gun, gun_state);
}

static void Weapon_Duke_Pipebombs(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	if (ent->server.client->pers.alt_weapon)
		Weapon_Doom(ent, 3, weapon_duke_detonator_fire, gun, gun_state);
	else
		Weapon_Doom(ent, 9, weapon_duke_pipebomb_fire, gun, gun_state);
}

static void Weapon_Duke_Devastator(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Doom(ent, 5, weapon_duke_devastate_fire, gun, gun_state);
}

static void Weapon_Duke_Freezer(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Doom(ent, 4, weapon_duke_freezer_fire, gun, gun_state);
}

static void Weapon_Duke_Tripwire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Doom(ent, 7, weapon_duke_tripwire_fire, gun, gun_state);
}

static G_WeaponRunFunc duke_weapon_run_funcs[] =
{
	[ITI_DUKE_FOOT - ITI_WEAPONS_START] = Weapon_Duke_Foot,
	[ITI_DUKE_PISTOL - ITI_WEAPONS_START] = Weapon_Duke_Pistol,
	[ITI_DUKE_SHOTGUN - ITI_WEAPONS_START] = Weapon_Duke_Shotgun,
	[ITI_DUKE_CANNON - ITI_WEAPONS_START] = Weapon_Duke_Cannon,
	[ITI_DUKE_RPG - ITI_WEAPONS_START] = Weapon_Duke_RPG,
	[ITI_DUKE_PIPEBOMBS - ITI_WEAPONS_START] = Weapon_Duke_Pipebombs,
	[ITI_DUKE_DEVASTATOR - ITI_WEAPONS_START] = Weapon_Duke_Devastator,
	[ITI_DUKE_TRIPWIRES - ITI_WEAPONS_START] = Weapon_Duke_Tripwire,
	[ITI_DUKE_FREEZER - ITI_WEAPONS_START] = Weapon_Duke_Freezer
};

void Weapon_Duke_Run(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	duke_weapon_run_funcs[ITEM_INDEX(ent->server.client->pers.weapon) - ITI_WEAPONS_START](ent, gun, gun_state);
	Weapon_Duke_Offhand_Foot(ent);
}
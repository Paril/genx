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

void Weapon_Doom(edict_t *ent, int num_frames, void(*fire) (edict_t *ent, gunindex_e gun, bool first), gunindex_e gun);
void Weapon_Doom_Anim_Regular(edict_t *ent);

void fire_doom_bullet(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, meansOfDeath_t mod);
void fire_doom_shotgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int count, int hspread, int vspread, meansOfDeath_t mod);

extern bool     is_quad;
extern byte     is_silenced;

const gtime_t DUKE_ANIM_TIME = 400;
const gtime_t DUKE_ANIM_FRAC = DUKE_ANIM_TIME;

int pipe_detonator_index, pipe_bomb_index, foot_index;

extern bool do_propagate;

void Pipe_Explode(edict_t *ent)
{
	ent->takedamage = DAMAGE_NO;

	vec3_t      origin;

	if (ent->activator->client)
		PlayerNoise(ent->activator, ent->s.origin, PNOISE_IMPACT);

	ent->owner = ent->activator;
	T_RadiusDamage(ent, ent->activator, ent->dmg, ent, DAMAGE_NO_PARTICLES | DAMAGE_DUKE, ent->dmg_radius, ent->meansOfDeath);

	VectorMA(ent->s.origin, -0.02, ent->velocity, origin);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DUKE_EXPLODE_PIPE);
	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);

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

	if (other->client && level.time > ent->chain->last_move_time)
	{
		edict_t *pipe = ent->chain;

		do_propagate = false;
		Touch_Item(pipe, other, plane, surf);
		do_propagate = true;

		if (!pipe->inuse)
			G_FreeEdict(ent);
	}
}

void Pipe_Think(edict_t *self)
{
	VectorCopy(self->s.origin, self->chain->s.origin);
	gi.linkentity(self->chain);

	self->nextthink = level.time + game.frametime;

	if (VectorLengthSquared(self->velocity))
		self->s.frame = (self->s.frame + 1) % 2;

	if (!self->activator || !self->activator->client || !self->activator->inuse || !self->activator->client->pers.connected)
		return;

	if (GetIndexByItem(self->activator->client->pers.weapon) == ITI_DUKE_PIPEBOMBS &&
		self->activator->client->pers.alt_weapon == true &&
		self->activator->client->ps.guns[GUN_MAIN].frame == 1)
		Pipe_Explode(self);
}

void pipe_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (attacker->client || (attacker->svflags & SVF_MONSTER))
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
	VectorCopy(start, grenade->s.origin);
	VectorScale(aimdir, speed, grenade->velocity);
	VectorMA(grenade->velocity, 250, up, grenade->velocity);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = CONTENTS_SOLID | CONTENTS_BULLETS;
	grenade->s.clip_contents = CONTENTS_BULLETS;
	grenade->solid = SOLID_BBOX;
	grenade->health = 1;
	grenade->takedamage = DAMAGE_YES;
	grenade->die = pipe_die;
	grenade->s.game = GAME_DUKE;
	VectorSet(grenade->mins, -8, -8, -4);
	VectorSet(grenade->maxs, 8, 8, 0);
	grenade->spawnflags |= DROPPED_ITEM | DROPPED_PLAYER_ITEM;
	grenade->item = grenade->real_item = GetItemByIndex(ITI_DUKE_PIPEBOMBS);
	grenade->count = 1;
	grenade->s.modelindex = gi.modelindex("%e_pipebomb");
	grenade->activator = self;
	grenade->touch = Pipe_Touch;
	grenade->nextthink = level.time + game.frametime;
	grenade->think = Pipe_Think;
	grenade->dmg = damage;
	grenade->dmg_radius = damage_radius;
	grenade->entitytype = ET_GRENADE;
	grenade->last_move_time = level.time + 500;

	if (self->client)
		grenade->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), grenade, DT_DIRECT);
	else
		grenade->meansOfDeath = MakeWeaponMeansOfDeath(self, ITI_GRENADE_LAUNCHER, grenade, DT_DIRECT);

	gi.linkentity(grenade);

	edict_t *trigger;

	trigger = G_Spawn();
	trigger->clipmask = 0;
	trigger->s.clip_contents = CONTENTS_PLAYERCLIP;
	trigger->solid = SOLID_TRIGGER;
	VectorCopy(grenade->mins, trigger->mins);
	VectorCopy(grenade->maxs, trigger->maxs);
	trigger->chain = grenade;
	grenade->chain = trigger;
	trigger->svflags = SVF_NOCLIENT;
	trigger->touch = PipeTrigger_Touch;
	VectorCopy(grenade->s.origin, trigger->s.origin);

	gi.linkentity(trigger);
}

void weapon_duke_pipebomb_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->client->ps.guns[gun].frame == 3)
	{
		if (ent->client->buttons & BUTTON_ATTACK)
		{
			if (ent->client->pers.pipebomb_hold < 10)
				ent->client->pers.pipebomb_hold++;

			return;
		}

		if (ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index])
		{
			if (!((int)dmflags->value & DF_INFINITE_AMMO))
				ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 1;

			vec3_t start, forward, right, offset;

			AngleVectors(ent->client->v_angle, forward, NULL, NULL);
			VectorSet(offset, 0, 0, ent->viewheight - 8);
			P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

			fire_pipe(ent, start, forward, 150, 250 + (ent->client->pers.pipebomb_hold * 25), 450);
		}
	}

	ent->client->pers.pipebomb_hold = 0;
	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame >= 9)
	{
		ent->client->pers.alt_weapon = true;

		ent->client->ps.guns[gun].offset[2] = DUKE_ANIM_FRAC;
		ent->client->ps.guns[gun].index = pipe_detonator_index;
		ent->client->ps.guns[gun].frame = 0;
		ent->client->gunstates[gun].weaponstate = WEAPON_ACTIVATING;
		ent->last_move_time = 0;
	}
}

void weapon_duke_detonator_fire(edict_t *ent, gunindex_e gun, bool first)
{
	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame >= 3)
	{
		if (ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index])
			ent->client->gunstates[gun].newweapon = ent->client->pers.weapon;
		else
			NoAmmoWeaponChange(ent, gun);
	}
}

void duke_rocket_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY)) {
		G_FreeEdict(ent);
		return;
	}

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);

	// calculate position for the explosion entity
	T_RadiusDamage(ent, ent->owner, ent->radius_dmg, ent, DAMAGE_NO_PARTICLES, ent->dmg_radius, ent->meansOfDeath);

	VectorNormalize(ent->velocity);
	VectorMA(ent->s.origin, -8, ent->velocity, origin);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(ent->spawnflags == 1 ? TE_DUKE_EXPLODE_SMALL : TE_DUKE_EXPLODE);
	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);

	G_FreeEdict(ent);
}

void fire_duke_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int radius_damage, int speed, bool tiny)
{
	edict_t *rocket;

	rocket = G_Spawn();
	VectorCopy(start, rocket->s.origin);
	VectorCopy(dir, rocket->movedir);
	vectoangles(dir, rocket->s.angles);
	VectorScale(dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->clipmask = MASK_SHOT;
	rocket->solid = SOLID_BBOX;
	rocket->s.game = GAME_DUKE;
	VectorClear(rocket->mins);
	VectorClear(rocket->maxs);
	rocket->s.modelindex = tiny ? gi.modelindex("%e_smallrocket") : gi.modelindex("%e_rocket");
	rocket->owner = self;
	rocket->touch = duke_rocket_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->radius_dmg = radius_damage;
	rocket->s.frame = 3;
	rocket->entitytype = ET_ROCKET;

	if (tiny)
	{
		rocket->spawnflags = 1;
		rocket->dmg_radius = radius_damage + 120;
	}
	else
		rocket->dmg_radius = radius_damage + 400;

	if (self->client)
		check_dodge(self, rocket->s.origin, dir, speed);

	if (self->client)
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), rocket, DT_DIRECT);
	else
		rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, rocket, MD_NONE, DT_DIRECT);

	gi.linkentity(rocket);
}

void weapon_duke_devastate_fire(edict_t *ent, gunindex_e gun, bool first)
{
	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 3 && (!(ent->client->buttons & BUTTON_ATTACK) || ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] < 2 || ent->client->gunstates[gun].newweapon))
	{
		ent->client->ps.guns[gun].frame = 6;
		return;
	}
	else if (ent->client->ps.guns[gun].frame >= 6 && (ent->client->buttons & BUTTON_ATTACK) && ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] >= 2 && !ent->client->gunstates[gun].newweapon)
		ent->client->ps.guns[gun].frame = 1;

	if (ent->client->ps.guns[gun].frame == 1 || ent->client->ps.guns[gun].frame == 3)
	{
		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_GRENADE | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		vec3_t start, forward, right, offset;
		vec3_t angles;

		for (int i = 0; i < 2; ++i)
		{
			if (!ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index])
				break;

			VectorCopy(ent->client->v_angle, angles);
			angles[0] += crandom() * 2;
			angles[1] += crandom() * 2;

			AngleVectors(angles, forward, right, NULL);
			VectorSet(offset, 0, ent->client->ps.guns[gun].frame == 1 ? 8 : -8, ent->viewheight - 8);
			P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

			fire_duke_rocket(ent, start, forward, 38, 50, 850 - i * 50, true);

			if (!((int)dmflags->value & DF_INFINITE_AMMO))
				ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;
		}
	}
}

void weapon_duke_rocket_fire(edict_t *ent, gunindex_e gun, bool first)
{
	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 1)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_ROCKET | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 1;

		vec3_t start, forward, right, offset;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 8);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		fire_duke_rocket(ent, start, forward, 140, sqrtf(1780) * 2, 850, false);
		Weapon_Doom_Anim_Regular(ent);
	}
}

void weapon_duke_foot_fire_generic(edict_t *ent, gunindex_e gun)
{
	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame >= 3 && ent->client->ps.guns[gun].frame <= 5)
	{
		ent->client->anim_priority = ANIM_ATTACK;
		ent->s.frame = 6;
		ent->client->anim_end = 7;
	}
	else
	{
		ent->client->anim_priority = ANIM_ATTACK;
		ent->s.frame = 5;
		ent->client->anim_end = 6;
	}

	if (ent->client->ps.guns[gun].frame == 3)
	{
		vec3_t source, org;
		vec3_t forward;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorCopy(ent->s.origin, source);
		source[2] += ent->viewheight - 8;

		VectorMA(source, 64, forward, org);

		trace_t tr = gi.trace(source, vec3_origin, vec3_origin, org, ent, MASK_SHOT);

		if (tr.fraction != 1.0f)
		{
			VectorMA(tr.endpos, -4, forward, org);

			if (tr.ent->takedamage)
			{
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_DUKE_BLOOD);
				gi.WritePosition(org);
				gi.multicast(org, MULTICAST_PVS);

				int damage = (ent->client->quad_time > level.time) ? 42 : 12;

				vec3_t dir;

				VectorSubtract(tr.ent->s.origin, ent->s.origin, dir);
				vectoangles(dir, dir);
				dir[0] = 0;
				dir[2] = 0;
				AngleVectors(dir, dir, NULL, NULL);

				T_Damage(tr.ent, ent, ent, dir, vec3_origin, vec3_origin, damage, damage, DAMAGE_NO_PARTICLES, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
			}

			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_DUKE_GUNSHOT);
			gi.WritePosition(org);
			gi.multicast(org, MULTICAST_PVS);

			gi.sound(ent, CHAN_WEAPON, gi.soundindex("duke/KICKHIT.wav"), 1, ATTN_NORM, 0);
		}
	}
}

void weapon_duke_foot_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->client->ps.guns[gun].frame == 5 && (ent->client->buttons & BUTTON_ATTACK) && !ent->client->gunstates[gun].newweapon)
		ent->client->ps.guns[gun].frame = 0;

	weapon_duke_foot_fire_generic(ent, gun);
}

void weapon_duke_offfoot_fire(edict_t *ent, gunindex_e gun, bool first)
{
	weapon_duke_foot_fire_generic(ent, gun);

	if (ent->client->ps.guns[gun].frame == 5)
	{
		ent->client->ps.guns[gun].index = 0;
		ent->client->ps.guns[gun].frame = 0;
	}
}

void Weapon_Duke_Offhand_Foot(edict_t *ent)
{
	if (ent->deadflag || !ent->solid || ent->freeze_time > level.time)
		return;

	if (ent->client->ps.guns[GUN_OFFHAND].index != foot_index)
	{
		vec3_t source, org;
		vec3_t forward;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorCopy(ent->s.origin, source);
		source[2] += ent->viewheight - 8;

		VectorMA(source, 64, forward, org);

		trace_t tr = gi.trace(source, vec3_origin, vec3_origin, org, ent, MASK_SHOT);

		if (tr.fraction != 1.0f)
		{
			VectorMA(tr.endpos, -4, forward, org);

			if (tr.ent->takedamage && tr.ent->freeze_time > level.time)
			{
				ent->client->ps.guns[GUN_OFFHAND].index = foot_index;
				ent->client->ps.guns[GUN_OFFHAND].frame = 0;
				ent->client->ps.guns[GUN_OFFHAND].offset[1] = 0;
				ent->client->gunstates[GUN_OFFHAND].weaponstate = WEAPON_FIRING;
			}
		}
	}

	if (ent->client->ps.guns[GUN_OFFHAND].index == foot_index)
	{
		Weapon_Doom(ent, 7, weapon_duke_offfoot_fire, GUN_OFFHAND);
		ent->client->ps.guns[GUN_OFFHAND].offset[1] = 0;
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
	VectorMA(ent->s.origin, -8, vel, origin);

	if (other->takedamage)
	{
		bool freeze_fx = false;

		ent->owner = ent->activator;

		if (other->freeze_time <= level.time)
		{
			if (other->health > ent->dmg)
			{
				T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);
				gi.sound(ent, CHAN_VOICE, gi.soundindex("duke/BULITHIT.wav"), 1, ATTN_NORM, 0);
			}
			else
			{
				if (other->client || (other->svflags & SVF_MONSTER))
				{
					other->freeze_time = level.time + 5000;
					other->health = 1;
				}
				else
					T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);

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
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_DUKE_FREEZE_HIT);
			gi.WritePosition(origin);
			gi.multicast(ent->s.origin, MULTICAST_PHS);
		}

		G_FreeEdict(ent);
		return;
	}

	ent->owner = NULL;

	float dot = DotProduct(vel, plane->normal);

	if (!ent->count || (ent->count <= 2 && dot > -0.4f))
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_DUKE_FREEZE_HIT);
		gi.WritePosition(origin);
		gi.multicast(ent->s.origin, MULTICAST_PHS);

		G_FreeEdict(ent);
	}
	else
		--ent->count;
}

void fire_duke_freezer(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t *rocket;

	rocket = G_Spawn();
	VectorCopy(start, rocket->s.origin);
	VectorCopy(dir, rocket->movedir);
	vectoangles(dir, rocket->s.angles);
	VectorScale(dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYBOUNCE;
	rocket->clipmask = MASK_SHOT;
	rocket->solid = SOLID_BBOX;
	rocket->s.effects |= EF_ANIM_ALLFAST;
	rocket->s.renderfx |= RF_FULLBRIGHT | RF_ANIM_BOUNCE;
	rocket->s.game = GAME_DOOM;
	VectorClear(rocket->mins);
	VectorClear(rocket->maxs);
	rocket->s.modelindex = gi.modelindex("%e_freezer");
	rocket->owner = rocket->activator = self;
	rocket->touch = duke_freezer_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = (damage + (Q_rand() & 7)) >> 2;
	rocket->count = 3;
	rocket->entitytype = ET_DOOM_PLASMA;

	if (self->client)
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), rocket, DT_DIRECT);
	else
		rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, rocket, MD_NONE, DT_DIRECT);

	if (self->client)
		check_dodge(self, rocket->s.origin, dir, speed);

	gi.linkentity(rocket);
}

void weapon_duke_freezer_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (!ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] || ent->client->gunstates[gun].newweapon || !((ent->client->buttons | ent->client->latched_buttons) & BUTTON_ATTACK))
	{
		ent->client->ps.guns[gun].frame = 5;
		return;
	}

	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame >= 4)
		ent->client->ps.guns[gun].frame = 1;

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_BFG | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	vec3_t start, forward, right, offset;

	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	VectorSet(offset, 0, 0, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_duke_freezer(ent, start, forward, 20, 900);

	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index]--;
}

void weapon_duke_pistol_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->client->ps.guns[gun].frame == 0 ||
		ent->client->ps.guns[gun].frame == 2)
	{
		if ((ent->client->buttons & BUTTON_ATTACK) && ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] && !ent->client->gunstates[gun].newweapon && ent->client->pers.pistol_clip)
			ent->client->ps.guns[gun].frame = 1;
		else
			ent->client->ps.guns[gun].frame = 21;
	}
	else
		ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 4)
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("duke/CLIPOUT.wav"), 1, ATTN_IDLE, 0);
		gi.sound(ent, CHAN_AUTO, gi.soundindex("duke/CLIPIN.wav"), 1, ATTN_IDLE, 0);
	}
	else if (ent->client->ps.guns[gun].frame == 7)
		ent->client->pers.pistol_clip = 12;
	else if (ent->client->ps.guns[gun].frame == 13)
		ent->client->ps.guns[gun].frame = 21;
	else if (ent->client->ps.guns[gun].frame == 15)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("duke/KNUCKLE.wav"), 1, ATTN_IDLE, 0);
	else if (ent->client->ps.guns[gun].frame == 1)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_BLASTER | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 1;

		vec3_t start, forward, right, offset;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		--ent->client->pers.pistol_clip;

		fire_doom_bullet(ent, start, forward, 6, 0, TE_DUKE_GUNSHOT, 250, 250, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
	}
}

void weapon_duke_chaingun_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (!(ent->client->buttons & BUTTON_ATTACK) || !ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] || ent->client->gunstates[gun].newweapon)
	{
		ent->client->ps.guns[gun].frame = 5;
		return;
	}

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_CHAINGUN1 | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 1;

	vec3_t start, forward, right, offset;

	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	VectorSet(offset, 0, 0, ent->viewheight - 2);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_doom_bullet(ent, start, forward, 9, 0, TE_DUKE_GUNSHOT, 300, 300, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));

	if (++ent->client->ps.guns[gun].frame == 5)
		ent->client->ps.guns[gun].frame = 2;
	//ent->attack_finished_time = level.time + DOOM_TIC_TO_TIME(4);
	//Weapon_Doom_Anim_Fast(ent);
}

void weapon_duke_shotgun_fire(edict_t *ent, gunindex_e gun, bool first)
{
	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 5)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("duke/SHOTGNCK.wav"), 1, ATTN_IDLE, 0);
	else if (ent->client->ps.guns[gun].frame == 7)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_SSHOTGUN | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);
	}

	if (ent->client->ps.guns[gun].frame == 1)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_SHOTGUN | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			ent->client->pers.inventory[ent->client->gunstates[gun].ammo_index] -= 1;

		vec3_t start, forward, right, offset;

		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		fire_doom_shotgun(ent, start, forward, 7, 0, TE_DUKE_GUNSHOT, 7, 250, 250, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
		Weapon_Doom_Anim_Regular(ent);
	}
}

// only Duke uses this atm
void Weapons_Init()
{
	pipe_detonator_index = gi.modelindex("%wv_detonator");
	pipe_bomb_index = gi.modelindex("%wv_grenades");
	foot_index = gi.modelindex("weaponscripts/duke/v_off_foot.wsc");
}
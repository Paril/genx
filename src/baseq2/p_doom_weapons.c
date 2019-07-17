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

extern bool     is_quad;
extern byte     is_silenced;

#define DOOM_ANIM_TIME 400
#define DOOM_ANIM_FRAC DOOM_ANIM_TIME

void Weapon_Doom(edict_t *ent, int num_frames, void(*fire)(edict_t *ent, gunindex_e gun, bool first), gunindex_e gun)
{
	if (ent->deadflag || !ent->solid || ent->freeze_time > level.time) // VWep animations screw up corpses
		return;

	/*if (ent->client->next_weapon_update > ent->client->player_time)
		return;

	ent->client->next_weapon_update = ent->client->player_time + 1;*/
	VectorClear(ent->client->ps.guns[gun].offset);

	switch (ent->client->gunstates[gun].weaponstate)
	{
		case WEAPON_ACTIVATING:
			ent->client->gunstates[gun].weapon_time = min(DOOM_ANIM_TIME, ent->client->gunstates[gun].weapon_time + game.frametime);

			if (ent->client->gunstates[gun].weapon_time == DOOM_ANIM_TIME)
				ent->client->ps.guns[gun].offset[2] = 0.0f;
			else
			{
				ent->client->ps.guns[gun].offset[2] = (float)(DOOM_ANIM_TIME - ent->client->gunstates[gun].weapon_time) / DOOM_ANIM_FRAC;
				break;
			}

		case WEAPON_READY:
			ent->client->gunstates[gun].weaponstate = WEAPON_READY;

			if ((ent->client->gunstates[gun].newweapon) && (ent->client->gunstates[gun].weaponstate != WEAPON_FIRING))
			{
				if (ent->s.game == GAME_DUKE)
				{
					const char *sound = NULL;

					switch (GetIndexByItem(ent->client->gunstates[gun].newweapon))
					{
						case ITI_DUKE_FOOT:

						//case ITI_DUKE_TRIPBOMBS:
						case ITI_DUKE_PIPEBOMBS:
							break;

						case ITI_DUKE_PISTOL:
							sound = "duke/CLIPIN.wav";
							break;

						case ITI_DUKE_CANNON:
						default:
							sound = "duke/WPNSEL21.wav";
							break;

						case ITI_DUKE_SHOTGUN:
							sound = "duke/SHOTGNCK.wav";
							break;
					}

					if (sound)
						gi.sound(ent, CHAN_ITEM | CHAN_NO_PHS_ADD | CHAN_NO_EAVESDROP, gi.soundindex(sound), 1, ATTN_NONE, 0);
				}

				ent->client->gunstates[gun].weaponstate = WEAPON_DROPPING;
				ent->client->gunstates[gun].weapon_time = DOOM_ANIM_TIME;
				goto dropping;
			}

			if ((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)
			{
				ent->client->latched_buttons &= ~BUTTON_ATTACK;

				if ((level.time - ent->client->respawn_time) > 500 &&
					ent->attack_finished_time < level.time)
				{
					if ((game_iteminfos[ent->s.game].ammo_usages[ITEM_INDEX(ent->client->pers.weapon)] <= 0) ||
						((ent->s.game == GAME_DUKE && ITEM_INDEX(ent->client->pers.weapon) == ITI_DUKE_PIPEBOMBS && ent->client->pers.alt_weapon) || (ent->client->pers.ammo >= GetWeaponUsageCount(ent, ent->client->pers.weapon))))
					{
						fire(ent, gun, true);

						if (ent->client->ps.guns[gun].frame >= num_frames)
						{
							ent->client->ps.guns[gun].frame = 0;
							ent->client->gunstates[gun].weaponstate = WEAPON_READY;
						}
						else
							ent->client->gunstates[gun].weaponstate = WEAPON_FIRING;
					}
					else
						NoAmmoWeaponChange(ent, gun);
				}

				if (ent->s.game == GAME_DUKE)
					ent->client->ps.guns[gun].offset[1] = 1.0f;
			}
			else
				ent->client->ps.guns[gun].offset[1] = 1.0f;

			break;

		case WEAPON_FIRING:
			if (ent->attack_finished_time < level.time)
				fire(ent, gun, false);

			if (ent->client->ps.guns[gun].frame >= num_frames)
			{
				ent->client->ps.guns[gun].frame = 0;
				ent->client->gunstates[gun].weaponstate = WEAPON_READY;
			}

			if (ent->client->gunstates[gun].weaponstate == WEAPON_FIRING && ent->s.game == GAME_DUKE)
				ent->client->ps.guns[gun].offset[1] = 1.0f;

			break;

		case WEAPON_DROPPING:
			ent->client->gunstates[gun].weapon_time = (gtime_t)max(0, (int32_t)ent->client->gunstates[gun].weapon_time - (int32_t)game.frametime);
dropping:

			if (ent->client->gunstates[gun].weapon_time == 0)
			{
				ent->client->ps.guns[gun].offset[2] = DOOM_ANIM_FRAC;
				ChangeWeapon(ent, gun);
				return;
			}
			else
				ent->client->ps.guns[gun].offset[2] = (float)(DOOM_ANIM_TIME - ent->client->gunstates[gun].weapon_time) / DOOM_ANIM_FRAC;

			break;
	}
}

void Weapon_Doom_Anim_Regular(edict_t *ent)
{
	if (ent->health <= 0)
		return;

	ent->client->anim_priority = ANIM_ATTACK | ANIM_REVERSE;
	ent->s.frame = 5 + 1;
	ent->client->anim_end = 4;
}

void Weapon_Doom_Anim_Fast(edict_t *ent)
{
	if (Q_rand() % 2 == 0)
		Weapon_Doom_Anim_Regular(ent);
	else
	{
		ent->client->anim_priority = ANIM_ATTACK;
		ent->s.frame = 4 - 1;
		ent->client->anim_end = 5;
	}
}

void doom_rocket_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
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
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_DOOM_EXPLODE);
	MSG_WritePos(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);
	G_FreeEdict(ent);
}

void fire_doom_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int radius_damage, int speed)
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
	rocket->s.game = GAME_DOOM;
	VectorClear(rocket->mins);
	VectorClear(rocket->maxs);
	rocket->s.modelindex = gi.modelindex("%e_rocket");
	rocket->owner = self;
	rocket->touch = doom_rocket_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->radius_dmg = radius_damage / 2.0f;
	rocket->dmg_radius = radius_damage + sqrtf(400);
	rocket->s.frame = 3;
	rocket->entitytype = ET_ROCKET;

	if (self->client)
		check_dodge(self, rocket->s.origin, dir, speed);

	if (self->client)
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), rocket, DT_DIRECT);
	else
		rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, rocket, MD_NONE, DT_DIRECT);

	gi.linkentity(rocket);
}

int Doom_MissileDamageRandomizer(int dmg)
{
	return ((Q_rand() % 8) + 1) * dmg;
}

void weapon_doom_rocket_fire(edict_t *ent, gunindex_e gun, bool first)
{
	bool fire = false;

	if (ent->client->ps.guns[gun].frame >= 7 && (ent->client->buttons & BUTTON_ATTACK) && HasEnoughAmmoToFire(ent, ent->client->pers.weapon) && !ent->client->gunstates[gun].newweapon)
	{
		ent->client->ps.guns[gun].frame = 1;
		fire = true;
	}

	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 2 || fire)
	{
		MSG_WriteByte(svc_muzzleflash);
		MSG_WriteShort(ent - g_edicts);
		MSG_WriteByte(MZ_ROCKET | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);
		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

		vec3_t start, forward, right, offset;
		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 8);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		fire_doom_rocket(ent, start, forward, Doom_MissileDamageRandomizer(20), 128, 650);
		Weapon_Doom_Anim_Regular(ent);
	}
}

void weapon_doom_punch_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->client->ps.guns[gun].frame == 5 && (ent->client->buttons & BUTTON_ATTACK) && !ent->client->gunstates[gun].newweapon)
		ent->client->ps.guns[gun].frame = 1;
	else
		ent->client->ps.guns[gun].frame++;

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
				MSG_WriteByte(svc_temp_entity);
				MSG_WriteByte(TE_DOOM_BLOOD);
				MSG_WritePos(org);
				gi.multicast(org, MULTICAST_PVS);
				int damage = (Q_rand() % 10 + 1) << 1;

				if (ent->client->quad_time > level.time)
					damage *= 10;

				vec3_t dir;
				VectorSubtract(tr.ent->s.origin, ent->s.origin, dir);
				vectoangles(dir, dir);
				dir[0] = 0;
				dir[2] = 0;
				AngleVectors(dir, dir, NULL, NULL);
				T_Damage(tr.ent, ent, ent, dir, vec3_origin, vec3_origin, damage, (ent->client->quad_time > level.time) ? damage : 0, DAMAGE_NO_PARTICLES, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
				gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/PUNCH.wav"), 1, ATTN_NORM, 0);
			}
			else
			{
				MSG_WriteByte(svc_temp_entity);
				MSG_WriteByte(TE_DOOM_GUNSHOT);
				MSG_WritePos(org);
				gi.multicast(org, MULTICAST_PVS);
			}
		}

		Weapon_Doom_Anim_Regular(ent);
	}
}

void doom_plasma_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t      origin;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, DAMAGE_NO_PARTICLES, ent->meansOfDeath);

	VectorNormalize(ent->velocity);
	VectorMA(ent->s.origin, -8, ent->velocity, origin);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_DOOM_PLASMA);
	MSG_WritePos(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);
	G_FreeEdict(ent);
}

void fire_doom_plasma(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
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
	rocket->s.effects |= EF_ANIM_ALLFAST;
	rocket->s.renderfx |= RF_FULLBRIGHT;
	rocket->s.game = GAME_DOOM;
	VectorClear(rocket->mins);
	VectorClear(rocket->maxs);
	rocket->s.modelindex = gi.modelindex("sprites/doom/PLSS.d2s");
	rocket->owner = self;
	rocket->touch = doom_plasma_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->entitytype = ET_DOOM_PLASMA;

	if (self->client)
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), rocket, DT_DIRECT);
	else
		rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, rocket, MD_NONE, DT_DIRECT);

	if (self->client)
		check_dodge(self, rocket->s.origin, dir, speed);

	gi.linkentity(rocket);
}

void weapon_doom_hyperblaster_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->client->ps.guns[gun].frame >= 3)
	{
		++ent->client->ps.guns[gun].frame;
		return;
	}

	if (!(ent->client->buttons & BUTTON_ATTACK) || ent->client->gunstates[gun].newweapon || !HasEnoughAmmoToFire(ent, ent->client->pers.weapon))
	{
		ent->attack_finished_time = level.time + 571;
		ent->client->ps.guns[gun].frame = 3;
		return;
	}

	MSG_WriteByte(svc_muzzleflash);
	MSG_WriteShort(ent - g_edicts);
	MSG_WriteByte(MZ_HYPERBLASTER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
	vec3_t start, forward, right, offset;
	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	VectorSet(offset, 0, 0, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	fire_doom_plasma(ent, start, forward, Doom_MissileDamageRandomizer(5), 850);
	Weapon_Doom_Anim_Fast(ent);
	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);
	ent->client->ps.guns[gun].frame = 1 + (Q_rand() % 2);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);
}

static void fire_doom_lead(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, meansOfDeath_t mod)
{
	trace_t     tr;
	vec3_t      dir;
	vec3_t      forward, right, up;
	vec3_t      end;
	float       r;
	float       u;
	tr = gi.trace(self->s.origin, NULL, NULL, start, self, MASK_BULLET);

	if (!(tr.fraction < 1.0f))
	{
		vectoangles(aimdir, dir);
		AngleVectors(dir, forward, right, up);
		r = crandom() * hspread;
		u = crandom() * vspread;
		VectorMA(start, 8192, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);
		tr = gi.trace(start, NULL, NULL, end, self, MASK_BULLET);
	}

	// send gun puff / flash
	if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))
	{
		if (tr.fraction < 1.0f)
		{
			vec3_t puff;
			VectorMA(tr.endpos, -4, forward, puff);

			if (tr.ent->takedamage)
			{
				AddMultiDamage(tr.ent, damage, kick, mod, DAMAGE_BULLET | DAMAGE_NO_PARTICLES, false, tr.endpos, tr.plane.normal);
				MSG_WriteByte(svc_temp_entity);
				MSG_WriteByte(te_impact == TE_DUKE_GUNSHOT ? TE_DUKE_BLOOD : TE_DOOM_BLOOD);
				MSG_WritePos(puff);
				gi.multicast(puff, MULTICAST_PVS);
			}
			else if (strncmp(tr.surface->name, "sky", 3) != 0)
			{
				MSG_WriteByte(svc_temp_entity);
				MSG_WriteByte(te_impact);
				MSG_WritePos(puff);
				gi.multicast(puff, MULTICAST_PVS);

				if (self->client)
					PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
			}
		}
	}
}

void fire_doom_bullet(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, meansOfDeath_t mod)
{
	fire_doom_lead(self, start, aimdir, damage, kick, te_impact, hspread, vspread, mod);
	ApplyMultiDamage(self, aimdir, DAMAGE_NO_PARTICLES, mod);
}

void fire_doom_shotgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int count, int hspread, int vspread, meansOfDeath_t mod)
{
	int i;

	for (i = 0; i < count; ++i)
		fire_doom_lead(self, start, aimdir, damage, kick, te_impact, hspread, vspread, mod);

	ApplyMultiDamage(self, aimdir, DAMAGE_NO_PARTICLES, mod);
}

void weapon_doom_pistol_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->client->ps.guns[gun].frame >= 3 && (ent->client->buttons & BUTTON_ATTACK) && HasEnoughAmmoToFire(ent, ent->client->pers.weapon) && !ent->client->gunstates[gun].newweapon)
		ent->client->ps.guns[gun].frame = 0;
	else
		ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 1)
	{
		MSG_WriteByte(svc_muzzleflash);
		MSG_WriteShort(ent - g_edicts);
		MSG_WriteByte(MZ_BLASTER | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);
		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

		vec3_t start, forward, right, offset;
		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		int damage = 5 * (Q_rand() % 3 + 1);
		fire_doom_bullet(ent, start, forward, damage, 0, TE_DOOM_GUNSHOT, first ? 0 : 400, 0, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
		Weapon_Doom_Anim_Regular(ent);
	}
}

#define DOOM_TIC_TO_TIME(t) (gtime_t)(floorf(((1.0f / 35.0f) * 1000) * (t - 1)))

void weapon_doom_chaingun_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if ((ent->client->ps.guns[gun].frame == 2 && !(ent->client->buttons & BUTTON_ATTACK)) || !HasEnoughAmmoToFire(ent, ent->client->pers.weapon) || ent->client->gunstates[gun].newweapon)
	{
		ent->client->ps.guns[gun].frame = 3;
		return;
	}

	MSG_WriteByte(svc_muzzleflash);
	MSG_WriteShort(ent - g_edicts);
	MSG_WriteByte(MZ_BLASTER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

	vec3_t start, forward, right, offset;
	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	VectorSet(offset, 0, 0, ent->viewheight - 2);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	int damage = 5 * (Q_rand() % 3 + 1);
	fire_doom_bullet(ent, start, forward, damage, 0, TE_DOOM_GUNSHOT, 400, 0, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
	ent->client->ps.guns[gun].frame = (ent->client->ps.guns[gun].frame == 1) ? 2 : 1;
	ent->attack_finished_time = level.time + DOOM_TIC_TO_TIME(4);
	Weapon_Doom_Anim_Fast(ent);
}

void weapon_doom_shotgun_fire(edict_t *ent, gunindex_e gun, bool first)
{
	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 1)
	{
		MSG_WriteByte(svc_muzzleflash);
		MSG_WriteShort(ent - g_edicts);
		MSG_WriteByte(MZ_SHOTGUN | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);
		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

		vec3_t start, forward, right, offset;
		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		int damage = 5 * (Q_rand() % 3 + 1);
		fire_doom_shotgun(ent, start, forward, damage, 0, TE_DOOM_GUNSHOT, 7, 400, 0, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
		Weapon_Doom_Anim_Regular(ent);
	}
}

void weapon_doom_sshotgun_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->client->ps.guns[gun].frame >= 16 && (ent->client->buttons & BUTTON_ATTACK) && HasEnoughAmmoToFire(ent, ent->client->pers.weapon) && !ent->client->gunstates[gun].newweapon)
		ent->client->ps.guns[gun].frame = 0;

	ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 1)
	{
		MSG_WriteByte(svc_muzzleflash);
		MSG_WriteShort(ent - g_edicts);
		MSG_WriteByte(MZ_SSHOTGUN | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);
		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

		if (!((int)dmflags->value & DF_INFINITE_AMMO))
			RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

		vec3_t start, forward, right, offset;
		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		int damage = 5 * (Q_rand() % 3 + 1);
		fire_doom_shotgun(ent, start, forward, damage, 0, TE_DOOM_GUNSHOT, 20, 1300, 900, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
		Weapon_Doom_Anim_Regular(ent);
	}
	else if (ent->client->ps.guns[gun].frame == 7)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("doom/DBOPN.wav"), 1, ATTN_NORM, 0);
	else if (ent->client->ps.guns[gun].frame == 11)
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/DBLOAD.wav"), 1, ATTN_NORM, 0);
	else if (ent->client->ps.guns[gun].frame == 15)
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/DBCLS.wav"), 1, ATTN_NORM, 0);
}

void weapon_doom_chainsaw_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (!(ent->client->buttons & BUTTON_ATTACK) || ent->client->gunstates[gun].newweapon)
	{
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWIDL.wav"), 1, ATTN_NORM, 0);
		ent->client->silencer_shots = 2;
		ent->client->ps.guns[gun].frame = 5;
		return;
	}

	if (ent->client->ps.guns[gun].frame == 2)
		ent->client->ps.guns[gun].frame = 3;
	else
		ent->client->ps.guns[gun].frame = 2;

	int damage = 2 * (Q_rand() % 10 + 1);
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
			MSG_WriteByte(svc_temp_entity);
			MSG_WriteByte(TE_DOOM_BLOOD);
			MSG_WritePos(org);
			gi.multicast(org, MULTICAST_PVS);
			T_Damage(tr.ent, ent, ent, vec3_origin, vec3_origin, vec3_origin, damage, 0, DAMAGE_NO_PARTICLES, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWHIT.wav"), 1, ATTN_NORM, 0);
		}
		else
		{
			MSG_WriteByte(svc_temp_entity);
			MSG_WriteByte(TE_DOOM_GUNSHOT);
			MSG_WritePos(org);
			gi.multicast(org, MULTICAST_PVS);
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWFUL.wav"), 1, ATTN_NORM, 0);
		}
	}
	else
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWFUL.wav"), 1, ATTN_NORM, 0);

	Weapon_Doom_Anim_Fast(ent);
}

/*
=================
fire_bfg
=================
*/
void bfg_doom_explode(edict_t *self)
{
	self->nextthink = level.time + (game.frametime * 2);
	self->s.frame++;

	if (self->s.frame == 2 && !self->activator)
	{
		self->meansOfDeath.damage_type = DT_INDIRECT;
		self->activator = self;
		int			i;
		int			j;
		int			damage;
		vec3_t		angles = { 0, 0, 0 };

		// offset angles from its attack angle
		for (i = 0; i < 40; i++)
		{
			const float one_angle = 90.0f / (40 - 1);
			float angle_mul = one_angle * i;
			const float move_left = 90.0f / 2.0f;
			angles[1] = self->s.angles[1] - move_left + angle_mul;
			vec3_t forward;
			AngleVectors(angles, forward, NULL, NULL);
			vec3_t start, end;
			VectorCopy(self->owner->s.origin, start);
			VectorMA(start, 1024, forward, end);
			trace_t tr = gi.trace(start, vec3_origin, vec3_origin, end, self->owner, CONTENTS_SOLID | CONTENTS_WINDOW | CONTENTS_MONSTER);

			if (tr.fraction == 1.0f || !tr.ent)
				continue;

			/*P_SpawnMobj(linetarget->x,
				linetarget->y,
				linetarget->z + (linetarget->height >> 2),
				MT_EXTRABFG);*/
			MSG_WriteByte(svc_temp_entity);
			MSG_WriteByte(TE_BFG_EXPLOSION);
			MSG_WritePos(tr.ent->s.origin);
			gi.multicast(tr.ent->s.origin, MULTICAST_PHS);
			damage = 0;

			for (j = 0; j < 15; j++)
				damage += (Q_rand() & 7) + 1;

			T_Damage(tr.ent, self, self->owner, vec3_origin, vec3_origin, vec3_origin, damage, 0, DAMAGE_NO_PARTICLES, self->meansOfDeath);
		}
	}

	if (self->s.frame == 6)
		G_FreeEdict(self);
}

void bfg_doom_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	// core explosion - prevents firing it into the wall/floor
	if (other->takedamage)
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 0, DAMAGE_NONE, self->meansOfDeath);

	gi.sound(self, CHAN_VOICE, gi.soundindex("doom/RXPLOD.wav"), 1, ATTN_NORM, 0);
	self->solid = SOLID_NOT;
	self->touch = NULL;
	VectorMA(self->s.origin, -1 * game.frameseconds, self->velocity, self->s.origin);
	VectorClear(self->velocity);
	self->s.modelindex = gi.modelindex("sprites/doom/BFE1.d2s");
	self->s.frame = 0;
	self->s.sound = 0;
	self->s.effects &= ~EF_ANIM_ALLFAST;
	self->think = bfg_doom_explode;
	self->nextthink = level.time + (game.frametime * 2);
	self->activator = NULL;
	self->enemy = other;
}

void fire_doom_bfg(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t *bfg;
	bfg = G_Spawn();
	VectorCopy(start, bfg->s.origin);
	VectorCopy(dir, bfg->movedir);
	bfg->s.angles[1] = (self->client ? self->client->v_angle[1] : self->s.angles[1]);
	VectorScale(dir, speed, bfg->velocity);
	bfg->movetype = MOVETYPE_FLYMISSILE;
	bfg->clipmask = MASK_SHOT;
	bfg->solid = SOLID_BBOX;
	bfg->s.effects |= EF_ANIM_ALLFAST;
	bfg->s.renderfx |= RF_FULLBRIGHT;
	VectorClear(bfg->mins);
	VectorClear(bfg->maxs);
	bfg->s.modelindex = gi.modelindex("sprites/doom/BFS1.d2s");
	bfg->owner = self;
	bfg->touch = bfg_doom_touch;
	bfg->nextthink = level.time + (8000000.0f / speed);
	bfg->think = G_FreeEdict;
	bfg->dmg = damage;
	bfg->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->client->pers.weapon), bfg, DT_DIRECT);

	if (self->client)
		check_dodge(self, bfg->s.origin, dir, speed);

	gi.linkentity(bfg);
}

void weapon_doom_bfg_fire(edict_t *ent, gunindex_e gun, bool first)
{
	if (ent->yaw_speed > 0)
	{
		if (ent->client->ps.guns[gun].frame >= 3 && !ent->client->gunstates[gun].newweapon && (ent->client->buttons & BUTTON_ATTACK) && HasEnoughAmmoToFire(ent, ent->client->pers.weapon))
		{
			ent->client->ps.guns[gun].frame = 0;
			ent->yaw_speed = 0;
			goto refire;
		}

		ent->yaw_speed -= 0.1f;
	}
	else
	{
refire:

		switch (ent->client->ps.guns[gun].frame)
		{
			case 0:
				MSG_WriteByte(svc_muzzleflash);
				MSG_WriteShort(ent - g_edicts);
				MSG_WriteByte(MZ_BFG | is_silenced);
				gi.multicast(ent->s.origin, MULTICAST_PVS);
				PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);
				ent->yaw_speed = 0.5f;
				break;

			case 1:
				ent->yaw_speed = 0.2f;
				break;

			case 2:
				ent->yaw_speed = 0.2f;
				{
					vec3_t forward, right, start, offset;
					AngleVectors(ent->client->v_angle, forward, right, NULL);
					VectorSet(offset, 8, 0, ent->viewheight - 8);
					P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
					fire_doom_bfg(ent, start, forward, Doom_MissileDamageRandomizer(100), 850);
					Weapon_Doom_Anim_Regular(ent);

					if (!((int)dmflags->value & DF_INFINITE_AMMO))
						RemoveAmmoFromFiring(ent, ent->client->pers.weapon);
				}
				break;

			case 3:
				ent->yaw_speed = 0.8f;
				break;
		}

		ent->client->ps.guns[gun].frame++;
	}
}

static void Weapon_Doom_Fist(edict_t *ent, gunindex_e gun)
{
	Weapon_Doom(ent, 6, weapon_doom_punch_fire, gun);
}

static void Weapon_Doom_Pistol(edict_t *ent, gunindex_e gun)
{
	Weapon_Doom(ent, 6, weapon_doom_pistol_fire, gun);
}

static void Weapon_Doom_Shotgun(edict_t *ent, gunindex_e gun)
{
	Weapon_Doom(ent, 10, weapon_doom_shotgun_fire, gun);
}

static void Weapon_Doom_SuperShotgun(edict_t *ent, gunindex_e gun)
{
	Weapon_Doom(ent, 17, weapon_doom_sshotgun_fire, gun);
}

static void Weapon_Doom_Chaingun(edict_t *ent, gunindex_e gun)
{
	Weapon_Doom(ent, 3, weapon_doom_chaingun_fire, gun);
}

static void Weapon_Doom_RocketLauncher(edict_t *ent, gunindex_e gun)
{
	Weapon_Doom(ent, 8, weapon_doom_rocket_fire, gun);
}

static void Weapon_Doom_PlasmaGun(edict_t *ent, gunindex_e gun)
{
	Weapon_Doom(ent, 4, weapon_doom_hyperblaster_fire, gun);
}

static void Weapon_Doom_BFG(edict_t *ent, gunindex_e gun)
{
	if (ent->client->gunstates[gun].weaponstate != WEAPON_FIRING)
		ent->yaw_speed = 0;

	Weapon_Doom(ent, 5, weapon_doom_bfg_fire, gun);
}

static void Weapon_Doom_Chainsaw(edict_t *ent, gunindex_e gun)
{
	if (ent->client->gunstates[gun].weaponstate == WEAPON_ACTIVATING && ent->client->silencer_shots == 0)
	{
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWUP.wav"), 1, ATTN_NORM, 0);
		ent->client->silencer_shots = 1;
	}
	else if (ent->client->gunstates[gun].weaponstate == WEAPON_READY)
	{
		if (--ent->client->silencer_shots <= 0 && !(ent->client->buttons & BUTTON_ATTACK))
		{
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWIDL.wav"), 1, ATTN_NORM, 0);
			ent->client->silencer_shots = 2;
		}

		ent->client->ps.guns[gun].frame ^= 1;

		if (ent->client->ps.guns[gun].frame >= 2)
			ent->client->ps.guns[gun].frame = 0;
	}
	else if (ent->client->gunstates[gun].weaponstate == WEAPON_DROPPING)
		ent->client->silencer_shots = 0;

	Weapon_Doom(ent, 4, weapon_doom_chainsaw_fire, gun);
}

void Weapon_Doom_Run(edict_t *ent, gunindex_e gun)
{
	switch (ITEM_INDEX(ent->client->pers.weapon))
	{
		case ITI_DOOM_FIST:
			Weapon_Doom_Fist(ent, gun);
			break;

		case ITI_DOOM_PISTOL:
			Weapon_Doom_Pistol(ent, gun);
			break;

		case ITI_DOOM_SHOTGUN:
			Weapon_Doom_Shotgun(ent, gun);
			break;

		case ITI_DOOM_SUPER_SHOTGUN:
			Weapon_Doom_SuperShotgun(ent, gun);
			break;

		case ITI_DOOM_CHAINGUN:
			Weapon_Doom_Chaingun(ent, gun);
			break;

		case ITI_DOOM_ROCKET_LAUNCHER:
			Weapon_Doom_RocketLauncher(ent, gun);
			break;

		case ITI_DOOM_PLASMA_GUN:
			Weapon_Doom_PlasmaGun(ent, gun);
			break;

		case ITI_DOOM_BFG:
			Weapon_Doom_BFG(ent, gun);
			break;

		case ITI_DOOM_CHAINSAW:
			Weapon_Doom_Chainsaw(ent, gun);
			break;
	}
}
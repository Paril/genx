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

#define DOOM_ANIM_TIME 400
#define DOOM_ANIM_FRAC DOOM_ANIM_TIME

void Weapon_Doom(edict_t *ent, int num_frames, G_DoomLikeWeaponFire fire, player_gun_t *gun, gun_state_t *gun_state)
{
	if (ent->deadflag || !ent->server.solid || ent->freeze_time > level.time) // VWep animations screw up corpses
		return;

	/*if (ent->server.client->next_weapon_update > ent->server.client->player_time)
		return;

	ent->server.client->next_weapon_update = ent->server.client->player_time + 1;*/
	VectorClear(gun->offset);

	switch (gun_state->weaponstate)
	{
		case WEAPON_ACTIVATING:
			gun_state->weapon_time = min(DOOM_ANIM_TIME, gun_state->weapon_time + game.frametime);

			if (gun_state->weapon_time == DOOM_ANIM_TIME)
				gun->offset[2] = 0.0f;
			else
			{
				gun->offset[2] = (float)(DOOM_ANIM_TIME - gun_state->weapon_time) / DOOM_ANIM_FRAC;
				break;
			}

		case WEAPON_READY:
			gun_state->weaponstate = WEAPON_READY;

			if ((gun_state->newweapon) && (gun_state->weaponstate != WEAPON_FIRING))
			{
				if (ent->server.state.game == GAME_DUKE)
				{
					const char *sound = NULL;

					switch (GetIndexByItem(gun_state->newweapon))
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

				gun_state->weaponstate = WEAPON_DROPPING;
				gun_state->weapon_time = DOOM_ANIM_TIME;
				goto dropping;
			}

			if ((ent->server.client->latched_buttons | ent->server.client->buttons) & BUTTON_ATTACK)
			{
				ent->server.client->latched_buttons &= ~BUTTON_ATTACK;

				if ((level.time - ent->server.client->respawn_time) > 500 &&
					ent->attack_finished_time < level.time)
				{
					if ((game_iteminfos[ent->server.state.game].ammo_usages[ITEM_INDEX(ent->server.client->pers.weapon)] <= 0) ||
						((ent->server.state.game == GAME_DUKE && ITEM_INDEX(ent->server.client->pers.weapon) == ITI_DUKE_PIPEBOMBS && ent->server.client->pers.alt_weapon) || (ent->server.client->pers.ammo >= GetWeaponUsageCount(ent, ent->server.client->pers.weapon))))
					{
						fire(ent, gun, gun_state, true);

						if (gun->frame >= num_frames)
						{
							gun->frame = 0;
							gun_state->weaponstate = WEAPON_READY;
						}
						else
							gun_state->weaponstate = WEAPON_FIRING;
					}
					else
						NoAmmoWeaponChange(ent, gun_state);
				}

				if (ent->server.state.game == GAME_DUKE)
					gun->offset[1] = 1.0f;
			}
			else
				gun->offset[1] = 1.0f;

			break;

		case WEAPON_FIRING:
			if (ent->attack_finished_time < level.time)
				fire(ent, gun, gun_state, false);

			if (gun->frame >= num_frames)
			{
				gun->frame = 0;
				gun_state->weaponstate = WEAPON_READY;
			}

			if (gun_state->weaponstate == WEAPON_FIRING && ent->server.state.game == GAME_DUKE)
				gun->offset[1] = 1.0f;

			break;

		case WEAPON_DROPPING:
			gun_state->weapon_time = (gtime_t)max(0, (int32_t)gun_state->weapon_time - (int32_t)game.frametime);
dropping:

			if (gun_state->weapon_time == 0)
			{
				gun->offset[2] = DOOM_ANIM_FRAC;
				ChangeWeapon(ent, gun, gun_state);
				return;
			}
			else
				gun->offset[2] = (float)(DOOM_ANIM_TIME - gun_state->weapon_time) / DOOM_ANIM_FRAC;

			break;
	}
}

void Weapon_Doom_Anim_Regular(edict_t *ent)
{
	if (ent->health <= 0)
		return;

	ent->server.client->anim_priority = ANIM_ATTACK | ANIM_REVERSE;
	ent->server.state.frame = 5 + 1;
	ent->server.client->anim_end = 4;
}

void Weapon_Doom_Anim_Fast(edict_t *ent)
{
	if (Q_rand() % 2 == 0)
		Weapon_Doom_Anim_Regular(ent);
	else
	{
		ent->server.client->anim_priority = ANIM_ATTACK;
		ent->server.state.frame = 4 - 1;
		ent->server.client->anim_end = 5;
	}
}

void doom_rocket_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
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
	T_RadiusDamage(ent, ent->server.owner, ent->radius_dmg, ent, DAMAGE_NO_PARTICLES, ent->dmg_radius, ent->meansOfDeath);
	VectorNormalize(ent->velocity);
	VectorMA(ent->server.state.origin, -8, ent->velocity, origin);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_DOOM_EXPLODE);
	MSG_WritePos(origin);
	gi.multicast(ent->server.state.origin, MULTICAST_PHS);
	G_FreeEdict(ent);
}

void fire_doom_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int radius_damage, int speed)
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
	rocket->server.state.game = GAME_DOOM;
	VectorClear(rocket->server.mins);
	VectorClear(rocket->server.maxs);
	rocket->server.state.modelindex = gi.modelindex("%e_rocket");
	rocket->server.owner = self;
	rocket->touch = doom_rocket_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->radius_dmg = radius_damage / 2.0f;
	rocket->dmg_radius = radius_damage + sqrtf(400);
	rocket->server.state.frame = 3;
	rocket->entitytype = ET_ROCKET;

#ifdef ENABLE_COOP
	check_dodge(self, rocket->server.state.origin, dir, speed);
#endif

	if (self->server.client)
		rocket->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->server.client->pers.weapon), rocket, DT_DIRECT);
	else
		rocket->meansOfDeath = MakeAttackerMeansOfDeath(self, rocket, MD_NONE, DT_DIRECT);

	gi.linkentity(rocket);
}

int Doom_MissileDamageRandomizer(int dmg)
{
	return ((Q_rand() % 8) + 1) * dmg;
}

void weapon_doom_rocket_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	bool fire = false;

	if (gun->frame >= 7 && (ent->server.client->buttons & BUTTON_ATTACK) && HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon) && !gun_state->newweapon)
	{
		gun->frame = 1;
		fire = true;
	}

	gun->frame++;

	if (gun->frame == 2 || fire)
	{
		G_SendMuzzleFlash(ent, MZ_ROCKET);

		RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

		vec3_t start, forward, right, offset;
		AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 8);
		P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
		fire_doom_rocket(ent, start, forward, Doom_MissileDamageRandomizer(20), 128, 650);
		Weapon_Doom_Anim_Regular(ent);
	}
}

void weapon_doom_punch_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	if (gun->frame == 5 && (ent->server.client->buttons & BUTTON_ATTACK) && !gun_state->newweapon)
		gun->frame = 1;
	else
		gun->frame++;

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
				MSG_WriteByte(TE_DOOM_BLOOD);
				MSG_WritePos(org);
				gi.multicast(org, MULTICAST_PVS);
				int damage = (Q_rand() % 10 + 1) << 1;

				if (ent->server.client->quad_time > level.time)
					damage *= 10;

				vec3_t dir;
				VectorSubtract(tr.ent->server.state.origin, ent->server.state.origin, dir);
				vectoangles(dir, dir);
				dir[0] = 0;
				dir[2] = 0;
				AngleVectors(dir, dir, NULL, NULL);
				T_Damage(tr.ent, ent, ent, dir, vec3_origin, vec3_origin, damage, (ent->server.client->quad_time > level.time) ? damage : 0, DAMAGE_NO_PARTICLES, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), ent, DT_DIRECT));
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

	VectorNormalize(ent->velocity);
	VectorMA(ent->server.state.origin, -8, ent->velocity, origin);
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_DOOM_PLASMA);
	MSG_WritePos(origin);
	gi.multicast(ent->server.state.origin, MULTICAST_PHS);
	G_FreeEdict(ent);
}

void fire_doom_plasma(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
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
	rocket->server.state.effects |= EF_ANIM_ALLFAST;
	rocket->server.state.renderfx |= RF_FULLBRIGHT;
	rocket->server.state.game = GAME_DOOM;
	VectorClear(rocket->server.mins);
	VectorClear(rocket->server.maxs);
	rocket->server.state.modelindex = gi.modelindex("sprites/doom/PLSS.d2s");
	rocket->server.owner = self;
	rocket->touch = doom_plasma_touch;
	rocket->nextthink = level.time + 8000000.0f / speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
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

void weapon_doom_hyperblaster_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	if (gun->frame >= 3)
	{
		++gun->frame;
		return;
	}

	if (!(ent->server.client->buttons & BUTTON_ATTACK) || gun_state->newweapon || !HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon))
	{
		ent->attack_finished_time = level.time + 571;
		gun->frame = 3;
		return;
	}

	G_SendMuzzleFlash(ent, MZ_HYPERBLASTER);

	vec3_t start, forward, right, offset;
	AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
	VectorSet(offset, 0, 0, ent->viewheight - 8);
	P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
	fire_doom_plasma(ent, start, forward, Doom_MissileDamageRandomizer(5), 850);
	Weapon_Doom_Anim_Fast(ent);

	gun->frame = 1 + (Q_rand() % 2);

	RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);
}

static void fire_doom_lead(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, meansOfDeath_t mod)
{
	trace_t     tr;
	vec3_t      dir;
	vec3_t      forward = { 0, 0, 0 }, right, up;
	vec3_t      end;
	float       r;
	float       u;
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
				
#ifdef ENABLE_COOP
				PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
#endif
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

void weapon_doom_pistol_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	if (gun->frame >= 3 && (ent->server.client->buttons & BUTTON_ATTACK) && HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon) && !gun_state->newweapon)
		gun->frame = 0;
	else
		gun->frame++;

	if (gun->frame == 1)
	{
		G_SendMuzzleFlash(ent, MZ_BLASTER);

		RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

		vec3_t start, forward, right, offset;
		AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
		int damage = 5 * (Q_rand() % 3 + 1);
		fire_doom_bullet(ent, start, forward, damage, 0, TE_DOOM_GUNSHOT, first ? 0 : 400, 0, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), ent, DT_DIRECT));
		Weapon_Doom_Anim_Regular(ent);
	}
}

#define DOOM_TIC_TO_TIME(t) (gtime_t)(floorf(((1.0f / 35.0f) * 1000) * (t - 1)))

void weapon_doom_chaingun_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	if ((gun->frame == 2 && !(ent->server.client->buttons & BUTTON_ATTACK)) || !HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon) || gun_state->newweapon)
	{
		gun->frame = 3;
		return;
	}

	G_SendMuzzleFlash(ent, MZ_BLASTER);

	RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

	vec3_t start, forward, right, offset;
	AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
	VectorSet(offset, 0, 0, ent->viewheight - 2);
	P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
	int damage = 5 * (Q_rand() % 3 + 1);
	fire_doom_bullet(ent, start, forward, damage, 0, TE_DOOM_GUNSHOT, 400, 0, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), ent, DT_DIRECT));
	gun->frame = (gun->frame == 1) ? 2 : 1;
	ent->attack_finished_time = level.time + DOOM_TIC_TO_TIME(4);
	Weapon_Doom_Anim_Fast(ent);
}

void weapon_doom_shotgun_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	gun->frame++;

	if (gun->frame == 1)
	{
		G_SendMuzzleFlash(ent, MZ_SHOTGUN);

		RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

		vec3_t start, forward, right, offset;
		AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
		int damage = 5 * (Q_rand() % 3 + 1);
		fire_doom_shotgun(ent, start, forward, damage, 0, TE_DOOM_GUNSHOT, 7, 400, 0, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), ent, DT_DIRECT));
		Weapon_Doom_Anim_Regular(ent);
	}
}

void weapon_doom_sshotgun_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	if (gun->frame >= 16 && (ent->server.client->buttons & BUTTON_ATTACK) && HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon) && !gun_state->newweapon)
		gun->frame = 0;

	gun->frame++;

	if (gun->frame == 1)
	{
		G_SendMuzzleFlash(ent, MZ_SSHOTGUN);

		RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);

		vec3_t start, forward, right, offset;
		AngleVectors(ent->server.client->v_angle, forward, NULL, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 2);
		P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
		int damage = 5 * (Q_rand() % 3 + 1);
		fire_doom_shotgun(ent, start, forward, damage, 0, TE_DOOM_GUNSHOT, 20, 1300, 900, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), ent, DT_DIRECT));
		Weapon_Doom_Anim_Regular(ent);
	}
	else if (gun->frame == 7)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("doom/DBOPN.wav"), 1, ATTN_NORM, 0);
	else if (gun->frame == 11)
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/DBLOAD.wav"), 1, ATTN_NORM, 0);
	else if (gun->frame == 15)
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/DBCLS.wav"), 1, ATTN_NORM, 0);
}

void weapon_doom_chainsaw_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	if (!(ent->server.client->buttons & BUTTON_ATTACK) || gun_state->newweapon)
	{
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWIDL.wav"), 1, ATTN_NORM, 0);
		ent->server.client->silencer_shots = 2;
		gun->frame = 5;
		return;
	}

	if (gun->frame == 2)
		gun->frame = 3;
	else
		gun->frame = 2;

	int damage = 2 * (Q_rand() % 10 + 1);
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
			MSG_WriteByte(TE_DOOM_BLOOD);
			MSG_WritePos(org);
			gi.multicast(org, MULTICAST_PVS);
			T_Damage(tr.ent, ent, ent, vec3_origin, vec3_origin, vec3_origin, damage, 0, DAMAGE_NO_PARTICLES, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->server.client->pers.weapon), ent, DT_DIRECT));
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
	self->server.state.frame++;

	if (self->server.state.frame == 2 && !self->activator)
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
			angles[1] = self->server.state.angles[1] - move_left + angle_mul;
			vec3_t forward;
			AngleVectors(angles, forward, NULL, NULL);
			vec3_t start, end;
			VectorCopy(self->server.owner->server.state.origin, start);
			VectorMA(start, 1024, forward, end);
			trace_t tr = gi.trace(start, vec3_origin, vec3_origin, end, self->server.owner, CONTENTS_SOLID | CONTENTS_WINDOW | CONTENTS_MONSTER);

			if (tr.fraction == 1.0f || !tr.ent)
				continue;

			/*P_SpawnMobj(linetarget->x,
				linetarget->y,
				linetarget->z + (linetarget->height >> 2),
				MT_EXTRABFG);*/
			MSG_WriteByte(svc_temp_entity);
			MSG_WriteByte(TE_BFG_EXPLOSION);
			MSG_WritePos(tr.ent->server.state.origin);
			gi.multicast(tr.ent->server.state.origin, MULTICAST_PHS);
			damage = 0;

			for (j = 0; j < 15; j++)
				damage += (Q_rand() & 7) + 1;

			T_Damage(tr.ent, self, self->server.owner, vec3_origin, vec3_origin, vec3_origin, damage, 0, DAMAGE_NO_PARTICLES, self->meansOfDeath);
		}
	}

	if (self->server.state.frame == 6)
		G_FreeEdict(self);
}

void bfg_doom_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
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

	// core explosion - prevents firing it into the wall/floor
	if (other->takedamage)
		T_Damage(other, self, self->server.owner, self->velocity, self->server.state.origin, plane->normal, self->dmg, 0, DAMAGE_NONE, self->meansOfDeath);

	gi.sound(self, CHAN_VOICE, gi.soundindex("doom/RXPLOD.wav"), 1, ATTN_NORM, 0);
	self->server.solid = SOLID_NOT;
	self->touch = NULL;
	VectorMA(self->server.state.origin, -1 * game.frameseconds, self->velocity, self->server.state.origin);
	VectorClear(self->velocity);
	self->server.state.modelindex = gi.modelindex("sprites/doom/BFE1.d2s");
	self->server.state.frame = 0;
	self->server.state.sound = 0;
	self->server.state.effects &= ~EF_ANIM_ALLFAST;
	self->think = bfg_doom_explode;
	self->nextthink = level.time + (game.frametime * 2);
	self->activator = NULL;
	self->enemy = other;
}

void fire_doom_bfg(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t *bfg;
	bfg = G_Spawn();
	VectorCopy(start, bfg->server.state.origin);
	VectorCopy(dir, bfg->movedir);
	bfg->server.state.angles[1] = (self->server.client ? self->server.client->v_angle[1] : self->server.state.angles[1]);
	VectorScale(dir, speed, bfg->velocity);
	bfg->movetype = MOVETYPE_FLYMISSILE;
	bfg->server.clipmask = MASK_SHOT;
	bfg->server.solid = SOLID_BBOX;
	bfg->server.state.effects |= EF_ANIM_ALLFAST;
	bfg->server.state.renderfx |= RF_FULLBRIGHT;
	VectorClear(bfg->server.mins);
	VectorClear(bfg->server.maxs);
	bfg->server.state.modelindex = gi.modelindex("sprites/doom/BFS1.d2s");
	bfg->server.owner = self;
	bfg->touch = bfg_doom_touch;
	bfg->nextthink = level.time + (8000000.0f / speed);
	bfg->think = G_FreeEdict;
	bfg->dmg = damage;
	bfg->meansOfDeath = MakeWeaponMeansOfDeath(self, GetIndexByItem(self->server.client->pers.weapon), bfg, DT_DIRECT);

#if ENABLE_COOP
	check_dodge(self, bfg->server.state.origin, dir, speed);
#endif

	gi.linkentity(bfg);
}

void weapon_doom_bfg_fire(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state, bool first)
{
	if (ent->server.client->doom_bfg_delay > 0)
	{
		if (gun->frame >= 3 && !gun_state->newweapon && (ent->server.client->buttons & BUTTON_ATTACK) && HasEnoughAmmoToFire(ent, ent->server.client->pers.weapon))
		{
			gun->frame = 0;
			ent->server.client->doom_bfg_delay = 0;
			goto refire;
		}

		ent->server.client->doom_bfg_delay--;
	}
	else
	{
refire:

		switch (gun->frame)
		{
			case 0:
				G_SendMuzzleFlash(ent, MZ_BFG);
				ent->server.client->doom_bfg_delay = 5;
				break;

			case 1:
				ent->server.client->doom_bfg_delay = 2;
				break;

			case 2:
				ent->server.client->doom_bfg_delay = 2;
				{
					vec3_t forward, right, start, offset;
					AngleVectors(ent->server.client->v_angle, forward, right, NULL);
					VectorSet(offset, 8, 0, ent->viewheight - 8);
					P_ProjectSource(ent->server.client, ent->server.state.origin, offset, forward, right, start);
					fire_doom_bfg(ent, start, forward, Doom_MissileDamageRandomizer(100), 850);
					Weapon_Doom_Anim_Regular(ent);

					RemoveAmmoFromFiring(ent, ent->server.client->pers.weapon);
				}
				break;

			case 3:
				ent->server.client->doom_bfg_delay = 8;
				break;
		}

		gun->frame++;
	}
}

static void Weapon_Doom_Fist(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Doom(ent, 6, weapon_doom_punch_fire, gun, gun_state);
}

static void Weapon_Doom_Pistol(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Doom(ent, 6, weapon_doom_pistol_fire, gun, gun_state);
}

static void Weapon_Doom_Shotgun(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Doom(ent, 10, weapon_doom_shotgun_fire, gun, gun_state);
}

static void Weapon_Doom_SuperShotgun(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Doom(ent, 17, weapon_doom_sshotgun_fire, gun, gun_state);
}

static void Weapon_Doom_Chaingun(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Doom(ent, 3, weapon_doom_chaingun_fire, gun, gun_state);
}

static void Weapon_Doom_RocketLauncher(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Doom(ent, 8, weapon_doom_rocket_fire, gun, gun_state);
}

static void Weapon_Doom_PlasmaGun(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	Weapon_Doom(ent, 4, weapon_doom_hyperblaster_fire, gun, gun_state);
}

static void Weapon_Doom_BFG(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	if (gun_state->weaponstate != WEAPON_FIRING)
		ent->server.client->doom_bfg_delay = 0;

	Weapon_Doom(ent, 5, weapon_doom_bfg_fire, gun, gun_state);
}

static void Weapon_Doom_Chainsaw(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	if (gun_state->weaponstate == WEAPON_ACTIVATING && ent->server.client->silencer_shots == 0)
	{
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWUP.wav"), 1, ATTN_NORM, 0);
		ent->server.client->silencer_shots = 1;
	}
	else if (gun_state->weaponstate == WEAPON_READY)
	{
		if (--ent->server.client->silencer_shots <= 0 && !(ent->server.client->buttons & BUTTON_ATTACK))
		{
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("doom/SAWIDL.wav"), 1, ATTN_NORM, 0);
			ent->server.client->silencer_shots = 2;
		}

		gun->frame ^= 1;

		if (gun->frame >= 2)
			gun->frame = 0;
	}
	else if (gun_state->weaponstate == WEAPON_DROPPING)
		ent->server.client->silencer_shots = 0;

	Weapon_Doom(ent, 4, weapon_doom_chainsaw_fire, gun, gun_state);
}

static G_WeaponRunFunc doom_weapon_run_funcs[] =
{
	[ITI_DOOM_FIST - ITI_WEAPONS_START] = Weapon_Doom_Fist,
	[ITI_DOOM_PISTOL - ITI_WEAPONS_START] = Weapon_Doom_Pistol,
	[ITI_DOOM_SHOTGUN - ITI_WEAPONS_START] = Weapon_Doom_Shotgun,
	[ITI_DOOM_SUPER_SHOTGUN - ITI_WEAPONS_START] = Weapon_Doom_SuperShotgun,
	[ITI_DOOM_CHAINGUN - ITI_WEAPONS_START] = Weapon_Doom_Chaingun,
	[ITI_DOOM_ROCKET_LAUNCHER - ITI_WEAPONS_START] = Weapon_Doom_RocketLauncher,
	[ITI_DOOM_PLASMA_GUN - ITI_WEAPONS_START] = Weapon_Doom_PlasmaGun,
	[ITI_DOOM_BFG - ITI_WEAPONS_START] = Weapon_Doom_BFG,
	[ITI_DOOM_CHAINSAW - ITI_WEAPONS_START] = Weapon_Doom_Chainsaw
};

void Weapon_Doom_Run(edict_t *ent, player_gun_t *gun, gun_state_t *gun_state)
{
	doom_weapon_run_funcs[ITEM_INDEX(ent->server.client->pers.weapon) - ITI_WEAPONS_START](ent, gun, gun_state);
}
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
// g_weapon.c

#include "g_local.h"
#include "m_player.h"

// Doom
void weapon_doom_rocket_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_doom_punch_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_doom_hyperblaster_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_doom_pistol_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_doom_chaingun_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_doom_shotgun_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_doom_sshotgun_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_doom_chainsaw_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_doom_bfg_fire(edict_t *ent, gunindex_e gun, bool first);
void Weapon_Doom(edict_t *ent, int num_frames, void(*fire)(edict_t *ent, gunindex_e gun, bool first), gunindex_e gun);

void weapon_q1_gl_fire(edict_t *ent, gunindex_e gun);
void weapon_q1_rl_fire(edict_t *ent, gunindex_e gun);
void weapon_q1_axe_fire(edict_t *ent, gunindex_e gun);
void weapon_q1_lightning_fire(edict_t *ent, gunindex_e gun);
void weapon_q1_nailgun_fire(edict_t *ent, gunindex_e gun);
void weapon_q1_snailgun_fire(edict_t *ent, gunindex_e gun);
void weapon_q1_shotgun_fire(edict_t *ent, gunindex_e gun);
void weapon_q1_sshotgun_fire(edict_t *ent, gunindex_e gun);
void Weapon_Q1(edict_t *ent, int num_frames, void(*fire)(edict_t *ent, gunindex_e gun), gunindex_e gun);

void weapon_duke_pipebomb_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_duke_detonator_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_duke_devastate_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_duke_rocket_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_duke_foot_fire(edict_t *ent, gunindex_e gun, bool first);
void Weapon_Duke_Offhand_Foot(edict_t *ent);
void weapon_duke_freezer_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_duke_pistol_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_duke_chaingun_fire(edict_t *ent, gunindex_e gun, bool first);
void weapon_duke_shotgun_fire(edict_t *ent, gunindex_e gun, bool first);

extern int pipe_detonator_index;

bool     is_quad;
byte     is_silenced;

void P_ProjectSource(gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t  _distance;
	VectorCopy(distance, _distance);

	if (client->pers.hand == LEFT_HANDED)
		_distance[1] *= -1;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;

	G_ProjectSource(point, _distance, forward, right, result);
}

void Weapon_QuadDamage(edict_t *ent)
{
	if (ent->client->quad_time > level.time)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
}

/*
===============
PlayerNoise

Each player can have two noise objects associated with it:
a personal noise (jumping, pain, weapon firing), and a weapon
target noise (bullet wall impacts)

Monsters that don't directly see the player can move
to a noise in hopes of seeing the player from there.
===============
*/
void PlayerNoise(edict_t *who, vec3_t where, int type)
{
	edict_t     *noise;

	if (type == PNOISE_WEAPON)
	{
		if (who->client->silencer_shots)
		{
			who->client->silencer_shots--;
			return;
		}
	}

	if (deathmatch->value)
		return;

	//if (who->flags & FL_NOTARGET)
	//	return;

	if (!who->mynoise)
	{
		noise = G_Spawn();
		noise->entitytype = ET_PLAYER_NOISE;
		VectorSet(noise->mins, -8, -8, -8);
		VectorSet(noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise = noise;
		noise = G_Spawn();
		noise->entitytype = ET_PLAYER_NOISE;
		VectorSet(noise->mins, -8, -8, -8);
		VectorSet(noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise2 = noise;
	}

	if (type == PNOISE_SELF || type == PNOISE_WEAPON)
	{
		noise = who->mynoise;
		level.sound_entity = noise;
		level.sound_entity_time = level.time + 100;
	}
	else   // type == PNOISE_IMPACT
	{
		noise = who->mynoise2;
		level.sound2_entity = noise;
		level.sound2_entity_time = level.time + 100;
	}

	VectorCopy(where, noise->s.origin);
	VectorSubtract(where, noise->maxs, noise->absmin);
	VectorAdd(where, noise->maxs, noise->absmax);
	noise->teleport_time = level.time;
	gi.linkentity(noise);
}

extern bool do_propagate;

bool ShouldSwapToPotentiallyBetterWeapon(edict_t *ent, gitem_t *new_item, bool is_new)
{
	bool weapon_uses_ammo = EntityGame(ent)->ammo_usages[GetIndexByItem(ent->client->pers.weapon)] > 0;
	bool swap_back_to_pipebombs;

	switch (ent->s.game)
	{
		case GAME_Q2:
			if (deathmatch->value && weapon_uses_ammo)
				return false;
			// switch to Grenades if we have none yet
			// or we have a Blaster out
			else if ((new_item && (new_item->flags & IT_AMMO) && is_new) || !weapon_uses_ammo)
				return true;

			break;

		case GAME_Q1:
			if (deathmatch->value && weapon_uses_ammo && GetIndexByItem(ent->client->pers.weapon) != ITI_Q1_SHOTGUN)
				return false;

			break;

		case GAME_DOOM:
			if (deathmatch->value && weapon_uses_ammo && GetIndexByItem(ent->client->pers.weapon) != ITI_DOOM_PISTOL)
				return false;

			break;

		case GAME_DUKE:
			swap_back_to_pipebombs = GetIndexByItem(ent->client->pers.weapon) == ITI_DUKE_PIPEBOMBS &&
				ent->client->pers.alt_weapon &&
				GetIndexByItem(new_item) == ITI_DUKE_PIPEBOMBS;

			if (swap_back_to_pipebombs)
				return true;
			else if (deathmatch->value && weapon_uses_ammo && GetIndexByItem(ent->client->pers.weapon) != ITI_DUKE_PISTOL)
				return false;
			else if (new_item != NULL && (new_item->flags & IT_AMMO) && is_new)
				return false;

			break;
	}

	return !new_item || ent->client->pers.weapon != new_item;
}

bool Pickup_Weapon(edict_t *ent, edict_t *other)
{
	int         index;
	bool		is_new;
	index = ITEM_INDEX(ent->item);

	if (!invasion->value && (((int)(dmflags->value) & DF_WEAPONS_STAY) || coop->value) && other->client->pers.inventory[index])
	{
		if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
			return false;
	}

	is_new = !other->client->pers.inventory[index];

	if (!other->client->pers.inventory[index])
		other->client->pers.inventory[index]++;

	if (!(ent->spawnflags & DROPPED_ITEM) || ent->count)
	{
		// give them some ammo with it
		if (EntityGame(other)->ammo_pickups[index] > 0 &&
			EntityGame(other)->ammo_usages[index] > 0)
		{
			float real_num;

			if (ent->count)
				real_num = ent->count; // TODO: float (use pack_ammo maybe?)
			else
				real_num = EntityGame(other)->ammo_pickups[index] * EntityGame(other)->ammo_usages[index];

			if ((int)dmflags->value & DF_INFINITE_AMMO)
				Add_Ammo(other, 1000, index, false);
			else
				Add_Ammo(other, real_num, index, false);
		}

		if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
		{
			if (deathmatch->value)
			{
				if ((int)(dmflags->value) & DF_WEAPONS_STAY)
					ent->flags |= FL_RESPAWN;
				else
					SetRespawn(ent, 30);
			}
			else if (coop->value)
				ent->flags |= FL_RESPAWN;
		}
	}

	if (ShouldSwapToPotentiallyBetterWeapon(other, ent->item, is_new))
	{
		gitem_t *best = GetBestWeapon(other, ent->item, is_new);

		if (best && best == ent->item)
			other->client->gunstates[GUN_MAIN].newweapon = best;
	}

	// in invasion, propagate item to other clients
	if (invasion->value && do_propagate)
	{
		int i;
		do_propagate = false;

		for (i = 0; i < game.maxclients; ++i)
		{
			edict_t *cl = &g_edicts[i + 1];

			if (!cl->inuse || !cl->client->pers.connected || !cl->s.game || cl == other)
				continue;

			Pickup_Weapon(ent, cl);
		}

		do_propagate = true;
	}

	// only the pickup duke player will have a quip
	if (other->s.game == GAME_DUKE && do_propagate)
	{
		int r = Q_rand() % 256;
		const char *sound;

		if (r < 64)
			sound = "duke/cool01.wav";
		else if (r < 96)
			sound = "duke/getsom1a.wav";
		else if (r < 128)
			sound = "duke/groovy02.wav";
		else if (r < 140)
			sound = "duke/wansom4a.wav";
		else
			sound = "duke/hail01.wav";

		gi.sound(other, CHAN_VOICE, gi.soundindex(sound), 1, ATTN_NORM, 0);
	}

	return true;
}

#define GRENADE_TIMER       3000.0f
#define GRENADE_MINSPEED    400.0f
#define GRENADE_MAXSPEED    800.0f

void weapon_grenade_fire(edict_t *ent, gunindex_e gun, bool held)
{
	vec3_t  offset;
	vec3_t  forward, right;
	vec3_t  start;
	int     damage = 125;
	int     speed;
	float   radius;
	radius = damage + 40;

	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	gtime_t timer;

	if (ent->client->grenade_time > level.time)
		timer = ent->client->grenade_time - level.time;
	else
		timer = 0;

	speed = GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER);
	fire_grenade2(ent, start, forward, damage, speed, timer, radius, held);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

	ent->client->grenade_time = level.time + 1000;

	if (ent->deadflag || !ent->solid)   // VWep animations screw up corpses
		return;

	if (ent->health <= 0)
		return;

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->client->anim_priority = ANIM_ATTACK;
		ent->s.frame = FRAME_crattak1 - 1;
		ent->client->anim_end = FRAME_crattak3;
	}
	else
	{
		ent->client->anim_priority = ANIM_WAVE | ANIM_REVERSE;
		ent->s.frame = FRAME_wave08;
		ent->client->anim_end = FRAME_wave01;
	}
}


/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
void ChangeWeapon(edict_t *ent, gunindex_e gun)
{
	int i;

	if (ent->client->grenade_time)
	{
		ent->client->grenade_time = level.time;
		ent->client->weapon_sound = 0;
		weapon_grenade_fire(ent, gun, false);
		ent->client->grenade_time = 0;
	}

	bool first = !ent->client->pers.lastweapon;

	if (ent->client->pers.weapon != ent->client->pers.lastweapon)
		ent->client->pers.lastweapon = ent->client->pers.weapon;

	gitem_t *old_weapon = ent->client->pers.weapon;
	ent->client->pers.weapon = ent->client->gunstates[gun].newweapon;
	ent->client->gunstates[gun].newweapon = NULL;
	ent->client->gunstates[gun].machinegun_shots = 0;

	// set visible model
	if (ent->s.modelindex == 255)
	{
		if (ent->client->pers.weapon)
			i = (((ITI_WEAPONS_START - ITEM_INDEX(ent->client->pers.weapon) + 1) & 0xff) << 8);
		else
			i = 0;

		ent->s.skinnum = (ent - g_edicts - 1) | i;
	}

	if (!ent->client->pers.weapon)
	{
		// dead
		memset(ent->client->ps.guns, 0, sizeof(ent->client->ps.guns));
		return;
	}

	ent->client->gunstates[gun].weaponstate = WEAPON_ACTIVATING;
	ent->client->ps.guns[gun].frame = 0;
	ent->client->ps.guns[gun].index = gi.modelindex(ent->client->pers.weapon->view_model);

	switch (ent->s.game)
	{
		case GAME_Q2:
			ent->client->anim_priority = ANIM_PAIN;

			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain1;
				ent->client->anim_end = FRAME_crpain4;
			}
			else
			{
				ent->s.frame = FRAME_pain301;
				ent->client->anim_end = FRAME_pain304;
			}

			break;

		case GAME_Q1:
			ent->client->force_anim = true;
			break;

		case GAME_DOOM:
		case GAME_DUKE:
			if (first)
			{
				//ent->client->next_weapon_update = ent->client->player_time;
				if (GetIndexByItem(ent->client->pers.weapon) == ITI_DUKE_PISTOL &&
					!ent->client->pers.pistol_clip &&
					HasEnoughAmmoToFire(ent, ent->client->pers.weapon))
				{
					ent->client->gunstates[gun].weaponstate = WEAPON_FIRING;
					ent->client->ps.guns[gun].frame = 14;
				}
			}

			// special case for pipebomb
			if (GetIndexByItem(ent->client->pers.weapon) == ITI_DUKE_PIPEBOMBS)
			{
				if (!ent->client->pers.inventory[GetIndexByItem(ent->client->pers.weapon)])
					ent->client->pers.alt_weapon = true;
				else if (GetIndexByItem(old_weapon) == ITI_DUKE_PIPEBOMBS)
					ent->client->pers.alt_weapon = !ent->client->pers.alt_weapon;
				else
				{
					if (ent->client->pers.inventory[GetIndexByItem(ent->client->pers.weapon)])
						ent->client->pers.alt_weapon = false;
					else
						ent->client->pers.alt_weapon = true;
				}

				if (ent->client->pers.alt_weapon)
					ent->client->ps.guns[gun].index = pipe_detonator_index;
			}
			else
				ent->client->pers.alt_weapon = false;

			break;
	}
}

/*static bool HasAmmoCount(edict_t *ent, itemid_e ammo_index, itemid_e new_item_index, int count)
{
	if (ent->client->pers.inventory[ammo_index] >= count)
		return true;

	if (new_item_index == ammo_index)
	{
		//int pickup_count = game_iteminfos[ent->s.game].dynamic.ammo_pickup_amounts[new_item_index];
		//return pickup_count >= count;
	}

	return false;
}*/

//#define HasAmmo(ent, ammo_index, new_item_index) HasAmmoCount(ent, ammo_index, new_item_index, 1)

static bool HasWeapon(edict_t *ent, itemid_e weapon_index, itemid_e new_item_index)
{
	return (ent->client->pers.inventory[weapon_index] || new_item_index == weapon_index) && HasEnoughAmmoToFire(ent, GetItemByIndex(weapon_index));
}

#define HasAmmoCountQuick(weapon, count) HasEnoughAmmoToFireShots(ent, GetItemByIndex(weapon), count)
//#define HasAmmoQuick(type) HasAmmo(ent, type, new_item_index)
#define HasWeaponQuick(type) HasWeapon(ent, type, new_item_index)

static itemid_e dukeWeaponChoice[] =
{
	ITI_DUKE_SHOTGUN,
	ITI_DUKE_CANNON,
	ITI_DUKE_RPG,
	//ITI_DUKE_SHRINKER,
	ITI_DUKE_DEVASTATOR,
	ITI_DUKE_PIPEBOMBS,
	ITI_DUKE_FREEZER,
	ITI_DUKE_PISTOL,
	//ITI_DUKE_TRIPBOMBS
};

gitem_t *GetBestWeapon(edict_t *ent, gitem_t *new_item, bool is_new)
{
	itemid_e new_item_index = GetIndexByItem(new_item);
	int i;
	itemid_e weap;

	switch (ent->s.game)
	{
		case GAME_Q2:
			if (HasWeaponQuick(ITI_Q2_RAILGUN))
				return GetItemByIndex(ITI_Q2_RAILGUN);
			else if (HasWeaponQuick(ITI_Q2_HYPERBLASTER))
				return GetItemByIndex(ITI_Q2_HYPERBLASTER);
			else if ((HasAmmoCountQuick(ITI_Q2_MACHINEGUN, 25) || !HasWeaponQuick(ITI_Q2_MACHINEGUN)) && HasWeaponQuick(ITI_Q2_CHAINGUN))
				return GetItemByIndex(ITI_Q2_CHAINGUN);
			else if (HasWeaponQuick(ITI_Q2_MACHINEGUN))
				return GetItemByIndex(ITI_Q2_MACHINEGUN);
			else if (HasWeaponQuick(ITI_Q2_SUPER_SHOTGUN))
				return GetItemByIndex(ITI_Q2_SUPER_SHOTGUN);
			else if (HasWeaponQuick(ITI_Q2_SHOTGUN))
				return GetItemByIndex(ITI_Q2_SHOTGUN);
			else if (HasWeaponQuick(ITI_Q2_ROCKET_LAUNCHER))
				return GetItemByIndex(ITI_Q2_ROCKET_LAUNCHER);
			else if (HasWeaponQuick(ITI_Q2_GRENADE_LAUNCHER))
				return GetItemByIndex(ITI_Q2_GRENADE_LAUNCHER);
			else if (HasWeaponQuick(ITI_Q2_BFG10K))
				return GetItemByIndex(ITI_Q2_BFG10K);
			else if (HasWeaponQuick(ITI_Q2_GRENADES))
				return GetItemByIndex(ITI_Q2_GRENADES);

			break;

		case GAME_Q1:
			if (ent->waterlevel <= 1 && HasWeaponQuick(ITI_Q1_THUNDERBOLT))
				return GetItemByIndex(ITI_Q1_THUNDERBOLT);
			else if (HasWeaponQuick(ITI_Q1_SUPER_NAILGUN))
				return GetItemByIndex(ITI_Q1_SUPER_NAILGUN);
			else if (HasWeaponQuick(ITI_Q1_SUPER_SHOTGUN))
				return GetItemByIndex(ITI_Q1_SUPER_SHOTGUN);
			else if (HasWeaponQuick(ITI_Q1_NAILGUN))
				return GetItemByIndex(ITI_Q1_NAILGUN);
			else if (HasWeaponQuick(ITI_Q1_SHOTGUN))
				return GetItemByIndex(ITI_Q1_SHOTGUN);

			break;

		case GAME_DOOM:
			if (HasWeaponQuick(ITI_DOOM_PLASMA_GUN))
				return GetItemByIndex(ITI_DOOM_PLASMA_GUN);
			else if (HasWeaponQuick(ITI_DOOM_SUPER_SHOTGUN))
				return GetItemByIndex(ITI_DOOM_SUPER_SHOTGUN);
			else if (HasWeaponQuick(ITI_DOOM_CHAINGUN))
				return GetItemByIndex(ITI_DOOM_CHAINGUN);
			else if (HasWeaponQuick(ITI_DOOM_SHOTGUN))
				return GetItemByIndex(ITI_DOOM_SHOTGUN);
			else if (HasWeaponQuick(ITI_DOOM_PISTOL))
				return GetItemByIndex(ITI_DOOM_PISTOL);
			else if (HasWeaponQuick(ITI_DOOM_CHAINSAW))
				return GetItemByIndex(ITI_DOOM_CHAINSAW);
			else if (HasWeaponQuick(ITI_DOOM_ROCKET_LAUNCHER))
				return GetItemByIndex(ITI_DOOM_ROCKET_LAUNCHER);
			else if (HasWeaponQuick(ITI_DOOM_BFG))
				return GetItemByIndex(ITI_DOOM_BFG);

			break;

		case GAME_DUKE:
			weap = ITI_DUKE_FOOT;

			for (i = 0; i < q_countof(dukeWeaponChoice); ++i)
			{
				weap = dukeWeaponChoice[i];

				if (HasWeaponQuick(weap))
					break;
			}

			return GetItemByIndex(weap);
	}

	return GetItemByIndex(ITI_Q2_BLASTER);
}

void AttemptBetterWeaponSwap(edict_t *ent)
{
	if (ShouldSwapToPotentiallyBetterWeapon(ent, NULL, false))
		NoAmmoWeaponChange(ent, GUN_MAIN);
}

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange(edict_t *ent, gunindex_e gun)
{
	gitem_t *best = GetBestWeapon(ent, NULL, false);

	if (!best || best == ent->client->pers.weapon)
		return;

	ent->client->gunstates[gun].newweapon = best;
}

/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon(edict_t *ent, gitem_t *item)
{
	// see if we're already using it
	// and we're not pipebombs, which Duke can always switch to
	if (item == ent->client->pers.weapon && !(ent->s.game == GAME_DUKE && GetIndexByItem(item) == ITI_DUKE_PIPEBOMBS))
		return;

	if (!g_select_empty->value && !(item->flags & IT_AMMO) && EntityGame(ent)->ammo_usages[ITEM_INDEX(item)] > 0)
	{
		if (!ent->client->pers.ammo)
		{
			gi.cprintf(ent, PRINT_HIGH, "No ammo for %s.\n", item->pickup_name);
			return;
		}

		if (ent->client->pers.ammo < GetWeaponUsageCount(ent, item))
		{
			gi.cprintf(ent, PRINT_HIGH, "Not enough ammo for %s.\n", item->pickup_name);
			return;
		}
	}

	// change to this weapon when down
	ent->client->gunstates[GUN_MAIN].newweapon = item;
}

/*
================
Drop_Weapon
================
*/
void Drop_Weapon(edict_t *ent, gitem_t *item)
{
	int     index;

	if ((int)(dmflags->value) & DF_WEAPONS_STAY)
		return;

	index = ITEM_INDEX(item);

	// see if we're already using it
	if (((item == ent->client->pers.weapon) || (item == ent->client->gunstates[GUN_MAIN].newweapon)) && (ent->client->pers.inventory[index] == 1))
	{
		gi.cprintf(ent, PRINT_HIGH, "Can't drop current weapon\n");
		return;
	}

	Drop_Item(ent, item);
	ent->client->pers.inventory[index]--;
}


/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/
#define FRAME_FIRE_FIRST        (FRAME_ACTIVATE_LAST + 1)
#define FRAME_IDLE_FIRST        (FRAME_FIRE_LAST + 1)
#define FRAME_DEACTIVATE_FIRST  (FRAME_IDLE_LAST + 1)

void Weapon_Generic(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, int *pause_frames, int *fire_frames, void(*fire)(edict_t *ent, gunindex_e gun), gunindex_e gun)
{
	int     n;

	if (ent->deadflag || !ent->solid || ent->freeze_time > level.time)   // VWep animations screw up corpses
		return;

	/*	if (ent->client->next_weapon_update > ent->client->player_time)
			return;

		ent->client->next_weapon_update = ent->client->player_time + 100;*/

	if (ent->client->gunstates[gun].weaponstate == WEAPON_DROPPING)
	{
		if (ent->client->ps.guns[gun].frame == FRAME_DEACTIVATE_LAST)
		{
			ChangeWeapon(ent, gun);
			return;
		}
		else if ((FRAME_DEACTIVATE_LAST - ent->client->ps.guns[gun].frame) == 4)
		{
			ent->client->anim_priority = ANIM_WAVE | ANIM_REVERSE;

			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4 + 1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304 + 1;
				ent->client->anim_end = FRAME_pain301;
			}
		}

		ent->client->ps.guns[gun].frame++;
		return;
	}

	if (ent->client->gunstates[gun].weaponstate == WEAPON_ACTIVATING)
	{
		if (ent->client->ps.guns[gun].frame == FRAME_ACTIVATE_LAST)
		{
			ent->client->gunstates[gun].weaponstate = WEAPON_READY;
			ent->client->ps.guns[gun].frame = FRAME_IDLE_FIRST;
			return;
		}

		ent->client->ps.guns[gun].frame++;
		return;
	}

	if ((ent->client->gunstates[gun].newweapon) && (ent->client->gunstates[gun].weaponstate != WEAPON_FIRING))
	{
		ent->client->gunstates[gun].weaponstate = WEAPON_DROPPING;
		ent->client->ps.guns[gun].frame = FRAME_DEACTIVATE_FIRST;

		if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
		{
			ent->client->anim_priority = ANIM_WAVE | ANIM_REVERSE;

			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4 + 1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304 + 1;
				ent->client->anim_end = FRAME_pain301;
			}
		}

		return;
	}

	if (ent->client->gunstates[gun].weaponstate == WEAPON_READY)
	{
		if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK))
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;

			if ((EntityGame(ent)->ammo_usages[ITEM_INDEX(ent->client->pers.weapon)] <= 0) ||
				(ent->client->pers.ammo >= GetWeaponUsageCount(ent, ent->client->pers.weapon)))
			{
				ent->client->ps.guns[gun].frame = FRAME_FIRE_FIRST;
				ent->client->gunstates[gun].weaponstate = WEAPON_FIRING;
				// start the animation
				ent->client->anim_priority = ANIM_ATTACK;

				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent->s.frame = FRAME_crattak1 - 1;
					ent->client->anim_end = FRAME_crattak9;
				}
				else
				{
					ent->s.frame = FRAME_attack1 - 1;
					ent->client->anim_end = FRAME_attack8;
				}
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1000;
				}

				NoAmmoWeaponChange(ent, gun);
			}
		}
		else
		{
			if (ent->client->ps.guns[gun].frame == FRAME_IDLE_LAST)
			{
				ent->client->ps.guns[gun].frame = FRAME_IDLE_FIRST;
				return;
			}

			if (pause_frames)
			{
				for (n = 0; pause_frames[n]; n++)
				{
					if (ent->client->ps.guns[gun].frame == pause_frames[n])
					{
						if (Q_rand() & 15)
							return;
					}
				}
			}

			ent->client->ps.guns[gun].frame++;
			return;
		}
	}

	if (ent->client->gunstates[gun].weaponstate == WEAPON_FIRING)
	{
		for (n = 0; fire_frames[n]; n++)
		{
			if (ent->client->ps.guns[gun].frame == fire_frames[n])
			{
				Weapon_QuadDamage(ent);
				fire(ent, gun);
				break;
			}
		}

		if (!fire_frames[n])
			ent->client->ps.guns[gun].frame++;

		if (ent->client->ps.guns[gun].frame == FRAME_IDLE_FIRST + 1)
			ent->client->gunstates[gun].weaponstate = WEAPON_READY;
	}
}


/*
======================================================================

GRENADE

======================================================================
*/

void Weapon_Grenade(edict_t *ent, gunindex_e gun)
{
	if ((ent->client->gunstates[gun].newweapon) && (ent->client->gunstates[gun].weaponstate == WEAPON_READY))
	{
		ChangeWeapon(ent, gun);
		return;
	}

	if (ent->client->next_weapon_update > ent->client->player_time)
	{
		if (!(ent->client->gunstates[gun].weaponstate == WEAPON_FIRING &&
				ent->client->ps.guns[gun].frame == 11 &&
				!(level.time >= ent->client->grenade_time)))
			return;
	}

	ent->client->next_weapon_update = ent->client->player_time + 1;

	if (ent->client->gunstates[gun].weaponstate == WEAPON_ACTIVATING)
	{
		ent->client->gunstates[gun].weaponstate = WEAPON_READY;
		ent->client->ps.guns[gun].frame = 16;
		return;
	}

	if (ent->client->gunstates[gun].weaponstate == WEAPON_READY)
	{
		if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK))
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;

			if (HasEnoughAmmoToFire(ent, ent->client->pers.weapon))
			{
				ent->client->ps.guns[gun].frame = 1;
				ent->client->gunstates[gun].weaponstate = WEAPON_FIRING;
				ent->client->grenade_time = 0;
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1000;
				}

				NoAmmoWeaponChange(ent, gun);
			}

			return;
		}

		if ((ent->client->ps.guns[gun].frame == 29) || (ent->client->ps.guns[gun].frame == 34) || (ent->client->ps.guns[gun].frame == 39) || (ent->client->ps.guns[gun].frame == 48))
		{
			if (Q_rand() & 15)
				return;
		}

		if (++ent->client->ps.guns[gun].frame > 48)
			ent->client->ps.guns[gun].frame = 16;

		return;
	}

	if (ent->client->gunstates[gun].weaponstate == WEAPON_FIRING)
	{
		if (ent->client->ps.guns[gun].frame == 5)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/hgrena1b.wav"), 1, ATTN_NORM, 0);

		if (ent->client->ps.guns[gun].frame == 11)
		{
			if (!ent->client->grenade_time)
			{
				ent->client->grenade_time = level.time + GRENADE_TIMER + 200;
				ent->client->weapon_sound = gi.soundindex("weapons/hgrenc1b.wav");
			}

			// they waited too long, detonate it in their hand
			if (!ent->client->grenade_blew_up && level.time >= ent->client->grenade_time)
			{
				ent->client->weapon_sound = 0;
				weapon_grenade_fire(ent, gun, true);
				ent->client->grenade_blew_up = true;
			}

			if (ent->client->buttons & BUTTON_ATTACK)
				return;

			if (ent->client->grenade_blew_up)
			{
				if (level.time >= ent->client->grenade_time)
				{
					ent->client->ps.guns[gun].frame = 15;
					ent->client->grenade_blew_up = false;
				}
				else
					return;
			}
		}

		if (ent->client->ps.guns[gun].frame == 12)
		{
			ent->client->weapon_sound = 0;
			weapon_grenade_fire(ent, gun, false);
		}

		if ((ent->client->ps.guns[gun].frame == 15) && (level.time < ent->client->grenade_time))
			return;

		ent->client->ps.guns[gun].frame++;

		if (ent->client->ps.guns[gun].frame == 16)
		{
			ent->client->grenade_time = 0;
			ent->client->gunstates[gun].weaponstate = WEAPON_READY;
		}
	}
}

/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void weapon_grenadelauncher_fire(edict_t *ent, gunindex_e gun)
{
	vec3_t  offset;
	vec3_t  forward, right;
	vec3_t  start;
	int     damage = 120;
	float   radius;
	radius = damage + 40;

	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	VectorScale(forward, -2, ent->client->gunstates[gun].kick_origin);
	ent->client->gunstates[gun].kick_angles[0] = -1;
	ent->client->gunstates[gun].kick_time = level.time + 100;
	fire_grenade(ent, start, forward, damage, 600, 2.5, radius);
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_GRENADE | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
	ent->client->ps.guns[gun].frame++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);
}

void Weapon_GrenadeLauncher(edict_t *ent, gunindex_e gun)
{
	static int  pause_frames[] = { 34, 51, 59, 0 };
	static int  fire_frames[] = { 6, 0 };
	Weapon_Generic(ent, 5, 16, 59, 64, pause_frames, fire_frames, weapon_grenadelauncher_fire, gun);
}

/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire(edict_t *ent, gunindex_e gun)
{
	vec3_t  offset, start;
	vec3_t  forward, right;
	int     damage;
	float   damage_radius;
	int     radius_damage;
	damage = 100 + (int)(random() * 20.0f);
	radius_damage = 120;
	damage_radius = 120;

	if (is_quad)
	{
		damage *= 4;
		radius_damage *= 4;
	}

	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorScale(forward, -2, ent->client->gunstates[gun].kick_origin);
	ent->client->gunstates[gun].kick_angles[0] = -1;
	ent->client->gunstates[gun].kick_time = level.time + 100;
	VectorSet(offset, 8, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	fire_rocket(ent, start, forward, damage, 650, damage_radius, radius_damage);
	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_ROCKET | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
	ent->client->ps.guns[gun].frame++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);
}

void Weapon_RocketLauncher(edict_t *ent, gunindex_e gun)
{
	static int  pause_frames[] = { 25, 33, 42, 50, 0 };
	static int  fire_frames[] = { 5, 0 };
	Weapon_Generic(ent, 4, 12, 50, 54, pause_frames, fire_frames, Weapon_RocketLauncher_Fire, gun);
}


/*
======================================================================

BLASTER / HYPERBLASTER

======================================================================
*/

void Blaster_Fire(edict_t *ent, gunindex_e gun, vec3_t g_offset, int damage, bool hyper, int effect)
{
	vec3_t  forward, right;
	vec3_t  start;
	vec3_t  offset;

	if (is_quad)
		damage *= 4;

	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent->viewheight - 8);
	VectorAdd(offset, g_offset, offset);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	VectorScale(forward, -2, ent->client->gunstates[gun].kick_origin);
	ent->client->gunstates[gun].kick_angles[0] = -1;
	ent->client->gunstates[gun].kick_time = level.time + 100;
	fire_blaster(ent, start, forward, damage, 1000, effect, hyper);
	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);

	if (hyper)
		gi.WriteByte(MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte(MZ_BLASTER | is_silenced);

	gi.multicast(ent->s.origin, MULTICAST_PVS);
	PlayerNoise(ent, start, PNOISE_WEAPON);
}


void Weapon_Blaster_Fire(edict_t *ent, gunindex_e gun)
{
	int     damage;

	if (deathmatch->value)
		damage = 15;
	else
		damage = 10;

	Blaster_Fire(ent, gun, vec3_origin, damage, false, EF_BLASTER);
	ent->client->ps.guns[gun].frame++;
}

void Weapon_Blaster(edict_t *ent, gunindex_e gun)
{
	static int  pause_frames[] = { 19, 32, 0 };
	static int  fire_frames[] = { 5, 0 };
	Weapon_Generic(ent, 4, 8, 52, 55, pause_frames, fire_frames, Weapon_Blaster_Fire, gun);
}

void Weapon_HyperBlaster_Fire(edict_t *ent, gunindex_e gun)
{
	float   rotation;
	vec3_t  offset;
	int     effect;
	int     damage;
	ent->client->weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");

	if (!(ent->client->buttons & BUTTON_ATTACK))
		ent->client->ps.guns[gun].frame++;
	else
	{
		if (!HasEnoughAmmoToFire(ent, ent->client->pers.weapon))
		{
			if (level.time >= ent->pain_debounce_time)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent->pain_debounce_time = level.time + 1000;
			}

			NoAmmoWeaponChange(ent, gun);
		}
		else
		{
			rotation = (ent->client->ps.guns[gun].frame - 5) * 2 * M_PI / 6;
			offset[0] = -4 * sinf(rotation);
			offset[1] = 0;
			offset[2] = 4 * cosf(rotation);

			if ((ent->client->ps.guns[gun].frame == 6) || (ent->client->ps.guns[gun].frame == 9))
				effect = EF_HYPERBLASTER;
			else
				effect = 0;

			if (deathmatch->value)
				damage = 15;
			else
				damage = 20;

			Blaster_Fire(ent, gun, offset, damage, true, effect);

			if (!((int)dmflags->value & DF_INFINITE_AMMO))
				RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

			ent->client->anim_priority = ANIM_ATTACK;

			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crattak1 - 1;
				ent->client->anim_end = FRAME_crattak9;
			}
			else
			{
				ent->s.frame = FRAME_attack1 - 1;
				ent->client->anim_end = FRAME_attack8;
			}
		}

		ent->client->ps.guns[gun].frame++;

		if (ent->client->ps.guns[gun].frame == 12 && HasEnoughAmmoToFire(ent, ent->client->pers.weapon))
			ent->client->ps.guns[gun].frame = 6;
	}

	if (ent->client->ps.guns[gun].frame == 12)
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
		ent->client->weapon_sound = 0;
	}
}

void Weapon_HyperBlaster(edict_t *ent, gunindex_e gun)
{
	static int  pause_frames[] = { 0 };
	static int  fire_frames[] = { 6, 7, 8, 9, 10, 11, 0 };
	Weapon_Generic(ent, 5, 20, 49, 53, pause_frames, fire_frames, Weapon_HyperBlaster_Fire, gun);
}

/*
======================================================================

MACHINEGUN / CHAINGUN

======================================================================
*/

void Machinegun_Fire(edict_t *ent, gunindex_e gun)
{
	int i;
	vec3_t      start;
	vec3_t      forward, right;
	vec3_t      angles;
	int         damage = 8;
	int         kick = 2;
	vec3_t      offset;

	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->gunstates[gun].machinegun_shots = 0;
		ent->client->ps.guns[gun].frame++;
		return;
	}

	if (ent->client->ps.guns[gun].frame == 5)
		ent->client->ps.guns[gun].frame = 4;
	else
		ent->client->ps.guns[gun].frame = 5;

	if (!HasEnoughAmmoToFire(ent, ent->client->pers.weapon))
	{
		ent->client->ps.guns[gun].frame = 6;

		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1000;
		}

		NoAmmoWeaponChange(ent, gun);
		return;
	}

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	for (i = 1; i < 3; i++)
	{
		ent->client->gunstates[gun].kick_origin[i] = crandom() * 0.35f;
		ent->client->gunstates[gun].kick_angles[i] = crandom() * 0.7f;
	}

	ent->client->gunstates[gun].kick_origin[0] = crandom() * 0.35f;
	ent->client->gunstates[gun].kick_angles[0] = ent->client->gunstates[gun].machinegun_shots * -1.5f;
	ent->client->gunstates[gun].kick_time = level.time + 100;

	// raise the gun as it is firing
	if (!deathmatch->value)
	{
		ent->client->gunstates[gun].machinegun_shots++;

		if (ent->client->gunstates[gun].machinegun_shots > 9)
			ent->client->gunstates[gun].machinegun_shots = 9;
	}

	// get start / end positions
	VectorAdd(ent->client->v_angle, ent->client->gunstates[gun].kick_angles, angles);
	AngleVectors(angles, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	fire_bullet(ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_MACHINEGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);

	ent->client->anim_priority = ANIM_ATTACK;

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - (int)(random() + 0.25f);
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - (int)(random() + 0.25f);
		ent->client->anim_end = FRAME_attack8;
	}
}

void Weapon_Machinegun(edict_t *ent, gunindex_e gun)
{
	static int  pause_frames[] = { 23, 45, 0 };
	static int  fire_frames[] = { 4, 5, 0 };
	Weapon_Generic(ent, 3, 5, 45, 49, pause_frames, fire_frames, Machinegun_Fire, gun);
}

void Chaingun_Fire(edict_t *ent, gunindex_e gun)
{
	int         i;
	int         shots;
	vec3_t      start;
	vec3_t      forward, right, up;
	float       r, u;
	vec3_t      offset;
	int         damage;
	int         kick = 2;

	if (deathmatch->value)
		damage = 6;
	else
		damage = 8;

	if (ent->client->ps.guns[gun].frame == 5)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);

	if ((ent->client->ps.guns[gun].frame == 14) && !(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.guns[gun].frame = 32;
		ent->client->weapon_sound = 0;
		return;
	}
	else if ((ent->client->ps.guns[gun].frame == 21) && (ent->client->buttons & BUTTON_ATTACK)
		&& HasEnoughAmmoToFire(ent, ent->client->pers.weapon))
		ent->client->ps.guns[gun].frame = 15;
	else
		ent->client->ps.guns[gun].frame++;

	if (ent->client->ps.guns[gun].frame == 22)
	{
		ent->client->weapon_sound = 0;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnd1a.wav"), 1, ATTN_IDLE, 0);
	}
	else
		ent->client->weapon_sound = gi.soundindex("weapons/chngnl1a.wav");

	ent->client->anim_priority = ANIM_ATTACK;

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - (ent->client->ps.guns[gun].frame & 1);
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - (ent->client->ps.guns[gun].frame & 1);
		ent->client->anim_end = FRAME_attack8;
	}

	if (ent->client->ps.guns[gun].frame <= 9)
		shots = 1;
	else if (ent->client->ps.guns[gun].frame <= 14)
	{
		if (ent->client->buttons & BUTTON_ATTACK)
			shots = 2;
		else
			shots = 1;
	}
	else
		shots = 3;

	if (ent->client->pers.ammo < GetWeaponUsageCount(ent, ent->client->pers.weapon) * shots)
		shots = floorf(ent->client->pers.ammo / GetWeaponUsageCount(ent, ent->client->pers.weapon));

	if (!shots)
	{
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1000;
		}

		NoAmmoWeaponChange(ent, gun);
		return;
	}

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	for (i = 0; i < 3; i++)
	{
		ent->client->gunstates[gun].kick_origin[i] = crandom() * 0.35f;
		ent->client->gunstates[gun].kick_angles[i] = crandom() * 0.7f;
	}

	ent->client->gunstates[gun].kick_time = level.time + 100;

	for (i = 0; i < shots; i++)
	{
		// get start / end positions
		AngleVectors(ent->client->v_angle, forward, right, up);
		r = 7 + crandom() * 4;
		u = crandom() * 4;
		VectorSet(offset, 0, r, u + ent->viewheight - 8);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		fire_bullet(ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
	}

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte((MZ_CHAINGUN1 + shots - 1) | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		RemoveAmmoFromFiringShots(ent, ent->client->pers.weapon, shots);
}

void Weapon_Chaingun(edict_t *ent, gunindex_e gun)
{
	static int  pause_frames[] = { 38, 43, 51, 61, 0 };
	static int  fire_frames[] = { 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0 };
	Weapon_Generic(ent, 4, 31, 61, 64, pause_frames, fire_frames, Chaingun_Fire, gun);
}


/*
======================================================================

SHOTGUN / SUPERSHOTGUN

======================================================================
*/

void weapon_shotgun_fire(edict_t *ent, gunindex_e gun)
{
	vec3_t      start;
	vec3_t      forward, right;
	vec3_t      offset;
	int         damage = 4;
	int         kick = 8;

	if (ent->client->ps.guns[gun].frame == 9)
	{
		ent->client->ps.guns[gun].frame++;
		return;
	}

	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorScale(forward, -2, ent->client->gunstates[gun].kick_origin);
	ent->client->gunstates[gun].kick_angles[0] = -2;
	ent->client->gunstates[gun].kick_time = level.time + 100;
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	if (deathmatch->value)
		fire_shotgun(ent, start, forward, damage, kick, 500, 500, DEFAULT_DEATHMATCH_SHOTGUN_COUNT, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
	else
		fire_shotgun(ent, start, forward, damage, kick, 500, 500, DEFAULT_SHOTGUN_COUNT, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_SHOTGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
	ent->client->ps.guns[gun].frame++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);
}

void Weapon_Shotgun(edict_t *ent, gunindex_e gun)
{
	static int  pause_frames[] = { 22, 28, 34, 0 };
	static int  fire_frames[] = { 8, 9, 0 };
	Weapon_Generic(ent, 7, 18, 36, 39, pause_frames, fire_frames, weapon_shotgun_fire, gun);
}

void weapon_supershotgun_fire(edict_t *ent, gunindex_e gun)
{
	vec3_t      start;
	vec3_t      forward, right;
	vec3_t      offset;
	vec3_t      v;
	int         damage = 6;
	int         kick = 12;
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorScale(forward, -2, ent->client->gunstates[gun].kick_origin);
	ent->client->gunstates[gun].kick_angles[0] = -2;
	ent->client->gunstates[gun].kick_time = level.time + 100;
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	v[PITCH] = ent->client->v_angle[PITCH];
	v[YAW] = ent->client->v_angle[YAW] - 5;
	v[ROLL] = ent->client->v_angle[ROLL];
	AngleVectors(v, forward, NULL, NULL);
	fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
	v[YAW] = ent->client->v_angle[YAW] + 5;
	AngleVectors(v, forward, NULL, NULL);
	fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MakeWeaponMeansOfDeath(ent, GetIndexByItem(ent->client->pers.weapon), ent, DT_DIRECT));
	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_SSHOTGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
	ent->client->ps.guns[gun].frame++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);
}

void Weapon_SuperShotgun(edict_t *ent, gunindex_e gun)
{
	static int  pause_frames[] = { 29, 42, 57, 0 };
	static int  fire_frames[] = { 7, 0 };
	Weapon_Generic(ent, 6, 17, 57, 61, pause_frames, fire_frames, weapon_supershotgun_fire, gun);
}



/*
======================================================================

RAILGUN

======================================================================
*/

void weapon_railgun_fire(edict_t *ent, gunindex_e gun)
{
	vec3_t      start;
	vec3_t      forward, right;
	vec3_t      offset;
	int         damage;
	int         kick;

	if (deathmatch->value)
	{
		// normal damage is too extreme in dm
		damage = 100;
		kick = 200;
	}
	else
	{
		damage = 150;
		kick = 250;
	}

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorScale(forward, -3, ent->client->gunstates[gun].kick_origin);
	ent->client->gunstates[gun].kick_angles[0] = -3;
	ent->client->gunstates[gun].kick_time = level.time + 100;
	VectorSet(offset, 0, 7, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	fire_rail(ent, start, forward, damage, kick);
	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_RAILGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
	ent->client->ps.guns[gun].frame++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);
}

void Weapon_Railgun(edict_t *ent, gunindex_e gun)
{
	static int  pause_frames[] = { 56, 0 };
	static int  fire_frames[] = { 4, 0 };
	Weapon_Generic(ent, 3, 18, 56, 61, pause_frames, fire_frames, weapon_railgun_fire, gun);
}


/*
======================================================================

BFG10K

======================================================================
*/

void weapon_bfg_fire(edict_t *ent, gunindex_e gun)
{
	vec3_t  offset, start;
	vec3_t  forward, right;
	int     damage;
	float   damage_radius = 1000;

	if (deathmatch->value)
		damage = 200;
	else
		damage = 500;

	if (ent->client->ps.guns[gun].frame == 9)
	{
		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_BFG | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS);
		ent->client->ps.guns[gun].frame++;
		PlayerNoise(ent, start, PNOISE_WEAPON);
		return;
	}

	// cells can go down during windup (from power armor hits), so
	// check again and abort firing if we don't have enough now
	if (!HasEnoughAmmoToFire(ent, ent->client->pers.weapon))
	{
		ent->client->ps.guns[gun].frame++;
		return;
	}

	if (is_quad)
		damage *= 4;

	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorScale(forward, -2, ent->client->gunstates[gun].kick_origin);
	ent->client->gunstates[gun].kick_time = level.time + 100;
	// make a big pitch kick with an inverse fall
	ent->client->v_dmg_pitch = -40;
	ent->client->v_dmg_roll = crandom() * 8;
	ent->client->v_dmg_time = level.time + DAMAGE_TIME;
	VectorSet(offset, 8, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	fire_bfg(ent, start, forward, damage, 400, damage_radius);
	ent->client->ps.guns[gun].frame++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		RemoveAmmoFromFiring(ent, ent->client->pers.weapon);
}

void Weapon_BFG(edict_t *ent, gunindex_e gun)
{
	static int  pause_frames[] = { 39, 45, 50, 55, 0 };
	static int  fire_frames[] = { 9, 17, 0 };
	Weapon_Generic(ent, 8, 32, 55, 58, pause_frames, fire_frames, weapon_bfg_fire, gun);
}

static void Weapon_Q2_Run(edict_t *ent, gunindex_e gun)
{
	switch (ITEM_INDEX(ent->client->pers.weapon))
	{
		case ITI_Q2_BLASTER:
			Weapon_Blaster(ent, gun);
			break;

		case ITI_Q2_SHOTGUN:
			Weapon_Shotgun(ent, gun);
			break;

		case ITI_Q2_SUPER_SHOTGUN:
			Weapon_SuperShotgun(ent, gun);
			break;

		case ITI_Q2_MACHINEGUN:
			Weapon_Machinegun(ent, gun);
			break;

		case ITI_Q2_CHAINGUN:
			Weapon_Chaingun(ent, gun);
			break;

		case ITI_Q2_GRENADE_LAUNCHER:
			Weapon_GrenadeLauncher(ent, gun);
			break;

		case ITI_Q2_ROCKET_LAUNCHER:
			Weapon_RocketLauncher(ent, gun);
			break;

		case ITI_Q2_HYPERBLASTER:
			Weapon_HyperBlaster(ent, gun);
			break;

		case ITI_Q2_RAILGUN:
			Weapon_Railgun(ent, gun);
			break;

		case ITI_Q2_BFG10K:
			Weapon_BFG(ent, gun);
			break;

		case ITI_Q2_GRENADES:
			Weapon_Grenade(ent, gun);
			break;
	}
}


//======================================================================

static void Weapon_Run(edict_t *ent, gunindex_e gun)
{
	switch (ent->s.game)
	{
		case GAME_Q2:
			Weapon_Q2_Run(ent, gun);
			return;

		case GAME_Q1:
			Weapon_Q1_Run(ent, gun);
			return;

		case GAME_DOOM:
			Weapon_Doom_Run(ent, gun);
			return;

		case GAME_DUKE:
			Weapon_Duke_Run(ent, gun);
			return;
	}
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/

void Think_Weapon(edict_t *ent)
{
	// if just died, put the weapon away
	if (ent->health < 1)
	{
		int i;

		for (i = 0; i < MAX_PLAYER_GUNS; ++i)
		{
			ent->client->gunstates[i].newweapon = NULL;
			ChangeWeapon(ent, i);
		}
	}

	// call active weapon think routine
	if (ent->client->pers.weapon)
	{
		is_quad = (ent->client->quad_time > level.time);

		if (ent->client->silencer_shots)
			is_silenced = MZ_SILENCED;
		else
			is_silenced = 0;

		Weapon_Run(ent, GUN_MAIN);

		if (ent->s.game == GAME_DUKE)
			Weapon_Duke_Offhand_Foot(ent);
	}
}
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

bool        Pickup_Weapon(edict_t *ent, edict_t *other);
void        Use_Weapon(edict_t *ent, gitem_t *inv);
void        Drop_Weapon(edict_t *ent, gitem_t *inv);

void Weapon_Blaster(edict_t *ent, gunindex_e gun);
void Weapon_Shotgun(edict_t *ent, gunindex_e gun);
void Weapon_SuperShotgun(edict_t *ent, gunindex_e gun);
void Weapon_Machinegun(edict_t *ent, gunindex_e gun);
void Weapon_Chaingun(edict_t *ent, gunindex_e gun);
void Weapon_HyperBlaster(edict_t *ent, gunindex_e gun);
void Weapon_RocketLauncher(edict_t *ent, gunindex_e gun);
void Weapon_Grenade(edict_t *ent, gunindex_e gun);
void Weapon_GrenadeLauncher(edict_t *ent, gunindex_e gun);
void Weapon_Railgun(edict_t *ent, gunindex_e gun);
void Weapon_BFG(edict_t *ent, gunindex_e gun);

typedef enum
{
	HEALTH_NONE			= 0,
	HEALTH_IGNORE_MAX   = 1,
	HEALTH_TIMED        = 2
} healthflags_e;

void Use_Quad(edict_t *ent, gitem_t *item);
static gtime_t  quad_drop_timeout_hack;

game_iteminfo_t game_iteminfos[GAME_TOTAL] = {
	{}, // None
	// Q2
	{
		{
			{ ITI_SHELLS_LARGE, ITI_SHELLS },
			{ ITI_BULLETS_LARGE, ITI_BULLETS }
		},

		{
			{ ITI_BLASTER, 1 }
		},

		ITI_BLASTER,

		{
			// weapon pickups
			{ ITI_SHOTGUN, 10 },
			{ ITI_SUPER_SHOTGUN, 10 },
			{ ITI_MACHINEGUN, 50 },
			{ ITI_CHAINGUN, 50 },
			{ ITI_GRENADE_LAUNCHER, 5 },
			{ ITI_ROCKET_LAUNCHER, 5 },
			{ ITI_HYPERBLASTER, 50 },
			{ ITI_RAILGUN, 5 },
			{ ITI_BFG10K, 1 },

			// health pickups
			{ ITI_STIMPACK, 2 },
			{ ITI_MEDIUM_HEALTH, 10 },
			{ ITI_LARGE_HEALTH, 25 },
			{ ITI_MEGA_HEALTH, 100 }
		},

		{
			{ 25,  50, .30f, .00f },
			{ 50, 100, .60f, .30f },
			{ 100, 200, .80f, .60f }
		},

		{
			{ ITI_SHOTGUN, COUNT_FOR_SHOTS(40) },
			{ ITI_SUPER_SHOTGUN, COUNT_FOR_SHOTS(20) },
			{ ITI_MACHINEGUN, COUNT_FOR_SHOTS(200) },
			{ ITI_CHAINGUN, COUNT_FOR_SHOTS(200) },
			{ ITI_GRENADES, COUNT_FOR_SHOTS(25) },
			{ ITI_GRENADE_LAUNCHER, COUNT_FOR_SHOTS(25) },
			{ ITI_ROCKET_LAUNCHER, COUNT_FOR_SHOTS(25) },
			{ ITI_HYPERBLASTER, COUNT_FOR_SHOTS(80) },
			{ ITI_RAILGUN, COUNT_FOR_SHOTS(15) },
			{ ITI_BFG10K, COUNT_FOR_SHOTS(4) },
		},

		{
			{ ITI_SHOTGUN, ITI_SHELLS },
			{ ITI_SUPER_SHOTGUN, ITI_SHELLS },
			{ ITI_MACHINEGUN, ITI_BULLETS },
			{ ITI_CHAINGUN, ITI_BULLETS },
			{ ITI_GRENADE_LAUNCHER, ITI_GRENADES },
			{ ITI_GRENADES, ITI_GRENADES },
			{ ITI_ROCKET_LAUNCHER, ITI_ROCKETS },
			{ ITI_HYPERBLASTER, ITI_CELLS },
			{ ITI_RAILGUN, ITI_SLUGS },
			{ ITI_BFG10K, ITI_CELLS }
		}
	},
	// Q1
	{
		{
			// replace ammos
			{ ITI_GRENADES, ITI_Q1_ROCKETS },
			{ ITI_SLUGS, ITI_Q1_CELLS },
			{ ITI_BULLETS_LARGE, ITI_Q1_NAILS },
			{ ITI_SHELLS_LARGE, ITI_Q1_SHELLS },

			// replace weapons
			{ ITI_SHOTGUN, ITI_Q1_SUPER_SHOTGUN },
			{ ITI_RAILGUN, ITI_Q1_THUNDERBOLT },
			{ ITI_BFG10K, ITI_Q1_THUNDERBOLT },

			// replace power screen/shield with invis
			{ ITI_POWER_SCREEN, ITI_Q1_INVISIBILITY },
			{ ITI_POWER_SHIELD, ITI_Q1_INVISIBILITY },

			// dummy these
			{ ITI_ARMOR_SHARD, ITI_NULL },
			{ ITI_STIMPACK, ITI_NULL },
			{ ITI_SILENCER, ITI_NULL },
			{ ITI_ANCIENT_HEAD, ITI_NULL },
			{ ITI_ADRENALINE, ITI_NULL },
			{ ITI_BANDOLIER, ITI_NULL },
			{ ITI_AMMO_PACK, ITI_NULL }
		},

		{
			{ ITI_Q1_AXE, 1 },
			{ ITI_Q1_SHOTGUN, 1 }
		},

		ITI_Q1_SHOTGUN,

		{
			// weapon pickups
			{ ITI_Q1_SHOTGUN, 20 },
			{ ITI_Q1_SUPER_SHOTGUN, 10 },
			{ ITI_Q1_NAILGUN, 50 },
			{ ITI_Q1_SUPER_NAILGUN, 25 },
			{ ITI_Q1_GRENADE_LAUNCHER, 5 },
			{ ITI_Q1_ROCKET_LAUNCHER, 5 },
			{ ITI_Q1_THUNDERBOLT, 25 },
			{ ITI_RAILGUN, 25 }, // tbolt
			{ ITI_BFG10K, 25 }, // tbolt

			// health
			{ ITI_MEDIUM_HEALTH, 10 },
			{ ITI_LARGE_HEALTH, 25 },
			{ ITI_MEGA_HEALTH, 100 }
		},

		{
			{ 100, 100, .30f, .30f },
			{ 150, 150, .60f, .60f },
			{ 200, 200, .80f, .80f }
		},

		{
			{ ITI_Q1_SHOTGUN, COUNT_FOR_SHOTS(40) },
			{ ITI_Q1_SUPER_SHOTGUN, COUNT_FOR_SHOTS(20) },
			{ ITI_Q1_NAILGUN, COUNT_FOR_SHOTS(200) },
			{ ITI_Q1_SUPER_NAILGUN, COUNT_FOR_SHOTS(100) },
			{ ITI_Q1_GRENADE_LAUNCHER, COUNT_FOR_SHOTS(25) },
			{ ITI_Q1_ROCKET_LAUNCHER, COUNT_FOR_SHOTS(25) },
			{ ITI_Q1_THUNDERBOLT, COUNT_FOR_SHOTS(60) }
		},

		{
			{ ITI_Q1_SHOTGUN, ITI_Q1_SHELLS },
			{ ITI_Q1_SUPER_SHOTGUN, ITI_Q1_SHELLS },
			{ ITI_Q1_NAILGUN, ITI_Q1_NAILS },
			{ ITI_Q1_SUPER_NAILGUN, ITI_Q1_NAILS },
			{ ITI_Q1_GRENADE_LAUNCHER, ITI_Q1_ROCKETS },
			{ ITI_Q1_ROCKET_LAUNCHER, ITI_Q1_ROCKETS },
			{ ITI_Q1_THUNDERBOLT, ITI_Q1_CELLS }
		}
	},
	// Doom
	{
		{
			// replace combat with jacket
			{ ITI_COMBAT_ARMOR, ITI_DOOM_ARMOR },

			// dummied ammos
			{ ITI_GRENADES, ITI_DOOM_ROCKETS },
			{ ITI_SLUGS, ITI_DOOM_CELLS },
			{ ITI_BULLETS_LARGE, ITI_DOOM_BULLETS },
			{ ITI_SHELLS_LARGE, ITI_DOOM_SHELLS },

			// replace weapons
			{ ITI_MACHINEGUN, ITI_DOOM_CHAINGUN },
			{ ITI_GRENADE_LAUNCHER, ITI_DOOM_ROCKET_LAUNCHER },

			// replace power screen/shield with invis
			{ ITI_POWER_SCREEN, ITI_DOOM_INVISIBILITY },
			{ ITI_POWER_SHIELD, ITI_DOOM_INVISIBILITY },

			// dummied items
			{ ITI_SILENCER, ITI_NULL },
			{ ITI_ANCIENT_HEAD, ITI_NULL },
			{ ITI_BANDOLIER, ITI_NULL }
		},

		{
			{ ITI_DOOM_FIST, 1 },
			{ ITI_DOOM_PISTOL, 1 }
		},

		ITI_DOOM_PISTOL,

		{
			// weapon pickups
			{ ITI_DOOM_SHOTGUN, 15 },
			{ ITI_DOOM_SUPER_SHOTGUN, 5 },
			{ ITI_DOOM_CHAINGUN, 50 },
			{ ITI_DOOM_PISTOL, 50 },
			{ ITI_DOOM_ROCKET_LAUNCHER, 5 },
			{ ITI_DOOM_PLASMA_GUN, 50 },
			{ ITI_DOOM_BFG, 1 },
			{ ITI_MACHINEGUN, 50 }, // is a chaingun
			{ ITI_GRENADE_LAUNCHER, 5 }, // is a RL

			// health
			{ ITI_STIMPACK, 2 },
			{ ITI_MEDIUM_HEALTH, 10 },
			{ ITI_LARGE_HEALTH, 25 },
			{ ITI_MEGA_HEALTH, 100 }
		},
		
		{
			{ 100,  100, .33f, .33f },
			{ 0, 0, 0, 0 },
			{ 200, 200, .50f, .50f }
		},

		{
			{ ITI_DOOM_SHOTGUN, COUNT_FOR_SHOTS(40) },
			{ ITI_DOOM_SUPER_SHOTGUN, COUNT_FOR_SHOTS(20) },
			{ ITI_DOOM_CHAINGUN, COUNT_FOR_SHOTS(150) },
			{ ITI_DOOM_PISTOL, COUNT_FOR_SHOTS(150) },
			{ ITI_DOOM_ROCKET_LAUNCHER, COUNT_FOR_SHOTS(25) },
			{ ITI_DOOM_PLASMA_GUN, COUNT_FOR_SHOTS(60) },
			{ ITI_DOOM_BFG, COUNT_FOR_SHOTS(4) }
		},

		{
			{ ITI_DOOM_SHOTGUN, ITI_DOOM_SHELLS },
			{ ITI_DOOM_SUPER_SHOTGUN, ITI_DOOM_SHELLS },
			{ ITI_DOOM_PISTOL, ITI_DOOM_BULLETS },
			{ ITI_DOOM_CHAINGUN, ITI_DOOM_BULLETS },
			{ ITI_DOOM_ROCKET_LAUNCHER, ITI_DOOM_ROCKETS },
			{ ITI_DOOM_PLASMA_GUN, ITI_DOOM_CELLS },
			{ ITI_DOOM_BFG, ITI_DOOM_CELLS }
		}
	},
	// Duke
	{
		{
			// nulled shells
			{ ITI_SHELLS_LARGE, ITI_DUKE_SHELLS },

			// replace all armors
			{ ITI_BODY_ARMOR, ITI_DUKE_ARMOR },
			{ ITI_COMBAT_ARMOR, ITI_DUKE_ARMOR },

			// nulled items
			{ ITI_ARMOR_SHARD, ITI_NULL },
			{ ITI_STIMPACK, ITI_NULL },
			{ ITI_SUPER_SHOTGUN, ITI_NULL }
		},

		{
			{ ITI_DUKE_FOOT, 1 },
			{ ITI_DUKE_PISTOL, 1 }
		},

		ITI_DUKE_PISTOL,

		{
			{ ITI_DUKE_PISTOL, 35 },
			{ ITI_DUKE_CANNON, 50 },
			{ ITI_DUKE_SHOTGUN, 10 },
			{ ITI_DUKE_RPG, 4 },
			{ ITI_DUKE_DEVASTATOR, 15 },
			{ ITI_DUKE_PIPEBOMBS, 5 },
			{ ITI_DUKE_FREEZER, 25 },

			// health
			{ ITI_MEDIUM_HEALTH, 10 },
			{ ITI_LARGE_HEALTH, 30 },
			{ ITI_MEGA_HEALTH, 50 }
		},
		
		{
			{ 100,  100, .50, .50 },
			{ 0, 0, 0, 0 },
			{ 0, 0, 0, 0 }
		},

		{
			{ ITI_DUKE_PISTOL, COUNT_FOR_SHOTS(150) },
			{ ITI_DUKE_CANNON, COUNT_FOR_SHOTS(150) },
			{ ITI_DUKE_SHOTGUN, COUNT_FOR_SHOTS(45) },
			{ ITI_DUKE_RPG, COUNT_FOR_SHOTS(20) },
			{ ITI_DUKE_DEVASTATOR, COUNT_FOR_SHOTS(75) },
			{ ITI_DUKE_PIPEBOMBS, COUNT_FOR_SHOTS(15) },
			{ ITI_DUKE_FREEZER, COUNT_FOR_SHOTS(75) }
		},

		{
			{ ITI_DUKE_PISTOL, ITI_DUKE_CLIP },
			{ ITI_DUKE_CANNON, ITI_DUKE_CANNON_AMMO },
			{ ITI_DUKE_SHOTGUN, ITI_DUKE_SHELLS },
			{ ITI_DUKE_RPG, ITI_DUKE_RPG_ROCKETS },
			{ ITI_DUKE_DEVASTATOR, ITI_DUKE_DEVASTATOR_ROCKETS },
			{ ITI_DUKE_PIPEBOMBS, ITI_DUKE_PIPEBOMBS },
			{ ITI_DUKE_FREEZER, ITI_DUKE_FREEZER_AMMO }
		}
	}
};

static void InitItemInfo()
{
	for (int i = 1; i < GAME_TOTAL; ++i)
	{
		// setup defaults
		for (itemid_e x = ITI_NULL; x < ITI_TOTAL; x++)
		{
			game_iteminfos[i].dynamic.item_redirects[x] = x;
			game_iteminfos[i].dynamic.weapon_usage_counts[x] = -1;
			game_iteminfos[i].dynamic.item_pickup_counts[x] = 0;
		}

		for (int x = 0; x < ITI_TOTAL; ++x)
		{
			if (game_iteminfos[i].item_redirects[x].from)
			{
				itemid_e from = game_iteminfos[i].item_redirects[x].from;

				if (game_iteminfos[i].item_redirects[x].to)
					game_iteminfos[i].dynamic.item_redirects[from] = game_iteminfos[i].item_redirects[x].to;
				else
					game_iteminfos[i].dynamic.item_redirects[from] = ITI_NULL;
			}

			if (game_iteminfos[i].ammo_usages[x].item)
			{
				itemid_e from = game_iteminfos[i].ammo_usages[x].item;
				game_iteminfos[i].dynamic.weapon_usage_counts[from] = game_iteminfos[i].ammo_usages[x].ammo_usage;
			}

			if (game_iteminfos[i].ammo_pickups[x].item)
			{
				itemid_e from = game_iteminfos[i].ammo_pickups[x].item;
				game_iteminfos[i].dynamic.item_pickup_counts[from] = game_iteminfos[i].ammo_pickups[x].num;
			}
		}
	}
}

gitem_t *ResolveInventoryItem(edict_t *ent, gitem_t *item)
{
	itemid_e currentWeapon = GetIndexByItem(ent->client->pers.weapon);
	itemid_e itemid = GetIndexByItem(item);

	// Q2 and Q1 are all good the way they are
	switch (ent->s.game)
	{
	default:
		break;
	case GAME_DOOM:
		switch (itemid)
		{
		case ITI_BLASTER:
			if (currentWeapon == ITI_DOOM_CHAINSAW)
				return GetItemByIndex(ITI_DOOM_FIST);
			return GetItemByIndex(ent->client->pers.inventory[ITI_DOOM_CHAINSAW] > 0 ? ITI_DOOM_CHAINSAW : ITI_DOOM_FIST);
		case ITI_SHOTGUN:
			return GetItemByIndex(ITI_DOOM_PISTOL);
		case ITI_SUPER_SHOTGUN:
			if (currentWeapon == ITI_DOOM_SUPER_SHOTGUN)
				return GetItemByIndex(ITI_DOOM_SHOTGUN);
			return GetItemByIndex(ent->client->pers.inventory[ITI_DOOM_SUPER_SHOTGUN] > 0 ? ITI_DOOM_SUPER_SHOTGUN : ITI_DOOM_SHOTGUN);
		case ITI_MACHINEGUN:
			return GetItemByIndex(ITI_DOOM_CHAINGUN);
		case ITI_CHAINGUN:
			return GetItemByIndex(ITI_DOOM_ROCKET_LAUNCHER);
		case ITI_GRENADE_LAUNCHER:
			return GetItemByIndex(ITI_DOOM_PLASMA_GUN);
		case ITI_ROCKET_LAUNCHER:
			return GetItemByIndex(ITI_DOOM_BFG);
		default:
			return NULL;
		}
	case GAME_DUKE:
		switch (itemid)
		{
		case ITI_BLASTER:
			return GetItemByIndex(ITI_DUKE_FOOT);
		case ITI_SHOTGUN:
			return GetItemByIndex(ITI_DUKE_PISTOL);
		case ITI_SUPER_SHOTGUN:
			return GetItemByIndex(ITI_DUKE_SHOTGUN);
		case ITI_MACHINEGUN:
			return GetItemByIndex(ITI_DUKE_CANNON);
		case ITI_CHAINGUN:
			return GetItemByIndex(ITI_DUKE_RPG);
		case ITI_GRENADE_LAUNCHER:
			return GetItemByIndex(ITI_DUKE_PIPEBOMBS);
		case ITI_ROCKET_LAUNCHER:
			//return GetItemByIndex(ITI_DUKE_SHRINKER);
			return NULL;
		case ITI_HYPERBLASTER:
			return GetItemByIndex(ITI_DUKE_DEVASTATOR);
		case ITI_RAILGUN:
			//return GetItemByIndex(ITI_DUKE_TRIPBOMBS);
			return NULL;
		case ITI_BFG10K:
			return GetItemByIndex(ITI_DUKE_FREEZER);
		default:
			return NULL;
		}
	}

	return NULL;
}

gitem_t *ResolveItemRedirect(edict_t *ent, gitem_t *item)
{
	gametype_t gametype = ent->s.game;
	itemid_e redirect = ITI_NULL;

	while (true)
	{
		int index = ITEM_INDEX(item);
		redirect = game_iteminfos[gametype].dynamic.item_redirects[index];

		item = GetItemByIndex(redirect);

		if (!redirect)
			return NULL;
		else if (redirect == index)
			return item;
	}

	return NULL;
}

#include <assert.h>

float GetWeaponUsageCount(edict_t *ent, gitem_t *weapon)
{
	if (game_iteminfos[ent->s.game].dynamic.weapon_usage_counts[ITEM_INDEX(weapon)] == -1)
	{
		assert(false);
		return 1; // assume it uses 1 ammo. this is temporary only; all weapons need a defined value.
	}

	return game_iteminfos[ent->s.game].dynamic.weapon_usage_counts[ITEM_INDEX(weapon)];
}

// Ammo routines
float GetMaxAmmo(edict_t *ent, int has_bandolier, int has_ammo_pack)
{
	float max_ammo = DEFAULT_MAX_AMMO;

	if (has_bandolier == -1)
		has_bandolier = ent->client->pers.inventory[ITI_BANDOLIER];
	if (has_ammo_pack == -1)
		has_ammo_pack = ent->client->pers.inventory[ITI_AMMO_PACK];

	// Bandolier/Ammo Pack
	if (has_ammo_pack)
		max_ammo *= 2.0;
	else if (has_bandolier)
		max_ammo *= 1.5;

	return max_ammo;
}

bool HasEnoughAmmoToFireShots(edict_t *ent, gitem_t *weapon, int shots)
{
	return ent->client->pers.ammo >= GetWeaponUsageCount(ent, weapon) * shots;
}

void RemoveAmmoFromFiringShots(edict_t *ent, gitem_t *weapon, int shots)
{
	ent->client->pers.ammo -= GetWeaponUsageCount(ent, weapon) * shots;

	if (ent->client->pers.ammo < 0)
		ent->client->pers.ammo = 0;
}


//======================================================================

/*
===============
GetItemByIndex
===============
*/
gitem_t *GetItemByIndex(itemid_e index)
{
	if (index == 0 || index > game.num_items)
		return NULL;

	return &itemlist[index];
}


/*
===============
FindItemByClassname

===============
*/
gitem_t *FindItemByClassname(const char *classname)
{
	int     i;
	gitem_t *it;

	it = itemlist;
    for (i = 0 ; i < game.num_items ; i++, it++) {
		if (!it->classname)
			continue;
		if (!Q_stricmp(it->classname, classname))
			return it;
	}

	return NULL;
}

itemid_e GetIndexByItem(gitem_t *item)
{
	if (!item)
		return ITI_NULL;

	return (itemid_e)(item - itemlist);
}

/*
===============
FindItem

===============
*/
gitem_t *FindItem(const char *pickup_name)
{
	int     i;
	gitem_t *it;

	it = itemlist;
    for (i = 0 ; i < game.num_items ; i++, it++) {
		if (!it->pickup_name)
			continue;
		if (!Q_stricmp(it->pickup_name, pickup_name))
			return it;
	}

	return NULL;
}

//======================================================================

void DoRespawn(edict_t *ent)
{
	if (ent->team) {
		edict_t *master;
		int count;
		int choice;

		master = ent->teammaster;

		for (count = 0, ent = master; ent; ent = ent->chain, count++)
			;

        choice = Q_rand_uniform(count);

		for (count = 0, ent = master; count < choice; ent = ent->chain, count++)
			;
	}

	ent->svflags &= ~SVF_NOCLIENT;
	ent->solid = SOLID_TRIGGER;
	gi.linkentity(ent);

	// send an effect
	ent->s.event = EV_ITEM_RESPAWN;
}

void SetRespawn(edict_t *ent, float delay)
{
	ent->flags |= FL_RESPAWN;
	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_NOT;
	ent->nextthink = level.time + delay * 1000;
	ent->think = DoRespawn;
	gi.linkentity(ent);
}


//======================================================================

bool Pickup_Powerup(edict_t *ent, edict_t *other)
{
	int			quantity;
	bool		instant;

	quantity = other->client->pers.inventory[ITEM_INDEX(ent->item)];

	switch (other->s.game)
	{
	case GAME_Q2:
		if ((skill->value == 1 && quantity >= 2) || (skill->value >= 2 && quantity >= 1))
			return false;

		if ((coop->value) && (ent->item->flags & IT_STAY_COOP) && (quantity > 0))
			return false;

		other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

		instant = ((int)dmflags->value & DF_INSTANT_ITEMS) || ((ent->item->use == Use_Quad) && (ent->spawnflags & DROPPED_PLAYER_ITEM));
		break;
	case GAME_DUKE:
		if (quantity > 100)
			return false;

		// intentional fallthrough
	default:
		instant = true;
		break;
	}

	if (deathmatch->value && !(ent->spawnflags & DROPPED_ITEM))
		SetRespawn(ent, ent->item->respawn_time);

	if (instant)
	{
		if (other->s.game == GAME_Q2 && (ent->item->use == Use_Quad) && (ent->spawnflags & DROPPED_PLAYER_ITEM))
			quad_drop_timeout_hack = (ent->nextthink - level.time);
		
		ent->item->use(other, ent->item);
	}

	return true;
}

void Drop_General(edict_t *ent, gitem_t *item)
{
	Drop_Item(ent, item);
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);
}


//======================================================================

bool Pickup_Adrenaline(edict_t *ent, edict_t *other)
{
	switch (other->s.game)
	{
	case GAME_Q2:
		if (!deathmatch->value)
			other->max_health += 1;

		if (other->health < other->max_health)
			other->health = other->max_health;

		break;
	case GAME_DOOM:
		other->health += 100;

		if (other->health > 200)
			other->health = 200;
		break;
	}

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn(ent, ent->item->respawn_time);

	return true;
}

bool Pickup_AncientHead(edict_t *ent, edict_t *other)
{
	other->max_health += 2;

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn(ent, ent->item->respawn_time);

	return true;
}

bool Pickup_Bandolier(edict_t *ent, edict_t *other)
{
	other->client->pers.ammo = min(GetMaxAmmo(other, true, CHECK_INVENTORY), other->client->pers.ammo + (DEFAULT_MAX_AMMO / 4));

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn(ent, ent->item->respawn_time);

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

	return true;
}

bool Pickup_Pack(edict_t *ent, edict_t *other)
{
	other->client->pers.ammo = min(GetMaxAmmo(other, CHECK_INVENTORY, true), other->client->pers.ammo + (DEFAULT_MAX_AMMO / 2));

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn(ent, ent->item->respawn_time);

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

	return true;
}

//======================================================================

void Use_Quad(edict_t *ent, gitem_t *item)
{
	int     timeout;

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);

	if (quad_drop_timeout_hack) {
		timeout = quad_drop_timeout_hack;
		quad_drop_timeout_hack = 0;
	}
	else {
		timeout = 30000;
	}

	if (ent->client->quad_time > level.time)
		ent->client->quad_time += timeout;
	else
		ent->client->quad_time = level.time + timeout;

	if (ent->s.game != GAME_DOOM)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
	else if (ent->client->pers.weapon != GetItemByIndex(ITI_DOOM_FIST))
		ent->client->gunstates[GUN_MAIN].newweapon = GetItemByIndex(ITI_DOOM_FIST);
}

//======================================================================

void Use_Breather(edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);

	if (ent->client->breather_time > level.time)
		ent->client->breather_time += 30000;
	else
		ent->client->breather_time = level.time + 30000;

	if (ent->s.game == GAME_Q1)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("q1/items/inv1.wav"), 1, ATTN_NORM, 0);

//  gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void Use_Envirosuit(edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);

	if (ent->client->enviro_time > level.time)
		ent->client->enviro_time += 30000;
	else					
		ent->client->enviro_time = level.time + 30000;

	if (ent->s.game == GAME_Q1)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("q1/items/suit.wav"), 1, ATTN_NORM, 0);

//  gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void    Use_Invulnerability(edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);

	if (ent->client->invincible_time > level.time)
		ent->client->invincible_time += 30000;
	else					   
		ent->client->invincible_time = level.time + 30000;

	if (ent->s.game == GAME_Q1 || ent->s.game == GAME_Q2)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void    Use_Silencer(edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);
	ent->client->silencer_shots += 30;

//  gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

bool Pickup_Key(edict_t *ent, edict_t *other)
{
	if (coop->value) {
		if (GetIndexByItem(ent->item) == ITI_POWER_CUBE) {
			if (other->client->pers.power_cubes & ((ent->spawnflags & 0x0000ff00) >> 8))
				return false;
			other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
			other->client->pers.power_cubes |= ((ent->spawnflags & 0x0000ff00) >> 8);
		}
		else {
			if (other->client->pers.inventory[ITEM_INDEX(ent->item)])
				return false;
			other->client->pers.inventory[ITEM_INDEX(ent->item)] = 1;
		}
		return true;
	}
	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
	return true;
}

//======================================================================

bool ShouldSwapToPotentiallyBetterWeapon(edict_t *ent, gitem_t *new_item, bool is_new);

bool Add_Ammo(edict_t *ent, float count, itemid_e index, bool pickup)
{
	int         max_ammo;

	if (!ent->client)
		return false;

	max_ammo = GetMaxAmmo(ent, CHECK_INVENTORY, CHECK_INVENTORY);

	if (!max_ammo || ent->client->pers.ammo >= max_ammo)
		return false;

	if (pickup)
	{
		gitem_t		*item = GetItemByIndex(index);

		// only affects Q2 and Duke realistically
		if ((item->flags & IT_WEAPON) && ShouldSwapToPotentiallyBetterWeapon(ent, item, true))
			ent->client->gunstates[GUN_MAIN].newweapon = item;
	}

	ent->client->pers.ammo = min(max_ammo, ent->client->pers.ammo + count);

	return true;
}

bool do_propagate = true;
bool Pickup_Ammo(edict_t *ent, edict_t *other)
{
	float       count;
	bool        weapon;
	gitem_t		*item = ent->item;

	weapon = !!(item->flags & IT_WEAPON);

	if ((weapon) && ((int)dmflags->value & DF_INFINITE_AMMO))
		count = DEFAULT_MAX_AMMO * 100;
	else if (ent->count)
		count = ent->count;
	else
	{
		// TODO
		float real_num = 20;//game_iteminfos[other->s.game].dynamic.ammo_pickup_amounts[ITEM_INDEX(ent->real_item)];

		if (real_num != -1)
			count = real_num;
		else
		{
			count = 1;
			gi.dprintf("No pickup count for %i %s\n", other->s.game, item->pickup_name);
		}
	}

	if (!Add_Ammo(other, count, GetIndexByItem(item), true) && !invasion->value)
		return false;

	if (!(ent->spawnflags & DROPPED_PLAYER_ITEM))
	{
		if (invasion->value)
		{
			if (do_propagate)
			{
				// in invasion, propagate item to other clients
				do_propagate = false;
				for (int i = 0; i < game.maxclients; ++i)
				{
					edict_t *cl = &g_edicts[i + 1];

					if (!cl->inuse || !cl->client->pers.connected || !cl->s.game || cl == other)
						continue;

					Pickup_Ammo(ent, cl);
				}
				do_propagate = true;
			}
		}
		else if (!(ent->spawnflags & DROPPED_ITEM) && deathmatch->value)
			SetRespawn(ent, 30);
	}

	return true;
}

void Drop_Ammo(edict_t *ent, gitem_t *item)
{
	/*edict_t *dropped;
	int     index;

	index = ITEM_INDEX(item);
	dropped = Drop_Item(ent, item);

	int quantity = game_iteminfos[ent->s.game].dynamic.ammo_pickup_amounts[index];

	if (ent->client->pers.inventory[index] >= quantity)
		dropped->count = quantity;
	else
		dropped->count = ent->client->pers.inventory[index];

	ent->client->pers.inventory[index] -= dropped->count;

	if (ent->client->pers.weapon &&
		ent->client->pers.weapon == item &&
		!ent->client->pers.inventory[index])
		AttemptBetterWeaponSwap(ent);

	ValidateSelectedItem(ent);*/
}


//======================================================================

void MegaHealth_think(edict_t *self)
{
	if (self->owner->health > self->owner->max_health) {
		self->nextthink = level.time + 1000;
		self->owner->health -= 1;
		return;
	}

	if (!(self->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn(self, 20);
	else
		G_FreeEdict(self);
}

bool Pickup_Health(edict_t *ent, edict_t *other)
{
	healthflags_e flags = HEALTH_NONE;

	switch (GetIndexByItem(ent->item))
	{
	case ITI_STIMPACK:
		flags = HEALTH_IGNORE_MAX;
		break;
	case ITI_MEGA_HEALTH:
		flags = HEALTH_IGNORE_MAX;

		if (other->s.game == GAME_Q2 || other->s.game == GAME_Q1)
			flags |= HEALTH_TIMED;
		else if (other->s.game == GAME_DUKE && other->health >= 200)
			return false;
		break;
	}

	if (!(flags & HEALTH_IGNORE_MAX) && (other->health >= other->max_health))
		return false;

	if (other->s.game == GAME_DUKE && other->health < (other->max_health / 4) && GetIndexByItem(ent->item) != ITI_MEGA_HEALTH)
		gi.sound(other, CHAN_VOICE, gi.soundindex("duke/needed03.wav"), 1, ATTN_NORM, 0);

	if (other->s.game == GAME_DOOM && GetIndexByItem(ent->item) == ITI_DOOM_MEGA_SPHERE)
	{
		other->health = 200;
		other->client->pers.inventory[ITI_DOOM_ARMOR] = 0;
		other->client->pers.inventory[ITI_DOOM_MEGA_ARMOR] = 200;
	}
	else
	{
		other->health += (ent->count) ? ent->count : (int) game_iteminfos[other->s.game].dynamic.item_pickup_counts[GetIndexByItem(ent->item)];

		if (!(flags & HEALTH_IGNORE_MAX))
		{
			if (other->health > other->max_health)
				other->health = other->max_health;
		}
		else
		{
			if (other->s.game == GAME_DUKE && other->health > 200)
				other->health = 200;
		}
	}

	if (flags & HEALTH_TIMED)
	{
		ent->think = MegaHealth_think;
		ent->nextthink = level.time + 5000;
		ent->owner = other;
		ent->flags |= FL_RESPAWN;
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	else
	{
		if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
			SetRespawn(ent, 30);
	}

	return true;
}

//======================================================================

itemid_e ArmorIndex(edict_t *ent)
{
	if (ent->client)
	{
		for (itemid_e i = ITI_JACKET_ARMOR; i <= ITI_BODY_ARMOR; i++)
		{
			if (ent->client->pers.inventory[i] > 0)
				return i;
		}
	}

	return ITI_NULL;
}

bool Can_Pickup_Armor(edict_t *ent, gitem_t *armor, pickup_armor_t *result)
{
	// use temp result if we weren't passed one
	// since we use this area for calculations
	static pickup_armor_t temp_result;

	if (!result)
		result = &temp_result;

	// store current armor, if we have any
	result->old_armor_index = ArmorIndex(ent);

	if (result->old_armor_index)
	{
		result->old_armor_count = ent->client->pers.inventory[result->old_armor_index];
		result->old_armor = &game_iteminfos[ent->s.game].armors[result->old_armor_index - ITI_JACKET_ARMOR];
	}
	else
	{
		result->old_armor_count = 0;
		result->old_armor = NULL;
	}

	// try armor shards first
	if (GetIndexByItem(armor) == ITI_ARMOR_SHARD)
	{
		switch (ent->s.game)
		{
		case GAME_Q2:
		case GAME_DOOM:
			if (!result->old_armor_index)
			{
				result->new_armor_index = ITI_JACKET_ARMOR;
				result->new_armor = &game_iteminfos[ent->s.game].armors[result->new_armor_index - ITI_JACKET_ARMOR];
			}
			else
			{
				result->new_armor_index = ITI_NULL;
				result->new_armor = NULL;
			}

			result->new_armor_count = result->old_armor_count + ((ent->s.game == GAME_Q2) ? 2 : 1);

			if (ent->s.game == GAME_DOOM)
			{
				if (result->old_armor_count >= 200)
					return false;

				if (result->new_armor_count > 200)
					result->new_armor_count = 200;
			}

			return true;
		case GAME_Q1:
			return false;
		}
	}

	// now armors stuff
	// if player has no armor, just use it
	if (!result->old_armor_index)
	{
		result->new_armor_index = ITEM_INDEX(armor);
		result->new_armor = &game_iteminfos[ent->s.game].armors[result->new_armor_index - ITI_JACKET_ARMOR];
		result->new_armor_count = result->new_armor->base_count;

		return true;
	}
	else
	{
		result->new_armor_index = ITEM_INDEX(armor);
		result->new_armor = &game_iteminfos[ent->s.game].armors[result->new_armor_index - ITI_JACKET_ARMOR];

		// picking up new armor
		// different games have different algorithms
		switch (ent->s.game)
		{
		case GAME_Q2:
			if (result->new_armor->normal_protection > result->old_armor->normal_protection)
			{
				// calc new armor values
				float salvage = result->old_armor->normal_protection / result->new_armor->normal_protection;
				float salvagecount = salvage * ent->client->pers.inventory[result->old_armor_index];
				int newcount = result->new_armor->base_count + salvagecount;

				if (newcount > result->new_armor->max_count)
					newcount = result->new_armor->max_count;

				// change armor to new item with computed value
				result->new_armor_count = newcount;
			}
			else
			{
				// calc new armor values
				float salvage = result->new_armor->normal_protection / result->old_armor->normal_protection;
				float salvagecount = salvage * result->new_armor->base_count;
				int newcount = ent->client->pers.inventory[result->old_armor_index] + salvagecount;

				if (newcount > result->old_armor->max_count)
					newcount = result->old_armor->max_count;

				// if we're already maxed out then we don't need the new armor
				if (ent->client->pers.inventory[result->old_armor_index] >= newcount)
					return false;

				// update current armor value
				result->new_armor_index = result->old_armor_index;
				result->new_armor = result->old_armor;
				result->new_armor_count = newcount;
			}
			return true;
		case GAME_Q1:
			// in Q1, if we're picking up new/existing armor, always good.
			if (result->new_armor_index >= result->old_armor_index)
			{
				result->new_armor_count = result->new_armor->base_count;

				// if we have existing armor though, don't pick up beyond max
				if (result->new_armor_index == result->old_armor_index)
					return result->old_armor_count < result->new_armor_count;
			}
			else
			{
				// picking up lower-levelled armor is weird in Q1.
				switch (result->old_armor_index)
				{
				case ITI_Q1_YELLOW_ARMOR:
					// can only pick up green if we have < 50
					if (result->old_armor_count >= 50)
						return false;

					break;
				case ITI_Q1_RED_ARMOR:
					switch (result->new_armor_index)
					{
					case ITI_Q1_YELLOW_ARMOR:
						// can only pick up yellow if we have < 50
						if (result->old_armor_count >= 50)
							return false;

						break;
					case ITI_Q1_GREEN_ARMOR:
						// can only pick up green if we have < 40
						if (result->old_armor_count >= 40)
							return false;

						break;
					}
					break;
				}

				// passed, so replace it
				result->new_armor_count = result->new_armor->base_count;
			}

			return true;
		case GAME_DOOM:
			// in Q1, if we're picking up new/existing armor, always good.
			if (result->new_armor_index >= result->old_armor_index)
			{
				result->new_armor_count = result->new_armor->base_count;

				// if we have existing armor though, don't pick up beyond max
				if (result->new_armor_index == result->old_armor_index)
					return result->old_armor_count < result->new_armor_count;
			}
			else
			{
				// picking up lower-levelled armor
				switch (result->old_armor_index)
				{
				case ITI_DOOM_MEGA_ARMOR:
					// can only pick up mega if we have < 100
					if (result->old_armor_count >= 100)
						return false;

				}

				// passed, so replace it
				result->new_armor_count = result->new_armor->base_count;
			}

			return true;
		}
	}

	return false;
}

bool Pickup_Armor(edict_t *ent, edict_t *other)
{
	pickup_armor_t armor;
	bool can_pickup = Can_Pickup_Armor(other, ent->item, &armor);

	if (!can_pickup)
		return false;

	if (armor.old_armor_index != armor.new_armor_index)
	{
		if (armor.old_armor_index)
			other->client->pers.inventory[armor.old_armor_index] = 0;
	}

	if (armor.new_armor_count)
	{
		if (armor.new_armor_index)
			other->client->pers.inventory[armor.new_armor_index] = armor.new_armor_count;
		else
			other->client->pers.inventory[armor.old_armor_index] = armor.new_armor_count;
	}

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn(ent, 20);

	return true;
}

//======================================================================

power_armor_type_e PowerArmorType(edict_t *ent)
{
	if (!ent->client)
		return POWER_ARMOR_NONE;

	if (!(ent->flags & FL_POWER_ARMOR))
		return POWER_ARMOR_NONE;

	if (ent->client->pers.inventory[ITI_POWER_SHIELD] > 0)
		return POWER_ARMOR_SHIELD;

	if (ent->client->pers.inventory[ITI_POWER_SCREEN] > 0)
		return POWER_ARMOR_SCREEN;

	return POWER_ARMOR_NONE;
}

void Use_PowerArmor(edict_t *ent, gitem_t *item)
{
	int     index;

	if (ent->flags & FL_POWER_ARMOR) {
		ent->flags &= ~FL_POWER_ARMOR;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
	} else {
		index = ITI_CELLS;
		if (!ent->client->pers.inventory[index]) {
			gi.cprintf(ent, PRINT_HIGH, "No cells for power armor.\n");
			return;
		}
		ent->flags |= FL_POWER_ARMOR;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power1.wav"), 1, ATTN_NORM, 0);
	}
}

bool Pickup_PowerArmor(edict_t *ent, edict_t *other)
{
	int     quantity;

	quantity = other->client->pers.inventory[ITEM_INDEX(ent->item)];

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

	if (deathmatch->value) {
		if (!(ent->spawnflags & DROPPED_ITEM))
			SetRespawn(ent, ent->item->respawn_time);
		// auto-use for DM only if we didn't already have one
		if (!quantity)
			ent->item->use(other, ent->item);
	}

	return true;
}

void Drop_PowerArmor(edict_t *ent, gitem_t *item)
{
	if ((ent->flags & FL_POWER_ARMOR) && (ent->client->pers.inventory[ITEM_INDEX(item)] == 1))
		Use_PowerArmor(ent, item);
	Drop_General(ent, item);
}

//======================================================================

/*
===============
Touch_Item
===============
*/
void Touch_Item(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	bool    taken;
	gitem_t		*item = ent->item;

	if (!other->client)
		return;
	if (other->health < 1)
		return;     // dead people can't pickup

	// check item redirections
	item = ResolveItemRedirect(other, item);

	if (!item || !item->pickup)
		return;     // not a grabbable item?

	bool had = !other->client->pers.inventory[ITEM_INDEX(item)];

	ent->real_item = ent->item;
	ent->item = item;
	taken = item->pickup(ent, other);
	ent->item = ent->real_item;

	if (taken)
	{
		// flash the screen
		other->client->bonus_alpha = 0.25f;

		// show icon and name on status bar
		other->client->pickup_msg_time = level.time + 3000;
		other->client->pickup_item = item;
		other->client->pickup_item_first = had;

		// change selected item
		if (item->use)
			other->client->ps.stats[STAT_SELECTED_ITEM] = other->client->pers.selected_item = ITEM_INDEX(item);

		gi.sound(other, CHAN_ITEM, gi.soundindex(item->pickup_sound), 1, ATTN_NORM, 0);
	}

	if (!(ent->spawnflags & ITEM_TARGETS_USED)) {
		G_UseTargets(ent, other);
		ent->spawnflags |= ITEM_TARGETS_USED;
	}

	if (!taken)
		return;

	if (invasion->value || !((coop->value) && (item->flags & IT_STAY_COOP)) || (ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM))) {
		if (ent->flags & FL_RESPAWN)
			ent->flags &= ~FL_RESPAWN;
		else
			G_FreeEdict(ent);
	}
}

//======================================================================

void drop_temp_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	Touch_Item(ent, other, plane, surf);
}

void drop_make_touchable(edict_t *ent)
{
	ent->touch = Touch_Item;
	if (deathmatch->value) {
		ent->nextthink = level.time + 29000;
		ent->think = G_FreeEdict;
	}
}

edict_t *Drop_Item(edict_t *ent, gitem_t *item)
{
	edict_t *dropped;
	vec3_t  forward, right, up;
	vec3_t  offset;

	dropped = G_Spawn();

	dropped->entitytype = ET_ITEM;
	dropped->item = item;
	dropped->spawnflags = DROPPED_ITEM;
	dropped->s.effects = item->world_model_flags;
	VectorSet(dropped->mins, -15, -15, -15);
	VectorSet(dropped->maxs, 15, 15, 15);
	gi.setmodel(dropped, dropped->item->world_model);
	dropped->solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;
	dropped->touch = drop_temp_touch;
	dropped->owner = ent;

	if (ent->client) {
		trace_t trace;

		AngleVectors(ent->client->v_angle, forward, right, up);
		VectorSet(offset, 24, 0, -16);
		G_ProjectSource(ent->s.origin, offset, forward, right, dropped->s.origin);
		trace = gi.trace(ent->s.origin, dropped->mins, dropped->maxs,
			dropped->s.origin, ent, CONTENTS_SOLID);
		VectorCopy(trace.endpos, dropped->s.origin);
	} else {
		AngleVectors(ent->s.angles, forward, right, up);
		VectorCopy(ent->s.origin, dropped->s.origin);
	}

	if (ent->health <= 0 && ent->s.game == GAME_DOOM)
	{
		dropped->velocity[2] = 300;
	}
	else
	{
		VectorScale(forward, 100, dropped->velocity);
		dropped->velocity[2] = 300;
	}

	dropped->think = drop_make_touchable;
	dropped->nextthink = level.time + 1000;

	gi.linkentity(dropped);

	return dropped;
}

void Use_Item(edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->svflags &= ~SVF_NOCLIENT;
	ent->use = NULL;

	if (ent->spawnflags & ITEM_NO_TOUCH) {
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
    } else {
		ent->solid = SOLID_TRIGGER;
		ent->touch = Touch_Item;
	}

	gi.linkentity(ent);
}

//======================================================================

/*
================
droptofloor
================
*/
void droptofloor(edict_t *ent)
{
	trace_t     tr;
	vec3_t      dest;
	float       *v;
	bool		startIndex = (ent->s.modelindex == 0);

	if (startIndex)
	{
		VectorSet(ent->mins, -15, -15, -15);
		VectorSet(ent->maxs, 15, 15, 15);

		if (ent->model)
			gi.setmodel(ent, ent->model);
		else
			gi.setmodel(ent, ent->item->world_model);
		ent->touch = Touch_Item;
	}

	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;

	v = tv(0, 0, -128);
	VectorAdd(ent->s.origin, v, dest);

	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	if (tr.startsolid) {
		if (ent->item)
			gi.dprintf("droptofloor: %s startsolid at %s\n", ent->item->classname, vtos(ent->s.origin));
		else
			gi.dprintf("droptofloor: entityid %i startsolid at %s\n", ent->entitytype, vtos(ent->s.origin));
		G_FreeEdict(ent);
		return;
	}

	VectorCopy(tr.endpos, ent->s.origin);

	if (ent->team) {
		ent->flags &= ~FL_TEAMSLAVE;
		ent->chain = ent->teamchain;
		ent->teamchain = NULL;

		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		if (ent == ent->teammaster) {
			ent->nextthink = level.time + 1;
			ent->think = DoRespawn;
		}
	}

	if (startIndex)
	{
		if (ent->spawnflags & ITEM_NO_TOUCH) {
			ent->solid = SOLID_BBOX;
			ent->touch = NULL;
		}

		if (ent->spawnflags & ITEM_TRIGGER_SPAWN) {
			ent->svflags |= SVF_NOCLIENT;
			ent->solid = SOLID_NOT;
			ent->use = Use_Item;
		}
	}

	gi.linkentity(ent);
}


/*
===============
PrecacheItem

Precaches all data needed for a given item.
This will be called for each item spawned in a level,
and for each item in each client's inventory.
===============
*/
void PrecacheItem(gitem_t *it)
{
	char    *s, *start;
	char    data[MAX_QPATH];
	int     len;

	if (!it)
		return;

	if (it->pickup_sound)
		gi.soundindex(it->pickup_sound);
	if (it->world_model)
		gi.modelindex(it->world_model);
	if (it->view_model)
		gi.modelindex(it->view_model);
	if (it->icon)
		gi.imageindex(it->icon);

	// parse the space seperated precache string for other items
	s = it->precaches;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ')
			s++;

		len = s - start;
		if (len >= MAX_QPATH || len < 5)
			gi.error("PrecacheItem: %s has bad precache string", it->classname);
		memcpy(data, start, len);
		data[len] = 0;
		if (*s)
			s++;

		// determine type based on extension
		if (!strcmp(data + len - 3, "md2") || strcmp(data + len - 3, "mdl"))
			gi.modelindex(data);
		else if (!strcmp(data + len - 3, "sp2") || !strcmp(data + len - 3, "spr"))
			gi.modelindex(data);
		else if (!strcmp(data + len - 3, "wav"))
			gi.soundindex(data);
		if (!strcmp(data + len - 3, "pcx"))
			gi.imageindex(data);
	}
}

/*
============
SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void SpawnItem(edict_t *ent, gitem_t *item)
{
	ent->entitytype = ET_ITEM;
	PrecacheItem(item);

	if (ent->spawnflags) {
		if (GetIndexByItem(item) != ITI_POWER_CUBE) {
			ent->spawnflags = 0;
			gi.dprintf("%s at %s has invalid spawnflags set\n", item->classname, vtos(ent->s.origin));
		}
	}

	// some items will be prevented in deathmatch
	if (deathmatch->value) {
		if ((int)dmflags->value & DF_NO_ARMOR) {
			if (item->flags & IT_ARMOR) {
				G_FreeEdict(ent);
				return;
			}
		}
		if ((int)dmflags->value & DF_NO_ITEMS) {
			if (item->flags & IT_POWERUP) {
				G_FreeEdict(ent);
				return;
			}
		}
		if ((int)dmflags->value & DF_NO_HEALTH) {
			if (item->flags & IT_HEALTH) {
				G_FreeEdict(ent);
				return;
			}
		}
		if ((int)dmflags->value & DF_INFINITE_AMMO) {
			if ((item->flags == IT_AMMO) || GetIndexByItem(item) == ITI_BFG10K) {
				G_FreeEdict(ent);
				return;
			}
		}
	}

	if (coop->value && GetIndexByItem(item) == ITI_POWER_CUBE) {
		ent->spawnflags |= (1 << (8 + level.power_cubes));
		level.power_cubes++;
	}

	// don't let them drop items that stay in a coop game
	if ((coop->value) && (item->flags & IT_STAY_COOP)) {
		item->drop = NULL;
	}

	ent->item = item;
	ent->nextthink = level.time + game.frametime * 2;    // items start after other solids
	ent->think = droptofloor;
	ent->s.effects = item->world_model_flags;
	ent->s.game = GAME_NONE;

	if (ent->model)
		gi.modelindex(ent->model);
}

//======================================================================

gitem_t itemlist[] = {
	{
		NULL
	},  // leave index 0 alone

	//
	// ARMOR
	//

	/*QUAKED item_armor_jacket (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"item_armor_jacket",
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"%i_armor",
		"%i_jacket_armor", 0,
		NULL,
		/* icon */      "%i_jacketarmor",
		/* pickup */    "Jacket Armor",
		IT_ARMOR,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"item_armor_combat",
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"%i_armor",
		"%i_combat_armor", 0,
		NULL,
		/* icon */      "%i_combatarmor",
		/* pickup */    "Combat Armor",
		IT_ARMOR,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"item_armor_body",
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"%i_armor",
		"%i_body_armor", 0,
		NULL,
		/* icon */      "%i_bodyarmor",
		/* pickup */    "Body Armor",
		IT_ARMOR,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"item_armor_shard",
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"%i_armorshard",
		"%i_armorshard", 0,
		NULL,
		/* icon */      "i_jacketarmor",
		/* pickup */    "Armor Shard",
		IT_ARMOR,
		WEAP_NONE,
		/* precache */ ""
	},


	/*QUAKED item_power_screen (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"item_power_screen",
		Pickup_PowerArmor,
		Use_PowerArmor,
		Drop_PowerArmor,
		NULL,
		"misc/ar3_pkup.wav",
		"%i_powerscreen", 0,
		NULL,
		/* icon */      "i_powerscreen",
		/* pickup */    "Power Screen",
		IT_ARMOR,
		WEAP_NONE,
		/* precache */ "misc/power2.wav misc/power1.wav",
		60
	},

	/*QUAKED item_power_shield (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"item_power_shield",
		Pickup_PowerArmor,
		Use_PowerArmor,
		Drop_PowerArmor,
		NULL,
		"misc/ar3_pkup.wav",
		"%i_powershield", 0,
		NULL,
		/* icon */      "i_powershield",
		/* pickup */    "Power Shield",
		IT_ARMOR,
		WEAP_NONE,
		/* precache */ "misc/power2.wav misc/power1.wav",
		60
	},


	//
	// WEAPONS
	//

	/* weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
	always owned, never in the world
	*/
	{
		"weapon_blaster",
		NULL,
		Use_Weapon,
		NULL,
		Weapon_Blaster,
		"%s_weappickup",
		NULL, 0,
		"%wv_blaster",
		/* icon */      "w_blaster",
		/* pickup */    "Blaster",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_BLASTER,
		/* precache */ "misc/lasfly.wav"
	},

	/*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"weapon_shotgun",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Shotgun,
		"%s_weappickup",
		"%wg_shotgun", 0,
		"%wv_shotgun",
		/* icon */      "w_shotgun",
		/* pickup */    "Shotgun",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_SHOTGUN,
		/* precache */ ""
	},

	/*QUAKED weapon_supershotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"weapon_supershotgun",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_SuperShotgun,
		"%s_weappickup",
		"%wg_sshotgun", 0,
		"%wv_sshotgun",
		/* icon */      "w_sshotgun",
		/* pickup */    "Super Shotgun",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_SUPERSHOTGUN,
		/* precache */ ""
	},

	/*QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"weapon_machinegun",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Machinegun,
		"%s_weappickup",
		"%wg_machinegun", 0,
		"%wv_machinegun",
		/* icon */      "w_machinegun",
		/* pickup */    "Machinegun",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_MACHINEGUN,
		/* precache */ ""
	},

	/*QUAKED weapon_chaingun (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"weapon_chaingun",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Chaingun,
		"%s_weappickup",
		"%wg_chaingun", 0,
		"%wv_chaingun",
		/* icon */      "w_chaingun",
		/* pickup */    "Chaingun",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_CHAINGUN,
		/* precache */ "weapons/chngnu1a.wav weapons/chngnd1a.wav weapons/chngnl1a.wav"
	},

	/*QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"weapon_grenadelauncher",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_GrenadeLauncher,
		"%s_weappickup",
		"%wg_glauncher", 0,
		"%wv_glauncher",
		/* icon */      "w_glauncher",
		/* pickup */    "Grenade Launcher",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_GRENADELAUNCHER,
		/* precache */ "models/objects/grenade/tris.md2 weapons/grenlb1b.wav"
	},

	/*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"weapon_rocketlauncher",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_RocketLauncher,
		"%s_weappickup",
		"%wg_rlauncher", 0,
		"%wv_rlauncher",
		/* icon */      "w_rlauncher",
		/* pickup */    "Rocket Launcher",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_ROCKETLAUNCHER,
		/* precache */ "%e_rocket weapons/rockfly.wav"
	},

	/*QUAKED weapon_hyperblaster (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"weapon_hyperblaster",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_HyperBlaster,
		"%s_weappickup",
		"%wg_hyperblaster", 0,
		"%wv_hyperblaster",
		/* icon */      "w_hyperblaster",
		/* pickup */    "HyperBlaster",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_HYPERBLASTER,
		/* precache */ "weapons/hyprbl1a.wav weapons/hyprbd1a.wav misc/lasfly.wav"
	},

	/*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"weapon_railgun",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Railgun,
		"%s_weappickup",
		"%wg_railgun", 0,
		"%wv_railgun",
		/* icon */      "w_railgun",
		/* pickup */    "Railgun",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_RAILGUN,
		/* precache */ "weapons/rg_hum.wav"
	},

	/*QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"weapon_bfg",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_BFG,
		"%s_weappickup",
		"%wg_bfg", 0,
		"%wv_bfg",
		/* icon */      "w_bfg",
		/* pickup */    "BFG10K",
		IT_WEAPON | IT_STAY_COOP,
		WEAP_BFG,
		/* precache */ "sprites/s_bfg1.sp2 sprites/s_bfg2.sp2 sprites/s_bfg3.sp2 weapons/bfg__l1a.wav weapons/bfg__x1b.wav weapons/bfg_hum.wav"
	},

	//
	// AMMO ITEMS
	//

	/*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"ammo_bullets",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"%s_ammopickup",
		"%wa_bullets", 0,
		NULL,
		/* icon */      "%a_bullets",
		/* pickup */    "Bullets",
		IT_AMMO,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"ammo_shells",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"%s_ammopickup",
		"%wa_shells", 0,
		NULL,
		/* icon */      "%a_shells",
		/* pickup */    "Shells",
		IT_AMMO,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"ammo_rockets",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"%s_ammopickup",
		"%wa_rockets", 0,
		NULL,
		/* icon */      "%a_rockets",
		/* pickup */    "Rockets",
		IT_AMMO,
		WEAP_NONE,
		/* precache */ ""
	},
			
	/*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"ammo_grenades",
		Pickup_Ammo,
		Use_Weapon,
		Drop_Ammo,
		Weapon_Grenade,
		"%s_ammopickup",
		"%wa_grenades", 0,
		"%wv_grenades",
		/* icon */      "a_grenades",
		/* pickup */    "Grenades",
		IT_AMMO | IT_WEAPON,
		WEAP_GRENADES,
		/* precache */ "weapons/hgrent1a.wav weapons/hgrena1b.wav weapons/hgrenc1b.wav weapons/hgrenb1a.wav weapons/hgrenb2a.wav"
	},

	/*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"ammo_cells",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"%s_ammopickup",
		"%wa_cells", 0,
		NULL,
		/* icon */      "%a_cells",
		/* pickup */    "Cells",
		IT_AMMO,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"ammo_slugs",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"%s_ammopickup",
		"%wa_slugs", 0,
		NULL,
		/* icon */      "a_slugs",
		/* pickup */    "Slugs",
		IT_AMMO,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED ammo_shells_large (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"ammo_shells_large",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"%s_ammopickup",
		"%wa_shells_large", 0,
		NULL,
		/* icon */      "a_shells",
		/* pickup */    "Shells (Large)",
		IT_AMMO,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED ammo_bullets_large (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"ammo_bullets_large",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"%s_ammopickup",
		"%wa_bullets_large", 0,
		NULL,
		/* icon */      "a_bullets",
		/* pickup */    "Bullets (Large)",
		IT_AMMO,
		WEAP_NONE,
		/* precache */ ""
	},


	{
		"item_health_small",
		Pickup_Health,
		NULL,
		NULL,
		NULL,
		"%i_stimpack",
		"%i_stimpack", 0,
		NULL,
		/* icon */      "i_health",
		/* pickup */    "Health",
		IT_HEALTH,
		WEAP_NONE,
		/* precache */ ""
	},

	{
		"item_health",
		Pickup_Health,
		NULL,
		NULL,
		NULL,
		"%i_smhealth",
		"%i_smhealth", 0,
		NULL,
		/* icon */      "i_health",
		/* pickup */    "Health",
		IT_HEALTH,
		WEAP_NONE,
		/* precache */ ""
	},

	{
		"item_health_large",
		Pickup_Health,
		NULL,
		NULL,
		NULL,
		"%i_lghealth",
		"%i_lghealth", 0,
		NULL,
		/* icon */      "i_health",
		/* pickup */    "Health",
		IT_HEALTH,
		WEAP_NONE,
		/* precache */ ""
	},

	{
		"item_health_mega",
		Pickup_Health,
		NULL,
		NULL,
		NULL,
		"%i_megahealth",
		"%i_megahealth", 0,
		NULL,
		/* icon */      "i_health",
		/* pickup */    "Health",
		IT_HEALTH,
		WEAP_NONE,
		/* precache */ ""
	},

	//
	// POWERUP ITEMS
	//
	/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"item_quad",
		Pickup_Powerup,
		Use_Quad,
		Drop_General,
		NULL,
		"%s_powerup",
		"%i_quaddamage", 0,
		NULL,
		/* icon */      "p_quad",
		/* pickup */    "Quad Damage",
		IT_POWERUP,
		WEAP_NONE,
		/* precache */ "items/damage.wav items/damage2.wav items/damage3.wav",
		60
	},

	/*QUAKED item_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"item_invulnerability",
		Pickup_Powerup,
		Use_Invulnerability,
		Drop_General,
		NULL,
		"%s_powerup",
		"%i_invulnerability", 0,
		NULL,
		/* icon */      "p_invulnerability",
		/* pickup */    "Invulnerability",
		IT_POWERUP,
		WEAP_NONE,
		/* precache */ "items/protect.wav items/protect2.wav items/protect4.wav",
		300
	},

	/*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"item_enviro",
		Pickup_Powerup,
		Use_Envirosuit,
		Drop_General,
		NULL,
		"%s_powerup",
		"%i_environmentsuit", 0,
		NULL,
		/* icon */      "p_envirosuit",
		/* pickup */    "Environment Suit",
		IT_STAY_COOP | IT_POWERUP,
		WEAP_NONE,
		/* precache */ "",
		60
	},

	/*QUAKED item_silencer (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"item_silencer",
		Pickup_Powerup,
		Use_Silencer,
		Drop_General,
		NULL,
		"%s_powerup",
		"%i_silencer", 0,
		NULL,
		/* icon */      "p_silencer",
		/* pickup */    "Silencer",
		IT_POWERUP,
		WEAP_NONE,
		/* precache */ "",
		60
	},

	/*QUAKED item_breather (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"item_breather",
		Pickup_Powerup,
		Use_Breather,
		Drop_General,
		NULL,
		"%s_powerup",
		"%i_rebreather", 0,
		NULL,
		/* icon */      "p_rebreather",
		/* pickup */    "Rebreather",
		IT_STAY_COOP | IT_POWERUP,
		WEAP_NONE,
		/* precache */ "items/airout.wav",
		60
	},

	/*QUAKED item_ancient_head (.3 .3 1) (-16 -16 -16) (16 16 16)
	Special item that gives +2 to maximum health
	*/
	{
		"item_ancient_head",
		Pickup_AncientHead,
		NULL,
		NULL,
		NULL,
		"%s_powerup",
		"%i_ancienthead", 0,
		NULL,
		/* icon */      "i_fixme",
		/* pickup */    "Ancient Head",
		IT_NONE,
		WEAP_NONE,
		/* precache */ "",
		60
	},

	/*QUAKED item_adrenaline (.3 .3 1) (-16 -16 -16) (16 16 16)
	gives +1 to maximum health
	*/
	{
		"item_adrenaline",
		Pickup_Adrenaline,
		NULL,
		NULL,
		NULL,
		"%s_powerup",
		"%i_adrenaline", 0,
		NULL,
		/* icon */      "p_adrenaline",
		/* pickup */    "Adrenaline",
		IT_NONE,
		WEAP_NONE,
		/* precache */ "",
		60
	},

	/*QUAKED item_bandolier (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"item_bandolier",
		Pickup_Bandolier,
		NULL,
		NULL,
		NULL,
		"%s_powerup",
		"%i_bandolier", 0,
		NULL,
		/* icon */      "p_bandolier",
		/* pickup */    "Bandolier",
		IT_NONE,
		WEAP_NONE,
		/* precache */ "",
		60
	},

	/*QUAKED item_pack (.3 .3 1) (-16 -16 -16) (16 16 16)
	*/
	{
		"item_pack",
		Pickup_Pack,
		NULL,
		NULL,
		NULL,
		"%s_powerup",
		"%i_backpack", 0,
		NULL,
		/* icon */      "i_pack",
		/* pickup */    "Ammo Pack",
		IT_NONE,
		WEAP_NONE,
		/* precache */ "",
		180
	},

	//
	// KEYS
	//
	/*QUAKED key_data_cd (0 .5 .8) (-16 -16 -16) (16 16 16)
	key for computer centers
	*/
	{
		"key_data_cd",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/data_cd/tris.md2", EF_ROTATE,
		NULL,
		"k_datacd",
		"Data CD",
		IT_STAY_COOP | IT_KEY,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED key_power_cube (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN NO_TOUCH
	warehouse circuits
	*/
	{
		"key_power_cube",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/power/tris.md2", EF_ROTATE,
		NULL,
		"k_powercube",
		"Power Cube",
		IT_STAY_COOP | IT_KEY,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED key_pyramid (0 .5 .8) (-16 -16 -16) (16 16 16)
	key for the entrance of jail3
	*/
	{
		"key_pyramid",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/pyramid/tris.md2", EF_ROTATE,
		NULL,
		"k_pyramid",
		"Pyramid Key",
		IT_STAY_COOP | IT_KEY,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED key_data_spinner (0 .5 .8) (-16 -16 -16) (16 16 16)
	key for the city computer
	*/
	{
		"key_data_spinner",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/spinner/tris.md2", EF_ROTATE,
		NULL,
		"k_dataspin",
		"Data Spinner",
		IT_STAY_COOP | IT_KEY,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED key_pass (0 .5 .8) (-16 -16 -16) (16 16 16)
	security pass for the security level
	*/
	{
		"key_pass",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/pass/tris.md2", EF_ROTATE,
		NULL,
		"k_security",
		"Security Pass",
		IT_STAY_COOP | IT_KEY,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED key_blue_key (0 .5 .8) (-16 -16 -16) (16 16 16)
	normal door key - blue
	*/
	{
		"key_blue_key",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/key/tris.md2", EF_ROTATE,
		NULL,
		"k_bluekey",
		"Blue Key",
		IT_STAY_COOP | IT_KEY,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED key_red_key (0 .5 .8) (-16 -16 -16) (16 16 16)
	normal door key - red
	*/
	{
		"key_red_key",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/red_key/tris.md2", EF_ROTATE,
		NULL,
		"k_redkey",
		"Red Key",
		IT_STAY_COOP | IT_KEY,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED key_commander_head (0 .5 .8) (-16 -16 -16) (16 16 16)
	tank commander's head
	*/
	{
		"key_commander_head",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/monsters/commandr/head/tris.md2", EF_GIB,
		NULL,
		/* icon */      "k_comhead",
		/* pickup */    "Commander's Head",
		IT_STAY_COOP | IT_KEY,
		WEAP_NONE,
		/* precache */ ""
	},

	/*QUAKED key_airstrike_target (0 .5 .8) (-16 -16 -16) (16 16 16)
	tank commander's head
	*/
	{
		"key_airstrike_target",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/target/tris.md2", EF_ROTATE,
		NULL,
		/* icon */      "i_airstrike",
		/* pickup */    "Airstrike Marker",
		IT_STAY_COOP | IT_KEY,
		WEAP_NONE,
		/* precache */ ""
	}
};

void Weapons_Init();

void InitItems(void)
{
	game.num_items = sizeof(itemlist) / sizeof(itemlist[0]) - 1;
}

/*
===============
SetItemNames

Called by worldspawn
===============
*/
void SetItemNames(void)
{
	int     i;
	gitem_t *it;

    for (i = 0 ; i < game.num_items ; i++) {
		it = &itemlist[i];
		gi.configstring(CS_ITEMS + i, it->pickup_name);
	}

	InitItemInfo();
}

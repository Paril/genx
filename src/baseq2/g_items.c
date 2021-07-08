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

void Use_Quad(edict_t *ent, gitem_t *item);
static gtime_t  quad_drop_timeout_hack;

#define GameInfoReplaceItem(old, new) info->item_redirects[old] = new
#define GameInfoDummyItem(item) GameInfoReplaceItem(item, ITI_NULL)
#define GameInfoStartsWith(item, count) info->item_starts[item] = count
#define GameInfoDefaultItem(item) info->default_item = item
#define GameInfoAmmoPickup(item, count) info->ammo_pickups[item] = count
#define GameInfoHealthPickup GameInfoAmmoPickup
#define GameInfoArmorStats(item, base_count, max_count, normal_protection, energy_protection) \
	info->armors[item - ITI_JACKET_ARMOR] = (gitem_armor_t) { base_count, max_count, normal_protection, energy_protection }
#define GameInfoWeaponAmmo(item, shots) info->ammo_usages[item] = info->default_ammo_usages[item] = shots ? COUNT_FOR_SHOTS(shots) : shots
#define GameInfoWeaponGroup(group, ...) \
	memcpy(info->weapon_groups[group].weapons, (itemid_e[]) { __VA_ARGS__ }, sizeof((itemid_e[]) { __VA_ARGS__ })); \
	info->weapon_groups[group].num_weapons = q_countof((itemid_e[]) { __VA_ARGS__ })

static void InitGameInfoBase(game_iteminfo_t *info)
{
	itemid_e i;

	for (i = 0; i < ITI_TOTAL; i++)
		info->item_redirects[i] = info->ammo_usages[i] = info->default_ammo_usages[i] = -1;
}

static void InitQ2GameInfo(game_iteminfo_t *info)
{
	// no replacements
	GameInfoStartsWith(ITI_Q2_BLASTER, 1);
	GameInfoStartsWith(ITI_Q2_SHOTGUN, 1);
	GameInfoDefaultItem(ITI_Q2_SHOTGUN);
	GameInfoAmmoPickup(ITI_Q2_SHOTGUN, 10);
	GameInfoAmmoPickup(ITI_Q2_SUPER_SHOTGUN, 10);
	GameInfoAmmoPickup(ITI_Q2_MACHINEGUN, 10);
	GameInfoAmmoPickup(ITI_Q2_CHAINGUN, 10);
	GameInfoAmmoPickup(ITI_Q2_GRENADE_LAUNCHER, 10);
	GameInfoAmmoPickup(ITI_Q2_ROCKET_LAUNCHER, 10);
	GameInfoAmmoPickup(ITI_Q2_HYPERBLASTER, 10);
	GameInfoAmmoPickup(ITI_Q2_RAILGUN, 10);
	GameInfoAmmoPickup(ITI_Q2_BFG10K, 10);
	GameInfoAmmoPickup(ITI_Q2_GRENADES, 10);
	GameInfoHealthPickup(ITI_STIMPACK, 2);
	GameInfoHealthPickup(ITI_MEDIUM_HEALTH, 10);
	GameInfoHealthPickup(ITI_LARGE_HEALTH, 25);
	GameInfoHealthPickup(ITI_MEGA_HEALTH, 100);
	GameInfoArmorStats(ITI_JACKET_ARMOR, 25, 50, .30f, 0);
	GameInfoArmorStats(ITI_COMBAT_ARMOR, 50, 100, .60f, .30f);
	GameInfoArmorStats(ITI_BODY_ARMOR, 100, 200, .80f, .60f);
	GameInfoWeaponAmmo(ITI_Q2_SHOTGUN, 40);
	GameInfoWeaponAmmo(ITI_Q2_SUPER_SHOTGUN, 20);
	GameInfoWeaponAmmo(ITI_Q2_MACHINEGUN, 200);
	GameInfoWeaponAmmo(ITI_Q2_CHAINGUN, 200);
	GameInfoWeaponAmmo(ITI_Q2_GRENADE_LAUNCHER, 25);
	GameInfoWeaponAmmo(ITI_Q2_ROCKET_LAUNCHER, 25);
	GameInfoWeaponAmmo(ITI_Q2_HYPERBLASTER, 80);
	GameInfoWeaponAmmo(ITI_Q2_RAILGUN, 15);
	GameInfoWeaponAmmo(ITI_Q2_BFG10K, 4);
	GameInfoWeaponAmmo(ITI_Q2_GRENADES, 25);
	GameInfoWeaponGroup(WEAPON_GROUP_RAPID, ITI_Q2_MACHINEGUN, ITI_Q2_CHAINGUN);
	GameInfoWeaponGroup(WEAPON_GROUP_SPREAD, ITI_Q2_SHOTGUN, ITI_Q2_SUPER_SHOTGUN);
	GameInfoWeaponGroup(WEAPON_GROUP_EXPLOSIVE, ITI_Q2_GRENADE_LAUNCHER, ITI_Q2_ROCKET_LAUNCHER);
	GameInfoWeaponGroup(WEAPON_GROUP_EXOTIC, ITI_Q2_HYPERBLASTER, ITI_Q2_RAILGUN);
	GameInfoWeaponGroup(WEAPON_GROUP_SUPER, ITI_Q2_BFG10K);
	GameInfoWeaponGroup(WEAPON_GROUP_ANY, ITI_Q2_SHOTGUN, ITI_Q2_SUPER_SHOTGUN, ITI_Q2_MACHINEGUN, ITI_Q2_CHAINGUN, ITI_Q2_GRENADE_LAUNCHER, ITI_Q2_ROCKET_LAUNCHER, ITI_Q2_HYPERBLASTER, ITI_Q2_RAILGUN, ITI_Q2_BFG10K);
}

static void InitQ1GameInfo(game_iteminfo_t *info)
{
	// replace power screen/shield with invis
	GameInfoReplaceItem(ITI_POWER_SCREEN, ITI_Q1_INVISIBILITY);
	GameInfoReplaceItem(ITI_POWER_SHIELD, ITI_Q1_INVISIBILITY);
	// dummy these
	GameInfoDummyItem(ITI_ARMOR_SHARD);
	GameInfoDummyItem(ITI_STIMPACK);
	GameInfoDummyItem(ITI_SILENCER);
	GameInfoDummyItem(ITI_ANCIENT_HEAD);
	GameInfoDummyItem(ITI_ADRENALINE);
	GameInfoDummyItem(ITI_BANDOLIER);
	GameInfoDummyItem(ITI_AMMO_PACK);
	GameInfoStartsWith(ITI_Q1_AXE, 1);
	GameInfoStartsWith(ITI_Q1_SHOTGUN, 1);
	GameInfoDefaultItem(ITI_Q1_SHOTGUN);
	GameInfoAmmoPickup(ITI_Q1_SHOTGUN, 20);
	GameInfoAmmoPickup(ITI_Q1_SUPER_SHOTGUN, 10);
	GameInfoAmmoPickup(ITI_Q1_NAILGUN, 50);
	GameInfoAmmoPickup(ITI_Q1_SUPER_NAILGUN, 25);
	GameInfoAmmoPickup(ITI_Q1_GRENADE_LAUNCHER, 5);
	GameInfoAmmoPickup(ITI_Q1_ROCKET_LAUNCHER, 5);
	GameInfoAmmoPickup(ITI_Q1_THUNDERBOLT, 25);
	GameInfoHealthPickup(ITI_MEDIUM_HEALTH, 10);
	GameInfoHealthPickup(ITI_LARGE_HEALTH, 25);
	GameInfoHealthPickup(ITI_MEGA_HEALTH, 100);
	GameInfoArmorStats(ITI_JACKET_ARMOR, 100, 100, .30f, .30f);
	GameInfoArmorStats(ITI_COMBAT_ARMOR, 150, 150, .60f, .60f);
	GameInfoArmorStats(ITI_BODY_ARMOR, 200, 200, .80f, .80f);
	GameInfoWeaponAmmo(ITI_Q1_SHOTGUN, 40);
	GameInfoWeaponAmmo(ITI_Q1_SUPER_SHOTGUN, 20);
	GameInfoWeaponAmmo(ITI_Q1_NAILGUN, 200);
	GameInfoWeaponAmmo(ITI_Q1_SUPER_NAILGUN, 100);
	GameInfoWeaponAmmo(ITI_Q1_GRENADE_LAUNCHER, 25);
	GameInfoWeaponAmmo(ITI_Q1_ROCKET_LAUNCHER, 25);
	GameInfoWeaponAmmo(ITI_Q1_THUNDERBOLT, 60);
	GameInfoWeaponGroup(WEAPON_GROUP_RAPID, ITI_Q1_NAILGUN, ITI_Q1_SUPER_NAILGUN);
	GameInfoWeaponGroup(WEAPON_GROUP_SPREAD, ITI_Q1_SUPER_SHOTGUN);
	GameInfoWeaponGroup(WEAPON_GROUP_EXPLOSIVE, ITI_Q1_GRENADE_LAUNCHER, ITI_Q1_ROCKET_LAUNCHER);
	GameInfoWeaponGroup(WEAPON_GROUP_EXOTIC, ITI_Q1_THUNDERBOLT);
	GameInfoWeaponGroup(WEAPON_GROUP_SUPER, ITI_Q1_THUNDERBOLT);
	GameInfoWeaponGroup(WEAPON_GROUP_ANY, ITI_Q1_SUPER_SHOTGUN, ITI_Q1_NAILGUN, ITI_Q1_SUPER_NAILGUN, ITI_Q1_GRENADE_LAUNCHER, ITI_Q1_ROCKET_LAUNCHER, ITI_Q1_THUNDERBOLT);
}

static void InitDoomGameInfo(game_iteminfo_t *info)
{
	// replace combat with jacket
	GameInfoReplaceItem(ITI_COMBAT_ARMOR, ITI_DOOM_ARMOR);
	// replace power screen/shield with invis
	GameInfoReplaceItem(ITI_POWER_SCREEN, ITI_DOOM_INVISIBILITY);
	GameInfoReplaceItem(ITI_POWER_SHIELD, ITI_DOOM_INVISIBILITY);
	// dummied items
	GameInfoDummyItem(ITI_SILENCER);
	GameInfoDummyItem(ITI_ANCIENT_HEAD);
	GameInfoDummyItem(ITI_BANDOLIER);
	GameInfoStartsWith(ITI_DOOM_FIST, 1);
	GameInfoStartsWith(ITI_DOOM_PISTOL, 1);
	GameInfoDefaultItem(ITI_DOOM_PISTOL);
	// weapon pickups
	GameInfoAmmoPickup(ITI_DOOM_SHOTGUN, 15);
	GameInfoAmmoPickup(ITI_DOOM_SUPER_SHOTGUN, 5);
	GameInfoAmmoPickup(ITI_DOOM_CHAINGUN, 50);
	GameInfoAmmoPickup(ITI_DOOM_PISTOL, 50);
	GameInfoAmmoPickup(ITI_DOOM_ROCKET_LAUNCHER, 5);
	GameInfoAmmoPickup(ITI_DOOM_PLASMA_GUN, 50);
	GameInfoAmmoPickup(ITI_DOOM_BFG, 1);
	// health
	GameInfoHealthPickup(ITI_STIMPACK, 2);
	GameInfoHealthPickup(ITI_MEDIUM_HEALTH, 10);
	GameInfoHealthPickup(ITI_LARGE_HEALTH, 25);
	GameInfoHealthPickup(ITI_MEGA_HEALTH, 100);
	GameInfoArmorStats(ITI_JACKET_ARMOR, 100,  100, .33f, .33f);
	GameInfoArmorStats(ITI_BODY_ARMOR, 200, 200, .50f, .50f);
	GameInfoWeaponAmmo(ITI_DOOM_FIST, 0);
	GameInfoWeaponAmmo(ITI_DOOM_CHAINSAW, 0);
	GameInfoWeaponAmmo(ITI_DOOM_SHOTGUN, 40);
	GameInfoWeaponAmmo(ITI_DOOM_SUPER_SHOTGUN, 20);
	GameInfoWeaponAmmo(ITI_DOOM_CHAINGUN, 150);
	GameInfoWeaponAmmo(ITI_DOOM_PISTOL, 150);
	GameInfoWeaponAmmo(ITI_DOOM_ROCKET_LAUNCHER, 25);
	GameInfoWeaponAmmo(ITI_DOOM_PLASMA_GUN, 60);
	GameInfoWeaponAmmo(ITI_DOOM_BFG, 4);
	GameInfoWeaponGroup(WEAPON_GROUP_RAPID, ITI_DOOM_CHAINGUN);
	GameInfoWeaponGroup(WEAPON_GROUP_SPREAD, ITI_DOOM_SHOTGUN, ITI_DOOM_SUPER_SHOTGUN);
	GameInfoWeaponGroup(WEAPON_GROUP_EXPLOSIVE, ITI_DOOM_ROCKET_LAUNCHER);
	GameInfoWeaponGroup(WEAPON_GROUP_EXOTIC, ITI_DOOM_PLASMA_GUN, ITI_DOOM_CHAINSAW);
	GameInfoWeaponGroup(WEAPON_GROUP_SUPER, ITI_DOOM_BFG);
	GameInfoWeaponGroup(WEAPON_GROUP_ANY, ITI_DOOM_CHAINSAW, ITI_DOOM_SHOTGUN, ITI_DOOM_SUPER_SHOTGUN, ITI_DOOM_CHAINGUN, ITI_DOOM_ROCKET_LAUNCHER, ITI_DOOM_PLASMA_GUN, ITI_DOOM_BFG);
}

static void InitDukeGameInfo(game_iteminfo_t *info)
{
	// replace all armors
	GameInfoReplaceItem(ITI_BODY_ARMOR, ITI_DUKE_ARMOR);
	GameInfoReplaceItem(ITI_COMBAT_ARMOR, ITI_DUKE_ARMOR);
	// nulled items
	GameInfoDummyItem(ITI_ARMOR_SHARD);
	GameInfoDummyItem(ITI_STIMPACK);
	GameInfoStartsWith(ITI_DUKE_FOOT, 1);
	GameInfoStartsWith(ITI_DUKE_PISTOL, 1);
	GameInfoDefaultItem(ITI_DUKE_PISTOL);
	// weapon pickups
	GameInfoAmmoPickup(ITI_DUKE_PISTOL, 35);
	GameInfoAmmoPickup(ITI_DUKE_CANNON, 50);
	GameInfoAmmoPickup(ITI_DUKE_SHOTGUN, 10);
	GameInfoAmmoPickup(ITI_DUKE_RPG, 4);
	GameInfoAmmoPickup(ITI_DUKE_DEVASTATOR, 15);
	GameInfoAmmoPickup(ITI_DUKE_PIPEBOMBS, 5);
	GameInfoAmmoPickup(ITI_DUKE_TRIPWIRES, 5);
	GameInfoAmmoPickup(ITI_DUKE_FREEZER, 5);
	// health
	GameInfoHealthPickup(ITI_MEDIUM_HEALTH, 10);
	GameInfoHealthPickup(ITI_LARGE_HEALTH, 30);
	GameInfoHealthPickup(ITI_MEGA_HEALTH, 50);
	GameInfoArmorStats(ITI_JACKET_ARMOR, 100, 100, .50f, .50f);
	GameInfoWeaponAmmo(ITI_DUKE_PISTOL, 150);
	GameInfoWeaponAmmo(ITI_DUKE_CANNON, 150);
	GameInfoWeaponAmmo(ITI_DUKE_SHOTGUN, 45);
	GameInfoWeaponAmmo(ITI_DUKE_RPG, 20);
	GameInfoWeaponAmmo(ITI_DUKE_DEVASTATOR, 75);
	GameInfoWeaponAmmo(ITI_DUKE_PIPEBOMBS, 15);
	GameInfoWeaponAmmo(ITI_DUKE_TRIPWIRES, 15);
	GameInfoWeaponAmmo(ITI_DUKE_FREEZER, 75);
	GameInfoWeaponGroup(WEAPON_GROUP_RAPID, ITI_DUKE_PISTOL, ITI_DUKE_CANNON);
	GameInfoWeaponGroup(WEAPON_GROUP_SPREAD, ITI_DUKE_SHOTGUN);
	GameInfoWeaponGroup(WEAPON_GROUP_EXPLOSIVE, ITI_DUKE_PIPEBOMBS, ITI_DUKE_RPG, ITI_DUKE_DEVASTATOR);
	GameInfoWeaponGroup(WEAPON_GROUP_EXOTIC, ITI_DUKE_TRIPWIRES, ITI_DUKE_FREEZER);
	//GameInfoWeaponGroup(WEAPON_GROUP_SUPER, ITI_DOOM_SHRINKER);
	GameInfoWeaponGroup(WEAPON_GROUP_ANY, ITI_DUKE_PISTOL, ITI_DUKE_CANNON, ITI_DUKE_SHOTGUN, ITI_DUKE_RPG, ITI_DUKE_DEVASTATOR, ITI_DUKE_PIPEBOMBS, ITI_DUKE_TRIPWIRES, ITI_DUKE_FREEZER);
}

game_weaponmap_t game_weaponmap[GAME_TOTAL];

#define GetMappedGameWeapon(game_from, game_to, item_id) \
	game_weaponmap[game_from].to[game_to].weapons[item_id - ITI_WEAPONS_START]
#define MapGameWeapon(game_from, game_to, item_id, new_item_id) \
	GetMappedGameWeapon(game_from, game_to, item_id) = new_item_id

#define MapWeapons(gametype, ...) \
	memcpy(game_weaponmap[GAME_Q2].to[gametype].weapons, (itemid_e[]) { __VA_ARGS__ }, sizeof((itemid_e[]) { __VA_ARGS__ }))

static void InitWeaponMap()
{
	memset(game_weaponmap, 0, sizeof(game_weaponmap));
	// start by mapping all Q2 weapons to game weapons
	// map Q2 -> Q1
	MapWeapons(GAME_Q1, ITI_Q1_AXE, ITI_Q1_SHOTGUN, ITI_Q1_SUPER_SHOTGUN, ITI_Q1_NAILGUN, ITI_Q1_SUPER_NAILGUN, ITI_Q1_GRENADE_LAUNCHER, ITI_Q1_ROCKET_LAUNCHER, ITI_Q1_THUNDERBOLT, ITI_UNUSED, ITI_UNUSED, ITI_UNUSED);
	// map Q2 -> Doom
	MapWeapons(GAME_DOOM, ITI_DOOM_FIST, ITI_DOOM_SHOTGUN, ITI_DOOM_SUPER_SHOTGUN, ITI_DOOM_PISTOL, ITI_DOOM_CHAINGUN, ITI_UNUSED, ITI_DOOM_ROCKET_LAUNCHER, ITI_DOOM_PLASMA_GUN, ITI_DOOM_CHAINSAW, ITI_DOOM_BFG, ITI_UNUSED);
	// map Q2 -> Duke
	MapWeapons(GAME_DUKE, ITI_DUKE_FOOT, ITI_DUKE_SHOTGUN, ITI_UNUSED, ITI_DUKE_PISTOL, ITI_DUKE_CANNON, ITI_DUKE_DEVASTATOR, ITI_DUKE_RPG, ITI_DUKE_FREEZER, ITI_DUKE_TRIPWIRES, ITI_UNUSED, ITI_DUKE_PIPEBOMBS);
	gametype_t game;
	itemid_e this_weapon;

	// from here, we can work backwards and figure out where the weapons map to each other between games.
	// start by completing the <game> -> Q2 round trip
	for (game = GAME_Q1; game < GAME_TOTAL; game++)
	{
		for (this_weapon = ITI_WEAPONS_START; this_weapon <= ITI_WEAPONS_END; this_weapon++)
		{
			// get weapon mapped from Q2 -> <game> from this slot
			itemid_e q2_to_this = GetMappedGameWeapon(GAME_Q2, game, this_weapon);

			if (q2_to_this != ITI_UNUSED)
				MapGameWeapon(game, GAME_Q2, q2_to_this, this_weapon);
		}
	}

	// now, we can map the rest of them by converting <game> -> Q2 then Q2 -> <new_game>
	for (game = GAME_Q1; game < GAME_TOTAL; game++)
	{
		gametype_t other_game;

		for (other_game = GAME_Q1; other_game < GAME_TOTAL; other_game++)
		{
			// don't assign to our own game
			if (game == other_game)
				continue;

			for (this_weapon = ITI_WEAPONS_START; this_weapon <= ITI_WEAPONS_END; this_weapon++)
			{
				itemid_e game_to_q2 = GetMappedGameWeapon(game, GAME_Q2, this_weapon);

				if (game_to_q2 == ITI_UNUSED)
					continue;

				itemid_e q2_to_other_game = GetMappedGameWeapon(GAME_Q2, other_game, game_to_q2);

				if (q2_to_other_game == ITI_UNUSED)
					continue;

				MapGameWeapon(game, other_game, this_weapon, q2_to_other_game);
			}
		}
	}
}

static void InitGameInfos()
{
	gametype_t game;

	for (game = GAME_Q2; game < GAME_TOTAL; game++)
		InitGameInfoBase(&game_iteminfos[game]);

	InitQ2GameInfo(&game_iteminfos[GAME_Q2]);
	InitQ1GameInfo(&game_iteminfos[GAME_Q1]);
	InitDoomGameInfo(&game_iteminfos[GAME_DOOM]);
	InitDukeGameInfo(&game_iteminfos[GAME_DUKE]);
	InitWeaponMap();
}

game_iteminfo_t game_iteminfos[GAME_TOTAL];

gitem_t *ResolveInventoryItem(edict_t *ent, gitem_t *item)
{
	itemid_e currentWeapon = GetIndexByItem(ent->server.client->pers.weapon);
	itemid_e itemid = GetIndexByItem(item);

	// Q2 and Q1 are all good the way they are
	switch (ent->server.state.game)
	{
		default:
			break;

		case GAME_DOOM:
			switch (itemid)
			{
				case ITI_WEAPON_1:
					if (currentWeapon == ITI_DOOM_CHAINSAW)
						return GetItemByIndex(ITI_DOOM_FIST);

					return GetItemByIndex(ent->server.client->pers.inventory[ITI_DOOM_CHAINSAW] > 0 ? ITI_DOOM_CHAINSAW : ITI_DOOM_FIST);

				case ITI_WEAPON_3:
					if (currentWeapon == ITI_DOOM_SUPER_SHOTGUN)
						return GetItemByIndex(ITI_DOOM_SHOTGUN);

					return GetItemByIndex(ent->server.client->pers.inventory[ITI_DOOM_SUPER_SHOTGUN] > 0 ? ITI_DOOM_SUPER_SHOTGUN : ITI_DOOM_SHOTGUN);
			}
	}

	return NULL;
}

gitem_t *ResolveItemRedirect(edict_t *ent, gitem_t *item)
{
	while (true)
	{
		int index = ITEM_INDEX(item);
		itemid_e redirect = EntityGame(ent)->item_redirects[index];

		if (redirect == ITI_UNUSED)
			return item;

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
	if (EntityGame(ent)->ammo_usages[ITEM_INDEX(weapon)] == -1)
	{
		assert(false);
		return 1; // assume it uses 1 ammo. this is temporary only; all weapons need a defined value.
	}

	return EntityGame(ent)->ammo_usages[ITEM_INDEX(weapon)];
}

// Ammo routines
float GetMaxAmmo(edict_t *ent, int has_bandolier, int has_ammo_pack)
{
	float max_ammo = DEFAULT_MAX_AMMO;

	if (has_bandolier == -1)
		has_bandolier = ent->server.client->pers.inventory[ITI_BANDOLIER];

	if (has_ammo_pack == -1)
		has_ammo_pack = ent->server.client->pers.inventory[ITI_AMMO_PACK];

	// Bandolier/Ammo Pack
	if (has_ammo_pack)
		max_ammo *= 2.0;
	else if (has_bandolier)
		max_ammo *= 1.5;

	return max_ammo;
}

bool HasEnoughAmmoToFireShots(edict_t *ent, gitem_t *weapon, int shots)
{
	return ent->server.client->pers.ammo >= GetWeaponUsageCount(ent, weapon) * shots;
}

void RemoveAmmoFromFiringShots(edict_t *ent, gitem_t *weapon, int shots)
{
	if (dmflags.infinite_ammo)
		return;

	ent->server.client->pers.ammo -= GetWeaponUsageCount(ent, weapon) * shots;

	if (ent->server.client->pers.ammo < 0)
		ent->server.client->pers.ammo = 0;
}


//======================================================================

/*
===============
GetItemByIndex
===============
*/
gitem_t *GetItemByIndex(itemid_e index)
{
	if (index == 0 || index >= game.num_items)
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

	for (i = 0 ; i < game.num_items ; i++, it++)
	{
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
static const char *q2_weapons[] = { "blaster", "shotgun", "super shotgun", "machinegun", "chaingun", "grenade launcher", "rocket launcher", "hyperblaster", "railgun", "bfg10k", "grenades" };

gitem_t *FindItem(const char *pickup_name)
{
	int     i;

	for (i = 0; i < q_countof(q2_weapons); i++)
		if (!Q_stricmp(pickup_name, q2_weapons[i]))
			return GetItemByIndex(ITI_WEAPONS_START + i);

	gitem_t *it;
	it = itemlist;

	for (i = 0 ; i < game.num_items ; i++, it++)
	{
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
	if (ent->team)
	{
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

	ent->server.flags.noclient = false;
	ent->server.solid = SOLID_TRIGGER;
	gi.linkentity(ent);
	// send an effect
	ent->server.state.event = EV_ITEM_RESPAWN;
}

bool SetRespawn(edict_t *ent, float delay)
{
	if ((ent->spawnflags & DROPPED_ITEM)
#ifdef ENABLE_COOP
		|| !deathmatch->integer
#endif
		)
		return false;

	ent->flags |= FL_RESPAWN;
	ent->server.flags.noclient = true;
	ent->server.solid = SOLID_NOT;
	ent->nextthink = level.time + delay * 1000;
	ent->think = DoRespawn;
	gi.linkentity(ent);

	if (ent->entitytype == ET_WEAPON_GROUPED || ent->entitytype == ET_AMMO)
		ent->server.state.game = (ent->server.state.game + 1) % 256;

	return true;
}


//======================================================================

bool Pickup_Powerup(edict_t *ent, edict_t *other)
{
	int			quantity;
	bool		instant;
	quantity = other->server.client->pers.inventory[ITEM_INDEX(ent->item)];

	switch (other->server.state.game)
	{
		case GAME_Q2:
#if ENABLE_COOP
			if ((skill->value == 1 && quantity >= 2) || (skill->value >= 2 && quantity >= 1))
				return false;

			if ((coop->value) && ent->item->flags.stay_coop && (quantity > 0))
				return false;
#endif

			other->server.client->pers.inventory[ITEM_INDEX(ent->item)]++;
			instant = !dmflags.store_items || ((ent->item->use == Use_Quad) && (ent->spawnflags & DROPPED_PLAYER_ITEM));
			break;

		case GAME_DUKE:
			if (quantity > 100)
				return false;

		// intentional fallthrough
		default:
			instant = true;
			break;
	}

	SetRespawn(ent, ent->item->respawn_time);

	if (instant)
	{
		if (other->server.state.game == GAME_Q2 && (ent->item->use == Use_Quad) && (ent->spawnflags & DROPPED_PLAYER_ITEM))
			quad_drop_timeout_hack = (ent->nextthink - level.time);

		ent->item->use(other, ent->item);
	}

	return true;
}

void Drop_General(edict_t *ent, gitem_t *item)
{
	Drop_Item(ent, item);
	ent->server.client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);
}


//======================================================================

bool Pickup_Adrenaline(edict_t *ent, edict_t *other)
{
	switch (other->server.state.game)
	{
		case GAME_Q2:
#ifdef ENABLE_COOP
			if (!deathmatch->integer)
				other->max_health += 1;
#endif
			if (other->health < other->max_health)
				other->health = other->max_health;

			break;

		case GAME_DOOM:
			other->health += 100;

			if (other->health > 200)
				other->health = 200;

			break;
	}

	SetRespawn(ent, ent->item->respawn_time);

	return true;
}

bool Pickup_AncientHead(edict_t *ent, edict_t *other)
{
	other->max_health += 2;

	SetRespawn(ent, ent->item->respawn_time);

	return true;
}

bool Pickup_Bandolier(edict_t *ent, edict_t *other)
{
	other->server.client->pers.ammo = min(GetMaxAmmo(other, true, CHECK_INVENTORY), other->server.client->pers.ammo + (DEFAULT_MAX_AMMO / 4));

	SetRespawn(ent, ent->item->respawn_time);

	other->server.client->pers.inventory[ITEM_INDEX(ent->item)]++;
	return true;
}

bool Pickup_Pack(edict_t *ent, edict_t *other)
{
	other->server.client->pers.ammo = min(GetMaxAmmo(other, CHECK_INVENTORY, true), other->server.client->pers.ammo + (DEFAULT_MAX_AMMO / 2));

	SetRespawn(ent, ent->item->respawn_time);

	other->server.client->pers.inventory[ITEM_INDEX(ent->item)]++;
	return true;
}

//======================================================================

void Use_Quad(edict_t *ent, gitem_t *item)
{
	int     timeout;
	ent->server.client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);

	if (quad_drop_timeout_hack)
	{
		timeout = quad_drop_timeout_hack;
		quad_drop_timeout_hack = 0;
	}
	else
		timeout = 30000;

	if (ent->server.client->quad_time > level.time)
		ent->server.client->quad_time += timeout;
	else
		ent->server.client->quad_time = level.time + timeout;

	if (ent->server.state.game != GAME_DOOM)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
	else if (ent->server.client->pers.weapon != GetItemByIndex(ITI_DOOM_FIST))
		ent->server.client->gunstates[GUN_MAIN].newweapon = GetItemByIndex(ITI_DOOM_FIST);
}

//======================================================================

void Use_Breather(edict_t *ent, gitem_t *item)
{
	ent->server.client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);

	if (ent->server.client->breather_time > level.time)
		ent->server.client->breather_time += 30000;
	else
		ent->server.client->breather_time = level.time + 30000;

	if (ent->server.state.game == GAME_Q1)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("q1/items/inv1.wav"), 1, ATTN_NORM, 0);

	//  gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void Use_Envirosuit(edict_t *ent, gitem_t *item)
{
	ent->server.client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);

	if (ent->server.client->enviro_time > level.time)
		ent->server.client->enviro_time += 30000;
	else
		ent->server.client->enviro_time = level.time + 30000;

	if (ent->server.state.game == GAME_Q1)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("q1/items/suit.wav"), 1, ATTN_NORM, 0);

	//  gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void    Use_Invulnerability(edict_t *ent, gitem_t *item)
{
	ent->server.client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);

	if (ent->server.client->invincible_time > level.time)
		ent->server.client->invincible_time += 30000;
	else
		ent->server.client->invincible_time = level.time + 30000;

	if (ent->server.state.game == GAME_Q1 || ent->server.state.game == GAME_Q2)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void    Use_Silencer(edict_t *ent, gitem_t *item)
{
	ent->server.client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem(ent);
	ent->server.client->silencer_shots += 30;
	//  gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

#if ENABLE_COOP
bool Pickup_Key(edict_t *ent, edict_t *other)
{
	if (coop->value)
	{
		if (GetIndexByItem(ent->item) == ITI_POWER_CUBE)
		{
			if (other->server.client->pers.power_cubes & ((ent->spawnflags & 0x0000ff00) >> 8))
				return false;

			other->server.client->pers.inventory[ITEM_INDEX(ent->item)]++;
			other->server.client->pers.power_cubes |= ((ent->spawnflags & 0x0000ff00) >> 8);
		}
		else
		{
			if (other->server.client->pers.inventory[ITEM_INDEX(ent->item)])
				return false;

			other->server.client->pers.inventory[ITEM_INDEX(ent->item)] = 1;
		}

		return true;
	}

	other->server.client->pers.inventory[ITEM_INDEX(ent->item)]++;
	return true;
}
#endif

//======================================================================

bool ShouldSwapToPotentiallyBetterWeapon(edict_t *ent, gitem_t *new_item, bool is_new);

bool Add_Ammo(edict_t *ent, float count, itemid_e index, bool pickup)
{
	int         max_ammo;

	if (!ent->server.client)
		return false;

	max_ammo = GetMaxAmmo(ent, CHECK_INVENTORY, CHECK_INVENTORY);

	if (!max_ammo || ent->server.client->pers.ammo >= max_ammo)
		return false;

	if (pickup)
	{
		gitem_t		*item = GetItemByIndex(index);

		// only affects Q2 and Duke realistically
		if (item->flags.is_weapon && ShouldSwapToPotentiallyBetterWeapon(ent, item, true))
			ent->server.client->gunstates[GUN_MAIN].newweapon = item;
	}

	ent->server.client->pers.ammo = min(max_ammo, ent->server.client->pers.ammo + count);
	return true;
}

bool do_propagate = true;
bool Pickup_Ammo(edict_t *ent, edict_t *other)
{
	float       count;
	gitem_t		*item = ent->item;

	if (item->flags.is_weapon && dmflags.infinite_ammo)
		count = DEFAULT_MAX_AMMO * 100;
	else if (ent->count)
		count = ent->count;
	else
	{
		// TODO
		float real_num = 20;//game_iteminfos[other->server.state.game].dynamic.ammo_pickup_amounts[ITEM_INDEX(ent->real_item)];

		if (real_num != -1)
			count = real_num;
		else
		{
			count = 1;
			Com_Printf("No pickup count for %i %s\n", other->server.state.game, item->pickup_name);
		}
	}

	if (!Add_Ammo(other, count, GetIndexByItem(item), true)
#if ENABLE_COOP
		&& !invasion->value
#endif
		)
		return false;

	if (!(ent->spawnflags & DROPPED_PLAYER_ITEM))
	{
#if ENABLE_COOP
		if (invasion->value)
		{
			if (do_propagate)
			{
				int i;
				// in invasion, propagate item to other clients
				do_propagate = false;

				for (i = 0; i < game.maxclients; ++i)
				{
					edict_t *cl = &g_edicts[i + 1];

					if (!cl->server.inuse || !cl->server.client->pers.connected || !cl->server.state.game || cl == other)
						continue;

					Pickup_Ammo(ent, cl);
				}

				do_propagate = true;
			}
		}
		else
#endif
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

	int quantity = game_iteminfos[ent->server.state.game].dynamic.ammo_pickup_amounts[index];

	if (ent->server.client->pers.inventory[index] >= quantity)
		dropped->count = quantity;
	else
		dropped->count = ent->server.client->pers.inventory[index];

	ent->server.client->pers.inventory[index] -= dropped->count;

	if (ent->server.client->pers.weapon &&
		ent->server.client->pers.weapon == item &&
		!ent->server.client->pers.inventory[index])
		AttemptBetterWeaponSwap(ent);

	ValidateSelectedItem(ent);*/
}


//======================================================================

void MegaHealth_think(edict_t *self)
{
	if (self->server.owner->health > self->server.owner->max_health)
	{
		self->nextthink = level.time + 1000;
		self->server.owner->health -= 1;
		return;
	}

	if (!SetRespawn(self, 20))
		G_FreeEdict(self);
}

typedef struct
{
	bool ignore_max : 1;
	bool timed : 1;
} health_flags_t;

bool Pickup_Health(edict_t *ent, edict_t *other)
{
	health_flags_t flags = { false, false };

	switch (GetIndexByItem(ent->item))
	{
		case ITI_STIMPACK:
			flags.ignore_max = true;
			break;

		case ITI_MEGA_HEALTH:
			flags.ignore_max = true;

			if (other->server.state.game == GAME_Q2 || other->server.state.game == GAME_Q1)
				flags.timed = true;
			else if (other->server.state.game == GAME_DUKE && other->health >= 200)
				return false;

			break;
	}

	if (!flags.ignore_max && (other->health >= other->max_health))
		return false;

	if (other->server.state.game == GAME_DUKE && other->health < (other->max_health / 4) && GetIndexByItem(ent->item) != ITI_MEGA_HEALTH)
		gi.sound(other, CHAN_VOICE, gi.soundindex("duke/needed03.wav"), 1, ATTN_NORM, 0);

	if (other->server.state.game == GAME_DOOM && GetIndexByItem(ent->item) == ITI_DOOM_MEGA_SPHERE)
	{
		other->health = 200;
		other->server.client->pers.inventory[ITI_DOOM_ARMOR] = 0;
		other->server.client->pers.inventory[ITI_DOOM_MEGA_ARMOR] = 200;
	}
	else
	{
		other->health += (ent->count) ? ent->count : (int) EntityGame(other)->ammo_pickups[GetIndexByItem(ent->item)];

		if (!flags.ignore_max)
		{
			if (other->health > other->max_health)
				other->health = other->max_health;
		}
		else
		{
			if (other->server.state.game == GAME_DUKE && other->health > 200)
				other->health = 200;
		}
	}

	if (flags.timed)
	{
		ent->think = MegaHealth_think;
		ent->nextthink = level.time + 5000;
		ent->server.owner = other;
		ent->flags |= FL_RESPAWN;
		ent->server.flags.noclient = true;
		ent->server.solid = SOLID_NOT;
	}
	else
		SetRespawn(ent, 30);

	return true;
}

//======================================================================

itemid_e ArmorIndex(edict_t *ent)
{
	if (ent->server.client)
	{
		itemid_e i;

		for (i = ITI_JACKET_ARMOR; i <= ITI_BODY_ARMOR; i++)
		{
			if (ent->server.client->pers.inventory[i] > 0)
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
		result->old_armor_count = ent->server.client->pers.inventory[result->old_armor_index];
		result->old_armor = &game_iteminfos[ent->server.state.game].armors[result->old_armor_index - ITI_JACKET_ARMOR];
	}
	else
	{
		result->old_armor_count = 0;
		result->old_armor = NULL;
	}

	// try armor shards first
	if (GetIndexByItem(armor) == ITI_ARMOR_SHARD)
	{
		switch (ent->server.state.game)
		{
			case GAME_Q2:
			case GAME_DOOM:
				if (!result->old_armor_index)
				{
					result->new_armor_index = ITI_JACKET_ARMOR;
					result->new_armor = &game_iteminfos[ent->server.state.game].armors[result->new_armor_index - ITI_JACKET_ARMOR];
				}
				else
				{
					result->new_armor_index = ITI_NULL;
					result->new_armor = NULL;
				}

				result->new_armor_count = result->old_armor_count + ((ent->server.state.game == GAME_Q2) ? 2 : 1);

				if (ent->server.state.game == GAME_DOOM)
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
		result->new_armor = &game_iteminfos[ent->server.state.game].armors[result->new_armor_index - ITI_JACKET_ARMOR];
		result->new_armor_count = result->new_armor->base_count;
		return true;
	}
	else
	{
		result->new_armor_index = ITEM_INDEX(armor);
		result->new_armor = &game_iteminfos[ent->server.state.game].armors[result->new_armor_index - ITI_JACKET_ARMOR];

		// picking up new armor
		// different games have different algorithms
		switch (ent->server.state.game)
		{
			case GAME_Q2:
				if (result->new_armor->normal_protection > result->old_armor->normal_protection)
				{
					// calc new armor values
					float salvage = result->old_armor->normal_protection / result->new_armor->normal_protection;
					float salvagecount = salvage * ent->server.client->pers.inventory[result->old_armor_index];
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
					int newcount = ent->server.client->pers.inventory[result->old_armor_index] + salvagecount;

					if (newcount > result->old_armor->max_count)
						newcount = result->old_armor->max_count;

					// if we're already maxed out then we don't need the new armor
					if (ent->server.client->pers.inventory[result->old_armor_index] >= newcount)
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
			other->server.client->pers.inventory[armor.old_armor_index] = 0;
	}

	if (armor.new_armor_count)
	{
		if (armor.new_armor_index)
			other->server.client->pers.inventory[armor.new_armor_index] = armor.new_armor_count;
		else
			other->server.client->pers.inventory[armor.old_armor_index] = armor.new_armor_count;
	}

	SetRespawn(ent, 20);

	return true;
}

//======================================================================

power_armor_type_e PowerArmorType(edict_t *ent)
{
	if (!ent->server.client)
		return POWER_ARMOR_NONE;

	if (!(ent->flags & FL_POWER_ARMOR))
		return POWER_ARMOR_NONE;

	if (ent->server.client->pers.inventory[ITI_POWER_SHIELD] > 0)
		return POWER_ARMOR_SHIELD;

	if (ent->server.client->pers.inventory[ITI_POWER_SCREEN] > 0)
		return POWER_ARMOR_SCREEN;

	return POWER_ARMOR_NONE;
}

void Use_PowerArmor(edict_t *ent, gitem_t *item)
{
	if (ent->flags & FL_POWER_ARMOR)
	{
		ent->flags &= ~FL_POWER_ARMOR;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
	}
	else
	{
		if (ent->server.client->pers.ammo < AMMO_PER_POWER_ARMOR_ABSORB)
		{
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
	quantity = other->server.client->pers.inventory[ITEM_INDEX(ent->item)];
	other->server.client->pers.inventory[ITEM_INDEX(ent->item)]++;

#ifdef ENABLE_COOP
	if (deathmatch->value)
#endif
	{
		SetRespawn(ent, ent->item->respawn_time);

		// auto-use for DM only if we didn't already have one
		if (!quantity)
			ent->item->use(other, ent->item);
	}

	return true;
}

void Drop_PowerArmor(edict_t *ent, gitem_t *item)
{
	if ((ent->flags & FL_POWER_ARMOR) && (ent->server.client->pers.inventory[ITEM_INDEX(item)] == 1))
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

	if (!other->server.client)
		return;

	if (other->health < 1)
		return;     // dead people can't pickup

	gitem_t *item;

	if (ent->entitytype == ET_WEAPON_GROUPED)
	{
		game_weapon_group_t *group = &GameTypeGame(other->server.state.game)->weapon_groups[ent->weapongroup];
		item = GetItemByIndex(group->weapons[ent->server.state.game % group->num_weapons]);
	}
	else
		item = ent->item;

	// check item redirections
	item = ResolveItemRedirect(other, item);

	if (!item || !item->pickup)
		return;     // not a grabbable item?

	bool had = !other->server.client->pers.inventory[ITEM_INDEX(item)];
	ent->real_item = ent->item;
	ent->item = item;
	taken = item->pickup(ent, other);
	ent->item = ent->real_item;

	if (taken)
	{
		// flash the screen
		other->server.client->bonus_alpha = 0.25f;
		// show icon and name on status bar
		other->server.client->pickup_msg_time = level.time + 3000;
		other->server.client->pickup_item = item;
		other->server.client->pickup_item_first = had;

		// change selected item
		if (item->use)
			other->server.client->server.ps.stats.selected_item = other->server.client->pers.selected_item = ITEM_INDEX(item);

		gi.sound(other, CHAN_ITEM, gi.soundindex(item->pickup_sound), 1, ATTN_NORM, 0);
	}

	if (!(ent->spawnflags & ITEM_TARGETS_USED))
	{
		G_UseTargets(ent, other);
		ent->spawnflags |= ITEM_TARGETS_USED;
	}

	if (!taken)
		return;

	if (
#if ENABLE_COOP
		invasion->value || !(coop->value && item->flags.stay_coop) ||
#endif
		(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
	{
		if (ent->flags & FL_RESPAWN)
			ent->flags &= ~FL_RESPAWN;
		else
			G_FreeEdict(ent);
	}
}

//======================================================================

void drop_temp_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->server.owner)
		return;

	Touch_Item(ent, other, plane, surf);
}

void drop_make_touchable(edict_t *ent)
{
	ent->touch = Touch_Item;

#ifdef ENABLE_COOP
	if (deathmatch->value)
#endif
	{
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
	dropped->server.state.effects = item->world_model_flags;
	VectorSet(dropped->server.mins, -15, -15, -15);
	VectorSet(dropped->server.maxs, 15, 15, 15);
	dropped->server.state.modelindex = gi.modelindex(dropped->item->world_model);
	dropped->server.solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;
	dropped->touch = drop_temp_touch;
	dropped->server.owner = ent;

	if (ent->server.client)
	{
		trace_t trace;
		AngleVectors(ent->server.client->v_angle, forward, right, up);
		VectorSet(offset, 24, 0, -16);
		G_ProjectSource(ent->server.state.origin, offset, forward, right, dropped->server.state.origin);
		trace = gi.trace(ent->server.state.origin, dropped->server.mins, dropped->server.maxs,
				dropped->server.state.origin, ent, CONTENTS_SOLID);
		VectorCopy(trace.endpos, dropped->server.state.origin);
	}
	else
	{
		AngleVectors(ent->server.state.angles, forward, right, up);
		VectorCopy(ent->server.state.origin, dropped->server.state.origin);
	}

	if (ent->health <= 0 && ent->server.state.game == GAME_DOOM)
		dropped->velocity[2] = 300;
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
	ent->server.flags.noclient = false;
	ent->use = NULL;

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->server.solid = SOLID_BBOX;
		ent->touch = NULL;
	}
	else
	{
		ent->server.solid = SOLID_TRIGGER;
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
	bool		startIndex = (ent->server.state.modelindex == 0);

	if (startIndex)
	{
		VectorSet(ent->server.mins, -15, -15, -15);
		VectorSet(ent->server.maxs, 15, 15, 15);

		if (ent->model)
			gi.setmodel(ent, ent->model);
		else if (ent->item)
			ent->server.state.modelindex = gi.modelindex(ent->item->world_model);

		ent->touch = Touch_Item;
	}

	ent->server.solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;
	v = tv(0, 0, -128);
	VectorAdd(ent->server.state.origin, v, dest);
	tr = gi.trace(ent->server.state.origin, ent->server.mins, ent->server.maxs, dest, ent, MASK_SOLID);

	if (tr.startsolid)
	{
		if (ent->item)
			Com_Printf("droptofloor: %s startsolid at %s\n", ent->item->classname, vtos(ent->server.state.origin));
		else
			Com_Printf("droptofloor: entityid %i startsolid at %s\n", ent->entitytype, vtos(ent->server.state.origin));

		G_FreeEdict(ent);
		return;
	}

	VectorCopy(tr.endpos, ent->server.state.origin);

	if (ent->team)
	{
		ent->flags &= ~FL_TEAMSLAVE;
		ent->chain = ent->teamchain;
		ent->teamchain = NULL;
		ent->server.flags.noclient = true;
		ent->server.solid = SOLID_NOT;

		if (ent == ent->teammaster)
		{
			ent->nextthink = level.time + 1;
			ent->think = DoRespawn;
		}
	}

	if (startIndex)
	{
		if (ent->spawnflags & ITEM_NO_TOUCH)
		{
			ent->server.solid = SOLID_BBOX;
			ent->touch = NULL;
		}

		if (ent->spawnflags & ITEM_TRIGGER_SPAWN)
		{
			ent->server.flags.noclient = true;
			ent->server.solid = SOLID_NOT;
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
	const char    *s, *start;
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

	if (it->ammo_model)
		gi.modelindex(it->ammo_model);

	if (it->ammo_icon)
		gi.modelindex(it->ammo_icon);

	// parse the space seperated precache string for other items
	s = it->precaches;

	if (!s || !s[0])
		return;

	while (*s)
	{
		start = s;

		while (*s && *s != ' ')
			s++;

		len = s - start;

		if (len > MAX_QPATH || len < 5)
			Com_Error(ERR_FATAL, "PrecacheItem: %s has bad precache string", it->classname);

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
	if (!ent->entitytype)
		ent->entitytype = ET_ITEM;

	ent->server.state.game = GAME_NONE;

	PrecacheItem(item);

#if ENABLE_COOP
	if (ent->spawnflags)
	{
		if (GetIndexByItem(item) != ITI_POWER_CUBE)
		{
			ent->spawnflags = 0;
			Com_Printf("%s at %s has invalid spawnflags set\n", item->classname, vtos(ent->server.state.origin));
		}
	}
#endif

	// some items will be prevented in deathmatch
#ifdef ENABLE_COOP
	if (deathmatch->value)
#endif
	{
		if (dmflags.no_armor && item->flags.is_armor)
		{
			G_FreeEdict(ent);
			return;
		}

		if (dmflags.no_items && item->flags.is_powerup)
		{
			G_FreeEdict(ent);
			return;
		}

		if (dmflags.no_health && item->flags.is_health)
		{
			G_FreeEdict(ent);
			return;
		}

		if (dmflags.infinite_ammo && item->flags.is_ammo)
		{
			G_FreeEdict(ent);
			return;
		}
	}
	
#if ENABLE_COOP
	if (coop->value && GetIndexByItem(item) == ITI_POWER_CUBE)
	{
		ent->spawnflags |= (1 << (8 + level.power_cubes));
		level.power_cubes++;
	}

	// don't let them drop items that stay in a coop game
	if ((coop->value) && (item->flags & IT_STAY_COOP))
		item->drop = NULL;
#endif

	ent->item = item;
	ent->server.state.effects = item->world_model_flags;

	if (ent->entitytype == ET_WEAPON_GROUPED || ent->entitytype == ET_AMMO)
	{
		ent->server.state.skinnum = ent->weapongroup;

		if (ent->entitytype == ET_AMMO)
		{
			ent->server.state.skinnum |= 1 << 7;
			ent->server.state.game = (level.ammo_spawn_id++) % 256;
		}
		else
			ent->server.state.game = (level.weapon_spawn_id++) % 256;

		ent->server.state.effects = EF_GROUPED_ITEM;
	}

	ent->nextthink = level.time + game.frametime * 2;    // items start after other solids
	ent->think = droptofloor;

	if (ent->model)
		gi.modelindex(ent->model);
}

//======================================================================

gitem_t itemlist[ITI_TOTAL];

static inline void InitItem(itemid_e id, const char *classname, item_pickup_func pickup, item_use_func use, item_drop_func drop,
	const char *pickup_sound, const char *world_model, int world_model_flags, const char *view_model, const char *icon,
	const char *pickup_name, int flags, const char *weapmodel, const char *precaches, int respawn_time, const char *ammo_model,
	const char *ammo_icon)
{
	itemlist[id] = (gitem_t)
	{
		.classname = classname,
		.pickup = pickup,
		.use = use,
		.drop = drop,
		.pickup_sound = pickup_sound,
		.world_model = world_model,
		.world_model_flags = world_model_flags,
		.view_model = view_model,
		.icon = icon,
		.pickup_name = pickup_name,
		.flags = flags,
		.weapmodel = weapmodel,
		.precaches = precaches,
		.respawn_time = respawn_time,
		.ammo_model = ammo_model,
		.ammo_icon = ammo_icon
	};
}

static inline void InitArmor(itemid_e id, const char *classname, const char *pickup_sound, const char *world_model, const char *icon, const char *pickup_name)
{
	InitItem(id, classname, Pickup_Armor, NULL, NULL, pickup_sound, world_model, 0, NULL, icon, pickup_name, IT_ARMOR, NULL, NULL, 0, NULL, NULL);
}

static inline void InitPowerup(itemid_e id, const char *classname, item_pickup_func pickup, item_use_func use, item_drop_func drop, const char *pickup_sound,
	const char *world_model, const char *icon, const char *pickup_name, int flags, const char *precaches, int respawn_time)
{
	InitItem(id, classname, pickup, use, drop, pickup_sound, world_model, 0, NULL, icon, pickup_name, flags, NULL, precaches, respawn_time, NULL, NULL);
}

static inline void InitWeapon(itemid_e id, const char *classname, const char *world_model, const char *view_model, const char *icon, const char *pickup_name,
	const char *weapmodel, const char *precaches, const char *ammo_model, const char *ammo_icon)
{
	InitItem(id, classname, world_model ? Pickup_Weapon : NULL, Use_Weapon, world_model ? Drop_Weapon : NULL, world_model ? "%s_weappickup" : NULL, world_model, 0, view_model, icon, pickup_name, IT_WEAPON | IT_STAY_COOP, weapmodel, precaches, 0, ammo_model, ammo_icon);
}

static inline void InitHealth(itemid_e id, const char *classname, const char *pickup_sound, const char *world_model)
{
	InitItem(id, classname, Pickup_Health, NULL, NULL, pickup_sound, world_model, 0, NULL, "i_health", "Health", IT_HEALTH, NULL, NULL, 0, NULL, NULL);
}

#if ENABLE_COOP
static inline void InitKey(itemid_e id, const char *classname, const char *world_model, const char *icon, const char *pickup_name)
{
	InitItem(id, classname, Pickup_Key, NULL, Drop_General, "items/pkup.wav", world_model, EF_ROTATE, NULL, icon, pickup_name, IT_STAY_COOP | IT_KEY, NULL, NULL, 0, NULL, NULL);
}
#endif

#define InitWeaponQuick(index, weapmodel, precaches) \
	InitWeapon(ITI_WEAPON_ ## index, "weapon_" #index, "%wg_weapon_" #index, "%wv_weapon_" #index, "%w_weapon_" #index, "Weapon " #index, weapmodel, precaches, "%wa_weapon_" #index, "%wa_weapon_" #index)

#define InitWeaponInitialQuick(index, weapmodel, precaches) \
	InitWeapon(ITI_WEAPON_ ## index, "weapon_" #index, NULL, "%wv_weapon_" #index, NULL, "Weapon " #index, weapmodel, precaches, NULL, NULL)

static void InitItemList()
{
	InitArmor(ITI_JACKET_ARMOR, "item_armor_jacket", "%i_armor", "%i_jacket_armor", "%i_jacketarmor", "Jacket Armor");
	InitArmor(ITI_COMBAT_ARMOR, "item_armor_combat", "%i_armor", "%i_combat_armor", "%i_combatarmor", "Combat Armor");
	InitArmor(ITI_BODY_ARMOR, "item_armor_body", "%i_armor", "%i_body_armor", "%i_bodyarmor", "Body Armor");
	InitArmor(ITI_ARMOR_SHARD, "item_armor_shard", "%i_armorshard", "%i_armorshard", "%i_jacketarmor", "Armor Shard");
	InitPowerup(ITI_POWER_SCREEN, "item_power_screen", Pickup_PowerArmor, Use_PowerArmor, Drop_PowerArmor, "misc/ar3_pkup.wav", "%i_powerscreen", "i_powerscreen", "Power Screen", IT_ARMOR, "misc/power2.wav misc/power1.wav", 60);
	InitPowerup(ITI_POWER_SHIELD, "item_power_shield", Pickup_PowerArmor, Use_PowerArmor, Drop_PowerArmor, "misc/ar3_pkup.wav", "%i_powershield", "i_powershield", "Power Shield", IT_ARMOR, "misc/power2.wav misc/power1.wav", 60);
	InitWeaponInitialQuick(1, "#w_blaster.md2", "misc/lasfly.wav");
	InitWeaponQuick(2, "#w_shotgun.md2", "");
	InitWeaponQuick(3, "#w_sshotgun.md2", "");
	InitWeaponQuick(4, "#w_machinegun.md2", "");
	InitWeaponQuick(5, "#w_chaingun.md2", "weapons/chngnu1a.wav weapons/chngnd1a.wav weapons/chngnl1a.wav");
	InitWeaponQuick(6, "#w_glauncher.md2", "models/objects/grenade/tris.md2 weapons/grenlb1b.wav");
	InitWeaponQuick(7, "#w_rlauncher.md2", "%e_rocket weapons/rockfly.wav");
	InitWeaponQuick(8, "#w_hyperblaster.md2", "weapons/hyprbl1a.wav weapons/hyprbd1a.wav");
	InitWeaponQuick(9, "#w_railgun.md2", "weapons/rg_hum.wav");
	InitWeaponQuick(0, "#w_bfg.md2", "sprites/s_bfg1.sp2 sprites/s_bfg2.sp2 sprites/s_bfg3.sp2 weapons/bfg__l1a.wav weapons/bfg__x1b.wav weapons/bfg_hum.wav");
	InitWeaponQuick(G, "#a_grenades.md2", "weapons/hgrent1a.wav weapons/hgrena1b.wav weapons/hgrenc1b.wav weapons/hgrenb1a.wav weapons/hgrenb2a.wav");
	InitItem(ITI_AMMO, "ammo", Pickup_Ammo, NULL, Drop_Ammo, "%s_ammopickup", NULL, 0, NULL, NULL, "Ammo", IT_AMMO, NULL, NULL, 0, NULL, NULL);
	InitHealth(ITI_STIMPACK, "item_health_small", "%i_stimpack", "%i_stimpack");
	InitHealth(ITI_MEDIUM_HEALTH, "item_health", "%i_smhealth", "%i_smhealth");
	InitHealth(ITI_LARGE_HEALTH, "item_health_large", "%i_lghealth", "%i_lghealth");
	InitHealth(ITI_MEGA_HEALTH, "item_health_mega", "%i_megahealth", "%i_megahealth");
	InitPowerup(ITI_QUAD_DAMAGE, "item_quad", Pickup_Powerup, Use_Quad, Drop_General, "%s_powerup", "%i_quaddamage", "p_quad", "Quad Damage", IT_POWERUP, "items/damage.wav items/damage2.wav items/damage3.wav", 60);
	InitPowerup(ITI_INVULNERABILITY, "item_invulnerability", Pickup_Powerup, Use_Invulnerability, Drop_General, "%s_powerup", "%i_invulnerability", "p_invulnerability", "Invulnerability", IT_POWERUP, "items/protect.wav items/protect2.wav items/protect4.wav", 300);
	InitPowerup(ITI_ENVIRONMENT_SUIT, "item_enviro", Pickup_Powerup, Use_Envirosuit, Drop_General, "%s_powerup", "%i_environmentsuit", "p_envirosuit", "Environment Suit", IT_STAY_COOP | IT_POWERUP, "", 60);
	InitPowerup(ITI_SILENCER, "item_silencer", Pickup_Powerup, Use_Silencer, Drop_General, "%s_powerup", "%i_silencer", "p_silencer", "Silencer", IT_POWERUP, "", 60);
	InitPowerup(ITI_REBREATHER, "item_breather", Pickup_Powerup, Use_Breather, Drop_General, "%s_powerup", "%i_rebreather", "p_rebreather", "Rebreather", IT_STAY_COOP | IT_POWERUP, "items/airout.wav", 60);
	InitPowerup(ITI_ANCIENT_HEAD, "item_ancient_head", Pickup_AncientHead, NULL, NULL, "%s_powerup", "%i_ancienthead", "i_fixme", "Ancient Head", IT_NONE, NULL, 60);
	InitPowerup(ITI_ADRENALINE, "item_adrenaline", Pickup_Adrenaline, NULL, NULL, "%s_powerup", "%i_adrenaline", "p_adrenaline", "Adrenaline", IT_NONE, NULL, 60);
	InitPowerup(ITI_BANDOLIER, "item_bandolier", Pickup_Bandolier, NULL, NULL, "%s_powerup", "%i_bandolier", "p_bandolier", "Bandolier", IT_NONE, NULL, 60);
	InitPowerup(ITI_AMMO_PACK, "item_pack", Pickup_Bandolier, NULL, NULL, "%s_powerup", "%i_backpack", "i_pack", "Ammo Pack", IT_NONE, NULL, 180);
#if ENABLE_COOP
	InitKey(ITI_DATA_CD, "key_data_cd", "models/items/keys/data_cd/tris.md2", "k_datacd", "Data CD");
	InitKey(ITI_POWER_CUBE, "key_power_cube", "models/items/keys/power/tris.md2", "k_powercube", "Power Cube");
	InitKey(ITI_PYRAMID_KEY, "key_pyramid", "models/items/keys/pyramid/tris.md2", "k_pyramid", "Pyramid Key");
	InitKey(ITI_DATA_SPINNER, "key_data_spinner", "models/items/keys/spinner/tris.md2", "k_dataspin", "Data Spinner");
	InitKey(ITI_SECURITY_PASS, "key_pass", "models/items/keys/pass/tris.md2", "k_security", "Security Pass");
	InitKey(ITI_BLUE_KEY, "key_blue_key", "models/items/keys/key/tris.md2", "k_bluekey", "Blue Key");
	InitKey(ITI_RED_KEY, "key_red_key", "models/items/keys/red_key/tris.md2", "k_redkey", "Red Key");
	InitKey(ITI_COMMANDERS_HEAD, "key_commander_head", "models/monsters/commandr/head/tris.md2", "k_comhead", "Commander's Head");
	InitKey(ITI_AIRSTRIKE_MARKER, "key_airstrike_target", "models/items/keys/target/tris.md2", "i_airstrike", "Airstrike Marker");
#endif
}

void Weapons_Init();

const char *weapon_names[GAME_TOTAL][ITI_WEAPONS_END - ITI_WEAPONS_START + 1] =
{
	{
		NULL
	}, // null
	// Q2
	{
		"blaster",
		"shotgun",
		"super_shotgun",
		"machinegun",
		"chaingun",
		"grenade_launcher",
		"rocket_launcher",
		"hyperblaster",
		"railgun",
		"bfg10k",
		"grenades"
	},
	// Q1
	{
		"axe",
		"shotgun",
		"super_shotgun",
		"nailgun",
		"super_nailgun",
		"grenade_launcher",
		"rocket_launcher",
		"thunderbolt",
		NULL,
		NULL,
		NULL
	},
	// Doom
	{
		"fist",
		"pistol",
		"shotgun",
		"chaingun",
		"rocket_launcher",
		"plasma_gun",
		"bfg9000",
		"super_shotgun",
		NULL,
		"chainsaw",
		NULL
	},
	// Duke
	{
		"foot",
		"pistol",
		"shotgun",
		"cannon",
		"rpg",
		"pipebombs",
		NULL,
		"devastator",
		"tripwires",
		"freezer",
		NULL
	}
};

const char *game_names[GAME_TOTAL] =
{
	NULL,
	"q2",
	"q1",
	"doom",
	"duke"
};

void CheckWeaponBalanceCvars()
{
	gametype_t game;
	int i;

	for (game = GAME_Q2; game < GAME_TOTAL; game++)
		for (i = ITI_WEAPONS_START; i <= ITI_WEAPONS_END; i++)
		{
			if (game_iteminfos[game].default_ammo_usages[i] <= 0)
				continue;

			const char *weapon_name = weapon_names[game][i - ITI_WEAPONS_START];

			if (!weapon_name)
				continue;

			char cvar_name[MAX_QPATH];
			Q_snprintf(cvar_name, sizeof(cvar_name), "balance_%s_%s", game_names[game], weapon_name);
			cvar_t *cvar = Cvar_Get(cvar_name, "", CVAR_ARCHIVE);

			if (!cvar->value)
			{
				game_iteminfos[game].ammo_usages[i] = game_iteminfos[game].default_ammo_usages[i];
				continue;
			}

			game_iteminfos[game].ammo_usages[i] = COUNT_FOR_SHOTS(cvar->value);
		}
}

void InitWeaponSeeds()
{
	char seed_info[CS_SIZE] = { 0 };
	char *seed_ptr = seed_info;
	size_t seed_size = 0;

	for (weapon_group_e group = WEAPON_GROUP_RAPID; group < WEAPON_GROUP_MAX; group++)
	{
		if (group != 0)
			seed_size = Q_strlcat(seed_ptr, "|", sizeof(seed_info));

		for (gametype_t game = GAME_Q2; game < GAME_TOTAL; game++)
		{
			game_weapon_group_t *weapon_group = &game_iteminfos[game].weapon_groups[group];

			if (game != GAME_Q2)
				seed_size = Q_strlcat(seed_ptr, ";", sizeof(seed_info));

			for (uint32_t i = 0; i < weapon_group->num_weapons; i++)
			{
				gitem_t *weapon = GetItemByIndex(weapon_group->weapons[i]);

				if (i)
					seed_size = Q_strlcat(seed_ptr, ",", sizeof(seed_info));

				seed_size = Q_strlcat(seed_ptr, va("%d", gi.modelindex(weapon->world_model)), sizeof(seed_info));

				if (weapon->ammo_model && game_iteminfos[game].ammo_usages[GetIndexByItem(weapon)])
				{
					seed_size = Q_strlcat(seed_ptr, ":", sizeof(seed_info));
					seed_size = Q_strlcat(seed_ptr, va("%d", gi.modelindex(weapon->ammo_model)), sizeof(seed_info));
				}
			}
		}
	}

	gi.configstring(CS_SEEDS, seed_info);
}

void InitItems(void)
{
	game.num_items = q_countof(itemlist);
	InitItemList();
	InitGameInfos();
	CheckWeaponBalanceCvars();
}

void Spawn_Weapons()
{
	level.weapon_spawn_id = Q_rand();
	level.ammo_spawn_id = Q_rand();

	edict_t *weapon = NULL, *ammo = NULL;

	while ((weapon = G_FindByType(weapon, ET_WEAPON_GROUPED)))
		SpawnItem(weapon, GetItemByIndex(ITI_WEAPON));

	while ((ammo = G_FindByType(ammo, ET_AMMO)))
		SpawnItem(ammo, GetItemByIndex(ITI_AMMO));
}

void SP_weapon(edict_t *ent, const char *classname)
{
	ent->entitytype = ET_WEAPON_GROUPED;
	ent->weapongroup = WEAPON_GROUP_ANY;
	
	if (!Q_strcasecmp(classname, "weapon_shotgun") || !Q_strcasecmp(classname, "weapon_supershotgun") || !Q_strcasecmp(classname, "weapon_spread"))
		ent->weapongroup = WEAPON_GROUP_SPREAD;
	else if (!Q_strcasecmp(classname, "weapon_machinegun") || !Q_strcasecmp(classname, "weapon_chaingun") || !Q_strcasecmp(classname, "weapon_rapid"))
		ent->weapongroup = WEAPON_GROUP_RAPID;
	else if (!Q_strcasecmp(classname, "weapon_grenadelauncher") || !Q_strcasecmp(classname, "weapon_rocketlauncher") || !Q_strcasecmp(classname, "weapon_explosive"))
		ent->weapongroup = WEAPON_GROUP_EXPLOSIVE;
	else if (!Q_strcasecmp(classname, "weapon_hyperblaster") || !Q_strcasecmp(classname, "weapon_railgun") || !Q_strcasecmp(classname, "weapon_exotic"))
		ent->weapongroup = WEAPON_GROUP_EXOTIC;
	else if (!Q_strcasecmp(classname, "weapon_bfg") || !Q_strcasecmp(classname, "weapon_super"))
		ent->weapongroup = WEAPON_GROUP_SUPER;

	gi.linkentity(ent);
}

void SP_ammo(edict_t *ent, const char *classname)
{
	ent->entitytype = ET_AMMO;
	ent->weapongroup = WEAPON_GROUP_ANY;
	
	if (!Q_strcasecmp(classname, "ammo_shells") || !Q_strcasecmp(classname, "ammo_spread"))
		ent->weapongroup = WEAPON_GROUP_SPREAD;
	else if (!Q_strcasecmp(classname, "ammo_bullets") || !Q_strcasecmp(classname, "ammo_rapid"))
		ent->weapongroup = WEAPON_GROUP_RAPID;
	else if (!Q_strcasecmp(classname, "ammo_grenades") || !Q_strcasecmp(classname, "ammo_rockets") || !Q_strcasecmp(classname, "ammo_explosive"))
		ent->weapongroup = WEAPON_GROUP_EXPLOSIVE;
	else if (!Q_strcasecmp(classname, "ammo_cells") || !Q_strcasecmp(classname, "ammo_slugs") || !Q_strcasecmp(classname, "ammo_exotic"))
		ent->weapongroup = WEAPON_GROUP_EXOTIC;
	else if (!Q_strcasecmp(classname, "ammo_super"))
		ent->weapongroup = WEAPON_GROUP_SUPER;

	gi.linkentity(ent);
}
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

typedef void (*spawnfunc_t)(edict_t *ent);

typedef struct
{
	char		*name;
	spawnfunc_t	spawn;
} spawn_func_t;

typedef struct
{
	char    *name;
	unsigned ofs;
	fieldtype_t type;
	size_t  len;
} spawn_field_t;

void SP_info_player_start(edict_t *ent);
void SP_info_player_deathmatch(edict_t *ent);
void SP_info_player_coop(edict_t *ent);
void SP_info_player_intermission(edict_t *ent);

void SP_func_plat(edict_t *ent);
void SP_func_rotating(edict_t *ent);
void SP_func_button(edict_t *ent);
void SP_func_door(edict_t *ent);
void SP_func_door_secret(edict_t *ent);
void SP_func_door_rotating(edict_t *ent);
void SP_func_water(edict_t *ent);
void SP_func_train(edict_t *ent);
void SP_func_conveyor(edict_t *self);
void SP_func_wall(edict_t *self);
void SP_func_object(edict_t *self);
void SP_func_explosive(edict_t *self);
void SP_func_timer(edict_t *self);
void SP_func_areaportal(edict_t *ent);
void SP_func_clock(edict_t *ent);
void SP_func_killbox(edict_t *ent);

void SP_trigger_always(edict_t *ent);
void SP_trigger_once(edict_t *ent);
void SP_trigger_multiple(edict_t *ent);
void SP_trigger_relay(edict_t *ent);
void SP_trigger_push(edict_t *ent);
void SP_trigger_hurt(edict_t *ent);
void SP_trigger_key(edict_t *ent);
void SP_trigger_counter(edict_t *ent);
void SP_trigger_elevator(edict_t *ent);
void SP_trigger_gravity(edict_t *ent);
void SP_trigger_monsterjump(edict_t *ent);

void SP_target_temp_entity(edict_t *ent);
void SP_target_speaker(edict_t *ent);
void SP_target_explosion(edict_t *ent);
void SP_target_changelevel(edict_t *ent);
void SP_target_secret(edict_t *ent);
void SP_target_goal(edict_t *ent);
void SP_target_splash(edict_t *ent);
void SP_target_spawner(edict_t *ent);
void SP_target_blaster(edict_t *ent);
void SP_target_crosslevel_trigger(edict_t *ent);
void SP_target_crosslevel_target(edict_t *ent);
void SP_target_laser(edict_t *self);
void SP_target_help(edict_t *ent);
void SP_target_lightramp(edict_t *self);
void SP_target_earthquake(edict_t *ent);
void SP_target_character(edict_t *ent);
void SP_target_string(edict_t *ent);

void SP_worldspawn(edict_t *ent);

void SP_light(edict_t *self);
void SP_light_mine1(edict_t *ent);
void SP_light_mine2(edict_t *ent);
void SP_info_null(edict_t *self);
void SP_info_notnull(edict_t *self);
void SP_path_corner(edict_t *self);
void SP_point_combat(edict_t *self);

void SP_misc_explobox(edict_t *self);
void SP_misc_banner(edict_t *self);
void SP_misc_satellite_dish(edict_t *self);
void SP_misc_gib_arm(edict_t *self);
void SP_misc_gib_leg(edict_t *self);
void SP_misc_gib_head(edict_t *self);
void SP_misc_deadsoldier(edict_t *self);
void SP_misc_viper(edict_t *self);
void SP_misc_viper_bomb(edict_t *self);
void SP_misc_bigviper(edict_t *self);
void SP_misc_strogg_ship(edict_t *self);
void SP_misc_teleporter(edict_t *self);
void SP_misc_teleporter_dest(edict_t *self);
void SP_misc_blackhole(edict_t *self);
void SP_misc_eastertank(edict_t *self);
void SP_misc_easterchick(edict_t *self);
void SP_misc_easterchick2(edict_t *self);

#ifdef ENABLE_COOP
void SP_misc_insane(edict_t *self);

void SP_monster_berserk(edict_t *self);
void SP_monster_gladiator(edict_t *self);
void SP_monster_gunner(edict_t *self);
void SP_monster_infantry(edict_t *self);
void SP_monster_soldier_light(edict_t *self);
void SP_monster_soldier(edict_t *self);
void SP_monster_soldier_ss(edict_t *self);
void SP_monster_tank(edict_t *self);
void SP_monster_medic(edict_t *self);
void SP_monster_flipper(edict_t *self);
void SP_monster_chick(edict_t *self);
void SP_monster_parasite(edict_t *self);
void SP_monster_flyer(edict_t *self);
void SP_monster_brain(edict_t *self);
void SP_monster_floater(edict_t *self);
void SP_monster_hover(edict_t *self);
void SP_monster_mutant(edict_t *self);
void SP_monster_supertank(edict_t *self);
void SP_monster_boss2(edict_t *self);
void SP_monster_jorg(edict_t *self);
void SP_monster_makron(edict_t *self);
void SP_monster_boss3_stand(edict_t *self);

void SP_monster_commander_body(edict_t *self);

void SP_turret_breach(edict_t *self);
void SP_turret_base(edict_t *self);
void SP_turret_driver(edict_t *self);

void q1_monster_army(edict_t *self);
void q1_monster_dog(edict_t *self);
void q1_monster_enforcer(edict_t *self);
void q1_monster_ogre(edict_t *self);
void q1_monster_wizard(edict_t *self);
void q1_monster_zombie(edict_t *self);
void q1_monster_fish(edict_t *self);
void q1_monster_tarbaby(edict_t *self);
void q1_monster_shalrath(edict_t *self);
void q1_monster_knight(edict_t *self);
void q1_monster_demon(edict_t *self);
void q1_monster_shambler(edict_t *self);
void q1_monster_hell_knight(edict_t *self);

// Doom
void doom_monster_poss(edict_t *self);
void doom_monster_troo(edict_t *self);
void doom_monster_sarg(edict_t *self);
void doom_monster_cpos(edict_t *self);
void doom_monster_head(edict_t *self);
void doom_monster_skul(edict_t *self);
void doom_monster_pain(edict_t *self);
void doom_monster_bspi(edict_t *self);
void doom_monster_skel(edict_t *self);
void doom_monster_boss(edict_t *self);
void doom_monster_spid(edict_t *self);
void doom_monster_cybr(edict_t *self);
#endif

typedef struct
{
	const char			*classname;
	const entitytype_e	entityid;

	list_t				hashEntry;
} classname_to_entitytype_t;

static classname_to_entitytype_t classname_to_entitytype[] =
{
	{ "info_player_start",			ET_INFO_PLAYER_START },
	{ "info_player_deathmatch",		ET_INFO_PLAYER_DEATHMATCH },
	{ "info_player_coop",			ET_INFO_PLAYER_COOP },
	{ "info_player_intermission",	ET_INFO_PLAYER_INTERMISSION },

	{ "func_plat",					ET_FUNC_PLAT },
	{ "func_button",				ET_FUNC_BUTTON },
	{ "func_door",					ET_FUNC_DOOR },
	{ "func_door_secret",			ET_FUNC_DOOR_SECRET },
	{ "func_door_rotating",			ET_FUNC_DOOR_ROTATING },
	{ "func_rotating",				ET_FUNC_ROTATING },
	{ "func_train",					ET_FUNC_TRAIN },
	{ "func_water",					ET_FUNC_WATER },
	{ "func_conveyor",				ET_FUNC_CONVEYOR },
	{ "func_areaportal",			ET_FUNC_AREAPORTAL },
	{ "func_clock",					ET_FUNC_CLOCK },
	{ "func_wall",					ET_FUNC_WALL },
	{ "func_object",				ET_FUNC_OBJECT },
	{ "func_timer",					ET_FUNC_TIMER },
	{ "func_explosive",				ET_FUNC_EXPLOSIVE },
	{ "func_killbox",				ET_FUNC_KILLBOX },

	{ "trigger_always",				ET_TRIGGER_ALWAYS },
	{ "trigger_once",				ET_TRIGGER_ONCE },
	{ "trigger_multiple",			ET_TRIGGER_MULTIPLE },
	{ "trigger_relay",				ET_TRIGGER_RELAY},
	{ "trigger_push",				ET_TRIGGER_PUSH },
	{ "trigger_hurt",				ET_TRIGGER_HURT },
	{ "trigger_key",				ET_TRIGGER_KEY },
	{ "trigger_counter",			ET_TRIGGER_COUNTER },
	{ "trigger_elevator",			ET_TRIGGER_ELEVATOR },
	{ "trigger_gravity",			ET_TRIGGER_GRAVITY },
	{ "trigger_monsterjump",		ET_TRIGGER_MONSTERJUMP },

	{ "target_temp_entity",			ET_TARGET_TEMP_ENTITY },
	{ "target_speaker",				ET_TARGET_SPEAKER },
	{ "target_explosion",			ET_TARGET_EXPLOSION },
	{ "target_changelevel",			ET_TARGET_CHANGELEVEL },
	{ "target_secret",				ET_TARGET_SECRET },
	{ "target_goal",				ET_TARGET_GOAL },
	{ "target_splash",				ET_TARGET_SPLASH },
	{ "target_spawner",				ET_TARGET_SPAWNER },
	{ "target_blaster",				ET_TARGET_BLASTER },
	{ "target_crosslevel_trigger",	ET_TARGET_CROSSLEVEL_TRIGGER },
	{ "target_crosslevel_target",	ET_TARGET_CROSSLEVEL_TARGET },
	{ "target_laser",				ET_TARGET_LASER },
	{ "target_help",				ET_TARGET_HELP },
	{ "target_lightramp",			ET_TARGET_LIGHTRAMP },
	{ "target_earthquake",			ET_TARGET_EARTHQUAKE },
	{ "target_character",			ET_TARGET_CHARACTER },
	{ "target_string",				ET_TARGET_STRING },

	{ "worldspawn",					ET_WORLDSPAWN },

	{ "light",						ET_LIGHT },
	{ "light_mine1",				ET_LIGHT_MINE1 },
	{ "light_mine2",				ET_LIGHT_MINE2 },
	{ "info_null",					ET_INFO_NULL },
	{ "func_group",					ET_FUNC_GROUP },
	{ "info_notnull",				ET_INFO_NOTNULL },
	{ "path_corner",				ET_PATH_CORNER },
	{ "point_combat",				ET_POINT_COMBAT },

	{ "misc_explobox",				ET_MISC_EXPLOBOX },
	{ "misc_banner",				ET_MISC_BANNER },
	{ "misc_satellite_dish",		ET_MISC_SATELLITE_DISH },
	{ "misc_gib_arm",				ET_MISC_GIB_ARM },
	{ "misc_gib_leg",				ET_MISC_GIB_LEG },
	{ "misc_gib_head",				ET_MISC_GIB_HEAD },
	{ "misc_deadsoldier",			ET_MISC_DEADSOLDIER },
	{ "misc_viper",					ET_MISC_VIPER },
	{ "misc_viper_bomb",			ET_MISC_VIPER_BOMB },
	{ "misc_bigviper",				ET_MISC_BIGVIPER },
	{ "misc_strogg_ship",			ET_MISC_STROGG_SHIP },
	{ "misc_teleporter"	,			ET_MISC_TELEPORTER },
	{ "misc_teleporter_dest",		ET_MISC_TELEPORTER_DEST },
	{ "misc_blackhole",				ET_MISC_BLACKHOLE },
	{ "misc_eastertank",			ET_MISC_EASTERTANK },
	{ "misc_easterchick",			ET_MISC_EASTERCHICK },
	{ "misc_easterchick2",			ET_MISC_EASTERCHICK2 },
	
#ifdef ENABLE_COOP
	{ "misc_insane",				ET_MISC_INSANE },

	{ "monster_berserk",			ET_MONSTER_BERSERK },
	{ "monster_gladiator",			ET_MONSTER_GLADIATOR },
	{ "monster_gunner",				ET_MONSTER_GUNNER },
	{ "monster_infantry",			ET_MONSTER_INFANTRY },
	{ "monster_soldier_light",		ET_MONSTER_SOLDIER_LIGHT },
	{ "monster_soldier",			ET_MONSTER_SOLDIER },
	{ "monster_soldier_ss",			ET_MONSTER_SOLDIER_SS },
	{ "monster_tank",				ET_MONSTER_TANK },
	{ "monster_tank_commander",		ET_MONSTER_TANK_COMMANDER },
	{ "monster_medic",				ET_MONSTER_MEDIC },
	{ "monster_flipper",			ET_MONSTER_FLIPPER },
	{ "monster_chick",				ET_MONSTER_CHICK },
	{ "monster_parasite",			ET_MONSTER_PARASITE },
	{ "monster_flyer",				ET_MONSTER_FLYER },
	{ "monster_brain",				ET_MONSTER_BRAIN },
	{ "monster_floater",			ET_MONSTER_FLOATER },
	{ "monster_hover",				ET_MONSTER_HOVER },
	{ "monster_mutant",				ET_MONSTER_MUTANT },
	{ "monster_supertank",			ET_MONSTER_SUPERTANK },
	{ "monster_boss2",				ET_MONSTER_BOSS2 },
	{ "monster_boss3_stand",		ET_MONSTER_BOSS3_STAND },
	{ "monster_jorg",				ET_MONSTER_JORG },
	{ "monster_makron",				ET_MONSTER_MAKRON },

	{ "monster_commander_body",		ET_MONSTER_COMMANDER_BODY },

	{ "turret_breach",				ET_TURRET_BREACH },
	{ "turret_base",				ET_TURRET_BASE },
	{ "turret_driver",				ET_TURRET_DRIVER },

	// Q1
	{ "monster_army",				ET_Q1_MONSTER_ARMY },
	{ "monster_dog",				ET_Q1_MONSTER_DOG },
	{ "monster_ogre",				ET_Q1_MONSTER_OGRE },
	{ "monster_ogre_marksman",		ET_Q1_MONSTER_OGRE_MARKSMAN },
	{ "monster_knight",				ET_Q1_MONSTER_KNIGHT },
	{ "monster_zombie",				ET_Q1_MONSTER_ZOMBIE },
	{ "monster_wizard",				ET_Q1_MONSTER_WIZARD },
	{ "monster_demon1",				ET_Q1_MONSTER_DEMON },
	{ "monster_shambler",			ET_Q1_MONSTER_SHAMBLER },
	{ "monster_enforcer",			ET_Q1_MONSTER_ENFORCER },
	{ "monster_hell_knight",		ET_Q1_MONSTER_HELL_KNIGHT },
	{ "monster_shalrath",			ET_Q1_MONSTER_SHALRATH },
	{ "monster_tarbaby",			ET_Q1_MONSTER_TARBABY },
	{ "monster_fish",				ET_Q1_MONSTER_FISH },

	// Doom
	{ "monster_poss",				ET_DOOM_MONSTER_POSS },
	{ "monster_spos",				ET_DOOM_MONSTER_SPOS },
	{ "monster_troo",				ET_DOOM_MONSTER_TROO },
	{ "monster_sarg",				ET_DOOM_MONSTER_SARG },
	{ "monster_cpos",				ET_DOOM_MONSTER_CPOS },
	{ "monster_head",				ET_DOOM_MONSTER_HEAD },
	{ "monster_skul",				ET_DOOM_MONSTER_SKUL },
	{ "monster_pain",				ET_DOOM_MONSTER_PAIN },
	{ "monster_spectre",			ET_DOOM_MONSTER_SPECTRE },
	{ "monster_bspi",				ET_DOOM_MONSTER_BSPI },
	{ "monster_skel",				ET_DOOM_MONSTER_SKEL },
	{ "monster_baron",				ET_DOOM_MONSTER_BOSS },
	{ "monster_bos2",				ET_DOOM_MONSTER_BOS2 },
	{ "monster_spid",				ET_DOOM_MONSTER_SPID },
	{ "monster_cybr",				ET_DOOM_MONSTER_CYBR }
#endif
};

/*
================
Com_HashString
================
*/
unsigned Com_MyHashString(const char *s, unsigned size)
{
	unsigned hash, c;
	hash = 0;

	while (*s)
	{
		c = *s++;
		hash = c + (hash << 6) + (hash << 16) - hash;
	}

	return hash & (size - 1);
}

static const	size_t ENTITY_NUM = q_countof(classname_to_entitytype);
#define CLASSHASH_SIZE 256
static list_t   classname_to_entitytype_hash[CLASSHASH_SIZE];

#define ITEMHASH_SIZE 128
static list_t   gitem_hash[ITEMHASH_SIZE];

#define HASH_FUNC Com_MyHashString

static void init_classname_to_entitytype_hashes()
{
	size_t i;

	for (i = 0; i < CLASSHASH_SIZE; i++)
		List_Init(&classname_to_entitytype_hash[i]);

	for (i = 0; i < ITEMHASH_SIZE; i++)
		List_Init(&gitem_hash[i]);

	for (i = 0; i < ENTITY_NUM; ++i)
	{
		classname_to_entitytype_t *cte = &classname_to_entitytype[i];
		uint32_t hash = HASH_FUNC(cte->classname, CLASSHASH_SIZE);
		List_Append(&classname_to_entitytype_hash[hash], &cte->hashEntry);
	}

	itemid_e it;

	for (it = ITI_NULL; it < ITI_TOTAL; ++it)
	{
		gitem_t *item = &itemlist[it];

		if (!item->classname)
			continue;

		uint32_t hash = HASH_FUNC(item->classname, ITEMHASH_SIZE);
		List_Append(&gitem_hash[hash], &item->hashEntry);
	}
}

static uint32_t cache_hits = 0, cache_misses = 0;
static uint32_t num_strcmps = 0, num_old_strcmps = 0;

static entitytype_e get_entitytype_from_classname(const char *classname)
{
	uint32_t hash = HASH_FUNC(classname, CLASSHASH_SIZE);
	classname_to_entitytype_t *entry;
	LIST_FOR_EACH(classname_to_entitytype_t, entry, &classname_to_entitytype_hash[hash], hashEntry)
	{
		++num_strcmps;

		if (!strcmp(entry->classname, classname))
		{
			cache_hits++;
			num_old_strcmps += (entry - classname_to_entitytype);
			return entry->entityid;
		}

		cache_misses++;
	}
	num_old_strcmps += ENTITY_NUM;
	return ET_NULL;
}

static itemid_e get_item_from_classname(const char *classname)
{
	uint32_t hash = HASH_FUNC(classname, ITEMHASH_SIZE);
	gitem_t *entry;
	LIST_FOR_EACH(gitem_t, entry, &gitem_hash[hash], hashEntry)
	{
		++num_strcmps;

		if (!strcmp(entry->classname, classname))
		{
			cache_hits++;
			num_old_strcmps += (entry - itemlist);
			return GetIndexByItem(entry);
		}

		cache_misses++;
	}
	num_old_strcmps += ITI_TOTAL;
	return ITI_NULL;
}

typedef struct
{
	entitytype_e	entityid;
	spawnfunc_t		func;
} entitytype_func_t;

static const entitytype_func_t q2_entitytype_funcs[] =
{
	// Q2
	{ ET_INFO_PLAYER_START, SP_info_player_start },
	{ ET_INFO_PLAYER_DEATHMATCH, SP_info_player_deathmatch },
	{ ET_INFO_PLAYER_COOP, SP_info_player_coop },
	{ ET_INFO_PLAYER_INTERMISSION, SP_info_player_intermission },

	{ ET_FUNC_PLAT, SP_func_plat },
	{ ET_FUNC_BUTTON, SP_func_button },
	{ ET_FUNC_DOOR, SP_func_door },
	{ ET_FUNC_DOOR_SECRET, SP_func_door_secret },
	{ ET_FUNC_DOOR_ROTATING, SP_func_door_rotating },
	{ ET_FUNC_ROTATING, SP_func_rotating },
	{ ET_FUNC_TRAIN, SP_func_train },
	{ ET_FUNC_WATER, SP_func_water },
	{ ET_FUNC_CONVEYOR, SP_func_conveyor },
	{ ET_FUNC_AREAPORTAL, SP_func_areaportal },
	{ ET_FUNC_CLOCK, SP_func_clock },
	{ ET_FUNC_WALL, SP_func_wall },
	{ ET_FUNC_OBJECT, SP_func_object },
	{ ET_FUNC_TIMER, SP_func_timer },
	{ ET_FUNC_EXPLOSIVE, SP_func_explosive },
	{ ET_FUNC_KILLBOX, SP_func_killbox },

	{ ET_TRIGGER_ALWAYS, SP_trigger_always },
	{ ET_TRIGGER_ONCE, SP_trigger_once },
	{ ET_TRIGGER_MULTIPLE, SP_trigger_multiple },
	{ ET_TRIGGER_RELAY, SP_trigger_relay },
	{ ET_TRIGGER_PUSH, SP_trigger_push },
	{ ET_TRIGGER_HURT, SP_trigger_hurt },
	{ ET_TRIGGER_KEY, SP_trigger_key },
	{ ET_TRIGGER_COUNTER, SP_trigger_counter },
	{ ET_TRIGGER_ELEVATOR, SP_trigger_elevator },
	{ ET_TRIGGER_GRAVITY, SP_trigger_gravity },
	{ ET_TRIGGER_MONSTERJUMP, SP_trigger_monsterjump },

	{ ET_TARGET_TEMP_ENTITY, SP_target_temp_entity },
	{ ET_TARGET_SPEAKER, SP_target_speaker },
	{ ET_TARGET_EXPLOSION, SP_target_explosion },
	{ ET_TARGET_CHANGELEVEL, SP_target_changelevel },
	{ ET_TARGET_SECRET, SP_target_secret },
	{ ET_TARGET_GOAL, SP_target_goal },
	{ ET_TARGET_SPLASH, SP_target_splash },
	{ ET_TARGET_SPAWNER, SP_target_spawner },
	{ ET_TARGET_BLASTER, SP_target_blaster },
	{ ET_TARGET_CROSSLEVEL_TRIGGER, SP_target_crosslevel_trigger },
	{ ET_TARGET_CROSSLEVEL_TARGET, SP_target_crosslevel_target },
	{ ET_TARGET_LASER, SP_target_laser },
	{ ET_TARGET_HELP, SP_target_help },
	{ ET_TARGET_LIGHTRAMP, SP_target_lightramp },
	{ ET_TARGET_EARTHQUAKE, SP_target_earthquake },
	{ ET_TARGET_CHARACTER, SP_target_character },
	{ ET_TARGET_STRING, SP_target_string },

	{ ET_WORLDSPAWN, SP_worldspawn },

	{ ET_LIGHT, SP_light },
	{ ET_LIGHT_MINE1, SP_light_mine1 },
	{ ET_LIGHT_MINE2, SP_light_mine2 },
	{ ET_INFO_NULL, SP_info_null },
	{ ET_FUNC_GROUP, SP_info_null },
	{ ET_INFO_NOTNULL, SP_info_notnull },
	{ ET_PATH_CORNER, SP_path_corner },
	{ ET_POINT_COMBAT, SP_point_combat },

	{ ET_MISC_EXPLOBOX, SP_misc_explobox },
	{ ET_MISC_BANNER, SP_misc_banner },
	{ ET_MISC_SATELLITE_DISH, SP_misc_satellite_dish },
	{ ET_MISC_GIB_ARM, SP_misc_gib_arm },
	{ ET_MISC_GIB_LEG, SP_misc_gib_leg },
	{ ET_MISC_GIB_HEAD, SP_misc_gib_head },
	{ ET_MISC_DEADSOLDIER, SP_misc_deadsoldier },
	{ ET_MISC_VIPER, SP_misc_viper },
	{ ET_MISC_VIPER_BOMB, SP_misc_viper_bomb },
	{ ET_MISC_BIGVIPER, SP_misc_bigviper },
	{ ET_MISC_STROGG_SHIP, SP_misc_strogg_ship },
	{ ET_MISC_TELEPORTER, SP_misc_teleporter },
	{ ET_MISC_TELEPORTER_DEST, SP_misc_teleporter_dest },
	{ ET_MISC_BLACKHOLE, SP_misc_blackhole },
	{ ET_MISC_EASTERTANK, SP_misc_eastertank },
	{ ET_MISC_EASTERCHICK, SP_misc_easterchick },
	{ ET_MISC_EASTERCHICK2, SP_misc_easterchick2 },
	
#ifdef ENABLE_COOP
	{ ET_MISC_INSANE, SP_misc_insane },

	{ ET_MONSTER_BERSERK, SP_monster_berserk },
	{ ET_MONSTER_GLADIATOR, SP_monster_gladiator },
	{ ET_MONSTER_GUNNER, SP_monster_gunner },
	{ ET_MONSTER_INFANTRY, SP_monster_infantry },
	{ ET_MONSTER_SOLDIER_LIGHT, SP_monster_soldier_light },
	{ ET_MONSTER_SOLDIER, SP_monster_soldier },
	{ ET_MONSTER_SOLDIER_SS, SP_monster_soldier_ss },
	{ ET_MONSTER_TANK, SP_monster_tank },
	{ ET_MONSTER_TANK_COMMANDER, SP_monster_tank },
	{ ET_MONSTER_MEDIC, SP_monster_medic },
	{ ET_MONSTER_FLIPPER, SP_monster_flipper },
	{ ET_MONSTER_CHICK, SP_monster_chick },
	{ ET_MONSTER_PARASITE, SP_monster_parasite },
	{ ET_MONSTER_FLYER, SP_monster_flyer },
	{ ET_MONSTER_BRAIN, SP_monster_brain },
	{ ET_MONSTER_FLOATER, SP_monster_floater },
	{ ET_MONSTER_HOVER, SP_monster_hover },
	{ ET_MONSTER_MUTANT, SP_monster_mutant },
	{ ET_MONSTER_SUPERTANK, SP_monster_supertank },
	{ ET_MONSTER_BOSS2, SP_monster_boss2 },
	{ ET_MONSTER_BOSS3_STAND, SP_monster_boss3_stand },
	{ ET_MONSTER_JORG, SP_monster_jorg },
	{ ET_MONSTER_MAKRON, SP_monster_makron },

	{ ET_MONSTER_COMMANDER_BODY, SP_monster_commander_body },

	{ ET_TURRET_BREACH, SP_turret_breach },
	{ ET_TURRET_BASE, SP_turret_base },
	{ ET_TURRET_DRIVER, SP_turret_driver }
#endif
};

#ifdef ENABLE_COOP
static const entitytype_func_t q1_entitytype_funcs[] =
{
	{ ET_Q1_MONSTER_ARMY, q1_monster_army },
	{ ET_Q1_MONSTER_DOG, q1_monster_dog },
	{ ET_Q1_MONSTER_OGRE, q1_monster_ogre },
	{ ET_Q1_MONSTER_OGRE_MARKSMAN, q1_monster_ogre },
	{ ET_Q1_MONSTER_KNIGHT, q1_monster_knight },
	{ ET_Q1_MONSTER_ZOMBIE, q1_monster_zombie },
	{ ET_Q1_MONSTER_WIZARD, q1_monster_wizard },
	{ ET_Q1_MONSTER_DEMON, q1_monster_demon },
	{ ET_Q1_MONSTER_SHAMBLER, q1_monster_shambler },
	{ ET_Q1_MONSTER_ENFORCER, q1_monster_enforcer },
	{ ET_Q1_MONSTER_HELL_KNIGHT, q1_monster_hell_knight },
	{ ET_Q1_MONSTER_SHALRATH, q1_monster_shalrath },
	{ ET_Q1_MONSTER_TARBABY, q1_monster_tarbaby },
	{ ET_Q1_MONSTER_FISH, q1_monster_fish }
};

static const entitytype_func_t doom_entitytype_funcs[] =
{
	{ ET_DOOM_MONSTER_POSS, doom_monster_poss },
	{ ET_DOOM_MONSTER_SPOS, doom_monster_poss },
	{ ET_DOOM_MONSTER_TROO, doom_monster_troo },
	{ ET_DOOM_MONSTER_SARG, doom_monster_sarg },
	{ ET_DOOM_MONSTER_CPOS, doom_monster_cpos },
	{ ET_DOOM_MONSTER_HEAD, doom_monster_head },
	{ ET_DOOM_MONSTER_SKUL, doom_monster_skul },
	{ ET_DOOM_MONSTER_PAIN, doom_monster_pain },
	{ ET_DOOM_MONSTER_SPECTRE, doom_monster_sarg },
	{ ET_DOOM_MONSTER_BSPI, doom_monster_bspi },
	{ ET_DOOM_MONSTER_SKEL, doom_monster_skel },
	{ ET_DOOM_MONSTER_BOSS, doom_monster_boss },
	{ ET_DOOM_MONSTER_BOS2, doom_monster_boss },
	{ ET_DOOM_MONSTER_SPID, doom_monster_spid },
	{ ET_DOOM_MONSTER_CYBR, doom_monster_cybr }
};
#endif

typedef struct
{
	spawnfunc_t		funcs[ENTITYTYPE_SPAWNABLE_COUNT];
} game_spawn_func_list_t;

static game_spawn_func_list_t game_spawn_func_list[GAME_TOTAL];

static void init_game_spawn_func_list(const entitytype_func_t *funcs, const size_t funccount, game_spawn_func_list_t *list)
{
	size_t i;

	for (i = 0; i < funccount; ++i)
		list->funcs[funcs[i].entityid] = funcs[i].func;
}

static void init_game_spawn_func_lists()
{
	init_game_spawn_func_list(q2_entitytype_funcs, q_countof(q2_entitytype_funcs), &game_spawn_func_list[GAME_Q2]);
#ifdef ENABLE_COOP
	init_game_spawn_func_list(q1_entitytype_funcs, q_countof(q1_entitytype_funcs), &game_spawn_func_list[GAME_Q1]);
	init_game_spawn_func_list(doom_entitytype_funcs, q_countof(doom_entitytype_funcs), &game_spawn_func_list[GAME_DOOM]);
#endif
}

static spawnfunc_t get_func_from_entitytype(entitytype_e id, gametype_t *game_ptr)
{
	// go through all types
	gametype_t gametype;

	for (gametype = GAME_Q2; gametype < GAME_TOTAL; ++gametype)
	{
		spawnfunc_t func = game_spawn_func_list[gametype].funcs[id];

		if (func)
		{
			*game_ptr = gametype;
			return func;
		}
	}

	return NULL;
}

static const spawn_field_t spawn_fields[] =
{
	{"model", FOFS(model), F_LSTRING},
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"accel", FOFS(accel), F_FLOAT},
	{"decel", FOFS(decel), F_FLOAT},
	{"target", FOFS(target), F_LSTRING},
	{"targetname", FOFS(targetname), F_LSTRING},
	{"pathtarget", FOFS(pathtarget), F_LSTRING},
	{"deathtarget", FOFS(deathtarget), F_LSTRING},
	{"killtarget", FOFS(killtarget), F_LSTRING},
	{"combattarget", FOFS(combattarget), F_LSTRING},
	{"message", FOFS(message), F_LSTRING},
	{"team", FOFS(team), F_LSTRING},
	{"wait", FOFS(wait), F_FLOAT},
	{"delay", FOFS(delay), F_FLOAT},
	{"random", FOFS(random), F_FLOAT},
	{"move_origin", FOFS(move_origin), F_VECTOR},
	{"move_angles", FOFS(move_angles), F_VECTOR},
	{"style", FOFS(style), F_INT},
	{"count", FOFS(count), F_INT},
	{"health", FOFS(health), F_INT},
	{"sounds", FOFS(sounds), F_INT},
	{"light", 0, F_IGNORE},
	{"dmg", FOFS(dmg), F_INT},
	{"mass", FOFS(mass), F_INT},
	{"volume", FOFS(volume), F_FLOAT},
	{"attenuation", FOFS(attenuation), F_FLOAT},
	{"map", FOFS(map), F_LSTRING},
	{"origin", FOFS(s.origin), F_VECTOR},
	{"angles", FOFS(s.angles), F_VECTOR},
	{ "angle", FOFS(s.angles), F_ANGLEHACK },
	{ "mangle", FOFS(s.angles), F_VECTOR },

	{NULL}
};

// temp spawn vars -- only valid when the spawn function is called
static const spawn_field_t temp_fields[] =
{
	{"lip", STOFS(lip), F_INT},
	{"distance", STOFS(distance), F_INT},
	{"height", STOFS(height), F_INT},
	{"noise", STOFS(noise), F_LSTRING},
	{"pausetime", STOFS(pausetime), F_FLOAT},
	{"item", STOFS(item), F_LSTRING},

	{"gravity", STOFS(gravity), F_LSTRING},
	{"sky", STOFS(sky), F_LSTRING},
	{"skyrotate", STOFS(skyrotate), F_FLOAT},
	{"skyaxis", STOFS(skyaxis), F_VECTOR},
	{"minyaw", STOFS(minyaw), F_FLOAT},
	{"maxyaw", STOFS(maxyaw), F_FLOAT},
	{"minpitch", STOFS(minpitch), F_FLOAT},
	{"maxpitch", STOFS(maxpitch), F_FLOAT},
	{"nextmap", STOFS(nextmap), F_LSTRING},

	{ "nextthink", STOFS(nextthink), F_FLOAT },
	{ "classname", STOFS(classname), F_ZSTRING, MAX_TOKEN_CHARS },

	{NULL}
};


/*
===============
ED_CallSpawn

Finds the spawn function for the entity and calls it
===============
*/

#ifdef ENABLE_COOP
static bool randomize_enemies = false;

typedef struct
{
	entitytype_e	root;
	entitytype_e	replacements[8];
} enemyrandomize_t;

static enemyrandomize_t enemy_randomizations[] =
{
	{
		ET_MONSTER_SOLDIER_LIGHT,
		{
			ET_MONSTER_SOLDIER_LIGHT,
			ET_Q1_MONSTER_ARMY,
			ET_Q1_MONSTER_DOG,
			ET_DOOM_MONSTER_POSS
		}
	},
	{
		ET_MONSTER_SOLDIER,
		{
			ET_MONSTER_SOLDIER,
			ET_Q1_MONSTER_ARMY,
			ET_Q1_MONSTER_DOG,
			ET_DOOM_MONSTER_SPOS
		}
	},
	{
		ET_MONSTER_SOLDIER_SS,
		{
			ET_MONSTER_SOLDIER_SS,
			ET_Q1_MONSTER_ARMY,
			ET_Q1_MONSTER_DOG,
			ET_DOOM_MONSTER_POSS,
			ET_DOOM_MONSTER_SPOS,
			ET_DOOM_MONSTER_TROO
		}
	},
	{
		ET_MONSTER_INFANTRY,
		{
			ET_MONSTER_INFANTRY,
			ET_Q1_MONSTER_ENFORCER,
			ET_Q1_MONSTER_OGRE,
			ET_DOOM_MONSTER_CPOS,
			ET_DOOM_MONSTER_TROO
		}
	},
	{
		ET_MONSTER_FLYER,
		{
			ET_MONSTER_FLYER,
			ET_Q1_MONSTER_WIZARD,
			ET_DOOM_MONSTER_SKUL
		}
	},
	{
		ET_MONSTER_BERSERK,
		{
			ET_MONSTER_BERSERK,
			ET_Q1_MONSTER_KNIGHT,
			ET_DOOM_MONSTER_SARG,
			ET_DOOM_MONSTER_SPECTRE,
			ET_DOOM_MONSTER_SKEL
		}
	},
	{
		ET_MONSTER_HOVER,
		{
			ET_MONSTER_HOVER,
			ET_Q1_MONSTER_WIZARD,
			ET_DOOM_MONSTER_HEAD,
			ET_DOOM_MONSTER_PAIN
		}
	},
	{
		ET_MONSTER_FLOATER,
		{
			ET_MONSTER_FLOATER,
			ET_DOOM_MONSTER_HEAD,
			ET_DOOM_MONSTER_PAIN
		}
	},
	{
		ET_MONSTER_GUNNER,
		{
			ET_MONSTER_GUNNER,
			ET_Q1_MONSTER_HELL_KNIGHT,
			ET_Q1_MONSTER_OGRE,
			ET_DOOM_MONSTER_SARG,
			ET_DOOM_MONSTER_SPECTRE,
			ET_DOOM_MONSTER_SKEL,
			ET_DOOM_MONSTER_BSPI
		}
	},
	{
		ET_MONSTER_PARASITE,
		{
			ET_MONSTER_PARASITE,
			ET_Q1_MONSTER_DEMON,
			ET_DOOM_MONSTER_SARG,
			ET_DOOM_MONSTER_SPECTRE
		}
	}
};

static entitytype_e randomize_monster(entitytype_e monster)
{
	size_t i;

	for (i = 0; i < q_countof(enemy_randomizations); ++i)
	{
		enemyrandomize_t *randomize = &enemy_randomizations[i];

		if (randomize->root == monster)
		{
			int x = 0;

			for (; randomize->replacements[x]; ++x);

			return randomize->replacements[Q_rand() % x];
		}
	}

	return monster;
}
#endif

bool ED_CallSpawnByType(edict_t *ent, entitytype_e type)
{
	spawnfunc_t func = get_func_from_entitytype(type, &ent->s.game);

	if (func != NULL)
	{
		ent->entitytype = type;
		func(ent);
		return true;
	}

	return false;
}

void SP_weapon(edict_t *ent);
void SP_ammo(edict_t *ent);

void ED_CallSpawn(edict_t *ent)
{
	if (!spawnTemp.classname[0])
	{
		Com_Printf("ED_CallSpawn: NULL classname\n");
		return;
	}

	itemid_e item = get_item_from_classname(spawnTemp.classname);

	if (item)
	{
		SpawnItem(ent, GetItemByIndex(item));
		return;
	}

	// regular funcs
	entitytype_e type = get_entitytype_from_classname(spawnTemp.classname);

	if (type != ET_NULL)
	{
#ifdef ENABLE_COOP
		if (randomize_enemies && type >= ET_MONSTER_BERSERK && type <= ET_MONSTER_JORG)
			type = randomize_monster(type);
#endif

		if (ED_CallSpawnByType(ent, type))
			return;
	}

	if (!Q_strncasecmp(spawnTemp.classname, "weapon_", 7))
	{
		SP_weapon(ent);
		return;
	}
	else if (!Q_strncasecmp(spawnTemp.classname, "ammo_", 5))
	{
		SP_ammo(ent);
		return;
	}

	Com_Printf("%s doesn't have a spawn function\n", spawnTemp.classname);
}

/*
=============
ED_NewString
=============
*/
static char *ED_NewString(const char *string)
{
	char    *newb, *new_p;
	int     i, l;
	l = strlen(string) + 1;
	newb = Z_TagMalloc(l, TAG_LEVEL);
	new_p = newb;

	for (i = 0 ; i < l ; i++)
	{
		if (string[i] == '\\' && i < l - 1)
		{
			i++;

			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}

	return newb;
}




/*
===============
ED_ParseField

Takes a key/value pair and sets the binary values
in an edict
===============
*/
static bool ED_ParseField(const spawn_field_t *fields, const char *key, const char *value, byte *b)
{
	const spawn_field_t *f;
	float   v;
	vec3_t  vec;

	for (f = fields ; f->name ; f++)
	{
		if (!Q_stricmp(f->name, key))
		{
			// found it
			switch (f->type)
			{
				case F_LSTRING:
					*(char **)(b + f->ofs) = ED_NewString(value);
					break;

				case F_VECTOR:
					if (sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]) != 3)
					{
						Com_Printf("%s: couldn't parse '%s'\n", __func__, key);
						VectorClear(vec);
					}

					((float *)(b + f->ofs))[0] = vec[0];
					((float *)(b + f->ofs))[1] = vec[1];
					((float *)(b + f->ofs))[2] = vec[2];
					break;

				case F_INT:
					*(int *)(b + f->ofs) = atoi(value);
					break;

				case F_FLOAT:
					*(float *)(b + f->ofs) = atof(value);
					break;

				case F_ANGLEHACK:
					v = atof(value);
					((float *)(b + f->ofs))[0] = 0;
					((float *)(b + f->ofs))[1] = v;
					((float *)(b + f->ofs))[2] = 0;
					break;

				case F_IGNORE:
					break;

				case F_ZSTRING:
					if (strlen(value) >= f->len)
						Com_Error(ERR_FATAL, "%s: key '%s' out of bounds", __func__, key);

					{
						char *ptr = (char *)(b + f->ofs);
						Q_snprintf(ptr, f->len, "%s", value);
					}
					break;

				default:
					break;
			}

			return true;
		}
	}

	return false;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
====================
*/
void ED_ParseEdict(const char **data, edict_t *ent)
{
	bool        init;
	char        *key, *value;
	init = false;
	memset(&spawnTemp, 0, sizeof(spawnTemp));

	// go through all the dictionary pairs
	while (1)
	{
		// parse key
		key = COM_Parse(data);

		if (key[0] == '}')
			break;

		if (!*data)
			Com_Error(ERR_FATAL, "%s: EOF without closing brace", __func__);

		// parse value
		value = COM_Parse(data);

		if (!*data)
			Com_Error(ERR_FATAL, "%s: EOF without closing brace", __func__);

		if (value[0] == '}')
			Com_Error(ERR_FATAL, "%s: closing brace without data", __func__);

		init = true;

		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by quake
		if (key[0] == '_')
			continue;

		if (!ED_ParseField(spawn_fields, key, value, (byte *)ent))
		{
			if (!ED_ParseField(temp_fields, key, value, (byte *)&spawnTemp))
				Com_Printf("%s: %s is not a field\n", __func__, key);
		}
	}

	if (!init)
		memset(ent, 0, sizeof(*ent));
}


/*
================
G_FindTeams

Chain together all entities with a matching team field.

All but the first will have the FL_TEAMSLAVE flag set.
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams(void)
{
	edict_t *e, *e2, *chain;
	int     i, j;
	int     c, c2;
	c = 0;
	c2 = 0;

	for (i = 1, e = g_edicts + i ; i < globals.num_edicts ; i++, e++)
	{
		if (!e->inuse)
			continue;

		if (!e->team)
			continue;

		if (e->flags & FL_TEAMSLAVE)
			continue;

		chain = e;
		e->teammaster = e;
		c++;
		c2++;

		for (j = i + 1, e2 = e + 1 ; j < globals.num_edicts ; j++, e2++)
		{
			if (!e2->inuse)
				continue;

			if (!e2->team)
				continue;

			if (e2->flags & FL_TEAMSLAVE)
				continue;

			if (!strcmp(e->team, e2->team))
			{
				c2++;
				chain->teamchain = e2;
				e2->teammaster = e;
				chain = e2;
				e2->flags |= FL_TEAMSLAVE;
			}
		}
	}

	Com_Printf("%i teams with %i entities\n", c, c2);
}

void Spawn_Weapons();

/*
==============
SpawnEntities

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
==============
*/
void SpawnEntities(const char *mapname, const char *entities, const char *spawnpoint)
{
	Q_srand(game.random_seed);
	init_classname_to_entitytype_hashes();
	init_game_spawn_func_lists();
	edict_t     *ent;
	int         inhibit;
	char        *com_token;
	int         i;

#ifdef ENABLE_COOP
	randomize_enemies = true;
	float       skill_level = floorf(skill->value);

	if (skill_level < 0)
		skill_level = 0;

	if (skill_level > 3)
		skill_level = 3;

	if (skill->value != skill_level)
		Cvar_Set("skill", va("%f", skill_level));
#endif

	SaveClientData();
	Z_FreeTags(TAG_LEVEL);
	memset(&level, 0, sizeof(level));
	memset(g_edicts, 0, game.maxentities * sizeof(g_edicts[0]));
	Q_strlcpy(level.mapname, mapname, sizeof(level.mapname));
#ifdef ENABLE_COOP
	Q_strlcpy(game.spawnpoint, spawnpoint, sizeof(game.spawnpoint));
#endif

	// set client fields on player ents
	for (i = 0 ; i < game.maxclients ; i++)
		g_edicts[i + 1].client = game.clients + i;

	ent = NULL;
	inhibit = 0;

	if (!level.spawn_rand)
		level.spawn_rand = time(NULL);

	// parse ents
	while (1)
	{
		// parse the opening brace
		com_token = COM_Parse(&entities);

		if (!entities)
			break;

		if (com_token[0] != '{')
			Com_Error(ERR_FATAL, "ED_LoadFromFile: found %s when expecting {", com_token);

		if (!ent)
			ent = g_edicts;
		else
			ent = G_Spawn();

		ED_ParseEdict(&entities, ent);

		// yet another map hack
		if (!Q_stricmp(level.mapname, "command") && ent->entitytype == ET_TRIGGER_ONCE && !Q_stricmp(ent->model, "*27"))
			ent->spawnflags &= ~SPAWNFLAG_NOT_HARD;

		// remove things (except the world) from different skill levels or deathmatch
		if (ent != g_edicts)
		{
#ifdef ENABLE_COOP
			if (deathmatch->value)
#endif
			{
				if (ent->spawnflags & SPAWNFLAG_NOT_DEATHMATCH)
				{
					G_FreeEdict(ent);
					inhibit++;
					continue;
				}
			}
#ifdef ENABLE_COOP
			else
			{
				if ( /* ((coop->value) && (ent->spawnflags & SPAWNFLAG_NOT_COOP)) || */
					((skill->value == 0) && (ent->spawnflags & SPAWNFLAG_NOT_EASY)) ||
					((skill->value == 1) && (ent->spawnflags & SPAWNFLAG_NOT_MEDIUM)) ||
					(((skill->value == 2) || (skill->value == 3)) && (ent->spawnflags & SPAWNFLAG_NOT_HARD))
				)
				{
					G_FreeEdict(ent);
					inhibit++;
					continue;
				}
			}
#endif

			ent->spawnflags &= ~(SPAWNFLAG_NOT_EASY | SPAWNFLAG_NOT_MEDIUM | SPAWNFLAG_NOT_HARD | SPAWNFLAG_NOT_COOP | SPAWNFLAG_NOT_DEATHMATCH);
		}

		ED_CallSpawn(ent);
	}
	
#ifdef ENABLE_COOP
	randomize_enemies = false;
#endif
	Com_Printf("%i entities inhibited\n", inhibit);
#ifdef DEBUG
	i = 1;
	ent = EDICT_NUM(i);

	while (i < globals.num_edicts)
	{
		if (ent->inuse != 0 || ent->inuse != 1)
			Com_DPrintf("Invalid entity %d\n", i);

		i++, ent++;
	}

#endif
	G_FindTeams();
	AI_NewMap();//JABot
	Spawn_Weapons();
}


//===================================================================

void Weapons_Init();

/*
===============
SetItemNames
===============
*/
static void SetItemNames(void)
{
	int     i;
	gitem_t *it;

	for (i = 0 ; i < game.num_items ; i++)
	{
		it = &itemlist[i];
		gi.configstring(CS_ITEMS + i, it->pickup_name);
	}
}


/*QUAKED worldspawn (0 0 0) ?

Only used for the world.
"sky"   environment map name
"skyaxis"   vector axis for rotating sky
"skyrotate" speed of rotation in degrees/second
"sounds"    music cd track number
"gravity"   800 is default gravity
"message"   text to print at user logon
*/
void SP_worldspawn(edict_t *ent)
{
	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	ent->inuse = true;          // since the world doesn't use G_Spawn()
	gi.setmodel(ent, "*1"); // world model is always index 1
	//---------------
	// reserve some spots for dead player bodies for coop / deathmatch
	InitBodyQue();
	SetItemNames();

	if (spawnTemp.nextmap)
		Q_strlcpy(level.nextmap, spawnTemp.nextmap, sizeof(level.nextmap));

	// make some data visible to the server

	if (ent->message && ent->message[0])
	{
		gi.configstring(CS_NAME, ent->message);
		Q_strlcpy(level.level_name, ent->message, sizeof(level.level_name));
	}
	else
		Q_strlcpy(level.level_name, level.mapname, sizeof(level.level_name));

	if (spawnTemp.sky && spawnTemp.sky[0])
		gi.configstring(CS_SKY, spawnTemp.sky);
	else
		gi.configstring(CS_SKY, "unit1_");

	gi.configstring(CS_SKYROTATE, va("%f", spawnTemp.skyrotate));
	gi.configstring(CS_SKYAXIS, va("%f %f %f", spawnTemp.skyaxis[0], spawnTemp.skyaxis[1], spawnTemp.skyaxis[2]));
	gi.configstring(CS_CDTRACK, va("%i", ent->sounds));
	gi.configstring(CS_MAXCLIENTS, va("%i", (int)(maxclients->value)));
	
#ifdef ENABLE_COOP
	// status bar program
	if (deathmatch->value)
		gi.configstring(CS_GAMEMODE, "deathmatch");
	else if (coop->value)
		gi.configstring(CS_GAMEMODE, "coop");
	else if (invasion->value)
		gi.configstring(CS_GAMEMODE, "invasion");
	else
		gi.configstring(CS_GAMEMODE, "singleplayer");
#endif

	//---------------
	// help icon for statusbar
#ifdef ENABLE_COOP
	gi.imageindex("i_help");
	gi.imageindex("help");
#endif
	level.pic_health = gi.imageindex("i_health");
	gi.imageindex("field_3");

	if (!spawnTemp.gravity)
		Cvar_UserSet("sv_gravity", "800");
	else
		Cvar_UserSet("sv_gravity", spawnTemp.gravity);

	snd_fry = gi.soundindex("player/fry.wav");  // standing in lava / slime
	gi.soundindex("player/lava1.wav");
	gi.soundindex("player/lava2.wav");
	gi.soundindex("misc/pc_up.wav");
	gi.soundindex("misc/talk1.wav");
	gi.soundindex("misc/udeath.wav");
	// gibs
	gi.soundindex("items/respawn1.wav");
	// sexed sounds
	gi.soundindex("*death1.wav");
	gi.soundindex("*death2.wav");
	gi.soundindex("*death3.wav");
	gi.soundindex("*death4.wav");
	gi.soundindex("*fall1.wav");
	gi.soundindex("*fall2.wav");
	gi.soundindex("*gurp1.wav");        // drowning damage
	gi.soundindex("*gurp2.wav");
	gi.soundindex("*jump1.wav");        // player jump
	gi.soundindex("*pain25_1.wav");
	gi.soundindex("*pain25_2.wav");
	gi.soundindex("*pain50_1.wav");
	gi.soundindex("*pain50_2.wav");
	gi.soundindex("*pain75_1.wav");
	gi.soundindex("*pain75_2.wav");
	gi.soundindex("*pain100_1.wav");
	gi.soundindex("*pain100_2.wav");
	// sexed models
	// THIS ORDER MUST MATCH THE DEFINES IN g_local.h
	// you can add more, max 15
	itemid_e item;

	for (item = ITI_WEAPONS_START; item <= ITI_WEAPONS_END; item++)
		gi.modelindex(GetItemByIndex(item)->weapmodel);

	//-------------------
	gi.soundindex("player/gasp1.wav");      // gasping for air
	gi.soundindex("player/gasp2.wav");      // head breaking surface, not gasping
	gi.soundindex("player/watr_in.wav");    // feet hitting water
	gi.soundindex("player/watr_out.wav");   // feet leaving water
	gi.soundindex("player/watr_un.wav");    // head going underwater
	gi.soundindex("player/u_breath1.wav");
	gi.soundindex("player/u_breath2.wav");
	gi.soundindex("items/pkup.wav");        // bonus item pickup
	gi.soundindex("world/land.wav");        // landing thud
	gi.soundindex("misc/h2ohit1.wav");      // landing splash
	gi.soundindex("items/damage.wav");
	gi.soundindex("items/protect.wav");
	gi.soundindex("items/protect4.wav");
	gi.soundindex("weapons/noammo.wav");
#ifdef ENABLE_COOP
	gi.soundindex("infantry/inflies1.wav");
#endif
	sm_meat_index = gi.modelindex("models/objects/gibs/sm_meat/tris.md2");
	gi.modelindex("models/objects/gibs/arm/tris.md2");
	gi.modelindex("models/objects/gibs/bone/tris.md2");
	gi.modelindex("models/objects/gibs/bone2/tris.md2");
	gi.modelindex("models/objects/gibs/chest/tris.md2");
	gi.modelindex("models/objects/gibs/skull/tris.md2");
	gi.modelindex("models/objects/gibs/head2/tris.md2");
	//
	// Setup light animation tables. 'a' is total darkness, 'z' is doublebright.
	//
	// 0 normal
	gi.configstring(CS_LIGHTS + 0, "m");
	// 1 FLICKER (first variety)
	gi.configstring(CS_LIGHTS + 1, "mmnmmommommnonmmonqnmmo");
	// 2 SLOW STRONG PULSE
	gi.configstring(CS_LIGHTS + 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
	// 3 CANDLE (first variety)
	gi.configstring(CS_LIGHTS + 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
	// 4 FAST STROBE
	gi.configstring(CS_LIGHTS + 4, "mamamamamama");
	// 5 GENTLE PULSE 1
	gi.configstring(CS_LIGHTS + 5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");
	// 6 FLICKER (second variety)
	gi.configstring(CS_LIGHTS + 6, "nmonqnmomnmomomno");
	// 7 CANDLE (second variety)
	gi.configstring(CS_LIGHTS + 7, "mmmaaaabcdefgmmmmaaaammmaamm");
	// 8 CANDLE (third variety)
	gi.configstring(CS_LIGHTS + 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");
	// 9 SLOW STROBE (fourth variety)
	gi.configstring(CS_LIGHTS + 9, "aaaaaaaazzzzzzzz");
	// 10 FLUORESCENT FLICKER
	gi.configstring(CS_LIGHTS + 10, "mmamammmmammamamaaamammma");
	// 11 SLOW PULSE NOT FADE TO BLACK
	gi.configstring(CS_LIGHTS + 11, "abcdefghijklmnopqrrqponmlkjihgfedcba");
	// styles 32-62 are assigned by the light program for switchable lights
	// 63 testing
	gi.configstring(CS_LIGHTS + 63, "a");

	for (item = ITI_WEAPONS_START; item <= ITI_WEAPONS_END; item++)
		PrecacheItem(GetItemByIndex(item));

#if ENABLE_COOP
	Wave_Precache();
#endif

	Weapons_Init();
}


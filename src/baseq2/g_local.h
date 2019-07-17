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
// g_local.h -- local definitions for game module

#pragma once

#include "shared/shared.h"
#include "shared/list.h"
#include "common/error.h"

// define GAME_INCLUDE so that game.h does not define the
// short, server-visible gclient_t and edict_t structures,
// because we define the full size ones in this file
#define GAME_INCLUDE
#include "shared/game.h"

#define USE_64BIT_TIME 1

#if USE_64BIT_TIME
	typedef uint64_t		gtime_t;

	#define GTIME_MAX		UINT64_MAX
	#define GTIME_MIN		0ul
#else
	typedef uint32_t		gtime_t;

	#define GTIME_MAX		UINT32_MAX
	#define GTIME_MIN		0u
#endif

static inline gtime_t time_diff(gtime_t l, gtime_t r)
{
	if (r > l)
		return 0;

	return l - r;
}

// the "gameversion" client command will print this plus compile date
#define GAMEVERSION "baseq2"

// protocol bytes that can be directly added to messages
#define svc_muzzleflash     1
#define svc_muzzleflash2    2
#define svc_temp_entity     3
#define svc_layout          4
#define svc_inventory       5
#define svc_stufftext       11

//==================================================================

// view pitching times
#define DAMAGE_TIME     500.0f
#define FALL_TIME       300.0f

typedef enum
{
	// 0 always empty, just for memset easiness
	ET_NULL,

	// Shared/Quake 2 entities
	ET_INFO_PLAYER_START,
	ET_INFO_PLAYER_DEATHMATCH,
	ET_INFO_PLAYER_COOP,
	ET_INFO_PLAYER_INTERMISSION,

	ET_FUNC_PLAT,
	ET_FUNC_BUTTON,
	ET_FUNC_DOOR,
	ET_FUNC_DOOR_SECRET,
	ET_FUNC_DOOR_ROTATING,
	ET_FUNC_ROTATING,
	ET_FUNC_TRAIN,
	ET_FUNC_WATER,
	ET_FUNC_CONVEYOR,
	ET_FUNC_AREAPORTAL,
	ET_FUNC_CLOCK,
	ET_FUNC_WALL,
	ET_FUNC_OBJECT,
	ET_FUNC_TIMER,
	ET_FUNC_EXPLOSIVE,
	ET_FUNC_KILLBOX,

	ET_TRIGGER_ALWAYS,
	ET_TRIGGER_ONCE,
	ET_TRIGGER_MULTIPLE,
	ET_TRIGGER_RELAY,
	ET_TRIGGER_PUSH,
	ET_TRIGGER_HURT,
	ET_TRIGGER_KEY,
	ET_TRIGGER_COUNTER,
	ET_TRIGGER_ELEVATOR,
	ET_TRIGGER_GRAVITY,
	ET_TRIGGER_MONSTERJUMP,

	ET_TARGET_TEMP_ENTITY,
	ET_TARGET_SPEAKER,
	ET_TARGET_EXPLOSION,
	ET_TARGET_CHANGELEVEL,
	ET_TARGET_SECRET,
	ET_TARGET_GOAL,
	ET_TARGET_SPLASH,
	ET_TARGET_SPAWNER,
	ET_TARGET_BLASTER,
	ET_TARGET_CROSSLEVEL_TRIGGER,
	ET_TARGET_CROSSLEVEL_TARGET,
	ET_TARGET_LASER,
	ET_TARGET_HELP,
	ET_TARGET_LIGHTRAMP,
	ET_TARGET_EARTHQUAKE,
	ET_TARGET_CHARACTER,
	ET_TARGET_STRING,

	ET_WORLDSPAWN,

	ET_LIGHT,
	ET_LIGHT_MINE1,
	ET_LIGHT_MINE2,
	ET_INFO_NULL,
	ET_FUNC_GROUP,
	ET_INFO_NOTNULL,
	ET_PATH_CORNER,
	ET_POINT_COMBAT,

	ET_MISC_EXPLOBOX,
	ET_MISC_BANNER,
	ET_MISC_SATELLITE_DISH,
	ET_MISC_GIB_ARM,
	ET_MISC_GIB_LEG,
	ET_MISC_GIB_HEAD,
	ET_MISC_INSANE,
	ET_MISC_DEADSOLDIER,
	ET_MISC_VIPER,
	ET_MISC_VIPER_BOMB,
	ET_MISC_BIGVIPER,
	ET_MISC_STROGG_SHIP,
	ET_MISC_TELEPORTER,
	ET_MISC_TELEPORTER_DEST,
	ET_MISC_BLACKHOLE,
	ET_MISC_EASTERTANK,
	ET_MISC_EASTERCHICK,
	ET_MISC_EASTERCHICK2,

	ET_MARKER_Q2_MONSTERS_START,
	ET_MONSTER_BERSERK = ET_MARKER_Q2_MONSTERS_START,
	ET_MONSTER_GLADIATOR,
	ET_MONSTER_GUNNER,
	ET_MONSTER_INFANTRY,
	ET_MONSTER_SOLDIER_LIGHT,
	ET_MONSTER_SOLDIER,
	ET_MONSTER_SOLDIER_SS,
	ET_MONSTER_TANK,
	ET_MONSTER_TANK_COMMANDER,
	ET_MONSTER_MEDIC,
	ET_MONSTER_FLIPPER,
	ET_MONSTER_CHICK,
	ET_MONSTER_PARASITE,
	ET_MONSTER_FLYER,
	ET_MONSTER_BRAIN,
	ET_MONSTER_FLOATER,
	ET_MONSTER_HOVER,
	ET_MONSTER_MUTANT,
	ET_MONSTER_SUPERTANK,
	ET_MONSTER_BOSS2,
	ET_MONSTER_BOSS3_STAND,
	ET_MONSTER_JORG,
	ET_MARKER_Q2_MONSTERS_END = ET_MONSTER_JORG,

	ET_MONSTER_COMMANDER_BODY,
	ET_TURRET_BREACH,
	ET_TURRET_BASE,
	ET_TURRET_DRIVER,

	// Quake 1
	ET_MARKER_Q1_MONSTERS_START,
	ET_Q1_MONSTER_ARMY = ET_MARKER_Q1_MONSTERS_START,
	ET_Q1_MONSTER_DOG,
	ET_Q1_MONSTER_OGRE,
	ET_Q1_MONSTER_OGRE_MARKSMAN,
	ET_Q1_MONSTER_KNIGHT,
	ET_Q1_MONSTER_ZOMBIE,
	ET_Q1_MONSTER_WIZARD,
	ET_Q1_MONSTER_DEMON,
	ET_Q1_MONSTER_SHAMBLER,
	ET_Q1_MONSTER_ENFORCER,
	ET_Q1_MONSTER_HELL_KNIGHT,
	ET_Q1_MONSTER_SHALRATH,
	ET_Q1_MONSTER_TARBABY,
	ET_Q1_MONSTER_FISH,
	ET_MARKER_Q1_MONSTERS_END = ET_Q1_MONSTER_FISH,

	// Doom
	ET_MARKER_DOOM_MONSTERS_START,
	ET_DOOM_MONSTER_POSS = ET_MARKER_DOOM_MONSTERS_START,
	ET_DOOM_MONSTER_SPOS,
	ET_DOOM_MONSTER_TROO,
	ET_DOOM_MONSTER_SARG,
	ET_DOOM_MONSTER_CPOS,
	ET_DOOM_MONSTER_HEAD,
	ET_DOOM_MONSTER_SKUL,
	ET_DOOM_MONSTER_PAIN,
	ET_DOOM_MONSTER_SPECTRE,
	ET_DOOM_MONSTER_BSPI,
	ET_DOOM_MONSTER_SKEL,
	ET_DOOM_MONSTER_BOSS,
	ET_DOOM_MONSTER_BOS2,
	ET_DOOM_MONSTER_SPID,
	ET_DOOM_MONSTER_CYBR,
	ET_MARKER_DOOM_MONSTERS_END = ET_DOOM_MONSTER_SPID,

	ET_LAST_SPAWNABLE,

	// IDs used by other stuff
	ET_PLAYER,
	ET_ITEM,
	ET_ROCKET,
	ET_GRENADE,
	ET_HAND_GRENADE,
	ET_BOTROAM,
	ET_PLAYER_NOISE,
	ET_WEAPON,
	ET_AMMO,

	ET_Q1_VORE_BALL,
	ET_Q1_ZOMBIE_GIB,
	ET_Q1_SPIKE,

	ET_DOOM_PLASMA,

	ET_MONSTER_MAKRON,

	ET_LAST,
	ET_FIRST = ET_NULL + 1,

	ENTITYTYPE_SPAWNABLE_COUNT = (ET_LAST_SPAWNABLE - ET_FIRST)
} entitytype_e;

// edict->spawnflags
// these are set with checkboxes on each entity in the map editor
#define SPAWNFLAG_NOT_EASY          0x00000100
#define SPAWNFLAG_NOT_MEDIUM        0x00000200
#define SPAWNFLAG_NOT_HARD          0x00000400
#define SPAWNFLAG_NOT_DEATHMATCH    0x00000800
#define SPAWNFLAG_NOT_COOP          0x00001000

// edict->flags
#define FL_FLY                  0x00000001
#define FL_SWIM                 0x00000002  // implied immunity to drowining
#define FL_IMMUNE_LASER         0x00000004
#define FL_INWATER              0x00000008
#define FL_GODMODE              0x00000010
#define FL_NOTARGET             0x00000020
#define FL_IMMUNE_SLIME         0x00000040
#define FL_IMMUNE_LAVA          0x00000080
#define FL_PARTIALGROUND        0x00000100  // not all corners are valid
#define FL_WATERJUMP            0x00000200  // player jumping out of water
#define FL_TEAMSLAVE            0x00000400  // not the first on the team
#define FL_NO_KNOCKBACK         0x00000800
#define FL_POWER_ARMOR          0x00001000  // power armor (if any) is active
#define FL_RESPAWN              0x00002000  // used for item respawning
#define FL_STUCK                0x00004000  // Generations

// memory tags to allow dynamic memory to be cleaned up
#define TAG_GAME    765     // clear when unloading the dll
#define TAG_LEVEL   766     // clear when loading a new level

#define MELEE_DISTANCE  80

#define BODY_QUEUE_SIZE     8

typedef enum
{
	DAMAGE_NO,
	DAMAGE_YES,         // will take damage if hit
	DAMAGE_AIM          // auto targeting recognizes this
} damage_t;

typedef enum
{
	WEAPON_READY,
	WEAPON_ACTIVATING,
	WEAPON_DROPPING,
	WEAPON_FIRING
} weaponstate_t;

//deadflag
#define DEAD_NO                 0
#define DEAD_DYING              1
#define DEAD_DEAD               2
#define DEAD_RESPAWNABLE        3

//range
#define RANGE_MELEE             0
#define RANGE_NEAR              1
#define RANGE_MID               2
#define RANGE_FAR               3

//gib types
typedef enum
{
	GIB_ORGANIC,
	GIB_METALLIC,
	GIB_Q1,
	GIB_DUKE,
	GIB_DUKE_SM,
	GIB_DUKE_LG
} gibtype_e;

//monster ai flags
#define AI_STAND_GROUND         0x00000001
#define AI_TEMP_STAND_GROUND    0x00000002
#define AI_SOUND_TARGET         0x00000004
#define AI_LOST_SIGHT           0x00000008
#define AI_PURSUIT_LAST_SEEN    0x00000010
#define AI_PURSUE_NEXT          0x00000020
#define AI_PURSUE_TEMP          0x00000040
#define AI_HOLD_FRAME           0x00000080
#define AI_GOOD_GUY             0x00000100
#define AI_BRUTAL               0x00000200
#define AI_NOSTEP               0x00000400
#define AI_DUCKED               0x00000800
#define AI_COMBAT_POINT         0x00001000
#define AI_MEDIC                0x00002000
#define AI_RESURRECTING         0x00004000
// Generations
#define AI_JUST_HIT				0x00008000
#define AI_NODE_PATH			0x00010000
#define AI_JUST_ATTACKED		0x00020000

//monster attack state
typedef enum
{
	AS_STRAIGHT,
	AS_SLIDING,
	AS_MELEE,
	AS_MISSILE
} attack_state_e;

// power armor types
typedef enum
{
	POWER_ARMOR_NONE,
	POWER_ARMOR_SCREEN,
	POWER_ARMOR_SHIELD
} power_armor_type_e;

// handedness values
#define RIGHT_HANDED            0
#define LEFT_HANDED             1
#define CENTER_HANDED           2

// game.serverflags values
#define SFL_CROSS_TRIGGER_1     0x00000001
#define SFL_CROSS_TRIGGER_2     0x00000002
#define SFL_CROSS_TRIGGER_3     0x00000004
#define SFL_CROSS_TRIGGER_4     0x00000008
#define SFL_CROSS_TRIGGER_5     0x00000010
#define SFL_CROSS_TRIGGER_6     0x00000020
#define SFL_CROSS_TRIGGER_7     0x00000040
#define SFL_CROSS_TRIGGER_8     0x00000080
#define SFL_CROSS_TRIGGER_MASK  0x000000ff

// noise types for PlayerNoise
#define PNOISE_SELF             0
#define PNOISE_WEAPON           1
#define PNOISE_IMPACT           2

// edict->movetype values
typedef enum
{
	MOVETYPE_NONE,          // never moves
	MOVETYPE_NOCLIP,        // origin and angles change with no interaction
	MOVETYPE_PUSH,          // no clip to world, push on box contact
	MOVETYPE_STOP,          // no clip to world, stops on box contact

	MOVETYPE_WALK,          // gravity
	MOVETYPE_STEP,          // gravity, special edge handling
	MOVETYPE_FLY,
	MOVETYPE_TOSS,          // gravity
	MOVETYPE_FLYMISSILE,    // extra size to monsters
	MOVETYPE_FLYBOUNCE,     // Generations; extra size to monsters
	MOVETYPE_BOUNCE
} movetype_t;



typedef struct
{
	int     base_count;
	int     max_count;
	float   normal_protection;
	float   energy_protection;
} gitem_armor_t;

#define IT_NONE         0       // Generations
#define IT_WEAPON       1       // use makes active weapon
#define IT_AMMO         2
#define IT_ARMOR        4
#define IT_STAY_COOP    8
#define IT_KEY          16
#define IT_POWERUP      32
#define IT_HEALTH		64		// Generations

// total default ammo
#define DEFAULT_MAX_AMMO				100.0f

// get # of ammo used by individual shots if this many shots are delivered
// relative to total default ammo
#define COUNT_FOR_SHOTS(total_shots)				(DEFAULT_MAX_AMMO / total_shots)

// get # of individual shots that can be fired if we have this count
#define SHOTS_FOR_COUNT(ammo, max_ammo, per_shot)	(ammo / max_ammo) * ((DEFAULT_MAX_AMMO / per_shot) * (max_ammo / DEFAULT_MAX_AMMO))

// get # of shots we can fire for current weapon with current ammo
#define PLAYER_SHOTS_FOR_WEAPON(ent, weapon)			SHOTS_FOR_COUNT(ent->client->pers.ammo, GetMaxAmmo(ent, CHECK_INVENTORY, CHECK_INVENTORY), EntityGame(ent)->ammo_usages[ITEM_INDEX(weapon)])

// get # of shots we can fire total for current weapon
#define PLAYER_TOTAL_SHOTS_FOR_WEAPON(ent, weapon)		SHOTS_FOR_COUNT(GetMaxAmmo(ent, CHECK_INVENTORY, CHECK_INVENTORY), GetMaxAmmo(ent, CHECK_INVENTORY, CHECK_INVENTORY), EntityGame(ent)->ammo_usages[ITEM_INDEX(weapon)])

// specific to Q2
#define AMMO_PER_POWER_ARMOR_ABSORB	0.5f

typedef struct gitem_s gitem_t;

typedef bool (*item_pickup_func)(edict_t *ent, edict_t *other);
typedef void (*item_use_func)(edict_t *ent, gitem_t *item);
typedef void (*item_drop_func)(edict_t *ent, gitem_t *item);
typedef void (*weapon_think_func)(edict_t *ent, gunindex_e gun);

typedef struct gitem_s
{
	const char        *classname; // spawning name
	item_pickup_func  pickup;
	item_use_func     use;
	item_drop_func    drop;
	const char        *pickup_sound;
	const char        *world_model;
	int               world_model_flags;
	const char        *view_model;

	// client side info
	const char        *icon;
	const char        *pickup_name;   // for printing on pickup

	int			      flags;          // IT_* flags

	const char	      *weapmodel;      // weapon model index (for weapons)

	const char        *precaches;     // string of all models, sounds, and images this item will use

	int			      respawn_time;
	const char		  *ammo_model;
	const char		  *ammo_icon;

	// private
	list_t		      hashEntry;
} gitem_t;

typedef struct
{
	gitem_armor_t	*old_armor;
	itemid_e		old_armor_index;
	int				old_armor_count;

	gitem_armor_t	*new_armor;
	itemid_e		new_armor_index;
	int				new_armor_count;
} pickup_armor_t;

typedef struct
{
	itemid_e	item_redirects[ITI_TOTAL];

	int			item_starts[ITI_TOTAL];

	itemid_e	default_item;

	float		ammo_pickups[ITI_TOTAL];

	gitem_armor_t armors[ITI_BODY_ARMOR - ITI_JACKET_ARMOR + 1];

	float		ammo_usages[ITI_TOTAL], default_ammo_usages[ITI_TOTAL];
} game_iteminfo_t;

extern game_iteminfo_t game_iteminfos[GAME_TOTAL];

#define GameTypeGame(e)	(&game_iteminfos[e])
#define EntityGame(e)	GameTypeGame(e->s.game)

typedef struct
{
	itemid_e	weapons[ITI_WEAPONS_END - ITI_WEAPONS_START + 1];
} game_weaponmap_list_t;

typedef struct
{
	game_weaponmap_list_t	to[GAME_TOTAL];
} game_weaponmap_t;

extern game_weaponmap_t game_weaponmap[GAME_TOTAL];

float GetWeaponUsageCount(edict_t *ent, gitem_t *weapon);
#define CHECK_INVENTORY -1
float GetMaxAmmo(edict_t *ent, int has_bandolier, int has_ammo_pack);
gitem_t *ResolveItemRedirect(edict_t *ent, gitem_t *item);
gitem_t *ResolveInventoryItem(edict_t *ent, gitem_t *item);
bool Can_Pickup_Armor(edict_t *ent, gitem_t *armor, pickup_armor_t *result);
bool HasEnoughAmmoToFireShots(edict_t *ent, gitem_t *weapon, int shots);
#define HasEnoughAmmoToFire(ent, weapon) HasEnoughAmmoToFireShots(ent, weapon, 1)
void RemoveAmmoFromFiringShots(edict_t *ent, gitem_t *weapon, int shots);
#define RemoveAmmoFromFiring(ent, weapon) RemoveAmmoFromFiringShots(ent, weapon, 1)

//
// this structure is left intact through an entire game
// it should be initialized at dll load time, and read/written to
// the server.ssv file for savegames
//
typedef struct
{
	int         helpchanged;    // flash F1 icon if non 0, play sound
	// and increment only if 1, 2, or 3

	char        helpmessage1[512];
	char        helpmessage2[512];
	gclient_t   *clients;       // [maxclients]

	// can't store spawnpoint in level, because
	// it would get overwritten by the savegame restore
	char        spawnpoint[512];    // needed for coop respawns

	// store latched cvars here that we want to get at often
	int         maxclients;
	int         maxentities;

	// cross level triggers
	int         serverflags;

	// items
	int         num_items;

	bool	    autosaved;

	int         framerate;
	int         frametime;
	int         framediv;
	float		frameseconds;

	time_t		random_seed;
} game_locals_t;

//
// this structure is cleared as each map is entered
// it is read/written to the level.sav file for savegames
//
typedef struct
{
	int         framenum;
	gtime_t     time;

	char        level_name[MAX_QPATH];  // the descriptive name (Outer Base, etc)
	char        mapname[MAX_QPATH];     // the server name (base1, etc)
	char        nextmap[MAX_QPATH];     // go here when fraglimit is hit

	// intermission state
	float       intermissiontime;       // time the intermission was started
	char        *changemap;
	int         exitintermission;
	vec3_t      intermission_origin;
	vec3_t      intermission_angle;

	edict_t     *sight_client;  // changed once each frame for coop games

	edict_t     *sight_entity;
	gtime_t     sight_entity_time;
	edict_t     *sound_entity;
	gtime_t     sound_entity_time;
	edict_t     *sound2_entity;
	gtime_t     sound2_entity_time;

	q_imagehandle pic_health;

	int         total_secrets;
	int         found_secrets;

	int         total_goals;
	int         found_goals;

	int         total_monsters;
	int         killed_monsters;

	edict_t     *current_entity;    // entity running from G_RunFrame
	int         body_que;           // dead bodies

	int         power_cubes;        // ugly necessity for coop
	time_t		spawn_rand;
	int			weapon_spawn_id;
	int			ammo_spawn_id;
} level_locals_t;


// spawn_temp_t is only used to hold entity field values that
// can be set from the editor, but aren't actualy present
// in edict_t during gameplay
typedef struct
{
	// world vars
	char        *sky;
	float       skyrotate;
	vec3_t      skyaxis;
	char        *nextmap;

	int         lip;
	int         distance;
	int         height;
	char        *noise;
	float       pausetime;
	char        *item;
	char        *gravity;

	float       minyaw;
	float       maxyaw;
	float       minpitch;
	float       maxpitch;

	float		nextthink;
	char		classname[MAX_TOKEN_CHARS];
} spawn_temp_t;

typedef enum
{
	STATE_TOP,
	STATE_BOTTOM,
	STATE_UP,
	STATE_DOWN
} movestate_e;

typedef struct
{
	// fixed data
	vec3_t      start_origin;
	vec3_t      start_angles;
	vec3_t      end_origin;
	vec3_t      end_angles;

	q_soundhandle sound_start;
	q_soundhandle sound_middle;
	q_soundhandle sound_end;

	float       accel;
	float       speed;
	float       decel;
	float       distance;

	float       wait;

	// state data
	movestate_e state;
	vec3_t      dir;
	float       current_speed;
	float       move_speed;
	float       next_speed;
	float       remaining_distance;
	float       decel_distance;
	void	(*endfunc)(edict_t *);
} moveinfo_t;

typedef void (*mai_func_t) (edict_t *self, float dist);

typedef struct
{
	mai_func_t	aifunc;
	float		dist;
	void		(*thinkfunc)(edict_t *self);
	int			real_frame;
} mframe_t;

typedef struct
{
	int         firstframe;
	int         lastframe;
	mframe_t	*frame;
	void		(*endfunc)(edict_t *self);

	char		name[32];
	list_t		listEntry, hashEntry;
} mmove_t;

typedef struct
{
	const mmove_t	*currentmove;
	int				aiflags;
	int				nextframe;
	float			scale;

	void	(*stand)(edict_t *self);
	void	(*idle)(edict_t *self);
	void	(*search)(edict_t *self);
	void	(*walk)(edict_t *self);
	void	(*run)(edict_t *self);
	void	(*dodge)(edict_t *self, edict_t *other, float eta);
	void	(*attack)(edict_t *self);
	void	(*melee)(edict_t *self);
	void	(*sight)(edict_t *self, edict_t *other);
	bool	(*checkattack)(edict_t *self);

	gtime_t     pausetime;
	gtime_t     attack_finished;

	vec3_t      saved_goal;
	gtime_t     search_time;
	gtime_t     trail_time;
	vec3_t      last_sighting;
	attack_state_e attack_state;
	int         lefty;
	gtime_t     idle_time;
	int         linkcount;

	power_armor_type_e         power_armor_type;
	int         power_armor_power;

	// Generations
	bool		special_frames;

	nav_igator_t	*navigator;
	uint32_t		navigator_node_current, navigator_node_max;
	vec3_t			navigator_pos;

	list_t		wave_entry;
} monsterinfo_t;

extern  q_modelhandle sm_meat_index;
extern  q_soundhandle snd_fry;

// Generations
typedef enum
{
	DT_DIRECT, // Damage done directly. May not have a different inflictor entity (hitscan weapons).
	DT_INDIRECT, // Indirect damage done by the inflictor.
	DT_EFFECT,
	DT_LASER
} damageType_e;

typedef enum
{
	MD_NONE,
	MD_WEAPON,

	MD_TELEFRAG,
	MD_FALLING,

	MD_MELEE,
	MD_BFG_EFFECT,
	MD_LASER,

	MD_WATER,
	MD_SLIME,
	MD_LAVA,

	MD_MEANS_MASK = (1 << 8) - 1,
	MD_FRIENDLY_FIRE = 1 << 8,
	MD_HELD = 1 << 9
} meansOfDeath_e;

typedef struct
{
	// Type of damage
	damageType_e	damage_type;

	// "attacker" is the entity that last tried to hurt
	// us. attacker is only really valid if it's a client, otherwise
	// use attacker_type to determine what did it (ex, rocket fired by
	// tank that somehow stayed alive for enough frames to kill you after
	// he was freed). Only monsters and clients can be attackers.
	edict_t			*attacker;
	entitytype_e	attacker_type;
	gametype_t		attacker_game;
	gtime_t			attacker_time;

	// Weapon that was used by attacker to kill us.
	itemid_e		weapon_id;

	// "inflictor" is the entity that actually did the killing.
	// Always valid, but might be same as attacker in some cases
	// (hitscan weapons).
	edict_t			*inflictor;

	// Extra info that might be attached from damage type
	meansOfDeath_e	damage_means;

	// Time we got hit
	gtime_t			time;
} meansOfDeath_t;

extern meansOfDeath_t blankMeansOfDeath;

extern  edict_t         *g_edicts;

#define FOFS(x)  q_offsetof(edict_t, x)
#define STOFS(x) q_offsetof(spawn_temp_t, x)
#define LLOFS(x) q_offsetof(level_locals_t, x)
#define GLOFS(x) q_offsetof(game_locals_t, x)
#define CLOFS(x) q_offsetof(gclient_t, x)

#define random()    frand()
#define crandom()   crand()

extern  cvar_t  *maxentities;
extern  cvar_t  *deathmatch;
extern  cvar_t  *coop;
extern  cvar_t  *dmflags;
extern  cvar_t  *skill;
extern  cvar_t  *fraglimit;
extern  cvar_t  *timelimit;
extern  cvar_t  *password;
extern  cvar_t  *spectator_password;
extern  cvar_t  *needpass;
extern  cvar_t  *g_select_empty;
extern  cvar_t  *dedicated;

extern  cvar_t  *filterban;

extern  cvar_t  *sv_gravity;
extern  cvar_t  *sv_maxvelocity;

extern  cvar_t  *gun_x, *gun_y, *gun_z;
extern  cvar_t  *sv_rollspeed;
extern  cvar_t  *sv_rollangle;

extern  cvar_t  *run_pitch;
extern  cvar_t  *run_roll;
extern  cvar_t  *bob_up;
extern  cvar_t  *bob_pitch;
extern  cvar_t  *bob_roll;

extern  cvar_t  *sv_cheats;
extern  cvar_t  *maxclients;
extern  cvar_t  *maxspectators;

extern  cvar_t  *flood_msgs;
extern  cvar_t  *flood_persecond;
extern  cvar_t  *flood_waitdelay;

extern  cvar_t  *sv_maplist;

// Generations
extern	cvar_t	*invasion;

#define world   (&g_edicts[0])

// item spawnflags
#define ITEM_TRIGGER_SPAWN      0x00000001
#define ITEM_NO_TOUCH           0x00000002
// 6 bits reserved for editor flags
// 8 bits used as power cube id bits for coop games
#define DROPPED_ITEM            0x00010000
#define DROPPED_PLAYER_ITEM     0x00020000
#define ITEM_TARGETS_USED       0x00040000

//
// fields are needed for spawning from the entity string
// and saving / loading games
//
typedef enum
{
	F_BAD,
	F_BYTE,
	F_SHORT,
	F_INT,
	F_BOOL,
	F_FLOAT,
	F_LSTRING,          // string on disk, pointer in memory, TAG_LEVEL
	F_GSTRING,          // string on disk, pointer in memory, TAG_GAME
	F_ZSTRING,          // string on disk, string in memory
	F_VECTOR,
	F_ANGLEHACK,
	F_EDICT,            // index on disk, pointer in memory
	F_ITEM,             // index on disk, pointer in memory
	F_CLIENT,           // index on disk, pointer in memory
	F_FUNCTION,
	F_POINTER,
	F_IGNORE
} fieldtype_t;

extern  gitem_t itemlist[];

//
// g_cmds.c
//
void Cmd_Help_f(edict_t *ent);
void Cmd_Score_f(edict_t *ent);

//
// g_items.c
//
void PrecacheItem(gitem_t *it);
void InitItems(void);
gitem_t *FindItem(const char *pickup_name);
gitem_t *FindItemByClassname(const char *classname);
#define ITEM_INDEX(x) GetIndexByItem(x)
itemid_e GetIndexByItem(gitem_t *item);
edict_t *Drop_Item(edict_t *ent, gitem_t *item);
void SetRespawn(edict_t *ent, float delay);
void ChangeWeapon(edict_t *ent, gunindex_e gun);
gitem_t *GetBestWeapon(edict_t *ent, gitem_t *new_item, bool is_new);
void SpawnItem(edict_t *ent, gitem_t *item);
void Think_Weapon(edict_t *ent);
itemid_e ArmorIndex(edict_t *ent);
power_armor_type_e PowerArmorType(edict_t *ent);
gitem_t *GetItemByIndex(itemid_e index);
bool Add_Ammo(edict_t *ent, float count, itemid_e index, bool pickup);
void Touch_Item(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);
void CheckWeaponBalanceCvars();

//
// g_utils.c
//
bool    KillBox(edict_t *ent);
void    G_ProjectSource(const vec3_t point, const vec3_t distance, const vec3_t forward, const vec3_t right, vec3_t result);
edict_t *G_Find(edict_t *from, int fieldofs, char *match);
edict_t *G_FindByType(edict_t *from, entitytype_e type);
edict_t *findradius(edict_t *from, vec3_t org, float rad);
edict_t *G_PickTarget(char *targetname);
void    G_UseTargets(edict_t *ent, edict_t *activator);
void    G_SetMovedir(vec3_t angles, vec3_t movedir);

void    G_InitEdict(edict_t *e);
edict_t *G_Spawn(void);
void    G_FreeEdict(edict_t *e);

void    G_TouchTriggers(edict_t *ent);
void    G_TouchSolids(edict_t *ent);

char    *G_CopyString(char *in);

float   *tv(float x, float y, float z);
char    *vtos(vec3_t v);

float vectoyaw(vec3_t vec);
void vectoangles(vec3_t vec, vec3_t angles);
void vectoangles2(const vec3_t value1, vec3_t angles);

//
// g_combat.c
//
bool OnSameTeam(edict_t *ent1, edict_t *ent2);
bool CanDamage(edict_t *targ, edict_t *inflictor);

// damage flags
enum
{
	DAMAGE_NONE				= 0,
	DAMAGE_RADIUS			= 1 << 0, // damage was indirect
	DAMAGE_NO_ARMOR			= 1 << 1, // armour does not protect from this damage
	DAMAGE_ENERGY			= 1 << 2, // damage is from an energy based weapon
	DAMAGE_NO_KNOCKBACK		= 1 << 3, // do not affect velocity, just view angles
	DAMAGE_BULLET			= 1 << 4, // damage is from a bullet (used for ricochets)
	DAMAGE_NO_PROTECTION	= 1 << 5, // armor, shields, invulnerability, and godmode have no effect

	// Paril
	DAMAGE_NO_PARTICLES		= 1 << 6,
	DAMAGE_Q1				= 1 << 7,
	DAMAGE_PARTICLES_ONLY	= 1 << 8,
	DAMAGE_DUKE				= 1 << 9
};

void T_Damage(edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, vec3_t normal, int damage, int knockback, int dflags, meansOfDeath_t mod);
void T_RadiusDamage(edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, int dflags, float radius, meansOfDeath_t mod);

#define DEFAULT_BULLET_HSPREAD  300
#define DEFAULT_BULLET_VSPREAD  500
#define DEFAULT_SHOTGUN_HSPREAD 1000
#define DEFAULT_SHOTGUN_VSPREAD 500
#define DEFAULT_DEATHMATCH_SHOTGUN_COUNT    12
#define DEFAULT_SHOTGUN_COUNT   12
#define DEFAULT_SSHOTGUN_COUNT  20

//
// g_monster.c
//
void monster_fire_bullet(edict_t *self, vec3_t start, vec3_t dir, int damage, int kick, int hspread, int vspread, int flashtype);
void monster_fire_shotgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int flashtype);
void monster_fire_blaster(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype, int effect);
void monster_fire_grenade(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int flashtype);
void monster_fire_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype);
void monster_fire_railgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int flashtype);
void monster_fire_bfg(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int kick, float damage_radius, int flashtype);
void M_droptofloor(edict_t *ent);
void monster_think(edict_t *self);
void walkmonster_start(edict_t *self);
void swimmonster_start(edict_t *self);
void flymonster_start(edict_t *self);
void AttackFinished(edict_t *self, float time);
void monster_death_use(edict_t *self);
void M_CatagorizePosition(edict_t *ent);
bool M_CheckAttack(edict_t *self);
bool M_Q1_CheckAttack(edict_t *self);
bool M_Doom_CheckAttack(edict_t *self);
void M_FlyCheck(edict_t *self);
void M_CheckGround(edict_t *ent);

//
// g_misc.c
//
void ThrowHead(edict_t *self, char *gibname, int damage, gibtype_e type);
void ThrowClientHead(edict_t *self, int damage);
void ThrowGib(edict_t *self, char *gibname, int damage, gibtype_e type);
void BecomeExplosion1(edict_t *self);

#define CLOCK_MESSAGE_SIZE  16

//
// g_ai.c
//
void AI_SetSightClient(void);

void ai_stand(edict_t *self, float dist);
void ai_move(edict_t *self, float dist);
void ai_walk(edict_t *self, float dist);
void ai_turn(edict_t *self, float dist);
void ai_run(edict_t *self, float dist);
void ai_charge(edict_t *self, float dist);
int range(edict_t *self, edict_t *other);

void FoundTarget(edict_t *self);
bool infront(edict_t *self, edict_t *other);
bool visible(edict_t *self, edict_t *other);
bool FacingIdeal(edict_t *self);

//
// g_weapon.c
//
void ThrowDebris(edict_t *self, char *modelname, float speed, vec3_t origin);
bool fire_hit(edict_t *self, vec3_t aim, int damage, int kick);
void fire_bullet(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, meansOfDeath_t mod);
void fire_shotgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, meansOfDeath_t mod);
void fire_blaster(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int effect, bool hyper);
void fire_grenade(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius);
void fire_grenade2(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, gtime_t timer, float damage_radius, bool held);
void fire_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage);
void fire_rail(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick);
void fire_bfg(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius);

//
// g_client.c
//
void respawn(edict_t *ent);
void BeginIntermission(edict_t *targ);
void PutClientInServer(edict_t *ent);
void InitClientResp(gclient_t *client);
void InitBodyQue(void);
void ClientBeginServerFrame(edict_t *ent);

//
// g_player.c
//
void player_pain(edict_t *self, edict_t *other, float kick, int damage);
void player_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

//
// g_svcmds.c
//
void    ServerCommand(void);
bool SV_FilterPacket(char *from);

//
// p_view.c
//
void ClientEndServerFrame(edict_t *ent);

//
// p_hud.c
//
void MoveClientToIntermission(edict_t *client);
void G_SetStats(edict_t *ent);
void G_SetSpectatorStats(edict_t *ent);
void G_CheckChaseStats(edict_t *ent);
void ValidateSelectedItem(edict_t *ent);
void DeathmatchScoreboardMessage(edict_t *client, edict_t *killer);

//
// m_move.c
//
bool M_CheckBottom(edict_t *ent);
bool M_walkmove(edict_t *ent, float yaw, float dist);
void M_MoveToGoal(edict_t *ent, float dist);
void M_ChangeYaw(edict_t *ent);

//
// g_phys.c
//
void G_RunEntity(edict_t *ent);

//
// g_main.c
//
void SaveClientData(void);
void FetchClientEntData(edict_t *ent);

//
// g_chase.c
//
void UpdateChaseCam(edict_t *ent);
void ChaseNext(edict_t *ent);
void ChasePrev(edict_t *ent);
void GetChaseTarget(edict_t *ent);

//
// p_weapon.c
//
void NoAmmoWeaponChange(edict_t *ent, gunindex_e gun);
void check_dodge(edict_t *, vec3_t, vec3_t, int);
void P_ProjectSource(gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
void Weapon_QuadDamage(edict_t *ent);
void PlayerNoise(edict_t *who, vec3_t where, int type);
void AttemptBetterWeaponSwap(edict_t *ent);

// p_q1_weapons.c
void ApplyMultiDamage(edict_t *self, vec3_t aimdir, int dflags, meansOfDeath_t multi_mod);
void AddMultiDamage(edict_t *hit, int damage, int kick, meansOfDeath_t multi_mod, int dflags, bool absorb_all, vec3_t point, vec3_t normal);
void Weapon_Q1_Run(edict_t *ent, gunindex_e gun);

// p_doom_weapons.c
void Weapon_Doom_Run(edict_t *ent, gunindex_e gun);

// p_duke_weapons.c
void Weapon_Duke_Run(edict_t *ent, gunindex_e gun);


//============================================================================

#ifndef NO_BOTS
	#include "ai\ai.h"//JABot
#endif

// client_t->anim_priority
enum
{
	ANIM_BASIC,       // stand / run
	ANIM_WAVE,
	ANIM_JUMP,
	ANIM_PAIN,
	ANIM_ATTACK,
	ANIM_DEATH,

	ANIM_REVERSE    = 1 << 3
};

// client data that stays across multiple level loads
typedef struct
{
	char        userinfo[MAX_INFO_STRING];
	char        netname[16];
	int         hand;

	bool        connected;  // a loadgame will leave valid entities that
	// just don't have a connection yet

	// values saved and restored from edicts when changing levels
	int         health;
	int         max_health;
	int			savedFlags;

	itemid_e    selected_item;
	int         inventory[ITI_TOTAL];

	gitem_t     *weapon;
	gitem_t     *lastweapon;

	int         power_cubes;    // used for tracking the cubes in coop games
	int         score;          // for calculating total unit score in coop games

	int         game_helpchanged;
	int         helpchanged;

	bool        spectator;          // client is a spectator
	gametype_t	game;

	float		ammo;
	int			pistol_clip; // duke only
	int			pipebomb_hold;
	bool    	alt_weapon;
} client_persistant_t;

// client data that stays across deathmatch respawns
typedef struct
{
	client_persistant_t coop_respawn;   // what to set client->pers to on a respawn
	gtime_t     entertime;         // level.time the client entered the game
	int         score;              // frags, etc
	vec3_t      cmd_angles;         // angles sent over in the last command

	bool	    spectator;          // client is a spectator
	gametype_t	game;
} client_respawn_t;


// this structure is cleared on each PutClientInServer(),
// except for 'client->pers'
struct gclient_s
{
	// known to server
	player_state_t  ps;             // communicated by server to clients
	int             ping;
	int				clientNum;

	// private to game
	client_persistant_t pers;
	client_respawn_t    resp;
	pmove_state_t       old_pmove;  // for detecting out-of-pmove changes

	bool        showscores;         // set layout stat
	bool        showinventory;      // set layout stat
	bool        showhelp;
	bool        showhelpicon;

	int         ammo_index;

	int         buttons;
	int         oldbuttons;
	int         latched_buttons;

	bool	    weapon_thunk;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int         damage_armor;       // damage absorbed by armor
	int         damage_parmor;      // damage absorbed by power armor
	int         damage_blood;       // damage taken out of health
	int         damage_knockback;   // impact damage
	vec3_t      damage_from;        // origin for vector calculation

	float       killer_yaw;         // when dead, look at killer

	struct gun_state_s
	{
		weaponstate_t   weaponstate;
		vec3_t			kick_angles;    // weapon kicks
		vec3_t			kick_origin;
		gtime_t			kick_time;
		gtime_t			weapon_time;
		gitem_t			*newweapon;
		int				machinegun_shots;   // for weapon raising
	}			gunstates[MAX_PLAYER_GUNS];

	float       v_dmg_roll, v_dmg_pitch;
	gtime_t		v_dmg_time;    // damage kicks
	gtime_t     fall_time;
	float		fall_value;      // for view drop on fall
	float       damage_alpha;
	float       bonus_alpha;
	vec3_t      damage_blend;
	vec3_t      v_angle;            // aiming direction
	float       bobtime;            // so off-ground doesn't change it
	vec3_t      oldviewangles;
	vec3_t      oldvelocity;

	gtime_t     next_drown_time;
	int         old_waterlevel;
	int         breather_sound;

	// animation vars
	int         anim_end;
	int			anim_priority;
	bool	    anim_duck;
	bool  	 	anim_run;
	bool		force_anim;
	gtime_t		hold_frame;

	// powerup timers
	gtime_t     quad_time;
	gtime_t     invincible_time;
	gtime_t     breather_time;
	gtime_t     enviro_time;

	bool	      grenade_blew_up;
	gtime_t       grenade_time;
	int           silencer_shots;
	q_soundhandle weapon_sound;

	gtime_t     pickup_msg_time;
	gitem_t		*pickup_item;
	bool		pickup_item_first;

	gtime_t     flood_locktill;     // locked from talking
	gtime_t     flood_when[10];     // when messages were said
	int         flood_whenhead;     // head pointer for when said

	gtime_t     respawn_time;       // can respawn when time > this

	edict_t     *chase_target;      // player we are chasing
	bool	    update_chase;       // need to update chase info?

	gtime_t		next_weapon_update;
	gtime_t		player_time;
	int			st_faceindex, st_facecount;
	int			old_health;
	float		damage_dir;
	int			lastattackdown;
	int			lastcalc;
	int			priority;
};


struct edict_s
{
	entity_state_t  s;
	struct gclient_s    *client;    // NULL if not a player
	// the server expects the first part
	// of gclient_s to be a player_state_t
	// but the rest of it is opaque

	bool    inuse;
	int         linkcount;

	// FIXME: move these fields to a server private sv_entity_t
	list_t      area;               // linked to a division node or leaf

	int         num_clusters;       // if -1, use headnode instead
	int         clusternums[MAX_ENT_CLUSTERS];
	int         headnode;           // unused if num_clusters != -1
	int         areanum, areanum2;

	//================================

	int         svflags;
	vec3_t      mins, maxs;
	vec3_t      absmin, absmax, size;
	solid_t     solid;
	int         clipmask;
	edict_t     *owner;


	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!

	//================================
	movetype_t         movetype;
	int			       flags;

	char        *model;
	gtime_t     freetime;           // sv.time when the object was freed

	//
	// only used locally in game, not by server
	//
	char        *message;
	entitytype_e	entitytype;
	int         spawnflags;

	float       timestamp;

	float       angle;          // set in qe3, -1 = up, -2 = down
	char        *target;
	char        *targetname;
	char        *killtarget;
	char        *team;
	char        *pathtarget;
	char        *deathtarget;
	char        *combattarget;
	edict_t     *target_ent;

	float       speed, accel, decel;
	vec3_t      movedir;
	vec3_t      pos1, pos2;

	vec3_t      velocity;
	vec3_t      avelocity;
	int         mass;
	gtime_t     air_finished;
	float       gravity;        // per entity gravity multiplier (1.0 is normal)
	// use for lowgrav artifact, flares

	edict_t     *goalentity;
	edict_t     *movetarget;
	float       yaw_speed;
	float       ideal_yaw, ideal_pitch;

	gtime_t     nextthink;
	void(*prethink)(edict_t *ent);
	void(*think)(edict_t *self);
	void(*blocked)(edict_t *self, edict_t *other);         // move to moveinfo?
	void(*touch)(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
	void(*use)(edict_t *self, edict_t *other, edict_t *activator);
	void(*pain)(edict_t *self, edict_t *other, float kick, int damage);
	void(*die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

	gtime_t     touch_debounce_time;        // are all these legit?  do we need more/less of them?
	gtime_t     pain_debounce_time;
	gtime_t     damage_debounce_time;
	gtime_t     fly_sound_debounce_time;    // move to clientinfo
	gtime_t     last_move_time;
	gtime_t		attack_finished_time;

	int         health;
	int         max_health;
	int         gib_health;
	int		    deadflag;
	gtime_t     show_hostile;

	gtime_t     powerarmor_time;

	char        *map;           // target_changelevel

	int         radius_dmg;
	union
	{
		int         dmg;
		itemid_e	weapon_id;
	};

	float		pack_ammo;
	int			pack_weapons;

	int         viewheight;     // height above origin where eyesight is determined
	damage_t    takedamage;

	float       dmg_radius;
	int         sounds;         // make this a spawntemp var?
	int         count;

	edict_t     *chain;
	edict_t     *enemy;
	edict_t     *oldenemy;
	edict_t     *activator;
	edict_t     *groundentity;
	int         groundentity_linkcount;
	edict_t     *teamchain;
	edict_t     *teammaster;

	union
	{
		struct
		{
			edict_t     *mynoise;       // can go in client only
			edict_t     *mynoise2;
		};

		struct
		{
			edict_t *entity_next;
			edict_t *entity_prev;
		};
	};

	q_soundhandle noise_index;
	q_soundhandle noise_index2;
	float       volume;
	float       attenuation;

	// timing variables
	float       wait;
	float       delay;          // before firing targets
	float       random;

	gtime_t     teleport_time;

	int         watertype;
	int         waterlevel;
	bool		watercheck;

	vec3_t      move_origin;
	vec3_t      move_angles;

	int         style;          // also used as areaportal number

	gitem_t     *item, *real_item;          // for bonus items

	// common data blocks
	moveinfo_t      moveinfo;
	monsterinfo_t   monsterinfo;

	meansOfDeath_t	meansOfDeath;
	gtime_t dmgtime;

	float t_width, t_length;
	vec3_t dest1, dest2;
	int noise, noise1, noise2, noise3, noise4;
	edict_t *trigger_field;
	int hit_index;

#ifndef NO_BOTS
	ai_handle_t		*ai;		//jabot092(2)
	bool			is_swim;	//AI_CategorizePosition
	bool			is_step;
	bool			is_ladder;
	bool			was_swim;
	bool			was_step;
#endif

	gtime_t			freeze_time;
};



extern  game_locals_t   game;
extern  level_locals_t  level;
extern  game_import_t   gi;
extern  spawn_temp_t    spawnTemp;
extern  game_export_t   globals;

static inline uint32_t G_FramesForTime(uint32_t ms)
{
	return (uint32_t)(ms / game.frametime);
}

static inline uint32_t G_SecondsForFrames(uint32_t frames)
{
	return (uint32_t)(frames * game.frametime);
}

static inline bool G_FlashyFrame(gtime_t time_left)
{
	return time_left > 3000 || (time_left % 1000) > 500;
}

// Means of death from non-client/non-monster stuff
static inline meansOfDeath_t MakeGenericMeansOfDeath(edict_t *inflictor, meansOfDeath_e damage_means, damageType_e damage_type)
{
	return (meansOfDeath_t)
	{
		damage_type, inflictor, inflictor->entitytype, inflictor->s.game, level.time + 10000, ITI_NULL, inflictor, damage_means, level.time
	};
}

// Means of death from client/monster from a non-weapon
static inline meansOfDeath_t MakeAttackerMeansOfDeath(edict_t *attacker, edict_t *inflictor, meansOfDeath_e damage_means, damageType_e damage_type)
{
	return (meansOfDeath_t)
	{
		damage_type, attacker, attacker->entitytype, attacker->s.game, level.time + 10000, ITI_NULL, inflictor, damage_means, level.time
	};
}

// Means of death from client/monster from a non-weapon
static inline meansOfDeath_t MakeWeaponMeansOfDeath(edict_t *attacker, itemid_e weapon, edict_t *inflictor, damageType_e damage_type)
{
	return (meansOfDeath_t)
	{
		damage_type, attacker, attacker->entitytype, attacker->s.game, level.time + 10000, weapon, inflictor, MD_WEAPON, level.time
	};
}

// Quick for blank stuff (platforms, etc)
static inline meansOfDeath_t MakeBlankMeansOfDeath(edict_t *inflictor)
{
	return MakeGenericMeansOfDeath(inflictor, MD_NONE, DT_DIRECT);
}
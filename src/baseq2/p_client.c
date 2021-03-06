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
#include "m_player.h"

void ClientUserinfoChanged(edict_t *ent, char *userinfo);

void SP_misc_teleporter_dest(edict_t *ent);


#ifdef ENABLE_COOP
//
// Gross, ugly, disgustuing hack section
//

// this function is an ugly as hell hack to fix some map flaws
//
// the coop spawn spots on some maps are SNAFU.  There are coop spots
// with the wrong targetname as well as spots with no name at all
//
// we use carnal knowledge of the maps to fix the coop spot targetnames to match
// that of the nearest named single player spot

static void SP_FixCoopSpots(edict_t *self)
{
	edict_t *spot;
	vec3_t  d;
	spot = NULL;

	while (1)
	{
		spot = G_FindByType(spot, ET_INFO_PLAYER_START);

		if (!spot)
			return;

		if (!spot->targetname)
			continue;

		VectorSubtract(self->server.state.origin, spot->server.state.origin, d);

		if (VectorLength(d) < 384)
		{
			if ((!self->targetname) || Q_stricmp(self->targetname, spot->targetname) != 0)
			{
				//              Com_Printf("FixCoopSpots changed %s at %s targetname from %s to %s\n", self->classname, vtos(self->server.state.origin), self->targetname, spot->targetname);
				self->targetname = spot->targetname;
			}

			return;
		}
	}
}

// now if that one wasn't ugly enough for you then try this one on for size
// some maps don't have any coop spots at all, so we need to create them
// where they should have been

static void SP_CreateCoopSpots(edict_t *self)
{
	edict_t *spot;

	if (Q_stricmp(level.mapname, "security") == 0)
	{
		spot = G_Spawn();
		spot->entitytype = ET_INFO_PLAYER_COOP;
		spot->server.state.origin[0] = 188 - 64;
		spot->server.state.origin[1] = -164;
		spot->server.state.origin[2] = 80;
		spot->targetname = "jail3";
		spot->server.state.angles[1] = 90;
		spot = G_Spawn();
		spot->entitytype = ET_INFO_PLAYER_COOP;
		spot->server.state.origin[0] = 188 + 64;
		spot->server.state.origin[1] = -164;
		spot->server.state.origin[2] = 80;
		spot->targetname = "jail3";
		spot->server.state.angles[1] = 90;
		spot = G_Spawn();
		spot->entitytype = ET_INFO_PLAYER_COOP;
		spot->server.state.origin[0] = 188 + 128;
		spot->server.state.origin[1] = -164;
		spot->server.state.origin[2] = 80;
		spot->targetname = "jail3";
		spot->server.state.angles[1] = 90;
		return;
	}
}
#endif

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
The normal starting point for a level.
*/
void SP_info_player_start(edict_t *self)
{
#ifdef ENABLE_COOP
	if (coop->value && Q_stricmp(level.mapname, "security") == 0)
	{
		// invoke one of our gross, ugly, disgusting hacks
		self->think = SP_CreateCoopSpots;
		self->nextthink = level.time + 1;
	}
#endif
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for deathmatch games
*/
void SP_info_player_deathmatch(edict_t *self)
{
#ifdef ENABLE_COOP
	if (!deathmatch->value && !invasion->value)
	{
		G_FreeEdict(self);
		return;
	}
#endif

	SP_misc_teleporter_dest(self);
}

/*QUAKED info_player_coop (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for coop games
*/

void SP_info_player_coop(edict_t *self)
{
#ifdef ENABLE_COOP
	if (!coop->value && !invasion->value)
	{
		G_FreeEdict(self);
		return;
	}

	if ((Q_stricmp(level.mapname, "jail2") == 0)   ||
		(Q_stricmp(level.mapname, "jail4") == 0)   ||
		(Q_stricmp(level.mapname, "mine1") == 0)   ||
		(Q_stricmp(level.mapname, "mine2") == 0)   ||
		(Q_stricmp(level.mapname, "mine3") == 0)   ||
		(Q_stricmp(level.mapname, "mine4") == 0)   ||
		(Q_stricmp(level.mapname, "lab") == 0)     ||
		(Q_stricmp(level.mapname, "boss1") == 0)   ||
		(Q_stricmp(level.mapname, "fact3") == 0)   ||
		(Q_stricmp(level.mapname, "biggun") == 0)  ||
		(Q_stricmp(level.mapname, "space") == 0)   ||
		(Q_stricmp(level.mapname, "command") == 0) ||
		(Q_stricmp(level.mapname, "power2") == 0) ||
		(Q_stricmp(level.mapname, "strike") == 0))
	{
		// invoke one of our gross, ugly, disgusting hacks
		self->think = SP_FixCoopSpots;
		self->nextthink = level.time + 1;
	}
#else
	G_FreeEdict(self);
#endif
}


/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
void SP_info_player_intermission(edict_t *ent)
{
}


//=======================================================================


void player_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	// player pain is handled at the end of the frame in P_DamageFeedback
}


bool IsFemale(edict_t *ent)
{
	char        *info;

	if (!ent->server.client)
		return false;

	info = Info_ValueForKey(ent->server.client->pers.userinfo, "gender");

	if (info[0] == 'f' || info[0] == 'F')
		return true;

	return false;
}

bool IsNeutral(edict_t *ent)
{
	char        *info;

	if (!ent->server.client)
		return false;

	info = Info_ValueForKey(ent->server.client->pers.userinfo, "gender");

	if (info[0] != 'f' && info[0] != 'F' && info[0] != 'm' && info[0] != 'M')
		return true;

	return false;
}

typedef enum
{
	MALE,
	FEMALE,
	NEUTRAL
} gender_e;

typedef enum
{
	ITSELF,
	IT,
	ITS
} gender_verb_e;

const char *gender_verbs[][3] =
{
	{ "himself", "him", "his" },
	{ "herself", "her", "her" },
	{ "itself", "it", "their" }
};

const char *GetGender(edict_t *ent, gender_verb_e verb)
{
	gender_e gender = NEUTRAL;

	if (ent->server.client)
	{
		const char *info = Info_ValueForKey(ent->server.client->pers.userinfo, "gender");

		if (info[0] == 'm' || info[0] == 'M')
			gender = MALE;
		else if (info[0] == 'f' || info[0] == 'F')
			gender = FEMALE;
	}

	return gender_verbs[gender][verb];
}

// Always constant, does not contain a/an or anything.
const char *EntityIDToName(edict_t *ent, entitytype_e type)
{
	if (ent->server.client)
		return ent->server.client->pers.netname;

	switch (type)
	{
		case ET_WORLDSPAWN:
			return "world";

		case ET_FUNC_PLAT:
			return "elevator";

		case ET_FUNC_BUTTON:
			return "button";

		case ET_FUNC_DOOR:
		case ET_FUNC_DOOR_ROTATING:
		case ET_FUNC_DOOR_SECRET:
			return "door";

		case ET_FUNC_ROTATING:
			return "rotating platform";

		case ET_FUNC_TRAIN:
			return "train";

		case ET_FUNC_OBJECT:
			return "object";

		case ET_FUNC_EXPLOSIVE:
			return "explosive";

		case ET_FUNC_KILLBOX:
			return "otherworldly force";

		case ET_TRIGGER_HURT:
			return "bad spot to be standing in";

		case ET_TRIGGER_PUSH:
			return "a heavy gust of wind";

		case ET_TARGET_EXPLOSION:
			return "explosion";

		case ET_TARGET_SPLASH:
			return "spark";

		case ET_TARGET_BLASTER:
			return "stray bolt";

		case ET_TARGET_LASER:
			return "laser";

		case ET_MISC_EXPLOBOX:
			return "explosive barrel";

#ifdef ENABLE_COOP
		case ET_MONSTER_BERSERK:
			return "Berserker";

		case ET_MONSTER_GLADIATOR:
			return "Gladiator";

		case ET_MONSTER_GUNNER:
			return "Gunner";

		case ET_MONSTER_INFANTRY:
			return "Infantry";

		case ET_MONSTER_SOLDIER_LIGHT:
			return "Light Soldier";

		case ET_MONSTER_SOLDIER:
			return "Shotgun Soldier";

		case ET_MONSTER_SOLDIER_SS:
			return "Machinegun Soldier";

		case ET_MONSTER_TANK:
			return "Tank";

		case ET_MONSTER_TANK_COMMANDER:
			return "Tank Commander";

		case ET_MONSTER_MEDIC:
			return "Medic";

		case ET_MONSTER_FLIPPER:
			return "Barracuda Shark";

		case ET_MONSTER_CHICK:
			return "Iron Maiden";

		case ET_MONSTER_PARASITE:
			return "Parasite";

		case ET_MONSTER_FLYER:
			return "Flyer";

		case ET_MONSTER_BRAIN:
			return "Brains";

		case ET_MONSTER_FLOATER:
			return "Technician";

		case ET_MONSTER_HOVER:
			return "Icarus";

		case ET_MONSTER_MUTANT:
			return "Mutant";

		case ET_MONSTER_SUPERTANK:
			return "Supertank";

		case ET_MONSTER_BOSS2:
			return "Hornet";

		case ET_MONSTER_JORG:
			return "JORG";

		case ET_TURRET_BREACH:
		case ET_TURRET_BASE:
		case ET_TURRET_DRIVER:
			return "turret";

		case ET_Q1_MONSTER_ARMY:
			return "Grunt";

		case ET_Q1_MONSTER_DOG:
			return "Rottweiler";

		case ET_Q1_MONSTER_OGRE:
		case ET_Q1_MONSTER_OGRE_MARKSMAN:
			return "Ogre";

		case ET_Q1_MONSTER_KNIGHT:
			return "Knight";

		case ET_Q1_MONSTER_ZOMBIE:
			return "Zombie";

		case ET_Q1_MONSTER_WIZARD:
			return "Scrag";

		case ET_Q1_MONSTER_DEMON:
			return "Fiend";

		case ET_Q1_MONSTER_SHAMBLER:
			return "Shambler";

		case ET_Q1_MONSTER_ENFORCER:
			return "Enforcer";

		case ET_Q1_MONSTER_HELL_KNIGHT:
			return "Hell Knight";

		case ET_Q1_MONSTER_SHALRATH:
			return "Vore";

		case ET_Q1_MONSTER_TARBABY:
			return "Spawn";

		case ET_Q1_MONSTER_FISH:
			return "Rotfish";

		case ET_DOOM_MONSTER_POSS:
			return "Zombieman";

		case ET_DOOM_MONSTER_SPOS:
			return "Shotgun Guy";

		case ET_DOOM_MONSTER_TROO:
			return "Imp";

		case ET_DOOM_MONSTER_SARG:
			return "Demon";

		case ET_DOOM_MONSTER_SPECTRE:
			return "Spectre";

		case ET_DOOM_MONSTER_CPOS:
			return "Heavy Weapon Dude";

		case ET_DOOM_MONSTER_HEAD:
			return "Cacodemon";

		case ET_DOOM_MONSTER_SKUL:
			return "Lost Soul";

		case ET_DOOM_MONSTER_PAIN:
			return "Pain Elemental";

		case ET_DOOM_MONSTER_BSPI:
			return "Arachnotron";

		case ET_DOOM_MONSTER_SKEL:
			return "Revenant";

		case ET_DOOM_MONSTER_BOSS:
			return "Baron of Hell";

		case ET_DOOM_MONSTER_BOS2:
			return "Doom Hell Knight";

		case ET_DOOM_MONSTER_SPID:
			return "Spider Mastermind";

		case ET_DOOM_MONSTER_CYBR:
			return "Cyberdemon";

		case ET_MONSTER_MAKRON:
			return "Makron";
#endif
	}

	return "something";
}

static inline bool Q_isvowel(char c)
{
	return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

const char *GetEntityName(edict_t *ent, entitytype_e type, bool first)
{
	const char *name = EntityIDToName(ent, type);

	if (type == ET_PLAYER)
		return name;

	const char *a = (first ? "A" : "a");
	const char *an = (first ? "An" : "an");
	return va("%s %s", (Q_isvowel(Q_tolower(name[0])) ? an : a), name);
}

#define ATK_MSG_FULL(msg, ...) do { *message_len = Q_snprintf(message, message_max, msg, ##__VA_ARGS__); return true; } while (0)
#define ATK_MSG(msg) ATK_MSG_FULL(msg, whom)

bool ObituaryFigureAttackMessage(edict_t *target, meansOfDeath_t *mod, const char *whom, char *message, size_t message_max, size_t *message_len)
{
	meansOfDeath_e noflags_mean = mod->damage_means & MD_MEANS_MASK;

	// if we were killed by another player's weapon
	switch (noflags_mean)
	{
		case MD_WEAPON:

			// killed by a player-wielded weapon
			switch (mod->attacker_game)
			{
				case GAME_Q2:
					switch (mod->weapon_id)
					{
						case ITI_Q2_BLASTER:
							ATK_MSG("was blasted by %s");
							break;

						case ITI_Q2_SHOTGUN:
							ATK_MSG("was gunned down by %s");
							break;

						case ITI_Q2_SUPER_SHOTGUN:
							ATK_MSG("was blown away by %s's super shotgun");
							break;

						case ITI_Q2_MACHINEGUN:
							ATK_MSG("was machinegunned by %s");
							break;

						case ITI_Q2_CHAINGUN:
							ATK_MSG("was cut in half by %s's chaingun");
							break;

						case ITI_Q2_GRENADE_LAUNCHER:
							if (target == mod->attacker)
								ATK_MSG_FULL("tripped on %s own grenade", GetGender(target, ITS));
							else if (mod->damage_type == DT_DIRECT)
								ATK_MSG("was popped by %s's grenade");
							else
								ATK_MSG("was shredded by %s's shrapnel");

							break;

						case ITI_Q2_GRENADES:
							if (mod->damage_means & MD_HELD)
							{
								if (target == mod->attacker)
									ATK_MSG_FULL("tried to put the pin back in");
								else
									ATK_MSG("felt %s's pain");
							}
							else
							{
								if (target == mod->attacker)
									ATK_MSG_FULL("tripped on %s own grenade", GetGender(target, ITS));
								else if (mod->damage_type == DT_DIRECT)
									ATK_MSG("caught %s's handgrenade");
								else
									ATK_MSG("didn't see %s's handgrenade");
							}

							break;

						case ITI_Q2_ROCKET_LAUNCHER:
							if (target == mod->attacker)
								ATK_MSG_FULL("blew %s up", GetGender(target, ITSELF));
							else if (mod->damage_type == DT_DIRECT)
								ATK_MSG("ate %s's rocket");
							else
								ATK_MSG("couldn't dodge %s's rocket");

							break;

						case ITI_Q2_HYPERBLASTER:
							ATK_MSG("was melted by %s's hyperblaster");
							break;

						case ITI_Q2_RAILGUN:
							if (mod->damage_type == DT_DIRECT)
								ATK_MSG("was railed %s");
							else
								ATK_MSG("was in the path of %s's rail slug");

							break;

						case ITI_Q2_BFG10K:
							if (target == mod->attacker)
								ATK_MSG_FULL("should have used a smaller gun");
							else if (mod->damage_type == DT_DIRECT || mod->damage_type == DT_INDIRECT)
								ATK_MSG("was disintegrated by %s's BFG blast");
							else if (mod->damage_type == DT_LASER)
								ATK_MSG("saw the pretty lights from %s's BFG");
							else
								ATK_MSG("couldn't hide from %s's BFG");

							break;
					}

					if (target == mod->attacker)
						ATK_MSG("becomes bored with life");

					break;

				case GAME_Q1:
					switch (mod->weapon_id)
					{
						case ITI_Q1_AXE:
							ATK_MSG("was ax-murdered by %s");
							break;

						case ITI_Q1_SHOTGUN:
							ATK_MSG("chewed on %s's boomstick");
							break;

						case ITI_Q1_SUPER_SHOTGUN:
							ATK_MSG("ate 2 loads of %s's buckshot");
							break;

						case ITI_Q1_NAILGUN:
							ATK_MSG("was nailed by %s");
							break;

						case ITI_Q1_SUPER_NAILGUN:
							ATK_MSG("was punctured by %s");
							break;

						case ITI_Q1_GRENADE_LAUNCHER:
							if (target == mod->attacker)
								ATK_MSG("tries to put the pin back in");
							else if (target->health <= target->gib_health)
								ATK_MSG("was gibbed by %s's grenade");
							else
								ATK_MSG("ate %s's pineapple");

							break;

						case ITI_Q1_ROCKET_LAUNCHER:
							if (target->health <= target->gib_health)
								ATK_MSG("was gibbed by %s's rocket");
							else
								ATK_MSG("rides %s's rocket");

							break;

						case ITI_Q1_THUNDERBOLT:
							if (target == mod->attacker)
								ATK_MSG("discharges into water");
							else if (mod->damage_type == DT_INDIRECT)
								ATK_MSG("accepts %s's discharge");
							else
								ATK_MSG("accepts %s's shaft");

							break;
					}

					if (target == mod->attacker)
						ATK_MSG("becomes bored with life");

					break;
			}

			break;

		case MD_FALLING:
			ATK_MSG("fell to their death");

		default:
			ATK_MSG("was killed by %s");
			break;
	}

	return false;
}

void ClientObituary(edict_t *self)
{
	/* int         mod;
	 char        *message;
	 char        *message2;
	 bool	    ff;*/
	static char		target_message[MAX_INFO_STRING];
	static char		inflictor_message[MAX_INFO_STRING];
	static char		attacker_message[MAX_INFO_STRING];
	size_t			target_message_len = 0;
	size_t			inflictor_message_len = 0;
	size_t			attacker_message_len = 0;

	if (!self->server.flags.monster && !self->server.client)
		return;

	meansOfDeath_t *mod = &self->meansOfDeath;
	
#ifdef ENABLE_COOP
	if (coop->value && mod->attacker_type == ET_PLAYER)
		self->meansOfDeath.damage_means |= MD_FRIENDLY_FIRE;
#endif

	target_message_len = Q_snprintf(target_message, sizeof(target_message), "%s", GetEntityName(self, self->entitytype, true));

	// "no attacker" is never a valid state.
	if (!mod->attacker || mod->attacker_type == ET_NULL)
		attacker_message_len = Q_snprintf(attacker_message, sizeof(attacker_message), "died");
	else
	{
		// we have an attacker, so let's figure out who killed us
		// and why.
		// if our attacker expired, replace attacker with the object that killed us.
		// This allows, for instance, a platform to become the killer
		// if the last thing that "hurt" you was ages ago/no longer relevant.
		if (mod->attacker_time < level.time)
		{
			mod->attacker = mod->inflictor;
			mod->attacker_type = mod->inflictor->entitytype;
		}

		const char *attacker_name = GetEntityName(mod->attacker, mod->attacker_type, false);

		if (!attacker_name)
			Com_Error(ERR_FATAL, "killed by something with no name");

		// set up attacker message.
		// try direct association
		if (mod->attacker == mod->inflictor || mod->inflictor->server.owner == mod->attacker)
			ObituaryFigureAttackMessage(self, mod, attacker_name, attacker_message, sizeof(attacker_message), &attacker_message_len);
		// see if we were trying to escape them
		else if (mod->attacker != mod->inflictor && mod->inflictor->server.owner != mod->attacker)
		{
			const char *inflictor_name = GetEntityName(mod->inflictor, mod->inflictor->entitytype, false);
			ObituaryFigureAttackMessage(self, mod, inflictor_name, inflictor_message, sizeof(inflictor_message), &inflictor_message_len);
			attacker_message_len = Q_snprintf(attacker_message, sizeof(attacker_message), "while trying escape %s", attacker_name);
		}

		//if (attacker_name)
		//	attacker_message_len = Q_snprintf(attacker_message, sizeof(attacker_message), "was somehow killed by %s", attacker_name);
	}

	// fallback in case we didn't get a detection for some reason
	if (!attacker_message_len)
		attacker_message_len = Q_snprintf(attacker_message, sizeof(attacker_message), "died");

	// put together the string
	if (inflictor_message_len && attacker_message_len)
	{
		// in this case, the inflictor killed us and wasn't connected at all
		// to the attacker.
		gi.bprintf(PRINT_MEDIUM, "%s %s %s.\n", target_message, inflictor_message, attacker_message);
	}
	else
		gi.bprintf(PRINT_MEDIUM, "%s %s.\n", target_message, attacker_message);
}


/*
===============
Touch_Item
===============
*/
void Touch_Backpack(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	bool    taken = false, newweap = false;

	if (!other->server.client)
		return;

	if (other->health < 1)
		return;     // dead people can't pickup

	int max;
#define TRY_AMMO(where) \
	if (where && other->server.client->pers.ammo < (max = GetMaxAmmo(other, CHECK_INVENTORY, CHECK_INVENTORY))) \
	{ \
		other->server.client->pers.ammo = min(max, other->server.client->pers.ammo + where); \
		taken = true; \
	}
	TRY_AMMO(ent->pack_ammo);
#define TRY_WEAPON(index, flag) \
	if ((ent->pack_weapons & flag) && !other->server.client->pers.inventory[index]) \
	{ \
		other->server.client->pers.inventory[index] = 1; \
		taken = true; \
		newweap = true; \
	}
	TRY_WEAPON(ITI_Q1_SUPER_SHOTGUN, IT_Q1_SSHOTGUN);
	TRY_WEAPON(ITI_Q1_NAILGUN, IT_Q1_NAILGUN);
	TRY_WEAPON(ITI_Q1_SUPER_NAILGUN, IT_Q1_SNAILGUN);
	TRY_WEAPON(ITI_Q1_GRENADE_LAUNCHER, IT_Q1_GLAUNCHER);
	TRY_WEAPON(ITI_Q1_ROCKET_LAUNCHER, IT_Q1_RLAUNCHER);
	TRY_WEAPON(ITI_Q1_THUNDERBOLT, IT_Q1_LIGHTNING);

	if (!taken)
		return;

	// flash the screen
	other->server.client->bonus_alpha = 0.25;
	// show icon and name on status bar
	gi.sound(other, CHAN_ITEM, gi.soundindex("%s_ammopickup"), 1, ATTN_NORM, 0);

	if (newweap)
		AttemptBetterWeaponSwap(other);

	G_FreeEdict(ent);
}

//======================================================================

void pack_temp_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->server.owner)
		return;

	Touch_Backpack(ent, other, plane, surf);
}

void pack_make_touchable(edict_t *ent)
{
	ent->touch = Touch_Backpack;
	
#ifdef ENABLE_COOP
	if (deathmatch->value)
#endif
	{
		ent->nextthink = level.time + 29000;
		ent->think = G_FreeEdict;
	}
}

edict_t *Drop_Backpack(edict_t *ent)
{
	edict_t *dropped;
	vec3_t  forward, right;
	vec3_t  offset;
	dropped = G_Spawn();
	dropped->server.state.modelindex = gi.modelindex("models/q1/backpack.mdl");
	dropped->server.state.effects = EF_ROTATE;
	VectorSet(dropped->server.mins, -15, -15, -15);
	VectorSet(dropped->server.maxs, 15, 15, 15);
	dropped->server.solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;
	dropped->touch = pack_temp_touch;
	dropped->server.owner = ent;

	if (ent->server.client)
	{
		dropped->pack_ammo = ent->server.client->pers.ammo;

		if (ent->server.client->pers.inventory[ITI_Q1_SUPER_SHOTGUN])
			dropped->pack_weapons |= IT_Q1_SSHOTGUN;

		if (ent->server.client->pers.inventory[ITI_Q1_NAILGUN])
			dropped->pack_weapons |= IT_Q1_NAILGUN;

		if (ent->server.client->pers.inventory[ITI_Q1_SUPER_NAILGUN])
			dropped->pack_weapons |= IT_Q1_SNAILGUN;

		if (ent->server.client->pers.inventory[ITI_Q1_GRENADE_LAUNCHER])
			dropped->pack_weapons |= IT_Q1_GLAUNCHER;

		if (ent->server.client->pers.inventory[ITI_Q1_ROCKET_LAUNCHER])
			dropped->pack_weapons |= IT_Q1_RLAUNCHER;

		if (ent->server.client->pers.inventory[ITI_Q1_THUNDERBOLT])
			dropped->pack_weapons |= IT_Q1_LIGHTNING;

		trace_t trace;
		AngleVectors(ent->server.client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 0, -16);
		G_ProjectSource(ent->server.state.origin, offset, forward, right, dropped->server.state.origin);
		trace = gi.trace(ent->server.state.origin, dropped->server.mins, dropped->server.maxs,
				dropped->server.state.origin, ent, CONTENTS_SOLID);
		VectorCopy(trace.endpos, dropped->server.state.origin);
	}
	else
	{
		AngleVectors(ent->server.state.angles, forward, right, NULL);
		VectorCopy(ent->server.state.origin, dropped->server.state.origin);
	}

	VectorScale(forward, 100, dropped->velocity);
	dropped->velocity[2] = 300;
	dropped->think = pack_make_touchable;
	dropped->nextthink = level.time + 1000;
	gi.linkentity(dropped);
	return dropped;
}

void Touch_Item(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

void TossClientWeapon(edict_t *self)
{
	gitem_t     *item;
	edict_t     *drop;
	bool        quad;
	float       spread;
	
#ifdef ENABLE_COOP
	if (!deathmatch->value)
		return;
#endif

	switch (self->server.state.game)
	{
		case GAME_Q2:
			item = self->server.client->pers.weapon;

			if (EntityGame(self)->ammo_usages[ITEM_INDEX(item)] <= 0)
				item = NULL;

			if (item && !item->world_model)
				item = NULL;

			if (!dmflags.quad_drop)
				quad = false;
			else
				quad = (self->server.client->quad_time > (level.time + 1000));

			if (item && quad)
				spread = 22.5f;
			else
				spread = 0.0f;

			if (item)
			{
				self->server.client->v_angle[YAW] -= spread;
				drop = Drop_Item(self, item);
				self->server.client->v_angle[YAW] += spread;
				drop->spawnflags = DROPPED_PLAYER_ITEM;
			}

			if (quad)
			{
				self->server.client->v_angle[YAW] += spread;
				drop = Drop_Item(self, GetItemByIndex(ITI_QUAD_DAMAGE));
				self->server.client->v_angle[YAW] -= spread;
				drop->spawnflags |= DROPPED_PLAYER_ITEM;
				drop->touch = Touch_Item;
				drop->nextthink = level.time + (self->server.client->quad_time - level.time);
				drop->think = G_FreeEdict;
			}

			break;

		case GAME_Q1:
			Drop_Backpack(self);
			break;
	}
}


/*
==================
LookAtKiller
==================
*/
void LookAtKiller(edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	vec3_t      dir;

	if (attacker && attacker != world && attacker != self)
		VectorSubtract(attacker->server.state.origin, self->server.state.origin, dir);
	else if (inflictor && inflictor != world && inflictor != self)
		VectorSubtract(inflictor->server.state.origin, self->server.state.origin, dir);
	else
	{
		self->server.client->killer_yaw = self->server.state.angles[YAW];
		return;
	}

	if (dir[0])
		self->server.client->killer_yaw = RAD2DEG(atan2f(dir[1], dir[0]));
	else
	{
		self->server.client->killer_yaw = 0;

		if (dir[1] > 0)
			self->server.client->killer_yaw = 90;
		else if (dir[1] < 0)
			self->server.client->killer_yaw = -90;
	}

	if (self->server.client->killer_yaw < 0)
		self->server.client->killer_yaw += 360;
}

/*
==================
player_die
==================
*/
void body_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

void player_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int     n;
	VectorClear(self->avelocity);
	self->takedamage = true;
	self->movetype = MOVETYPE_TOSS;
	self->server.state.modelindex2 = 0;    // remove linked weapon model
	self->server.state.angles[0] = 0;
	self->server.state.angles[2] = 0;
	self->server.state.sound = 0;
	self->server.client->weapon_sound = 0;
	self->server.maxs[2] = -8;
	self->server.flags.deadmonster = true;
	self->server.state.clip_contents = CONTENTS_DEADMONSTER;
	self->server.client->hold_frame = 0;

	if (!self->deadflag)
	{
		self->server.client->respawn_time = level.time + 1000;
		LookAtKiller(self, inflictor, attacker);
		self->server.client->server.ps.pmove.pm_type = PM_DEAD;
		
#ifdef ENABLE_COOP
		if (deathmatch->value || invasion->value)
#endif
			Cmd_Help_f(self);       // show scores
		
#ifdef ENABLE_COOP
		// clear inventory
		// this is kind of ugly, but it's how we want to handle keys in coop
		if (!invasion->value)
#endif
		{
			TossClientWeapon(self);
			
			for (n = 0; n < game.num_items; n++)
			{
#ifdef ENABLE_COOP
				if (coop->value && (itemlist[n].flags & IT_KEY))
					self->server.client->resp.coop_respawn.inventory[n] = self->server.client->pers.inventory[n];
#endif
			}

			memset(self->server.client->pers.inventory, 0, sizeof(self->server.client->pers.inventory));
		}
	}

	// remove powerups
	self->server.client->quad_time = 0;
	self->server.client->invincible_time = 0;
	self->server.client->breather_time = 0;
	self->server.client->enviro_time = 0;
	self->flags &= ~FL_POWER_ARMOR;

	if (self->freeze_time > level.time)
	{
		MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(TE_DUKE_GLASS);
		MSG_WritePos(self->server.state.origin);
		gi.multicast(self->server.state.origin, MULTICAST_PVS);
		self->server.state.modelindex = 0;
		self->takedamage = false;
		self->server.clipmask = MASK_PLAYERSOLID;
		self->server.solid = SOLID_TRIGGER;
		self->freeze_time = 0;
	}
	else if (self->health <= self->gib_health)
		body_die(self, inflictor, attacker, damage, point);
	else
	{
		// normal death
		if (!self->deadflag)
		{
			static int i;

			switch (self->server.state.game)
			{
				case GAME_Q2:
					i = (i + 1) % 3;
					// start a death animation
					self->server.client->anim_priority = ANIM_DEATH;

					if (self->server.client->server.ps.pmove.pm_flags & PMF_DUCKED)
					{
						self->server.state.frame = FRAME_crdeath1 - 1;
						self->server.client->anim_end = FRAME_crdeath5;
					}
					else
						switch (i)
						{
							case 0:
								self->server.state.frame = FRAME_death101 - 1;
								self->server.client->anim_end = FRAME_death106;
								break;

							case 1:
								self->server.state.frame = FRAME_death201 - 1;
								self->server.client->anim_end = FRAME_death206;
								break;

							case 2:
								self->server.state.frame = FRAME_death301 - 1;
								self->server.client->anim_end = FRAME_death308;
								break;
						}

					gi.sound(self, CHAN_VOICE, gi.soundindex(va("*death%i.wav", (rand() % 4) + 1)), 1, ATTN_NORM, 0);
					break;

				case GAME_Q1:
					self->server.client->anim_priority = ANIM_DEATH;

					if ((self->server.state.frame >= 0 && self->server.state.frame <= 5) ||
						(self->server.state.frame >= 17 && self->server.state.frame <= 28) ||
						(self->server.state.frame >= 29 && self->server.state.frame <= 34) ||
						(self->server.state.frame >= 119 && self->server.state.frame <= 142))
					{
						self->server.state.frame = 41 - 1;
						self->server.client->anim_end = 49;
					}
					else
					{
						i = (i + 1) % 5;

						switch (i)
						{
							case 0:
								self->server.state.frame = 50 - 1;
								self->server.client->anim_end = 60;
								break;

							case 1:
								self->server.state.frame = 61 - 1;
								self->server.client->anim_end = 69;
								break;

							case 2:
								self->server.state.frame = 70 - 1;
								self->server.client->anim_end = 84;
								break;

							case 3:
								self->server.state.frame = 85 - 1;
								self->server.client->anim_end = 93;
								break;

							case 4:
								self->server.state.frame = 94 - 1;
								self->server.client->anim_end = 102;
								break;
						}
					}

					{
						const char *noise;
						int rs = Q_rint((random() * 4) + 1);

						if (rs == 1)
							noise = "q1/player/death1.wav";
						else if (rs == 2)
							noise = "q1/player/death2.wav";
						else if (rs == 3)
							noise = "q1/player/death3.wav";
						else if (rs == 4)
							noise = "q1/player/death4.wav";
						else
							noise = "q1/player/death5.wav";

						gi.sound(self, CHAN_VOICE, gi.soundindex(noise), 1, ATTN_NONE, 0);
					}

					break;

				case GAME_DOOM:
					if (self->health <= (self->gib_health / 2))
						gi.sound(self, CHAN_VOICE, gi.soundindex("doom/PDIEHI.wav"), 1, ATTN_NORM, 0);
					else
						gi.sound(self, CHAN_VOICE, gi.soundindex("doom/PLDETH.wav"), 1, ATTN_NORM, 0);

					self->server.client->anim_priority = ANIM_DEATH;
					self->server.state.frame = 6;
					self->server.client->anim_end = 13;
					self->server.clipmask = MASK_PLAYERSOLID;
					self->server.state.clip_contents = 0;
					self->takedamage = false;
					self->server.solid = SOLID_TRIGGER;
					break;

				case GAME_DUKE:
					gi.sound(self, CHAN_VOICE, gi.soundindex("duke/DMDEATH.wav"), 1, ATTN_NORM, 0);
					self->server.client->anim_priority = ANIM_DEATH;
					self->server.state.frame = 14;
					self->server.client->anim_end = 19;
					self->server.clipmask = MASK_PLAYERSOLID;
					self->server.state.clip_contents = 0;
					self->server.solid = SOLID_TRIGGER;
					break;
			}
		}
	}

	self->deadflag = DEAD_DEAD;
	gi.linkentity(self);
}

//=======================================================================

/*
==============
InitClientPersistant

This is only called when the game first initializes in single player,
but is called after each death and level change in deathmatch
==============
*/
static void InitClientPersistant(edict_t *ent)
{
	gclient_t	*client = ent->server.client;
	memset(&client->pers, 0, sizeof(client->pers));
	client->pers.health         = 100;
	client->pers.max_health     = 100;
	client->pers.connected = true;
}

static void InitClientItems(edict_t *ent)
{
	itemid_e i;

	for (i = ITI_FIRST; i < ITI_TOTAL; ++i)
	{
		int num = GameTypeGame(ent->server.client->pers.game)->item_starts[i];

		if (!num)
			continue;

		ent->server.client->pers.inventory[i] = num;
	}

	i = GameTypeGame(ent->server.client->pers.game)->default_item;

	if (i)
	{
		ent->server.client->pers.selected_item = i;
		ent->server.client->pers.weapon = GetItemByIndex(i);
	}

	ent->server.client->pers.ammo = DEFAULT_MAX_AMMO / 4;
}

void InitClientResp(gclient_t *client)
{
	memset(&client->resp, 0, sizeof(client->resp));
	client->resp.entertime = level.time;
#ifdef ENABLE_COOP
	client->resp.coop_respawn = client->pers;
#endif
}

/*
==================
SaveClientData

Some information that should be persistant, like health,
is still stored in the edict structure, so it needs to
be mirrored out to the client structure before all the
edicts are wiped.
==================
*/
void SaveClientData(void)
{
	int     i;
	edict_t *ent;

	for (i = 0 ; i < game.maxclients ; i++)
	{
		ent = &g_edicts[1 + i];

		if (!ent->server.inuse)
			continue;

		game.clients[i].pers.health = ent->health;
		game.clients[i].pers.max_health = ent->max_health;
#ifdef ENABLE_COOP
		game.clients[i].pers.savedFlags = (ent->flags & (FL_GODMODE | FL_NOTARGET | FL_POWER_ARMOR));

		if (coop->value)
			game.clients[i].pers.score = ent->server.client->resp.score;
#endif
	}
}

void FetchClientEntData(edict_t *ent)
{
	ent->health = ent->server.client->pers.health;
	ent->max_health = ent->server.client->pers.max_health;
#ifdef ENABLE_COOP
	ent->flags |= ent->server.client->pers.savedFlags;

	if (coop->value)
		ent->server.client->resp.score = ent->server.client->pers.score;
#endif
}



/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
PlayersRangeFromSpot

Returns the distance to the nearest player from the given spot
================
*/
float   PlayersRangeFromSpot(edict_t *spot)
{
	edict_t *player;
	float   bestplayerdistance;
	vec3_t  v;
	int     n;
	float   playerdistance;
	bestplayerdistance = 9999999;

	for (n = 1; n <= maxclients->value; n++)
	{
		player = &g_edicts[n];

		if (!player->server.inuse)
			continue;

		if (player->health <= 0)
			continue;

		VectorSubtract(spot->server.state.origin, player->server.state.origin, v);
		playerdistance = VectorLength(v);

		if (playerdistance < bestplayerdistance)
			bestplayerdistance = playerdistance;
	}

	return bestplayerdistance;
}

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point, but NOT the two points closest
to other players
================
*/
edict_t *SelectRandomDeathmatchSpawnPoint(void)
{
	edict_t *spot, *spot1, *spot2;
	int     count = 0;
	int     selection;
	float   range, range1, range2;
	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	while ((spot = G_FindByType(spot, ET_INFO_PLAYER_DEATHMATCH)) != NULL)
	{
		count++;
		range = PlayersRangeFromSpot(spot);

		if (range < range1)
		{
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2)
		{
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return NULL;

	if (count <= 2)
		spot1 = spot2 = NULL;
	else
		count -= 2;

	selection = Q_rand_uniform(count);
	spot = NULL;

	do
	{
		spot = G_FindByType(spot, ET_INFO_PLAYER_DEATHMATCH);

		if (spot == spot1 || spot == spot2)
			selection++;
	}
	while (selection--);

	return spot;
}

/*
================
SelectFarthestDeathmatchSpawnPoint

================
*/
edict_t *SelectFarthestDeathmatchSpawnPoint(void)
{
	edict_t *bestspot;
	float   bestdistance, bestplayerdistance;
	edict_t *spot;
	spot = NULL;
	bestspot = NULL;
	bestdistance = 0;

	while ((spot = G_FindByType(spot, ET_INFO_PLAYER_DEATHMATCH)) != NULL)
	{
		bestplayerdistance = PlayersRangeFromSpot(spot);

		if (bestplayerdistance > bestdistance)
		{
			bestspot = spot;
			bestdistance = bestplayerdistance;
		}
	}

	if (bestspot)
		return bestspot;

	// if there is a player just spawned on each and every start spot
	// we have no choice to turn one into a telefrag meltdown
	spot = G_FindByType(NULL, ET_INFO_PLAYER_DEATHMATCH);
	return spot;
}

edict_t *SelectDeathmatchSpawnPoint(void)
{
	if (dmflags.spawn_farthest)
		return SelectFarthestDeathmatchSpawnPoint();
	else
		return SelectRandomDeathmatchSpawnPoint();
}

#ifdef ENABLE_COOP
edict_t *SelectCoopSpawnPoint(edict_t *ent)
{
	int     index;
	edict_t *spot = NULL;
	char    *target;
	index = ent->server.client - game.clients;

	// player 0 starts in normal player spawn point
	if (!index)
		return NULL;

	spot = NULL;

	// assume there are four coop spots at each spawnpoint
	while (1)
	{
		spot = G_FindByType(spot, ET_INFO_PLAYER_COOP);

		if (!spot)
			return NULL;    // we didn't have enough...

		target = spot->targetname;

		if (!target)
			target = "";

		if (Q_stricmp(game.spawnpoint, target) == 0)
		{
			// this is a coop spawn point for one of the clients here
			index--;

			if (!index)
				return spot;        // this is it
		}
	}
}
#endif


/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, coop start, etc
============
*/
void    SelectSpawnPoint(edict_t *ent, vec3_t origin, vec3_t angles)
{
	edict_t *spot = NULL;
	
#ifdef ENABLE_COOP
	if (deathmatch->value)
#endif
		spot = SelectDeathmatchSpawnPoint();
#ifdef ENABLE_COOP
	else if (coop->value || invasion->value)
		spot = SelectCoopSpawnPoint(ent);
#endif

#ifdef ENABLE_COOP
	// find a single player start spot
	if (!spot)
	{
		while ((spot = G_FindByType(spot, ET_INFO_PLAYER_START)) != NULL)
		{
			if (!game.spawnpoint[0] && !spot->targetname)
				break;

			if (!game.spawnpoint[0] || !spot->targetname)
				continue;

			if (Q_stricmp(game.spawnpoint, spot->targetname) == 0)
				break;
		}
	}
#endif

	if (!spot)
	{
#ifdef ENABLE_COOP
		if (!game.spawnpoint[0])
#endif
		{
			// there wasn't a spawnpoint without a target, so use any
			spot = G_FindByType(spot, ET_INFO_PLAYER_START);
		}

		if (!spot)
			Com_Error(ERR_FATAL, "Couldn't find spawn point %s",
#ifdef ENABLE_COOP
				game.spawnpoint
#else
				""
#endif
			);
	}

	VectorCopy(spot->server.state.origin, origin);
	origin[2] += 9;
	VectorCopy(spot->server.state.angles, angles);
}

//======================================================================

void InitBodyQue(void)
{
	int     i;
	edict_t *ent;
	level.body_que = 0;

	for (i = 0; i < BODY_QUEUE_SIZE ; i++)
	{
		ent = G_Spawn();

		if (!level.body_queue_begin)
			level.body_queue_begin = ent;
	}
}

void body_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

	if (self->health <= self->gib_health)
	{
		switch (self->server.state.game)
		{
			case GAME_Q2:
				gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

				for (n = 0; n < 4; n++)
					ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

				break;

			case GAME_Q1:
				if (random() < 0.5f)
					gi.sound(self, CHAN_BODY, gi.soundindex("q1/player/gib.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound(self, CHAN_BODY, gi.soundindex("q1/player/udeath.wav"), 1, ATTN_NORM, 0);

				ThrowGib(self, "models/q1/gib1.mdl", self->health, GIB_Q1);
				ThrowGib(self, "models/q1/gib2.mdl", self->health, GIB_Q1);
				ThrowGib(self, "models/q1/gib3.mdl", self->health, GIB_Q1);
				break;

			case GAME_DUKE:
				gi.sound(self, CHAN_VOICE, gi.soundindex("duke/SQUISH.wav"), 1, ATTN_NORM, 0);
				ThrowGib(self, "%e_duke_gib1", 0, GIB_DUKE);
				ThrowGib(self, "%e_duke_gib2", 0, GIB_DUKE);
				ThrowGib(self, "%e_duke_gib3", 0, GIB_DUKE);
				ThrowGib(self, "%e_duke_gib4", 0, GIB_DUKE);
				ThrowGib(self, "%e_duke_gib5", 0, GIB_DUKE);
				ThrowGib(self, "%e_duke_gibgun", 0, GIB_DUKE_LG);
				ThrowGib(self, "%e_duke_gibleg", 0, GIB_DUKE_LG);
				ThrowGib(self, "%e_duke_gibtorso", 0, GIB_DUKE_LG);

				for (n = 0; n < 10; ++n)
					ThrowGib(self, "%e_duke_gib6", 0, GIB_DUKE_SM);

				break;
		}

		//self->server.state.origin[2] -= 48;
		ThrowClientHead(self, damage);
		self->takedamage = false;
	}
}

void G_RemoveOutdatedBodies(edict_t *ent)
{
	return;

	for (int i = 0; i < BODY_QUEUE_SIZE; i++)
	{
		edict_t *body = level.body_queue_begin + i;

		if (!body->server.inuse)
			continue;

		int32_t clientnum = body->server.state.skinnum & 0xFF;

		if (clientnum != ent->server.client->server.clientNum)
			continue;

		if (body->server.state.game == ent->server.state.game)
			continue;

		G_FreeEdict(body);
	}
}

void CopyToBodyQue(edict_t *ent)
{
	G_RemoveOutdatedBodies(ent);

	gi.unlinkentity(ent);

	// grab a body que and cycle to the next one
	edict_t *body = &g_edicts[game.maxclients + level.body_que + 1];
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;

	// send an effect on the removed body
	if (body->server.state.modelindex && body->server.state.game == GAME_Q2)
	{
		MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(TE_BLOOD);
		MSG_WritePos(body->server.state.origin);
		MSG_WriteDir(vec3_origin);
		gi.multicast(body->server.state.origin, MULTICAST_PVS);
	}

	gi.unlinkentity(body);
	body->server.state = ent->server.state;
	body->server.state.number = body - g_edicts;
	body->server.state.event = EV_OTHER_TELEPORT;
	body->server.flags = ent->server.flags;
	VectorCopy(ent->server.mins, body->server.mins);
	VectorCopy(ent->server.maxs, body->server.maxs);
	VectorCopy(ent->server.absmin, body->server.absmin);
	VectorCopy(ent->server.absmax, body->server.absmax);
	VectorCopy(ent->server.size, body->server.size);
	VectorCopy(ent->velocity, body->velocity);
	VectorCopy(ent->avelocity, body->avelocity);
	body->server.solid = ent->server.solid;
	body->server.clipmask = ent->server.clipmask;
	body->server.owner = ent->server.owner;
	body->movetype = ent->movetype;
	body->groundentity = ent->groundentity;
	body->takedamage = ent->takedamage;

	if (ent->server.state.game == GAME_Q2 || ent->server.state.game == GAME_Q1 || ent->server.state.game == GAME_DUKE)
	{
		body->gib_health = ent->gib_health;
		body->die = body_die;
		body->takedamage = true;
	}
	else
	{
		body->gib_health = 0;
		body->die = NULL;
		body->takedamage = false;
	}

	gi.linkentity(body);
}

void respawn(edict_t *self)
{
#ifdef ENABLE_COOP
	if (!deathmatch->integer && coop->integer && !invasion->integer)
	{
		gi.AddCommandString("pushmenu loadgame\n");
		return;
	}
#endif

	//JABot[start]
	if (self->ai && self->ai->is_bot)
	{
		BOT_Respawn(self);
		return;
	}

	//JABot[end]

	// spectator's don't leave bodies
	if (self->movetype != MOVETYPE_NOCLIP)
		CopyToBodyQue(self);

	PutClientInServer(self);
	// add a teleportation effect
	self->server.state.event = EV_PLAYER_TELEPORT;
	// hold in place briefly
	self->server.client->server.ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self->server.client->server.ps.pmove.pm_time = 14;
	self->server.client->respawn_time = level.time;
}

/*
 * only called when pers.spectator changes
 * note that resp.spectator should be the opposite of pers.spectator here
 */
void spectator_respawn(edict_t *ent)
{
	int i, numspec;

	// if the user wants to become a spectator, make sure he doesn't
	// exceed max_spectators

	if (ent->server.client->pers.spectator)
	{
		char *value = Info_ValueForKey(ent->server.client->pers.userinfo, "spectator");

		if (*spectator_password->string &&
			strcmp(spectator_password->string, "none") &&
			strcmp(spectator_password->string, value))
		{
			gi.cprintf(ent, PRINT_HIGH, "Spectator password incorrect.\n");
			ent->server.client->pers.spectator = false;
			MSG_WriteByte(svc_stufftext);
			MSG_WriteString("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}

		// count spectators
		for (i = 1, numspec = 0; i <= maxclients->value; i++)
			if (g_edicts[i].server.inuse && game.clients[i - 1].pers.spectator)
				numspec++;

		if (numspec >= maxspectators->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "Server spectator limit is full.");
			ent->server.client->pers.spectator = false;
			// reset his spectator var
			MSG_WriteByte(svc_stufftext);
			MSG_WriteString("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}
	}
	else
	{
		// he was a spectator and wants to join the game
		// he must have the right password
		char *value = Info_ValueForKey(ent->server.client->pers.userinfo, "password");

		if (*password->string && strcmp(password->string, "none") &&
			strcmp(password->string, value))
		{
			gi.cprintf(ent, PRINT_HIGH, "Password incorrect.\n");
			ent->server.client->pers.spectator = true;
			MSG_WriteByte(svc_stufftext);
			MSG_WriteString("spectator 1\n");
			gi.unicast(ent, true);
			return;
		}
	}

	// clear client on respawn
	ent->server.client->resp.score = 0;
#ifdef ENABLE_COOP
	ent->server.client->pers.score = 0;
#endif
	ent->server.flags.noclient = false;
	PutClientInServer(ent);

	// add a teleportation effect
	if (!ent->server.client->pers.spectator)
	{
		// send effect
		G_SendMuzzleFlash(ent, MZ_LOGIN);
		// hold in place briefly
		ent->server.client->server.ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		ent->server.client->server.ps.pmove.pm_time = 14;
	}

	ent->server.client->respawn_time = level.time;

	if (ent->server.client->pers.spectator)
		gi.bprintf(PRINT_HIGH, "%s has moved to the sidelines\n", ent->server.client->pers.netname);
	else
		gi.bprintf(PRINT_HIGH, "%s joined the game\n", ent->server.client->pers.netname);
}

//==============================================================


/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void PutClientInServer(edict_t *ent)
{
	vec3_t  mins = { -16, -16, -24};
	vec3_t  maxs = {16, 16, 32};
	int     index;
	vec3_t  spawn_origin, spawn_angles;
	gclient_t   *client;
	int     i;
	client_persistant_t saved;
	client_respawn_t    resp;
	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	SelectSpawnPoint(ent, spawn_origin, spawn_angles);
	index = ent - g_edicts - 1;
	client = ent->server.client;

	// deathmatch wipes most client data every spawn
#ifdef ENABLE_COOP
	if (deathmatch->value || (invasion->value && (ent->movetype == MOVETYPE_NOCLIP || ent->movetype == MOVETYPE_NONE)))
	{
#endif
		char        userinfo[MAX_INFO_STRING];
		resp = client->resp;
		memcpy(userinfo, client->pers.userinfo, sizeof(userinfo));
		InitClientPersistant(ent);
		ClientUserinfoChanged(ent, userinfo);
		InitClientItems(ent);
#ifdef ENABLE_COOP
	}
	else if (coop->value)
	{
		//      int         n;
		char        userinfo[MAX_INFO_STRING];
		resp = client->resp;
		memcpy(userinfo, client->pers.userinfo, sizeof(userinfo));
		// this is kind of ugly, but it's how we want to handle keys in coop
		//      for (n = 0; n < game.num_items; n++)
		//      {
		//          if (itemlist[n].flags & IT_KEY)
		//              resp.coop_respawn.inventory[n] = client->pers.inventory[n];
		//      }
		resp.coop_respawn.game_helpchanged = client->pers.game_helpchanged;
		resp.coop_respawn.helpchanged = client->pers.helpchanged;
		client->pers = resp.coop_respawn;
		ClientUserinfoChanged(ent, userinfo);

		if (resp.score > client->pers.score)
			client->pers.score = resp.score;
	}
	else
#endif
		memset(&resp, 0, sizeof(resp));

	// clear everything but the persistant data
	saved = client->pers;
	memset(client, 0, sizeof(*client));
	client->pers = saved;
	client->server.clientNum = ent - g_edicts - 1;

	if (client->pers.health <= 0)
	{
		InitClientPersistant(ent);

#ifdef ENABLE_COOP
		if (!invasion->value || ent->movetype != MOVETYPE_WALK)
#endif
			InitClientItems(ent);
	}

	client->resp = resp;
	// copy some data from the client to the entity
	FetchClientEntData(ent);
	// clear entity values
	ent->groundentity = NULL;
	ent->server.client = &game.clients[index];
	ent->takedamage = true;
	ent->movetype = MOVETYPE_WALK;
	ent->viewheight = 22;
	ent->server.inuse = true;
	ent->entitytype = ET_PLAYER;
	ent->mass = 200;
	ent->server.solid = SOLID_BBOX;
	ent->deadflag = DEAD_NO;
	ent->air_finished = level.time + 12;
	ent->server.clipmask = MASK_PLAYERSOLID;
	ent->model = "players/male/tris.md2";
	ent->pain = player_pain;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags &= ~FL_NO_KNOCKBACK;
	ent->server.flags.deadmonster = ent->server.flags.noclient = false;
	ent->server.state.clip_contents = CONTENTS_MONSTER;
	ent->gib_health = -40;
	ent->server.client->old_health = -1;
	VectorCopy(mins, ent->server.mins);
	VectorCopy(maxs, ent->server.maxs);
	VectorClear(ent->velocity);
	// clear playerstate values
	memset(&client->server.ps, 0, sizeof(client->server.ps));
	client->server.ps.pmove.origin[0] = spawn_origin[0] * 8;
	client->server.ps.pmove.origin[1] = spawn_origin[1] * 8;
	client->server.ps.pmove.origin[2] = spawn_origin[2] * 8;

	client->server.ps.fov = atoi(Info_ValueForKey(client->pers.userinfo, "fov"));

	if (client->server.ps.fov < 1)
		client->server.ps.fov = 90;
	else if (client->server.ps.fov > 160)
		client->server.ps.fov = 160;

	if (client->pers.weapon)
		client->server.ps.guns[GUN_MAIN].index = gi.modelindex(client->pers.weapon->view_model);

	client->resp.game = client->pers.game;
	ent->server.state.game = client->pers.game;

	if (ent->server.state.game == GAME_DOOM)
		ent->gib_health = -100;

	// clear entity state values
	ent->server.state.effects = 0;
	ent->server.state.modelindex = MODEL_HANDLE_PLAYER;        // will use the skin specified model

	switch (ent->server.state.game)
	{
		case GAME_Q2:
			ent->server.state.modelindex2 = MODEL_HANDLE_PLAYER;       // custom gun model
			break;

		default:
			ent->server.state.modelindex2 = 0;
			break;
	}

	// sknum is player num and weapon number
	// weapon number will be added in changeweapon
	ent->server.state.skinnum = ent - g_edicts - 1;
	ent->server.state.frame = 0;
	VectorCopy(spawn_origin, ent->server.state.origin);
	ent->server.state.origin[2] += 1;  // make sure off ground
	VectorCopy(ent->server.state.origin, ent->server.state.old_origin);

	// set the delta angle
	for (i = 0 ; i < 3 ; i++)
		client->server.ps.pmove.delta_angles[i] = ANGLE2SHORT(spawn_angles[i] - client->resp.cmd_angles[i]);

	ent->server.state.angles[PITCH] = 0;
	ent->server.state.angles[YAW] = spawn_angles[YAW];
	ent->server.state.angles[ROLL] = 0;
	VectorCopy(ent->server.state.angles, client->server.ps.viewangles);
	VectorCopy(ent->server.state.angles, client->v_angle);

	//JABot[start]
	if (ent->ai && ent->ai->is_bot)
		return;

	//JABot[end]

	// spawn a spectator
	if (client->pers.spectator)
	{
		client->chase_target = NULL;
		client->resp.spectator = true;
		ent->movetype = MOVETYPE_NOCLIP;
		ent->server.solid = SOLID_NOT;
		ent->server.flags.noclient = true;
		memset(ent->server.client->server.ps.guns, 0, sizeof(ent->server.client->server.ps.guns));
		gi.linkentity(ent);
		return;
	}
	else
		client->resp.spectator = false;

	if (!KillBox(ent))
	{
		// could't spawn in?
	}

	gi.linkentity(ent);

	// force the current weapon up
	if (!client->pers.weapon)
		client->pers.weapon = GetBestWeapon(ent, NULL, true);

	client->gunstates[GUN_MAIN].newweapon = client->pers.weapon;
	ChangeWeapon(ent, &client->server.ps.guns[GUN_MAIN], &client->gunstates[GUN_MAIN]);
	
#ifdef ENABLE_COOP
	if (invasion->value)
		ent->server.client->invincible_time = level.time + 3100;
#endif

	if (ent->server.state.game == GAME_DUKE)
	{
		int r = Q_rand() % 4;
		const char *sound;

		switch (r)
		{
			default:
				sound = "duke/letsrk03.wav";
				break;

			case 1:
				sound = "duke/ready2a.wav";
				break;

			case 2:
				sound = "duke/ripem08.wav";
				break;

			case 3:
				sound = "duke/rockin02.wav";
				break;
		}

		gi.sound(ent, CHAN_VOICE, gi.soundindex(sound), 1, ATTN_STATIC, 0);
	}

	client->player_time = level.time;
}

/*
=====================
ClientBeginDeathmatch

A client has just connected to the server in
deathmatch mode, so clear everything out before starting them.
=====================
*/
static void ClientBeginDeathmatch(edict_t *ent)
{
	G_InitEdict(ent);
	InitClientResp(ent->server.client);
	PutClientInServer(ent);
}

#ifdef ENABLE_COOP
/*
=====================
ClientBeginDeathmatch

A client has just connected to the server in
a non-deathmatch mode. They might have a savegame.
=====================
*/
static void ClientBeginNotDeathmatch(edict_t *ent)
{
	ent->server.client->pers.connected = true;

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->server.inuse == true)
	{
		// the client has cleared the client side viewangles upon
		// connecting to the server, which is different than the
		// state when the game is saved, so we need to compensate
		// with deltaangles
		for (int i = 0 ; i < 3 ; i++)
			ent->server.client->server.ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->server.client->server.ps.viewangles[i]);
	}
	else
		// a spawn point will completely reinitialize the entity
		// except for the persistant data that was initialized at
		// ClientConnect() time
		ClientBeginDeathmatch(ent);
}
#endif

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
void ClientBegin(edict_t *ent)
{
#ifdef ENABLE_COOP
	if (deathmatch->value)
#endif
		ClientBeginDeathmatch(ent);
#ifdef ENABLE_COOP
	else
		ClientBeginNonDeathmatch(ent);
#endif

	if (level.intermissiontime)
		MoveClientToIntermission(ent);
	else if (game.maxclients > 1 && !ent->server.client->pers.spectator)
		G_SendMuzzleFlash(ent, MZ_LOGIN);

	gi.bprintf(PRINT_HIGH, "%s entered the game\n", ent->server.client->pers.netname);

	// make sure all view stuff is valid
	ClientEndServerFrame(ent);
}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.

The game can override any of the settings in place
(forcing skins or names, etc) before copying it off.
============
*/

void ClientUserinfoChanged(edict_t *ent, char *userinfo)
{
	char    *s;
	int     playernum;

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
		strcpy(userinfo, "\\name\\badinfo\\skin\\male/grunt");

	// set name
	s = Info_ValueForKey(userinfo, "name");
	strncpy(ent->server.client->pers.netname, s, sizeof(ent->server.client->pers.netname) - 1);
	// wanted game
	s = Info_ValueForKey(userinfo, "gameclass");

	if (!Q_stricmp(s, "q2"))
		ent->server.client->pers.game = GAME_Q2;
	else if (!Q_stricmp(s, "q1"))
		ent->server.client->pers.game = GAME_Q1;
	else if (!Q_stricmp(s, "doom"))
		ent->server.client->pers.game = GAME_DOOM;
	else if (!Q_stricmp(s, "duke"))
		ent->server.client->pers.game = GAME_DUKE;
	else if (!Q_stricmp(s, "random"))
		ent->server.client->pers.game = (gametype_t)(GAME_Q2 + (rand() % ((GAME_TOTAL - 1) - (GAME_NONE + 1) + 1)));
	else
		ent->server.client->pers.game = GAME_NONE;

	// set spectator
	s = Info_ValueForKey(userinfo, "spectator");

	// spectators are only supported in deathmatch
	if ((
#ifdef ENABLE_COOP
		deathmatch->value && 
#endif
		((*s && strcmp(s, "0")) || ent->server.client->pers.game == GAME_NONE))
#ifdef ENABLE_COOP
		|| (invasion->value && !Wave_GameReady())
#endif
		)
	{
		ent->server.client->pers.spectator = true;
		ent->server.client->pers.game = GAME_NONE;
	}
	else
		ent->server.client->pers.spectator = false;

	// set skin
	s = Info_ValueForKey(userinfo, "skin");
	playernum = ent - g_edicts - 1;
	// combine name and skin into a configstring
	gi.configstring(CS_PLAYERSKINS + playernum, va("%s\\%s", ent->server.client->pers.netname, s));

	// fov
	ent->server.client->server.ps.fov = atoi(Info_ValueForKey(userinfo, "fov"));

	if (ent->server.client->server.ps.fov < 1)
		ent->server.client->server.ps.fov = 90;
	else if (ent->server.client->server.ps.fov > 160)
		ent->server.client->server.ps.fov = 160;

	// handedness
	s = Info_ValueForKey(userinfo, "hand");

	if (strlen(s))
		ent->server.client->pers.hand = atoi(s);

	// save off the userinfo in case we want to check something later
	Q_strlcpy(ent->server.client->pers.userinfo, userinfo, sizeof(ent->server.client->pers.userinfo));
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
bool ClientConnect(edict_t *ent, char *userinfo)
{
	char    *value;
	// check to see if they are on the banned IP list
	value = Info_ValueForKey(userinfo, "ip");

	if (SV_FilterPacket(value))
	{
		Info_SetValueForKey(userinfo, "rejmsg", "Banned.");
		return false;
	}

	// check for a spectator
	value = Info_ValueForKey(userinfo, "spectator");

	if (
#ifdef ENABLE_COOP
		deathmatch->value &&
#endif
		*value && strcmp(value, "0"))
	{
		int i, numspec;

		if (*spectator_password->string &&
			strcmp(spectator_password->string, "none") &&
			strcmp(spectator_password->string, value))
		{
			Info_SetValueForKey(userinfo, "rejmsg", "Spectator password required or incorrect.");
			return false;
		}

		// count spectators
		for (i = numspec = 0; i < maxclients->value; i++)
			if (g_edicts[i + 1].server.inuse && game.clients[i].pers.spectator)
				numspec++;

		if (numspec >= maxspectators->value)
		{
			Info_SetValueForKey(userinfo, "rejmsg", "Server spectator limit is full.");
			return false;
		}
	}
	else
	{
		// check for a password
		value = Info_ValueForKey(userinfo, "password");

		if (*password->string && strcmp(password->string, "none") &&
			strcmp(password->string, value))
		{
			Info_SetValueForKey(userinfo, "rejmsg", "Password required or incorrect.");
			return false;
		}
	}

	// they can connect
	ent->server.client = game.clients + (ent - g_edicts - 1);
	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	bool created = false;

	if (ent->server.inuse == false)
	{
		// clear the respawning variables
		InitClientResp(ent->server.client);

		if (
#ifdef ENABLE_COOP
			!game.autosaved ||
#endif
			!ent->server.client->pers.weapon)
		{
			InitClientPersistant(ent);
			created = true;
		}
	}

	ClientUserinfoChanged(ent, userinfo);

	if (created)
		InitClientItems(ent);

	if (game.maxclients > 1)
		Com_Printf("%s connected\n", ent->server.client->pers.netname);

	ent->server.flags.bits = 0; // make sure we start with known default
	ent->server.client->pers.connected = true;
	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will be called between levels.
============
*/
void ClientDisconnect(edict_t *ent)
{
	if (!ent->server.client)
		return;

	// send effect
	if (ent->server.inuse)
		G_SendMuzzleFlash(ent, MZ_LOGOUT);

	gi.unlinkentity(ent);

	bool wasInUse = ent->server.inuse;

	ent->server.inuse = false;
	ent->server.state.modelindex = 0;
	ent->server.state.sound = 0;
	ent->server.state.event = 0;
	ent->server.state.effects = 0;
	ent->server.state.game = 0;
	ent->server.solid = SOLID_NOT;
	ent->entitytype = ET_NULL;
	ent->server.client->pers.connected = false;

	G_RemoveOutdatedBodies(ent);

	if (wasInUse)
		gi.bprintf(PRINT_HIGH, "%s disconnected\n", ent->server.client->pers.netname);

	//JABot[start]
	AI_EnemyRemoved(ent);
	//[end]
}


//==============================================================


edict_t *pm_passent;

// pmove doesn't need to know about passent and contentmask
trace_t q_gameabi PM_trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	return gi.trace(start, mins, maxs, end, pm_passent, (pm_passent->health > 0) ? MASK_PLAYERSOLID : MASK_DEADSOLID);
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void SV_CalcViewOffset(edict_t *ent);
void P_FallingDamage(edict_t *ent, byte msec);

void ClientThink(edict_t *ent, usercmd_t *ucmd)
{
	gclient_t   *client;
	edict_t *other;
	int     i, j;
	pmove_t pm;
	level.current_entity = ent;
	client = ent->server.client;

	if (level.intermissiontime)
	{
		client->server.ps.pmove.pm_type = PM_FREEZE;

		// can exit intermission after five seconds
		if (level.time > level.intermissiontime + 5.0f
			&& (ucmd->buttons & BUTTON_ANY))
			level.exitintermission = true;

		return;
	}

	pm_passent = ent;

	if (ent->server.client->chase_target)
	{
		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);
	}
	else
	{
		// set up for pmove
		memset(&pm, 0, sizeof(pm));

		if (ent->movetype == MOVETYPE_NOCLIP)
			client->server.ps.pmove.pm_type = PM_SPECTATOR;
		else if (ent->deadflag && !ent->server.solid)
			client->server.ps.pmove.pm_type = PM_GIB;
		else if (ent->deadflag)
			client->server.ps.pmove.pm_type = PM_DEAD;
		else if (ent->freeze_time > level.time)
			client->server.ps.pmove.pm_type = PM_DUKE_FROZEN;
		else
			client->server.ps.pmove.pm_type = PM_NORMAL;

		client->server.ps.pmove.gravity = sv_gravity->value;

		if (ent->server.state.game == GAME_DUKE)
			client->server.ps.pmove.gravity *= 1.2f;

		pm.state = client->server.ps.pmove;

		for (i = 0 ; i < 3 ; i++)
		{
			pm.state.origin[i] = ent->server.state.origin[i] * 8;
			pm.state.velocity[i] = ent->velocity[i] * 8;
		}

		if (memcmp(&client->old_pmove, &pm.state, sizeof(pm.state)))
		{
			pm.snapinitial = true;
			//      Com_Printf("pmove changed!\n");
		}

		pm.cmd = *ucmd;
		pm.trace = PM_trace;    // adds default parms
		pm.pointcontents = gi.pointcontents;
		pm.game = ent->server.state.game;
		// perform a pmove
		gi.Pmove(&pm);
		// save results of pmove
		client->server.ps.pmove = pm.state;

		if (ent->server.state.game == GAME_DUKE && !(client->old_pmove.pm_flags & PMF_AWAIT_JUMP) && (client->server.ps.pmove.pm_flags & PMF_AWAIT_JUMP))
		{
			ent->server.client->fall_value = 14;
			SV_CalcViewOffset(ent);
		}

		client->old_pmove = pm.state;

		for (i = 0 ; i < 3 ; i++)
		{
			ent->server.state.origin[i] = pm.state.origin[i] * 0.125f;
			ent->velocity[i] = pm.state.velocity[i] * 0.125f;
		}

		VectorCopy(pm.mins, ent->server.mins);
		VectorCopy(pm.maxs, ent->server.maxs);

		if (ent->freeze_time <= level.time)
		{
			client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
			client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
			client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);
		}

		if (ent->groundentity && !pm.groundentity && (pm.cmd.upmove >= 10) && (pm.waterlevel == 0))
		{
			switch (ent->server.state.game)
			{
				case GAME_Q2:
					gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
					break;

				case GAME_Q1:
					gi.sound(ent, CHAN_VOICE, gi.soundindex("q1/player/plyrjmp8.wav"), 1, ATTN_NORM, 0);
					break;
			}
			
#ifdef ENABLE_COOP
			PlayerNoise(ent, ent->server.state.origin, PNOISE_SELF);
#endif
		}

		ent->viewheight = pm.viewheight;
		ent->waterlevel = pm.waterlevel;
		ent->watertype = pm.watertype;
		ent->groundentity = pm.groundentity;

		if (pm.groundentity)
			ent->groundentity_linkcount = pm.groundentity->server.linkcount;

		if (ent->deadflag)
		{
			client->server.ps.viewangles[ROLL] = 40;
			client->server.ps.viewangles[PITCH] = -15;
			client->server.ps.viewangles[YAW] = client->killer_yaw;
		}
		else if (client->server.ps.pmove.pm_type != PM_DUKE_FROZEN)
		{
			VectorCopy(pm.viewangles, client->v_angle);
			VectorCopy(pm.viewangles, client->server.ps.viewangles);
		}

		gi.linkentity(ent);

		if (ent->movetype != MOVETYPE_NOCLIP)
			G_TouchTriggers(ent);

		// touch other objects
		for (i = 0 ; i < pm.numtouch ; i++)
		{
			other = pm.touchents[i];

			for (j = 0 ; j < i ; j++)
				if (pm.touchents[j] == other)
					break;

			if (j != i)
				continue;   // duplicated

			if (!other->touch)
				continue;

			other->touch(other, ent, NULL, NULL);
		}

		// detect hitting the floor
		P_FallingDamage(ent, ucmd->msec);
		VectorCopy(ent->velocity, ent->server.client->oldvelocity);
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// fire weapon from final position if needed
	if (client->latched_buttons & BUTTON_ATTACK)
	{
		if (client->resp.spectator)
		{
			client->latched_buttons = 0;

			if (client->chase_target)
			{
				client->chase_target = NULL;
				client->server.ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
			}
			else
				GetChaseTarget(ent);
		}
		else if (!client->weapon_thunk)
		{
			client->player_time = level.time + game.frametime;
			client->weapon_thunk = true;
			Think_Weapon(ent);
		}
	}

	if (client->resp.spectator)
	{
		if (ucmd->upmove >= 10)
		{
			if (!(client->server.ps.pmove.pm_flags & PMF_JUMP_HELD))
			{
				client->server.ps.pmove.pm_flags |= PMF_JUMP_HELD;

				if (client->chase_target)
					ChaseNext(ent);
				else
					GetChaseTarget(ent);
			}
		}
		else
			client->server.ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
	}

	// update chase cam if being followed
	for (i = 1; i <= maxclients->value; i++)
	{
		other = g_edicts + i;

		if (other->server.inuse && other->server.client->chase_target == ent)
			UpdateChaseCam(other);
	}

	//JABot[start]
	AITools_DropNodes(ent);
	//JABot[end]
	//client->player_time += ucmd->msec;
}


/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void ClientBeginServerFrame(edict_t *ent)
{
	gclient_t   *client;
	int         buttonMask = -1;

	if (level.intermissiontime)
		return;

	client = ent->server.client;
	client->player_time = level.time;

	if (
#ifdef ENABLE_COOP
		deathmatch->value &&
#endif
		client->pers.spectator != client->resp.spectator &&
		(level.time - client->respawn_time) >= 5000)
	{
		spectator_respawn(ent);
		return;
	}

	// run weapon animations if it hasn't been done by a ucmd_t
	if (!client->weapon_thunk && !client->resp.spectator)
		Think_Weapon(ent);
	else
		client->weapon_thunk = false;

	if (ent->deadflag)
	{
		// wait for any button just going down
		if (level.time > client->respawn_time)
		{
			// in deathmatch, only wait for attack button
#ifdef ENABLE_COOP
			if (deathmatch->value)
#endif
				buttonMask = BUTTON_ATTACK;

			if ((client->latched_buttons & buttonMask) ||
				(
#ifdef ENABLE_COOP
					deathmatch->value &&
#endif
					dmflags.force_respawn))
			{
				respawn(ent);
				client->latched_buttons = 0;
			}
		}

		return;
	}

	client->latched_buttons = 0;
}

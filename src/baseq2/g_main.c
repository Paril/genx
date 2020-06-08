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

#if _WIN32
	#define strdup _strdup
#endif

game_locals_t   game;
level_locals_t  level;
game_import_t   gi;
game_export_t   globals;
spawn_temp_t    spawnTemp;

q_modelhandle sm_meat_index;
q_soundhandle snd_fry;

edict_t     *g_edicts;

#ifdef ENABLE_COOP
cvar_t  *deathmatch;
cvar_t  *coop;
cvar_t  *skill;
#endif
cvar_t  *dmflags_cvar;
cvar_t  *fraglimit;
cvar_t  *timelimit;
cvar_t  *password;
cvar_t  *spectator_password;
cvar_t  *needpass;
cvar_t  *maxclients;
cvar_t  *maxspectators;
cvar_t  *maxentities;
cvar_t  *g_select_empty;
cvar_t  *dedicated;

cvar_t  *filterban;

cvar_t  *sv_maxvelocity;
cvar_t  *sv_gravity;

cvar_t  *sv_rollspeed;
cvar_t  *sv_rollangle;
cvar_t  *gun_x;
cvar_t  *gun_y;
cvar_t  *gun_z;

cvar_t  *run_pitch;
cvar_t  *run_roll;
cvar_t  *bob_up;
cvar_t  *bob_pitch;
cvar_t  *bob_roll;

cvar_t  *sv_cheats;

cvar_t  *flood_msgs;
cvar_t  *flood_persecond;
cvar_t  *flood_waitdelay;

cvar_t  *sv_maplist;

// Generations
#ifdef ENABLE_COOP
cvar_t	*invasion;
#endif

dmflags_t dmflags;


cvar_t				*bot_showpath;
cvar_t				*bot_showcombat;
cvar_t				*bot_showsrgoal;
cvar_t				*bot_showlrgoal;
cvar_t				*bot_debugmonster;

void SpawnEntities(const char *mapname, const char *entities, const char *spawnpoint);
void ClientThink(edict_t *ent, usercmd_t *cmd);
bool ClientConnect(edict_t *ent, char *userinfo);
void ClientUserinfoChanged(edict_t *ent, char *userinfo);
void ClientDisconnect(edict_t *ent);
void ClientBegin(edict_t *ent);
void ClientCommand(edict_t *ent);
void WriteGame(const char *filename, bool autosave);
void ReadGame(const char *filename);
void WriteLevel(const char *filename);
void ReadLevel(const char *filename);
void InitGame(void);
void G_RunFrame(void);


//===================================================================


void ShutdownGame(void)
{
	Com_Printf("==== ShutdownGame ====\n");
	Z_FreeTags(TAG_LEVEL);
	Z_FreeTags(TAG_GAME);
}

/*
============
InitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/
void InitGame(void)
{
	Com_Printf("==== InitGame ====\n");
	gun_x = Cvar_Get("gun_x", "0", 0);
	gun_y = Cvar_Get("gun_y", "0", 0);
	gun_z = Cvar_Get("gun_z", "0", 0);
	//FIXME: sv_ prefix is wrong for these
	sv_rollspeed = Cvar_Get("sv_rollspeed", "200", 0);
	sv_rollangle = Cvar_Get("sv_rollangle", "2", 0);
	sv_maxvelocity = Cvar_Get("sv_maxvelocity", "2000", 0);
	sv_gravity = Cvar_Get("sv_gravity", "800", 0);
	// noset vars
	dedicated = Cvar_Get("dedicated", "0", CVAR_NOSET);
	// latched vars
	sv_cheats = Cvar_Get("cheats", "0", CVAR_SERVERINFO | CVAR_LATCH);
	Cvar_Get("gamename", GAMEVERSION, CVAR_SERVERINFO | CVAR_LATCH);
	Cvar_Get("gamedate", __DATE__, CVAR_SERVERINFO | CVAR_LATCH);
	maxclients = Cvar_Get("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
	maxspectators = Cvar_Get("maxspectators", "4", CVAR_SERVERINFO);
#ifdef ENABLE_COOP
	coop = Cvar_Get("coop", "0", CVAR_LATCH);
	invasion = Cvar_Get("invasion", "0", CVAR_LATCH);
	skill = Cvar_Get("skill", "1", CVAR_LATCH);
#endif
	maxentities = Cvar_Get("maxentities", "1024", CVAR_LATCH);
	// change anytime vars
	dmflags_cvar = Cvar_Get("dmflags", "0", CVAR_SERVERINFO);
	fraglimit = Cvar_Get("fraglimit", "0", CVAR_SERVERINFO);
	timelimit = Cvar_Get("timelimit", "0", CVAR_SERVERINFO);
	password = Cvar_Get("password", "", CVAR_USERINFO);
	spectator_password = Cvar_Get("spectator_password", "", CVAR_USERINFO);
	needpass = Cvar_Get("needpass", "0", CVAR_SERVERINFO);
	filterban = Cvar_Get("filterban", "1", 0);
	g_select_empty = Cvar_Get("g_select_empty", "0", CVAR_ARCHIVE);
	run_pitch = Cvar_Get("run_pitch", "0.002", 0);
	run_roll = Cvar_Get("run_roll", "0.005", 0);
	bob_up  = Cvar_Get("bob_up", "0.005", 0);
	bob_pitch = Cvar_Get("bob_pitch", "0.002", 0);
	bob_roll = Cvar_Get("bob_roll", "0.002", 0);
	// flood control
	flood_msgs = Cvar_Get("flood_msgs", "4", 0);
	flood_persecond = Cvar_Get("flood_persecond", "4", 0);
	flood_waitdelay = Cvar_Get("flood_waitdelay", "10", 0);
	// dm map list
	sv_maplist = Cvar_Get("sv_maplist", "", 0);
	// items
	InitItems();

#ifdef ENABLE_COOP
	deathmatch = Cvar_Get("deathmatch", "0", CVAR_LATCH);

	game.helpmessage1[0] = 0;
	game.helpmessage2[0] = 0;
#else
	Cvar_Get("deathmatch", "1", CVAR_LATCH | CVAR_NOSET);
#endif

	// initialize all entities for this game
	game.maxentities = maxentities->value;
	game.maxclients = maxclients->value;
	g_edicts = Z_TagMallocz(game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	game.clients = Z_TagMallocz(game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	globals.pool = (edict_pool_t)
	{
		.edicts = g_edicts,
		.max_edicts = game.maxentities,
		.num_edicts = game.maxclients + 1,
		.edict_size = sizeof(edict_t)
	};
	// initialize all clients for this game
	AI_Init();//JABot
	game.framerate = BASE_FRAMERATE;
	game.frametime = BASE_FRAMETIME;
	game.framediv = 1;
	game.frameseconds = BASE_FRAMETIME_1000;
	game.random_seed = time(NULL);

	dmflags.flags = dmflags_cvar->integer;
	
#ifdef ENABLE_COOP
	if (invasion->integer)
		Wave_Init();
#endif
}


/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
q_exported game_export_t *GetGameAPI(game_import_t *import)
{
	srand(time(NULL));
	gi = *import;
	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;
	globals.WriteGame = WriteGame;
	globals.ReadGame = ReadGame;
	globals.WriteLevel = WriteLevel;
	globals.ReadLevel = ReadLevel;
	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;
	globals.RunFrame = G_RunFrame;
	globals.ServerCommand = ServerCommand;
	globals.pool.edict_size = sizeof(edict_t);
	return &globals;
}

//======================================================================


/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames(void)
{
	int     i;
	edict_t *ent;

	// calc the player views now that all pushing
	// and damage has been added
	for (i = 0 ; i < maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;

		if (!ent->server.inuse || !ent->server.client || !ent->server.client->pers.connected)
			continue;

		ClientEndServerFrame(ent);
	}
}

/*
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
edict_t *CreateTargetChangeLevel(char *map)
{
	edict_t *ent;
	ent = G_Spawn();
	ent->entitytype = ET_TARGET_CHANGELEVEL;
	Q_snprintf(level.nextmap, sizeof(level.nextmap), "%s", map);
	ent->map = level.nextmap;
	return ent;
}

/*
=================
EndDMLevel

The timelimit or fraglimit has been exceeded
=================
*/
void EndDMLevel(void)
{
	edict_t     *ent;
	char *s, *t, *f;
	static const char *seps = " ,\n\r";

	// stay on same level flag
	if (dmflags.same_level)
	{
		BeginIntermission(CreateTargetChangeLevel(level.mapname));
		return;
	}

	// see if it's in the map list
	if (*sv_maplist->string)
	{
		s = strdup(sv_maplist->string);
		f = NULL;
		t = strtok(s, seps);

		while (t != NULL)
		{
			if (Q_stricmp(t, level.mapname) == 0)
			{
				// it's in the list, go to the next one
				t = strtok(NULL, seps);

				if (t == NULL)   // end of list, go to first one
				{
					if (f == NULL) // there isn't a first one, same level
						BeginIntermission(CreateTargetChangeLevel(level.mapname));
					else
						BeginIntermission(CreateTargetChangeLevel(f));
				}
				else
					BeginIntermission(CreateTargetChangeLevel(t));

				free(s);
				return;
			}

			if (!f)
				f = t;

			t = strtok(NULL, seps);
		}

		free(s);
	}

	if (level.nextmap[0]) // go to a specific map
		BeginIntermission(CreateTargetChangeLevel(level.nextmap));
	else    // search for a changelevel
	{
		ent = G_FindByType(NULL, ET_TARGET_CHANGELEVEL);

		if (!ent)
		{
			// the map designer didn't include a changelevel,
			// so create a fake ent that goes back to the same level
			BeginIntermission(CreateTargetChangeLevel(level.mapname));
			return;
		}

		BeginIntermission(ent);
	}
}


/*
=================
CheckNeedPass
=================
*/
void CheckNeedPass(void)
{
	int need;

	// if password or spectator_password has changed, update needpass
	// as needed
	if (password->modified || spectator_password->modified)
	{
		password->modified = spectator_password->modified = false;
		need = 0;

		if (*password->string && Q_stricmp(password->string, "none"))
			need |= 1;

		if (*spectator_password->string && Q_stricmp(spectator_password->string, "none"))
			need |= 2;

		Cvar_UserSet("needpass", va("%d", need));
	}
}

/*
=================
CheckDMRules
=================
*/
void CheckDMRules(void)
{
	int         i;
	gclient_t   *cl;

	if (level.intermissiontime)
		return;

#ifdef ENABLE_COOP
	if (!deathmatch->value)
		return;
#endif

	if (timelimit->value)
	{
		if (level.time >= timelimit->value * 60)
		{
			gi.bprintf(PRINT_HIGH, "Timelimit hit.\n");
			EndDMLevel();
			return;
		}
	}

	if (fraglimit->value)
	{
		for (i = 0 ; i < maxclients->value ; i++)
		{
			cl = game.clients + i;

			if (!g_edicts[i + 1].server.inuse)
				continue;

			if (cl->resp.score >= fraglimit->value)
			{
				gi.bprintf(PRINT_HIGH, "Fraglimit hit.\n");
				EndDMLevel();
				return;
			}
		}
	}
}


/*
=============
ExitLevel
=============
*/
void ExitLevel(void)
{
	int     i;
	edict_t *ent;
	char    command [256];
	//JABot[start] (Disconnect all bots before changing map)
	BOT_RemoveBot("all");
	//[end]
	Q_snprintf(command, sizeof(command), "gamemap \"%s\"\n", level.changemap);
	gi.AddCommandString(command);
	level.changemap = NULL;
	level.exitintermission = 0;
	level.intermissiontime = 0;
	ClientEndServerFrames();

	// clear some things before going to next level
	for (i = 0 ; i < maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;

		if (!ent->server.inuse)
			continue;

		if (ent->health > ent->server.client->pers.max_health)
			ent->health = ent->server.client->pers.max_health;
	}
}

/*
================
G_RunFrame

Advances the world by game.frameseconds seconds
================
*/

void G_RunFrame(void)
{
	int     i;
	edict_t *ent;
	
	level.framenum++;
	level.time += game.frametime;
	
	dmflags.flags = dmflags_cvar->integer;

	// exit intermissions

	if (level.exitintermission)
	{
		ExitLevel();
		return;
	}

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	ent = &g_edicts[0];

	for (i = 0 ; i < globals.pool.num_edicts ; i++, ent++)
	{
		if (!ent->server.inuse)
			continue;

		level.current_entity = ent;

		if (!(ent->server.state.renderfx & (RF_PROJECTILE | RF_BEAM)))
			VectorCopy(ent->server.state.origin, ent->server.state.old_origin);

		// if the ground entity moved, make sure we are still on it
		if (ent->groundentity && (ent->groundentity->server.linkcount != ent->groundentity_linkcount))
		{
			ent->groundentity = NULL;

#if ENABLE_COOP
			if (!(ent->flags & (FL_SWIM | FL_FLY)) && ent->server.flags.monster)
				M_CheckGround(ent);
#endif
		}

		if (i > 0 && i <= maxclients->value)
		{
			ClientBeginServerFrame(ent);

			//JABot[start]
			if (!ent->ai) //jabot092(2)
				//[end]
				continue;
		}

		G_RunEntity(ent);

		if (ent->server.state.renderfx & RF_PROJECTILE)
			VectorCopy(ent->velocity, ent->server.state.old_origin);
	}

	// see if it is time to end a deathmatch
	CheckDMRules();
	// see if needpass needs updated
	CheckNeedPass();
	// build the playerstate_t structures for all players
	ClientEndServerFrames();
	//JABot[start]
	AITools_Frame();	//give think time to AI debug tools
	//[end]
	
#ifdef ENABLE_COOP
	if (invasion->integer)
		Wave_Frame();
#endif
}


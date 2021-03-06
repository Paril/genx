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

#include "server.h"

server_static_t svs;                // persistant server info
server_t        sv;                 // local server

void SV_ClientReset(client_t *client)
{
	if (client->state < cs_connected)
		return;

	// any partially connected client will be restarted
	client->state = cs_connected;
	client->framenum = 1; // frame 0 can't be used
	client->lastframe = -1;
	client->frames_nodelta = 0;
	client->send_delta = 0;
	client->suppress_count = 0;
	memset(&client->lastcmd, 0, sizeof(client->lastcmd));
}

static void resolve_masters(void)
{
#if !USE_CLIENT
	master_t *m;
	time_t now, delta;
	now = time(NULL);
	FOR_EACH_MASTER(m)
	{
		// re-resolve valid address after one day,
		// resolve invalid address after three hours
		delta = m->adr.port ? 24 * 60 * 60 : 3 * 60 * 60;

		if (now < m->last_resolved)
		{
			m->last_resolved = now;
			continue;
		}

		if (now - m->last_resolved < delta)
			continue;

		if (NET_StringToAdr(m->name, &m->adr, PORT_MASTER))
			Com_DPrintf("Master server at %s.\n", NET_AdrToString(&m->adr));
		else
		{
			Com_WPrintf("Couldn't resolve master: %s\n", m->name);
			m->adr.port = 0;
		}

		m->last_resolved = now = time(NULL);
	}
#endif
}

// optionally load the entity string from external source
static void override_entity_string(const char *server)
{
	char *path = map_override_path->string;
	char buffer[MAX_QPATH], *str;
	int len;

	if (!*path)
		return;

	len = Q_concat(buffer, sizeof(buffer), path, server, ".ent", NULL);

	if (len >= sizeof(buffer))
	{
		len = Q_ERR_NAMETOOLONG;
		goto fail1;
	}

	len = SV_LoadFile(buffer, (void **)&str);

	if (!str)
	{
		if (len == Q_ERR_NOENT)
			return;

		goto fail1;
	}

	if (len > MAX_MAP_ENTSTRING)
	{
		len = Q_ERR_FBIG;
		goto fail2;
	}

	Com_Printf("Loaded entity string from %s\n", buffer);
	sv.entitystring = str;
	return;
fail2:
	SV_FreeFile(str);
fail1:
	Com_EPrintf("Couldn't load entity string from %s: %s\n",
		buffer, Q_ErrorString(len));
}


/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.
================
*/
void SV_SpawnServer(mapcmd_t *cmd)
{
	int         i;
	client_t    *client;
	char        *entitystring;
	SCR_BeginLoadingPlaque();           // for local system
	Com_Printf("------- Server Initialization -------\n");
	Com_Printf("SpawnServer: %s\n", cmd->server);
	// everyone needs to reconnect
	FOR_EACH_CLIENT(client)
	{
		SV_ClientReset(client);
	}
	SV_BroadcastCommand("changing map=%s\n", cmd->server);
	SV_SendClientMessages();
	SV_SendAsyncPackets();
	// free current level
	CM_FreeMap(&sv.cm);
	SV_FreeFile(sv.entitystring);
	// wipe the entire per-level structure
	memset(&sv, 0, sizeof(sv));
	sv.spawncount = Q_rand() & 0x7fffffff;
	// set legacy spawncounts
	FOR_EACH_CLIENT(client)
	{
		client->spawncount = sv.spawncount;
	}
	// reset entity counter
	svs.next_entity = 0;
	// save name for levels that don't set message
	Q_strlcpy(sv.configstrings[CS_NAME], cmd->server, CS_SIZE);
	Q_strlcpy(sv.name, cmd->server, sizeof(sv.name));
	Q_strlcpy(sv.mapcmd, cmd->buffer, sizeof(sv.mapcmd));
	sprintf(sv.configstrings[CS_AIRACCEL], "%d", sv_airaccelerate->integer);

	resolve_masters();

	if (cmd->state == ss_game)
	{
		override_entity_string(cmd->server);
		sv.cm = cmd->cm;
		sprintf(sv.configstrings[CS_MAPCHECKSUM], "%d", (int)sv.cm.cache->checksum);
		// set inline model names
		Q_concat(sv.configstrings[CS_PRECACHE + 1], CS_SIZE, "maps/", cmd->server, ".bsp", NULL);
		Q_SetPrecacheBitsetType(sv.precache_bitset, 1, PRECACHE_MODEL);

		for (i = 1; i < sv.cm.cache->nummodels; i++)
		{
			sprintf(sv.configstrings[CS_PRECACHE + 1 + i], "*%d", i);
			Q_SetPrecacheBitsetType(sv.precache_bitset, i + 1, PRECACHE_MODEL);
		}

		entitystring = sv.entitystring ? sv.entitystring : sv.cm.cache->entitystring;
	}
	else
	{
		// no real map
		strcpy(sv.configstrings[CS_MAPCHECKSUM], "0");
		entitystring = "";
	}

	//
	// clear physics interaction links
	//
	SV_ClearWorld();
	//
	// spawn the rest of the entities on the map
	//
	// precache and static commands can be issued during
	// map initialization
	sv.state = ss_loading;
	// load and spawn all other entities
	ge->SpawnEntities(sv.name, entitystring, cmd->spawnpoint);
	// run two frames to allow everything to settle
	ge->RunFrame();
	sv.framenum++;
	ge->RunFrame();
	sv.framenum++;
	// make sure maxclients string is correct
	sprintf(sv.configstrings[CS_MAXCLIENTS], "%d", sv_maxclients->integer);
	// check for a savegame
	SV_CheckForSavegame(cmd);
	// all precaches are complete
	sv.state = cmd->state;
	// set serverinfo variable
	SV_InfoSet("mapname", sv.name);
	SV_InfoSet("port", net_port->string);
	Cvar_SetInteger(sv_running, sv.state, FROM_CODE);
	Cvar_Set("sv_paused", "0");
	EXEC_TRIGGER(sv_changemapcmd);
#if USE_SYSCON
	SV_SetConsoleTitle();
#endif
	SV_BroadcastCommand("reconnect\n");
	Com_Printf("-------------------------------------\n");
}

/*
==============
SV_ParseMapCmd

Parses mapcmd into more C friendly form.
Loads and fully validates the map to make sure server doesn't get killed.
==============
*/
bool SV_ParseMapCmd(mapcmd_t *cmd)
{
	char        expanded[MAX_QPATH];
	char        *s, *ch;
	int         ret;
	size_t      len;
	s = cmd->buffer;

	// skip the end-of-unit flag if necessary
	if (*s == '*')
	{
		s++;
		cmd->endofunit = true;
	}

	// if there is a + in the map, set nextserver to the remainder.
	// we go directly to nextserver because we don't support cinematics.
	ch = strchr(s, '+');

	if (ch)
	{
		s = ch + 1;

		// skip the end-of-unit flag if necessary
		if (*s == '*')
		{
			s++;
			cmd->endofunit = true;
		}
	}

	cmd->server = s;
	// if there is a $, use the remainder as a spawnpoint
	ch = strchr(s, '$');

	if (ch)
	{
		*ch = 0;
		cmd->spawnpoint = ch + 1;
	}
	else
		cmd->spawnpoint = cmd->buffer + strlen(cmd->buffer);

	// now expand and try to load the map
	if (!COM_CompareExtension(s, ".pcx"))
	{
		len = Q_concat(expanded, sizeof(expanded), "pics/", s, NULL);

		if (len >= sizeof(expanded))
			ret = Q_ERR_NAMETOOLONG;
		else
			ret = FS_LoadFile(expanded, NULL);

		cmd->state = ss_pic;
	}
	else
	{
		len = Q_concat(expanded, sizeof(expanded), "maps/", s, ".bsp", NULL);

		if (len >= sizeof(expanded))
			ret = Q_ERR_NAMETOOLONG;
		else
			ret = CM_LoadMap(&cmd->cm, expanded);

		cmd->state = ss_game;
	}

	if (ret < 0)
	{
		Com_Printf("Couldn't load %s: %s\n", expanded, Q_ErrorString(ret));
		return false;
	}

	return true;
}

/*
==============
SV_InitGame

A brand new game has been started.
==============
*/
void SV_InitGame(void)
{
	int     i, entnum;
	edict_t *ent;
	client_t *client;

	if (svs.initialized)
	{
		// cause any connected clients to reconnect
		SV_Shutdown("Server restarted\n", ERR_RECONNECT);
	}
	else
	{
		// make sure the client is down
		CL_Disconnect(ERR_RECONNECT);
		SCR_BeginLoadingPlaque();
		CM_FreeMap(&sv.cm);
		SV_FreeFile(sv.entitystring);
		memset(&sv, 0, sizeof(sv));
	}

	// get any latched variable changes (maxclients, etc)
	Cvar_GetLatchedVars();
#if !USE_CLIENT
	Cvar_Reset(sv_recycle);
#endif

#ifdef ENABLE_COOP
	if (Cvar_VariableInteger("coop") &&
		Cvar_VariableInteger("deathmatch"))
	{
		Com_Printf("Deathmatch and Coop both set, disabling Coop\n");
		Cvar_Set("coop", "0");
	}
	else if (Cvar_VariableInteger("invasion"))
	{
		Cvar_Set("coop", "0");
		Cvar_Set("deathmatch", "0");
	}

	// dedicated servers can't be single player and are usually DM
	// so unless they explicity set coop, force it to deathmatch
	if (COM_DEDICATED)
	{
		if (!Cvar_VariableInteger("coop"))
			Cvar_Set("deathmatch", "1");
	}

	// init clients
	if (Cvar_VariableInteger("deathmatch") || Cvar_VariableInteger("invasion"))
	{
#endif
		if (sv_maxclients->integer <= 1)
			Cvar_SetInteger(sv_maxclients, 8, FROM_CODE);
		else if (sv_maxclients->integer > CLIENTNUM_RESERVED)
			Cvar_SetInteger(sv_maxclients, CLIENTNUM_RESERVED, FROM_CODE);
#ifdef ENABLE_COOP
	}
	else if (Cvar_VariableInteger("coop"))
	{
		if (sv_maxclients->integer <= 1 || sv_maxclients->integer > 4)
			Cvar_Set("maxclients", "4");
	}
	else        // non-deathmatch, non-coop is one player
		Cvar_FullSet("maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH, FROM_CODE);
#endif

	// enable networking
	if (sv_maxclients->integer > 1)
		NET_Config(NET_SERVER);

	svs.client_pool = SV_Mallocz(sizeof(client_t) * sv_maxclients->integer);
	svs.num_entities = sv_maxclients->integer * UPDATE_BACKUP * MAX_PACKET_ENTITIES;
	svs.entities = SV_Mallocz(sizeof(entity_packed_t) * svs.num_entities);
	Cvar_ClampInteger(sv_reserved_slots, 0, sv_maxclients->integer - 1);
#if USE_ZLIB
	svs.z.zalloc = SV_zalloc;
	svs.z.zfree = SV_zfree;

	if (deflateInit2(&svs.z, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
			-MAX_WBITS, 9, Z_DEFAULT_STRATEGY) != Z_OK)
		Com_Error(ERR_FATAL, "%s: deflateInit2() failed", __func__);

#endif
	// init game
	SV_InitGameProgs();
	// send heartbeat very soon
	svs.last_heartbeat = -(HEARTBEAT_SECONDS - 5) * 1000;

	for (i = 0; i < sv_maxclients->integer; i++)
	{
		client = svs.client_pool + i;
		entnum = i + 1;
		ent = EDICT_NUM(entnum);
		ent->state.number = entnum;
		client->edict = ent;
		client->number = i;
	}

	svs.initialized = true;
}


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

#ifndef GAME_H
#define GAME_H

#include "shared/list.h"

//
// game.h -- game dll information visible to server
//

#define GAME_API_VERSION    4 // Generations

// edict->svflags

#define SVF_NOCLIENT            0x00000001  // don't send entity to clients, even if it has effects
#define SVF_DEADMONSTER         0x00000002  // treat as CONTENTS_DEADMONSTER for collision
#define SVF_MONSTER             0x00000004  // treat as CONTENTS_MONSTER for collision

// edict->solid values

typedef enum
{
	SOLID_NOT,          // no interaction with other objects
	SOLID_TRIGGER,      // only touch when inside, after moving
	SOLID_BBOX,         // touch on edge
	SOLID_BSP           // bsp clip, touch on edge
} solid_t;

//===============================================================

#define MAX_ENT_CLUSTERS    16


typedef struct edict_s edict_t;
typedef struct gclient_s gclient_t;

#ifndef GAME_INCLUDE

struct gclient_s
{
	player_state_t  ps;     // communicated by server to clients
	int             ping;
	int             clientNum;

	// the game dll can add anything it wants after
	// this point in the structure
};


struct edict_s
{
	entity_state_t  s;
	struct gclient_s    *client;
	bool       inuse;
	int         linkcount;

	// FIXME: move these fields to a server private sv_entity_t
	list_t      area;               // linked to a division node or leaf

	int         num_clusters;       // if -1, use headnode instead
	int         clusternums[MAX_ENT_CLUSTERS];
	int         headnode;           // unused if num_clusters != -1
	int         areanum, areanum2;

	//================================

	int         svflags;            // SVF_NOCLIENT, SVF_DEADMONSTER, SVF_MONSTER, etc
	vec3_t      mins, maxs;
	vec3_t      absmin, absmax, size;
	solid_t     solid;
	int         clipmask;
	edict_t     *owner;

	// the game dll can add anything it wants after
	// this point in the structure
};

#endif      // GAME_INCLUDE

//===============================================================

//
// functions provided by the main engine
//
typedef struct
{
	// special messages
	void (* q_printf(2, 3) bprintf)(int printlevel, const char *fmt, ...);
	void (*q_printf(3, 4) cprintf)(edict_t *ent, int printlevel, const char *fmt, ...);
	void (* q_printf(2, 3) centerprintf)(edict_t *ent, const char *fmt, ...);
	void (*q_noreturn q_printf(1, 2) error)(error_type_t type, const char *fmt, ...);
	void (*q_printf(1, 2) dprintf)(print_type_t type, const char *fmt, ...);
	void (*sound)(edict_t *ent, int channel, q_soundhandle soundindex, float volume, float attenuation, float timeofs);
	void (*positioned_sound)(vec3_t origin, edict_t *ent, int channel, q_soundhandle soundinedex, float volume, float attenuation, float timeofs);

	// config strings hold all the index strings, the lightstyles,
	// and misc data like the sky definition and cdtrack.
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	void (*configstring)(int num, const char *string);

	// the *index functions create configstrings and some internal server state
	q_modelhandle (*modelindex)(const char *name);
	q_soundhandle (*soundindex)(const char *name);
	q_imagehandle (*imageindex)(const char *name);

	void (*setmodel)(edict_t *ent, const char *name);

	// collision detection
	trace_t (* q_gameabi trace)(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passent, int contentmask);
	int (*pointcontents)(vec3_t point);
	bool (*inPVS)(vec3_t p1, vec3_t p2);
	bool (*inPHS)(vec3_t p1, vec3_t p2);
	void (*SetAreaPortalState)(int portalnum, bool open);
	bool (*AreasConnected)(int area1, int area2);

	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.
	void (*linkentity)(edict_t *ent);
	void (*unlinkentity)(edict_t *ent);     // call before removing an interactive edict
	int (*BoxEdicts)(vec3_t mins, vec3_t maxs, edict_t **list, int maxcount, int areatype);
	void (*Pmove)(pmove_t *pmove);          // player movement code common with client prediction

	// network messaging
	void (*multicast)(vec3_t origin, multicast_t to);
	void (*unicast)(edict_t *ent, bool reliable);
	void (*MSG_WriteChar)(int c);
	void (*MSG_WriteByte)(int c);
	void (*MSG_WriteShort)(int c);
	void (*MSG_WriteUShort)(int c);
	void (*MSG_WriteLong)(int c);
	void (*MSG_WriteFloat)(float f);
	void (*MSG_WriteString)(const char *s);
	void (*MSG_WritePos)(const vec3_t pos);    // some fractional bits
	void (*MSG_WriteDir)(const vec3_t pos);         // single byte encoded, very coarse
	void (*MSG_WriteAngle)(float f);

	// console variable interaction
	cvar_t *(*Cvar_Get)(const char *var_name, const char *value, int flags);
	cvar_t *(*Cvar_UserSet)(const char *var_name, const char *value);
	cvar_t *(*Cvar_Set)(const char *var_name, const char *value);

	// ClientCommand and ServerCommand parameter access
	int (*Cmd_Argc)(void);
	char *(*Cmd_Argv)(int n);
	char *(*Cmd_Args)(void);     // concatenation of all argv >= 1

	// add commands to the server console as if they were typed in
	// for map changing, etc
	void (*AddCommandString)(const char *text);

	// Navigation stuff
	uint32_t (*Nav_NumNodes)();
	void (*Nav_GetNodePosition)(nav_node_id node, vec3_t position);
	nav_node_type_e(*Nav_GetNodeType)(nav_node_id node);
	uint32_t (*Nav_GetNodeNumConnections)(nav_node_id node);
	uint32_t (*Nav_GetNodeConnection)(nav_node_id node, uint32_t connection_id);
	nav_node_id(*Nav_GetClosestNode)(vec3_t position);
	nav_igator_t	*(*Nav_CreateNavigator)();
	void (*Nav_SetNavigatorStartNode)(nav_igator_t *navigator, nav_node_id node);
	void (*Nav_SetNavigatorHeuristicFunction)(nav_igator_t *navigator, nav_igator_heuristic_func function);
	void (*Nav_SetNavigatorClosedNodeFunction)(nav_igator_t *navigator, nav_igator_node_func function);
	void (*Nav_SetNavigatorEndNode)(nav_igator_t *navigator, nav_node_id node);
	uint32_t (*Nav_GetNavigatorPathCount)(nav_igator_t *navigator);
	nav_node_id(*Nav_GetNavigatorPathNode)(nav_igator_t *navigator, uint32_t index);
	bool (*Nav_NavigatorAStar)(nav_igator_t *navigator);
	void (*Nav_DestroyNavigator)(nav_igator_t *navigator);

	// memory
	void    (*Z_Free)(void *ptr);
	void	*(*Z_Realloc)(void *ptr, size_t size);
	void	*(*Z_TagMalloc)(size_t size, memtag_t tag) q_malloc;
	void	*(*Z_TagMallocz)(size_t size, memtag_t tag) q_malloc;
	void    (*Z_FreeTags)(memtag_t tag);

	// filesystem
	int64_t (*FS_FOpenFile)(const char *filename, qhandle_t *f, unsigned mode);
	int64_t	(*FS_Tell)(qhandle_t f);
	int		(*FS_Seek)(qhandle_t f, int64_t offset);
	int64_t (*FS_Length)(qhandle_t f);
	int		(*FS_Read)(void *buffer, size_t len, qhandle_t f);
	int		(*FS_Write)(const void *buffer, size_t len, qhandle_t f);
	int		(*FS_VPrintf)(qhandle_t f, const char *format, va_list args);
	int		(*FS_LoadFileEx)(const char *path, void **buf, unsigned flags, memtag_t tag);
	int     (*FS_FCloseFile)(qhandle_t f);
} game_import_t;

//
// functions exported by the game subsystem
//
typedef struct
{
	int         apiversion;

	// the init function will only be called when a game starts,
	// not each time a level is loaded.  Persistant data for clients
	// and the server can be allocated in init
	void (*Init)(void);
	void (*Shutdown)(void);

	// each new level entered will cause a call to SpawnEntities
	void (*SpawnEntities)(const char *mapname, const char *entstring, const char *spawnpoint);

	// Read/Write Game is for storing persistant cross level information
	// about the world state and the clients.
	// WriteGame is called every time a level is exited.
	// ReadGame is called on a loadgame.
	void (*WriteGame)(const char *filename, bool autosave);
	void (*ReadGame)(const char *filename);

	// ReadLevel is called after the default map information has been
	// loaded with SpawnEntities
	void (*WriteLevel)(const char *filename);
	void (*ReadLevel)(const char *filename);

	bool (*ClientConnect)(edict_t *ent, char *userinfo);
	void (*ClientBegin)(edict_t *ent);
	void (*ClientUserinfoChanged)(edict_t *ent, char *userinfo);
	void (*ClientDisconnect)(edict_t *ent);
	void (*ClientCommand)(edict_t *ent);
	void (*ClientThink)(edict_t *ent, usercmd_t *cmd);

	void (*RunFrame)(void);

	// ServerCommand will be called when an "sv <command>" command is issued on the
	// server console.
	// The game can issue Cmd_Argx commands to get the rest
	// of the parameters
	void (*ServerCommand)(void);

	//
	// global variables shared between game and server
	//

	// The edict array is allocated in the game dll so it
	// can vary in size from one game to another.
	//
	// The size will be fixed when ge->Init() is called
	struct edict_s	*edicts;
	int				edict_size;
	int				num_edicts;     // current number, <= max_edicts
	int				max_edicts;
} game_export_t;

#endif // GAME_H

/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
--------------------------------------------------------------
The ACE Bot is a product of Steve Yeager, and is available from
the ACE Bot homepage, at http://www.axionfx.com/ace.

This program is a modification of the ACE Bot, and is therefore
in NO WAY supported by Steve Yeager.
*/

#include "..\g_local.h"
#include "ai_local.h"


//ACE


typedef struct
{
	edict_t		*ent;

	bool		was_falling;
	int			last_node;

} player_dropping_nodes_t;
static player_dropping_nodes_t	player;


void AI_CategorizePosition(edict_t *ent);


//===========================================================
//
//				EDIT NODES
//
//===========================================================


//==========================================
// AI_AddNode
// Add a node of normal navigation types.
// Not valid to add nodes from entities nor items
//==========================================
static int AI_AddNode(vec3_t origin, int flagsmask)
{
	if (nav.num_nodes + 1 > MAX_NODES)
		return -1;

	if (flagsmask & NODEFLAGS_WATER)
		flagsmask |= NODEFLAGS_FLOAT;

	VectorCopy(origin, nodes[nav.num_nodes].origin);

	if (!(flagsmask & NODEFLAGS_FLOAT))
		AI_DropNodeOriginToFloor(nodes[nav.num_nodes].origin, player.ent);

	//if( !(flagsmask & NODEFLAGS_NOWORLD) ) {	//don't spawn inside solids
	//	trace_t	trace;
	//	trace = gi.trace( nodes[nav.num_nodes].origin, tv(-15, -15, -8), tv(15, 15, 8), nodes[nav.num_nodes].origin, player.ent, MASK_NODESOLID );
	//	if( trace.startsolid )
	//		return -1;
	//}
	nodes[nav.num_nodes].flags = flagsmask;
	nodes[nav.num_nodes].flags |= AI_FlagsForNode(nodes[nav.num_nodes].origin, player.ent);
	//Com_Printf("Dropped Node\n");
	//if( sv_cheats->value )
	Com_Printf("Dropped Node\n");
	nav.num_nodes++;
	return nav.num_nodes - 1; // return the node added
}

//==========================================
// AI_ClientPathMap
// Clients try to create new nodes while walking the map
//==========================================
void AITools_AddBotRoamNode(void)
{
	if (nav.loaded)
		return;

	AI_AddNode(player.ent->s.origin, NODEFLAGS_BOTROAM);
}


//==========================================
// AI_ClientPathMap
// Clients try to create new nodes while walking the map
//==========================================
void AITools_DropNodes(edict_t *ent)
{
	if (nav.loaded)
		return;

	//AI_CategorizePosition (ent);
	//player.ent = ent;
	//AI_PathMap();
}


//==========================================
//
//==========================================
static void AITools_EraseNodes(void)
{
	//Init nodes arrays
	nav.num_nodes = 0;
	memset(nodes, 0, sizeof(nav_node_t) * MAX_NODES);
	memset(pLinks, 0, sizeof(nav_plink_t) * MAX_NODES);
	nav.num_ents = 0;
	memset(nav.ents, 0, sizeof(nav_ents_t) * MAX_EDICTS);
	nav.num_broams = 0;
	memset(nav.broams, 0, sizeof(nav_broam_t) * MAX_BOT_ROAMS);
	nav.num_items = 0;
	memset(nav.items, 0, sizeof(nav_item_t) * MAX_EDICTS);
	nav.loaded = false;
}

void AITools_InitEditnodes(void)
{
	if (nav.loaded)
	{
		AITools_EraseNodes();
		AI_LoadPLKFile(level.mapname);
		//delete everything but nodes
		memset(pLinks, 0, sizeof(nav_plink_t) * MAX_NODES);
		nav.num_ents = 0;
		memset(nav.ents, 0, sizeof(nav_ents_t) * MAX_EDICTS);
		nav.num_broams = 0;
		memset(nav.broams, 0, sizeof(nav_broam_t) * MAX_BOT_ROAMS);
		nav.num_items = 0;
		memset(nav.items, 0, sizeof(nav_item_t) * MAX_EDICTS);
		nav.loaded = false;
	}

	Com_Printf("EDITNODES: on\n");
}

void AITools_InitMakenodes(void)
{
	if (nav.loaded)
		AITools_EraseNodes();

	Com_Printf("EDITNODES: on\n");
}


//-------------------------------------------------------------


//==========================================
// AI_SavePLKFile
// save nodes and plinks to file.
// Only navigation nodes are saved. Item nodes aren't
//==========================================
static bool AI_SavePLKFile(char *mapname)
{
	FILE		*pOut;
	char		filename[MAX_OSPATH];
	int			i;
	int			version = NAV_FILE_VERSION;
	Q_snprintf(filename, sizeof(filename), "%s/%s/%s.%s", AI_MOD_FOLDER, AI_NODES_FOLDER, mapname, NAV_FILE_EXTENSION);
	pOut = fopen(filename, "wb");

	if (!pOut)
		return false;

	fwrite(&version, sizeof(int), 1, pOut);
	fwrite(&nav.num_nodes, sizeof(int), 1, pOut);

	// write out nodes
	for (i = 0; i < nav.num_nodes; i++)
		fwrite(&nodes[i], sizeof(nav_node_t), 1, pOut);

	// write out plinks array
	for (i = 0; i < nav.num_nodes; i++)
		fwrite(&pLinks[i], sizeof(nav_plink_t), 1, pOut);

	fclose(pOut);
	return true;
}


//===========================================================
//
//				EDITOR
//
//===========================================================

int AI_LinkCloseNodes_JumpPass(int start);
//==================
// AITools_SaveNodes
//==================
void AITools_SaveNodes(void)
{
	int newlinks;
	int	jumplinks;

	if (!nav.num_nodes)
	{
		Com_Printf("CGame AITools: No nodes to save\n");
		return;
	}

	//find links
	newlinks = AI_LinkCloseNodes();
	Com_Printf("       : Added %i new links\n", newlinks);
	//find jump links
	jumplinks = AI_LinkCloseNodes_JumpPass(0);
	Com_Printf("       : Added %i new jump links\n", jumplinks);

	if (!AI_SavePLKFile(level.mapname))
		Com_Printf("       : Failed: Couldn't create the nodes file\n");
	else
		Com_Printf("       : Nodes files saved\n");

	//restart navigation
	AITools_EraseNodes();
	AI_InitNavigationData();
}


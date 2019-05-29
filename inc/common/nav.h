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

#ifndef NAV_H
#define NAV_H

//
// nav.h --- navigation routines
//

typedef struct
{
	vec3_t				position;
	uint8_t				type;
	nav_node_id			connections[MAX_NODE_CONNECTIONS];
	uint32_t			num_connections;

	mleaf_t				*leaf;
} nav_node_t;

extern nav_node_t	*loaded_nodes;
extern uint32_t		num_loaded_nodes;
extern uint32_t		num_allocated_nodes;

nav_node_t *Nav_NodeIDToNode(nav_node_id id);
nav_node_id Nav_NodeToNodeID(nav_node_t *node);

typedef enum
{
	NODE_UNDISCOVERED,
	NODE_OPEN,
	NODE_CLOSED
} node_state_t;

typedef struct nav_igator_s
{
	nav_node_t					*start;
	nav_node_t					*goal;
	nav_igator_heuristic_func	heuristic_func;
	nav_igator_heuristic_func	distance_func;
	nav_igator_node_func		close_func;

	nav_node_t					**path;
	uint32_t					path_count;

	// temporary state
	bool						*accessible_nodes;
	struct Node_Ident_s			*node_redirections;
} nav_igator_t;

typedef struct
{
	bool				is_running;
	bool				nodes_loaded;
	bool				debug_mode;
	bool				auto_drop;
	bool				auto_connect;

	nav_node_id			node_selected_a;
	nav_node_id			node_selected_b;
	nav_igator_t		*test_navigator;
	bsp_t				*bsp;
	char				mapname[MAX_QPATH];
} nav_state_t;

extern nav_state_t	nav_debug;

extern cvar_t		*nav_tools;
extern cvar_t		*nav_autodrop;

void Nav_CmInit(void);

void Nav_Save(void);
void Nav_MapLoaded(const char *name, bsp_t *bsp);
void Nav_MapUnloaded(void);

void NavStateDestroyNagivator(void);

void Nav_ToggleAutoDrop(void);
void Nav_ToggleAutoConnect(void);
void Nav_ToggleDebug(void);

nav_node_t *Nav_CreateNode();
bool Nav_ConnectNodes(nav_node_t *from, nav_node_t *to, bool two_way);
bool Nav_UnconnectNodes(nav_node_t *from, nav_node_t *to, bool both);
void Nav_ToggleConnection(nav_node_t *first, nav_node_t *second);
bool Nav_NodeConnectedTo(nav_node_t *from, nav_node_t *to, bool both_ways);

void Nav_DeleteNode(nav_node_t *node);

void Nav_SetNodePosition(nav_node_t *node, vec3_t new_position);
void Nav_SetNodeType(nav_node_t *node, nav_node_type_e type);

nav_node_t *Nav_ClosestNode(vec3_t position, float *distance);

bool Nav_LeavesConnected(mleaf_t *leaf1, mleaf_t *leaf2, int vis);

nav_igator_t *Nav_CreateNavigator();
void Nav_DestroyNavigator(nav_igator_t *navigator);
bool Nav_AStar(nav_igator_t *navigator);

#endif
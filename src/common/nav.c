#include "shared/shared.h"
#include "shared/list.h"
#include "common/cmd.h"
#include "common/common.h"
#include "common/cvar.h"
#include "client/client.h"
#include "common/files.h"
#include "common/bsp.h"
#include "common/math.h"
#include "common/utils.h"
#include "common/mdfour.h"
#include "common/nav.h"
#include <float.h>

nav_state_t		nav_debug;

nav_node_t		*loaded_nodes = NULL;
uint32_t		num_loaded_nodes = 0;
uint32_t		num_allocated_nodes = 0;

cvar_t			*nav_tools;
cvar_t			*nav_autodrop;
cvar_t			*nav_autoconnect;

nav_node_t *Nav_NodeIDToNode(nav_node_id id)
{
	if (id == NAV_NODE_INVALID || id >= num_loaded_nodes)
		return NULL;

	return loaded_nodes + id;
}

nav_node_id Nav_NodeToNodeID(nav_node_t *node)
{
	if (node == NULL)
		return NAV_NODE_INVALID;

	return node - loaded_nodes;
}

double default_heuristic_func(nav_node_id a, nav_node_id b)
{
	return ManhattanDistance(Nav_NodeIDToNode(a)->position, Nav_NodeIDToNode(b)->position);
}

double default_distance_func(nav_node_id a, nav_node_id b)
{
	return Distance(Nav_NodeIDToNode(a)->position, Nav_NodeIDToNode(b)->position);
}

typedef struct Node_Ident_s {
	nav_node_t		*node;
	nav_node_id		id;
} Node_Ident_t;

nav_igator_t *Nav_CreateNavigator()
{
	nav_igator_t *navigator = Z_Malloc(sizeof(nav_igator_t));

	navigator->start = NULL;
	navigator->goal = NULL;
	navigator->heuristic_func = default_heuristic_func;
	navigator->distance_func = default_distance_func;

	navigator->path = NULL;
	navigator->path_count = 0;

	navigator->accessible_nodes = Z_Mallocz(sizeof(bool) * num_loaded_nodes);
	navigator->node_redirections = Z_Mallocz(sizeof(Node_Ident_t) * num_loaded_nodes);

	return navigator;
}

void Nav_DestroyNavigator(nav_igator_t *navigator)
{
	if (navigator->path)
		Z_Free(navigator->path);

	Z_Free(navigator->accessible_nodes);
	Z_Free(navigator->node_redirections);

	Z_Free(navigator);
}

void NavStateDestroyNagivator()
{
	if (nav_debug.test_navigator)
	{
		Nav_DestroyNavigator(nav_debug.test_navigator);
		nav_debug.test_navigator = NULL;
	}
}

void ResetNavState()
{
	NavStateDestroyNagivator();

	memset(&nav_debug, 0, sizeof(nav_debug));

	nav_debug.node_selected_a = nav_debug.node_selected_b = NAV_NODE_INVALID;
}

nav_node_t *Nav_GetClosestNode(vec3_t position)
{
	nav_node_t *closest = NULL;
	float closest_distance = 0;

	for (uint32_t i = 0; i < num_loaded_nodes; ++i)
	{
		float distance = DistanceSquared(position, loaded_nodes[i].position);

		if (!closest || distance < closest_distance)
		{
			closest = &loaded_nodes[i];
			closest_distance = distance;
		}
	}

	return closest;
}

#include "common/astar.h"

float astarPathCostHeuristic(void *start, void *end, void *ctx)
{
	Node_Ident_t *startnode_id = (Node_Ident_t*)start;
	Node_Ident_t *endnode_id = (Node_Ident_t*)end;
	nav_igator_t *navigator = (nav_igator_t*)ctx;

	return navigator->heuristic_func(startnode_id->id, endnode_id->id);
}

void astarNodeNeighbors(ASNeighborList neighbors, void *node, void *ctx)
{
	Node_Ident_t *node_id = (Node_Ident_t*)node;
	nav_igator_t *navigator = (nav_igator_t*)ctx;

	for (uint32_t i = 0; i < node_id->node->num_connections; ++i)
	{
		if (navigator->accessible_nodes[node_id->node->connections[i]])
			ASNeighborListAdd(neighbors, &navigator->node_redirections[node_id->node->connections[i]], navigator->distance_func(node_id->id, node_id->node->connections[i]));
	}
}

int astarNodeComparator(void *node1, void *node2, void *ctx)
{
	Node_Ident_t *startnode_id = (Node_Ident_t*)node1;
	Node_Ident_t *endnode_id = (Node_Ident_t*)node2;

	if (startnode_id->id < endnode_id->id)
		return -1;
	else if (startnode_id->id > endnode_id->id)
		return 1;

	return 0;
}

bool Nav_AStar(nav_igator_t *navigator)
{
	if (!num_loaded_nodes)
		return false;

	if (navigator->path)
	{
		Z_Free(navigator->path);
		navigator->path_count = 0;
	}

	uint32_t start_id = Nav_NodeToNodeID(navigator->start);
	uint32_t goal_id = Nav_NodeToNodeID(navigator->goal);

	for (uint32_t i = 0; i < num_loaded_nodes; ++i)
	{
		navigator->accessible_nodes[i] = navigator->close_func(i);
		navigator->node_redirections[i] = (Node_Ident_t) { Nav_NodeIDToNode(i), i };
	}

	static ASPathNodeSource source;

	source.nodeSize = sizeof(Node_Ident_t);

	source.pathCostHeuristic = astarPathCostHeuristic;

	source.nodeNeighbors = astarNodeNeighbors;

	source.nodeComparator = astarNodeComparator;

	ASPath path = ASPathCreate(&source, navigator, &navigator->node_redirections[start_id], &navigator->node_redirections[goal_id]);

	navigator->path_count = ASPathGetCount(path);
	navigator->path = (nav_node_t**)Z_Malloc(sizeof(nav_node_t*) * navigator->path_count);

	for (uint32_t i = 0; i < navigator->path_count; ++i)
		navigator->path[i] = ((Node_Ident_t*)ASPathGetNode(path, i))->node;

	ASPathDestroy(path);

	return navigator->path_count > 0;
}

void Nav_ToggleAutoDrop()
{
	if (nav_debug.auto_drop != !!nav_autodrop->integer)
	{
		nav_debug.auto_drop = !!nav_autodrop->integer;

		if (nav_debug.auto_drop)
			Com_Printf("Enabled node autodrop.\n");
		else
			Com_Printf("Disabled node autodrop.\n");
		
		nav_debug.node_selected_a = NAV_NODE_INVALID;
		nav_debug.node_selected_b = NAV_NODE_INVALID;
		
		Cvar_SetInteger(nav_autodrop, nav_debug.auto_drop ? 1 : 0, FROM_CODE);
		Cvar_SetInteger(nav_autoconnect, 0, FROM_CODE);
	}
}

void Nav_ToggleAutoConnect()
{
	if (nav_debug.auto_connect != !!nav_autoconnect->integer)
	{
		nav_debug.auto_connect = !!nav_autoconnect->integer;

		if (nav_debug.auto_connect)
			Com_Printf("Enabled node autoconnect.\n");
		else
			Com_Printf("Disabled node autoconnect.\n");
		
		nav_debug.node_selected_a = NAV_NODE_INVALID;
		nav_debug.node_selected_b = NAV_NODE_INVALID;

		Cvar_SetInteger(nav_autoconnect, nav_debug.auto_connect ? 1 : 0, FROM_CODE);
		Cvar_SetInteger(nav_autodrop, 0, FROM_CODE);
	}
}

void Nav_ToggleDebug()
{
	if (nav_debug.debug_mode != !!nav_tools->integer)
	{
		nav_debug.debug_mode = !!nav_tools->integer;

		if (nav_debug.debug_mode)
			Com_Printf("Enabled navigation debug.\n");
		else
			Com_Printf("Disabled navigation debug .\n");

		Cvar_SetInteger(nav_tools, nav_debug.debug_mode ? 1 : 0, FROM_CODE);
	}
}

nav_node_t *Nav_CreateNode()
{
	NavStateDestroyNagivator();

	if (num_loaded_nodes == num_allocated_nodes)
	{
		num_allocated_nodes *= 2;
		loaded_nodes = (nav_node_t*)Z_Realloc(loaded_nodes, sizeof(nav_node_t) * num_allocated_nodes);
	}

	nav_node_t *node = &loaded_nodes[num_loaded_nodes++];
	memset(node, 0, sizeof(*node));
	return node;
}

bool silence_connect = false;

bool Nav_ConnectNodes(nav_node_t *from, nav_node_t *to, bool two_way)
{
	NavStateDestroyNagivator();

	if (two_way)
	{
		silence_connect = true;
		bool connected = Nav_ConnectNodes(from, to, false);
		connected = Nav_ConnectNodes(to, from, false) || connected;
		silence_connect = false;

		if (connected)
			Com_Printf("Connected %i <-> %i\n", Nav_NodeToNodeID(from), Nav_NodeToNodeID(to));

		return true;
	}

	if (from->num_connections >= MAX_NODE_CONNECTIONS)
		Com_Error(ERR_FATAL, "Outta connections, uh oh");

	nav_node_id to_id = Nav_NodeToNodeID(to);

	for (uint32_t i = 0; i < from->num_connections; ++i)
	{
		if (from->connections[i] == to_id)
			return false;
	}

	from->connections[from->num_connections++] = to_id;

	if (!silence_connect)
		Com_Printf("Connected %i -> %i\n", Nav_NodeToNodeID(from), to_id);

	return true;
}

bool Nav_UnconnectNodes(nav_node_t *from, nav_node_t *to, bool both)
{
	NavStateDestroyNagivator();

	if (both)
	{
		bool did = Nav_UnconnectNodes(from, to, false);
		did = Nav_UnconnectNodes(to, from, false) || did;
		return did;
	}

	if (from->num_connections == 0)
		return false;

	uint32_t location = 0;
	nav_node_id to_id = Nav_NodeToNodeID(to);

	for (; location < from->num_connections; ++location)
	{
		if (from->connections[location] == to_id)
			break;
	}

	if (location == from->num_connections)
		return false;
	// best case
	else if (location == from->num_connections - 1)
		from->connections[location] = 0;
	else
	{
		// harder case, have to shift everything
		for (uint32_t shift = location + 1; shift < from->num_connections; ++shift)
			from->connections[shift - 1] = from->connections[shift];
	}

	from->num_connections--;
	return true;
}

void Nav_DeleteNode(nav_node_t *node)
{
	NavStateDestroyNagivator();

	// fix up a/b first
	nav_node_id id = Nav_NodeToNodeID(node);

	if (nav_debug.node_selected_b == id)
		nav_debug.node_selected_b = NAV_NODE_INVALID;
	else if (nav_debug.node_selected_a == id)
	{
		nav_debug.node_selected_a = nav_debug.node_selected_b;
		nav_debug.node_selected_b = NAV_NODE_INVALID;
	}

	// see if we have to shift the nodes
	if (id != num_loaded_nodes)
	{
		// move all the noves after this one back by 1
		memmove(loaded_nodes + id, loaded_nodes + id + 1, sizeof(nav_node_t) * (num_loaded_nodes - id));

		// fix the connections
		for (uint32_t i = 0; i < num_loaded_nodes - 1; ++i)
		{
			for (uint32_t c = 0; c < loaded_nodes[i].num_connections; )
			{
				// if it's us, we have to shift us back
				if (loaded_nodes[i].connections[c] == id)
					Nav_UnconnectNodes(&loaded_nodes[i], node, false);
				else
				{
					if (loaded_nodes[i].connections[c] > id)
						--loaded_nodes[i].connections[c];

					++c;
				}
			}
		}
	}

	num_loaded_nodes--;
}

bool Nav_LeavesConnected(mleaf_t *leaf1, mleaf_t *leaf2, int vis)
{
	byte mask[VIS_MAX_BYTES];
	bsp_t *bsp = nav_debug.bsp;

	if (!bsp) {
		Com_Error(ERR_DROP, "%s: no map loaded", __func__);
	}

	BSP_ClusterVis(bsp, mask, leaf1->cluster, vis);

	if (leaf2->cluster == -1)
		return false;
	if (!Q_IsBitSet(mask, leaf2->cluster))
		return false;
	//if (!CM_AreasConnected(&cl.s.cm, leaf1->area, leaf2->area))
	//return false;        // a door blocks it
	return true;
}

void Nav_SetNodePosition(nav_node_t *node, vec3_t new_position)
{
	NavStateDestroyNagivator();

	VectorCopy(new_position, node->position);
	node->leaf = BSP_PointLeaf(nav_debug.bsp->nodes, new_position);
}

void Nav_SetNodeType(nav_node_t *node, nav_node_type_e type)
{
	NavStateDestroyNagivator();
	node->type = type;
}

nav_node_t *Nav_ClosestNode(vec3_t position, float *distance)
{
	*distance = INFINITY;
	nav_node_t *closest = NULL;

	for (uint32_t i = 0; i < num_loaded_nodes; ++i)
	{
		nav_node_t *node = &loaded_nodes[i];
		float dist_to = Distance(node->position, position);

		if (!closest || dist_to < *distance)
		{
			closest = node;
			*distance = dist_to;
		}
	}

	return closest;
}

bool Nav_NodeConnectedTo(nav_node_t *from, nav_node_t *to, bool both_ways)
{
	if (both_ways)
		return Nav_NodeConnectedTo(from, to, false) && Nav_NodeConnectedTo(to, from, false);

	nav_node_id to_id = Nav_NodeToNodeID(to);

	for (uint32_t c = 0; c < from->num_connections; ++c)
	{
		if (from->connections[c] == to_id)
			return true;
	}

	return false;
}

enum
{
	LOW_TO_HIGH = 0,
	HIGH_TO_LOW = 1,
	BOTH = 2,
	NONE = 3
};

void Nav_ToggleConnection(nav_node_t *first, nav_node_t *second)
{
	// have low always be first
	if (second < first)
	{
		nav_node_t *tmp = first;
		first = second;
		second = tmp;
	}

	uint8_t state = 0;
	bool lth = Nav_NodeConnectedTo(first, second, false);
	bool htl = Nav_NodeConnectedTo(second, first, false);

	if (lth && htl)
		state = BOTH;
	else if (htl)
		state = HIGH_TO_LOW;
	else if (lth)
		state = LOW_TO_HIGH;
	else
		state = NONE;

	Nav_UnconnectNodes(first, second, true);

	uint8_t new_state = (state + 1) % 4;

	switch (new_state)
	{
	case LOW_TO_HIGH:
		Nav_ConnectNodes(first, second, false);
		break;
	case HIGH_TO_LOW:
		Nav_ConnectNodes(second, first, false);
		break;
	case BOTH:
		Nav_ConnectNodes(first, second, true);
		break;
	}
}

void Nav_Save(void)
{
	char buffer[MAX_QPATH];
	
	Q_snprintf(buffer, sizeof(buffer), "%s/nav/", fs_gamedir);
	FS_CreatePath(buffer);

	Q_snprintf(buffer, MAX_QPATH, "nav/%s.nod", nav_debug.mapname);

	qhandle_t f;
	FS_FOpenFile(buffer, &f, FS_MODE_WRITE);

	FS_Write(&num_loaded_nodes, sizeof(num_loaded_nodes), f);

	for (uint32_t i = 0; i < num_loaded_nodes; ++i)
	{
		nav_node_t *node = &loaded_nodes[i];

		FS_Write(node->position, sizeof(node->position), f);
		FS_Write(&node->type, 1, f);
		FS_Write(&node->num_connections, sizeof(node->num_connections), f);
		FS_Write(node->connections, sizeof(*node->connections) * node->num_connections, f);
	}

	FS_FCloseFile(f);

	Com_Printf("Nodes saved.\n");
}

void Nav_MapUnloaded(void)
{
	if (!nav_debug.is_running)
		return;

	Com_Printf("------- Nav_MapUnloaded -------\n");
	nav_debug.is_running = false;
	nav_debug.bsp = NULL;

	Z_Free(loaded_nodes);
	loaded_nodes = NULL;
	num_loaded_nodes = num_allocated_nodes = 0;
}

void Nav_MapLoaded(const char *name, bsp_t *bsp)
{
	if (nav_debug.is_running)
		Nav_MapUnloaded();

	ResetNavState();
	nav_debug.bsp = bsp;
	nav_debug.is_running = true;

	Cvar_SetInteger(nav_autodrop, 0, FROM_CODE);

	Com_Printf("------- Nav_MapLoaded -------\n");

	size_t len = strlen(name);
	memcpy(nav_debug.mapname, bsp->name + 5, len - 9);
	nav_debug.mapname[len - 9] = 0;

	char buffer[MAX_QPATH];
	Q_snprintf(buffer, MAX_QPATH, "nav/%s.nod", nav_debug.mapname);

	if (!FS_FileExists(buffer))
	{
		Com_Printf("No navigation nodes found for %s\n", nav_debug.mapname);

		num_allocated_nodes = 64;
		loaded_nodes = (nav_node_t*)Z_Malloc(sizeof(nav_node_t) * num_allocated_nodes);
		num_loaded_nodes = 0;

		return;
	}

	qhandle_t f;
	FS_FOpenFile(buffer, &f, FS_MODE_READ);

	FS_Read(&num_loaded_nodes, sizeof(num_loaded_nodes), f);

	num_allocated_nodes = num_loaded_nodes;
	loaded_nodes = (nav_node_t*)Z_Malloc(sizeof(nav_node_t) * num_allocated_nodes);

	for (uint32_t i = 0; i < num_loaded_nodes; ++i)
	{
		nav_node_t *node = &loaded_nodes[i];

		FS_Read(node->position, sizeof(node->position), f);
		node->leaf = BSP_PointLeaf(nav_debug.bsp->nodes, node->position);
		FS_Read(&node->type, 1, f);
		FS_Read(&node->num_connections, sizeof(node->num_connections), f);
		FS_Read(node->connections, sizeof(*node->connections) * node->num_connections, f);
	}

	Com_Printf("Loaded %i nodes for %s\n", num_loaded_nodes, nav_debug.mapname);

	FS_FCloseFile(f);

	nav_debug.nodes_loaded = true;
}

void Nav_CmInit()
{
	ResetNavState();

	nav_tools = Cvar_Get("nav_tools", "0", CVAR_LATCH);
	nav_autodrop = Cvar_Get("nav_autodrop", "0", 0);
	nav_autoconnect = Cvar_Get("nav_autoconnect", "0", 0);
}
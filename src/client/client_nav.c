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
// nav.c  -- navigation stuff

#include "client.h"
#include "common/nav.h"
#include "shared/hashset.h"

void Nav_ForceDrop()
{
	nav_node_t *closest;
	float closest_node_dist;

	closest = Nav_ClosestNode(cl.predicted_origin, &closest_node_dist);

	if (closest && closest_node_dist < 24)
	{
		Com_Printf("Picked close node\n");
		nav_debug.node_selected_a = Nav_NodeToNodeID(closest);
		nav_debug.node_selected_b = NAV_NODE_INVALID;
	}
	else
	{
		Com_Printf("Dropped node\n");

		nav_node_t *new_node = Nav_CreateNode();
		VectorCopy(cl.predicted_origin, new_node->position);
		new_node->leaf = BSP_PointLeaf(cl.bsp->nodes, new_node->position);

		if (nav_debug.node_selected_a != NAV_NODE_INVALID)
			Nav_ConnectNodes(Nav_NodeIDToNode(nav_debug.node_selected_a), new_node, true);

		nav_debug.node_selected_a = Nav_NodeToNodeID(new_node);
	}
}

void Nav_Frame()
{
	if (cls.state != ca_active || cls.demo.playback || sv_paused->integer)
		return;
	if (!nav_debug.is_running)
		return;

	Nav_ToggleAutoDrop();
	Nav_ToggleAutoConnect();
	Nav_ToggleDebug();

	if (nav_debug.auto_drop)
	{
		float dist_to_last;
		nav_node_t *closest;
		float closest_node_dist;

		if (nav_debug.node_selected_a == NAV_NODE_INVALID)
			dist_to_last = INFINITY;
		else
			dist_to_last = Distance(Nav_NodeIDToNode(nav_debug.node_selected_a)->position, cl.predicted_origin);

		closest = Nav_ClosestNode(cl.predicted_origin, &closest_node_dist);

		if (closest && nav_debug.node_selected_a != NAV_NODE_INVALID && dist_to_last > 24 && closest_node_dist < 24 && closest != Nav_NodeIDToNode(nav_debug.node_selected_a))
		{
			Com_Printf("Touched old node\n");

			Nav_ConnectNodes(Nav_NodeIDToNode(nav_debug.node_selected_a), closest, true);
			nav_debug.node_selected_a = Nav_NodeToNodeID(closest);
		}
		else if (dist_to_last > 128)
			Nav_ForceDrop();
	}

	if (nav_debug.auto_connect)
	{
		float closest_node_dist;
		nav_node_t *closest = Nav_ClosestNode(cl.predicted_origin, &closest_node_dist);

		if (closest_node_dist < 32.0)
		{
			if (nav_debug.node_selected_a != Nav_NodeToNodeID(closest))
			{
				Com_Printf("Connected two nodes\n");

				if (nav_debug.node_selected_a != NAV_NODE_INVALID)
					Nav_ConnectNodes(Nav_NodeIDToNode(nav_debug.node_selected_a), closest, true);

				nav_debug.node_selected_b = nav_debug.node_selected_a;
				nav_debug.node_selected_a = Nav_NodeToNodeID(closest);
			}
		}
	}
}

static inline uint32_t node_hash(uint16_t node_left, uint16_t node_right)
{
	return min(node_left, node_right) | (max(node_left, node_right) << 16);
}

void Nav_AddEntities()
{
	if (!nav_debug.debug_mode)
		return;

	mleaf_t *leaf = BSP_PointLeaf(cl.bsp->nodes, cl.predicted_origin);

	entity_t ent;
	memset(&ent, 0, sizeof(ent));
	ent.scale = 1;
	ent.game = GAME_Q2;

	nav_node_id selected_node_min = min(nav_debug.node_selected_a, nav_debug.node_selected_b);
	nav_node_id selected_node_max = max(nav_debug.node_selected_a, nav_debug.node_selected_b);

	hashset_t connections_rendered = hashset_create();

	for (uint32_t i = 0; i < num_loaded_nodes; ++i)
	{
		if (!Nav_LeavesConnected(leaf, loaded_nodes[i].leaf, DVIS_PVS))
			continue;

		ent.alpha = 1.0;
		ent.flags = RF_TRANSLUCENT;

		switch (loaded_nodes[i].type)
		{
		case NAV_NODE_NORMAL:
			ent.model = R_RegisterModel("models/objects/grenade2/tris.md2");
			break;
		case NAV_NODE_JUMP:
			ent.model = R_RegisterModel("models/objects/gibs/sm_meat/tris.md2");
			break;
		case NAV_NODE_PLATFORM:
			ent.model = R_RegisterModel("models/objects/gibs/sm_metal/tris.md2");
			break;
		}

		if (nav_debug.test_navigator)
		{
			for (uint32_t nd = 0; nd < nav_debug.test_navigator->path_count; ++nd)
			{
				nav_node_t *path = nav_debug.test_navigator->path[nd];

				if (path == &loaded_nodes[i])
				{
					ent.model = R_RegisterModel("models/objects/grenade/tris.md2");
					break;
				}
			}
		}

		ent.frame = 0;

		VectorCopy(loaded_nodes[i].position, ent.origin);

		if (nav_debug.node_selected_a == i || nav_debug.node_selected_b == i)
		{
			ent.flags |= ((nav_debug.node_selected_a == i) ? RF_SHELL_GREEN : RF_SHELL_BLUE);
			ent.alpha = 0.30;
			V_AddEntity(&ent);

			ent.flags = RF_TRANSLUCENT;
		}

		V_AddEntity(&ent);

		for (uint32_t c = 0; c < loaded_nodes[i].num_connections; ++c)
		{
			nav_node_id connection_id = loaded_nodes[i].connections[c];
			nav_node_t *connection = Nav_NodeIDToNode(connection_id);

			//if (!Nav_LeavesConnected(leaf, connection->leaf, DVIS_PVS))
			//	continue;

			VectorCopy(connection->position, ent.oldorigin);
			ent.alpha = (nav_debug.node_selected_a == i || nav_debug.node_selected_a == connection_id) ? 1.0f : 0.25f;

			uint32_t connection_hash = node_hash(i, connection_id);

			//if (!Nav_NodeConnectedTo(connection, &loaded_nodes[i], false) || i < connection_id)
			if (hashset_is_member(connections_rendered, (void*)(connection_hash + 2)) == -1)
			{
				hashset_add(connections_rendered, (void*)(connection_hash + 2));

				color_t laser_color = { 0 };
				laser_color.u8[3] = (int)(ent.alpha * 255);

				ent.frame = 1;
				ent.flags = RF_TRANSLUCENT | RF_BEAM;

				if (selected_node_min == min(i, connection_id) && selected_node_max == max(i, connection_id))
					ent.frame = 3;

				if (loaded_nodes[i].type == NAV_NODE_JUMP || connection->type == NAV_NODE_JUMP)
					laser_color.u8[2] = 255;
				else if (loaded_nodes[i].type == NAV_NODE_PLATFORM || connection->type == NAV_NODE_PLATFORM)
					laser_color.u8[1] = 255;
				else
					laser_color.u8[0] = 255;

				CL_AddNavLaser(ent.origin, ent.oldorigin, ent.frame, laser_color);
			}

			if (!Nav_NodeConnectedTo(connection, &loaded_nodes[i], true))
			{
				vec3_t sub;
				VectorSubtract(ent.oldorigin, ent.origin, sub);
				float len = VectorNormalize(sub);

				VectorMA(ent.origin, (len / 1000) * (cls.realtime % 1000), sub, ent.origin);
				ent.flags = RF_TRANSLUCENT;
				ent.skinnum = 0;
				ent.frame = 0;
				ent.model = R_RegisterModel("%e_rocket");
				vectoangles2(sub, ent.angles);

				V_AddEntity(&ent);

				VectorCopy(loaded_nodes[i].position, ent.origin);
				VectorClear(ent.angles);
			}
		}
	}

	hashset_destroy(connections_rendered);
}

void Nav_SwitchDir()
{
	if (nav_debug.node_selected_a == NAV_NODE_INVALID || nav_debug.node_selected_b == NAV_NODE_INVALID)
	{
		Com_Printf("Select a line first with nav_select_line\n");
		return;
	}

	Nav_ToggleConnection(&loaded_nodes[nav_debug.node_selected_a], &loaded_nodes[nav_debug.node_selected_b]);
}

void Nav_SelectLine()
{
	trace_t trace;
	vec3_t mins = { -4, -4, -4 }, maxs = { 4, 4, 4 };
	VectorAdd(cl.predicted_origin, mins, mins);
	VectorAdd(cl.predicted_origin, maxs, maxs);

	mnode_t *headnode = CM_HeadnodeForBox(mins, maxs, CONTENTS_SOLID);

	for (uint32_t i = 0; i < num_loaded_nodes; ++i)
	{
		for (uint32_t c = 0; c < loaded_nodes[i].num_connections; ++c)
		{
			nav_node_id connection_id = loaded_nodes[i].connections[c];
			nav_node_t *connection = Nav_NodeIDToNode(connection_id);

			CM_BoxTrace(&trace, loaded_nodes[i].position, connection->position, vec3_origin, vec3_origin, headnode, CONTENTS_SOLID);

			if (trace.fraction < 1.0)
			{
				nav_debug.node_selected_a = min(i, connection_id);
				nav_debug.node_selected_b = max(i, connection_id);
				return;
			}
		}
	}
}

void Nav_SelectNode()
{
	float dist;
	nav_node_t *closest = Nav_ClosestNode(cl.predicted_origin, &dist);

	if (dist > 48)
	{
		nav_debug.node_selected_a = nav_debug.node_selected_b = NAV_NODE_INVALID;
		Com_Printf("Unselected node(s).\n");
		return;
	}

	if (closest)
	{
		nav_debug.node_selected_b = nav_debug.node_selected_a;
		nav_debug.node_selected_a = Nav_NodeToNodeID(closest);
	}
}

void Nav_TestPath()
{
	NavStateDestroyNagivator();
	nav_debug.test_navigator = Nav_CreateNavigator();
	nav_debug.test_navigator->start = &loaded_nodes[nav_debug.node_selected_a];
	nav_debug.test_navigator->goal = &loaded_nodes[nav_debug.node_selected_b];
	Nav_AStar(nav_debug.test_navigator);
}

void Nav_DeleteNodeCmd()
{
	char *which = Cmd_Argv(1);
	nav_node_id *to_delete = NULL;

	if (!*which)
		to_delete = &nav_debug.node_selected_a;
	else
	{
		if (*which == 'a' || *which == '1')
			to_delete = &nav_debug.node_selected_a;
		else if (*which == 'b' || *which == '2')
			to_delete = &nav_debug.node_selected_b;
		else
		{
			Com_Printf("Only a/1 or b/2 should be passed to this. It'll delete A by default always.\n");
			return;
		}
	}

	if (!to_delete || *to_delete == NAV_NODE_INVALID)
		return;

	Nav_DeleteNode(Nav_NodeIDToNode(*to_delete));
}

void Nav_MoveNodeCmd()
{
	char *which = Cmd_Argv(1);
	nav_node_id *to_move = NULL;

	if (!*which)
		to_move = &nav_debug.node_selected_a;
	else
	{
		if (*which == 'a' || *which == '1')
			to_move = &nav_debug.node_selected_a;
		else if (*which == 'b' || *which == '2')
			to_move = &nav_debug.node_selected_b;
		else
		{
			Com_Printf("Only a/1 or b/2 should be passed to this. It'll move A by default always.\n");
			return;
		}
	}

	if (!to_move || *to_move == NAV_NODE_INVALID)
		return;

	Nav_SetNodePosition(Nav_NodeIDToNode(*to_move), cl.predicted_origin);
}

void Nav_SpliceNode()
{
	if (nav_debug.node_selected_a == NAV_NODE_INVALID ||
		nav_debug.node_selected_b == NAV_NODE_INVALID)
	{
		Com_Printf("Need two nodes selected to splice");
		return;
	}

	nav_node_t *new_node = Nav_CreateNode();
	VectorCopy(cl.predicted_origin, new_node->position);
	new_node->leaf = BSP_PointLeaf(cl.bsp->nodes, new_node->position);

	Nav_UnconnectNodes(Nav_NodeIDToNode(nav_debug.node_selected_a), Nav_NodeIDToNode(nav_debug.node_selected_b), true);
	Nav_ConnectNodes(Nav_NodeIDToNode(nav_debug.node_selected_a), new_node, true);
	Nav_ConnectNodes(Nav_NodeIDToNode(nav_debug.node_selected_b), new_node, true);

	nav_debug.node_selected_a = Nav_NodeToNodeID(new_node);
	nav_debug.node_selected_b = NAV_NODE_INVALID;
}

void Nav_SliceNode()
{
	char *which = Cmd_Argv(1);
	nav_node_id *to_move = NULL;

	if (!*which)
		to_move = &nav_debug.node_selected_a;
	else
	{
		if (*which == 'a' || *which == '1')
			to_move = &nav_debug.node_selected_a;
		else if (*which == 'b' || *which == '2')
			to_move = &nav_debug.node_selected_b;
		else
		{
			Com_Printf("Only a/1 or b/2 should be passed to this. It'll move A by default always.\n");
			return;
		}
	}

	if (!to_move || *to_move == NAV_NODE_INVALID)
		return;

	// connect all of the connections from this node to each other
	// so that all of the nodes we were connected to are still
	// joinable.
	// Start from the ones we're connected outbound to
	nav_node_t *to_slice = Nav_NodeIDToNode(*to_move);

	for (uint32_t i = 0; i < to_slice->num_connections; ++i)
		for (uint32_t x = 0; x < to_slice->num_connections; ++x)
			Nav_ConnectNodes(Nav_NodeIDToNode(to_slice->connections[i]), Nav_NodeIDToNode(to_slice->connections[x]), true);

	// TODO: connect inbound-only nodes

	// delete the main node now that we're done with inter-connections
	Nav_DeleteNode(to_slice);
}

void Nav_SetTypeCmd()
{
	char *which = Cmd_Argv(1);
	nav_node_type_e new_type;

	if (nav_debug.node_selected_a == NAV_NODE_INVALID)
		return;

	if (!strcmp(which, "normal"))
		new_type = NAV_NODE_NORMAL;
	else if (!strcmp(which, "jump"))
		new_type = NAV_NODE_JUMP;
	else if (!strcmp(which, "plat"))
		new_type = NAV_NODE_PLATFORM;
	else
	{
		Com_Printf("Bad type; normal, jump, plat\n");
		return;
	}

	Nav_SetNodeType(Nav_NodeIDToNode(nav_debug.node_selected_a), new_type);
}

/*
			VectorAdd(center, surfedge->edge->v[surfedge->vert]->point, center);
*/
#define VERTEX_OF(n) ((firstsurfedge + n)->edge->v[(firstsurfedge + n)->vert]->point)

void CalculateCentroid(const msurfedge_t *firstsurfedge, const int32_t numsurfedges, vec3_t out)
{
	vec3_t s = { 0, 0, 0 };
    vec_t areaTotal = 0.0f;

	vec_t *p1 = VERTEX_OF(0);
	vec_t *p2 = VERTEX_OF(1);

    for (int32_t i = 2; i < numsurfedges; i++)
    {
        vec_t *p3 = VERTEX_OF(i);
        vec3_t edge1;
		VectorSubtract(p3, p1, edge1);
        vec3_t edge2;
		VectorSubtract(p3, p2, edge2);

        vec3_t crossProduct;
		CrossProduct(edge1, edge2, crossProduct);
        vec_t area = VectorLength(crossProduct) / 2;

        s[0] += area * (p1[0] + p2[0] + p3[0])/3;
        s[1] += area * (p1[1] + p2[1] + p3[1])/3;
        s[2] += area * (p1[2] + p2[2] + p3[2])/3;

        areaTotal += area;
        p2 = p3;
    }

	VectorScale(s, 1.0 / areaTotal, out);
}

static inline void GL_DrawNode(mnode_t *node)
{
    mface_t *face, *last = node->firstface + node->numfaces;

    for (face = node->firstface; face < last; face++) {
		if (face->plane->normal[2] != 1.0)
			continue;
		if (face->drawflags & DSURF_PLANEBACK)
			continue;
		
		vec3_t center;
		CalculateCentroid(face->firstsurfedge, face->numsurfedges, center);

		VectorMA(center, 16.0, face->plane->normal, center);

		nav_node_t *new_node = Nav_CreateNode();
		VectorCopy(center, new_node->position);
		new_node->leaf = BSP_PointLeaf(cl.bsp->nodes, new_node->position);
    }
}

static void GL_WorldNode_r(mnode_t *node)
{
	if (!node || !node->plane)
		return;

    GL_WorldNode_r(node->children[0]);
    GL_WorldNode_r(node->children[1]);

	GL_DrawNode(node);
}

void Nav_Generate()
{
	if (num_loaded_nodes) {
		Com_WPrintf("Already have nodes loaded.\n");
		return;
	}

	GL_WorldNode_r(cl.bsp->nodes);
}

void Nav_Init()
{
	Com_Printf("------- Nav_Init -------\n");

	Cmd_AddCommand("nav_path_test", Nav_TestPath);
	Cmd_AddCommand("nav_node_drop", Nav_ForceDrop);
	Cmd_AddCommand("nav_line_select", Nav_SelectLine);
	Cmd_AddCommand("nav_line_dir", Nav_SwitchDir);
	Cmd_AddCommand("nav_save", Nav_Save);
	Cmd_AddCommand("nav_node_telect", Nav_SelectNode);
	Cmd_AddCommand("nav_node_delete", Nav_DeleteNodeCmd);
	Cmd_AddCommand("nav_node_tplice", Nav_SpliceNode);
	Cmd_AddCommand("nav_node_tlice", Nav_SliceNode);
	Cmd_AddCommand("nav_node_move", Nav_MoveNodeCmd);
	Cmd_AddCommand("nav_node_tettype", Nav_SetTypeCmd);
	Cmd_AddCommand("nav_generate", Nav_Generate);
}

void Nav_Shutdown()
{
	Com_Printf("------- Nav_Shutdown -------\n");

	if (nav_debug.is_running)
		Nav_MapUnloaded();
}
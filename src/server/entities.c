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

/*
=============================================================================

Encode a client frame onto the network channel

=============================================================================
*/

#define Q2PRO_OPTIMIZE(c) true

/*
=============
SV_EmitPacketEntities

Writes a delta update of an entity_packed_t list to the message.
=============
*/
static void SV_EmitPacketEntities(client_t         *client,
	client_frame_t   *from,
	client_frame_t   *to,
	int              clientEntityNum)
{
	entity_packed_t *newent;
	const entity_packed_t *oldent;
	int i, oldnum, newnum, oldindex, newindex, from_num_entities;
	msgEsFlags_t flags;

	if (!from)
		from_num_entities = 0;
	else
		from_num_entities = from->num_entities;

	newindex = 0;
	oldindex = 0;
	oldent = newent = NULL;

	while (newindex < to->num_entities || oldindex < from_num_entities)
	{
		if (newindex >= to->num_entities)
			newnum = 9999;
		else
		{
			i = (to->first_entity + newindex) % svs.num_entities;
			newent = &svs.entities[i];
			newnum = newent->number;
		}

		if (oldindex >= from_num_entities)
			oldnum = 9999;
		else
		{
			i = (from->first_entity + oldindex) % svs.num_entities;
			oldent = &svs.entities[i];
			oldnum = oldent->number;
		}

		if (newnum == oldnum)
		{
			// Delta update from old position. Because the force parm is false,
			// this will not result in any bytes being emitted if the entity has
			// not changed at all. Note that players are always 'newentities',
			// this updates their old_origin always and prevents warping in case
			// of packet loss.
			flags = client->esFlags;

			if (newnum <= client->maxclients)
				flags |= MSG_ES_NEWENTITY;

			if (newnum == clientEntityNum)
			{
				flags |= MSG_ES_FIRSTPERSON;
				VectorCopy(oldent->origin, newent->origin);
				VectorCopy(oldent->angles, newent->angles);
			}

			if (Q2PRO_SHORTANGLES(client, newnum))
				flags |= MSG_ES_SHORTANGLES;

			MSG_WriteDeltaEntity(oldent, newent, flags);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{
			// this is a new entity, send it from the baseline
			flags = client->esFlags | MSG_ES_FORCE | MSG_ES_NEWENTITY;
			oldent = client->baselines[newnum >> SV_BASELINES_SHIFT];

			if (oldent)
				oldent += (newnum & SV_BASELINES_MASK);
			else
				oldent = &nullEntityState;

			if (newnum == clientEntityNum)
			{
				flags |= MSG_ES_FIRSTPERSON;
				VectorCopy(oldent->origin, newent->origin);
				VectorCopy(oldent->angles, newent->angles);
			}

			if (Q2PRO_SHORTANGLES(client, newnum))
				flags |= MSG_ES_SHORTANGLES;

			MSG_WriteDeltaEntity(oldent, newent, flags);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{
			// the old entity isn't present in the new message
			MSG_WriteDeltaEntity(oldent, NULL, MSG_ES_FORCE);
			oldindex++;
			continue;
		}
	}

	MSG_WriteShort(0);      // end of packetentities
}

static client_frame_t *get_last_frame(client_t *client)
{
	client_frame_t *frame;

	if (client->lastframe <= 0)
	{
		// client is asking for a retransmit
		client->frames_nodelta++;
		return NULL;
	}

	client->frames_nodelta = 0;

	if (client->framenum - client->lastframe >= UPDATE_BACKUP)
	{
		// client hasn't gotten a good message through in a long time
		Com_DPrintf("%s: delta request from out-of-date packet.\n", client->name);
		return NULL;
	}

	// we have a valid message to delta from
	frame = &client->frames[client->lastframe & UPDATE_MASK];

	if (frame->number != client->lastframe)
	{
		// but it got never sent
		Com_DPrintf("%s: delta request from dropped frame.\n", client->name);
		return NULL;
	}

	if (svs.next_entity - frame->first_entity > svs.num_entities)
	{
		// but entities are too old
		Com_DPrintf("%s: delta request from out-of-date entities.\n", client->name);
		return NULL;
	}

	return frame;
}

/*
==================
SV_WriteFrameToClient_Enhanced
==================
*/
void SV_WriteFrameToClient_Enhanced(client_t *client)
{
	client_frame_t  *frame, *oldframe;
	player_packed_t *oldstate;
	uint32_t        extraflags, delta;
	int             suppressed;
	byte            *b1, *b2;
	msgPsFlags_t    psFlags;
	int             clientEntityNum;
	// this is the frame we are creating
	frame = &client->frames[client->framenum & UPDATE_MASK];
	// this is the frame we are delta'ing from
	oldframe = get_last_frame(client);

	if (oldframe)
	{
		oldstate = &oldframe->ps;
		delta = client->framenum - client->lastframe;
	}
	else
	{
		oldstate = NULL;
		delta = 31;
	}

	// first byte to be patched
	b1 = SZ_GetSpace(&msg_write, 1);
	MSG_WriteLong((client->framenum & FRAMENUM_MASK) | (delta << FRAMENUM_BITS));
	// second byte to be patched
	b2 = SZ_GetSpace(&msg_write, 1);
	// send over the areabits
	MSG_WriteByte(frame->areabytes);
	MSG_WriteData(frame->areabits, frame->areabytes);
	// ignore some parts of playerstate if not recording demo
	psFlags = MSG_PS_NONE;

	if (client->settings[CLS_NOGUN])
	{
		psFlags |= MSG_PS_IGNORE_GUNFRAMES;

		if (client->settings[CLS_NOGUN] != 2)
			psFlags |= MSG_PS_IGNORE_GUNINDEX;
	}

	if (client->settings[CLS_NOBLEND])
		psFlags |= MSG_PS_IGNORE_BLEND;

	if (frame->ps.pmove.pm_type < PM_DEAD)
	{
		if (!(frame->ps.pmove.pm_flags & PMF_NO_PREDICTION))
			psFlags |= MSG_PS_IGNORE_VIEWANGLES;
	}
	else
	{
		// lying dead on a rotating platform?
		psFlags |= MSG_PS_IGNORE_DELTAANGLES;
	}

	clientEntityNum = 0;

	if (frame->ps.pmove.pm_type < PM_DEAD)
		clientEntityNum = frame->clientNum + 1;

	if (client->settings[CLS_NOPREDICT])
		psFlags |= MSG_PS_IGNORE_PREDICTION;

	suppressed = client->frameflags;

	// delta encode the playerstate
	extraflags = MSG_WriteDeltaPlayerstate_Enhanced(oldstate, &frame->ps, psFlags);

	// delta encode the clientNum
	int clientNum = oldframe ? oldframe->clientNum : 0;

	if (clientNum != frame->clientNum)
	{
		extraflags |= EPS_CLIENTNUM;
		MSG_WriteByte(frame->clientNum);
	}

	// save 3 high bits of extraflags
	*b1 = svc_frame | (((extraflags & 0x70) << 1));
	// save 4 low bits of extraflags
	*b2 = (suppressed & SUPPRESSCOUNT_MASK) |
		((extraflags & 0x0F) << SUPPRESSCOUNT_BITS);
	client->suppress_count = 0;
	client->frameflags = 0;
	// delta encode the entities
	SV_EmitPacketEntities(client, oldframe, frame, clientEntityNum);
}

/*
=============================================================================

Build a client frame structure

=============================================================================
*/


/*
=============
SV_BuildClientFrame

Decides which entities are going to be visible to the client, and
copies off the playerstat and areabits.
=============
*/
void SV_BuildClientFrame(client_t *client)
{
	int         e, i;
	vec3_t      org;
	edict_t     *ent;
	edict_t     *clent;
	client_frame_t  *frame;
	entity_packed_t *state;
	player_state_t  *ps;
	int         clientarea, clientcluster;
	mleaf_t     *leaf;
	byte        clientphs[VIS_MAX_BYTES];
	byte        clientpvs[VIS_MAX_BYTES];
	clent = client->edict;

	if (!clent->client)
		return;        // not in game yet

	// this is the frame we are creating
	frame = &client->frames[client->framenum & UPDATE_MASK];
	frame->number = client->framenum;
	frame->sentTime = com_eventTime; // save it for ping calc later
	frame->latency = -1; // not yet acked
	client->frames_sent++;
	// find the client's PVS
	ps = &clent->client->ps;
	VectorMA(ps->viewoffset, 0.125f, ps->pmove.origin, org);
	leaf = CM_PointLeaf(client->cm, org);
	clientarea = leaf->area;
	clientcluster = leaf->cluster;
	// calculate the visible areas
	frame->areabytes = CM_WriteAreaBits(client->cm, frame->areabits, clientarea);

	// grab the current player_state_t
	MSG_PackPlayer(&frame->ps, ps);

	// grab the current clientNum
	frame->clientNum = clent->client->clientNum;

	CM_FatPVS(client->cm, clientpvs, org);
	BSP_ClusterVis(client->cm->cache, clientphs, clientcluster, DVIS_PHS);
	// build up the list of visible entities
	frame->num_entities = 0;
	frame->first_entity = svs.next_entity;

	for (e = 1; e < client->pool->num_edicts; e++)
	{
		ent = EDICT_POOL(client, e);

		// ignore entities not in use
		if (!ent->inuse)
			continue;

		// ignore ents without visible models
		if (ent->flags.noclient)
			continue;

		// ignore ents without visible models unless they have an effect
		if (!ent->state.modelindex && !ent->state.effects && !ent->state.sound)
		{
			if (!ent->state.event)
				continue;

			if (ent->state.event == EV_FOOTSTEP && client->settings[CLS_NOFOOTSTEPS])
				continue;
		}

		if ((ent->state.effects & EF_GIB) && client->settings[CLS_NOGIBS])
			continue;

		// ignore if not touching a PV leaf
		if (ent != clent && !sv_novis->integer)
		{
			server_entity_t *sent = &sv.entities[NUM_FOR_EDICT(ent)];

			// check area
			if (!CM_AreasConnected(client->cm, clientarea, sent->areanum))
			{
				// doors can legally straddle two areas, so
				// we may need to check another one
				if (!CM_AreasConnected(client->cm, clientarea, sent->areanum2))
				{
					continue;        // blocked by a door
				}
			}

			// beams just check one point for PHS
			if (ent->state.renderfx & (RF_BEAM | RF_PROJECTILE))
			{
				if (!Q_IsBitSet(clientphs, sent->clusternums[0]))
					continue;
			}
			else
			{
				if (sent->num_clusters == -1)
				{
					// too many leafs for individual check, go by headnode
					if (!CM_HeadnodeVisible(CM_NodeNum(client->cm, sent->headnode), clientpvs))
						continue;
				}
				else
				{
					// check individual leafs
					for (i = 0; i < sent->num_clusters; i++)
						if (Q_IsBitSet(clientpvs, sent->clusternums[i]))
							break;

					if (i == sent->num_clusters)
						continue;       // not visible
				}

				if (!ent->state.modelindex && ent->state.sound)
				{
					// don't send sounds if they will be attenuated away
					vec3_t    delta;
					float    len;
					VectorSubtract(org, ent->state.origin, delta);
					len = VectorLength(delta);

					if (len > 400)
						continue;
				}
			}
		}

		if (ent->state.number != e)
		{
			Com_WPrintf("%s: fixing ent->state.number: %d to %d\n",
				__func__, ent->state.number, e);
			ent->state.number = e;
		}

		// add it to the circular client_entities array
		state = &svs.entities[svs.next_entity % svs.num_entities];
		MSG_PackEntity(state, &ent->state, Q2PRO_SHORTANGLES(client, e));

		// clear footsteps
		if (state->event == EV_FOOTSTEP && client->settings[CLS_NOFOOTSTEPS])
			state->event = 0;

		if (ent->owner == clent)
			state->solid = 0; // don't mark players missiles as solid
		else if (client->esFlags & MSG_ES_LONGSOLID)
			state->solid = sv.entities[e].solid32;

		state->clip_contents = sv.entities[e].clip_contents;
		svs.next_entity++;

		if (++frame->num_entities == MAX_PACKET_ENTITIES)
			break;
	}
}


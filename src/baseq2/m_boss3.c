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
/*
==============================================================================

boss3

==============================================================================
*/
#ifdef ENABLE_COOP
#include "g_local.h"
#include "m_boss32.h"

void Use_Boss3(edict_t *ent, edict_t *other, edict_t *activator)
{
	MSG_WriteByte(svc_temp_entity);
	MSG_WriteByte(TE_BOSSTPORT);
	MSG_WritePos(ent->server.state.origin);
	gi.multicast(ent->server.state.origin, MULTICAST_PVS);
	G_FreeEdict(ent);
}

void Think_Boss3Stand(edict_t *ent)
{
	if (ent->server.state.frame == FRAME_stand260)
		ent->server.state.frame = FRAME_stand201;
	else
		ent->server.state.frame++;

	ent->nextthink = level.time + game.frametime;
}

/*QUAKED monster_boss3_stand (1 .5 0) (-32 -32 0) (32 32 90)

Just stands and cycles in one place until targeted, then teleports away.
*/
void SP_monster_boss3_stand(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	self->movetype = MOVETYPE_STEP;
	self->server.solid = SOLID_BBOX;
	self->model = "models/monsters/boss3/rider/tris.md2";
	self->server.state.modelindex = gi.modelindex(self->model);
	self->server.state.frame = FRAME_stand201;
	gi.soundindex("misc/bigtele.wav");
	VectorSet(self->server.mins, -32, -32, 0);
	VectorSet(self->server.maxs, 32, 32, 90);
	self->use = Use_Boss3;
	self->think = Think_Boss3Stand;
	self->nextthink = level.time + game.frametime;
	gi.linkentity(self);
}
#endif
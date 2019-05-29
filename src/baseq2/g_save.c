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

//#define _DEBUG
typedef struct
{
	fieldtype_t		type;
#ifdef _DEBUG
	char *name;
#endif
	unsigned ofs;
	unsigned size;
} save_field_t;

#ifdef _DEBUG
	#define FA(type, name, size) { type, #name, OFS(name), size }
#else
	#define FA(type, name, size) { type, OFS(name), size }
#endif

#define T(name) FA(F_ITEM, name, sizeof(itemid_e))
#define L(name) FA(F_LSTRING, name, sizeof(char*))
#define E(name) FA(F_EDICT, name, sizeof(short))
#define NOP(name, size) FA(F_IGNORE, name, size)


static const save_field_t entityfields[] =
{
#define OFS FOFS
	NOP(client, sizeof(gclient_t *)),
	NOP(area, sizeof(list_t)),
	E(owner),
	L(model),
	L(message),
	L(target),
	L(targetname),
	L(killtarget),
	L(team),
	L(pathtarget),
	L(deathtarget),
	L(combattarget),
	E(target_ent),
	L(map),
	E(chain),
	E(enemy),
	E(oldenemy),
	E(activator),
	E(groundentity),
	E(teamchain),
	E(teammaster),

	E(mynoise),
	E(mynoise2),
	T(item),

	T(real_item),
	E(trigger_field),

	{0}
#undef OFS
};

static const save_field_t levelfields[] =
{
#define OFS LLOFS
	L(changemap),
	E(sight_client),
	E(sight_entity),
	E(sound_entity),
	E(sound2_entity),
	E(current_entity),
	{0}
#undef OFS
};


static const save_field_t clientfields[] =
{
#define OFS CLOFS
	T(pers.weapon),
	T(pers.lastweapon),
	T(gunstates[GUN_MAIN].newweapon),
	E(chase_target),
	{0}
#undef OFS
};

static const save_field_t gamefields[] =
{
#define OFS GLOFS
	NOP(clients, sizeof(gclient_t *)),
	{0}
#undef OFS
};

//=========================================================

static void write_data(void *buf, size_t len, FILE *f)
{
	if (fwrite(buf, 1, len, f) != len)
		gi.error("%s: couldn't write %"PRIz" bytes", __func__, len);
}

static void write_byte(FILE *f, byte v)
{
	write_data(&v, sizeof(v), f);
}

static void write_short(FILE *f, short v)
{
	v = LittleShort(v);
	write_data(&v, sizeof(v), f);
}

static void write_int(FILE *f, int v)
{
	v = LittleLong(v);
	write_data(&v, sizeof(v), f);
}

static void write_string(FILE *f, char *s)
{
	size_t len;

	if (!s)
	{
		write_int(f, -1);
		return;
	}

	len = strlen(s);
	write_int(f, len);
	write_data(s, len, f);
}

void write_rle(FILE *f, void *base, size_t len)
{
	fwrite(base, len, 1, f);
}

static void write_field(FILE *f, const save_field_t *field, void *base)
{
	void *p = (byte *)base + field->ofs;
	edict_t *ent;

	switch (field->type)
	{
		case F_LSTRING:
			write_string(f, *(char **)p);
			break;

		case F_EDICT:
			ent = *(edict_t **)p;

			if (ent)
				write_short(f, *(edict_t **)p - g_edicts);
			else
				write_short(f, -1);

			break;

		case F_ITEM:
			write_byte(f, GetIndexByItem(*(gitem_t **)p));
			break;

		case F_IGNORE:
			break;

		default:
			gi.error("%s: unknown field type", __func__);
	}
}

static void write_fields(FILE *f, const save_field_t *fields, void *base)
{
	const save_field_t *field;

	for (field = fields; field->type; field++)
		write_field(f, field, base);
}

static void read_data(void *buf, size_t len, FILE *f)
{
	if (fread(buf, 1, len, f) != len)
		gi.error("%s: couldn't read %"PRIz" bytes", __func__, len);
}

static int read_byte(FILE *f)
{
	byte v;
	read_data(&v, sizeof(v), f);
	return v;
}

static int read_short(FILE *f)
{
	short v;
	read_data(&v, sizeof(v), f);
	v = LittleShort(v);
	return v;
}

static int read_int(FILE *f)
{
	int v;
	read_data(&v, sizeof(v), f);
	v = LittleLong(v);
	return v;
}

static char *read_string(FILE *f)
{
	int len;
	char *s;
	len = read_int(f);

	if (len == -1)
		return NULL;

	if (len < 0 || len > 65536)
		gi.error("%s: bad length", __func__);

	s = gi.TagMalloc(len + 1, TAG_LEVEL);
	read_data(s, len, f);
	s[len] = 0;
	return s;
}

void unset_fields(void *base, const save_field_t *fields)
{
	const save_field_t *field;

	for (field = fields; field->type; field++)
		memset((byte *)base + field->ofs, 0, field->size);
}

void read_rle(FILE *f, void *base, size_t len)
{
	fread(base, len, 1, f);
}

static void read_field(FILE *f, const save_field_t *field, void *base)
{
	void *p = (byte *)base + field->ofs;
	short s;

	switch (field->type)
	{
		case F_LSTRING:
			*(char **)p = read_string(f);
			break;

		case F_EDICT:
			s = read_short(f);

			if (s == -1)
				*(edict_t **)p = NULL;
			else
				*(edict_t **)p = g_edicts + s;

			break;

		case F_ITEM:
			*(gitem_t **)p = GetItemByIndex(read_byte(f));
			break;

		case F_IGNORE:
			break;

		default:
			gi.error("%s: unknown field type", __func__);
	}
}

static void read_fields(FILE *f, const save_field_t *fields, void *base)
{
	const save_field_t *field;

	for (field = fields; field->type; field++)
		read_field(f, field, base);
}

//=========================================================

#define SAVE_MAGIC1     (('1'<<24)|('V'<<16)|('S'<<8)|'S')  // "SSV1"
#define SAVE_MAGIC2     (('1'<<24)|('V'<<16)|('A'<<8)|'S')  // "SAV1"
#define SAVE_VERSION    3

/*
============
WriteGame

This will be called whenever the game goes to a new level,
and when the user explicitly saves the game.

Game information include cross level data, like multi level
triggers, help computer info, and all client states.

A single player death will automatically restore from the
last save position.
============
*/
void WriteGame(const char *filename, qboolean autosave)
{
	FILE    *f;
	int     i;

	if (!autosave)
		SaveClientData();

	f = fopen(filename, "wb");

	if (!f)
		gi.error("Couldn't open %s", filename);

	write_int(f, SAVE_MAGIC1);
	write_int(f, SAVE_VERSION);
	game.autosaved = autosave;
	// make a copy of writable_game with
	// all of the special fields zeroed, for RLE purposes
	static game_locals_t writable_game;
	// zero special fields
	memcpy(&writable_game, &game, sizeof(game));
	unset_fields(&writable_game, gamefields);
	// write whole structure first
	write_rle(f, &writable_game, sizeof(writable_game));
	// write special fields
	write_fields(f, gamefields, &game);
	game.autosaved = false;
	static gclient_t writable_client;

	for (i = 0; i < game.maxclients; i++)
	{
		gclient_t *client = &game.clients[i];
		// zero special fields
		memcpy(&writable_client, client, sizeof(*client));
		unset_fields(&writable_client, clientfields);
		// write whole structure first
		write_rle(f, &writable_client, sizeof(writable_client));
		// write special fields
		write_fields(f, clientfields, client);
	}

	if (fclose(f))
		gi.error("Couldn't write %s", filename);
}

void ReadGame(const char *filename)
{
	FILE    *f;
	int     i;
	gi.FreeTags(TAG_GAME);
	f = fopen(filename, "rb");

	if (!f)
		gi.error("Couldn't open %s", filename);

	i = read_int(f);

	if (i != SAVE_MAGIC1)
	{
		fclose(f);
		gi.error("Not a save game");
	}

	i = read_int(f);

	if (i != SAVE_VERSION)
	{
		fclose(f);
		gi.error("Savegame from an older version");
	}

	read_rle(f, &game, sizeof(game));
	read_fields(f, gamefields, &game);

	// should agree with server's version
	if (game.maxclients != (int)maxclients->value)
	{
		fclose(f);
		gi.error("Savegame has bad maxclients");
	}

	if (game.maxentities <= game.maxclients || game.maxentities > MAX_EDICTS)
	{
		fclose(f);
		gi.error("Savegame has bad maxentities");
	}

	g_edicts = gi.TagMalloc(game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;
	globals.max_edicts = game.maxentities;
	game.clients = gi.TagMalloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);

	for (i = 0; i < game.maxclients; i++)
	{
		gclient_t *client = &game.clients[i];
		// write whole structure first
		read_rle(f, client, sizeof(*client));
		// write special fields
		read_fields(f, clientfields, client);
	}

	fclose(f);
}

//==========================================================


/*
=================
WriteLevel

=================
*/
void WriteLevel(const char *filename)
{
	int     i;
	edict_t *ent;
	FILE    *f;
	f = fopen(filename, "wb");

	if (!f)
		gi.error("Couldn't open %s", filename);

	write_int(f, SAVE_MAGIC2);
	write_int(f, SAVE_VERSION);
	// make a copy of writable_game with
	// all of the special fields zeroed, for RLE purposes
	static level_locals_t writable_level;
	// zero special fields
	memcpy(&writable_level, &level, sizeof(level));
	unset_fields(&writable_level, levelfields);
	// write whole structure first
	write_rle(f, &writable_level, sizeof(writable_level));
	// write special fields
	write_fields(f, levelfields, &level);
	static edict_t writable_entity;

	// write out all the entities
	for (i = 0; i < globals.num_edicts; i++)
	{
		ent = &g_edicts[i];

		if (!ent->inuse)
			continue;

		write_short(f, i);
		// zero special fields
		memcpy(&writable_entity, ent, sizeof(*ent));
		unset_fields(&writable_entity, entityfields);
		// write whole structure first
		write_rle(f, &writable_entity, sizeof(writable_entity));
		// write special fields
		write_fields(f, entityfields, ent);
	}

	write_short(f, -1);

	if (fclose(f))
		gi.error("Couldn't write %s", filename);
}


/*
=================
ReadLevel

SpawnEntities will allready have been called on the
level the same way it was when the level was saved.

That is necessary to get the baselines
set up identically.

The server will have cleared all of the world links before
calling ReadLevel.

No clients are connected yet.
=================
*/
void ReadLevel(const char *filename)
{
	int     entnum;
	FILE    *f;
	int     i;
	edict_t *ent;
	// free any dynamic memory allocated by loading the level
	// base state
	gi.FreeTags(TAG_LEVEL);
	f = fopen(filename, "rb");

	if (!f)
		gi.error("Couldn't open %s", filename);

	// wipe all the entities
	memset(g_edicts, 0, game.maxentities * sizeof(g_edicts[0]));
	globals.num_edicts = maxclients->value + 1;
	i = read_int(f);

	if (i != SAVE_MAGIC2)
	{
		fclose(f);
		gi.error("Not a save game");
	}

	i = read_int(f);

	if (i != SAVE_VERSION)
	{
		fclose(f);
		gi.error("Savegame from an older version");
	}

	// load the level locals
	read_rle(f, &level, sizeof(level));
	read_fields(f, levelfields, &level);

	// load all the entities
	while (1)
	{
		entnum = read_short(f);

		if (entnum == -1)
			break;

		if (entnum < 0 || entnum >= game.maxentities)
			gi.error("%s: bad entity number", __func__);

		if (entnum >= globals.num_edicts)
			globals.num_edicts = entnum + 1;

		ent = &g_edicts[entnum];
		// read whole structure first
		read_rle(f, ent, sizeof(*ent));
		// read special fields
		read_fields(f, entityfields, ent);
		// let the server rebuild world links for this ent
		gi.linkentity(ent);
	}

	fclose(f);

	// mark all clients as unconnected
	for (i = 0 ; i < maxclients->value ; i++)
	{
		ent = &g_edicts[i + 1];
		ent->client = game.clients + i;
		ent->client->pers.connected = false;
	}

	// do any load time things at this point
	for (i = 0 ; i < globals.num_edicts ; i++)
	{
		ent = &g_edicts[i];

		if (!ent->inuse)
			continue;

		// fire any cross-level triggers
		if (ent->entitytype == ET_TARGET_CROSSLEVEL_TARGET)
			ent->nextthink = level.time + (ent->delay * 1000);
	}
}


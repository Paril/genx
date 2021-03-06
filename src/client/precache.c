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

//
// cl_precache.c
//

#include "client.h"

/*
================
CL_ParsePlayerSkin

Breaks up playerskin into name (optional), model and skin components.
If model or skin are found to be invalid, replaces them with sane defaults.
================
*/
void CL_ParsePlayerSkin(char *name, char *model, char *skin, const char *s)
{
	size_t len;
	char *t;
	// configstring parsing guarantees that playerskins can never
	// overflow, but still check the length to be entirely fool-proof
	len = strlen(s);

	if (len >= MAX_QPATH)
		Com_Error(ERR_DROP, "%s: oversize playerskin", __func__);

	// isolate the player's name
	t = strchr(s, '\\');

	if (t)
	{
		len = t - s;
		strcpy(model, t + 1);
	}
	else
	{
		len = 0;
		strcpy(model, s);
	}

	// copy the player's name
	if (name)
	{
		memcpy(name, s, len);
		name[len] = 0;
	}

	// isolate the model name
	t = strchr(model, '/');

	if (!t)
		t = strchr(model, '\\');

	if (!t)
		goto default_model;

	*t = 0;
	// isolate the skin name
	strcpy(skin, t + 1);

	// fix empty model to male
	if (t == model)
		strcpy(model, "male");

	// apply restrictions on skins
	if (cl_noskins->integer == 2 || !COM_IsPath(skin))
		goto default_skin;

	if (cl_noskins->integer || !COM_IsPath(model))
		goto default_model;

	return;
default_skin:

	if (!Q_stricmp(model, "female"))
	{
		strcpy(model, "female");
		strcpy(skin, "athena");
	}
	else
	{
default_model:
		strcpy(model, "male");
		strcpy(skin, "grunt");
	}
}

/*
================
CL_LoadClientinfo

================
*/
void CL_LoadClientinfo(clientinfo_t *ci, const char *s)
{
	int         i;
	char        model_name[MAX_QPATH];
	char        skin_name[MAX_QPATH];
	char        model_filename[MAX_QPATH];
	char        skin_filename[MAX_QPATH];
	char        weapon_filename[MAX_QPATH];
	char        icon_filename[MAX_QPATH];
	CL_ParsePlayerSkin(ci->name, model_name, skin_name, s);
	// model file
	Q_concat(model_filename, sizeof(model_filename),
		"players/", model_name, "/tris.md2", NULL);
	ci->model = R_RegisterModel(model_filename);

	if (!ci->model && Q_stricmp(model_name, "male"))
	{
		strcpy(model_name, "male");
		strcpy(model_filename, "players/male/tris.md2");
		ci->model = R_RegisterModel(model_filename);
	}

	// skin file
	Q_concat(skin_filename, sizeof(skin_filename),
		"players/", model_name, "/", skin_name, ".pcx", NULL);
	ci->skin = R_RegisterSkin(skin_filename);

	// if we don't have the skin and the model was female,
	// see if athena skin exists
	if (!ci->skin && !Q_stricmp(model_name, "female"))
	{
		strcpy(skin_name, "athena");
		strcpy(skin_filename, "players/female/athena.pcx");
		ci->skin = R_RegisterSkin(skin_filename);
	}

	// if we don't have the skin and the model wasn't male,
	// see if the male has it (this is for CTF's skins)
	if (!ci->skin && Q_stricmp(model_name, "male"))
	{
		// change model to male
		strcpy(model_name, "male");
		strcpy(model_filename, "players/male/tris.md2");
		ci->model = R_RegisterModel(model_filename);
		// see if the skin exists for the male model
		Q_concat(skin_filename, sizeof(skin_filename),
			"players/male/", skin_name, ".pcx", NULL);
		ci->skin = R_RegisterSkin(skin_filename);
	}

	// if we still don't have a skin, it means that the male model
	// didn't have it, so default to grunt
	if (!ci->skin)
	{
		// see if the skin exists for the male model
		strcpy(skin_name, "grunt");
		strcpy(skin_filename, "players/male/grunt.pcx");
		ci->skin = R_RegisterSkin(skin_filename);
	}

	// weapon file
	for (i = 0; i < cl.numWeaponModels; i++)
	{
		Q_concat(weapon_filename, sizeof(weapon_filename),
			"players/", model_name, "/", cl.weaponModels[i], NULL);
		ci->weaponmodel[i] = R_RegisterModel(weapon_filename);

		if (!ci->weaponmodel[i] && !Q_stricmp(model_name, "cyborg"))
		{
			// try male
			Q_concat(weapon_filename, sizeof(weapon_filename),
				"players/male/", cl.weaponModels[i], NULL);
			ci->weaponmodel[i] = R_RegisterModel(weapon_filename);
		}
	}

	// icon file
	Q_concat(icon_filename, sizeof(icon_filename),
		"/players/", model_name, "/", skin_name, "_i.pcx", NULL);
	ci->icon = R_RegisterPic2(icon_filename);
	strcpy(ci->model_name, model_name);
	strcpy(ci->skin_name, skin_name);

	// base info should be at least partially valid
	if (ci == &cl.baseclientinfo)
		return;

	// must have loaded all data types to be valid
	if (!ci->skin || !ci->icon || !ci->model || !ci->weaponmodel[0])
	{
		ci->skin = 0;
		ci->icon = 0;
		ci->model = 0;
		ci->weaponmodel[0] = 0;
		ci->model_name[0] = 0;
		ci->skin_name[0] = 0;
	}
}

/*
=================
CL_RegisterSounds
=================
*/
void CL_RegisterSounds(void)
{
	int i;
	char    *s;
	S_BeginRegistration();
	CL_RegisterTEntSounds();
	CL_RegisterEffectSounds();
	const byte *precache_bitset = (const byte *)cl.precache_bitset;

	for (i = 1; i < MAX_PRECACHE; i++)
	{
		if (Q_GetPrecacheBitsetType(precache_bitset, i) != PRECACHE_SOUND)
			continue;

		s = cl.configstrings[CS_PRECACHE + i];

		if (!s[0])
			continue;

		cl.precache[i].type = PRECACHE_SOUND;
		cl.precache[i].sound = S_RegisterSound(s);
	}

	S_EndRegistration();
}

/*
=================
CL_RegisterBspModels

Registers main BSP file and inline models
=================
*/
void CL_RegisterBspModels(void)
{
	int ret;
	char *name;
	int i;

	for (i = 1; i < MAX_PRECACHE; i++)
	{
		name = cl.configstrings[CS_PRECACHE + i];

		if (!name[0])
			continue;

		if (Q_strncasecmp(name, "maps/", 5) == 0)
		{
			ret = BSP_Load(name, &cl.bsp);

			if (cl.bsp == NULL)
				Com_Error(ERR_DROP, "Couldn't load %s: %s", name, Q_ErrorString(ret));

#if USE_MAPCHECKSUM

			if (cl.bsp->checksum != (unsigned int)atoi(cl.configstrings[CS_MAPCHECKSUM]))
			{
				Com_Error(ERR_DROP, "Local map version differs from server: %i != %s",
					cl.bsp->checksum, cl.configstrings[CS_MAPCHECKSUM]);
			}

#endif
		}
		else if (name[0] == '*')
			cl.precache[i].model.clip = BSP_InlineModel(cl.bsp, name);
		else
			break;
	}
}

/*
=================
CL_RegisterVWepModels

Builds a list of visual weapon models
=================
*/
void CL_RegisterVWepModels(void)
{
	int         i;
	char        *name;
	cl.numWeaponModels = 1;
	strcpy(cl.weaponModels[0], "weapon.md2");

	// only default model when vwep is off
	if (!cl_vwep->integer)
		return;

	const byte *precache_bitset = (const byte *)cl.precache_bitset;

	for (i = 1; i < MAX_PRECACHE; i++)
	{
		if (Q_GetPrecacheBitsetType(precache_bitset, i) != PRECACHE_MODEL)
			continue;

		name = cl.configstrings[MAX_PRECACHE + i];

		if (!name[0])
			continue;
		else if (name[0] != '#')
			continue;

		// special player weapon model
		strcpy(cl.weaponModels[cl.numWeaponModels++], name + 1);

		if (cl.numWeaponModels == MAX_CLIENTWEAPONMODELS)
			break;
	}
}

/*
=================
CL_SetSky

=================
*/
void CL_SetSky(void)
{
	float       rotate;
	vec3_t      axis;
	rotate = atof(cl.configstrings[CS_SKYROTATE]);

	if (sscanf(cl.configstrings[CS_SKYAXIS], "%f %f %f",
			&axis[0], &axis[1], &axis[2]) != 3)
	{
		Com_DPrintf("Couldn't parse CS_SKYAXIS\n");
		VectorClear(axis);
	}

	R_SetSky(cl.configstrings[CS_SKY], rotate, axis);
}

static void CL_ParseWeaponSeeds()
{
	weapon_group_e group = WEAPON_GROUP_RAPID;
	gametype_t game = GAME_Q2;
	uint32_t weapon_seed_index = 0, ammo_seed_index = 0;

	for (const char *ptr = cl.configstrings[CS_SEEDS]; *ptr; )
	{
		if (*ptr == '|')
		{
			group++;
			game = GAME_Q2;
			weapon_seed_index = ammo_seed_index = 0;
			ptr++;
			continue;
		}
		else if (*ptr == ';')
		{
			game++;
			weapon_seed_index = ammo_seed_index = 0;
			ptr++;
			continue;
		}
		else if (*ptr == ',')
		{
			ptr++;
			continue;
		}

		uint16_t index;
		sscanf(ptr, "%hu", &index);
		cl.weapon_seeds[game][group].precache[weapon_seed_index++] = cl.precache[index];
		cl.weapon_seeds[game][group].num_seeds = weapon_seed_index;

		while (Q_isdigit(*ptr))
			ptr++;

		if (*ptr == ':')
		{
			ptr++;

			sscanf(ptr, "%hu", &index);
			cl.ammo_seeds[game][group].precache[ammo_seed_index++] = cl.precache[index];
			cl.ammo_seeds[game][group].num_seeds = ammo_seed_index;

			while (Q_isdigit(*ptr))
				ptr++;
		}
	}
}

/*
=================
CL_PrepRefresh

Call before entering a new level, or after changing dlls
=================
*/
void CL_PrepRefresh(void)
{
	int         i;
	char        *name;

	if (!cls.ref_initialized)
		return;

	if (!cl.mapname[0])
		return;     // no map loaded

	// register models, pics, and skins
	R_BeginRegistration(cl.mapname);
	CL_LoadState(LOAD_MODELS);
	CL_RegisterTEntModels();
	const byte *precache_bitset = (const byte *)cl.precache_bitset;

	for (i = 1; i < MAX_PRECACHE; i++)
	{
		if (Q_GetPrecacheBitsetType(precache_bitset, i) != PRECACHE_MODEL)
			continue;

		name = cl.configstrings[CS_PRECACHE + i];

		if (!name[0])
			continue;
		else if (name[0] == '#' || Q_strncasecmp(name, "maps/", 5) == 0)
		{
			cl.precache[i].type = PRECACHE_MODEL;
			continue;
		}

		cl.precache[i].type = PRECACHE_MODEL;
		cl.precache[i].model.handle = R_RegisterModel(name);
	}

	CL_LoadState(LOAD_IMAGES);

	for (i = 1; i < MAX_PRECACHE; i++)
	{
		if (Q_GetPrecacheBitsetType(precache_bitset, i) != PRECACHE_PIC)
			continue;

		name = cl.configstrings[CS_PRECACHE + i];

		if (!name[0])
			continue;

		cl.precache[i].type = PRECACHE_PIC;
		cl.precache[i].image = R_RegisterPic2(name);

		if (cl.precache[i].image < 0)
			cl.precache[i].image = cl.precache[i].image;
	}

	CL_LoadState(LOAD_CLIENTS);

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		name = cl.configstrings[CS_PLAYERSKINS + i];

		if (!name[0])
			continue;

		CL_LoadClientinfo(&cl.clientinfo[i], name);
	}

	CL_LoadClientinfo(&cl.baseclientinfo, "unnamed\\male/grunt");
	// set sky textures and speed
	CL_SetSky();

	// set game mode
#ifdef ENABLE_COOP
	if (!Q_strcasecmp(cl.configstrings[CS_GAMEMODE], "deathmatch"))
		cl.gamemode = GAMEMODE_DEATHMATCH;
	else if (!Q_strcasecmp(cl.configstrings[CS_GAMEMODE], "coop"))
		cl.gamemode = GAMEMODE_COOP;
	else if (!Q_strcasecmp(cl.configstrings[CS_GAMEMODE], "invasion"))
		cl.gamemode = GAMEMODE_INVASION;
	else
		cl.gamemode = GAMEMODE_SINGLEPLAYER;
#endif

	CL_ParseWeaponSeeds();

	// the renderer can now free unneeded stuff
	R_EndRegistration();
	// clear any lines of console text
	Con_ClearNotify_f();
	SCR_UpdateScreen();
}

/*
=================
CL_UpdateConfigstring

A configstring update has been parsed.
=================
*/
void CL_UpdateConfigstring(int index)
{
	const char *s = cl.configstrings[index];

	if (index == CS_MAXCLIENTS)
	{
		cl.maxclients = atoi(s);
		return;
	}

	if (index == CS_AIRACCEL)
	{
		cl.pmp.airaccelerate = cl.pmp.qwmode || atoi(s);
		return;
	}

	if (index >= CS_PRECACHE && index <= CS_PRECACHE + MAX_PRECACHE && Q_strncasecmp(s, "maps/", 5) == 0)
	{
		size_t len = strlen(s);

		if (len <= 9)
			Com_Error(ERR_DROP, "%s: bad world model: %s", __func__, s);

		memcpy(cl.mapname, s + 5, len - 9);   // skip "maps/"
		cl.mapname[len - 9] = 0; // cut off ".bsp"
		return;
	}

	if (index >= CS_LIGHTS && index < CS_LIGHTS + MAX_LIGHTSTYLES)
	{
		CL_SetLightStyle(index - CS_LIGHTS, s);
		return;
	}

	if (cls.state < ca_precached)
		return;

	if (index >= CS_PRECACHE && index <= CS_PRECACHE + MAX_PRECACHE)
	{
		const byte *precache_bitset = (const byte *)cl.precache_bitset;
		const int i = index - CS_PRECACHE;
		precache_type_e type = Q_GetPrecacheBitsetType(precache_bitset, i);
		cl.precache[i].type = type;

		switch (type)
		{
			case PRECACHE_MODEL:
				cl.precache[i].model.handle = R_RegisterModel(s);

				if (*s == '*')
					cl.precache[i].model.clip = BSP_InlineModel(cl.bsp, s);
				else
					cl.precache[i].model.clip = NULL;

				break;

			case PRECACHE_SOUND:
				cl.precache[i].sound = S_RegisterSound(s);
				break;

			case PRECACHE_PIC:
				cl.precache[i].image = R_RegisterPic2(s);
				break;
		}
	}

	if (index >= CS_PLAYERSKINS && index < CS_PLAYERSKINS + MAX_CLIENTS)
	{
		CL_LoadClientinfo(&cl.clientinfo[index - CS_PLAYERSKINS], s);
		return;
	}
}

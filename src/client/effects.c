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
// cl_fx.c -- entity effects parsing and management

#include "client.h"

static void CL_LogoutEffect(gametype_t game, vec3_t org, int type);

static vec3_t avelocities[NUMVERTEXNORMALS];


extern int		ramp1[8];
extern int		ramp2[8];
extern int		ramp3[8];

/*
==============================================================

LIGHT STYLE MANAGEMENT

==============================================================
*/

typedef struct clightstyle_s
{
	list_t  entry;
	int     length;
	vec4_t  value;
	float   map[MAX_QPATH];
} clightstyle_t;

static clightstyle_t    cl_lightstyles[MAX_LIGHTSTYLES];
static LIST_DECL(cl_lightlist);
static int          cl_lastofs;

void CL_ClearLightStyles(void)
{
	int     i;
	clightstyle_t   *ls;

	for (i = 0, ls = cl_lightstyles; i < MAX_LIGHTSTYLES; i++, ls++)
	{
		List_Init(&ls->entry);
		ls->length = 0;
		ls->value[0] =
			ls->value[1] =
				ls->value[2] =
					ls->value[3] = 1;
	}

	List_Init(&cl_lightlist);
	cl_lastofs = -1;
}

/*
================
CL_RunLightStyles
================
*/
void CL_RunLightStyles(void)
{
	int     ofs;
	clightstyle_t   *ls;
	ofs = cl.time / 100;

	if (ofs == cl_lastofs)
		return;

	cl_lastofs = ofs;
	LIST_FOR_EACH(clightstyle_t, ls, &cl_lightlist, entry)
	{
		ls->value[0] =
			ls->value[1] =
				ls->value[2] =
					ls->value[3] = ls->map[ofs % ls->length];
	}
}

void CL_SetLightStyle(int index, const char *s)
{
	int     i;
	clightstyle_t   *ls;
	ls = &cl_lightstyles[index];
	ls->length = strlen(s);

	if (ls->length > MAX_QPATH)
		Com_Error(ERR_DROP, "%s: oversize style", __func__);

	for (i = 0; i < ls->length; i++)
		ls->map[i] = (float)(s[i] - 'a') / (float)('m' - 'a');

	if (ls->entry.prev)
		List_Delete(&ls->entry);

	if (ls->length > 1)
	{
		List_Append(&cl_lightlist, &ls->entry);
		return;
	}

	if (ls->length == 1)
	{
		ls->value[0] =
			ls->value[1] =
				ls->value[2] =
					ls->value[3] = ls->map[0];
		return;
	}

	ls->value[0] =
		ls->value[1] =
			ls->value[2] =
				ls->value[3] = 1;
}

/*
================
CL_AddLightStyles
================
*/
void CL_AddLightStyles(void)
{
	int     i;
	clightstyle_t   *ls;

	for (i = 0, ls = cl_lightstyles; i < MAX_LIGHTSTYLES; i++, ls++)
		V_AddLightStyle(i, ls->value);
}

/*
==============================================================

DLIGHT MANAGEMENT

==============================================================
*/

static cdlight_t       cl_dlights[MAX_DLIGHTS];

static void CL_ClearDlights(void)
{
	memset(cl_dlights, 0, sizeof(cl_dlights));
}

/*
===============
CL_AllocDlight

===============
*/
cdlight_t *CL_AllocDlight(int key)
{
	int     i;
	cdlight_t   *dl;

	// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;

		for (i = 0; i < MAX_DLIGHTS; i++, dl++)
		{
			if (dl->key == key)
			{
				memset(dl, 0, sizeof(*dl));
				dl->key = key;
				return dl;
			}
		}
	}

	// then look for anything else
	dl = cl_dlights;

	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			memset(dl, 0, sizeof(*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	memset(dl, 0, sizeof(*dl));
	dl->key = key;
	return dl;
}

/*
===============
CL_RunDLights

===============
*/
void CL_RunDLights(void)
{
	int         i;
	cdlight_t   *dl;
	dl = cl_dlights;

	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (!dl->radius)
			continue;

		if (dl->die < cl.time)
		{
			dl->radius = 0;
			return;
		}
	}
}

/*
===============
CL_AddDLights

===============
*/
void CL_AddDLights(void)
{
	int         i;
	cdlight_t   *dl;
	dl = cl_dlights;

	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (!dl->radius)
			continue;

		V_AddLight(dl->origin, dl->radius,
			dl->color[0], dl->color[1], dl->color[2]);
	}
}

// ==============================================================

qhandle_t sfx_q2_blaster;
qhandle_t sfx_q2_hyperblaster_fire;
qhandle_t sfx_q2_machinegun_fire[5];
qhandle_t sfx_q2_shotgun_fire;
qhandle_t sfx_q2_shotgun_reload;
qhandle_t sfx_q2_sshotgun_fire;
qhandle_t sfx_q2_railgun_fire;
qhandle_t sfx_q2_rocketlauncher_fire;
qhandle_t sfx_q2_rocketlauncher_reload;
qhandle_t sfx_q2_grenadelauncher_fire;
qhandle_t sfx_q2_grenadelauncher_reload;
qhandle_t sfx_q2_bfg_fire;

qhandle_t sfx_q1_lightning_start;
qhandle_t sfx_q1_nailgun_fire;
qhandle_t sfx_q1_shotgun_fire;
qhandle_t sfx_q1_sshotgun_fire;
qhandle_t sfx_q1_snailgun_fire;
qhandle_t sfx_q1_rocketlauncher_fire;
qhandle_t sfx_q1_grenadelauncher_fire;

qhandle_t sfx_doom_pistol;
qhandle_t sfx_doom_shotgun_fire;
qhandle_t sfx_doom_sshotgun_fire;
qhandle_t sfx_doom_rocketlauncher_fire;
qhandle_t sfx_doom_plasma;
qhandle_t sfx_doom_bfg_fire;

qhandle_t sfx_duke_pistol;
qhandle_t sfx_duke_shotgun_fire;
qhandle_t sfx_duke_cannon_fire;
qhandle_t sfx_duke_rpg_fire;
qhandle_t sfx_duke_dev_fire;

void CL_RegisterEffectSounds()
{
	sfx_q2_blaster = S_RegisterSound("weapons/blastf1a.wav");
	sfx_q2_hyperblaster_fire = S_RegisterSound("weapons/hyprbf1a.wav");
	sfx_q1_lightning_start = S_RegisterSound("q1/weapons/lstart.wav");

	for (int i = 0; i < 5; ++i)
		sfx_q2_machinegun_fire[i] = S_RegisterSound(va("weapons/machgf%ib.wav", i + 1));

	sfx_q1_nailgun_fire = S_RegisterSound("q1/weapons/rocket1i.wav");
	sfx_q2_shotgun_fire = S_RegisterSound("weapons/shotgf1b.wav");
	sfx_q2_shotgun_reload = S_RegisterSound("weapons/shotgr1b.wav");
	sfx_q1_shotgun_fire = S_RegisterSound("q1/weapons/guncock.wav");
	sfx_q2_sshotgun_fire = S_RegisterSound("weapons/sshotf1b.wav");
	sfx_q1_sshotgun_fire = S_RegisterSound("q1/weapons/shotgn2.wav");
	sfx_q1_snailgun_fire = S_RegisterSound("q1/weapons/spike2.wav");
	sfx_q2_railgun_fire = S_RegisterSound("weapons/railgf1a.wav");
	sfx_q2_rocketlauncher_fire = S_RegisterSound("weapons/rocklf1a.wav");
	sfx_q2_rocketlauncher_reload = S_RegisterSound("weapons/rocklr1b.wav");
	sfx_q1_rocketlauncher_fire = S_RegisterSound("q1/weapons/sgun1.wav");
	sfx_q2_grenadelauncher_fire = S_RegisterSound("weapons/grenlf1a.wav");
	sfx_q2_grenadelauncher_reload = S_RegisterSound("weapons/grenlr1b.wav");
	sfx_q1_grenadelauncher_fire = S_RegisterSound("q1/weapons/grenade.wav");
	sfx_q2_bfg_fire = S_RegisterSound("weapons/bfg__f1y.wav");
	sfx_doom_pistol = S_RegisterSound("doom/PISTOL.wav");
	sfx_doom_shotgun_fire = S_RegisterSound("doom/SHOTGN.wav");
	sfx_doom_sshotgun_fire = S_RegisterSound("doom/DSHTGN.wav");
	sfx_doom_rocketlauncher_fire = S_RegisterSound("doom/RLAUNC.wav");
	sfx_doom_plasma = S_RegisterSound("doom/PLASMA.wav");
	sfx_doom_bfg_fire = S_RegisterSound("doom/BFG.wav");
	sfx_duke_pistol = S_RegisterSound("%s_duke_pistol_fire");
	sfx_duke_shotgun_fire = S_RegisterSound("%s_duke_shotgun_fire");
	sfx_duke_cannon_fire = S_RegisterSound("%s_duke_ripper_fire");
	sfx_duke_rpg_fire = S_RegisterSound("duke/RPGFIRE.wav");
	sfx_duke_dev_fire = S_RegisterSound("%s_duke_freeze_fire");
}

void CL_MakeDukeShell(vec3_t origin, vec3_t velocity, bool shotgun);

void CL_DukeThrowShell(bool shotgun, float rVel, float zVel, float originY)
{
	vec3_t forward, right, vel = { 0 }, origin;

	if (mz.entity == cl.clientNum + 1)
	{
		if (!cl.thirdPersonView)
		{
			AngleVectors(cl.refdef.viewangles, forward, right, NULL);
			VectorCopy(cl.refdef.vieworg, origin);
		}
		else
		{
			AngleVectors(cl.playerEntityAngles, forward, right, NULL);
			VectorCopy(cl.playerEntityOrigin, origin);
			originY -= 22;
		}
	}
	else
	{
		AngleVectors(cl_entities[mz.entity].current.angles, forward, right, NULL);
		VectorCopy(cl_entities[mz.entity].current.origin, origin);
		originY -= 22;
	}

	VectorMA(vel, rVel, right, vel);
	vel[2] = zVel;
	VectorMA(origin, 2, right, origin);
	VectorMA(origin, 8, forward, origin);
	origin[2] -= originY;
	CL_MakeDukeShell(origin, vel, shotgun);
}

void CL_DukeThrowPistolShell()
{
	CL_DukeThrowShell(false, -45 + (crand() * 15), 45 + (crand() * 15), 2);
}

void CL_DukeThrowCannonShell()
{
	CL_DukeThrowShell(false, 60 + (crand() * 5), 25 + (crand() * 5), 4);
}

void CL_DukeThrowShotgunShell()
{
	CL_DukeThrowShell(true, -45 + (crand() * 15), 5 + (crand() * 5), 2);
}

/*
==============
CL_MuzzleFlash
==============
*/
void CL_MuzzleFlash(void)
{
	vec3_t      fv, rv;
	cdlight_t   *dl;
	centity_t   *pl;
	float       volume;
#ifdef _DEBUG

	if (developer->integer)
		CL_CheckEntityPresent(mz.entity, "muzzleflash");

#endif
	pl = &cl_entities[mz.entity];
	dl = CL_AllocDlight(mz.entity);
	VectorCopy(pl->current.origin,  dl->origin);
	AngleVectors(pl->current.angles, fv, rv, NULL);
	VectorMA(dl->origin, 18, fv, dl->origin);
	VectorMA(dl->origin, 16, rv, dl->origin);

	if (mz.silenced)
		dl->radius = 100 + (Q_rand() & 31);
	else
		dl->radius = 200 + (Q_rand() & 31);

	dl->die = cl.time + 50; // + 0.1;
#define DL_COLOR(r, g, b)   VectorSet(dl->color, r, g, b)
#define DL_RADIUS(r)        (dl->radius = r)
#define DL_DIE(t)           (dl->die = cl.time + t)

	if (mz.silenced)
		volume = 0.2f;
	else
		volume = 1;

	switch (mz.weapon)
	{
		case MZ_BLASTER:
			DL_COLOR(1, 1, 0);

			switch (cl_entities[mz.entity].current.game)
			{
				case GAME_Q2:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_blaster, volume, ATTN_NORM, 0);
					break;

				case GAME_DOOM:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_doom_pistol, volume, ATTN_NORM, 0);
					break;

				case GAME_DUKE:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_duke_pistol, volume, ATTN_NORM, 0);
					CL_DukeThrowPistolShell();
					break;
			}

			break;

		case MZ_BLUEHYPERBLASTER:
			DL_COLOR(0, 0, 1);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_hyperblaster_fire, volume, ATTN_NORM, 0);
			break;

		case MZ_HYPERBLASTER:
			DL_COLOR(1, 1, 0);

			switch (cl_entities[mz.entity].current.game)
			{
				case GAME_Q2:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_hyperblaster_fire, volume, ATTN_NORM, 0);
					break;

				case GAME_Q1:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q1_lightning_start, volume, ATTN_NORM, 0);
					break;

				case GAME_DOOM:
					DL_COLOR(0, 0, 1);
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_doom_plasma, volume, ATTN_NORM, 0);
					break;
			}

			break;

		case MZ_MACHINEGUN:
			DL_COLOR(1, 1, 0);

			switch (cl_entities[mz.entity].current.game)
			{
				case GAME_Q2:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_machinegun_fire[(rand() % 5)], volume, ATTN_NORM, 0);
					break;

				case GAME_Q1:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q1_nailgun_fire, volume, ATTN_NORM, 0);
					break;
			}

			break;

		case MZ_SHOTGUN:
			DL_COLOR(1, 1, 0);

			switch (cl_entities[mz.entity].current.game)
			{
				case GAME_Q2:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_shotgun_fire, volume, ATTN_NORM, 0);
					S_StartSound(NULL, mz.entity, CHAN_AUTO, sfx_q2_shotgun_reload, volume, ATTN_NORM, 0.1);
					break;

				case GAME_Q1:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q1_shotgun_fire, volume, ATTN_NORM, 0);
					break;

				case GAME_DOOM:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_doom_shotgun_fire, volume, ATTN_NORM, 0);
					break;

				case GAME_DUKE:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_duke_shotgun_fire, volume, ATTN_NORM, 0);
					break;
			}

			break;

		case MZ_SSHOTGUN:
			DL_COLOR(1, 1, 0);

			switch (cl_entities[mz.entity].current.game)
			{
				case GAME_Q2:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_sshotgun_fire, volume, ATTN_NORM, 0);
					break;

				case GAME_Q1:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q1_sshotgun_fire, volume, ATTN_NORM, 0);
					break;

				case GAME_DOOM:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_doom_sshotgun_fire, volume, ATTN_NORM, 0);
					break;

				case GAME_DUKE:
					CL_DukeThrowShotgunShell();
					DL_RADIUS(0);
					break;
			}

			break;

		case MZ_CHAINGUN1:
			DL_RADIUS(200 + (Q_rand() & 31));
			DL_COLOR(1, 0.25f, 0);

			switch (cl_entities[mz.entity].current.game)
			{
				case GAME_Q2:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_machinegun_fire[(rand() % 5)], volume, ATTN_NORM, 0);
					break;

				case GAME_Q1:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q1_snailgun_fire, volume, ATTN_NORM, 0);
					break;

				case GAME_DUKE:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_duke_cannon_fire, volume, ATTN_NORM, 0);
					CL_DukeThrowCannonShell();
					break;
			}

			break;

		case MZ_CHAINGUN2:
			DL_RADIUS(225 + (Q_rand() & 31));
			DL_COLOR(1, 0.5f, 0);
			DL_DIE(100);   // long delay
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_machinegun_fire[(rand() % 5)], volume, ATTN_NORM, 0);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_machinegun_fire[(rand() % 5)], volume, ATTN_NORM, 0.05);
			break;

		case MZ_CHAINGUN3:
			DL_RADIUS(250 + (Q_rand() & 31));
			DL_COLOR(1, 1, 0);
			DL_DIE(100);   // long delay
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_machinegun_fire[(rand() % 5)], volume, ATTN_NORM, 0);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_machinegun_fire[(rand() % 5)], volume, ATTN_NORM, 0.033);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_machinegun_fire[(rand() % 5)], volume, ATTN_NORM, 0.066);
			break;

		case MZ_RAILGUN:
			DL_COLOR(0.5f, 0.5f, 1.0f);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_railgun_fire, volume, ATTN_NORM, 0);
			break;

		case MZ_ROCKET:
			DL_COLOR(1, 0.5f, 0.2f);

			switch (cl_entities[mz.entity].current.game)
			{
				case GAME_Q2:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_rocketlauncher_fire, volume, ATTN_NORM, 0);
					S_StartSound(NULL, mz.entity, CHAN_AUTO,   sfx_q2_rocketlauncher_reload, volume, ATTN_NORM, 0.1);
					break;

				case GAME_Q1:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q1_rocketlauncher_fire, volume, ATTN_NORM, 0);
					break;

				case GAME_DOOM:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_doom_rocketlauncher_fire, volume, ATTN_NORM, 0);
					break;

				case GAME_DUKE:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_duke_rpg_fire, volume, ATTN_NORM, 0);
					break;
			}

			break;

		case MZ_GRENADE:
			DL_COLOR(1, 0.5f, 0);

			switch (cl_entities[mz.entity].current.game)
			{
				case GAME_Q2:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_grenadelauncher_fire, volume, ATTN_NORM, 0);
					S_StartSound(NULL, mz.entity, CHAN_AUTO, sfx_q2_grenadelauncher_reload, volume, ATTN_NORM, 0.1);
					break;

				case GAME_Q1:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q1_grenadelauncher_fire, volume, ATTN_NORM, 0);
					break;

				case GAME_DUKE:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_duke_rpg_fire, volume, ATTN_NORM, 0);
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_duke_rpg_fire, volume, ATTN_NORM, 0.06);
					S_StartSound(NULL, mz.entity, CHAN_WEAPON2, sfx_duke_dev_fire, volume, ATTN_NORM, 0);
					break;
			}

			break;

		case MZ_BFG:
			switch (cl_entities[mz.entity].current.game)
			{
				case GAME_Q2:
					DL_COLOR(0, 1, 0);
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_bfg_fire, volume, ATTN_NORM, 0);
					break;

				case GAME_DOOM:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_doom_bfg_fire, volume, ATTN_NORM, 0);
					break;

				case GAME_DUKE:
					S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_duke_dev_fire, volume, ATTN_NORM, 0);
					break;
			}

			break;

		case MZ_LOGIN:
			if (pl->current.game == GAME_DUKE)
				DL_COLOR(0, 0, 1);
			else
				DL_COLOR(0, 1, 0);

			DL_DIE(1.0f);
			CL_LogoutEffect(pl->current.game, pl->current.origin, mz.weapon);
			break;

		case MZ_LOGOUT:
			if (pl->current.game == GAME_DUKE)
				DL_COLOR(0, 0, 1);
			else
				DL_COLOR(1, 0, 0);

			DL_DIE(100);
			CL_LogoutEffect(pl->current.game, pl->current.origin, mz.weapon);
			break;

		case MZ_RESPAWN:
			if (pl->current.game == GAME_DUKE)
				DL_COLOR(0, 0, 1);
			else
				DL_COLOR(1, 1, 0);

			DL_DIE(100);
			CL_LogoutEffect(pl->current.game, pl->current.origin, mz.weapon);
			break;

		case MZ_PHALANX:
			DL_COLOR(1, 0.5f, 0.5f);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("weapons/plasshot.wav"), volume, ATTN_NORM, 0);
			break;

		case MZ_IONRIPPER:
			DL_COLOR(1, 0.5f, 0.5f);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("weapons/rippfiref.wav"), volume, ATTN_NORM, 0);
			break;

		case MZ_ETF_RIFLE:
			DL_COLOR(0.9f, 0.7f, 0);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("weapons/nail1.wav"), volume, ATTN_NORM, 0);
			break;

		case MZ_SHOTGUN2:
			DL_COLOR(1, 1, 0);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("weapons/shotg2.wav"), volume, ATTN_NORM, 0);
			break;

		case MZ_HEATBEAM:
			DL_COLOR(1, 1, 0);
			DL_DIE(100);
			//      S_StartSound (NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("weapons/bfg__l1a.wav"), volume, ATTN_NORM, 0);
			break;

		case MZ_BLASTER2:
			DL_COLOR(0, 1, 0);
			// FIXME - different sound for blaster2 ??
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_blaster, volume, ATTN_NORM, 0);
			break;

		case MZ_TRACKER:
			// negative flashes handled the same in gl/soft until CL_AddDLights
			DL_COLOR(-1, -1, -1);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("weapons/disint2.wav"), volume, ATTN_NORM, 0);
			break;

		case MZ_NUKE1:
			DL_COLOR(1, 0, 0);
			DL_DIE(100);
			break;

		case MZ_NUKE2:
			DL_COLOR(1, 1, 0);
			DL_DIE(100);
			break;

		case MZ_NUKE4:
			DL_COLOR(0, 0, 1);
			DL_DIE(100);
			break;

		case MZ_NUKE8:
			DL_COLOR(0, 1, 1);
			DL_DIE(100);
			break;
	}
}


/*
==============
CL_MuzzleFlash2
==============
*/
void CL_MuzzleFlash2(void)
{
	centity_t   *ent;
	vec3_t      origin;
	const vec_t *ofs;
	cdlight_t   *dl;
	vec3_t      forward, right;
	char        soundname[MAX_QPATH];
	// locate the origin
	ent = &cl_entities[mz.entity];
	AngleVectors(ent->current.angles, forward, right, NULL);
	ofs = monster_flash_offset[mz.weapon];
	origin[0] = ent->current.origin[0] + forward[0] * ofs[0] + right[0] * ofs[1];
	origin[1] = ent->current.origin[1] + forward[1] * ofs[0] + right[1] * ofs[1];
	origin[2] = ent->current.origin[2] + forward[2] * ofs[0] + right[2] * ofs[1] + ofs[2];
	dl = CL_AllocDlight(mz.entity);
	VectorCopy(origin,  dl->origin);
	dl->radius = 200 + (Q_rand() & 31);
	dl->die = cl.time + 100;  // + 0.1;

	switch (mz.weapon)
	{
		case MZ2_INFANTRY_MACHINEGUN_1:
		case MZ2_INFANTRY_MACHINEGUN_2:
		case MZ2_INFANTRY_MACHINEGUN_3:
		case MZ2_INFANTRY_MACHINEGUN_4:
		case MZ2_INFANTRY_MACHINEGUN_5:
		case MZ2_INFANTRY_MACHINEGUN_6:
		case MZ2_INFANTRY_MACHINEGUN_7:
		case MZ2_INFANTRY_MACHINEGUN_8:
		case MZ2_INFANTRY_MACHINEGUN_9:
		case MZ2_INFANTRY_MACHINEGUN_10:
		case MZ2_INFANTRY_MACHINEGUN_11:
		case MZ2_INFANTRY_MACHINEGUN_12:
		case MZ2_INFANTRY_MACHINEGUN_13:
			DL_COLOR(1, 1, 0);
			CL_ParticleEffect(origin, vec3_origin, 0, 40);
			CL_SmokeAndFlash(origin);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_SOLDIER_MACHINEGUN_1:
		case MZ2_SOLDIER_MACHINEGUN_2:
		case MZ2_SOLDIER_MACHINEGUN_3:
		case MZ2_SOLDIER_MACHINEGUN_4:
		case MZ2_SOLDIER_MACHINEGUN_5:
		case MZ2_SOLDIER_MACHINEGUN_6:
		case MZ2_SOLDIER_MACHINEGUN_7:
		case MZ2_SOLDIER_MACHINEGUN_8:
			DL_COLOR(1, 1, 0);
			CL_ParticleEffect(origin, vec3_origin, 0, 40);
			CL_SmokeAndFlash(origin);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("soldier/solatck3.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_GUNNER_MACHINEGUN_1:
		case MZ2_GUNNER_MACHINEGUN_2:
		case MZ2_GUNNER_MACHINEGUN_3:
		case MZ2_GUNNER_MACHINEGUN_4:
		case MZ2_GUNNER_MACHINEGUN_5:
		case MZ2_GUNNER_MACHINEGUN_6:
		case MZ2_GUNNER_MACHINEGUN_7:
		case MZ2_GUNNER_MACHINEGUN_8:
			DL_COLOR(1, 1, 0);
			CL_ParticleEffect(origin, vec3_origin, 0, 40);
			CL_SmokeAndFlash(origin);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("gunner/gunatck2.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_ACTOR_MACHINEGUN_1:
		case MZ2_SUPERTANK_MACHINEGUN_1:
		case MZ2_SUPERTANK_MACHINEGUN_2:
		case MZ2_SUPERTANK_MACHINEGUN_3:
		case MZ2_SUPERTANK_MACHINEGUN_4:
		case MZ2_SUPERTANK_MACHINEGUN_5:
		case MZ2_SUPERTANK_MACHINEGUN_6:
		case MZ2_TURRET_MACHINEGUN:
			DL_COLOR(1, 1, 0);
			CL_ParticleEffect(origin, vec3_origin, 0, 40);
			CL_SmokeAndFlash(origin);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_BOSS2_MACHINEGUN_L1:
		case MZ2_BOSS2_MACHINEGUN_L2:
		case MZ2_BOSS2_MACHINEGUN_L3:
		case MZ2_BOSS2_MACHINEGUN_L4:
		case MZ2_BOSS2_MACHINEGUN_L5:
		case MZ2_CARRIER_MACHINEGUN_L1:
		case MZ2_CARRIER_MACHINEGUN_L2:
			DL_COLOR(1, 1, 0);
			CL_ParticleEffect(origin, vec3_origin, 0, 40);
			CL_SmokeAndFlash(origin);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NONE, 0);
			break;

		case MZ2_SOLDIER_BLASTER_1:
		case MZ2_SOLDIER_BLASTER_2:
		case MZ2_SOLDIER_BLASTER_3:
		case MZ2_SOLDIER_BLASTER_4:
		case MZ2_SOLDIER_BLASTER_5:
		case MZ2_SOLDIER_BLASTER_6:
		case MZ2_SOLDIER_BLASTER_7:
		case MZ2_SOLDIER_BLASTER_8:
		case MZ2_TURRET_BLASTER:
			DL_COLOR(1, 1, 0);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("soldier/solatck2.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_FLYER_BLASTER_1:
		case MZ2_FLYER_BLASTER_2:
			DL_COLOR(1, 1, 0);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("flyer/flyatck3.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_MEDIC_BLASTER_1:
			DL_COLOR(1, 1, 0);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("medic/medatck1.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_HOVER_BLASTER_1:
			DL_COLOR(1, 1, 0);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("hover/hovatck1.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_FLOAT_BLASTER_1:
			DL_COLOR(1, 1, 0);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("floater/fltatck1.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_SOLDIER_SHOTGUN_1:
		case MZ2_SOLDIER_SHOTGUN_2:
		case MZ2_SOLDIER_SHOTGUN_3:
		case MZ2_SOLDIER_SHOTGUN_4:
		case MZ2_SOLDIER_SHOTGUN_5:
		case MZ2_SOLDIER_SHOTGUN_6:
		case MZ2_SOLDIER_SHOTGUN_7:
		case MZ2_SOLDIER_SHOTGUN_8:
			DL_COLOR(1, 1, 0);
			CL_SmokeAndFlash(origin);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("soldier/solatck1.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_TANK_BLASTER_1:
		case MZ2_TANK_BLASTER_2:
		case MZ2_TANK_BLASTER_3:
			DL_COLOR(1, 1, 0);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_TANK_MACHINEGUN_1:
		case MZ2_TANK_MACHINEGUN_2:
		case MZ2_TANK_MACHINEGUN_3:
		case MZ2_TANK_MACHINEGUN_4:
		case MZ2_TANK_MACHINEGUN_5:
		case MZ2_TANK_MACHINEGUN_6:
		case MZ2_TANK_MACHINEGUN_7:
		case MZ2_TANK_MACHINEGUN_8:
		case MZ2_TANK_MACHINEGUN_9:
		case MZ2_TANK_MACHINEGUN_10:
		case MZ2_TANK_MACHINEGUN_11:
		case MZ2_TANK_MACHINEGUN_12:
		case MZ2_TANK_MACHINEGUN_13:
		case MZ2_TANK_MACHINEGUN_14:
		case MZ2_TANK_MACHINEGUN_15:
		case MZ2_TANK_MACHINEGUN_16:
		case MZ2_TANK_MACHINEGUN_17:
		case MZ2_TANK_MACHINEGUN_18:
		case MZ2_TANK_MACHINEGUN_19:
			DL_COLOR(1, 1, 0);
			CL_ParticleEffect(origin, vec3_origin, 0, 40);
			CL_SmokeAndFlash(origin);
			Q_snprintf(soundname, sizeof(soundname), "tank/tnkatk2%c.wav", 'a' + Q_rand() % 5);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound(soundname), 1, ATTN_NORM, 0);
			break;

		case MZ2_CHICK_ROCKET_1:
		case MZ2_TURRET_ROCKET:
			DL_COLOR(1, 0.5f, 0.2f);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("chick/chkatck2.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_TANK_ROCKET_1:
		case MZ2_TANK_ROCKET_2:
		case MZ2_TANK_ROCKET_3:
			DL_COLOR(1, 0.5f, 0.2f);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("tank/tnkatck1.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_SUPERTANK_ROCKET_1:
		case MZ2_SUPERTANK_ROCKET_2:
		case MZ2_SUPERTANK_ROCKET_3:
		case MZ2_BOSS2_ROCKET_1:
		case MZ2_BOSS2_ROCKET_2:
		case MZ2_BOSS2_ROCKET_3:
		case MZ2_BOSS2_ROCKET_4:
		case MZ2_CARRIER_ROCKET_1:
			//  case MZ2_CARRIER_ROCKET_2:
			//  case MZ2_CARRIER_ROCKET_3:
			//  case MZ2_CARRIER_ROCKET_4:
			DL_COLOR(1, 0.5f, 0.2f);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("tank/rocket.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_GUNNER_GRENADE_1:
		case MZ2_GUNNER_GRENADE_2:
		case MZ2_GUNNER_GRENADE_3:
		case MZ2_GUNNER_GRENADE_4:
			DL_COLOR(1, 0.5f, 0);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("gunner/gunatck3.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_GLADIATOR_RAILGUN_1:
		case MZ2_CARRIER_RAILGUN:
		case MZ2_WIDOW_RAIL:
			DL_COLOR(0.5f, 0.5f, 1.0f);
			break;

		case MZ2_MAKRON_BFG:
			DL_COLOR(0.5f, 1, 0.5f);
			//S_StartSound (NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("makron/bfg_firef.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_MAKRON_BLASTER_1:
		case MZ2_MAKRON_BLASTER_2:
		case MZ2_MAKRON_BLASTER_3:
		case MZ2_MAKRON_BLASTER_4:
		case MZ2_MAKRON_BLASTER_5:
		case MZ2_MAKRON_BLASTER_6:
		case MZ2_MAKRON_BLASTER_7:
		case MZ2_MAKRON_BLASTER_8:
		case MZ2_MAKRON_BLASTER_9:
		case MZ2_MAKRON_BLASTER_10:
		case MZ2_MAKRON_BLASTER_11:
		case MZ2_MAKRON_BLASTER_12:
		case MZ2_MAKRON_BLASTER_13:
		case MZ2_MAKRON_BLASTER_14:
		case MZ2_MAKRON_BLASTER_15:
		case MZ2_MAKRON_BLASTER_16:
		case MZ2_MAKRON_BLASTER_17:
			DL_COLOR(1, 1, 0);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("makron/blaster.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_JORG_MACHINEGUN_L1:
		case MZ2_JORG_MACHINEGUN_L2:
		case MZ2_JORG_MACHINEGUN_L3:
		case MZ2_JORG_MACHINEGUN_L4:
		case MZ2_JORG_MACHINEGUN_L5:
		case MZ2_JORG_MACHINEGUN_L6:
			DL_COLOR(1, 1, 0);
			CL_ParticleEffect(origin, vec3_origin, 0, 40);
			CL_SmokeAndFlash(origin);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("boss3/xfiref.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_JORG_MACHINEGUN_R1:
		case MZ2_JORG_MACHINEGUN_R2:
		case MZ2_JORG_MACHINEGUN_R3:
		case MZ2_JORG_MACHINEGUN_R4:
		case MZ2_JORG_MACHINEGUN_R5:
		case MZ2_JORG_MACHINEGUN_R6:
			DL_COLOR(1, 1, 0);
			CL_ParticleEffect(origin, vec3_origin, 0, 40);
			CL_SmokeAndFlash(origin);
			break;

		case MZ2_JORG_BFG_1:
			DL_COLOR(0.5f, 1, 0.5f);
			break;

		case MZ2_BOSS2_MACHINEGUN_R1:
		case MZ2_BOSS2_MACHINEGUN_R2:
		case MZ2_BOSS2_MACHINEGUN_R3:
		case MZ2_BOSS2_MACHINEGUN_R4:
		case MZ2_BOSS2_MACHINEGUN_R5:
		case MZ2_CARRIER_MACHINEGUN_R1:
		case MZ2_CARRIER_MACHINEGUN_R2:
			DL_COLOR(1, 1, 0);
			CL_ParticleEffect(origin, vec3_origin, 0, 40);
			CL_SmokeAndFlash(origin);
			break;

		case MZ2_STALKER_BLASTER:
		case MZ2_DAEDALUS_BLASTER:
		case MZ2_MEDIC_BLASTER_2:
		case MZ2_WIDOW_BLASTER:
		case MZ2_WIDOW_BLASTER_SWEEP1:
		case MZ2_WIDOW_BLASTER_SWEEP2:
		case MZ2_WIDOW_BLASTER_SWEEP3:
		case MZ2_WIDOW_BLASTER_SWEEP4:
		case MZ2_WIDOW_BLASTER_SWEEP5:
		case MZ2_WIDOW_BLASTER_SWEEP6:
		case MZ2_WIDOW_BLASTER_SWEEP7:
		case MZ2_WIDOW_BLASTER_SWEEP8:
		case MZ2_WIDOW_BLASTER_SWEEP9:
		case MZ2_WIDOW_BLASTER_100:
		case MZ2_WIDOW_BLASTER_90:
		case MZ2_WIDOW_BLASTER_80:
		case MZ2_WIDOW_BLASTER_70:
		case MZ2_WIDOW_BLASTER_60:
		case MZ2_WIDOW_BLASTER_50:
		case MZ2_WIDOW_BLASTER_40:
		case MZ2_WIDOW_BLASTER_30:
		case MZ2_WIDOW_BLASTER_20:
		case MZ2_WIDOW_BLASTER_10:
		case MZ2_WIDOW_BLASTER_0:
		case MZ2_WIDOW_BLASTER_10L:
		case MZ2_WIDOW_BLASTER_20L:
		case MZ2_WIDOW_BLASTER_30L:
		case MZ2_WIDOW_BLASTER_40L:
		case MZ2_WIDOW_BLASTER_50L:
		case MZ2_WIDOW_BLASTER_60L:
		case MZ2_WIDOW_BLASTER_70L:
		case MZ2_WIDOW_RUN_1:
		case MZ2_WIDOW_RUN_2:
		case MZ2_WIDOW_RUN_3:
		case MZ2_WIDOW_RUN_4:
		case MZ2_WIDOW_RUN_5:
		case MZ2_WIDOW_RUN_6:
		case MZ2_WIDOW_RUN_7:
		case MZ2_WIDOW_RUN_8:
			DL_COLOR(0, 1, 0);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_WIDOW_DISRUPTOR:
			DL_COLOR(-1, -1, -1);
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, S_RegisterSound("weapons/disint2.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_WIDOW_PLASMABEAM:
		case MZ2_WIDOW2_BEAMER_1:
		case MZ2_WIDOW2_BEAMER_2:
		case MZ2_WIDOW2_BEAMER_3:
		case MZ2_WIDOW2_BEAMER_4:
		case MZ2_WIDOW2_BEAMER_5:
		case MZ2_WIDOW2_BEAM_SWEEP_1:
		case MZ2_WIDOW2_BEAM_SWEEP_2:
		case MZ2_WIDOW2_BEAM_SWEEP_3:
		case MZ2_WIDOW2_BEAM_SWEEP_4:
		case MZ2_WIDOW2_BEAM_SWEEP_5:
		case MZ2_WIDOW2_BEAM_SWEEP_6:
		case MZ2_WIDOW2_BEAM_SWEEP_7:
		case MZ2_WIDOW2_BEAM_SWEEP_8:
		case MZ2_WIDOW2_BEAM_SWEEP_9:
		case MZ2_WIDOW2_BEAM_SWEEP_10:
		case MZ2_WIDOW2_BEAM_SWEEP_11:
			DL_RADIUS(300 + (Q_rand() & 100));
			DL_COLOR(1, 1, 0);
			DL_DIE(200);
			break;
	}
}

/*
==============================================================

PARTICLE MANAGEMENT

==============================================================
*/

static cparticle_t  *active_particles, *free_particles;

static cparticle_t  particles[MAX_PARTICLES];

static void CL_ClearParticles(void)
{
	int     i;
	free_particles = &particles[0];
	active_particles = NULL;

	for (i = 0; i < MAX_PARTICLES - 1; i++)
		particles[i].next = &particles[i + 1];

	particles[i].next = NULL;
}

cparticle_t *CL_AllocParticle(void)
{
	cparticle_t *p;

	if (!free_particles)
		return NULL;

	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->type = PARTICLE_NOPE;
	return p;
}

/*
===============
CL_ParticleEffect

Wall impact puffs
===============
*/
void CL_ParticleEffect(vec3_t org, vec3_t dir, int color, int count)
{
	int         i, j;
	cparticle_t *p;
	float       d;

	for (i = 0; i < count; i++)
	{
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		p->color = color + (Q_rand() & 7);
		d = Q_rand() & 31;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((int)(Q_rand() & 7) - 4) + d * dir[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0f;
		p->alphavel = -1.0f / (0.5f + frand() * 0.3f);
	}
}


/*
===============
CL_ParticleEffect2
===============
*/
void CL_ParticleEffect2(vec3_t org, vec3_t dir, int color, int count)
{
	int         i, j;
	cparticle_t *p;
	float       d;

	for (i = 0; i < count; i++)
	{
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		p->color = color;
		d = Q_rand() & 7;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((int)(Q_rand() & 7) - 4) + d * dir[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0f;
		p->alphavel = -1.0f / (0.5f + frand() * 0.3f);
	}
}


/*
===============
CL_TeleporterParticles
===============
*/
void CL_TeleporterParticles(vec3_t org)
{
	int         i, j;
	cparticle_t *p;

	for (i = 0; i < 8; i++)
	{
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		p->color = 0xdb;

		for (j = 0; j < 2; j++)
		{
			p->org[j] = org[j] - 16 + (Q_rand() & 31);
			p->vel[j] = crand() * 14;
		}

		p->org[2] = org[2] - 8 + (Q_rand() & 7);
		p->vel[2] = 80 + (Q_rand() & 7);
		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0f;
		p->alphavel = -0.5f;
	}
}

void R_Doom_Teleport_Fog(int, vec3_t);

void R_Q1_TeleportSound(int ent)
{
	const char *tmpstr;
	float v = frand() * 5;

	if (v < 1)
		tmpstr = "q1/misc/r_tele1.wav";
	else if (v < 2)
		tmpstr = "q1/misc/r_tele2.wav";
	else if (v < 3)
		tmpstr = "q1/misc/r_tele3.wav";
	else if (v < 4)
		tmpstr = "q1/misc/r_tele4.wav";
	else
		tmpstr = "q1/misc/r_tele5.wav";

	S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound(tmpstr), 1, ATTN_NORM, 0);
}

void R_Q1_TeleportSplash(int ent, vec3_t org)
{
	int			i, j, k;
	cparticle_t	*p;
	float		vel;
	vec3_t		dir;
	R_Q1_TeleportSound(mz.entity);

	for (i = -16; i < 16; i += 4)
		for (j = -16; j < 16; j += 4)
			for (k = -24; k < 32; k += 4)
			{
				p = CL_AllocParticle();

				if (!p)
					return;

				p->time = cl.time;
				p->die = cl.time + (0.2f + (Q_rand() & 7) * 0.02f) * 1000;
				p->color = -1;
				p->rgba.u32 = d_palettes[GAME_Q1][7 + (Q_rand() & 7)];
				p->type = PARTICLE_Q1_SLOWGRAV;
				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;
				p->org[0] = org[0] + i + (Q_rand() & 3);
				p->org[1] = org[1] + j + (Q_rand() & 3);
				p->org[2] = org[2] + k + (Q_rand() & 3);
				VectorClear(p->accel);
				p->alpha = 1;
				p->alphavel = 0;
				p->accel[2] = -PARTICLE_GRAVITY;
				VectorNormalize(dir);
				vel = 50 + (Q_rand() & 63);
				VectorScale(dir, vel, p->vel);
			}
}

void R_Q1_RocketTrail(vec3_t start, vec3_t end, int type)
{
	vec3_t		vec, move;
	float		len;
	int			j;
	cparticle_t	*p;
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	int dec = 3;
	VectorScale(vec, dec, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		VectorCopy(vec3_origin, p->vel);
		VectorClear(p->accel);
		p->alpha = 1;
		p->die = cl.time + 2000;

		/*if (type == 4)
		{	// slight blood
			p->type = PARTICLE_Q1_SLOWGRAV;
			p->color = 67 + (rand() & 3);
			for (j = 0; j<3; j++)
				p->org[j] = start[j] + ((rand() % 6) - 3);
			len -= 3;
		}

		else*/ if (type == 2)
		{
			// blood
			p->accel[2] = -PARTICLE_GRAVITY / 2;
			p->color = -1;
			p->rgba.u32 = d_palettes[GAME_Q1][67 + (rand() & 3)];
			p->type = PARTICLE_Q1_SLOWGRAV;

			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + ((rand() % 6) - 3);
		}
		else if (type == 6)
		{
			// voor trail
			p->type = PARTICLE_Q1_STATIC;
			p->color = -1;
			p->rgba.u32 = d_palettes[GAME_Q1][9 * 16 + 8 + (rand() & 3)];
			p->die = cl.time + 300;

			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + ((rand() & 15) - 8);
		}
		else if (type == 1)
		{
			// smoke smoke
			p->ramp = (rand() & 3) + 2;
			p->color = -1;
			p->rgba.u32 = d_palettes[GAME_Q1][ramp3[(int)p->ramp]];
			p->type = PARTICLE_Q1_FIRE;

			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + ((rand() % 6) - 3);

			p->accel[2] = PARTICLE_GRAVITY / 2;
		}
		else if (type == 0)
		{
			// rocket trail
			p->ramp = (rand() & 3);
			p->color = -1;
			p->rgba.u32 = d_palettes[GAME_Q1][ramp3[(int)p->ramp]];
			p->type = PARTICLE_Q1_FIRE;

			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + ((rand() % 6) - 3);

			p->accel[2] = PARTICLE_GRAVITY / 2;
		}
		else if (type == 3 || type == 5)
		{
			// tracer
			static int tracercount = 0;
			p->die = cl.time + 500;
			p->type = PARTICLE_Q1_STATIC;
			p->color = -1;

			if (type == 3)
				p->rgba.u32 = d_palettes[GAME_Q1][52 + ((tracercount & 4) << 1)];
			else
				p->rgba.u32 = d_palettes[GAME_Q1][230 + ((tracercount & 4) << 1)];

			tracercount++;
			VectorCopy(start, p->org);

			if (tracercount & 1)
			{
				p->vel[0] = 30 * vec[1];
				p->vel[1] = 30 * -vec[0];
			}
			else
			{
				p->vel[0] = 30 * -vec[1];
				p->vel[1] = 30 * vec[0];
			}
		}

		VectorAdd(move, vec, move);
	}
}

/*
===============
CL_LogoutEffect

===============
*/
void Duke_Teleport(int entity, vec3_t org);

static void CL_LogoutEffect(gametype_t game, vec3_t org, int type)
{
	switch (game)
	{
		case GAME_DOOM:
			R_Doom_Teleport_Fog(mz.entity, org);
			break;

		case GAME_Q1:
			R_Q1_TeleportSplash(mz.entity, org);
			break;

		case GAME_Q2:
		{
			S_StartSound(NULL, mz.entity, CHAN_WEAPON, sfx_q2_grenadelauncher_fire, 1, ATTN_NORM, 0);
			int         i, j;
			cparticle_t *p;

			for (i = 0; i < 500; i++)
			{
				p = CL_AllocParticle();

				if (!p)
					return;

				p->time = cl.time;

				if (type == MZ_LOGIN)
					p->color = 0xd0 + (Q_rand() & 7); // green
				else if (type == MZ_LOGOUT)
					p->color = 0x40 + (Q_rand() & 7); // red
				else
					p->color = 0xe0 + (Q_rand() & 7); // yellow

				p->org[0] = org[0] - 16 + frand() * 32;
				p->org[1] = org[1] - 16 + frand() * 32;
				p->org[2] = org[2] - 24 + frand() * 56;

				for (j = 0; j < 3; j++)
					p->vel[j] = crand() * 20;

				p->accel[0] = p->accel[1] = 0;
				p->accel[2] = -PARTICLE_GRAVITY;
				p->alpha = 1.0f;
				p->alphavel = -1.0f / (1.0f + frand() * 0.3f);
			}

			break;
		}

		case GAME_DUKE:
			Duke_Teleport(mz.entity, org);
			break;
	}
}


/*
===============
CL_ItemRespawnParticles

===============
*/
void CL_ItemRespawnParticles(vec3_t org)
{
	int         i, j;
	cparticle_t *p;

	for (i = 0; i < 64; i++)
	{
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		p->color = 0xd4 + (Q_rand() & 3); // green
		p->org[0] = org[0] + crand() * 8;
		p->org[1] = org[1] + crand() * 8;
		p->org[2] = org[2] + crand() * 8;

		for (j = 0; j < 3; j++)
			p->vel[j] = crand() * 8;

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY * 0.2f;
		p->alpha = 1.0f;
		p->alphavel = -1.0f / (1.0f + frand() * 0.3f);
	}
}


/*
===============
CL_ExplosionParticles
===============
*/
void CL_ExplosionParticles(vec3_t org)
{
	int         i, j;
	cparticle_t *p;

	for (i = 0; i < 256; i++)
	{
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		p->color = 0xe0 + (Q_rand() & 7);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((int)(Q_rand() % 32) - 16);
			p->vel[j] = (int)(Q_rand() % 384) - 192;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0f;
		p->alphavel = -0.8f / (0.5f + frand() * 0.3f);
	}
}

/*
===============
CL_BigTeleportParticles
===============
*/
void CL_BigTeleportParticles(vec3_t org)
{
	static const byte   colortable[4] = {2 * 8, 13 * 8, 21 * 8, 18 * 8};
	int         i;
	cparticle_t *p;
	float       angle, dist;

	for (i = 0; i < 4096; i++)
	{
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		p->color = colortable[Q_rand() & 3];
		angle = (Q_rand() & 1023) * (M_PI * 2 / 1023);
		dist = Q_rand() & 31;
		p->org[0] = org[0] + cos(angle) * dist;
		p->vel[0] = cos(angle) * (70 + (Q_rand() & 63));
		p->accel[0] = -cos(angle) * 100;
		p->org[1] = org[1] + sin(angle) * dist;
		p->vel[1] = sin(angle) * (70 + (Q_rand() & 63));
		p->accel[1] = -sin(angle) * 100;
		p->org[2] = org[2] + 8 + (Q_rand() % 90);
		p->vel[2] = -100 + (int)(Q_rand() & 31);
		p->accel[2] = PARTICLE_GRAVITY * 4;
		p->alpha = 1.0f;
		p->alphavel = -0.3f / (0.5f + frand() * 0.3f);
	}
}


/*
===============
CL_BlasterParticles

Wall impact puffs
===============
*/
void CL_BlasterParticles(vec3_t org, vec3_t dir)
{
	int         i, j;
	cparticle_t *p;
	float       d;

	for (i = 0; i < 40; i++)
	{
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		p->color = 0xe0 + (Q_rand() & 7);
		d = Q_rand() & 15;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((int)(Q_rand() & 7) - 4) + d * dir[j];
			p->vel[j] = dir[j] * 30 + crand() * 40;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0f;
		p->alphavel = -1.0f / (0.5f + frand() * 0.3f);
	}
}


/*
===============
CL_BlasterTrail

===============
*/
void CL_BlasterTrail(vec3_t start, vec3_t end)
{
	vec3_t      move;
	vec3_t      vec;
	float       len;
	int         j;
	cparticle_t *p;
	int         dec;
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	dec = 5;
	VectorScale(vec, 5, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;
		p = CL_AllocParticle();

		if (!p)
			return;

		VectorClear(p->accel);
		p->time = cl.time;
		p->alpha = 1.0f;
		p->alphavel = -1.0f / (0.3f + frand() * 0.2f);
		p->color = 0xe0;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand();
			p->vel[j] = crand() * 5;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}

/*
===============
CL_FlagTrail

===============
*/
void CL_FlagTrail(vec3_t start, vec3_t end, int color)
{
	vec3_t      move;
	vec3_t      vec;
	float       len;
	int         j;
	cparticle_t *p;
	int         dec;
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0)
	{
		len -= dec;
		p = CL_AllocParticle();

		if (!p)
			return;

		VectorClear(p->accel);
		p->time = cl.time;
		p->alpha = 1.0f;
		p->alphavel = -1.0f / (0.8f + frand() * 0.2f);
		p->color = color;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand() * 16;
			p->vel[j] = crand() * 5;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}

/*
===============
CL_DiminishingTrail

===============
*/
void CL_DiminishingTrail(vec3_t start, vec3_t end, centity_t *old, int flags)
{
	vec3_t      move;
	vec3_t      vec;
	float       len;
	int         j;
	cparticle_t *p;
	float       dec;
	float       orgscale;
	float       velscale;
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	dec = 0.5f;
	VectorScale(vec, dec, vec);

	if (old->trailcount > 900)
	{
		orgscale = 4;
		velscale = 15;
	}
	else if (old->trailcount > 800)
	{
		orgscale = 2;
		velscale = 10;
	}
	else
	{
		orgscale = 1;
		velscale = 5;
	}

	while (len > 0)
	{
		len -= dec;

		// drop less particles as it flies
		if ((Q_rand() & 1023) < old->trailcount)
		{
			p = CL_AllocParticle();

			if (!p)
				return;

			VectorClear(p->accel);
			p->time = cl.time;

			if (flags & EF_GIB)
			{
				p->alpha = 1.0f;
				p->alphavel = -1.0f / (1 + frand() * 0.4f);
				p->color = 0xe8 + (Q_rand() & 7);

				for (j = 0; j < 3; j++)
				{
					p->org[j] = move[j] + crand() * orgscale;
					p->vel[j] = crand() * velscale;
					p->accel[j] = 0;
				}

				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else if (flags & EF_GREENGIB)
			{
				p->alpha = 1.0f;
				p->alphavel = -1.0f / (1 + frand() * 0.4f);
				p->color = 0xdb + (Q_rand() & 7);

				for (j = 0; j < 3; j++)
				{
					p->org[j] = move[j] + crand() * orgscale;
					p->vel[j] = crand() * velscale;
					p->accel[j] = 0;
				}

				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else
			{
				p->alpha = 1.0f;
				p->alphavel = -1.0f / (1 + frand() * 0.2f);
				p->color = 4 + (Q_rand() & 7);

				for (j = 0; j < 3; j++)
				{
					p->org[j] = move[j] + crand() * orgscale;
					p->vel[j] = crand() * velscale;
				}

				p->accel[2] = 20;
			}
		}

		old->trailcount -= 5;

		if (old->trailcount < 100)
			old->trailcount = 100;

		VectorAdd(move, vec, move);
	}
}

/*
===============
CL_RocketTrail

===============
*/
void CL_RocketTrail(vec3_t start, vec3_t end, centity_t *old)
{
	vec3_t      move;
	vec3_t      vec;
	float       len;
	int         j;
	cparticle_t *p;
	float       dec;
	// smoke
	CL_DiminishingTrail(start, end, old, EF_ROCKET);
	// fire
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	dec = 1;
	VectorScale(vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if ((Q_rand() & 7) == 0)
		{
			p = CL_AllocParticle();

			if (!p)
				return;

			VectorClear(p->accel);
			p->time = cl.time;
			p->alpha = 1.0f;
			p->alphavel = -1.0f / (1 + frand() * 0.2f);
			p->color = 0xdc + (Q_rand() & 3);

			for (j = 0; j < 3; j++)
			{
				p->org[j] = move[j] + crand() * 5;
				p->vel[j] = crand() * 20;
			}

			p->accel[2] = -PARTICLE_GRAVITY;
		}

		VectorAdd(move, vec, move);
	}
}

/*
===============
CL_RailTrail

===============
*/
void CL_OldRailTrail(void)
{
	vec3_t      move;
	vec3_t      vec;
	float       len;
	int         j;
	cparticle_t *p;
	float       dec;
	vec3_t      right, up;
	int         i;
	float       d, c, s;
	vec3_t      dir;
	byte        clr = 0x74;
	VectorCopy(te.pos1, move);
	VectorSubtract(te.pos2, te.pos1, vec);
	len = VectorNormalize(vec);
	MakeNormalVectors(vec, right, up);

	for (i = 0; i < len; i++)
	{
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		VectorClear(p->accel);
		d = i * 0.1f;
		c = cos(d);
		s = sin(d);
		VectorScale(right, c, dir);
		VectorMA(dir, s, up, dir);
		p->alpha = 1.0f;
		p->alphavel = -1.0f / (1 + frand() * 0.2f);
		p->color = clr + (Q_rand() & 7);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + dir[j] * 3;
			p->vel[j] = dir[j] * 6;
		}

		VectorAdd(move, vec, move);
	}

	dec = 0.75f;
	VectorScale(vec, dec, vec);
	VectorCopy(te.pos1, move);

	while (len > 0)
	{
		len -= dec;
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		VectorClear(p->accel);
		p->alpha = 1.0f;
		p->alphavel = -1.0f / (0.6f + frand() * 0.2f);
		p->color = Q_rand() & 15;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand() * 3;
			p->vel[j] = crand() * 3;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}


/*
===============
CL_BubbleTrail

===============
*/
void CL_BubbleTrail(vec3_t start, vec3_t end)
{
	vec3_t      move;
	vec3_t      vec;
	float       len;
	int         i, j;
	cparticle_t *p;
	float       dec;
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	dec = 32;
	VectorScale(vec, dec, vec);

	for (i = 0; i < len; i += dec)
	{
		p = CL_AllocParticle();

		if (!p)
			return;

		VectorClear(p->accel);
		p->time = cl.time;
		p->alpha = 1.0f;
		p->alphavel = -1.0f / (1 + frand() * 0.2f);
		p->color = 4 + (Q_rand() & 7);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand() * 2;
			p->vel[j] = crand() * 5;
		}

		p->vel[2] += 6;
		VectorAdd(move, vec, move);
	}
}


/*
===============
CL_FlyParticles
===============
*/

#define BEAMLENGTH  16

static void CL_FlyParticles(vec3_t origin, int count)
{
	int         i;
	cparticle_t *p;
	float       angle;
	float       sp, sy, cp, cy;
	vec3_t      forward;
	float       dist;
	float       ltime;

	if (count > NUMVERTEXNORMALS)
		count = NUMVERTEXNORMALS;

	ltime = cl.time * 0.001f;

	for (i = 0; i < count; i += 2)
	{
		angle = ltime * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = ltime * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		dist = sin(ltime + i) * 64;
		p->org[0] = origin[0] + bytedirs[i][0] * dist + forward[0] * BEAMLENGTH;
		p->org[1] = origin[1] + bytedirs[i][1] * dist + forward[1] * BEAMLENGTH;
		p->org[2] = origin[2] + bytedirs[i][2] * dist + forward[2] * BEAMLENGTH;
		VectorClear(p->vel);
		VectorClear(p->accel);
		p->color = 0;
		p->alpha = 1;
		p->alphavel = INSTANT_PARTICLE;
	}
}

void CL_FlyEffect(centity_t *ent, vec3_t origin)
{
	int     n;
	int     count;
	int     starttime;

	if (ent->fly_stoptime < cl.time)
	{
		starttime = cl.time;
		ent->fly_stoptime = cl.time + 60000;
	}
	else
		starttime = ent->fly_stoptime - 60000;

	n = cl.time - starttime;

	if (n < 20000)
		count = n * NUMVERTEXNORMALS / 20000;
	else
	{
		n = ent->fly_stoptime - cl.time;

		if (n < 20000)
			count = n * NUMVERTEXNORMALS / 20000;
		else
			count = NUMVERTEXNORMALS;
	}

	CL_FlyParticles(origin, count);
}


/*
===============
CL_BfgParticles
===============
*/
void CL_BfgParticles(entity_t *ent)
{
	int         i;
	cparticle_t *p;
	float       angle;
	float       sp, sy, cp, cy;
	vec3_t      forward;
	float       dist;
	vec3_t      v;
	float       ltime;
	ltime = cl.time * 0.001f;

	for (i = 0; i < NUMVERTEXNORMALS; i++)
	{
		angle = ltime * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = ltime * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		dist = sin(ltime + i) * 64;
		p->org[0] = ent->origin[0] + bytedirs[i][0] * dist + forward[0] * BEAMLENGTH;
		p->org[1] = ent->origin[1] + bytedirs[i][1] * dist + forward[1] * BEAMLENGTH;
		p->org[2] = ent->origin[2] + bytedirs[i][2] * dist + forward[2] * BEAMLENGTH;
		VectorClear(p->vel);
		VectorClear(p->accel);
		VectorSubtract(p->org, ent->origin, v);
		dist = VectorLength(v) / 90.0f;
		p->color = floor(0xd0 + dist * 7);
		p->alpha = 1.0f - dist;
		p->alphavel = INSTANT_PARTICLE;
	}
}


/*
===============
CL_BFGExplosionParticles
===============
*/
//FIXME combined with CL_ExplosionParticles
void CL_BFGExplosionParticles(vec3_t org)
{
	int         i, j;
	cparticle_t *p;

	for (i = 0; i < 256; i++)
	{
		p = CL_AllocParticle();

		if (!p)
			return;

		p->time = cl.time;
		p->color = 0xd0 + (Q_rand() & 7);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((int)(Q_rand() % 32) - 16);
			p->vel[j] = (int)(Q_rand() % 384) - 192;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0f;
		p->alphavel = -0.8f / (0.5f + frand() * 0.3f);
	}
}


/*
===============
CL_TeleportParticles

===============
*/
void CL_TeleportParticles(vec3_t org)
{
	int         i, j, k;
	cparticle_t *p;
	float       vel;
	vec3_t      dir;

	for (i = -16; i <= 16; i += 4)
		for (j = -16; j <= 16; j += 4)
			for (k = -16; k <= 32; k += 4)
			{
				p = CL_AllocParticle();

				if (!p)
					return;

				p->time = cl.time;
				p->color = 7 + (Q_rand() & 7);
				p->alpha = 1.0f;
				p->alphavel = -1.0f / (0.3f + (Q_rand() & 7) * 0.02f);
				p->org[0] = org[0] + i + (Q_rand() & 3);
				p->org[1] = org[1] + j + (Q_rand() & 3);
				p->org[2] = org[2] + k + (Q_rand() & 3);
				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;
				VectorNormalize(dir);
				vel = 50 + (Q_rand() & 63);
				VectorScale(dir, vel, p->vel);
				p->accel[0] = p->accel[1] = 0;
				p->accel[2] = -PARTICLE_GRAVITY;
			}
}

extern int          r_numparticles;
extern particle_t   r_particles[MAX_PARTICLES];

/*
===============
CL_AddParticles
===============
*/
void CL_AddParticles(void)
{
	cparticle_t     *p, *next;
	float           alpha;
	float           time = 0, time2;
	int             color;
	cparticle_t     *active, *tail;
	particle_t      *part;
	active = NULL;
	tail = NULL;

	for (p = active_particles; p; p = next)
	{
		next = p->next;

		if (p->alphavel != INSTANT_PARTICLE)
		{
			time = (cl.time - p->time) * 0.001f;
			alpha = p->alpha + time * p->alphavel;

			switch (p->type)
			{
				case PARTICLE_Q1_EXPLODE:
					p->ramp += cls.frametime * 10;

					if (p->ramp >= 8)
						p->die = -1;
					else
					{
						p->color = -1;
						p->rgba.u32 = d_palettes[GAME_Q1][ramp1[(int)p->ramp]];
					}

					for (int i = 0; i < 3; i++)
						p->vel[i] += p->vel[i] * (cls.frametime * cls.frametime * 4);

					break;

				case PARTICLE_Q1_EXPLODE2:
					p->ramp += cls.frametime * 15;

					if (p->ramp >= 8)
						p->die = -1;
					else
					{
						p->color = -1;
						p->rgba.u32 = d_palettes[GAME_Q1][ramp2[(int)p->ramp]];
					}

					for (int i = 0; i < 3; i++)
						p->vel[i] -= p->vel[i] * cls.frametime * cls.frametime;

					break;

				case PARTICLE_Q1_BLOB1:
					for (int i = 0; i < 2; i++)
						p->vel[i] -= p->vel[i] * cls.frametime * cls.frametime;

					break;

				case PARTICLE_Q1_BLOB2:
					for (int i = 0; i < 3; i++)
						p->vel[i] -= p->vel[i] * cls.frametime * cls.frametime;

					break;

				case PARTICLE_Q1_FIRE:
					alpha = (6 - p->ramp) / 6;
					p->ramp += cls.frametime * 5;

					if (p->ramp >= 6)
						p->die = -1;
					else
					{
						p->color = -1;
						p->rgba.u32 = d_palettes[GAME_Q1][ramp3[(int)p->ramp]];
					}

					break;
			}

			if (alpha <= 0 || (p->type != PARTICLE_NOPE && p->die < cl.time))
			{
				// faded out
				p->next = free_particles;
				free_particles = p;
				continue;
			}
		}
		else
			alpha = p->alpha;

		if (r_numparticles >= MAX_PARTICLES)
			break;

		part = &r_particles[r_numparticles++];
		p->next = NULL;

		if (!tail)
			active = tail = p;
		else
		{
			tail->next = p;
			tail = p;
		}

		if (alpha > 1.0f)
			alpha = 1;

		color = p->color;
		time2 = time * time;
		part->origin[0] = p->org[0] + p->vel[0] * time + p->accel[0] * time2;
		part->origin[1] = p->org[1] + p->vel[1] * time + p->accel[1] * time2;
		part->origin[2] = p->org[2] + p->vel[2] * time + p->accel[2] * time2;

		if (color == -1)
		{
			part->rgba.u8[0] = p->rgba.u8[0];
			part->rgba.u8[1] = p->rgba.u8[1];
			part->rgba.u8[2] = p->rgba.u8[2];
			part->rgba.u8[3] = p->rgba.u8[3] * alpha;
		}

		part->color = color;
		part->alpha = alpha;

		if (p->alphavel == INSTANT_PARTICLE)
		{
			p->alphavel = 0.0f;
			p->alpha = 0.0f;
		}

		if (p->type != PARTICLE_NOPE)
			part->type = PARTICLE_OLDSCHOOL;
		else
			part->type = PARTICLE_NOPE;
	}

	active_particles = active;
}


/*
==============
CL_ClearEffects

==============
*/
void CL_ClearEffects(void)
{
	CL_ClearParticles();
	CL_ClearDlights();
}

void CL_InitEffects(void)
{
	int i, j;

	for (i = 0; i < NUMVERTEXNORMALS; i++)
		for (j = 0; j < 3; j++)
			avelocities[i][j] = (Q_rand() & 255) * 0.01f;
}

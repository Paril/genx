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



/*
======================================================================

INTERMISSION

======================================================================
*/

void MoveClientToIntermission(edict_t *ent)
{
	if (deathmatch->value || coop->value)
		ent->client->showscores = true;

	VectorCopy(level.intermission_origin, ent->s.origin);
	ent->client->ps.pmove.origin[0] = level.intermission_origin[0] * 8;
	ent->client->ps.pmove.origin[1] = level.intermission_origin[1] * 8;
	ent->client->ps.pmove.origin[2] = level.intermission_origin[2] * 8;
	VectorCopy(level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	memset(ent->client->ps.guns, 0, sizeof(ent->client->ps.guns));
	ent->client->ps.blend[3] = 0;
	ent->client->ps.rdflags &= ~RDF_UNDERWATER;
	// clean up powerup info
	ent->client->quad_time = 0;
	ent->client->invincible_time = 0;
	ent->client->breather_time = 0;
	ent->client->enviro_time = 0;
	ent->client->grenade_blew_up = false;
	ent->client->grenade_time = 0;
	ent->freeze_time = 0;
	ent->viewheight = 0;
	ent->s.modelindex = 0;
	ent->s.modelindex2 = 0;
	ent->s.modelindex3 = 0;
	ent->s.modelindex = 0;
	ent->s.effects = 0;
	ent->s.sound = 0;
	ent->solid = SOLID_NOT;

	// add the layout

	if (deathmatch->value || coop->value)
	{
		DeathmatchScoreboardMessage(ent, NULL);
		gi.unicast(ent, true);
	}
}

void BeginIntermission(edict_t *targ)
{
	int     i, n;
	edict_t *ent, *client;

	if (level.intermissiontime)
		return;     // already activated

	game.autosaved = false;

	// respawn any dead clients
	for (i = 0 ; i < maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;

		if (!client->inuse)
			continue;

		if (client->health <= 0)
			respawn(client);
	}

	level.intermissiontime = level.time;
	level.changemap = targ->map;

	if (strstr(level.changemap, "*"))
	{
		if (coop->value)
		{
			for (i = 0 ; i < maxclients->value ; i++)
			{
				client = g_edicts + 1 + i;

				if (!client->inuse)
					continue;

				// strip players of all keys between units
				for (n = 0; n < ITI_TOTAL; n++)
				{
					if (itemlist[n].flags & IT_KEY)
						client->client->pers.inventory[n] = 0;
				}
			}
		}
	}
	else
	{
		if (!deathmatch->value)
		{
			level.exitintermission = 1;     // go immediately to the next level
			return;
		}
	}

	level.exitintermission = 0;
	// find an intermission spot
	ent = G_FindByType(NULL, ET_INFO_PLAYER_INTERMISSION);

	if (!ent)
	{
		// the map creator forgot to put in an intermission point...
		ent = G_FindByType(NULL, ET_INFO_PLAYER_START);

		if (!ent)
			ent = G_FindByType(NULL, ET_INFO_PLAYER_DEATHMATCH);
	}
	else
	{
		// chose one of four spots
		i = Q_rand() & 3;

		while (i--)
		{
			ent = G_FindByType(ent, ET_INFO_PLAYER_INTERMISSION);

			if (!ent)   // wrap around the list
				ent = G_FindByType(ent, ET_INFO_PLAYER_INTERMISSION);
		}
	}

	VectorCopy(ent->s.origin, level.intermission_origin);
	VectorCopy(ent->s.angles, level.intermission_angle);

	// move all clients to the intermission point
	for (i = 0 ; i < maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;

		if (!client->inuse)
			continue;

		MoveClientToIntermission(client);
	}
}


/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage(edict_t *ent, edict_t *killer)
{
	char    entry[1024];
	char    string[1400];
	int     stringlength;
	int     i, j, k;
	int     sorted[MAX_CLIENTS];
	int     sortedscores[MAX_CLIENTS];
	int     score, total;
	int     x, y;
	gclient_t   *cl;
	edict_t     *cl_ent;
	char    *tag;
	// sort the clients by score
	total = 0;

	for (i = 0 ; i < game.maxclients ; i++)
	{
		cl_ent = g_edicts + 1 + i;

		if (!cl_ent->inuse || game.clients[i].resp.spectator)
			continue;

		score = game.clients[i].resp.score;

		for (j = 0 ; j < total ; j++)
		{
			if (score > sortedscores[j])
				break;
		}

		for (k = total ; k > j ; k--)
		{
			sorted[k] = sorted[k - 1];
			sortedscores[k] = sortedscores[k - 1];
		}

		sorted[j] = i;
		sortedscores[j] = score;
		total++;
	}

	// print level name and exit rules
	string[0] = 0;
	stringlength = strlen(string);

	// add the clients in sorted order
	if (total > 12)
		total = 12;

	for (i = 0 ; i < total ; i++)
	{
		cl = &game.clients[sorted[i]];
		cl_ent = g_edicts + 1 + sorted[i];
		x = (i >= 6) ? 160 : 0;
		y = 32 + 32 * (i % 6);

		// add a dogtag
		if (cl_ent == ent)
			tag = "tag1";
		else if (cl_ent == killer)
			tag = "tag2";
		else
			tag = NULL;

		if (tag)
		{
			Q_snprintf(entry, sizeof(entry),
				"xv %i yv %i picn %s ", x + 32, y, tag);
			j = strlen(entry);

			if (stringlength + j > 1024)
				break;

			strcpy(string + stringlength, entry);
			stringlength += j;
		}

		// send the layout
		Q_snprintf(entry, sizeof(entry),
			"client %i %i %i %i %i %i ",
			x, y, sorted[i], cl->resp.score, cl->ping, (int)((level.time - cl->resp.entertime) / 6000));
		j = strlen(entry);

		if (stringlength + j > 1024)
			break;

		strcpy(string + stringlength, entry);
		stringlength += j;
	}

	MSG_WriteByte(svc_layout);
	MSG_WriteString(string);
}


/*
==================
DeathmatchScoreboard

Draw instead of help message.
Note that it isn't that hard to overflow the 1400 byte message limit!
==================
*/
void DeathmatchScoreboard(edict_t *ent)
{
	DeathmatchScoreboardMessage(ent, ent->enemy);
	gi.unicast(ent, true);
}


/*
==================
Cmd_Score_f

Display the scoreboard
==================
*/
void Cmd_Score_f(edict_t *ent)
{
	ent->client->showinventory = false;
	ent->client->showhelp = false;

	if (!deathmatch->value && !coop->value)
		return;

	if (ent->client->showscores)
	{
		ent->client->showscores = false;
		return;
	}

	ent->client->showscores = true;
	DeathmatchScoreboard(ent);
}


/*
==================
HelpComputer

Draw help computer.
==================
*/
void HelpComputer(edict_t *ent)
{
	char    string[1024];
	char    *sk;

	if (skill->value == 0)
		sk = "easy";
	else if (skill->value == 1)
		sk = "medium";
	else if (skill->value == 2)
		sk = "hard";
	else
		sk = "hard+";

	// send the layout
	Q_snprintf(string, sizeof(string),
		"xv 32 yv 8 picn help "         // background
		"xv 202 yv 12 string2 \"%s\" "      // skill
		"xv 0 yv 24 cstring2 \"%s\" "       // level name
		"xv 0 yv 54 cstring2 \"%s\" "       // help 1
		"xv 0 yv 110 cstring2 \"%s\" "      // help 2
		"xv 50 yv 164 string2 \" kills     goals    secrets\" "
		"xv 50 yv 172 string2 \"%3i/%3i     %i/%i       %i/%i\" ",
		sk,
		level.level_name,
		game.helpmessage1,
		game.helpmessage2,
		level.killed_monsters, level.total_monsters,
		level.found_goals, level.total_goals,
		level.found_secrets, level.total_secrets);
	MSG_WriteByte(svc_layout);
	MSG_WriteString(string);
	gi.unicast(ent, true);
}


/*
==================
Cmd_Help_f

Display the current help message
==================
*/
void Cmd_Help_f(edict_t *ent)
{
	// this is for backwards compatability
	if (deathmatch->value)
	{
		Cmd_Score_f(ent);
		return;
	}

	ent->client->showinventory = false;
	ent->client->showscores = false;

	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))
	{
		ent->client->showhelp = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->pers.helpchanged = 0;
	HelpComputer(ent);
}


//=======================================================================

#define ST_EVILGRINCOUNT		(2*game.framerate)
#define ST_STRAIGHTFACECOUNT	(game.framerate/2)
#define ST_TURNCOUNT		(1*game.framerate)
#define ST_RAMPAGEDELAY		(2*game.framerate)

const int ST_MUCHPAIN			= 20;

// Stolen from Doom's source
int ST_calcPainOffset(edict_t *plyr)
{
	int health = max(0, min(100, plyr->health));

	if (health != plyr->client->old_health)
	{
		plyr->client->lastcalc = ST_FACESTRIDE * (((100 - health) * ST_NUMPAINFACES) / 101);
		plyr->client->old_health = health;
	}

	return plyr->client->lastcalc;
}

//
// This is a not-very-pretty routine which handles
//  the face states and their timing.
// the precedence of expressions is:
//  dead > evil grin > turned head > straight ahead
//
void ST_updateFaceWidget(edict_t *plyr)
{
	if (plyr->client->priority < 10)
	{
		// dead
		if (plyr->health <= 0)
		{
			plyr->client->priority = 9;
			plyr->client->st_faceindex = ST_DEADFACE;
			plyr->client->st_facecount = 1;
		}
	}

	if (plyr->client->priority < 9)
	{
		if (time_diff(plyr->client->pickup_msg_time, level.time) >= (gtime_t)(3000 - game.frametime))
		{
			// picking up bonus
			if ((plyr->client->pickup_item->flags & IT_WEAPON) && plyr->client->pickup_item_first)
			{
				// evil grin if just picked up weapon
				plyr->client->priority = 8;
				plyr->client->st_facecount = ST_EVILGRINCOUNT;
				plyr->client->st_faceindex = ST_calcPainOffset(plyr) + ST_EVILGRINOFFSET;
			}
		}
	}

	if (plyr->client->priority < 8)
	{
		if (((plyr->meansOfDeath.time + game.frametime) >= level.time)
			&& plyr->meansOfDeath.attacker
			&& plyr->meansOfDeath.attacker != plyr)
		{
			// being attacked
			plyr->client->priority = 7;

			if (plyr->health - plyr->client->old_health > ST_MUCHPAIN)
			{
				plyr->client->st_facecount = ST_TURNCOUNT;
				plyr->client->st_faceindex = ST_calcPainOffset(plyr) + ST_OUCHOFFSET;
			}
			else
			{
				plyr->client->st_facecount = ST_TURNCOUNT;
				plyr->client->st_faceindex = ST_calcPainOffset(plyr);
				float yaw_sub = anglemod((plyr->client->v_angle[1] - plyr->client->damage_dir) + 180);

				if (yaw_sub <= 45 || yaw_sub >= (360 - 45))
				{
					// head-on
					plyr->client->st_faceindex += ST_RAMPAGEOFFSET;
				}
				else if (yaw_sub > 180)
				{
					// turn face right
					plyr->client->st_faceindex += ST_TURNOFFSET;
				}
				else
				{
					// turn face left
					plyr->client->st_faceindex += ST_TURNOFFSET + 1;
				}
			}
		}
	}

	if (plyr->client->priority < 7)
	{
		// getting hurt because of your own damn stupidity
		if (((plyr->meansOfDeath.time + game.frametime) >= level.time) &&
			plyr->meansOfDeath.attacker == plyr)
		{
			if (plyr->health - plyr->client->old_health > ST_MUCHPAIN)
			{
				plyr->client->priority = 7;
				plyr->client->st_facecount = ST_TURNCOUNT;
				plyr->client->st_faceindex = ST_calcPainOffset(plyr) + ST_OUCHOFFSET;
			}
			else
			{
				plyr->client->priority = 6;
				plyr->client->st_facecount = ST_TURNCOUNT;
				plyr->client->st_faceindex = ST_calcPainOffset(plyr) + ST_RAMPAGEOFFSET;
			}
		}
	}

	if (plyr->client->priority < 6)
	{
		// rapid firing
		if (plyr->client->buttons & BUTTON_ATTACK)
		{
			if (plyr->client->lastattackdown == -1)
				plyr->client->lastattackdown = ST_RAMPAGEDELAY;
			else if (!--plyr->client->lastattackdown)
			{
				plyr->client->priority = 5;
				plyr->client->st_faceindex = ST_calcPainOffset(plyr) + ST_RAMPAGEOFFSET;
				plyr->client->st_facecount = 1;
				plyr->client->lastattackdown = 1;
			}
		}
		else
			plyr->client->lastattackdown = -1;
	}

	if (plyr->client->priority < 5)
	{
		// invulnerability
		if ((plyr->flags & FL_GODMODE) || plyr->client->invincible_time > level.time)
		{
			plyr->client->priority = 4;
			plyr->client->st_faceindex = ST_GODFACE;
			plyr->client->st_facecount = 1;
		}
	}

	// look left or look right if the facecount has timed out
	if (!plyr->client->st_facecount)
	{
		plyr->client->st_faceindex = ST_calcPainOffset(plyr) + (Q_rand() % 3);
		plyr->client->st_facecount = ST_STRAIGHTFACECOUNT;
		plyr->client->priority = 0;
	}

	plyr->client->st_facecount--;
	plyr->client->ps.stats.doom.face = plyr->client->st_faceindex;
}


/*
===============
G_SetStats
===============
*/
void G_SetStats(edict_t *ent)
{
	gitem_t     *item;
	itemid_e    index;
	int         power_armor_type = 0;

	//
	// health
	//
	if (ent->s.game == GAME_Q2)
		ent->client->ps.stats.q2.health_icon = level.pic_health;

	ent->client->ps.stats.health = ent->health;

	//
	// ammo
	//
	if (game_iteminfos[ent->s.game].ammo_usages[ITEM_INDEX(ent->client->pers.weapon)] <= 0)
	{
		ent->client->ps.stats.ammo_icon = 0;
		ent->client->ps.stats.ammo = 0;
	}
	else
	{
		//item = &itemlist[ent->client->gunstates[GUN_MAIN].ammo_index];
		//ent->client->ps.stats[STAT_AMMO_ICON] = gi.imageindex(item->icon);
		ent->client->ps.stats.ammo_icon = gi.imageindex(ent->client->pers.weapon->ammo_icon);
		ent->client->ps.stats.ammo = (int) floorf(PLAYER_SHOTS_FOR_WEAPON(ent, ent->client->pers.weapon));
	}

	//
	// armor
	//
	if (ent->s.game == GAME_Q2)
	{
		power_armor_type = PowerArmorType(ent);

		if (power_armor_type)
		{
			if (ent->client->pers.ammo < AMMO_PER_POWER_ARMOR_ABSORB)
			{
				// ran out of cells for power armor
				ent->flags &= ~FL_POWER_ARMOR;
				gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
				power_armor_type = 0;
			}
		}
	}

	index = ArmorIndex(ent);

	if (power_armor_type && (!index || (level.framenum & (8 * game.framediv))))
	{
		// flash between power armor and other armor icon
		ent->client->ps.stats.armor_icon = gi.imageindex("i_powershield");
		ent->client->ps.stats.armor = (int)(ent->client->pers.ammo * AMMO_PER_POWER_ARMOR_ABSORB);
	}
	else if (index)
	{
		item = GetItemByIndex(index);
		ent->client->ps.stats.armor_icon = gi.imageindex(item->icon);
		ent->client->ps.stats.armor = ent->client->pers.inventory[index];
	}
	else
	{
		ent->client->ps.stats.armor_icon = 0;
		ent->client->ps.stats.armor = 0;
	}

	//
	// pickup message
	//
	if (ent->s.game == GAME_Q2)
	{
		if (level.time > ent->client->pickup_msg_time)
		{
			ent->client->ps.stats.q2.pickup_icon = 0;
			ent->client->ps.stats.q2.pickup_string = 0;
		}
		else
		{
			ent->client->ps.stats.q2.pickup_icon = gi.imageindex(ent->client->pickup_item->icon);
			ent->client->ps.stats.q2.pickup_string = CS_ITEMS + ITEM_INDEX(ent->client->pickup_item);
		}

		//
		// timers
		//
		if (ent->client->quad_time > level.time)
		{
			ent->client->ps.stats.q2.timer_icon = gi.imageindex("p_quad");
			ent->client->ps.stats.q2.timer = (ent->client->quad_time - level.time) / 1000;
		}
		else if (ent->client->invincible_time > level.time)
		{
			ent->client->ps.stats.q2.timer_icon = gi.imageindex("p_invulnerability");
			ent->client->ps.stats.q2.timer = (ent->client->invincible_time - level.time) / 1000;
		}
		else if (ent->client->enviro_time > level.time)
		{
			ent->client->ps.stats.q2.timer_icon = gi.imageindex("p_envirosuit");
			ent->client->ps.stats.q2.timer = (ent->client->enviro_time - level.time) / 1000;
		}
		else if (ent->client->breather_time > level.time)
		{
			ent->client->ps.stats.q2.timer_icon = gi.imageindex("p_rebreather");
			ent->client->ps.stats.q2.timer = (ent->client->breather_time - level.time) / 1000;
		}
		else
		{
			ent->client->ps.stats.q2.timer_icon = 0;
			ent->client->ps.stats.q2.timer = 0;
		}

		//
		// selected item
		//
		if (ent->client->pers.selected_item == ITI_NULL)
			ent->client->ps.stats.q2.selected_icon = 0;
		else
			ent->client->ps.stats.q2.selected_icon = gi.imageindex(itemlist[ent->client->pers.selected_item].icon);
	}

	ent->client->ps.stats.selected_item = ent->client->pers.selected_item;
	//
	// layouts
	//
	ent->client->ps.stats.layouts = 0;

	if (deathmatch->value)
	{
		if (ent->client->pers.health <= 0 || level.intermissiontime
			|| ent->client->showscores)
			ent->client->ps.stats.layouts |= 1;

		if (ent->client->showinventory && ent->client->pers.health > 0)
			ent->client->ps.stats.layouts |= 2;
	}
	else
	{
		if (ent->client->showscores || ent->client->showhelp)
			ent->client->ps.stats.layouts |= 1;

		if (ent->client->showinventory && ent->client->pers.health > 0)
			ent->client->ps.stats.layouts |= 2;
	}

	//
	// frags
	//
	ent->client->ps.stats.frags = ent->client->resp.score;

	//
	// help icon / current weapon if not shown
	//
	if (ent->s.game == GAME_Q2)
	{
		if (ent->client->pers.helpchanged && (level.framenum & (8 * game.framediv)))
			ent->client->ps.stats.q2.help_icon = gi.imageindex("i_help");
		else if ((ent->client->pers.hand == CENTER_HANDED || ent->client->ps.fov > 91)
			&& ent->client->pers.weapon)
			ent->client->ps.stats.q2.help_icon = gi.imageindex(ent->client->pers.weapon->icon);
		else
			ent->client->ps.stats.q2.help_icon = 0;
	}

	ent->client->ps.stats.spectator = 0;

	// Q1 stuff
	if (ent->s.game == GAME_Q1)
	{
		ent->client->ps.stats.q1.ammo_shells = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_Q1_SHOTGUN));
		ent->client->ps.stats.q1.ammo_nails = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_Q1_NAILGUN));
		ent->client->ps.stats.q1.ammo_rockets = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_Q1_ROCKET_LAUNCHER));
		ent->client->ps.stats.q1.ammo_cells = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_Q1_THUNDERBOLT));
		ent->client->ps.stats.q1.items = 0;

		if (ent->client->pers.inventory[ITI_Q1_SHOTGUN])
			ent->client->ps.stats.q1.items |= IT_Q1_SHOTGUN;

		if (ent->client->pers.inventory[ITI_Q1_SUPER_SHOTGUN])
			ent->client->ps.stats.q1.items |= IT_Q1_SSHOTGUN;

		if (ent->client->pers.inventory[ITI_Q1_NAILGUN])
			ent->client->ps.stats.q1.items |= IT_Q1_NAILGUN;

		if (ent->client->pers.inventory[ITI_Q1_SUPER_NAILGUN])
			ent->client->ps.stats.q1.items |= IT_Q1_SNAILGUN;

		if (ent->client->pers.inventory[ITI_Q1_GRENADE_LAUNCHER])
			ent->client->ps.stats.q1.items |= IT_Q1_GLAUNCHER;

		if (ent->client->pers.inventory[ITI_Q1_ROCKET_LAUNCHER])
			ent->client->ps.stats.q1.items |= IT_Q1_RLAUNCHER;

		if (ent->client->pers.inventory[ITI_Q1_THUNDERBOLT])
			ent->client->ps.stats.q1.items |= IT_Q1_LIGHTNING;

		if (ent->client->quad_time > level.time)
			ent->client->ps.stats.q1.items |= IT_Q1_QUAD;

		if (ent->client->invincible_time > level.time)
			ent->client->ps.stats.q1.items |= IT_Q1_INVUL;

		if (ent->client->enviro_time > level.time)
			ent->client->ps.stats.q1.items |= IT_Q1_SUIT;

		if (ent->client->breather_time > level.time)
			ent->client->ps.stats.q1.items |= IT_Q1_INVIS;

		if (ent->health <= 0)
			ent->client->ps.stats.q1.face_anim = 0;
		else if (ent->client->ps.stats.q1.face_anim)
			--ent->client->ps.stats.q1.face_anim;

		if (ent->client->pers.weapon)
			ent->client->ps.stats.q1.cur_weap = ITEM_INDEX(ent->client->pers.weapon) - ITI_WEAPON_1;
		else
			ent->client->ps.stats.q1.cur_weap = 0;
	}
	// Doom stuff
	else if (ent->s.game == GAME_DOOM)
	{
		ent->client->ps.stats.doom.ammo_bullets = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DOOM_PISTOL));
		ent->client->ps.stats.doom.ammo_shells = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DOOM_SHOTGUN));
		ent->client->ps.stats.doom.ammo_rockets = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DOOM_ROCKET_LAUNCHER));
		ent->client->ps.stats.doom.ammo_cells = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DOOM_PLASMA_GUN));
		ent->client->ps.stats.doom.max_ammo_bullets = PLAYER_TOTAL_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DOOM_PISTOL));
		ent->client->ps.stats.doom.max_ammo_shells = PLAYER_TOTAL_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DOOM_SHOTGUN));
		ent->client->ps.stats.doom.max_ammo_rockets = PLAYER_TOTAL_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DOOM_ROCKET_LAUNCHER));
		ent->client->ps.stats.doom.max_ammo_cells = PLAYER_TOTAL_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DOOM_PLASMA_GUN));
		ent->client->ps.stats.doom.weapons = 0;

		if (ent->client->pers.inventory[ITI_DOOM_PISTOL])
			ent->client->ps.stats.doom.weapons |= IT_DOOM_PISTOL;

		if (ent->client->pers.inventory[ITI_DOOM_SHOTGUN] || ent->client->pers.inventory[ITI_DOOM_SUPER_SHOTGUN])
			ent->client->ps.stats.doom.weapons |= IT_DOOM_SHOTGUNS;

		if (ent->client->pers.inventory[ITI_DOOM_CHAINGUN])
			ent->client->ps.stats.doom.weapons |= IT_DOOM_CHAINGUN;

		if (ent->client->pers.inventory[ITI_DOOM_ROCKET_LAUNCHER])
			ent->client->ps.stats.doom.weapons |= IT_DOOM_ROCKET;

		if (ent->client->pers.inventory[ITI_DOOM_PLASMA_GUN])
			ent->client->ps.stats.doom.weapons |= IT_DOOM_PLASMA;

		if (ent->client->pers.inventory[ITI_DOOM_BFG])
			ent->client->ps.stats.doom.weapons |= IT_DOOM_BFG;

		ST_updateFaceWidget(ent);
	}
	// Duke stuff
	else if (ent->s.game == GAME_DUKE)
	{
		size_t i;
		ent->client->ps.stats.duke.weapons = ent->client->ps.stats.duke.selected_weapon = 0;
		struct
		{
			itemid_e	item;
			uint16_t	flag;
			uint8_t		index;
		} weapon_hud_mapping[] =
		{
			{ ITI_DUKE_PISTOL, IT_DUKE_PISTOL, IT_DUKE_INDEX_PISTOL },
			{ ITI_DUKE_SHOTGUN, IT_DUKE_SHOTGUN, IT_DUKE_INDEX_SHOTGUN },
			{ ITI_DUKE_CANNON, IT_DUKE_CANNON, IT_DUKE_INDEX_CANNON },
			{ ITI_DUKE_RPG, IT_DUKE_RPG, IT_DUKE_INDEX_RPG },
			{ ITI_DUKE_PIPEBOMBS, IT_DUKE_PIPE, IT_DUKE_INDEX_PIPE },
			//{ ITI_DUKE_SHRINKER, IT_DUKE_SHRINKER, IT_DUKE_INDEX_SHRINKER },
			{ ITI_DUKE_DEVASTATOR, IT_DUKE_DEVASTATOR, IT_DUKE_INDEX_DEVASTATOR },
			{ ITI_DUKE_TRIPWIRES, IT_DUKE_TRIPWIRE, IT_DUKE_INDEX_TRIPWIRE },
			{ ITI_DUKE_FREEZER, IT_DUKE_FREEZETHROWER, IT_DUKE_INDEX_FREEZETHROWER }
		};

		for (i = 0; i < q_countof(weapon_hud_mapping); i++)
		{
			if (ent->client->pers.inventory[weapon_hud_mapping[i].item])
				ent->client->ps.stats.duke.weapons |= weapon_hud_mapping[i].flag;

			if (GetIndexByItem(ent->client->pers.weapon) == weapon_hud_mapping[i].item)
				ent->client->ps.stats.duke.selected_weapon = weapon_hud_mapping[i].index;
		}

		ent->client->ps.stats.duke.ammo_clip = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_PISTOL));
		ent->client->ps.stats.duke.ammo_shells = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_SHOTGUN));
		ent->client->ps.stats.duke.ammo_cannon = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_CANNON));
		ent->client->ps.stats.duke.ammo_rpg = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_RPG));
		ent->client->ps.stats.duke.ammo_pipebombs = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_PIPEBOMBS));
		//ent->client->ps.stats.duke.ammo_shrinker = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_SHRINKER));
		ent->client->ps.stats.duke.ammo_devastator = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_DEVASTATOR));
		ent->client->ps.stats.duke.ammo_tripwire = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_TRIPWIRES));
		ent->client->ps.stats.duke.ammo_freezer = PLAYER_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_FREEZER));
		ent->client->ps.stats.duke.max_ammo_clip = PLAYER_TOTAL_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_PISTOL));
		ent->client->ps.stats.duke.max_ammo_shells = PLAYER_TOTAL_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_SHOTGUN));
		ent->client->ps.stats.duke.max_ammo_cannon = PLAYER_TOTAL_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_CANNON));
		ent->client->ps.stats.duke.max_ammo_rpg = PLAYER_TOTAL_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_RPG));
		ent->client->ps.stats.duke.max_ammo_pipebombs = PLAYER_TOTAL_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_PIPEBOMBS));
		//ent->client->ps.stats.duke.max_ammo_shrinker = PLAYER_TOTAL_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_SHRINKER));
		ent->client->ps.stats.duke.max_ammo_devastator = PLAYER_TOTAL_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_DEVASTATOR));
		ent->client->ps.stats.duke.max_ammo_tripwire = PLAYER_TOTAL_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_TRIPWIRES));
		ent->client->ps.stats.duke.max_ammo_freezer = PLAYER_TOTAL_SHOTS_FOR_WEAPON(ent, GetItemByIndex(ITI_DUKE_FREEZER));
	}
}

/*
===============
G_CheckChaseStats
===============
*/
void G_CheckChaseStats(edict_t *ent)
{
	int i;
	gclient_t *cl;

	for (i = 1; i <= maxclients->value; i++)
	{
		cl = g_edicts[i].client;

		if (!g_edicts[i].inuse || cl->chase_target != ent)
			continue;

		memcpy(&cl->ps.stats, &ent->client->ps.stats, sizeof(cl->ps.stats));
		G_SetSpectatorStats(g_edicts + i);
	}
}

/*
===============
G_SetSpectatorStats
===============
*/
void G_SetSpectatorStats(edict_t *ent)
{
	gclient_t *cl = ent->client;

	if (!cl->chase_target)
		G_SetStats(ent);

	cl->ps.stats.spectator = 1;
	// layouts are independant in spectator
	cl->ps.stats.layouts = 0;

	if (cl->pers.health <= 0 || level.intermissiontime || cl->showscores)
		cl->ps.stats.layouts |= 1;

	if (cl->showinventory && cl->pers.health > 0)
		cl->ps.stats.layouts |= 2;

	if (cl->chase_target && cl->chase_target->inuse)
		cl->ps.stats.chase = CS_PLAYERSKINS +
			(cl->chase_target - g_edicts) - 1;
	else
		cl->ps.stats.chase = 0;
}


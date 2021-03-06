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


//===============================================================
//
//				BOT SPAWN
//
//===============================================================



///////////////////////////////////////////////////////////////////////
// Respawn the bot
///////////////////////////////////////////////////////////////////////
void BOT_Respawn(edict_t *self)
{
	CopyToBodyQue(self);
	PutClientInServer(self);
	// add a teleportation effect
	self->server.state.event = EV_PLAYER_TELEPORT;
	// hold in place briefly
	self->server.client->server.ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self->server.client->server.ps.pmove.pm_time = 14;
	self->server.client->respawn_time = level.time;
	AI_ResetWeights(self);
	AI_ResetNavigation(self);
}


///////////////////////////////////////////////////////////////////////
// Find a free client spot - //jabot092(2)
///////////////////////////////////////////////////////////////////////
static edict_t *BOT_FindFreeClient(void)
{
	edict_t *bot;
	edict_t	*ent;
	int	i;
	int max_count = 0;
	bot = NULL;

	for (i = 0, ent = g_edicts + 1; i < game.maxclients; i++, ent++)
	{
		if (!ent->server.inuse && bot == NULL)
			bot = ent;

		//count bots for bot names
		if (ent->count > max_count)
			max_count = ent->count;
	}

	if (bot == NULL || (max_count + 2) >= game.maxclients)  //always leave room for 1 player
		return NULL;

	bot->count = max_count + 1; // Will become bot name...
	return bot;
}

///////////////////////////////////////////////////////////////////////
// Set the name of the bot and update the userinfo
///////////////////////////////////////////////////////////////////////
static void BOT_SetName(edict_t *bot, char *name, char *skin, char *team)
{
	float rnd;
	char userinfo[MAX_INFO_STRING];
	char bot_skin[MAX_INFO_STRING];
	char bot_name[MAX_INFO_STRING];

	// Set the name for the bot.
	// name
	if (strlen(name) == 0)
		sprintf(bot_name, "Bot%d", bot->count);
	else
		strcpy(bot_name, name);

	// skin
	if (strlen(skin) == 0)
	{
		// randomly choose skin
		rnd = random();

		if (rnd  < 0.05f)
			sprintf(bot_skin, "female/athena");
		else if (rnd < 0.1f)
			sprintf(bot_skin, "female/brianna");
		else if (rnd < 0.15f)
			sprintf(bot_skin, "female/cobalt");
		else if (rnd < 0.2f)
			sprintf(bot_skin, "female/ensign");
		else if (rnd < 0.25f)
			sprintf(bot_skin, "female/jezebel");
		else if (rnd < 0.3f)
			sprintf(bot_skin, "female/jungle");
		else if (rnd < 0.35f)
			sprintf(bot_skin, "female/lotus");
		else if (rnd < 0.4f)
			sprintf(bot_skin, "female/stiletto");
		else if (rnd < 0.45f)
			sprintf(bot_skin, "female/venus");
		else if (rnd < 0.5f)
			sprintf(bot_skin, "female/voodoo");
		else if (rnd < 0.55f)
			sprintf(bot_skin, "male/cipher");
		else if (rnd < 0.6f)
			sprintf(bot_skin, "male/flak");
		else if (rnd < 0.65f)
			sprintf(bot_skin, "male/grunt");
		else if (rnd < 0.7f)
			sprintf(bot_skin, "male/howitzer");
		else if (rnd < 0.75f)
			sprintf(bot_skin, "male/major");
		else if (rnd < 0.8f)
			sprintf(bot_skin, "male/nightops");
		else if (rnd < 0.85f)
			sprintf(bot_skin, "male/pointman");
		else if (rnd < 0.9f)
			sprintf(bot_skin, "male/psycho");
		else if (rnd < 0.95f)
			sprintf(bot_skin, "male/razor");
		else
			sprintf(bot_skin, "male/sniper");
	}
	else
		strcpy(bot_skin, skin);

	// initialise userinfo
	memset(userinfo, 0, sizeof(userinfo));
	// add bot's name/skin/hand to userinfo
	Info_SetValueForKey(userinfo, "name", bot_name);
	Info_SetValueForKey(userinfo, "skin", bot_skin);
	Info_SetValueForKey(userinfo, "hand", "2");  // bot is center handed for now!
	int r = Q_rand() % 3;

	switch (r)
	{
		case 0:
			Info_SetValueForKey(userinfo, "gameclass", "q2");
			Info_SetValueForKey(userinfo, "name", "Grunt");
			break;

		case 1:
			Info_SetValueForKey(userinfo, "gameclass", "q1");
			Info_SetValueForKey(userinfo, "name", "Ranger");
			break;

		case 2:
			Info_SetValueForKey(userinfo, "gameclass", "doom");
			Info_SetValueForKey(userinfo, "name", "Crash");
			break;
	}

	ClientConnect(bot, userinfo);
	//	ACESP_SaveBots(); // make sure to save the bots
}

#if CTF
//==========================================
// BOT_NextCTFTeam
// Get the emptier CTF team
//==========================================
int	BOT_NextCTFTeam()
{
	int	i;
	int	onteam1 = 0;
	int	onteam2 = 0;
	edict_t		*self;

	// Only use in CTF games
	if (!ctf->value)
		return 0;

	for (i = 0; i < game.maxclients + 1; i++)
	{
		self = g_edicts + i + 1;

		if (self->server.inuse && self->server.client)
		{
			if (self->server.client->resp.ctf_team == CTF_TEAM1)
				onteam1++;
			else if (self->server.client->resp.ctf_team == CTF_TEAM2)
				onteam2++;
		}
	}

	if (onteam1 > onteam2)
		return (2);
	else if (onteam2 >= onteam1)
		return (1);

	return (1);
}

//==========================================
// BOT_JoinCTFTeam
// Assign a team for the bot
//==========================================
bool BOT_JoinCTFTeam(edict_t *ent, char *team_name)
{
	char	*s;
	int		team;
	//	edict_t	*event;

	if (ent->server.client->resp.ctf_team != CTF_NOTEAM)
		return false;

	// find what ctf team
	if ((team_name != NULL) && (strcmp(team_name, "blue") == 0))
		team = CTF_TEAM2;
	else if ((team_name != NULL) && (strcmp(team_name, "red") == 0))
		team = CTF_TEAM1;
	else
		team = BOT_NextCTFTeam();

	if (team == CTF_NOTEAM)
		return false;

	//join ctf team
	ent->server.flags.noclient = false;
	ent->server.client->resp.ctf_state = 1;//0?
	ent->server.client->resp.ctf_team = team;
	s = Info_ValueForKey(ent->server.client->pers.userinfo, "skin");
	CTFAssignSkin(ent, s);
	PutClientInServer(ent);
	// hold in place briefly
	ent->server.client->server.ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	ent->server.client->server.ps.pmove.pm_time = 14;
	Com_Printf("%s joined the %s team.\n",
		ent->server.client->pers.netname, CTFTeamName(ent->server.client->resp.ctf_team));
	return true;
}
#endif

//==========================================
// BOT_DMClass_JoinGame
// put the bot into the game.
//==========================================
static void BOT_DMClass_JoinGame(edict_t *ent, char *team_name)
{
#if CTF

	if (!BOT_JoinCTFTeam(ent, team_name))
#endif
		Com_Printf("%s joined the game.\n",
			ent->server.client->pers.netname);

	ent->think = AI_Think;
	ent->nextthink = level.time + 1;
	//join game
	ent->movetype = MOVETYPE_WALK;
	ent->server.solid = SOLID_BBOX;
	ent->server.flags.noclient = false;
	memset(ent->server.client->server.ps.guns, 0, sizeof(ent->server.client->server.ps.guns));

	if (!KillBox(ent))
	{
		// could't spawn in?
	}

	gi.linkentity(ent);
}

//==========================================
// BOT_StartAsSpectator
//==========================================
static void BOT_StartAsSpectator(edict_t *ent)
{
	// start as 'observer'
	ent->movetype = MOVETYPE_NOCLIP;
	ent->server.solid = SOLID_NOT;
	ent->server.flags.noclient = true;
#if CTF
	ent->server.client->resp.ctf_team = CTF_NOTEAM;
#endif
	memset(ent->server.client->server.ps.guns, 0, sizeof(ent->server.client->server.ps.guns));
	gi.linkentity(ent);
}


//==========================================
// BOT_JoinGame
// 3 for teams and such
//==========================================
#if CTF
static void BOT_JoinBlue(edict_t *ent)
{
	BOT_DMClass_JoinGame(ent, "blue");
}
static void BOT_JoinRed(edict_t *ent)
{
	BOT_DMClass_JoinGame(ent, "red");
}
#endif
static void BOT_JoinGame(edict_t *ent)
{
	BOT_DMClass_JoinGame(ent, NULL);
}

///////////////////////////////////////////////////////////////////////
// Spawn the bot
///////////////////////////////////////////////////////////////////////
void BOT_SpawnBot(char *team, char *name, char *skin, char *userinfo)
{
	edict_t	*bot;

	if (!nav.loaded)
	{
		Com_Printf("Can't spawn bots without a valid navigation file\n");
		return;
	}

	bot = BOT_FindFreeClient();

	if (!bot)
	{
		safe_bprintf(PRINT_MEDIUM, "Server is full, increase Maxclients.\n");
		return;
	}

	//init the bot
	bot->server.inuse = true;
	bot->yaw_speed = 100;

	// To allow bots to respawn
	if (userinfo == NULL)
		BOT_SetName(bot, name, skin, team);
	else
		ClientConnect(bot, userinfo);

	G_InitEdict(bot);
	G_SpawnAI(bot); //jabot092(2)
	bot->ai->is_bot = true;
	InitClientResp(bot->server.client);
	PutClientInServer(bot);
	BOT_StartAsSpectator(bot);
	//skill
	bot->ai->pers.skillLevel = MAX_BOT_SKILL;/*(int)(random()*MAX_BOT_SKILL);
	if (bot->ai->pers.skillLevel > MAX_BOT_SKILL)	//fix if off-limits
		bot->ai->pers.skillLevel =  MAX_BOT_SKILL;
	else if (bot->ai->pers.skillLevel < 0)
		bot->ai->pers.skillLevel =  0;*/
	BOT_DMclass_InitPersistant(bot);
	AI_ResetWeights(bot);
	AI_ResetNavigation(bot);
	bot->think = BOT_JoinGame;
	bot->nextthink = level.time + (Q_rand() % 6000);
#if CTF

	if (ctf->value && team != NULL)
	{
		if (!Q_stricmp(team, "blue"))
			bot->think = BOT_JoinBlue;
		else if (!Q_stricmp(team, "red"))
			bot->think = BOT_JoinRed;
	}

#endif
	AI_EnemyAdded(bot);  // let the ai know we added another
}


///////////////////////////////////////////////////////////////////////
// Remove a bot by name or all bots
///////////////////////////////////////////////////////////////////////
void BOT_RemoveBot(char *name)
{
	int i;
	bool freed = false;
	edict_t *bot;

	for (i = 0; i < maxclients->value; i++)
	{
		bot = g_edicts + i + 1;

		if (!bot->server.inuse || !bot->ai)   //jabot092(2)
			continue;

		if (bot->ai->is_bot && (!strcmp(bot->server.client->pers.netname, name) || !strcmp(name, "all")))
		{
			bot->health = 0;
			player_die(bot, bot, bot, 100000, vec3_origin);
			// don't even bother waiting for death frames
			bot->deadflag = DEAD_DEAD;
			bot->server.inuse = false;
			freed = true;
			AI_EnemyRemoved(bot);
			G_FreeAI(bot);   //jabot092(2)
			//safe_bprintf (PRINT_MEDIUM, "%s removed\n", bot->server.client->pers.netname);
		}
	}

	//	if(!freed && !Q_stricmp( name, "all") )
	//		safe_bprintf (PRINT_MEDIUM, "%s not found\n", name);
}
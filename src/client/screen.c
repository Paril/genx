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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

#include "client.h"

#define STAT_PICS       11
#define STAT_MINUS      (STAT_PICS - 1)  // num frame for '-' stats digit

static struct
{
	bool        initialized;        // ready to draw

	qhandle_t	crosshair_pic;
	int         crosshair_width, crosshair_height;
	color_t     crosshair_color;

	qhandle_t	pause_pic;
	int         pause_width, pause_height;

	qhandle_t	loading_pic;
	int         loading_width, loading_height;
	bool        draw_loading;

	qhandle_t	sb_pics[2][STAT_PICS];
	qhandle_t	inven_pic;
	qhandle_t	field_pic;

	qhandle_t	backtile_pic;

	qhandle_t	net_pic;
	qhandle_t	font_pic;

	int         hud_width, hud_height;
	float       hud_scale;

	// Generations
	qhandle_t	q1_backtile_pic;
	qhandle_t	d_backtile_pic;

	qhandle_t	sb_faces[5][2];
	qhandle_t	sb_face_invis;
	qhandle_t	sb_face_invuln;
	qhandle_t	sb_face_invis_invuln;
	qhandle_t	sb_face_quad;

	qhandle_t	sb_doom_nums[10];
	qhandle_t	sb_doom_gray_nums[10];
	qhandle_t	sb_doom_faces[ST_NUMFACES];

	qhandle_t	sb_duke_nums[12];
} scr;

static cvar_t   *scr_viewsize;
static cvar_t   *scr_centertime;
static cvar_t   *scr_showpause;
#ifdef _DEBUG
	static cvar_t   *scr_showstats;
	static cvar_t   *scr_showpmove;
#endif
static cvar_t   *scr_showturtle;

static cvar_t   *scr_draw2d;
static cvar_t   *scr_lag_x;
static cvar_t   *scr_lag_y;
static cvar_t   *scr_lag_draw;
static cvar_t   *scr_lag_min;
static cvar_t   *scr_lag_max;
static cvar_t   *scr_alpha;

static cvar_t   *scr_font;
static cvar_t   *scr_scale;

static cvar_t   *scr_crosshair;

static cvar_t   *scr_chathud;
static cvar_t   *scr_chathud_lines;
static cvar_t   *scr_chathud_time;
static cvar_t   *scr_chathud_x;
static cvar_t   *scr_chathud_y;

static cvar_t   *ch_health;
static cvar_t   *ch_red;
static cvar_t   *ch_green;
static cvar_t   *ch_blue;
static cvar_t   *ch_alpha;

static cvar_t   *ch_scale;
static cvar_t   *ch_x;
static cvar_t   *ch_y;

// Paril
cvar_t	*scr_gunfov;

vrect_t     scr_vrect;      // position of render window on screen

static const char *const sb_nums[2][STAT_PICS] =
{
	{
		"%num_0", "%num_1", "%num_2", "%num_3", "%num_4", "%num_5",
		"%num_6", "%num_7", "%num_8", "%num_9", "%num_minus"
	},
	{
		"%anum_0", "%anum_1", "%anum_2", "%anum_3", "%anum_4", "%anum_5",
		"%anum_6", "%anum_7", "%anum_8", "%anum_9", "%anum_minus"
	}
};

uint32_t colorTable[8];

/*
===============================================================================

UTILS

===============================================================================
*/

#define SCR_DrawString(x, y, flags, string) \
	SCR_DrawStringEx(x, y, flags, MAX_STRING_CHARS, string, scr.font_pic, CL_GetClientGame())

/*
==============
SCR_DrawStringEx
==============
*/
int SCR_DrawStringEx(int x, int y, int flags, size_t maxlen,
	const char *s, qhandle_t font, gametype_t game)
{
	size_t len = strlen(s);

	if (len > maxlen)
		len = maxlen;

	if ((flags & UI_CENTER) == UI_CENTER)
		x -= len * CHAR_WIDTH / 2;
	else if (flags & UI_RIGHT)
		x -= len * CHAR_WIDTH;

	return R_DrawString(x, y, flags, maxlen, s, font, game);
}


/*
==============
SCR_DrawStringMulti
==============
*/
void SCR_DrawStringMulti(int x, int y, int flags, size_t maxlen,
	const char *s, qhandle_t font, gametype_t game)
{
	char    *p;
	size_t  len;

	while (*s)
	{
		p = strchr(s, '\n');

		if (!p)
		{
			SCR_DrawStringEx(x, y, flags, maxlen, s, font, game);
			break;
		}

		len = p - s;

		if (len > maxlen)
			len = maxlen;

		SCR_DrawStringEx(x, y, flags, len, s, font, game);
		y += CHAR_HEIGHT;
		s = p + 1;
	}
}


/*
=================
SCR_FadeAlpha
=================
*/
float SCR_FadeAlpha(unsigned startTime, unsigned visTime, unsigned fadeTime)
{
	float alpha;
	unsigned timeLeft, delta = cls.realtime - startTime;

	if (delta >= visTime)
		return 0;

	if (fadeTime > visTime)
		fadeTime = visTime;

	alpha = 1;
	timeLeft = visTime - delta;

	if (timeLeft < fadeTime)
		alpha = (float)timeLeft / fadeTime;

	return alpha;
}

bool SCR_ParseColor(const char *s, color_t *color)
{
	int i;
	int c[8];

	// parse generic color
	if (*s == '#')
	{
		s++;

		for (i = 0; s[i]; i++)
		{
			if (i == 8)
				return false;

			c[i] = Q_charhex(s[i]);

			if (c[i] == -1)
				return false;
		}

		switch (i)
		{
			case 3:
				color->u8[0] = c[0] | (c[0] << 4);
				color->u8[1] = c[1] | (c[1] << 4);
				color->u8[2] = c[2] | (c[2] << 4);
				color->u8[3] = 255;
				break;

			case 6:
				color->u8[0] = c[1] | (c[0] << 4);
				color->u8[1] = c[3] | (c[2] << 4);
				color->u8[2] = c[5] | (c[4] << 4);
				color->u8[3] = 255;
				break;

			case 8:
				color->u8[0] = c[1] | (c[0] << 4);
				color->u8[1] = c[3] | (c[2] << 4);
				color->u8[2] = c[5] | (c[4] << 4);
				color->u8[3] = c[7] | (c[6] << 4);
				break;

			default:
				return false;
		}

		return true;
	}

	// parse name or index
	i = Com_ParseColor(s, COLOR_WHITE);

	if (i == COLOR_NONE)
		return false;

	color->u32 = colorTable[i];
	return true;
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

static char     scr_centerstring[MAX_STRING_CHARS];
static unsigned scr_centertime_start;   // for slow victory printing
static int      scr_center_lines;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint(const char *str)
{
	const char  *s;
	scr_centertime_start = cls.realtime;

	if (!strcmp(scr_centerstring, str))
		return;

	Q_strlcpy(scr_centerstring, str, sizeof(scr_centerstring));
	// count the number of lines for centering
	scr_center_lines = 1;
	s = str;

	while (*s)
	{
		if (*s == '\n')
			scr_center_lines++;

		s++;
	}

	// echo it to the console
	Com_Printf("%s\n", scr_centerstring);
	Con_ClearNotify_f();
}

static void SCR_DrawCenterString(void)
{
	int y;
	float alpha;
	Cvar_ClampValue(scr_centertime, 0.3f, 10.0f);
	alpha = SCR_FadeAlpha(scr_centertime_start, scr_centertime->value * 1000, 300);

	if (!alpha)
		return;

	R_SetAlpha(alpha * scr_alpha->value);
	y = scr.hud_height / 4 - scr_center_lines * 8 / 2;
	SCR_DrawStringMulti(scr.hud_width / 2, y, UI_CENTER,
		MAX_STRING_CHARS, scr_centerstring, scr.font_pic, CL_GetClientGame());
	R_SetAlpha(scr_alpha->value);
}

/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define LAG_WIDTH   48
#define LAG_HEIGHT  48

#define LAG_CRIT_BIT    (1U << 31)
#define LAG_WARN_BIT    (1U << 30)

#define LAG_BASE    0xD5
#define LAG_WARN    0xDC
#define LAG_CRIT    0xF2

static struct
{
	unsigned samples[LAG_WIDTH];
	unsigned head;
} lag;

void SCR_LagClear(void)
{
	lag.head = 0;
}

void SCR_LagSample(void)
{
	int i = cls.netchan->incoming_acknowledged & CMD_MASK;
	client_history_t *h = &cl.history[i];
	unsigned ping;
	h->rcvd = cls.realtime;

	if (!h->cmdNumber || h->rcvd < h->sent)
		return;

	ping = h->rcvd - h->sent;

	for (i = 0; i < cls.netchan->dropped; i++)
	{
		lag.samples[lag.head % LAG_WIDTH] = ping | LAG_CRIT_BIT;
		lag.head++;
	}

	if (cl.frameflags & FF_SUPPRESSED)
		ping |= LAG_WARN_BIT;

	lag.samples[lag.head % LAG_WIDTH] = ping;
	lag.head++;
}

static void SCR_LagDraw(int x, int y)
{
	int i, j, v, c, v_min, v_max, v_range;
	v_min = Cvar_ClampInteger(scr_lag_min, 0, LAG_HEIGHT * 10);
	v_max = Cvar_ClampInteger(scr_lag_max, 0, LAG_HEIGHT * 10);
	v_range = v_max - v_min;

	if (v_range < 1)
		return;

	for (i = 0; i < LAG_WIDTH; i++)
	{
		j = lag.head - i - 1;

		if (j < 0)
			break;

		v = lag.samples[j % LAG_WIDTH];

		if (v & LAG_CRIT_BIT)
			c = LAG_CRIT;
		else if (v & LAG_WARN_BIT)
			c = LAG_WARN;
		else
			c = LAG_BASE;

		v &= ~(LAG_WARN_BIT | LAG_CRIT_BIT);
		v = (v - v_min) * LAG_HEIGHT / v_range;
		clamp(v, 0, LAG_HEIGHT);
		R_DrawFill8(x + LAG_WIDTH - i - 1, y + LAG_HEIGHT - v, 1, v, c);
	}
}

static void SCR_DrawNet(void)
{
	int x = scr_lag_x->integer;
	int y = scr_lag_y->integer;

	if (x < 0)
		x += scr.hud_width - LAG_WIDTH + 1;

	if (y < 0)
		y += scr.hud_height - LAG_HEIGHT + 1;

	// draw ping graph
	if (scr_lag_draw->integer)
	{
		if (scr_lag_draw->integer > 1)
			R_DrawFill8(x, y, LAG_WIDTH, LAG_HEIGHT, 4);

		SCR_LagDraw(x, y);
	}

	// draw phone jack
	if (cls.netchan && cls.netchan->outgoing_sequence - cls.netchan->incoming_acknowledged >= CMD_BACKUP)
	{
		if ((cls.realtime >> 8) & 3)
			R_DrawStretchPic(x, y, LAG_WIDTH, LAG_HEIGHT, scr.net_pic, CL_GetClientGame());
	}
}


/*
===============================================================================

DRAW OBJECTS

===============================================================================
*/

typedef struct
{
	list_t          entry;
	int             x, y;
	cvar_t          *cvar;
	cmd_macro_t     *macro;
	int             flags;
	color_t         color;
} drawobj_t;

#define FOR_EACH_DRAWOBJ(obj) \
	LIST_FOR_EACH(drawobj_t, obj, &scr_objects, entry)
#define FOR_EACH_DRAWOBJ_SAFE(obj, next) \
	LIST_FOR_EACH_SAFE(drawobj_t, obj, next, &scr_objects, entry)

static LIST_DECL(scr_objects);

static void SCR_Color_g(genctx_t *ctx)
{
	int color;

	for (color = 0; color < 10; color++)
		Prompt_AddMatch(ctx, colorNames[color]);
}

static void SCR_Draw_c(genctx_t *ctx, int argnum)
{
	if (argnum == 1)
	{
		Cvar_Variable_g(ctx);
		Cmd_Macro_g(ctx);
	}
	else if (argnum == 4)
		SCR_Color_g(ctx);
}

// draw cl_fps -1 80
static void SCR_Draw_f(void)
{
	int x, y;
	const char *s, *c;
	drawobj_t *obj;
	cmd_macro_t *macro;
	cvar_t *cvar;
	color_t color;
	int flags;
	int argc = Cmd_Argc();

	if (argc == 1)
	{
		if (LIST_EMPTY(&scr_objects))
		{
			Com_Printf("No draw strings registered.\n");
			return;
		}

		Com_Printf("Name               X    Y\n"
			"--------------- ---- ----\n");
		FOR_EACH_DRAWOBJ(obj)
		{
			s = obj->macro ? obj->macro->name : obj->cvar->name;
			Com_Printf("%-15s %4d %4d\n", s, obj->x, obj->y);
		}
		return;
	}

	if (argc < 4)
	{
		Com_Printf("Usage: %s <name> <x> <y> [color]\n", Cmd_Argv(0));
		return;
	}

	color.u32 = U32_BLACK;
	flags = UI_IGNORECOLOR;
	s = Cmd_Argv(1);
	x = atoi(Cmd_Argv(2));
	y = atoi(Cmd_Argv(3));

	if (x < 0)
		flags |= UI_RIGHT;

	if (argc > 4)
	{
		c = Cmd_Argv(4);

		if (!strcmp(c, "alt"))
			flags |= UI_ALTCOLOR;
		else if (strcmp(c, "none"))
		{
			if (!SCR_ParseColor(c, &color))
			{
				Com_Printf("Unknown color '%s'\n", c);
				return;
			}

			flags &= ~UI_IGNORECOLOR;
		}
	}

	cvar = NULL;
	macro = Cmd_FindMacro(s);

	if (!macro)
		cvar = Cvar_WeakGet(s);

	FOR_EACH_DRAWOBJ(obj)
	{
		if (obj->macro == macro && obj->cvar == cvar)
		{
			obj->x = x;
			obj->y = y;
			obj->flags = flags;
			obj->color.u32 = color.u32;
			return;
		}
	}
	obj = Z_Malloc(sizeof(*obj));
	obj->x = x;
	obj->y = y;
	obj->cvar = cvar;
	obj->macro = macro;
	obj->flags = flags;
	obj->color.u32 = color.u32;
	List_Append(&scr_objects, &obj->entry);
}

static void SCR_Draw_g(genctx_t *ctx)
{
	drawobj_t *obj;
	const char *s;

	if (LIST_EMPTY(&scr_objects))
		return;

	Prompt_AddMatch(ctx, "all");
	FOR_EACH_DRAWOBJ(obj)
	{
		s = obj->macro ? obj->macro->name : obj->cvar->name;
		Prompt_AddMatch(ctx, s);
	}
}

static void SCR_UnDraw_c(genctx_t *ctx, int argnum)
{
	if (argnum == 1)
		SCR_Draw_g(ctx);
}

static void SCR_UnDraw_f(void)
{
	char *s;
	drawobj_t *obj, *next;
	cmd_macro_t *macro;
	cvar_t *cvar;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("Usage: %s <name>\n", Cmd_Argv(0));
		return;
	}

	if (LIST_EMPTY(&scr_objects))
	{
		Com_Printf("No draw strings registered.\n");
		return;
	}

	s = Cmd_Argv(1);

	if (!strcmp(s, "all"))
	{
		FOR_EACH_DRAWOBJ_SAFE(obj, next)
		{
			Z_Free(obj);
		}
		List_Init(&scr_objects);
		Com_Printf("Deleted all draw strings.\n");
		return;
	}

	cvar = NULL;
	macro = Cmd_FindMacro(s);

	if (!macro)
		cvar = Cvar_WeakGet(s);

	FOR_EACH_DRAWOBJ_SAFE(obj, next)
	{
		if (obj->macro == macro && obj->cvar == cvar)
		{
			List_Remove(&obj->entry);
			Z_Free(obj);
			return;
		}
	}
	Com_Printf("Draw string '%s' not found.\n", s);
}

static void SCR_DrawObjects(void)
{
	char buffer[MAX_QPATH];
	int x, y;
	drawobj_t *obj;
	FOR_EACH_DRAWOBJ(obj)
	{
		x = obj->x;
		y = obj->y;

		if (x < 0)
			x += scr.hud_width + 1;

		if (y < 0)
			y += scr.hud_height - CHAR_HEIGHT + 1;

		if (!(obj->flags & UI_IGNORECOLOR))
			R_SetColor(obj->color.u32);

		if (obj->macro)
		{
			obj->macro->function(buffer, sizeof(buffer));
			SCR_DrawString(x, y, obj->flags, buffer);
		}
		else
			SCR_DrawString(x, y, obj->flags, obj->cvar->string);

		if (!(obj->flags & UI_IGNORECOLOR))
		{
			R_ClearColor();
			R_SetAlpha(scr_alpha->value);
		}
	}
}

/*
===============================================================================

CHAT HUD

===============================================================================
*/

#define MAX_CHAT_TEXT       150
#define MAX_CHAT_LINES      32
#define CHAT_LINE_MASK      (MAX_CHAT_LINES - 1)

typedef struct
{
	char        text[MAX_CHAT_TEXT];
	unsigned    time;
} chatline_t;

static chatline_t   scr_chatlines[MAX_CHAT_LINES];
static unsigned     scr_chathead;

void SCR_ClearChatHUD_f(void)
{
	memset(scr_chatlines, 0, sizeof(scr_chatlines));
	scr_chathead = 0;
}

void SCR_AddToChatHUD(const char *text)
{
	chatline_t *line;
	char *p;
	line = &scr_chatlines[scr_chathead++ & CHAT_LINE_MASK];
	Q_strlcpy(line->text, text, sizeof(line->text));
	line->time = cls.realtime;
	p = strrchr(line->text, '\n');

	if (p)
		*p = 0;
}

static void SCR_DrawChatHUD(void)
{
	int x, y, i, lines, flags, step;
	float alpha;
	chatline_t *line;

	if (scr_chathud->integer == 0)
		return;

	x = scr_chathud_x->integer;
	y = scr_chathud_y->integer;

	if (scr_chathud->integer == 2)
		flags = UI_ALTCOLOR;
	else
		flags = 0;

	if (x < 0)
	{
		x += scr.hud_width + 1;
		flags |= UI_RIGHT;
	}
	else
		flags |= UI_LEFT;

	if (y < 0)
	{
		y += scr.hud_height - CHAR_HEIGHT + 1;
		step = -CHAR_HEIGHT;
	}
	else
		step = CHAR_HEIGHT;

	lines = scr_chathud_lines->integer;

	if (lines > scr_chathead)
		lines = scr_chathead;

	for (i = 0; i < lines; i++)
	{
		line = &scr_chatlines[(scr_chathead - i - 1) & CHAT_LINE_MASK];

		if (scr_chathud_time->integer)
		{
			alpha = SCR_FadeAlpha(line->time, scr_chathud_time->integer, 1000);

			if (!alpha)
				break;

			R_SetAlpha(alpha * scr_alpha->value);
			SCR_DrawString(x, y, flags, line->text);
			R_SetAlpha(scr_alpha->value);
		}
		else
			SCR_DrawString(x, y, flags, line->text);

		y += step;
	}
}

/*
===============================================================================

DEBUG STUFF

===============================================================================
*/

static void SCR_DrawTurtle(void)
{
	int x, y;

	if (scr_showturtle->integer <= 0)
		return;

	if (!cl.frameflags)
		return;

	x = CHAR_WIDTH;
	y = scr.hud_height - 11 * CHAR_HEIGHT;
#define DF(f) \
	if (cl.frameflags & FF_##f) { \
		SCR_DrawString(x, y, UI_ALTCOLOR, #f); \
		y += CHAR_HEIGHT; \
	}

	if (scr_showturtle->integer > 1)
	{
		DF(SUPPRESSED)
	}

	DF(CLIENTPRED)

	if (scr_showturtle->integer > 1)
	{
		DF(CLIENTDROP)
		DF(SERVERDROP)
	}

	DF(BADFRAME)
	DF(OLDFRAME)
	DF(OLDENT)
	DF(NODELTA)
#undef DF
}

#ifdef _DEBUG

static void SCR_DrawDebugStats(void)
{
	/*char buffer[MAX_QPATH];
	int i, j;
	int x, y;

	j = scr_showstats->integer;
	if (j <= 0)
		return;

	if (j > MAX_STATS)
		j = MAX_STATS;

	x = CHAR_WIDTH;
	y = (scr.hud_height - j * CHAR_HEIGHT) / 2;
	for (i = 0; i < j; i++) {
		Q_snprintf(buffer, sizeof(buffer), "%2d: %d", i, cl.frame.ps.stats[i]);
		if (cl.oldframe.ps.stats[i] != cl.frame.ps.stats[i]) {
			R_SetColor(U32_RED);
		}
		R_DrawString(x, y, 0, MAX_STRING_CHARS, buffer, scr.font_pic, CL_GetClientGame());
		R_ClearColor();
		y += CHAR_HEIGHT;
	}*/
}

static void SCR_DrawDebugPmove(void)
{
	static const char *const types[] =
	{
		"NORMAL", "SPECTATOR", "DEAD", "GIB", "FREEZE"
		// Generations
		, "PM_DUKE_FROZEN"
	};
	static const char *const flags[] =
	{
		"DUCKED", "JUMP_HELD", "ON_GROUND",
		"TIME_WATERJUMP", "TIME_LAND", "TIME_TELEPORT",
		"NO_PREDICTION", "TELEPORT_BIT"
	};
	unsigned i, j;
	int x, y;

	if (!scr_showpmove->integer)
		return;

	x = CHAR_WIDTH;
	y = (scr.hud_height - 2 * CHAR_HEIGHT) / 2;
	i = cl.frame.ps.pmove.pm_type;

	// Generations
	if (i > PM_DUKE_FROZEN)
		i = PM_DUKE_FROZEN;

	R_DrawString(x, y, 0, MAX_STRING_CHARS, types[i], scr.font_pic, CL_GetClientGame());
	y += CHAR_HEIGHT;
	j = cl.frame.ps.pmove.pm_flags;

	for (i = 0; i < 8; i++)
	{
		if (j & (1 << i))
		{
			x = R_DrawString(x, y, 0, MAX_STRING_CHARS, flags[i], scr.font_pic, CL_GetClientGame());
			x += CHAR_WIDTH;
		}
	}
}

#endif

//============================================================================

// Sets scr_vrect, the coordinates of the rendered window
static void SCR_CalcVrect(void)
{
	int     size;

	// Generations
	switch (CL_GetClientGame())
	{
		case GAME_Q1:
			scr_vrect.x = scr_vrect.y = 0;
			scr_vrect.width = scr.hud_width & ~7;
			scr_vrect.height = (int)(scr.hud_height - (48 / scr.hud_scale)) & ~1;
			break;

		case GAME_DOOM:
		case GAME_DUKE:
			scr_vrect.x = scr_vrect.y = 0;
			scr_vrect.width = scr.hud_width & ~7;
			scr_vrect.height = (int)(scr.hud_height - (32 / scr.hud_scale)) & ~1;
			break;

		default:
			// bound viewsize
			size = Cvar_ClampInteger(scr_viewsize, 40, 100);
			scr_viewsize->modified = false;
			scr_vrect.width = scr.hud_width * size / 100;
			scr_vrect.width &= ~7;
			scr_vrect.height = scr.hud_height * size / 100;
			scr_vrect.height &= ~1;
			scr_vrect.x = (scr.hud_width - scr_vrect.width) / 2;
			scr_vrect.y = (scr.hud_height - scr_vrect.height) / 2;
			break;
	}
}

/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
static void SCR_SizeUp_f(void)
{
	Cvar_SetInteger(scr_viewsize, scr_viewsize->integer + 10, FROM_CONSOLE);
}

/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
static void SCR_SizeDown_f(void)
{
	Cvar_SetInteger(scr_viewsize, scr_viewsize->integer - 10, FROM_CONSOLE);
}

/*
=================
SCR_Sky_f

Set a specific sky and rotation speed. If empty sky name is provided, falls
back to server defaults.
=================
*/
static void SCR_Sky_f(void)
{
	char    *name;
	float   rotate;
	vec3_t  axis;
	int     argc = Cmd_Argc();

	if (argc < 2)
	{
		Com_Printf("Usage: sky <basename> [rotate] [axis x y z]\n");
		return;
	}

	if (cls.state != ca_active)
	{
		Com_Printf("No map loaded.\n");
		return;
	}

	name = Cmd_Argv(1);

	if (!*name)
	{
		CL_SetSky();
		return;
	}

	if (argc > 2)
		rotate = atof(Cmd_Argv(2));
	else
		rotate = 0;

	if (argc == 6)
	{
		axis[0] = atof(Cmd_Argv(3));
		axis[1] = atof(Cmd_Argv(4));
		axis[2] = atof(Cmd_Argv(5));
	}
	else
		VectorSet(axis, 0, 0, 1);

	R_SetSky(name, rotate, axis);
}

/*
================
SCR_TimeRefresh_f
================
*/
static void SCR_TimeRefresh_f(void)
{
	int     i;
	unsigned    start, stop;
	float       time;

	if (cls.state != ca_active)
	{
		Com_Printf("No map loaded.\n");
		return;
	}

	start = Sys_Milliseconds();

	if (Cmd_Argc() == 2)
	{
		// run without page flipping
		R_BeginFrame();

		for (i = 0; i < 128; i++)
		{
			cl.refdef.viewangles[1] = i / 128.0f * 360.0f;
			R_RenderFrame(&cl.refdef);
		}

		R_EndFrame();
	}
	else
	{
		for (i = 0; i < 128; i++)
		{
			cl.refdef.viewangles[1] = i / 128.0f * 360.0f;
			R_BeginFrame();
			R_RenderFrame(&cl.refdef);
			R_EndFrame();
		}
	}

	stop = Sys_Milliseconds();
	time = (stop - start) * 0.001f;
	Com_Printf("%f seconds (%f fps)\n", time, 128.0f / time);
}


//============================================================================

static void scr_crosshair_changed(cvar_t *self)
{
	char buffer[16];
	int w, h;
	float scale;

	if (scr_crosshair->integer > 0)
	{
		Q_snprintf(buffer, sizeof(buffer), "ch%i", scr_crosshair->integer);
		scr.crosshair_pic = R_RegisterPic(buffer);
		R_GetPicSize(&w, &h, scr.crosshair_pic, CL_GetClientGame());
		// prescale
		scale = Cvar_ClampValue(ch_scale, 0.1f, 9.0f);
		scr.crosshair_width = w * scale;
		scr.crosshair_height = h * scale;

		if (scr.crosshair_width < 1)
			scr.crosshair_width = 1;

		if (scr.crosshair_height < 1)
			scr.crosshair_height = 1;

		if (ch_health->integer)
			SCR_SetCrosshairColor();
		else
		{
			scr.crosshair_color.u8[0] = Cvar_ClampValue(ch_red, 0, 1) * 255;
			scr.crosshair_color.u8[1] = Cvar_ClampValue(ch_green, 0, 1) * 255;
			scr.crosshair_color.u8[2] = Cvar_ClampValue(ch_blue, 0, 1) * 255;
		}

		scr.crosshair_color.u8[3] = Cvar_ClampValue(ch_alpha, 0, 1) * 255;
	}
	else
		scr.crosshair_pic = 0;
}

void SCR_SetCrosshairColor(void)
{
	int health;

	if (!ch_health->integer)
		return;

	health = cl.frame.ps.stats.health;

	if (health <= 0)
	{
		VectorSet(scr.crosshair_color.u8, 0, 0, 0);
		return;
	}

	// red
	scr.crosshair_color.u8[0] = 255;

	// green
	if (health >= 66)
		scr.crosshair_color.u8[1] = 255;
	else if (health < 33)
		scr.crosshair_color.u8[1] = 0;
	else
		scr.crosshair_color.u8[1] = (255 * (health - 33)) / 33;

	// blue
	if (health >= 99)
		scr.crosshair_color.u8[2] = 255;
	else if (health < 66)
		scr.crosshair_color.u8[2] = 0;
	else
		scr.crosshair_color.u8[2] = (255 * (health - 66)) / 33;
}

void SCR_ModeChanged(void)
{
	IN_Activate();
	Con_CheckResize();
	UI_ModeChanged();
	// video sync flag may have changed
	CL_UpdateFrameTimes();
	cls.disable_screen = 0;

	if (scr.initialized)
		scr.hud_scale = R_ClampScale(scr_scale);
}

/*
==================
SCR_RegisterMedia
==================
*/
void SCR_RegisterMedia(void)
{
	int     i, j;

	for (i = 0; i < 2; i++)
		for (j = 0; j < STAT_PICS; j++)
			scr.sb_pics[i][j] = R_RegisterPic(sb_nums[i][j]);

	// Generations
	scr.sb_faces[4][0] = R_RegisterPic("q1/face1.q1p");
	scr.sb_faces[4][1] = R_RegisterPic("q1/face_p1.q1p");
	scr.sb_faces[3][0] = R_RegisterPic("q1/face2.q1p");
	scr.sb_faces[3][1] = R_RegisterPic("q1/face_p2.q1p");
	scr.sb_faces[2][0] = R_RegisterPic("q1/face3.q1p");
	scr.sb_faces[2][1] = R_RegisterPic("q1/face_p3.q1p");
	scr.sb_faces[1][0] = R_RegisterPic("q1/face4.q1p");
	scr.sb_faces[1][1] = R_RegisterPic("q1/face_p4.q1p");
	scr.sb_faces[0][0] = R_RegisterPic("q1/face5.q1p");
	scr.sb_faces[0][1] = R_RegisterPic("q1/face_p5.q1p");
	scr.sb_face_invis = R_RegisterPic("q1/face_invis.q1p");
	scr.sb_face_invuln = R_RegisterPic("q1/face_invul2.q1p");
	scr.sb_face_invis_invuln = R_RegisterPic("q1/face_inv2.q1p");
	scr.sb_face_quad = R_RegisterPic("q1/face_quad.q1p");
	scr.sb_doom_nums[0] = R_RegisterPic("doom/STYSNUM0.d2p");
	scr.sb_doom_nums[1] = R_RegisterPic("doom/STYSNUM1.d2p");
	scr.sb_doom_nums[2] = R_RegisterPic("doom/STYSNUM2.d2p");
	scr.sb_doom_nums[3] = R_RegisterPic("doom/STYSNUM3.d2p");
	scr.sb_doom_nums[4] = R_RegisterPic("doom/STYSNUM4.d2p");
	scr.sb_doom_nums[5] = R_RegisterPic("doom/STYSNUM5.d2p");
	scr.sb_doom_nums[6] = R_RegisterPic("doom/STYSNUM6.d2p");
	scr.sb_doom_nums[7] = R_RegisterPic("doom/STYSNUM7.d2p");
	scr.sb_doom_nums[8] = R_RegisterPic("doom/STYSNUM8.d2p");
	scr.sb_doom_nums[9] = R_RegisterPic("doom/STYSNUM9.d2p");
	scr.sb_doom_gray_nums[0] = R_RegisterPic("doom/STGNUM0.d2p");
	scr.sb_doom_gray_nums[1] = R_RegisterPic("doom/STGNUM1.d2p");
	scr.sb_doom_gray_nums[2] = R_RegisterPic("doom/STGNUM2.d2p");
	scr.sb_doom_gray_nums[3] = R_RegisterPic("doom/STGNUM3.d2p");
	scr.sb_doom_gray_nums[4] = R_RegisterPic("doom/STGNUM4.d2p");
	scr.sb_doom_gray_nums[5] = R_RegisterPic("doom/STGNUM5.d2p");
	scr.sb_doom_gray_nums[6] = R_RegisterPic("doom/STGNUM6.d2p");
	scr.sb_doom_gray_nums[7] = R_RegisterPic("doom/STGNUM7.d2p");
	scr.sb_doom_gray_nums[8] = R_RegisterPic("doom/STGNUM8.d2p");
	scr.sb_doom_gray_nums[9] = R_RegisterPic("doom/STGNUM9.d2p");
	scr.sb_duke_nums[0] = R_RegisterPic("duke/35_0.dnp");
	scr.sb_duke_nums[1] = R_RegisterPic("duke/35_1.dnp");
	scr.sb_duke_nums[2] = R_RegisterPic("duke/35_2.dnp");
	scr.sb_duke_nums[3] = R_RegisterPic("duke/35_3.dnp");
	scr.sb_duke_nums[4] = R_RegisterPic("duke/35_4.dnp");
	scr.sb_duke_nums[5] = R_RegisterPic("duke/35_5.dnp");
	scr.sb_duke_nums[6] = R_RegisterPic("duke/35_6.dnp");
	scr.sb_duke_nums[7] = R_RegisterPic("duke/35_7.dnp");
	scr.sb_duke_nums[8] = R_RegisterPic("duke/35_8.dnp");
	scr.sb_duke_nums[9] = R_RegisterPic("duke/35_9.dnp");
	scr.sb_duke_nums[10] = R_RegisterPic("duke/35_colon.dnp");
	scr.sb_duke_nums[11] = R_RegisterPic("duke/35_slash.dnp");
	scr.inven_pic = R_RegisterPic("inventory");
	scr.field_pic = R_RegisterPic("field_3");
	scr.backtile_pic = R_RegisterImage("backtile", IT_PIC, IF_PERMANENT | IF_REPEAT, NULL);
	// Generations
	scr.q1_backtile_pic = R_RegisterImage("q1/backtile.q1p", IT_PIC, IF_PERMANENT | IF_REPEAT, NULL);
	scr.d_backtile_pic = R_RegisterImage("doom/backtile.d2p", IT_PIC, IF_PERMANENT | IF_REPEAT, NULL);
	scr.pause_pic = R_RegisterPic("pause");
	R_GetPicSize(&scr.pause_width, &scr.pause_height, scr.pause_pic, CL_GetClientGame());
	scr.loading_pic = R_RegisterPic("loading");
	R_GetPicSize(&scr.loading_width, &scr.loading_height, scr.loading_pic, CL_GetClientGame());
	scr.net_pic = R_RegisterPic("net");
	scr.font_pic = R_RegisterFont(scr_font->string);
	scr_crosshair_changed(scr_crosshair);
	// Generations
	// face states
	int facenum = 0;
	char namebuf[MAX_QPATH];

	for (i = 0; i < ST_NUMPAINFACES; i++)
	{
		for (j = 0; j < ST_NUMSTRAIGHTFACES; j++)
		{
			Q_snprintf(namebuf, sizeof(namebuf), "doom/STFST%d%d.d2p", i, j);
			scr.sb_doom_faces[facenum++] = R_RegisterPic(namebuf);
		}

		Q_snprintf(namebuf, sizeof(namebuf), "doom/STFTR%d0.d2p", i);	// turn right
		scr.sb_doom_faces[facenum++] = R_RegisterPic(namebuf);
		Q_snprintf(namebuf, sizeof(namebuf), "doom/STFTL%d0.d2p", i);	// turn left
		scr.sb_doom_faces[facenum++] = R_RegisterPic(namebuf);
		Q_snprintf(namebuf, sizeof(namebuf), "doom/STFOUCH%d.d2p", i);	// ouch!
		scr.sb_doom_faces[facenum++] = R_RegisterPic(namebuf);
		Q_snprintf(namebuf, sizeof(namebuf), "doom/STFEVL%d.d2p", i);	// evil grin ;)
		scr.sb_doom_faces[facenum++] = R_RegisterPic(namebuf);
		Q_snprintf(namebuf, sizeof(namebuf), "doom/STFKILL%d.d2p", i);	// pissed off
		scr.sb_doom_faces[facenum++] = R_RegisterPic(namebuf);
	}

	scr.sb_doom_faces[facenum++] = R_RegisterPic("doom/STFGOD0.d2p");
	scr.sb_doom_faces[facenum++] = R_RegisterPic("doom/STFDEAD0.d2p");
	SCR_FreeHUDLayouts();
}

static void scr_font_changed(cvar_t *self)
{
	scr.font_pic = R_RegisterFont(self->string);
}

static void scr_scale_changed(cvar_t *self)
{
	scr.hud_scale = R_ClampScale(self);
}

static const cmdreg_t scr_cmds[] =
{
	{ "timerefresh", SCR_TimeRefresh_f },
	{ "sizeup", SCR_SizeUp_f },
	{ "sizedown", SCR_SizeDown_f },
	{ "sky", SCR_Sky_f },
	{ "draw", SCR_Draw_f, SCR_Draw_c },
	{ "undraw", SCR_UnDraw_f, SCR_UnDraw_c },
	{ "clearchathud", SCR_ClearChatHUD_f },
	{ NULL }
};

/*
==================
SCR_Init
==================
*/
void SCR_Init(void)
{
	colorTable[0] = U32_BLACK;
	colorTable[1] = U32_RED;
	colorTable[2] = U32_GREEN;
	colorTable[3] = U32_YELLOW;
	colorTable[4] = U32_BLUE;
	colorTable[5] = U32_CYAN;
	colorTable[6] = U32_MAGENTA;
	colorTable[7] = U32_WHITE;

	scr_viewsize = Cvar_Get("viewsize", "100", CVAR_ARCHIVE);
	scr_showpause = Cvar_Get("scr_showpause", "1", CVAR_ARCHIVE);
	scr_centertime = Cvar_Get("scr_centertime", "2.5", CVAR_ARCHIVE);
	// Generations
	scr_font = Cvar_Get("scr_font", "%conchars", 0);
	scr_font->changed = scr_font_changed;
	scr_scale = Cvar_Get("scr_scale", "0", CVAR_ARCHIVE);
	scr_scale->changed = scr_scale_changed;
	scr_crosshair = Cvar_Get("crosshair", "0", CVAR_ARCHIVE);
	scr_crosshair->changed = scr_crosshair_changed;
	scr_chathud = Cvar_Get("scr_chathud", "0", CVAR_ARCHIVE);
	scr_chathud_lines = Cvar_Get("scr_chathud_lines", "4", CVAR_ARCHIVE);
	scr_chathud_time = Cvar_Get("scr_chathud_time", "0", CVAR_ARCHIVE);
	scr_chathud_time->changed = cl_timeout_changed;
	scr_chathud_time->changed(scr_chathud_time);
	scr_chathud_x = Cvar_Get("scr_chathud_x", "8", CVAR_ARCHIVE);
	scr_chathud_y = Cvar_Get("scr_chathud_y", "-64", CVAR_ARCHIVE);
	ch_health = Cvar_Get("ch_health", "0", CVAR_ARCHIVE);
	ch_health->changed = scr_crosshair_changed;
	ch_red = Cvar_Get("ch_red", "1", CVAR_ARCHIVE);
	ch_red->changed = scr_crosshair_changed;
	ch_green = Cvar_Get("ch_green", "1", CVAR_ARCHIVE);
	ch_green->changed = scr_crosshair_changed;
	ch_blue = Cvar_Get("ch_blue", "1", CVAR_ARCHIVE);
	ch_blue->changed = scr_crosshair_changed;
	ch_alpha = Cvar_Get("ch_alpha", "1", CVAR_ARCHIVE);
	ch_alpha->changed = scr_crosshair_changed;
	ch_scale = Cvar_Get("ch_scale", "1", CVAR_ARCHIVE);
	ch_scale->changed = scr_crosshair_changed;
	ch_x = Cvar_Get("ch_x", "0", CVAR_ARCHIVE);
	ch_y = Cvar_Get("ch_y", "0", CVAR_ARCHIVE);
	scr_draw2d = Cvar_Get("scr_draw2d", "2", 0);
	scr_showturtle = Cvar_Get("scr_showturtle", "1", 0);
	scr_lag_x = Cvar_Get("scr_lag_x", "-1", 0);
	scr_lag_y = Cvar_Get("scr_lag_y", "-1", 0);
	scr_lag_draw = Cvar_Get("scr_lag_draw", "0", 0);
	scr_lag_min = Cvar_Get("scr_lag_min", "0", 0);
	scr_lag_max = Cvar_Get("scr_lag_max", "200", 0);
	scr_alpha = Cvar_Get("scr_alpha", "1", 0);
#ifdef _DEBUG
	scr_showstats = Cvar_Get("scr_showstats", "0", 0);
	scr_showpmove = Cvar_Get("scr_showpmove", "0", 0);
#endif
	// Paril
	scr_gunfov = Cvar_Get("scr_gunfov", "75", CVAR_ARCHIVE);
	Cmd_Register(scr_cmds);
	scr_scale_changed(scr_scale);
	scr.initialized = true;
}

void SCR_Shutdown(void)
{
	Cmd_Deregister(scr_cmds);
	scr.initialized = false;
}

//=============================================================================

void SCR_FinishCinematic(void)
{
	// tell the server to advance to the next map / cinematic
	CL_ClientCommand(va("nextserver %i\n", cl.servercount));
}

void SCR_PlayCinematic(const char *name)
{
	// only static pictures are supported
	if (COM_CompareExtension(name, ".pcx"))
	{
		SCR_FinishCinematic();
		return;
	}

	cl.precache[0].type = PRECACHE_PIC;
	cl.precache[0].image = R_RegisterPic2(name);

	if (!cl.precache[0].image)
	{
		SCR_FinishCinematic();
		return;
	}

	// save picture name for reloading
	Q_strlcpy(cl.mapname, name, sizeof(cl.mapname));
	cls.state = ca_cinematic;
	SCR_EndLoadingPlaque();     // get rid of loading plaque
	Con_Close(false);           // get rid of connection screen
}

/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque(void)
{
	if (!cls.state)
		return;

	if (cls.disable_screen)
		return;

#ifdef _DEBUG

	if (developer->integer)
		return;

#endif

	// if at console or menu, don't bring up the plaque
	if (cls.key_dest & (KEY_CONSOLE | KEY_MENU))
		return;

	scr.draw_loading = true;
	SCR_UpdateScreen();
	cls.disable_screen = Sys_Milliseconds();
}

/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque(void)
{
	if (!cls.state)
		return;

	cls.disable_screen = 0;
	Con_ClearNotify_f();
}

// Clear any parts of the tiled background that were drawn on last frame
static void SCR_TileClear(void)
{
	int top, bottom, left, right;

	//if (con.currentHeight == 1)
	//  return;     // full screen console

	// Generations
	switch (cl_entities[cl.clientNum + 1].current.game)
	{
		case GAME_Q1:
			top = scr_vrect.y + (scr_vrect.height - 48);
			bottom = scr_vrect.y + scr_vrect.height - 1;
			left = scr_vrect.x;
			right = left + scr_vrect.width - 1;
			R_TileClear(left, top, right, bottom, scr.q1_backtile_pic, CL_GetClientGame());
			break;

		case GAME_DOOM:
		case GAME_DUKE:
			top = scr_vrect.y + (scr_vrect.height - 32);
			bottom = scr_vrect.y + scr_vrect.height - 1;
			left = scr_vrect.x;
			right = left + scr_vrect.width - 1;
			R_TileClear(left, top, right, bottom, scr.d_backtile_pic, CL_GetClientGame());
			break;

		default:
			if (scr_viewsize->integer == 100)
				return;     // full screen rendering

			top = scr_vrect.y;
			bottom = top + scr_vrect.height - 1;
			left = scr_vrect.x;
			right = left + scr_vrect.width - 1;
			// clear above view screen
			R_TileClear(0, 0, r_config.width, top, scr.backtile_pic, CL_GetClientGame());
			// clear below view screen
			R_TileClear(0, bottom, r_config.width,
				r_config.height - bottom, scr.backtile_pic, CL_GetClientGame());
			// clear left of view screen
			R_TileClear(0, top, left, scr_vrect.height, scr.backtile_pic, CL_GetClientGame());
			// clear right of view screen
			R_TileClear(right, top, r_config.width - right,
				scr_vrect.height, scr.backtile_pic, CL_GetClientGame());
			break;
	}
}

/*
===============================================================================

STAT PROGRAMS

===============================================================================
*/

#define ICON_WIDTH  24
#define ICON_HEIGHT 24
#define DIGIT_WIDTH 16
#define ICON_SPACE  8

// Generations
#define DIGIT_Q1_WIDTH 24

#define HUD_DrawString(x, y, string) \
	R_DrawString(x, y, 0, MAX_STRING_CHARS, string, scr.font_pic, CL_GetClientGame())

#define HUD_DrawAltString(x, y, string) \
	R_DrawString(x, y, UI_XORCOLOR, MAX_STRING_CHARS, string, scr.font_pic, CL_GetClientGame())

#define HUD_DrawCenterString(x, y, string) \
	SCR_DrawStringMulti(x, y, UI_CENTER, MAX_STRING_CHARS, string, scr.font_pic, CL_GetClientGame())

#define HUD_DrawAltCenterString(x, y, string) \
	SCR_DrawStringMulti(x, y, UI_CENTER | UI_XORCOLOR, MAX_STRING_CHARS, string, scr.font_pic, CL_GetClientGame())

static void HUD_DrawNumber_Signed(int x, int y, int color, int width, int value, bool isSigned)
{
	char    num[16], *ptr;
	int     l;
	int     frame;
	int		digit_width;

	if (width < 1)
		return;

	// draw number string
	if (width > 5)
		width = 5;

	l = Q_scnprintf(num, sizeof(num), isSigned ? "%i" : "%u", value);

	if (l > width)
		l = width;

	if (cl_entities[cl.clientNum + 1].current.game == GAME_Q2)
		x += 2;

	R_GetPicSize(&digit_width, NULL, scr.sb_pics[color][0], CL_GetClientGame());
	x += digit_width * (width - l);
	ptr = num;

	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr - '0';

		R_DrawPic(x, y, scr.sb_pics[color][frame], CL_GetClientGame());
		x += digit_width;
		ptr++;
		l--;
	}
}

#define HUD_DrawNumber(x, y, color, width, value) HUD_DrawNumber_Signed(x, y, color, width, value, true)

static void HUD_DrawNumber_Reverse(int x, int y, int color, int width, int value, bool percent, int digit_spacing)
{
	char    num[16], *ptr;
	int     l;
	int     frame;

	if (width < 1)
		return;

	// draw number string
	if (width > 5)
		width = 5;

	l = Q_scnprintf(num, sizeof(num), percent ? "%i-" : "%i", value);

	if (l > width)
		l = width;

	if (cl_entities[cl.clientNum + 1].current.game == GAME_Q2)
		x += 2;

	int digit_width, pct_size = 0;

	if (percent)
		R_GetPicSize(&pct_size, NULL, scr.sb_pics[color][STAT_MINUS], CL_GetClientGame());
	else
		pct_size = 0;

	ptr = num + l - 1;

	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr - '0';

		R_GetPicSize(&digit_width, NULL, scr.sb_pics[color][frame], CL_GetClientGame());
		digit_width += digit_spacing;
		R_DrawPic(x + pct_size - digit_width, y, scr.sb_pics[color][frame], CL_GetClientGame());
		x -= digit_width;
		--ptr;
		l--;
	}
}

static void R_DrawDoomNum(int x, int y, int width, int value)
{
	char    num[16], *ptr;
	int     l;
	int     frame;

	if (width < 1)
		return;

	// draw number string
	if (width > 5)
		width = 5;

	l = Q_scnprintf(num, sizeof(num), "%i", value);

	if (l > width)
		l = width;

	int digit_width;
	ptr = num + l - 1;

	while (*ptr && l)
	{
		frame = *ptr - '0';
		R_GetPicSize(&digit_width, NULL, scr.sb_doom_nums[frame], CL_GetClientGame());
		R_DrawPic(x - digit_width, y, scr.sb_doom_nums[frame], CL_GetClientGame());
		x -= digit_width;
		--ptr;
		l--;
	}
}

static void R_DrawDukeNum(int x, int y, int width, int value, bool reverse)
{
	char    num[16], *ptr;
	int     l;
	int     frame;

	if (width < 1)
		return;

	// draw number string
	if (width > 3)
		width = 3;

	l = Q_scnprintf(num, sizeof(num), "%i", value);

	if (l > width)
		l = width;

	int digit_width;

	if (reverse)
	{
		ptr = num + l - 1;

		while (*ptr && l)
		{
			frame = *ptr - '0';
			R_GetPicSize(&digit_width, NULL, scr.sb_duke_nums[frame], CL_GetClientGame());
			R_DrawPic(x - digit_width, y, scr.sb_duke_nums[frame], CL_GetClientGame());
			x -= digit_width + 1;
			--ptr;
			l--;
		}
	}
	else
	{
		ptr = num;

		while (*ptr && l)
		{
			frame = *ptr - '0';
			R_GetPicSize(&digit_width, NULL, scr.sb_duke_nums[frame], CL_GetClientGame());
			R_DrawPic(x, y, scr.sb_duke_nums[frame], CL_GetClientGame());
			x += digit_width + 1;
			++ptr;
			l--;
		}
	}
}

#define DISPLAY_ITEMS   17

static void SCR_DrawInventory(void)
{
	int     i;
	int     	num, selected_num, item;
	itemid_e    index[ITI_TOTAL];
	char    	string[MAX_STRING_CHARS];
	int     	x, y;
	char    	*bind;
	int     	selected;
	int     	top;

	if (!(cl.frame.ps.stats.layouts & 2))
		return;

	selected = cl.frame.ps.stats.selected_item;
	num = 0;
	selected_num = 0;

	for (i = 0; i < ITI_TOTAL; i++)
	{
		if (i == selected)
			selected_num = num;

		if (cl.inventory[i])
			index[num++] = i;
	}

	// determine scroll point
	top = selected_num - DISPLAY_ITEMS / 2;

	if (top > num - DISPLAY_ITEMS)
		top = num - DISPLAY_ITEMS;

	if (top < 0)
		top = 0;

	x = (scr.hud_width - 256) / 2;
	y = (scr.hud_height - 240) / 2;
	R_DrawPic(x, y + 8, scr.inven_pic, CL_GetClientGame());
	y += 24;
	x += 24;
	HUD_DrawString(x, y, "hotkey ### item");
	y += CHAR_HEIGHT;
	HUD_DrawString(x, y, "------ --- ----");
	y += CHAR_HEIGHT;

	for (i = top; i < num && i < top + DISPLAY_ITEMS; i++)
	{
		item = index[i];
		// search for a binding
		Q_concat(string, sizeof(string),
			"use ", cl.configstrings[CS_ITEMS + item], NULL);
		bind = Key_GetBinding(string);
		Q_snprintf(string, sizeof(string), "%6s %3i %s",
			bind, cl.inventory[item], cl.configstrings[CS_ITEMS + item]);

		if (item != selected)
			HUD_DrawAltString(x, y, string);
		else      // draw a blinky cursor by the selected item
		{
			HUD_DrawString(x, y, string);

			if ((cls.realtime >> 8) & 1)
				R_DrawChar(x - CHAR_WIDTH, y, 0, 15, scr.font_pic, CL_GetClientGame());
		}

		y += CHAR_HEIGHT;
	}
}

// Generations
typedef enum
{
	LAYOUT_NONE,

	LAYOUT_PIC,
	LAYOUT_CLIENT,
	LAYOUT_CTF,

	LAYOUT_PICN,
	LAYOUT_NUM,
	LAYOUT_HNUM,
	LAYOUT_ANUM,
	LAYOUT_RNUM,
	LAYOUT_STAT_STRING,
	LAYOUT_CSTRING,
	LAYOUT_CSTRING2,
	LAYOUT_STRING,
	LAYOUT_STRING2,
	LAYOUT_IF,
	LAYOUT_IFBIT,
	LAYOUT_COLOR,

	LAYOUT_Q1_HNUM,
	LAYOUT_Q1_ANUM,
	LAYOUT_Q1_RNUM,
	LAYOUT_Q1_AMMO,
	LAYOUT_Q1_WEAPON,
	LAYOUT_Q1_ITEM,
	LAYOUT_Q1_FACE,

	LAYOUT_DOOM_HNUM,
	LAYOUT_DOOM_ANUM,
	LAYOUT_DOOM_RNUM,
	LAYOUT_DOOM_WEAPON,
	LAYOUT_DOOM_AMMO,
	LAYOUT_DOOM_FACE,

	LAYOUT_DUKE_HNUM,
	LAYOUT_DUKE_ANUM,
	LAYOUT_DUKE_RNUM,

	LAYOUT_DUKE_WEAPON
} layout_type_t;

typedef enum
{
	LAYOUT_POS_LEFT = 0,
	LAYOUT_POS_TOP = 0,
	LAYOUT_POS_RIGHT = 1,
	LAYOUT_POS_BOTTOM = 1,
	LAYOUT_POS_VIRTUAL = 2
} layout_position_type_t;

typedef struct
{
	layout_position_type_t	position;
	int						offset;
} layout_position_t;

static int SCR_LayoutOffsetX(const layout_position_t *position)
{
	switch (position->position)
	{
		case LAYOUT_POS_LEFT:
			return position->offset;

		case LAYOUT_POS_RIGHT:
			return scr.hud_width + position->offset;

		case LAYOUT_POS_VIRTUAL:
		default:
			return (scr.hud_width / 2) - 160 + position->offset;
	}
}

static int SCR_LayoutOffsetY(const layout_position_t *position)
{
	switch (position->position)
	{
		case LAYOUT_POS_TOP:
			return position->offset;

		case LAYOUT_POS_BOTTOM:
			return scr.hud_height + position->offset;

		case LAYOUT_POS_VIRTUAL:
		default:
			return (scr.hud_height / 2) - 120 + position->offset;
	}
}

typedef enum
{
	LAYOUT_TYPE_UINT8,
	LAYOUT_TYPE_INT8,
	LAYOUT_TYPE_UINT16,
	LAYOUT_TYPE_INT16,
	LAYOUT_TYPE_UINT32,
	LAYOUT_TYPE_INT32
} layout_stat_type_t;

typedef struct layout_string_s layout_string_t;

typedef struct
{
	uint8_t		offset;
} layout_string_pic_t;

typedef struct
{
	int		client;
	int		score;
	int		ping;
	int		time;
} layout_string_client_t;

typedef struct
{
	qhandle_t	pic;
} layout_string_picn_t;

typedef struct
{
	uint8_t				width;
	uint8_t				offset;
	layout_stat_type_t	type;
} layout_string_num_t;

typedef struct
{
	char		string[MAX_QPATH];
} layout_string_cstring_t;

typedef struct
{
	uint8_t					offset;
	layout_stat_type_t		type;
	layout_string_t			*entries;
} layout_string_if_t;

typedef struct
{
	uint8_t					offset;
	uint32_t				bits;
	layout_stat_type_t		type;
	layout_string_t			*entries;
} layout_string_ifbit_t;

typedef struct
{
	color_t		color;
} layout_string_color_t;

typedef struct
{
	itemid_e	type;
} layout_string_q1_ammo_t;

typedef struct
{
	int			weapon_index;
	int			stat_bit;
	qhandle_t	selected_pic;
	qhandle_t	unselected_pic;
} layout_string_q1_weapon_t;

typedef struct
{
	int			weapon_index;
	int			stat_bit;
} layout_string_doom_weapon_t;

typedef struct
{
	itemid_e	type;
} layout_string_doom_ammo_t;

typedef struct
{
	int			weapon_index, weapon_bit;
	int			width;
} layout_string_duke_weapon_t;

typedef struct layout_string_s
{
	layout_string_t			*parent;
	layout_position_t		x, y;
	layout_type_t			type;

	union
	{
		layout_string_pic_t			pic;
		layout_string_client_t		client;
		layout_string_picn_t		picn;
		layout_string_num_t			num;
		layout_string_pic_t			stat_string;
		layout_string_cstring_t		string;
		layout_string_color_t		color;
		layout_string_if_t			if_stmt;
		layout_string_ifbit_t		ifbit_stmt;
		layout_string_q1_ammo_t		q1_ammo;
		layout_string_q1_weapon_t	q1_weapon;
		layout_string_doom_weapon_t	doom_weapon;
		layout_string_doom_ammo_t	doom_ammo;
		layout_string_duke_weapon_t	duke_weapon;
	};

	layout_string_t			*next, *mem_next;
} layout_string_t;

#define SCR_LayoutAlloc() (layout_string_t*)Z_TagMallocz(sizeof(layout_string_t), TAG_LAYOUT)

typedef struct
{
	const char *stat;
	uint8_t offset;
} stat_offset_t;

uint8_t SCR_ParseLayoutStatOffset(const char *token)
{
	if (token[0] != '$')
		Com_Error(ERR_DROP, "%s: invalid stat offset %s", __func__, token);

#define STAT_OFFSET(name) { #name, offsetof(player_stats_t, name) }
	static stat_offset_t stat_offsets[] =
	{
		STAT_OFFSET(ammo_icon),
		STAT_OFFSET(armor_icon),
		STAT_OFFSET(health),
		STAT_OFFSET(ammo),
		STAT_OFFSET(armor),
		STAT_OFFSET(frags),
		STAT_OFFSET(chase),
		STAT_OFFSET(selected_item),
		STAT_OFFSET(q2.health_icon),
		STAT_OFFSET(q2.pickup_icon),
		STAT_OFFSET(q2.timer_icon),
		STAT_OFFSET(q2.selected_icon),
		STAT_OFFSET(q2.help_icon),
		STAT_OFFSET(q2.timer),
		STAT_OFFSET(q2.pickup_string),
		STAT_OFFSET(visual_bits),
		{ NULL }
	};
	stat_offset_t *o;

	for (o = stat_offsets; o->stat; o++)
	{
		if (Q_stricmp(token + 1, o->stat) != 0)
			continue;

		return o->offset;
	}

	Com_Error(ERR_DROP, "%s: invalid stat offset %s", __func__, token);
}

layout_string_t *SCR_ParseLayoutString(const char *filename)
{
	layout_position_t x = { LAYOUT_POS_LEFT, 0 }, y = { LAYOUT_POS_TOP, 0 };
	char    *token;
	layout_string_t *layout_string, *entry, *mem_entry;
	char *buf;
	int len = FS_LoadFile(filename, (void **)&buf);

	if (len < 0)
		return NULL;

	const char *s = buf;
	layout_string = SCR_LayoutAlloc();
	entry = mem_entry = layout_string;

	while (s)
	{
		token = COM_Parse(&s);

		// stuff that doesn't make an entry
		if (token[2] == 0)
		{
			if (token[0] == 'x')
			{
				if (token[1] == 'l')
					x.position = LAYOUT_POS_LEFT;
				else if (token[1] == 'r')
					x.position = LAYOUT_POS_RIGHT;
				else if (token[1] == 'v')
					x.position = LAYOUT_POS_VIRTUAL;

				token = COM_Parse(&s);
				x.offset = atoi(token);
				continue;
			}
			else if (token[0] == 'y')
			{
				if (token[1] == 't')
					y.position = LAYOUT_POS_TOP;
				else if (token[1] == 'b')
					y.position = LAYOUT_POS_BOTTOM;
				else if (token[1] == 'v')
					y.position = LAYOUT_POS_VIRTUAL;

				token = COM_Parse(&s);
				y.offset = atoi(token);
				continue;
			}
		}
		else if (!Q_strcasecmp(token, "endif"))
		{
			if (!entry || !entry->parent)
				Com_Error(ERR_DROP, "%s: invalid endif", __func__);

			// back out of the current if
			entry = entry->parent;
			continue;
		}

		// stuff that does make a new entry
		if (entry->type != LAYOUT_NONE)
		{
			layout_string_t *new_entry = SCR_LayoutAlloc();
			entry->next = new_entry;
			mem_entry->mem_next = new_entry;
			new_entry->parent = entry->parent;
			entry = new_entry;
			mem_entry = entry;
		}

		entry->x = x;
		entry->y = y;

		if (!strcmp(token, "ifbit"))
		{
			entry->type = LAYOUT_IFBIT;
			token = COM_Parse(&s);
			entry->ifbit_stmt.offset = SCR_ParseLayoutStatOffset(token);

			if (entry->ifbit_stmt.offset < 0 || entry->ifbit_stmt.offset >= sizeof(player_stats_t))
				Com_Error(ERR_DROP, "%s: invalid stat index", __func__);

			token = COM_Parse(&s);

			if (Q_stricmp(token, "uint8") == 0 || Q_stricmp(token, "int8") == 0)
				entry->ifbit_stmt.type = LAYOUT_TYPE_UINT8;
			else if (Q_stricmp(token, "uint16") == 0 || Q_stricmp(token, "int16") == 0)
				entry->ifbit_stmt.type = LAYOUT_TYPE_UINT16;
			else if (Q_stricmp(token, "uint32") == 0 || Q_stricmp(token, "uint32") == 0)
				entry->ifbit_stmt.type = LAYOUT_TYPE_UINT32;
			else
				Com_Error(ERR_DROP, "%s: invalid stat type", __func__);

			token = COM_Parse(&s);
			entry->ifbit_stmt.bits = atoi(token);
			entry->ifbit_stmt.entries = SCR_LayoutAlloc();
			entry->ifbit_stmt.entries->parent = entry;
			entry = entry->ifbit_stmt.entries;
			mem_entry->mem_next = entry;
			mem_entry = entry;
			continue;
		}
		else if (!strcmp(token, "if"))
		{
			entry->type = LAYOUT_IF;
			token = COM_Parse(&s);
			entry->if_stmt.offset = SCR_ParseLayoutStatOffset(token);

			if (entry->if_stmt.offset < 0 || entry->if_stmt.offset >= sizeof(player_stats_t))
				Com_Error(ERR_DROP, "%s: invalid stat index", __func__);

			token = COM_Parse(&s);

			if (Q_stricmp(token, "uint8") == 0 || Q_stricmp(token, "int8") == 0)
				entry->if_stmt.type = LAYOUT_TYPE_UINT8;
			else if (Q_stricmp(token, "uint16") == 0 || Q_stricmp(token, "int16") == 0)
				entry->if_stmt.type = LAYOUT_TYPE_UINT16;
			else if (Q_stricmp(token, "uint32") == 0 || Q_stricmp(token, "uint32") == 0)
				entry->if_stmt.type = LAYOUT_TYPE_UINT32;
			else
				Com_Error(ERR_DROP, "%s: invalid stat type", __func__);

			entry->if_stmt.entries = SCR_LayoutAlloc();
			entry->if_stmt.entries->parent = entry;
			entry = entry->if_stmt.entries;
			mem_entry->mem_next = entry;
			mem_entry = entry;
			continue;
		}
		else if (!strcmp(token, "pic"))
		{
			// draw a pic from a stat number
			entry->type = LAYOUT_PIC;
			token = COM_Parse(&s);
			entry->pic.offset = SCR_ParseLayoutStatOffset(token);

			if (entry->pic.offset < 0 || entry->pic.offset >= sizeof(player_stats_t))
				Com_Error(ERR_DROP, "%s: invalid stat index", __func__);

			continue;
		}
		else if (!strcmp(token, "client"))
		{
			// draw a deathmatch client block
			entry->type = LAYOUT_CLIENT;
			token = COM_Parse(&s);
			x.position = LAYOUT_POS_VIRTUAL;
			y.offset = atoi(token);
			token = COM_Parse(&s);
			y.position = LAYOUT_POS_VIRTUAL;
			y.offset = atoi(token);
			entry->x = x;
			entry->y = y;
			token = COM_Parse(&s);
			entry->client.client = atoi(token);

			if (entry->client.client < 0 || entry->client.client >= MAX_CLIENTS)
				Com_Error(ERR_DROP, "%s: invalid client index", __func__);

			token = COM_Parse(&s);
			entry->client.score = atoi(token);
			token = COM_Parse(&s);
			entry->client.ping = atoi(token);
			token = COM_Parse(&s);
			entry->client.time = atoi(token);
			continue;
		}
		else if (!strcmp(token, "ctf"))
		{
			// draw a ctf client block
			entry->type = LAYOUT_CTF;
			token = COM_Parse(&s);
			x.position = LAYOUT_POS_VIRTUAL;
			y.offset = atoi(token);
			token = COM_Parse(&s);
			y.position = LAYOUT_POS_VIRTUAL;
			y.offset = atoi(token);
			entry->x = x;
			entry->y = y;
			token = COM_Parse(&s);
			entry->client.client = atoi(token);

			if (entry->client.client < 0 || entry->client.client >= MAX_CLIENTS)
				Com_Error(ERR_DROP, "%s: invalid client index", __func__);

			token = COM_Parse(&s);
			entry->client.score = atoi(token);
			token = COM_Parse(&s);
			entry->client.ping = min(999, atoi(token));
			continue;
		}
		else if (!strcmp(token, "picn"))
		{
			// draw a pic from a name
			entry->type = LAYOUT_PICN;
			token = COM_Parse(&s);
			entry->picn.pic = R_RegisterPic2(token);
			continue;
		}
		else if (!strcmp(token, "num"))
		{
			// draw a number
			entry->type = LAYOUT_NUM;
			token = COM_Parse(&s);
			entry->num.width = atoi(token);
			token = COM_Parse(&s);
			entry->num.offset = SCR_ParseLayoutStatOffset(token);

			if (entry->num.offset < 0 || entry->num.offset >= sizeof(player_stats_t))
				Com_Error(ERR_DROP, "%s: invalid stat index", __func__);

			token = COM_Parse(&s);

			if (Q_stricmp(token, "uint8") == 0)
				entry->num.type = LAYOUT_TYPE_UINT8;
			else if (Q_stricmp(token, "int8") == 0)
				entry->num.type = LAYOUT_TYPE_INT8;
			else if (Q_stricmp(token, "uint16") == 0)
				entry->num.type = LAYOUT_TYPE_UINT16;
			else if (Q_stricmp(token, "int16") == 0)
				entry->num.type = LAYOUT_TYPE_INT16;
			else if (Q_stricmp(token, "uint32") == 0)
				entry->num.type = LAYOUT_TYPE_UINT32;
			else if (Q_stricmp(token, "int32") == 0)
				entry->num.type = LAYOUT_TYPE_INT32;
			else
				Com_Error(ERR_DROP, "%s: invalid stat type", __func__);

			continue;
		}
		else if (!strcmp(token, "hnum"))
		{
			// health number
			entry->type = LAYOUT_HNUM;
			continue;
		}
		else if (!strcmp(token, "anum"))
		{
			// ammo number
			entry->type = LAYOUT_ANUM;
			continue;
		}
		else if (!strcmp(token, "rnum"))
		{
			// armor number
			entry->type = LAYOUT_RNUM;
			continue;
		}
		else if (!strcmp(token, "q1_hnum"))
		{
			// health number
			entry->type = LAYOUT_Q1_HNUM;
			continue;
		}
		else if (!strcmp(token, "q1_anum"))
		{
			// ammo number
			entry->type = LAYOUT_Q1_ANUM;
			continue;
		}
		else if (!strcmp(token, "q1_rnum"))
		{
			// armor number
			entry->type = LAYOUT_Q1_RNUM;
			continue;
		}
		else if (!strcmp(token, "q1_ammo"))
		{
			entry->type = LAYOUT_Q1_AMMO;
			token = COM_Parse(&s);

			if (Q_stricmp(token, "shells") == 0)
				entry->q1_ammo.type = 0;
			else if (Q_stricmp(token, "nails") == 0)
				entry->q1_ammo.type = 1;
			else if (Q_stricmp(token, "rockets") == 0)
				entry->q1_ammo.type = 2;
			else if (Q_stricmp(token, "cells") == 0)
				entry->q1_ammo.type = 3;
			else
				Com_Error(ERR_DROP, "%s: invalid ammo type %s", __func__, token);

			continue;
		}
		else if (!strcmp(token, "q1_weapon"))
		{
			entry->type = LAYOUT_Q1_WEAPON;
			token = COM_Parse(&s);
			entry->q1_weapon.weapon_index = atoi(token);
			entry->q1_weapon.stat_bit = 1 << (entry->q1_weapon.weapon_index - 1);
			token = COM_Parse(&s);
			entry->q1_weapon.unselected_pic = R_RegisterPic2(token);
			token = COM_Parse(&s);
			entry->q1_weapon.selected_pic = R_RegisterPic2(token);
			continue;
		}
		else if (!strcmp(token, "q1_item"))
		{
			entry->type = LAYOUT_Q1_ITEM;
			token = COM_Parse(&s);
			entry->q1_weapon.weapon_index = atoi(token);
			entry->q1_weapon.stat_bit = 1 << (entry->q1_weapon.weapon_index - 1);
			token = COM_Parse(&s);
			entry->q1_weapon.selected_pic = R_RegisterPic2(token);
			continue;
		}
		else if (!strcmp(token, "q1_face"))
		{
			entry->type = LAYOUT_Q1_FACE;
			continue;
		}
		else if (!strcmp(token, "doom_hnum"))
		{
			// health number
			entry->type = LAYOUT_DOOM_HNUM;
			continue;
		}
		else if (!strcmp(token, "doom_anum"))
		{
			// ammo number
			entry->type = LAYOUT_DOOM_ANUM;
			continue;
		}
		else if (!strcmp(token, "doom_rnum"))
		{
			// armor number
			entry->type = LAYOUT_DOOM_RNUM;
			continue;
		}
		else if (!strcmp(token, "doom_weapon"))
		{
			// doom weapon
			entry->type = LAYOUT_DOOM_WEAPON;
			token = COM_Parse(&s);
			entry->doom_weapon.weapon_index = atoi(token);
			entry->doom_weapon.stat_bit = 1 << (entry->doom_weapon.weapon_index - 1);
			continue;
		}
		else if (!strcmp(token, "doom_ammo"))
		{
			// doom ammo
			entry->type = LAYOUT_DOOM_AMMO;
			token = COM_Parse(&s);

			if (Q_stricmp(token, "bullets") == 0)
				entry->doom_ammo.type = 0;
			else if (Q_stricmp(token, "shells") == 0)
				entry->doom_ammo.type = 1;
			else if (Q_stricmp(token, "rockets") == 0)
				entry->doom_ammo.type = 2;
			else if (Q_stricmp(token, "cells") == 0)
				entry->doom_ammo.type = 3;
			else
				Com_Error(ERR_DROP, "%s: invalid ammo type %s", __func__, token);

			continue;
		}
		else if (!strcmp(token, "doom_face"))
		{
			// doom ammo
			entry->type = LAYOUT_DOOM_FACE;
			continue;
		}
		else if (!strcmp(token, "duke_hnum"))
		{
			// health number
			entry->type = LAYOUT_DUKE_HNUM;
			continue;
		}
		else if (!strcmp(token, "duke_anum"))
		{
			// health number
			entry->type = LAYOUT_DUKE_ANUM;
			continue;
		}
		else if (!strcmp(token, "duke_rnum"))
		{
			// armor number
			entry->type = LAYOUT_DUKE_RNUM;
			continue;
		}
		else if (!strcmp(token, "duke_weapon"))
		{
			// doom weapon
			entry->type = LAYOUT_DUKE_WEAPON;
			token = COM_Parse(&s);
			entry->duke_weapon.weapon_index = atoi(token);
			entry->duke_weapon.weapon_bit = 1 << entry->duke_weapon.weapon_index;
			token = COM_Parse(&s);
			entry->duke_weapon.width = atoi(token);
			continue;
		}
		else if (!strcmp(token, "stat_string"))
		{
			entry->type = LAYOUT_STAT_STRING;
			token = COM_Parse(&s);
			entry->stat_string.offset = SCR_ParseLayoutStatOffset(token);

			if (entry->stat_string.offset < 0 || entry->stat_string.offset >= sizeof(player_stats_t))
				Com_Error(ERR_DROP, "%s: invalid stat index", __func__);

			continue;
		}
		else if (!strcmp(token, "cstring"))
		{
			entry->type = LAYOUT_CSTRING;
			token = COM_Parse(&s);
			Q_snprintf(entry->string.string, sizeof(entry->string.string), "%s", token);
			continue;
		}
		else if (!strcmp(token, "cstring2"))
		{
			entry->type = LAYOUT_CSTRING2;
			token = COM_Parse(&s);
			Q_snprintf(entry->string.string, sizeof(entry->string.string), "%s", token);
			continue;
		}
		else if (!strcmp(token, "string"))
		{
			entry->type = LAYOUT_STRING;
			token = COM_Parse(&s);
			Q_snprintf(entry->string.string, sizeof(entry->string.string), "%s", token);
			continue;
		}
		else if (!strcmp(token, "string2"))
		{
			entry->type = LAYOUT_STRING2;
			token = COM_Parse(&s);
			Q_snprintf(entry->string.string, sizeof(entry->string.string), "%s", token);
			continue;
		}
		else if (!strcmp(token, "color"))
		{
			entry->type = LAYOUT_COLOR;
			token = COM_Parse(&s);

			if (SCR_ParseColor(token, &entry->color.color))
				entry->color.color.u8[3] *= scr_alpha->value;

			continue;
		}
		else if (!token[0])
			break;
		else
			Com_Error(ERR_DROP, "%s: invalid layout token %s", __func__, token);
	}

	FS_FreeFile(buf);
	return layout_string;
}


static void SCR_ExecuteLayoutStruct(const layout_string_t *s)
{
	if (!s || !s->type)
		return;

	int value;
	int index;
	const char *token;
	const layout_string_t *entry = s;
	const clientinfo_t *ci;
	char buffer[MAX_QPATH];
	int color;

	while (s)
	{
		switch (entry->type)
		{
			case LAYOUT_PIC:
				// draw a pic from a stat number
				value = *(uint16_t *)(((uint8_t *)&cl.frame.ps.stats) + entry->pic.offset);

				if (value <= 0 || value >= MAX_PRECACHE)
					Com_Error(ERR_DROP, "%s: invalid pic index", __func__);

				token = cl.configstrings[CS_PRECACHE + value];

				if (token[0])
					R_DrawPic(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), cl.precache[value].image, CL_GetClientGame());

				break;

			case LAYOUT_CLIENT:
				// draw a deathmatch client block
				ci = &cl.clientinfo[entry->client.client];
				HUD_DrawAltString(SCR_LayoutOffsetX(&entry->x) + 32, SCR_LayoutOffsetY(&entry->y), ci->name);
				HUD_DrawString(SCR_LayoutOffsetX(&entry->x) + 32, SCR_LayoutOffsetY(&entry->y) + CHAR_HEIGHT, "Score: ");
				Q_snprintf(buffer, sizeof(buffer), "%i", entry->client.score);
				HUD_DrawAltString(SCR_LayoutOffsetX(&entry->x) + 32 + 7 * CHAR_WIDTH, SCR_LayoutOffsetY(&entry->y) + CHAR_HEIGHT, buffer);
				Q_snprintf(buffer, sizeof(buffer), "Ping:  %i", entry->client.ping);
				HUD_DrawString(SCR_LayoutOffsetX(&entry->x) + 32, SCR_LayoutOffsetY(&entry->y) + 2 * CHAR_HEIGHT, buffer);
				Q_snprintf(buffer, sizeof(buffer), "Time:  %i", entry->client.time);
				HUD_DrawString(SCR_LayoutOffsetX(&entry->x) + 32, SCR_LayoutOffsetY(&entry->y) + 3 * CHAR_HEIGHT, buffer);

				if (!ci->icon)
					ci = &cl.baseclientinfo;

				R_DrawPic(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), ci->icon, CL_GetClientGame());
				break;

			case LAYOUT_CTF:
				// draw a ctf client block
				ci = &cl.clientinfo[entry->client.client];
				Q_snprintf(buffer, sizeof(buffer), "%3d %3d %-12.12s",
					entry->client.score, entry->client.ping, ci->name);

				if (entry->client.client == cl.frame.clientNum)
					HUD_DrawAltString(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), buffer);
				else
					HUD_DrawString(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), buffer);

				break;

			case LAYOUT_PICN:
				// draw a pic from a name
				R_DrawPic(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), entry->picn.pic, CL_GetClientGame());
				break;

			case LAYOUT_NUM:
				// draw a number
			{
				void *ptr = ((uint8_t *) &cl.frame.ps.stats) + entry->num.offset;

				if (entry->num.type == LAYOUT_TYPE_UINT8)
					value = *(uint8_t *)ptr;
				else if (entry->num.type == LAYOUT_TYPE_INT8)
					value = *(int8_t *)ptr;
				else if (entry->num.type == LAYOUT_TYPE_UINT16)
					value = *(uint16_t *)ptr;
				else if (entry->num.type == LAYOUT_TYPE_INT16)
					value = *(int16_t *)ptr;
				else
					value = *(int32_t *)ptr;

				HUD_DrawNumber_Signed(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), 0, entry->num.width, value, !!(entry->num.type & 1));
			}
			break;

			case LAYOUT_HNUM:
				// health number
				value = cl.frame.ps.stats.health;

				if (value > 25)
					color = 0;  // green
				else if (value > 0)
					color = ((cl.frame.number / CL_FRAMEDIV) >> 2) & 1;     // flash
				else
					color = 1;

				if (cl.frame.ps.stats.flashes & 1)
					R_DrawPic(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), scr.field_pic, CL_GetClientGame());

				HUD_DrawNumber(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), color, 3, value);
				break;

			case LAYOUT_ANUM:
				// ammo number
				value = cl.frame.ps.stats.ammo;

				if (value > 5)
					color = 0;  // green
				else if (value >= 0)
					color = ((cl.frame.number / CL_FRAMEDIV) >> 2) & 1;     // flash
				else
					break;   // negative number = don't show

				if (cl.frame.ps.stats.flashes & 4)
					R_DrawPic(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), scr.field_pic, CL_GetClientGame());

				HUD_DrawNumber(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), color, 3, value);
				break;

			case LAYOUT_RNUM:
				// armor number
				value = cl.frame.ps.stats.armor;

				if (value < 1)
					break;

				color = 0;  // green

				if (cl.frame.ps.stats.flashes & 2)
					R_DrawPic(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), scr.field_pic, CL_GetClientGame());

				HUD_DrawNumber(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), color, 3, value);
				break;

			case LAYOUT_Q1_HNUM:
				// health number
				value = cl.frame.ps.stats.health;
				color = 1;

				if (value > 25)
					color = 0;  // green

				HUD_DrawNumber(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), color, 3, value);
				break;

			case LAYOUT_Q1_ANUM:
				// ammo number
				value = cl.frame.ps.stats.ammo;
				color = 1;

				if (value > 10)
					color = 0;  // green

				HUD_DrawNumber(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), color, 3, value);
				break;

			case LAYOUT_Q1_RNUM:
				// armor number
				value = cl.frame.ps.stats.armor;
				color = 0;  // green

				if (value <= 25)
					color |= 1;

				HUD_DrawNumber(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), color, 3, value);
				break;

			case LAYOUT_Q1_AMMO:
			{
				value = 0;

				switch (entry->q1_ammo.type)
				{
					case 0:
						value = cl.frame.ps.stats.q1.ammo_shells;
						break;

					case 1:
						value = cl.frame.ps.stats.q1.ammo_nails;
						break;

					case 2:
						value = cl.frame.ps.stats.q1.ammo_rockets;
						break;

					case 3:
						value = cl.frame.ps.stats.q1.ammo_cells;
						break;
				}

				Q_snprintf(buffer, sizeof(buffer), "%3i", value);
				color_t old_color = R_GetColor();
				color_t new_color = { 0xFFFFFFFF };
				new_color.u8[0] = 240;
				new_color.u8[1] = 180;
				new_color.u8[2] = 80;
				R_SetColor(new_color.u32);
				HUD_DrawString(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), buffer);
				R_SetColor(old_color.u32);
			}
			break;

			case LAYOUT_Q1_WEAPON:
				value = cl.frame.ps.stats.q1.items & entry->q1_weapon.stat_bit;

				if (value)
					R_DrawPic(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y),
						(cl.frame.ps.stats.q1.cur_weap == entry->q1_weapon.weapon_index) ? entry->q1_weapon.selected_pic : entry->q1_weapon.unselected_pic, CL_GetClientGame());

				break;

			case LAYOUT_Q1_ITEM:
				value = cl.frame.ps.stats.q1.items & entry->q1_weapon.stat_bit;

				if (value)
					R_DrawPic(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y),
						entry->q1_weapon.selected_pic, CL_GetClientGame());

				break;

			case LAYOUT_Q1_FACE:
			{
				qhandle_t face;

				if ((cl.frame.ps.stats.q1.items & (IT_Q1_INVIS | IT_Q1_INVUL)) == (IT_Q1_INVIS | IT_Q1_INVUL))
					face = scr.sb_face_invis_invuln;
				else if (cl.frame.ps.stats.q1.items & IT_Q1_QUAD)
					face = scr.sb_face_quad;
				else if (cl.frame.ps.stats.q1.items & IT_Q1_INVIS)
					face = scr.sb_face_invis;
				else if (cl.frame.ps.stats.q1.items & IT_Q1_INVUL)
					face = scr.sb_face_invuln;
				else
				{
					int f, anim = 0;

					if (cl.frame.ps.stats.health >= 100)
						f = 4;
					else
						f = max(0, cl.frame.ps.stats.health / 20);

					anim = cl.frame.ps.stats.q1.face_anim ? 1 : 0;
					face = scr.sb_faces[f][anim];
				}

				R_DrawPic(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), face, CL_GetClientGame());
			}
			break;

			case LAYOUT_DOOM_HNUM:
				// health number
				value = max(0, (int)cl.frame.ps.stats.health);
				color = 0;
				HUD_DrawNumber_Reverse(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), color, 4, value, true, 0);
				break;

			case LAYOUT_DOOM_ANUM:
				// ammo number
				value = max(0, (int)cl.frame.ps.stats.ammo);
				color = 0;
				HUD_DrawNumber_Reverse(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), color, 3, value, false, 0);
				break;

			case LAYOUT_DOOM_RNUM:
				// health number
				value = max(0, (int)cl.frame.ps.stats.armor);
				color = 0;
				HUD_DrawNumber_Reverse(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), color, 4, value, true, 0);
				break;

			case LAYOUT_DOOM_WEAPON:
				// weapon number
				value = cl.frame.ps.stats.doom.weapons & entry->doom_weapon.stat_bit;
				R_DrawPic(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), value ? scr.sb_doom_nums[entry->doom_weapon.weapon_index] : scr.sb_doom_gray_nums[entry->doom_weapon.weapon_index], CL_GetClientGame());
				break;

			case LAYOUT_DOOM_AMMO:
				// ammo
			{
				uint16_t current_ammo = 0, max_ammo = 0;
#define SET_AMMO(id, type) case id: current_ammo = cl.frame.ps.stats.doom.type; max_ammo = cl.frame.ps.stats.doom.max_##type; break;

					switch (entry->doom_ammo.type)
					{
							SET_AMMO(0, ammo_bullets)
							SET_AMMO(1, ammo_shells)
							SET_AMMO(2, ammo_rockets)
							SET_AMMO(3, ammo_cells)
					}

#undef SET_AMMO
					R_DrawDoomNum(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), 3, current_ammo);
					layout_position_t max_x = entry->x;
					max_x.offset += 26;
					R_DrawDoomNum(SCR_LayoutOffsetX(&max_x), SCR_LayoutOffsetY(&entry->y), 3, max_ammo);
				}
				break;

			case LAYOUT_DOOM_FACE:
				value = cl.frame.ps.stats.doom.face;
				R_DrawPic(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), scr.sb_doom_faces[value], CL_GetClientGame());
				break;

			case LAYOUT_DUKE_HNUM:
				// health number
				value = max(0, (int)cl.frame.ps.stats.health);
				color = 0;
				HUD_DrawNumber_Reverse(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), color, 3, value, false, 1);
				break;

			case LAYOUT_DUKE_ANUM:
				// health number
				value = max(0, (int)cl.frame.ps.stats.ammo);
				color = 0;
				HUD_DrawNumber_Reverse(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), color, 3, value, false, 1);
				break;

			case LAYOUT_DUKE_RNUM:
				// health number
				value = max(0, (int)cl.frame.ps.stats.armor);
				color = 0;
				HUD_DrawNumber_Reverse(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), color, 3, value, false, 1);
				break;

			case LAYOUT_DUKE_WEAPON:
				// weapon number
			{
				const uint16_t available_weapons = cl.frame.ps.stats.duke.weapons & 0xFFFF;
				const uint8_t current_weapon = cl.frame.ps.stats.duke.selected_weapon;
				uint16_t current_ammo = 0, max_ammo = 0;
#define SET_AMMO(id, type) case id: current_ammo = cl.frame.ps.stats.duke.type; max_ammo = cl.frame.ps.stats.duke.max_##type; break;

					switch (entry->duke_weapon.weapon_index)
					{
							SET_AMMO(IT_DUKE_INDEX_PISTOL, ammo_clip)
							SET_AMMO(IT_DUKE_INDEX_SHOTGUN, ammo_shells)
							SET_AMMO(IT_DUKE_INDEX_CANNON, ammo_cannon)
							SET_AMMO(IT_DUKE_INDEX_RPG, ammo_rpg)
							SET_AMMO(IT_DUKE_INDEX_PIPE, ammo_pipebombs)
							SET_AMMO(IT_DUKE_INDEX_SHRINKER, ammo_shrinker)
							SET_AMMO(IT_DUKE_INDEX_DEVASTATOR, ammo_devastator)
							SET_AMMO(IT_DUKE_INDEX_TRIPWIRE, ammo_tripwire)
							SET_AMMO(IT_DUKE_INDEX_FREEZETHROWER, ammo_freezer)
					}

#undef SET_AMMO
					R_SetColor(ColorFromRGBA(255, 207, 10, 255).u32);
					R_DrawDukeNum(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), 3, entry->duke_weapon.weapon_index % 10, true);
					R_DrawPic(SCR_LayoutOffsetX(&entry->x) + 1, SCR_LayoutOffsetY(&entry->y) + 1, scr.sb_duke_nums[10], CL_GetClientGame());
					R_ClearColor();

					if (current_weapon == entry->duke_weapon.weapon_index)
						R_ClearColor();
					else if (available_weapons & entry->duke_weapon.weapon_bit)
						R_SetColor(ColorFromRGBA(127, 127, 127, 255).u32);
					else
						R_SetColor(ColorFromRGBA(80, 80, 80, 255).u32);

					int ammo_offset = 3 + ((3 + 1) * entry->duke_weapon.width) - 1;
					R_DrawDukeNum(SCR_LayoutOffsetX(&entry->x) + ammo_offset, SCR_LayoutOffsetY(&entry->y), entry->duke_weapon.width, current_ammo, true);
					ammo_offset += 1;
					R_DrawPic(SCR_LayoutOffsetX(&entry->x) + ammo_offset, SCR_LayoutOffsetY(&entry->y), scr.sb_duke_nums[11], CL_GetClientGame());
					R_DrawDukeNum(SCR_LayoutOffsetX(&entry->x) + ammo_offset + 5, SCR_LayoutOffsetY(&entry->y), entry->duke_weapon.width, max_ammo, false);
					R_ClearColor();
				}
				break;

			case LAYOUT_STAT_STRING:
				index = value = *(uint16_t *)(((uint8_t *)&cl.frame.ps.stats) + entry->stat_string.offset);

				if (index < 0 || index >= MAX_CONFIGSTRINGS)
					Com_Error(ERR_DROP, "%s: invalid string index", __func__);

				HUD_DrawString(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), cl.configstrings[index]);
				break;

			case LAYOUT_CSTRING:
				HUD_DrawCenterString(SCR_LayoutOffsetX(&entry->x) + 320 / 2, SCR_LayoutOffsetY(&entry->y), entry->string.string);
				break;

			case LAYOUT_CSTRING2:
				HUD_DrawAltCenterString(SCR_LayoutOffsetX(&entry->x) + 320 / 2, SCR_LayoutOffsetY(&entry->y), entry->string.string);
				break;

			case LAYOUT_STRING:
				HUD_DrawString(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), entry->string.string);
				break;

			case LAYOUT_STRING2:
				HUD_DrawAltString(SCR_LayoutOffsetX(&entry->x), SCR_LayoutOffsetY(&entry->y), entry->string.string);
				break;

			case LAYOUT_COLOR:
				R_SetColor(entry->color.color.u32);
				break;

			case LAYOUT_IF:
			{
				void *ptr = ((uint8_t *) &cl.frame.ps.stats) + entry->if_stmt.offset;

				if (entry->if_stmt.type <= LAYOUT_TYPE_INT8)
					value = *(uint8_t *)ptr;
				else if (entry->if_stmt.type <= LAYOUT_TYPE_INT16)
					value = *(uint16_t *)ptr;
				else
					value = *(uint32_t *)ptr;

				if (!value)
					break;

				entry = entry->if_stmt.entries;
				continue;
			}

			case LAYOUT_IFBIT:
			{
				void *ptr = ((uint8_t *) &cl.frame.ps.stats) + entry->ifbit_stmt.offset;

				if (entry->ifbit_stmt.type <= LAYOUT_TYPE_INT8)
					value = *(uint8_t *)ptr;
				else if (entry->ifbit_stmt.type <= LAYOUT_TYPE_INT16)
					value = *(uint16_t *)ptr;
				else
					value = *(uint32_t *)ptr;

				if (!(value & entry->ifbit_stmt.bits))
					break;

				entry = entry->ifbit_stmt.entries;
				continue;
			}
		}

		if (!entry->next)
		{
			if (entry->parent)
				entry = entry->parent;
			else
				break;
		}

		entry = entry->next;

		if (!entry)
			break;
	}

	R_ClearColor();
	R_SetAlpha(scr_alpha->value);
}

void SCR_FreeLayoutString(layout_string_t *layout)
{
	layout_string_t *entry;

	for (entry = layout; entry;)
	{
		layout_string_t *next = entry->mem_next;
		Z_Free(entry);
		entry = next;
	}
}

//=============================================================================

static void SCR_DrawPause(void)
{
	int x, y;

	if (!sv_paused->integer)
		return;

	if (!cl_paused->integer)
		return;

	if (scr_showpause->integer != 1)
		return;

	x = (scr.hud_width - scr.pause_width) / 2;
	y = (scr.hud_height - scr.pause_height) / 2;
	R_DrawPic(x, y, scr.pause_pic, CL_GetClientGame());
}

static void SCR_DrawLoading(void)
{
	int x, y;

	if (!scr.draw_loading)
		return;

	scr.draw_loading = false;
	R_SetScale(scr.hud_scale);
	x = (r_config.width * scr.hud_scale - scr.loading_width) / 2;
	y = (r_config.height * scr.hud_scale - scr.loading_height) / 2;
	R_DrawPic(x, y, scr.loading_pic, CL_GetClientGame());
	R_SetScale(1.0f);
}

static void SCR_DrawCrosshair(void)
{
	int x, y;

	if (!scr_crosshair->integer || !scr.crosshair_pic)
		return;

	x = (scr_vrect.x + (scr_vrect.width / 2)) * scr.hud_scale - (scr.crosshair_width / 2);
	y = (scr_vrect.y + (scr_vrect.height / 2)) * scr.hud_scale - (scr.crosshair_height / 2);
	R_SetColor(scr.crosshair_color.u32);
	R_DrawStretchPic(x + ch_x->integer,
		y + ch_y->integer,
		scr.crosshair_width,
		scr.crosshair_height,
		scr.crosshair_pic, CL_GetClientGame());
}


#if 0
	// cursor positioning
	xl <value>
	xr <value>
	yb <value>
	yt <value>
	xv <value>
	yv <value>

	// drawing
	statpic <name>
	pic <stat>
	num <fieldwidth> <stat>
	string <stat>

	// control
	if <stat>
	ifeq <stat> <value>
	ifbit <stat> <value>
	endif

#endif

layout_string_t *layout_singleplayer;

layout_string_t *layout_deathmatch;
layout_string_t *layout_q1;
layout_string_t *layout_doom;
layout_string_t *layout_duke;

void SCR_FreeHUDLayouts()
{
	if (!layout_singleplayer)
		return;

	SCR_FreeLayoutString(layout_singleplayer);
	SCR_FreeLayoutString(layout_deathmatch);
	SCR_FreeLayoutString(layout_q1);
	SCR_FreeLayoutString(layout_doom);
	SCR_FreeLayoutString(layout_duke);
	layout_singleplayer = NULL;
	layout_deathmatch = NULL;
	layout_q1 = NULL;
	layout_doom = NULL;
	layout_duke = NULL;
}

layout_string_t *SCR_GetHUDLayoutStruct()
{
	if (!layout_singleplayer)
	{
		layout_singleplayer = SCR_ParseLayoutString("hudscripts/q2_single.hsc");
		layout_deathmatch = SCR_ParseLayoutString("hudscripts/q2_dm.hsc");
		layout_q1 = SCR_ParseLayoutString("hudscripts/q1.hsc");
		layout_doom = SCR_ParseLayoutString("hudscripts/doom.hsc");
		layout_duke = SCR_ParseLayoutString("hudscripts/duke.hsc");
	}

	switch (cl_entities[cl.clientNum + 1].current.game)
	{
		default:
			switch (cl.gamemode)
			{
				case GAMEMODE_COOP:
				case GAMEMODE_SINGLEPLAYER:
					return layout_singleplayer;

				default:
					return layout_deathmatch;
			}

		case GAME_Q1:
			return layout_q1;

		case GAME_DOOM:
			return layout_doom;

		case GAME_DUKE:
			return layout_duke;
	}
}

// The status bar is a small layout program that is based on the stats array
static void SCR_DrawStats(void)
{
	if (scr_draw2d->integer <= 1)
		return;

	SCR_ExecuteLayoutStruct(SCR_GetHUDLayoutStruct());
}

static void SCR_DrawLayout(void)
{
	if (scr_draw2d->integer == 3 && !Key_IsDown(K_F1))
		return;

	if (!(cl.frame.ps.stats.layouts & 1))
		return;

	SCR_ExecuteLayoutStruct(cl.layout);
}

void R_DrawWeaponSprite(qhandle_t handle, gunindex_e index, float velocity, int time, float frametime, int oldframe, int frame, float frac, int hud_width, int hud_height, bool firing, float yofs, float scale, color_t color, bool invul, float height_diff);

static inline void MoveTowards(float *inout, const float *to, const float howmuch)
{
	if (*inout > *to)
	{
		if (*inout - howmuch > *to)
			*inout -= howmuch;
		else
			*inout = *to;
	}
	else if (*inout < *to)
	{
		if (*inout + howmuch < *to)
			*inout += howmuch;
		else
			*inout = *to;
	}
}

void GL_LightPoint(vec3_t origin, vec3_t color);

static void SCR_DrawWeapon()
{
	if (cl.thirdPersonView)
		return;

	static clipRect_t clr;
	clr.top = scr_vrect.y;
	clr.left = scr_vrect.x;
	clr.bottom = scr_vrect.y + scr_vrect.height;// * scr.hud_scale;
	clr.right = scr_vrect.x + scr_vrect.width;// * scr.hud_scale;
	R_SetScale(1.0f);
	R_SetClipRect(&clr);
	R_SetScale(scr.hud_scale);
	vec3_t vel, ovel;
	vel[0] = cl.frame.ps.pmove.velocity[0] * 0.125f;
	vel[1] = cl.frame.ps.pmove.velocity[1] * 0.125f;
	vel[2] = cl.frame.ps.pmove.velocity[2] * 0.125f;
	ovel[0] = cl.oldframe.ps.pmove.velocity[0] * 0.125f;
	ovel[1] = cl.oldframe.ps.pmove.velocity[1] * 0.125f;
	ovel[2] = cl.oldframe.ps.pmove.velocity[2] * 0.125f;
	LerpVector(ovel, vel, cl.lerpfrac, vel);
	static float curVel = 0;
	float vels = sqrtf(vel[0] * vel[0] + vel[1] * vel[1]);
	MoveTowards(&curVel, &vels, 750 * cls.frametime);
	color_t color = { colorTable[COLOR_WHITE] };

	if (cl_entities[cl.frame.clientNum + 1].current.renderfx & RF_SPECTRE)
	{
		int i;

		for (i = 0; i < 3; ++i)
			color.u8[i] = (byte)(0.25f * 255);

		color.u8[3] = (byte)(0.25f * 255);
	}
	else
	{
		vec3_t shade_color;
		GL_LightPoint(cl.predicted_origin, shade_color);
		color.u8[0] = clamp(shade_color[0], 0.0f, 1.0f) * 255;
		color.u8[1] = clamp(shade_color[1], 0.0f, 1.0f) * 255;
		color.u8[2] = clamp(shade_color[2], 0.0f, 1.0f) * 255;
		color.u8[3] = 255;

		if (color.u8[0] < 0.1f && color.u8[1] < 0.1f && color.u8[2] < 0.1f)
			color.u8[0] = color.u8[1] = color.u8[2] = 0.1f;
	}

	if (cl_entities[cl.frame.clientNum + 1].current.renderfx & RF_FROZEN)
	{
		color.u8[0] /= 8;
		color.u8[1] /= 8;
	}

	gunindex_e gun;

	for (gun = GUN_MAIN; gun < MAX_PLAYER_GUNS; ++gun)
	{
		bool firing = cl.frame.ps.guns[gun].offset[1] <= 0;
		float ofs = LerpFloat(cl.oldframe.ps.guns[gun].offset[2], cl.frame.ps.guns[gun].offset[2], cl.lerpfrac);
		R_DrawWeaponSprite(cl.precache[(uint16_t)cl.frame.ps.guns[gun].index].model.handle, gun, curVel, cl.time, cls.frametime,
			cl.oldframe.ps.guns[gun].frame, cl.frame.ps.guns[gun].frame, cl.lerpfrac, scr.hud_width,
			scr.hud_height, firing, ofs, scr.hud_scale, color, !!(cl.refdef.rdflags & RDF_INVUL), r_config.height - scr_vrect.height);
	}

	R_SetClipRect(NULL);
}

static void SCR_Draw2D(void)
{
	R_SetScale(scr.hud_scale);
	scr.hud_height *= scr.hud_scale;
	scr.hud_width *= scr.hud_scale;

	if (CL_GetClientGame() == GAME_DOOM ||
		CL_GetClientGame() == GAME_DUKE)
		SCR_DrawWeapon();

	if (scr_draw2d->integer <= 0)
		return;     // turn off for screenshots

	if (cls.key_dest & KEY_MENU)
		return;

	// crosshair has its own color and alpha
	SCR_DrawCrosshair();
	// the rest of 2D elements share common alpha
	R_ClearColor();
	R_SetAlpha(Cvar_ClampValue(scr_alpha, 0, 1));
	SCR_DrawStats();
	SCR_DrawLayout();
	SCR_DrawInventory();
	SCR_DrawCenterString();
	SCR_DrawNet();
	SCR_DrawObjects();
	SCR_DrawChatHUD();
	SCR_DrawTurtle();
	SCR_DrawPause();
	// debug stats have no alpha
	R_ClearColor();
#ifdef _DEBUG
	SCR_DrawDebugStats();
	SCR_DrawDebugPmove();
#endif
	R_SetScale(1.0f);
}

static void SCR_DrawActive(void)
{
	// if full screen menu is up, do nothing at all
	if (!UI_IsTransparent())
		return;

	// draw black background if not active
	if (cls.state < ca_active)
	{
		R_DrawFill8(0, 0, r_config.width, r_config.height, 0);
		return;
	}

	if (cls.state == ca_cinematic)
	{
		R_DrawStretchPic(0, 0, r_config.width, r_config.height, cl.precache[0].image, CL_GetClientGame());
		return;
	}

	// start with full screen HUD
	scr.hud_height = r_config.height;
	scr.hud_width = r_config.width;
	SCR_CalcVrect();
	// clear any dirty part of the background
	SCR_TileClear();
	// draw 3D game view
	V_RenderView();
	// draw all 2D elements
	SCR_Draw2D();
}

//=======================================================

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen(void)
{
	static int recursive;

	if (!scr.initialized)
	{
		return;             // not initialized yet
	}

	// if the screen is disabled (loading plaque is up), do nothing at all
	if (cls.disable_screen)
	{
		unsigned delta = Sys_Milliseconds() - cls.disable_screen;

		if (delta < 120 * 1000)
			return;

		cls.disable_screen = 0;
		Com_Printf("Loading plaque timed out.\n");
	}

	if (recursive > 1)
		Com_Error(ERR_FATAL, "%s: recursively called", __func__);

	recursive++;
	R_BeginFrame();
	// do 3D refresh drawing
	SCR_DrawActive();
	// draw main menu
	UI_Draw(cls.realtime);
	// draw console
	Con_DrawConsole();
	// draw loading plaque
	SCR_DrawLoading();
	R_EndFrame();
	recursive--;
}

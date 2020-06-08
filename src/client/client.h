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

// client.h -- primary header for client

#include "shared/shared.h"
#include "shared/list.h"

#include "common/bsp.h"
#include "common/cmd.h"
#include "common/cmodel.h"
#include "common/common.h"
#include "common/cvar.h"
#include "common/field.h"
#include "common/files.h"
#include "common/pmove.h"
#include "common/math.h"
#include "common/msg.h"
#include "common/net/chan.h"
#include "common/net/net.h"
#include "common/prompt.h"
#include "common/protocol.h"
#include "common/sizebuf.h"
#include "common/zone.h"

#include "system/system.h"
#include "refresh/refresh.h"
#include "server/server.h"

#include "client/client.h"
#include "client/input.h"
#include "client/keys.h"
#include "client/sound/sound.h"
#include "client/ui.h"
#include "client/video.h"

#if USE_ZLIB
	#include <zlib.h>
#endif

//=============================================================================

typedef struct centity_s
{
	entity_state_t    current;
	entity_state_t    prev;            // will always be valid, but might just be a copy of current

	vec3_t          mins, maxs;

	int             serverframe;        // if not current, this ent isn't in the frame

	int             trailcount;         // for diminishing grenade trails
	vec3_t          lerp_origin;        // for trails (variable hz)
	int				last_trail;

	int             fly_stoptime;
} centity_t;

extern centity_t    cl_entities[MAX_EDICTS];

#define MAX_CLIENTWEAPONMODELS        20        // PGM -- upped from 16 to fit the chainfist vwep

typedef struct clientinfo_s
{
	char name[MAX_QPATH];
	qhandle_t skin;
	qhandle_t icon;
	char model_name[MAX_QPATH];
	char skin_name[MAX_QPATH];
	qhandle_t model;
	qhandle_t weaponmodel[MAX_CLIENTWEAPONMODELS];
} clientinfo_t;

typedef struct
{
	unsigned    sent;    // time sent, for calculating pings
	unsigned    rcvd;    // time rcvd, for calculating pings
	unsigned    cmdNumber;    // current cmdNumber for this frame
} client_history_t;

typedef struct
{
	bool            valid;

	int             number;
	int             delta;

	byte            areabits[MAX_MAP_AREA_BYTES];
	int             areabytes;

	player_state_t  ps;
	int             clientNum;

	int             numEntities;
	int             firstEntity;
} server_frame_t;

// locally calculated frame flags for debug display
#define FF_SERVERDROP   (1<<4)
#define FF_BADFRAME     (1<<5)
#define FF_OLDFRAME     (1<<6)
#define FF_OLDENT       (1<<7)
#define FF_NODELTA      (1<<8)

#define CL_FRAMETIME    BASE_FRAMETIME
#define CL_1_FRAMETIME  BASE_1_FRAMETIME
#define CL_FRAMEDIV     1
#define CL_FRAMESYNC    1
#define CL_KEYPS        &cl.frame.ps
#define CL_OLDKEYPS     &cl.oldframe.ps
#define CL_KEYLERPFRAC  cl.lerpfrac

typedef struct layout_string_s layout_string_t;

typedef struct
{
	precache_type_e		type;

	union
	{
		struct
		{
			qhandle_t	handle;
			mmodel_t	*clip;
		} model;

		qhandle_t sound;
		qhandle_t image;
	};
} precache_entry_t;

typedef struct
{
	size_t				num_seeds;
	precache_entry_t	precache[ITI_WEAPONS_END - ITI_WEAPONS_START + 1];
} weapon_seeds_t;

//
// the client_state_t structure is wiped completely at every
// server map change
//
typedef struct client_state_s
{
	int         timeoutcount;

	unsigned    lastTransmitTime;
	unsigned    lastTransmitCmdNumber;
	unsigned    lastTransmitCmdNumberReal;
	bool        sendPacketNow;

	usercmd_t    cmd;
	usercmd_t    cmds[CMD_BACKUP];    // each mesage will send several old cmds
	unsigned     cmdNumber;
	short        predicted_origins[CMD_BACKUP][3];    // for debug comparing against server
	client_history_t    history[CMD_BACKUP];
	int         initialSeq;

	float       predicted_step;                // for stair up smoothing
	unsigned    predicted_step_time;
	unsigned    predicted_step_frame;

	vec3_t      predicted_origin;    // generated by CL_PredictMovement
	vec3_t      predicted_angles;
	vec3_t      predicted_velocity;
	vec3_t      prediction_error;

	// rebuilt each valid frame
	centity_t       *solidEntities[MAX_PACKET_ENTITIES];
	int             numSolidEntities;

	entity_state_t  baselines[MAX_EDICTS];

	entity_state_t  entityStates[MAX_PARSE_ENTITIES];
	int             numEntityStates;

	msgEsFlags_t    esFlags;

	server_frame_t  frames[UPDATE_BACKUP];
	unsigned        frameflags;

	server_frame_t  frame;                // received from server
	server_frame_t  oldframe;
	int             servertime;
	int             serverdelta;

	byte            dcs[CS_BITMAP_BYTES];

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t      viewangles;

	// interpolated movement vector used for local prediction,
	// never sent to server, rebuilt each client frame
	vec3_t      localmove;

	// accumulated mouse forward/side movement, added to both
	// localmove and pending cmd, cleared each time cmd is finalized
	vec2_t      mousemove;

#if USE_SMOOTH_DELTA_ANGLES
	short       delta_angles[3]; // interpolated
#endif

	int         time;           // this is the time value that the client
	// is rendering at.  always <= cl.servertime
	float       lerpfrac;       // between oldframe and frame

	refdef_t    refdef;
	float       fov_x;      // interpolated
	float       fov_y;      // derived from fov_x assuming 4/3 aspect ratio

	vec3_t      v_forward, v_right, v_up;    // set when refdef.angles is set

	bool        thirdPersonView;

	// predicted values, used for smooth player entity movement in thirdperson view
	vec3_t      playerEntityOrigin;
	vec3_t      playerEntityAngles;

	//
	// transient data from server
	//
	layout_string_t *layout;     // general 2D overlay
	char			layout_raw[2048];
	int				inventory[ITI_TOTAL];

	//
	// server state information
	//
	int         serverstate;    // ss_* constants
	int         servercount;    // server identification for prespawns
	char        gamedir[MAX_QPATH];
	int         clientNum;            // never changed during gameplay, set by serverdata packet
	int         maxclients;
	pmoveParams_t pmp;

	char        baseconfigstrings[MAX_CONFIGSTRINGS][CS_SIZE];
	char        configstrings[MAX_CONFIGSTRINGS][CS_SIZE];
	byte		precache_bitset[MAX_PRECACHE_BITSET];
	char        mapname[MAX_QPATH]; // short format - q2dm1, etc

	//
	// locally derived information from server state
	//
	bsp_t        *bsp;

	precache_entry_t	precache[MAX_PRECACHE];
	weapon_seeds_t		weapon_seeds[GAME_TOTAL][WEAPON_GROUP_MAX];
	weapon_seeds_t		ammo_seeds[GAME_TOTAL][WEAPON_GROUP_MAX];

	clientinfo_t    clientinfo[MAX_CLIENTS];
	clientinfo_t    baseclientinfo;

	char    weaponModels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
	int     numWeaponModels;

#ifdef ENABLE_COOP
	// Generations
	int		gamemode;
#endif
} client_state_t;

extern    client_state_t    cl;

/*
==================================================================

the client_static_t structure is persistant through an arbitrary number
of server connections

==================================================================
*/

// resend delay for challenge/connect packets
#define CONNECT_DELAY       3000u

#define CONNECT_INSTANT     CONNECT_DELAY
#define CONNECT_FAST        (CONNECT_DELAY - 1000u)

typedef enum
{
	ca_uninitialized,
	ca_disconnected,    // not talking to a server
	ca_challenging,     // sending getchallenge packets to the server
	ca_connecting,      // sending connect packets to the server
	ca_connected,       // netchan_t established, waiting for svc_serverdata
	ca_loading,         // loading level data
	ca_precached,       // loaded level data, waiting for svc_frame
	ca_active,          // game views should be displayed
	ca_cinematic        // running a cinematic
} connstate_t;

typedef struct client_static_s
{
	connstate_t state;
	keydest_t   key_dest;

	active_t    active;

	bool        ref_initialized;
	unsigned    disable_screen;

	int         userinfo_modified;
	cvar_t      *userinfo_updates[MAX_PACKET_USERINFOS];
	// this is set each time a CVAR_USERINFO variable is changed
	// so that the client knows to send it to the server

	int         framecount;
	unsigned    realtime;           // always increasing, no clamping, etc
	float       frametime;          // seconds since last frame

	// preformance measurement
#define C_FPS   cls.measure.fps[0]
#define R_FPS   cls.measure.fps[1]
#define C_MPS   cls.measure.fps[2]
#define C_PPS   cls.measure.fps[3]
#define C_FRAMES    cls.measure.frames[0]
#define R_FRAMES    cls.measure.frames[1]
#define M_FRAMES    cls.measure.frames[2]
#define P_FRAMES    cls.measure.frames[3]
	struct
	{
		unsigned    time;
		int         frames[4];
		int         fps[4];
		int         ping;
	} measure;

	// connection information
	netadr_t    serverAddress;
	char        servername[MAX_OSPATH]; // name of server from original connect
	unsigned    connect_time;           // for connection retransmits
	int         connect_count;
	bool        passive;

#if USE_ZLIB
	z_stream    z;
#endif

	int         quakePort;          // a 16 bit value that allows quake servers
	// to work around address translating routers
	netchan_t   *netchan;
	int         serverProtocol;     // in case we are doing some kind of version hack

	int         challenge;          // from the server to use for connecting

#if USE_ICMP
	bool        errorReceived;      // got an ICMP error from server
#endif

#define RECENT_ADDR 4
#define RECENT_MASK (RECENT_ADDR - 1)

	netadr_t    recent_addr[RECENT_ADDR];
	int         recent_head;
} client_static_t;

extern client_static_t    cls;

extern cmdbuf_t    cl_cmdbuf;
extern char        cl_cmdbuf_text[MAX_STRING_CHARS];

//=============================================================================

#define NOPART_GRENADE_EXPLOSION    1
#define NOPART_GRENADE_TRAIL        2
#define NOPART_ROCKET_EXPLOSION     4
#define NOPART_ROCKET_TRAIL         8
#define NOPART_BLOOD                16

#define NOEXP_GRENADE   1
#define NOEXP_ROCKET    2

//
// cvars
//
extern cvar_t    *cl_gun;
extern cvar_t    *cl_gunalpha;
extern cvar_t    *cl_predict;
extern cvar_t    *cl_footsteps;
extern cvar_t    *cl_noskins;
extern cvar_t    *cl_kickangles;
extern cvar_t    *cl_rollhack;
extern cvar_t    *cl_noglow;
extern cvar_t    *cl_nolerp;

#ifdef _DEBUG
#define SHOWNET(level, ...) \
	if (cl_shownet->integer > level) \
		Com_LPrintf(PRINT_DEVELOPER, __VA_ARGS__)
#define SHOWCLAMP(level, ...) \
	if (cl_showclamp->integer > level) \
		Com_LPrintf(PRINT_DEVELOPER, __VA_ARGS__)
#define SHOWMISS(...) \
	if (cl_showmiss->integer) \
		Com_LPrintf(PRINT_DEVELOPER, __VA_ARGS__)
extern cvar_t    *cl_shownet;
extern cvar_t    *cl_showmiss;
extern cvar_t    *cl_showclamp;
#else
#define SHOWNET(...)
#define SHOWCLAMP(...)
#define SHOWMISS(...)
#endif

extern cvar_t    *cl_vwep;

extern cvar_t    *cl_disable_particles;
extern cvar_t    *cl_disable_explosions;

extern cvar_t    *cl_chat_notify;
extern cvar_t    *cl_chat_sound;
extern cvar_t    *cl_chat_filter;

extern cvar_t    *cl_disconnectcmd;
extern cvar_t    *cl_changemapcmd;
extern cvar_t    *cl_beginmapcmd;

extern cvar_t    *cl_gibs;

extern cvar_t    *cl_thirdperson;
extern cvar_t    *cl_thirdperson_angle;
extern cvar_t    *cl_thirdperson_range;

extern cvar_t    *cl_async;

//
// userinfo
//
extern cvar_t    *info_password;
extern cvar_t    *info_spectator;
extern cvar_t    *info_name;
extern cvar_t    *info_skin;
extern cvar_t    *info_rate;
extern cvar_t    *info_fov;
extern cvar_t    *info_msg;
extern cvar_t    *info_hand;
extern cvar_t    *info_gender;

//=============================================================================

//
// main.c
//

void CL_Init(void);
void CL_Quit_f(void);
void CL_Disconnect(error_type_t type);
void CL_Begin(void);
void CL_CheckForResend(void);
void CL_ClearState(void);
void CL_RestartFilesystem(bool total);
void CL_RestartRefresh(bool total);
void CL_ClientCommand(const char *string);
void CL_SendRcon(const netadr_t *adr, const char *pass, const char *cmd);
const char *CL_Server_g(const char *partial, int argnum, int state);
void CL_CheckForPause(void);
void CL_UpdateFrameTimes(void);
bool CL_CheckForIgnore(const char *s);
gametype_t CL_GetClientGame();

void cl_timeout_changed(cvar_t *self);

//
// precache.c
//

typedef enum
{
	LOAD_NONE,
	LOAD_MAP,
	LOAD_MODELS,
	LOAD_IMAGES,
	LOAD_CLIENTS,
	LOAD_SOUNDS
} load_state_t;

void CL_ParsePlayerSkin(char *name, char *model, char *skin, const char *s);
void CL_LoadClientinfo(clientinfo_t *ci, const char *s);
void CL_LoadState(load_state_t state);
void CL_RegisterSounds(void);
void CL_RegisterBspModels(void);
void CL_RegisterVWepModels(void);
void CL_PrepRefresh(void);
void CL_UpdateConfigstring(int index);


//
// input.c
//
void IN_Init(void);
void IN_Shutdown(void);
void IN_Frame(void);
void IN_Activate(void);

void CL_RegisterInput(void);
void CL_UpdateCmd(int msec);
void CL_FinalizeCmd(void);
void CL_SendCmd(void);


//
// parse.c
//

typedef struct
{
	int type;
	vec3_t pos1;
	vec3_t pos2;
	vec3_t offset;
	vec3_t dir;
	int count;
	int color;
	int entity1;
	int entity2;
	int time;
} tent_params_t;

typedef struct
{
	int entity;
	int weapon;
	int silenced;
} mz_params_t;

typedef struct
{
	int     flags;
	int     index;
	int     entity;
	int     channel;
	vec3_t  pos;
	float   volume;
	float   attenuation;
	float   timeofs;
} snd_params_t;

extern tent_params_t    te;
extern mz_params_t      mz;
extern snd_params_t     snd;

void CL_ParseServerMessage(void);


//
// entities.c
//
void CL_DeltaFrame(void);
void CL_AddEntities(void);
void CL_CalcViewValues(void);

#ifdef _DEBUG
	void CL_CheckEntityPresent(int entnum, const char *what);
#endif

// the sound code makes callbacks to the client for entitiy position
// information, so entities can be dynamically re-spatialized
void CL_GetEntitySoundOrigin(int ent, vec3_t org);


//
// view.c
//
extern    int       	gun_frame;
extern    qhandle_t		gun_model;

void V_Init(void);
void V_Shutdown(void);
void V_RenderView(void);
void V_AddEntity(entity_t *ent);
void V_AddParticle(particle_t *p);
void V_AddLight(vec3_t org, float intensity, float r, float g, float b);
void V_AddLightStyle(int style, vec4_t value);
void CL_UpdateBlendSetting(void);


//
// tent.c
//

typedef struct cl_sustain_s
{
	int     id;
	int     type;
	int     endtime;
	int     nextthink;
	int     thinkinterval;
	vec3_t  org;
	vec3_t  dir;
	int     color;
	int     count;
	int     magnitude;
	void (*think)(struct cl_sustain_s *self);
} cl_sustain_t;

void CL_SmokeAndFlash(vec3_t origin);

void CL_RegisterTEntSounds(void);
void CL_RegisterTEntModels(void);
void CL_RegisterEffectSounds(void);
void CL_ParseTEnt(void);
void CL_AddTEnts(void);
void CL_ClearTEnts(void);
void CL_InitTEnts(void);

void CL_AddNavLaser(vec3_t first, vec3_t second, int width, color_t color);


//
// predict.c
//
void CL_PredictAngles(void);
void CL_PredictMovement(void);
void CL_CheckPredictionError(void);


//
// effects.c
//
#define PARTICLE_GRAVITY        40
#define BLASTER_PARTICLE_COLOR  0xe0
#define INSTANT_PARTICLE    -10000.0f

typedef enum
{
	PARTICLE_NOPE,

	PARTICLE_Q1_EXPLODE,
	PARTICLE_Q1_EXPLODE2,
	PARTICLE_Q1_SLOWGRAV,
	PARTICLE_Q1_FIRE,
	PARTICLE_Q1_STATIC,
	PARTICLE_Q1_BLOB1,
	PARTICLE_Q1_BLOB2
} particletype_t;

typedef struct cparticle_s
{
	struct cparticle_s    *next;

	float   time;

	vec3_t  org;
	vec3_t  vel;
	vec3_t  accel;
	int     color;      // -1 => use rgba
	float   alpha;
	float   alphavel;
	color_t rgba;

	// Generations
	float			die;
	particletype_t	type;
	float			ramp;
} cparticle_t;

typedef struct cdlight_s
{
	int     key;        // so entities can reuse same entry
	vec3_t  color;
	vec3_t  origin;
	float   radius;
	float   die;        // stop lighting after this time
} cdlight_t;

void CL_BigTeleportParticles(vec3_t org);
void CL_RocketTrail(vec3_t start, vec3_t end, centity_t *old);
void CL_DiminishingTrail(vec3_t start, vec3_t end, centity_t *old, int flags);
void CL_FlyEffect(centity_t *ent, vec3_t origin);
void CL_BfgParticles(entity_t *ent);
void CL_ItemRespawnParticles(vec3_t org);
void CL_InitEffects(void);
void CL_ClearEffects(void);
void CL_BlasterParticles(vec3_t org, vec3_t dir);
void CL_ExplosionParticles(vec3_t org);
void CL_BFGExplosionParticles(vec3_t org);
void CL_BlasterTrail(vec3_t start, vec3_t end);
void CL_OldRailTrail(void);
void CL_BubbleTrail(vec3_t start, vec3_t end);
void CL_FlagTrail(vec3_t start, vec3_t end, int color);
void CL_MuzzleFlash(void);
void CL_MuzzleFlash2(void);
void CL_TeleporterParticles(vec3_t org);
void CL_TeleportParticles(vec3_t org);
void CL_ParticleEffect(vec3_t org, vec3_t dir, int color, int count);
void CL_ParticleEffect2(vec3_t org, vec3_t dir, int color, int count);
cparticle_t *CL_AllocParticle(void);
void CL_AddParticles(void);
cdlight_t *CL_AllocDlight(int key);
void CL_RunDLights(void);
void CL_AddDLights(void);
void CL_ClearLightStyles(void);
void CL_SetLightStyle(int index, const char *s);
void CL_RunLightStyles(void);
void CL_AddLightStyles(void);

//
// newfx.c
//

void CL_BlasterParticles2(vec3_t org, vec3_t dir, unsigned int color);
void CL_BlasterTrail2(vec3_t start, vec3_t end);
void CL_DebugTrail(vec3_t start, vec3_t end);
void CL_Flashlight(int ent, vec3_t pos);
void CL_ForceWall(vec3_t start, vec3_t end, int color);
void CL_BubbleTrail2(vec3_t start, vec3_t end, int dist);
void CL_Heatbeam(vec3_t start, vec3_t end);
void CL_ParticleSteamEffect(vec3_t org, vec3_t dir, int color, int count, int magnitude);
void CL_TrackerTrail(vec3_t start, vec3_t end, int particleColor);
void CL_TagTrail(vec3_t start, vec3_t end, int color);
void CL_ColorFlash(vec3_t pos, int ent, int intensity, float r, float g, float b);
void CL_Tracker_Shell(vec3_t origin);
void CL_MonsterPlasma_Shell(vec3_t origin);
void CL_ColorExplosionParticles(vec3_t org, int color, int run);
void CL_ParticleSmokeEffect(vec3_t org, vec3_t dir, int color, int count, int magnitude);
void CL_Widowbeamout(cl_sustain_t *self);
void CL_Nukeblast(cl_sustain_t *self);
void CL_WidowSplash(void);
void CL_IonripperTrail(vec3_t start, vec3_t end);
void CL_TrapParticles(centity_t *ent, vec3_t origin);
void CL_ParticleEffect3(vec3_t org, vec3_t dir, int color, int count);
void CL_ParticleSteamEffect2(cl_sustain_t *self);


//
// locs.c
//
void LOC_Init(void);
void LOC_LoadLocations(void);
void LOC_FreeLocations(void);
void LOC_UpdateCvars(void);
void LOC_AddLocationsToScene(void);


//
// console.c
//
void Con_Init(void);
void Con_PostInit(void);
void Con_Shutdown(void);
void Con_DrawConsole(void);
void Con_RunConsole(void);
void Con_Print(const char *txt);
void Con_ClearNotify_f(void);
void Con_ToggleConsole_f(void);
void Con_ClearTyping(void);
void Con_Close(bool force);
void Con_Popup(bool force);
void Con_SkipNotify(bool skip);
void Con_RegisterMedia(void);
void Con_CheckResize(void);

void Key_Console(int key);
void Key_Message(int key);
void Char_Console(int key);
void Char_Message(int key);


//
// refresh.c
//
void    CL_InitRefresh(void);
void    CL_ShutdownRefresh(void);
void    CL_RunRefresh(void);


//
// screen.c
//
extern vrect_t      scr_vrect;        // position of render window

void    SCR_Init(void);
void    SCR_Shutdown(void);
void    SCR_UpdateScreen(void);
void    SCR_SizeUp(void);
void    SCR_SizeDown(void);
void    SCR_CenterPrint(const char *str);
void    SCR_FinishCinematic(void);
void    SCR_PlayCinematic(const char *name);
void    SCR_BeginLoadingPlaque(void);
void    SCR_EndLoadingPlaque(void);
void    SCR_TouchPics(void);
void    SCR_RegisterMedia(void);
void    SCR_ModeChanged(void);
void    SCR_LagSample(void);
void    SCR_LagClear(void);
void    SCR_SetCrosshairColor(void);

float   SCR_FadeAlpha(unsigned startTime, unsigned visTime, unsigned fadeTime);
int     SCR_DrawStringEx(int x, int y, int flags, size_t maxlen, const char *s, qhandle_t font, gametype_t game);
void    SCR_DrawStringMulti(int x, int y, int flags, size_t maxlen, const char *s, qhandle_t font, gametype_t game);

void    SCR_ClearChatHUD_f(void);
void    SCR_AddToChatHUD(const char *text);


//
// crc.c
//
byte COM_BlockSequenceCRCByte(byte *base, size_t length, int sequence);

// Nav stuff
void Nav_AddEntities(void);
void Nav_Frame(void);
void Nav_Init(void);
void Nav_Shutdown(void);
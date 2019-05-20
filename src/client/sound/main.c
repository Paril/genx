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
// snd_main.c -- common sound functions

#include "sound.h"

// =======================================================================
// Internal sound data & structures
// =======================================================================

int         s_registration_sequence;

channel_t   channels[MAX_CHANNELS];
int         s_numchannels;

sndstarted_t s_started;
bool        s_active;

vec3_t      listener_origin;
vec3_t      listener_forward;
vec3_t      listener_right;
vec3_t      listener_up;
int         listener_entnum;

bool        s_registering;

int         paintedtime;    // sample PAIRS

// during registration it is possible to have more sounds
// than could actually be referenced during gameplay,
// because we don't want to free anything until we are
// sure we won't need it.
#define     MAX_SFX     (MAX_SOUNDS*2)
static sfx_t        known_sfx[MAX_SFX];
static int          num_sfx;

#define     MAX_PLAYSOUNDS  128
playsound_t s_playsounds[MAX_PLAYSOUNDS];
playsound_t s_freeplays;
playsound_t s_pendingplays;

cvar_t      *s_volume;
cvar_t      *s_ambient;
#ifdef _DEBUG
cvar_t      *s_show;
#endif

static cvar_t   *s_enable;
static cvar_t   *s_auto_focus;
static cvar_t   *s_swapstereo;

// =======================================================================
// Console functions
// =======================================================================

static void S_SoundInfo_f(void)
{
    if (!s_started) {
        Com_Printf("Sound system not started.\n");
        return;
    }

    if (s_started == SS_OAL)
        AL_SoundInfo();
}

static void S_SoundList_f(void)
{
    int     i;
    sfx_t   *sfx;
    sfxcache_t  *sc;
    int     size, total;

    total = 0;
    for (sfx = known_sfx, i = 0; i < num_sfx; i++, sfx++) {
        if (!sfx->name[0])
            continue;
        sc = sfx->cache;
        if (sc) {
            if (s_started == SS_OAL)
                size = sc->size;
            else
                size = sc->length * sc->width;
            total += size;
            if (sc->loopstart >= 0)
                Com_Printf("L");
            else
                Com_Printf(" ");
            Com_Printf("(%2db) %6i : %s\n", sc->width * 8,  size, sfx->name) ;
        } else {
            if (sfx->name[0] == '*')
                Com_Printf("  placeholder : %s\n", sfx->name);
            else
                Com_Printf("  not loaded  : %s (%s)\n",
                           sfx->name, Q_ErrorString(sfx->error));
        }
    }
    Com_Printf("Total resident: %i\n", total);
}

static const cmdreg_t c_sound[] = {
    { "stopsound", S_StopAllSounds },
    { "soundlist", S_SoundList_f },
    { "soundinfo", S_SoundInfo_f },

    { NULL }
};

// =======================================================================
// Init sound engine
// =======================================================================

static void s_auto_focus_changed(cvar_t *self)
{
    S_Activate();
}

/*
================
S_Init
================
*/
void S_Init(void)
{
    s_enable = Cvar_Get("s_enable", "1", CVAR_SOUND);
    if (s_enable->integer <= SS_NOT) {
        Com_Printf("Sound initialization disabled.\n");
        return;
    }

    Com_Printf("------- S_Init -------\n");

    s_volume = Cvar_Get("s_volume", "0.7", CVAR_ARCHIVE);
    s_ambient = Cvar_Get("s_ambient", "1", 0);
#ifdef _DEBUG
    s_show = Cvar_Get("s_show", "0", 0);
#endif
    s_auto_focus = Cvar_Get("s_auto_focus", "0", 0);
    s_swapstereo = Cvar_Get("s_swapstereo", "0", 0);

    // start one of available sound engines
    s_started = SS_NOT;

    if (s_started == SS_NOT && s_enable->integer >= SS_OAL && AL_Init())
        s_started = SS_OAL;

    if (s_started == SS_NOT) {
        Com_EPrintf("Sound failed to initialize.\n");
        goto fail;
    }

    Cmd_Register(c_sound);

    // init playsound list
    // clear DMA buffer
    S_StopAllSounds();

    s_auto_focus->changed = s_auto_focus_changed;
    s_auto_focus_changed(s_auto_focus);

    num_sfx = 0;

    paintedtime = 0;

    s_registration_sequence = 1;

fail:
    Cvar_SetInteger(s_enable, s_started, FROM_CODE);
    Com_Printf("----------------------\n");
}


// =======================================================================
// Shutdown sound engine
// =======================================================================

static void S_FreeSound(sfx_t *sfx)
{
    if (s_started == SS_OAL)
        AL_DeleteSfx(sfx);
    if (sfx->cache)
        Z_Free(sfx->cache);
    if (sfx->truename)
        Z_Free(sfx->truename);
    memset(sfx, 0, sizeof(*sfx));
}

void S_FreeAllSounds(void)
{
    int     i;
    sfx_t   *sfx;

    // free all sounds
    for (i = 0, sfx = known_sfx; i < num_sfx; i++, sfx++) {
        if (!sfx->name[0])
            continue;
        S_FreeSound(sfx);
    }

    num_sfx = 0;
}

void S_Shutdown(void)
{
    if (!s_started)
        return;

    S_StopAllSounds();
    S_FreeAllSounds();

    if (s_started == SS_OAL)
        AL_Shutdown();

    s_started = SS_NOT;
    s_active = false;

    s_auto_focus->changed = NULL;

    Cmd_Deregister(c_sound);

    Z_LeakTest(TAG_SOUND);
}

void S_Activate(void)
{
    bool active;
    active_t level;

    if (!s_started)
        return;

    level = Cvar_ClampInteger(s_auto_focus, ACT_MINIMIZED, ACT_ACTIVATED);

    active = cls.active >= level;

    if (active == s_active)
        return;

    s_active = active;

    Com_DDDPrintf("%s: %d\n", __func__, s_active);

    if (s_started == SS_OAL)
        S_StopAllSounds();
}


// =======================================================================
// Load a sound
// =======================================================================

/*
==================
S_SfxForHandle
==================
*/
sfx_t *S_SfxForHandle(qhandle_t hSfx)
{
    if (!hSfx) {
        return NULL;
    }

    if (hSfx < 1 || hSfx > num_sfx) {
        Com_Error(ERR_DROP, "S_SfxForHandle: %d out of range", hSfx);
    }

    return &known_sfx[hSfx - 1];
}

static sfx_t *S_AllocSfx(void)
{
    sfx_t   *sfx;
    int     i;

    // find a free sfx
    for (i = 0, sfx = known_sfx; i < num_sfx; i++, sfx++) {
        if (!sfx->name[0])
            break;
    }

    if (i == num_sfx) {
        if (num_sfx == MAX_SFX)
            return NULL;
        num_sfx++;
    }

    return sfx;
}

/*
==================
S_FindName

==================
*/
static sfx_t *S_FindName(const char *name, size_t namelen)
{
    int     i;
    sfx_t   *sfx;

    // see if already loaded
    for (i = 0, sfx = known_sfx; i < num_sfx; i++, sfx++) {
        if (!FS_pathcmp(sfx->name, name)) {
            sfx->registration_sequence = s_registration_sequence;
            return sfx;
        }
    }

    // allocate new one
    sfx = S_AllocSfx();
    if (sfx) {
        memcpy(sfx->name, name, namelen + 1);
        sfx->registration_sequence = s_registration_sequence;
    }
    return sfx;
}

/*
=====================
S_BeginRegistration

=====================
*/
void S_BeginRegistration(void)
{
    s_registration_sequence++;
    s_registering = true;
}

/*
==================
S_RegisterSound

==================
*/
qhandle_t S_RegisterSound(const char *name)
{
    char    buffer[MAX_QPATH];
    sfx_t   *sfx;
    size_t  len;

    if (!s_started)
        return 0;

    // empty names are legal, silently ignore them
    if (!*name)
        return 0;

    if (*name == '*') {
        len = Q_strlcpy(buffer, name, MAX_QPATH);
    } else if (*name == '#') {
        len = FS_NormalizePathBuffer(buffer, name + 1, MAX_QPATH);
    } else {
        len = Q_concat(buffer, MAX_QPATH, "sound/", name, NULL);
        if (len < MAX_QPATH)
            len = FS_NormalizePath(buffer, buffer);
    }

    // this MAY happen after prepending "sound/"
    if (len >= MAX_QPATH) {
        Com_DPrintf("%s: oversize name\n", __func__);
        return 0;
    }

    // normalized to empty name?
    if (len == 0) {
        Com_DPrintf("%s: empty name\n", __func__);
        return 0;
    }

    sfx = S_FindName(buffer, len);
    if (!sfx) {
        Com_DPrintf("%s: out of slots\n", __func__);
        return 0;
    }

    if (!s_registering) {
        S_LoadSound(sfx);
    }

    return (sfx - known_sfx) + 1;
}

/*
====================
S_RegisterSexedSound
====================
*/
static sfx_t *S_RegisterSexedSound(int entnum, const char *base)
{
    sfx_t           *sfx;
    char            *model;
    char            buffer[MAX_QPATH];
    size_t          len;

    // determine what model the client is using
    if (entnum > 0 && entnum <= MAX_CLIENTS)
        model = cl.clientinfo[entnum - 1].model_name;
    else
        model = cl.baseclientinfo.model_name;

    // if we can't figure it out, they're male
    if (!*model)
        model = "male";

    // see if we already know of the model specific sound
    len = Q_concat(buffer, MAX_QPATH,
                   "players/", model, "/", base + 1, NULL);
    if (len >= MAX_QPATH) {
        len = Q_concat(buffer, MAX_QPATH,
                       "players/", "male", "/", base + 1, NULL);
        if (len >= MAX_QPATH)
            return NULL;
    }

    len = FS_NormalizePath(buffer, buffer);
    sfx = S_FindName(buffer, len);

    // see if it exists
    if (sfx && !sfx->truename && !s_registering && !S_LoadSound(sfx)) {
        // no, revert to the male sound in the pak0.pak
        len = Q_concat(buffer, MAX_QPATH,
                       "sound/player/male/", base + 1, NULL);
        if (len < MAX_QPATH) {
            FS_NormalizePath(buffer, buffer);
            sfx->error = Q_ERR_SUCCESS;
            sfx->truename = S_CopyString(buffer);
        }
    }

    return sfx;
}

static void S_RegisterSexedSounds(void)
{
    int     sounds[MAX_SFX];
    int     i, j, total;
    sfx_t   *sfx;

    // find sexed sounds
    total = 0;
    for (i = 0, sfx = known_sfx; i < num_sfx; i++, sfx++) {
        if (sfx->name[0] != '*')
            continue;
        if (sfx->registration_sequence != s_registration_sequence)
            continue;
        sounds[total++] = i;
    }

    // register sounds for baseclientinfo and other valid clientinfos
    for (i = 0; i <= MAX_CLIENTS; i++) {
        if (i > 0 && !cl.clientinfo[i - 1].model_name[0])
            continue;
        for (j = 0; j < total; j++) {
            sfx = &known_sfx[sounds[j]];
            S_RegisterSexedSound(i, sfx->name);
        }
    }
}

/*
=====================
S_EndRegistration

=====================
*/
void S_EndRegistration(void)
{
    int     i;
    sfx_t   *sfx;

    S_RegisterSexedSounds();

    // clear playsound list, so we don't free sfx still present there
    S_StopAllSounds();

    // free any sounds not from this registration sequence
    for (i = 0, sfx = known_sfx; i < num_sfx; i++, sfx++) {
        if (!sfx->name[0])
            continue;
        if (sfx->registration_sequence != s_registration_sequence) {
            // don't need this sound
            S_FreeSound(sfx);
            continue;
        }
        if (s_started == SS_OAL)
            continue;
    }

    // load everything in
    for (i = 0, sfx = known_sfx; i < num_sfx; i++, sfx++) {
        if (!sfx->name[0])
            continue;
        S_LoadSound(sfx);
    }

    s_registering = false;
}


//=============================================================================

/*
=================
S_PickChannel

picks a channel based on priorities, empty slots, number of channels
=================
*/
channel_t *S_PickChannel(int entnum, int entchannel)
{
    int         ch_idx;
    int         first_to_die;
    int         life_left;
    channel_t   *ch;

    if (entchannel < 0)
        Com_Error(ERR_DROP, "S_PickChannel: entchannel < 0");

// Check for replacement sound, or find the best one to replace
    first_to_die = -1;
    life_left = 0x7fffffff;
    for (ch_idx = 0; ch_idx < s_numchannels; ch_idx++) {
        ch = &channels[ch_idx];
        // channel 0 never overrides unless out of channels
        if (ch->entnum == entnum && ch->entchannel == entchannel && entchannel != 0) {
            if (entchannel > 255 && ch->sfx) {
                return NULL; // channels >255 never override
            }
            // always override sound from same entity
            first_to_die = ch_idx;
            break;
        }

        // don't let monster sounds override player sounds
        if (ch->entnum == listener_entnum && entnum != listener_entnum && ch->sfx)
            continue;

        if (ch->end - paintedtime < life_left) {
            life_left = ch->end - paintedtime;
            first_to_die = ch_idx;
        }
    }

    if (first_to_die == -1)
        return NULL;

    ch = &channels[first_to_die];
    if (s_started == SS_OAL && ch->sfx)
        AL_StopChannel(ch);
    memset(ch, 0, sizeof(*ch));

    return ch;
}

/*
=================
S_AllocPlaysound
=================
*/
static playsound_t *S_AllocPlaysound(void)
{
    playsound_t *ps;

    ps = s_freeplays.next;
    if (ps == &s_freeplays)
        return NULL;        // no free playsounds

    // unlink from freelist
    ps->prev->next = ps->next;
    ps->next->prev = ps->prev;

    return ps;
}


/*
=================
S_FreePlaysound
=================
*/
static void S_FreePlaysound(playsound_t *ps)
{
    // unlink from channel
    ps->prev->next = ps->next;
    ps->next->prev = ps->prev;

    // add to free list
    ps->next = s_freeplays.next;
    s_freeplays.next->prev = ps;
    ps->prev = &s_freeplays;
    s_freeplays.next = ps;
}

/*
===============
S_IssuePlaysound

Take the next playsound and begin it on the channel
This is never called directly by S_Play*, but only
by the update loop.
===============
*/
void S_IssuePlaysound(playsound_t *ps)
{
    channel_t   *ch;
    sfxcache_t  *sc;

#ifdef _DEBUG
    if (s_show->integer)
        Com_Printf("Issue %i\n", ps->begin);
#endif
    // pick a channel to play on
    ch = S_PickChannel(ps->entnum, ps->entchannel);
    if (!ch) {
        S_FreePlaysound(ps);
        return;
    }

    sc = S_LoadSound(ps->sfx);
    if (!sc) {
        Com_Printf("S_IssuePlaysound: couldn't load %s\n", ps->sfx->name);
        S_FreePlaysound(ps);
        return;
    }

    // spatialize
    if (ps->attenuation == ATTN_STATIC)
        ch->dist_mult = ps->attenuation * 0.001f;
    else
        ch->dist_mult = ps->attenuation * 0.0005f;
    ch->master_vol = ps->volume;
    ch->entnum = ps->entnum;
    ch->entchannel = ps->entchannel;
    ch->sfx = ps->sfx;
    VectorCopy(ps->origin, ch->origin);
    ch->fixed_origin = ps->fixed_origin;

    if (s_started == SS_OAL)
        AL_PlayChannel(ch);

    ch->pos = 0;
    ch->end = paintedtime + sc->length;

    // free the playsound
    S_FreePlaysound(ps);
}

// =======================================================================
// Start a sound effect
// =======================================================================

/*
====================
S_StartSound

Validates the parms and ques the sound up
if pos is NULL, the sound will be dynamically sourced from the entity
Entchannel 0 will never override a playing sound
====================
*/
void S_StartSound(const vec3_t origin, int entnum, int entchannel, qhandle_t hSfx, float vol, float attenuation, float timeofs)
{
    sfxcache_t  *sc;
    playsound_t *ps, *sort;
    sfx_t       *sfx;

    if (!s_started)
        return;
    if (!s_active)
        return;

    if (!(sfx = S_SfxForHandle(hSfx))) {
        return;
    }

    if (sfx->name[0] == '*') {
        sfx = S_RegisterSexedSound(entnum, sfx->name);
        if (!sfx)
            return;
    }

    // make sure the sound is loaded
    sc = S_LoadSound(sfx);
    if (!sc)
        return;     // couldn't load the sound's data

    // make the playsound_t
    ps = S_AllocPlaysound();
    if (!ps)
        return;

    if (origin) {
        VectorCopy(origin, ps->origin);
        ps->fixed_origin = true;
    } else {
        ps->fixed_origin = false;
    }

    ps->entnum = entnum;
    ps->entchannel = entchannel;
    ps->attenuation = attenuation;
    ps->volume = vol;
    ps->sfx = sfx;

    if (s_started == SS_OAL)
        ps->begin = paintedtime + timeofs * 1000;

    // sort into the pending sound list
    for (sort = s_pendingplays.next; sort != &s_pendingplays && sort->begin < ps->begin; sort = sort->next)
        ;

    ps->next = sort;
    ps->prev = sort->prev;

    ps->next->prev = ps;
    ps->prev->next = ps;
}

void S_ParseStartSound(void)
{
    qhandle_t handle = cl.sound_precache[snd.index];

    if (!handle)
        return;

#ifdef _DEBUG
    if (developer->integer && !(snd.flags & SND_POS))
        CL_CheckEntityPresent(snd.entity, "sound");
#endif

    S_StartSound((snd.flags & SND_POS) ? snd.pos : NULL,
                 snd.entity, snd.channel, handle,
                 snd.volume, snd.attenuation, snd.timeofs);
}

/*
==================
S_StartLocalSound
==================
*/
void S_StartLocalSound(const char *sound)
{
    if (s_started) {
        qhandle_t sfx = S_RegisterSound(sound);
        S_StartSound(NULL, listener_entnum, 0, sfx, 1, ATTN_NONE, 0);
    }
}

void S_StartLocalSound_(const char *sound)
{
    if (s_started) {
        qhandle_t sfx = S_RegisterSound(sound);
        S_StartSound(NULL, listener_entnum, 256, sfx, 1, ATTN_NONE, 0);
    }
}

/*
==================
S_StopAllSounds
==================
*/
void S_StopAllSounds(void)
{
    int     i;

    if (!s_started)
        return;

    // clear all the playsounds
    memset(s_playsounds, 0, sizeof(s_playsounds));
    s_freeplays.next = s_freeplays.prev = &s_freeplays;
    s_pendingplays.next = s_pendingplays.prev = &s_pendingplays;

    for (i = 0; i < MAX_PLAYSOUNDS; i++) {
        s_playsounds[i].prev = &s_freeplays;
        s_playsounds[i].next = s_freeplays.next;
        s_playsounds[i].prev->next = &s_playsounds[i];
        s_playsounds[i].next->prev = &s_playsounds[i];
    }

    if (s_started == SS_OAL)
        AL_StopAllChannels();

    // clear all the channels
    memset(channels, 0, sizeof(channels));
}

// =======================================================================
// Update sound buffer
// =======================================================================

void S_BuildSoundList(int *sounds)
{
    int         i;
    int         num;
    entity_state_t  *ent;

    for (i = 0; i < cl.frame.numEntities; i++) {
        num = (cl.frame.firstEntity + i) & PARSE_ENTITIES_MASK;
        ent = &cl.entityStates[num];
        if (s_ambient->integer == 2 && !ent->modelindex) {
            sounds[i] = 0;
        } else if (s_ambient->integer == 3 && ent->number != listener_entnum) {
            sounds[i] = 0;
        } else {
            sounds[i] = ent->sound;
        }
    }
}

/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update(void)
{
    if (cvar_modified & CVAR_SOUND) {
        Cbuf_AddText(&cmd_buffer, "snd_restart\n");
        cvar_modified &= ~CVAR_SOUND;
        return;
    }

    if (!s_started)
        return;

    // if the laoding plaque is up, clear everything
    // out to make sure we aren't looping a dirty
    // dma buffer while loading
    if (cls.state == ca_loading) {
        // S_ClearBuffer should be already done in S_StopAllSounds
        return;
    }

    // set listener entity number
    // other parameters should be already set up by CL_CalcViewValues
    if (cl.clientNum == -1 || cl.frame.clientNum == CLIENTNUM_NONE) {
        listener_entnum = -1;
    } else {
        listener_entnum = cl.frame.clientNum + 1;
    }

    if (s_started == SS_OAL) {
        AL_Update();
        return;
    }

#ifdef _DEBUG
    //
    // debugging output
    //
    if (s_show->integer) {
        int total = 0, i;
        channel_t *ch = channels;
        for (i = 0; i < s_numchannels; i++, ch++)
            if (ch->sfx && ch->master_vol) {
                Com_Printf("%f %s\n", ch->master_vol, ch->sfx->name);
                total++;
            }

        if (s_show->integer > 1 || total) {
            Com_Printf("----(%i)---- painted: %i\n", total, paintedtime);
        }
    }
#endif
}

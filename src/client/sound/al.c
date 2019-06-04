/*
Copyright (C) 2010 Andrey Nazarov

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

#include "sound.h"
#include "qal/dynamic.h"

// translates from AL coordinate system to quake
#define AL_UnpackVector(v)  -v[1],v[2],-v[0]
#define AL_CopyVector(a,b)  ((b)[0]=-(a)[1],(b)[1]=(a)[2],(b)[2]=-(a)[0])

// OpenAL implementation should support at least this number of sources
#define MIN_CHANNELS 16

static ALuint s_srcnums[MAX_CHANNELS];
static int s_framecount;

void AL_SoundInfo(void)
{
	Com_Printf("AL_VENDOR: %s\n", qalGetString(AL_VENDOR));
	Com_Printf("AL_RENDERER: %s\n", qalGetString(AL_RENDERER));
	Com_Printf("AL_VERSION: %s\n", qalGetString(AL_VERSION));
	Com_Printf("AL_EXTENSIONS: %s\n", qalGetString(AL_EXTENSIONS));
	Com_Printf("Number of sources: %d\n", s_numchannels);
}

bool AL_Init(void)
{
	int i;
	Com_DPrintf("Initializing OpenAL\n");

	if (!QAL_Init())
		goto fail0;

	// check for linear distance extension
	if (!qalIsExtensionPresent("AL_EXT_LINEAR_DISTANCE"))
	{
		Com_SetLastError("AL_EXT_LINEAR_DISTANCE extension is missing");
		goto fail1;
	}

	// generate source names
	qalGetError();

	for (i = 0; i < MAX_CHANNELS; i++)
	{
		qalGenSources(1, &s_srcnums[i]);

		if (qalGetError() != AL_NO_ERROR)
			break;
	}

	Com_DPrintf("Got %d AL sources\n", i);

	if (i < MIN_CHANNELS)
	{
		Com_SetLastError("Insufficient number of AL sources");
		goto fail1;
	}

	s_numchannels = i;
	Com_Printf("OpenAL initialized.\n");
	return true;
fail1:
	QAL_Shutdown();
fail0:
	Com_EPrintf("Failed to initialize OpenAL: %s\n", Com_GetLastError());
	return false;
}

void AL_Shutdown(void)
{
	Com_Printf("Shutting down OpenAL.\n");

	if (s_numchannels)
	{
		// delete source names
		qalDeleteSources(s_numchannels, s_srcnums);
		memset(s_srcnums, 0, sizeof(s_srcnums));
		s_numchannels = 0;
	}

	QAL_Shutdown();
}

bool AL_UploadSfx(sfx_t *s, byte **data)
{
	ALenum format = s_info.width == 2 ? AL_FORMAT_MONO16 : AL_FORMAT_MONO8;
	ALuint name;
	int speed;

	if (s_khz->integer == 44)
		speed = 44100;
	else if (s_khz->integer == 22)
		speed = 22050;
	else
		speed = 11025;

	size_t samples;

	if (!S_ResampleSfx(s, speed, data, &samples))
		return false;

	s->length = samples * 1000 / speed; // in msec
	//sc->size = samples * s_info.width;
	qalGetError();
	qalGenBuffers(1, &name);
	qalBufferData(name, format, *data, samples * s_info.width, speed);

	if (qalGetError() != AL_NO_ERROR)
	{
		s->error = Q_ERR_LIBRARY_ERROR;
		return false;
	}

	// specify OpenAL-Soft style loop points
	if (s_info.loopstart > 0 && qalIsExtensionPresent("AL_SOFT_loop_points"))
	{
		ALint points[2] = { s->loopstart, samples };
		qalBufferiv(name, AL_LOOP_POINTS_SOFT, points);
	}

	s->bufnum = name;
	return true;
}

void AL_DeleteSfx(sfx_t *s)
{
	qalDeleteBuffers(1, &s->bufnum);
	s->bufnum = 0;
}

static void AL_Spatialize(channel_t *ch)
{
	vec3_t      origin;

	// anything coming from the view entity will always be full volume
	// no attenuation = no spatialization
	if (ch->entnum == -1 || ch->entnum == listener_entnum || !ch->dist_mult)
		VectorCopy(listener_origin, origin);
	else if (ch->fixed_origin)
		VectorCopy(ch->origin, origin);
	else
		CL_GetEntitySoundOrigin(ch->entnum, origin);

	qalSource3f(ch->srcnum, AL_POSITION, AL_UnpackVector(origin));
}

void AL_StopChannel(channel_t *ch)
{
#ifdef _DEBUG

	if (s_show->integer > 1)
		Com_Printf("%s: %s\n", __func__, ch->sfx->name);

#endif
	// stop it
	qalSourceStop(ch->srcnum);
	qalSourcei(ch->srcnum, AL_BUFFER, AL_NONE);
	memset(ch, 0, sizeof(*ch));
}

static float AL_AdjustPitch(channel_t *ch)
{
	float pitch = ch->pitch_scale;

	if (cl.frame.ps.rdflags & RDF_UNDERWATER)
		pitch /= 1.5;

	return pitch;
}

void AL_PlayChannel(channel_t *ch)
{
#ifdef _DEBUG

	if (s_show->integer > 1)
		Com_Printf("%s: %s\n", __func__, ch->sfx->name);

#endif
	ch->srcnum = s_srcnums[ch - channels];
	qalGetError();
	qalSourcei(ch->srcnum, AL_BUFFER, ch->sfx->bufnum);

	if (ch->autosound || (ch->sfx->loopstart >= 0 && qalIsExtensionPresent("AL_SOFT_loop_points")))
		qalSourcei(ch->srcnum, AL_LOOPING, AL_TRUE);
	else
		qalSourcei(ch->srcnum, AL_LOOPING, AL_FALSE);

	qalSourcef(ch->srcnum, AL_GAIN, ch->master_vol);
	qalSourcef(ch->srcnum, AL_REFERENCE_DISTANCE, SOUND_FULLVOLUME);
	qalSourcef(ch->srcnum, AL_MAX_DISTANCE, 8192);
	qalSourcef(ch->srcnum, AL_ROLLOFF_FACTOR, ch->dist_mult * (8192 - SOUND_FULLVOLUME));
	qalSourcef(ch->srcnum, AL_PITCH, AL_AdjustPitch(ch));
	AL_Spatialize(ch);
	// play it
	qalSourcePlay(ch->srcnum);

	if (qalGetError() != AL_NO_ERROR)
		AL_StopChannel(ch);
}

static void AL_IssuePlaysounds(void)
{
	playsound_t *ps;

	// start any playsounds
	while (1)
	{
		ps = s_pendingplays.next;

		if (ps == &s_pendingplays)
			break;  // no more pending sounds

		if (ps->begin > paintedtime)
			break;

		S_IssuePlaysound(ps);
	}
}

void AL_StopAllChannels(void)
{
	int         i;
	channel_t   *ch;
	ch = channels;

	for (i = 0; i < s_numchannels; i++, ch++)
	{
		if (!ch->sfx)
			continue;

		AL_StopChannel(ch);
	}
}

static channel_t *AL_FindLoopingSound(int entnum, sfx_t *sfx)
{
	int         i;
	channel_t   *ch;
	ch = channels;

	for (i = 0; i < s_numchannels; i++, ch++)
	{
		if (!ch->sfx)
			continue;

		if (!ch->autosound)
			continue;

		if (entnum && ch->entnum != entnum)
			continue;

		if (ch->sfx != sfx)
			continue;

		return ch;
	}

	return NULL;
}

static void AL_AddLoopSounds(void)
{
	int         i;
	q_soundhandle sounds[MAX_EDICTS];
	channel_t   *ch, *ch2;
	sfx_t       *sfx;
	int         num;
	entity_state_t  *ent;

	if (cls.state != ca_active || sv_paused->integer || !s_ambient->integer)
		return;

	S_BuildSoundList(sounds);

	for (i = 0; i < cl.frame.numEntities; i++)
	{
		if (!sounds[i])
			continue;

		sfx = S_SfxForHandle(cl.precache[(uint16_t)sounds[i]].sound, CL_GetClientGame());

		if (!sfx || !sfx->bufnum)
			continue;       // bad sound effect

		num = (cl.frame.firstEntity + i) & PARSE_ENTITIES_MASK;
		ent = &cl.entityStates[num];
		ch = AL_FindLoopingSound(ent->number, sfx);

		if (ch)
		{
			ch->autoframe = s_framecount;
			ch->end = paintedtime + sfx->length;
			continue;
		}

		// allocate a channel
		ch = S_PickChannel(0, 0);

		if (!ch)
			continue;

		ch2 = AL_FindLoopingSound(0, sfx);
		ch->autosound = true;   // remove next frame
		ch->autoframe = s_framecount;
		ch->sfx = sfx;
		ch->entnum = ent->number;
		ch->master_vol = 1;
		ch->dist_mult = SOUND_LOOPATTENUATE;
		ch->end = paintedtime + sfx->length;
		ch->pitch_scale = 1; // TODO
		AL_PlayChannel(ch);

		// attempt to synchronize with existing sounds of the same type
		if (!ch2)
			continue;

		ALint offset;
		qalGetSourcei(ch2->srcnum, AL_SAMPLE_OFFSET, &offset);
		qalSourcei(ch->srcnum, AL_SAMPLE_OFFSET, offset);
	}
}

void AL_Update(void)
{
	int         i;
	channel_t   *ch;
	vec_t       orientation[6];

	if (!s_active)
		return;

	paintedtime = cl.time;
	// set listener parameters
	qalListener3f(AL_POSITION, AL_UnpackVector(listener_origin));
	AL_CopyVector(listener_forward, orientation);
	AL_CopyVector(listener_up, orientation + 3);
	qalListenerfv(AL_ORIENTATION, orientation);
	qalListenerf(AL_GAIN, s_volume->value);
	qalDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
	// update spatialization for dynamic sounds
	ch = channels;

	for (i = 0; i < s_numchannels; i++, ch++)
	{
		if (!ch->sfx)
			continue;

		if (ch->autosound)
		{
			// autosounds are regenerated fresh each frame
			if (ch->autoframe != s_framecount)
			{
				AL_StopChannel(ch);
				continue;
			}
		}
		else
		{
			ALenum state;
			qalGetError();
			qalGetSourcei(ch->srcnum, AL_SOURCE_STATE, &state);

			if (qalGetError() != AL_NO_ERROR || state == AL_STOPPED)
			{
				AL_StopChannel(ch);
				continue;
			}
		}

		// set pitch
		qalSourcef(ch->srcnum, AL_PITCH, AL_AdjustPitch(ch));
#ifdef _DEBUG

		if (s_show->integer)
		{
			Com_Printf("%.1f %s\n", ch->master_vol, ch->sfx->name);
			//    total++;
		}

#endif
		AL_Spatialize(ch);         // respatialize channel
	}

	s_framecount++;
	// add loopsounds
	AL_AddLoopSounds();
	AL_IssuePlaysounds();
}